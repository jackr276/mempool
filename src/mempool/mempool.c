/**
 * Author: Jack Robbins
 * This file contains the implementation and functionality for mempool as described in 
 * mempool.h
 */

#include "mempool.h"
//For our thread safety and mutexes
#include <pthread.h>

/**
 * Define a struct for a block of memory
 */
struct block {
	//------------ Block metadata --------------
	//For the linked list functionality
	struct block* next;
	//The size may change if we coalesce
	u_int32_t size;
	//------------------------------------------
	
	//The pointer that is actually usable
	void* ptr;
};


//Overall size of the mempool
static u_int32_t mempool_size;

//The total bytes currently in use
static u_int32_t mempool_used;

//The default block size
static u_int32_t block_size;

//A list of all free blocks
static struct block* free_list = NULL;

//A list of all allocated blocks
static struct block* allocated_list = NULL;

//For thread safety
static pthread_mutex_t free_mutex;
static pthread_mutex_t allocated_mutex;

//The entire monolithic memory pool
static void* memory_pool = NULL;


/**
 * Initialize the memory pool to be of "size" bytes
 */
int mempool_init(u_int32_t size, u_int32_t default_block_size){
	//Input checking
	if(size <= 0){
		printf("MEMPOOL_ERROR: Invalid size for memory pool, memory pool will not be initialized\n");
		return -1;
	}

	//Check for default_block_size validity
	if(default_block_size == 0 || default_block_size >= size){
		printf("MEMPOOL_ERROR: Invalid default block size. Block size must be strictly less than overall size. Memory pool will not be initialized.\n");
		return -1;
	}

	//Check if a memory pool already exists
	if(free_list != NULL && allocated_list != NULL){
		printf("MEMPOOL_ERROR: A memory pool has already been created. If you wish to create a new one, you must first call mempool_destroy()\n");
		return -1;
	}

	//Allocate the entire monolithic memory pool
	memory_pool = malloc(size);

	//Store the block size
	block_size = default_block_size;
	
	//Determine how many blocks we need to allocated
	u_int32_t num_blocks = size / block_size; 

	//Current block pointer
	struct block* current;

	struct block* free_list_tail = NULL;
	 
	//Go through and allocate every block
	//Note -- these blocks are in order in the memory(block 1's pointer ends and block 2's pointer begins)
	for(u_int32_t offset = 0; offset < num_blocks; offset++){
		//Reserve space for the block metadata
		current = (struct block*)malloc(sizeof(struct block));
		//"Allocate" the memory as an offset of the pool start pointer
		current->ptr = memory_pool + offset * block_size; 
		//Initially everything has the same block size
		current->size = block_size;
		//Set to be NULL
		current->next = NULL;

		//Very first allocation
		if(free_list == NULL){
			//Set the free list head
			free_list = current;
			//Set the tail
			free_list_tail = current;
		} else {
			//Generic case, attach to tail
			free_list_tail->next = current;
			free_list_tail = current;
		}
	}

	//Once we get here, every block will have been allocated
	mempool_size = size;
	mempool_used = 0;

	//Initialize the mutexes
	pthread_mutex_init(&free_mutex,  NULL);
	pthread_mutex_init(&allocated_mutex,  NULL);


	//Let the caller know all went well
	return 1;
}


/**
 * Allocate a block or block(s) of size num_bytes
 * 
 * NOTE: A reminder that this memory allocator gives you the power to choose the block size. If you are consistently allocating
 * chunks of memory that are larger than the block size, you should consider upping the block size on creation.
 */
void* mempool_alloc(u_int32_t num_bytes){
	//Simple error checking but just in case
	if(num_bytes >= mempool_size){
		printf("MEMPOOL_ERROR: Attempt to allocate a number of bytes greater than or equal to the entire memory pool size\n");
		return NULL;
	}
	 
	//Make sure we actually have blocks to give
	if(free_list == NULL){
		printf("MEMPOOL_ERROR: No available memory. You either have a memory leak, or you gave the memory pool too small an amount of memory on creation\n");
		return NULL;
	}

	//This would also lead to an error that we may not catch before if we had to coalesce
	if(mempool_used + num_bytes >= mempool_size){
		printf("MEMPOOL_ERROR: Insufficient available memory. You either have a memory leak, or you gave the memory pool too small an amount of memory on creation\n");
		return NULL;
	}

	//Our allocated block
	struct block* allocated;

	//If this is the case, we don't need to coalesce any blocks. If the user was intelligent about how they chose the block size,
	//this should be the case the majority of the time
	if(num_bytes <= block_size){
		//Lock the free list
		pthread_mutex_lock(&free_mutex);

		//Grab the head
		allocated = free_list;

		//"Delete" this from the free list
		free_list = free_list->next;

		//Unlock the free list
		pthread_mutex_unlock(&free_mutex);

		//Now we'll use the allocated list, so lock it
		pthread_mutex_lock(&allocated_mutex);
		
		//Attach it to the allocated list
		allocated->next = allocated_list;
		
		//modify the allocated list head
		allocated_list = allocated;
		
		//All done so unlock
		pthread_mutex_unlock(&allocated_mutex);

	} else {
		//If we get here, we're going to need to coalesce some blocks to have enough space

		//Figure out how many blocks we need to coalesce
		u_int32_t blocks_needed = num_bytes / block_size + ((num_bytes % block_size == 0) ? 0 : 1);
		
		//We will be modifying the free_list, so lock
		pthread_mutex_lock(&free_mutex);

		//We need contiguous blocks, i.e, their memory addresses have to be one after the other
		//This is not a guarantee in the free list, so we have to search until we find this
		struct block* cursor = free_list;
		//Store the address of the previous pointer 
		u_int64_t previous_address = (u_int64_t)(cursor->ptr);
		//We already have 1 contiguous block by default
		u_int16_t contiguous_blocks = 1;
		//By default, the cursor start is the very start of our continuous blocks
		struct block* contiguous_chunk_head = cursor;

		//Advance the pointer since we already dealt with the first one
		cursor = cursor->next;

		//While the cursor isn't null and we don't have enough blocks
		while(cursor != NULL && contiguous_blocks < blocks_needed){
			//If the difference the cursor's pointer and the previous one is the block size, we have 
			//a contiguous block
			if((u_int64_t)(cursor->ptr) - previous_address == block_size){
				contiguous_blocks++;
			} else {
				//Otherwise, we're back to square one
				contiguous_blocks = 1;
				//We will also mark the cursor as the start of the next contiguous block
				contiguous_chunk_head = cursor;
			}

			//Set the previous_address to be the cursor's address
			previous_address = (u_int64_t)(cursor->ptr);
			//Advance the cursor
			cursor = cursor->next;
		}

		//Once we get here, we either found enough contiguous blocks or not
		//This means we did not find enough blocks in a row
		if(contiguous_blocks < blocks_needed){
			printf("MEMPOOL_ERROR: Unable to allocated block of size %d bytes due to insufficient space.\n"
						   "Either make the mempool larger, or free more space.\n", num_bytes);

			//Unlock before erroring out
			pthread_mutex_unlock(&free_mutex);
			return NULL;
		}

		//If we get here though, this means that we do have enough space to merge "blocks_needed" blocks, starting at the pointer
		//contiguous_chunk_head
		
		//Let's find the tail so that we can easily remove
		struct block* contiguous_chunk_tail = contiguous_chunk_head;

		//First find the tail of the chunk
		for(u_int16_t i = 1; i < contiguous_blocks; i++){
			contiguous_chunk_tail = contiguous_chunk_tail->next;
		}

		//Once we have the tail, we need the block before the contiguous_chunk_head
		//SPECIAL CASE: The chunk_head is the head of the free list
		if((u_int64_t)(free_list->ptr) == (u_int64_t)(contiguous_chunk_head->ptr)){
			//Remove everything from the free list by setting the free_list to be after the tail
			free_list = contiguous_chunk_tail->next;
		} else {
			//Otherwise, we have to traverse to find the block before the head of the contiguous chunk
			struct block* previous = free_list;

			//Keep searching until we have the block directly before the chunk head
			while((u_int64_t)(previous->next->ptr) != (u_int64_t)(contiguous_chunk_head->ptr)){
				previous = previous->next;
			}

			//We now have the block directly before the chunk head, so we'll remove the entire chunk at once
			previous->next = contiguous_chunk_tail->next;
		}

		//Done modifying the free list, so unlock
		pthread_mutex_unlock(&free_mutex);

		//Set this for later
		contiguous_chunk_tail->next = NULL;

		//We've now remove everything from the free list, so all that's left to do is "coalesce"	
		
		//Increase this pointer's size
		contiguous_chunk_head->size *= blocks_needed;

		//Prepare to delete all of the other coalesced blocks
		struct block* needs_freeing = contiguous_chunk_head->next;
		struct block* temp;

		//Now we can just free all of the other blocks, so their memory regions, which no belong to the contiguous_chunk_head, can no longer be used
		//Free all of the blocks blocks after the head
		while(needs_freeing != NULL){
			temp = needs_freeing;
			//Advance the pointer
			needs_freeing = needs_freeing->next;
			//Free the current one
			free(temp);
		}

		//Modifying the allocated_list, so lock
		pthread_mutex_lock(&allocated_mutex);

		//Once we're here, we've successfully coalesced and removed everything, so we can add the head to the allocated_list
		contiguous_chunk_head->next = allocated_list;
		allocated_list = contiguous_chunk_head;

		//All done so unlock
		pthread_mutex_unlock(&allocated_mutex);

		//Set this for our return
		allocated = contiguous_chunk_head;
	}

	//If we get here, we were able to allocated, so increase the bytes used
	mempool_used += allocated->size;
	//Return the allocated block
	return allocated->ptr;
}


/**
 * "Free" the block pointed to by the mem_ptr. This isn't actually a free, all we do here is remove it from the allocated_list 
 * and attach it to the free list
 *
 * Free is thread safe
 */
void mempool_free(void* ptr){
	//Check we aren't freeing a null
	if(ptr == NULL){
		printf("MEMPOOL_ERROR: Attempt to free a null pointer\n");
		return;
	}

	//If nothing was allocated, then we can't free anything
	if(allocated_list == NULL){
		printf("MEMPOOL_ERROR: Attempt to free a nonexistent pointer. Potential double free detected\n");
		return;
	}

	//A reference to the block that we want to free
	struct block* freed;

	//Since we are modifying the allocated list, we need to lock
	pthread_mutex_lock(&allocated_mutex);

	//Search through the allocated list to find the pointer directly previous to this one
	struct block* cursor = allocated_list;

	//SPECIAL CASE: the head of the allocated list is the one we want to free
	if((u_int64_t)(cursor->ptr) == (u_int64_t)(ptr)){
		//Save the guy we want to free
		freed = cursor;
		//"Delete" this from the allocated list
		allocated_list = allocated_list->next;

	} else {
		//Case -- we are in the middle of the list
		//Keep searching so long as the blocks aren't null
		while(cursor->next != NULL && (u_int64_t)(cursor->next->ptr) != (u_int64_t)ptr){
			//Advance the pointer
			cursor = cursor->next;
		}

		//If this somehow happened, the free is invalid
		if(cursor == NULL){
			printf("MEMPOOL_ERROR: Attempt to free a nonexistent pointer. Potential double free detected\n");
			return;
		}

		//If we get here, we know that we have the pointer to the block directly preceeding this one
	
		//The block we're going to free
		freed = cursor->next;

		//Remove this from the allocated list
		cursor->next = freed->next;
	}

	//Now we're done with the allocated list, so we can unlock it for someone else to use
	pthread_mutex_unlock(&allocated_mutex);

	//The "tail" of the freed allocation
	struct block* freed_tail;

	//The bytes we will free
	u_int32_t bytes_freed = freed->size;

	//If the block size is equal, then freed and freed_tail are the same thing
	if(freed->size == block_size){
		freed_tail = freed;
	} else {
		//The number of blocks that we'll need to make
		u_int32_t num_blocks = freed->size / block_size;

		//Freed will be our first block, and we need to reduce his size to the block size
		freed->size = block_size;

		//This means that we have a coalesced block, so we're going to need to "uncoalesce" it
		struct block* intermediate = freed;

		//We now need to make new blocks
		for(u_int16_t i = 1; i < num_blocks; i++){
			//Create a new block
			struct block* block = (struct block*)malloc(sizeof(struct block));
			block->size = block_size;
			//Assign the pointer to have the appropraite offset
			block->ptr = freed->ptr + block_size * i;
			
			//Attach to the linked list
			intermediate->next = block;
			intermediate = block;
		}

		//The very last intermediary is the tail
		freed_tail = intermediate;
	}

	//We will be modifying the free list, so we need to lock it
	pthread_mutex_lock(&free_mutex);

	//We now need to strategically add this back onto the free list. We want the free list to be as in order as possible according to
	//the memory addresses of the blocks, in case we ever need to coalesce blocks
	cursor = free_list;

	//Special case -- insert at head if the head's address is higher than the freeds
	if((u_int64_t)(cursor->ptr) > (u_int64_t)(freed->ptr)){
		//Insert at head
		freed_tail->next = free_list;
		free_list = freed;

	} else {
		//We want to keep going until the cursor's next pointer is not less than the freed's pointer
		while(cursor->next != NULL && (u_int64_t)(cursor->next->ptr) < (u_int64_t)(freed->ptr)){
			cursor = cursor->next;
		}

		//If we get here, the cursor's memory address is less than the freed's, but the one after the cursor is not, so we need to insert the freed
		//inbetween the cursor and what comes after it
	
		//Save the cursor's next 
		struct block* temp = cursor->next;

		//Insert freed inbetween
		cursor->next = freed;
		freed_tail->next = temp;
	}

	//All done so unlock
	pthread_mutex_unlock(&free_mutex);

	//Record that we freed these bytes
	mempool_used -= bytes_freed;
}


/**
 * Allocate num_members members of size size each, all set to 0
 */
void* mempool_calloc(u_int32_t num_members, size_t size){
	//Check if we are trying to memset too much
	if(num_members * size == 0){
		printf("MEMPOOL_ERROR: Attempt to allocate 0 bytes\n");
		return NULL;
	}

	//Use mempool_alloc to give us the space
	void* allocated = mempool_alloc(num_members * size);

	//Set all to be 0	
	memset(allocated, 0, num_members * size);
	
	//Return the allocated pointer
	return allocated;
}


/**
 * Reallocate the pointer and copy over the contents of the previous pointer to the new memory space
 */
void* mempool_realloc(void* ptr, u_int32_t num_bytes){
	//If the allocated list is NULL, we can't realloc anything
	if(allocated_list == NULL){
		printf("MEMPOOL_ERROR: Nothing from the mempool was allocated, realloc is impossible\n");
		return NULL;
	}

	//If the pointer is NULL, error out
	if(ptr == NULL){
		printf("MEMPOOL_ERROR: Attempt to realloc a null pointer. Potential use after free detected.\n");
		return NULL;
	}

	//Just in case a mistake like this happens...
	if(num_bytes == 0){
		printf("MEMPOOL_ERROR: Attempt to realloc with size of 0 bytes. Invalid input.\n");
	}

	//We will be searching through a list, so be sure to lock
	pthread_mutex_lock(&allocated_mutex);

	//We first need to find this value in the allocated list
	struct block* cursor = allocated_list;
	struct block* realloc_target = NULL;

	//Keep searching through the list until we find what we're looking for
	while(cursor != NULL){
		//If we find what we're looking for, leave the loop
		if((u_int64_t)(cursor->ptr) == (u_int64_t)(ptr)){
			realloc_target = cursor;
			break;
		}
		
		//If we didn't find what we're after, advance the pointer
		cursor = cursor->next;
	}

	//Unlock when done
	pthread_mutex_unlock(&allocated_mutex);

	//If the target is still NULL, that means we didn't find the block
	if(realloc_target == NULL){
		printf("MEMPOOL_ERROR: Attempt to realloc a nonexistent pointer. Potential use after free detected\n");
		return NULL;
	}

	//The user may have inadvertently tried to realloc when there already is enough space in the block. If this is the
	//case, just return the pointer they gave without any fanfare
	if(realloc_target->size <= num_bytes){
		return ptr;
	}

	//Otherwise, we actually do need to resize the entire thing
	//Allocate fresh space
	void* reallocated = mempool_alloc(num_bytes);

	//Copy over the entirety of the contents into the new pointer
	memcpy(reallocated, ptr, realloc_target->size);

	//Now that we have all of the contents copied over, we can free the old pointer
	mempool_free(ptr);

	//Return the realloc'd pointer
	return reallocated;
}


/**
 * Deallocate everything in our mempool. 
 *  
 * NOTE: This is a completely destructive process. Everything block allocated will be deallocated.
 */
int mempool_destroy(){
	//Check to make sure there is actually something to destroy
	if(free_list == NULL && allocated_list == NULL){
		printf("MEMPOOL_ERROR: No memory pool was ever initialized. Invalid call to destroy.\n");
		return -1;
	}

	//First deallocate the entire free list
	struct block* current = free_list;
	struct block* temp;

	//Walk the list
	while(current != NULL){
		//Save the address of current
		temp = current; 
		//Advance the pointer
		current = current->next;
		//Free the current block stored in temp
		//TESTING
		free(temp);
	}
	
	//Set to be null
	free_list = NULL;

	//Now deallocate the entire allocated_list
	current = allocated_list;

	//Walk the list
	while(current != NULL){
		//Save the address of current
		temp = current; 
		//Advance the pointer
		current = current->next;
		//Free the current block stored in temp
		free(temp);
	}

	//Set to be null
	allocated_list = NULL;

	//Free the entire mempool
	free(memory_pool);

	//Reset these values
	mempool_size = 0;
	mempool_used = 0;

	//Destroy the mutexes
	pthread_mutex_destroy(&free_mutex);
	pthread_mutex_destroy(&allocated_mutex);
	
	//Let the caller know all went well
	return 1;
}
