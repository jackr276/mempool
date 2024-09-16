/**
 * Author: Jack Robbins
 * This file contains the implementation and functionality for mempool as described in 
 * mempool.h
 */

#include "mempool.h"
//For our thread safety and mutexes
#include <pthread.h>

/**
 * Initialize the memory pool to be of "size" bytes
 */
mempool_t* mempool_init(u_int32_t size, u_int32_t default_block_size, mempool_thread_safety_t thread_safe){
	//Input checking
	if(size <= 0){
		printf("MEMPOOL_ERROR: Invalid size for memory pool, memory pool will not be initialized\n");
		return NULL;
	}

	//Check for default_block_size validity
	if(default_block_size == 0 || default_block_size >= size){
		printf("MEMPOOL_ERROR: Invalid default block size. Block size must be strictly less than overall size. Memory pool will not be initialized.\n");
		return NULL;
	}

	//Allocate the mempool
	mempool_t* mempool = (mempool_t*)malloc(sizeof(mempool_t));

	mempool->num_coalesced = 0;
	mempool->thread_safe = thread_safe;

	//Allocate the entire monolithic memory pool
	mempool->memory_pool_original = malloc(size);

	//Align the memory pool
	u_int64_t alignment = (u_int64_t)(mempool->memory_pool_original) % 8;
	mempool->memory_pool_aligned = mempool->memory_pool_original + alignment;

	//Store the block size
 	mempool->block_size = default_block_size + default_block_size % 8;
	
	//Determine how many blocks we need to allocated
	u_int32_t num_blocks = size / mempool->block_size; 

	//Current block pointer
	register struct mem_block_t* current;

	register struct mem_block_t* free_list_tail = NULL;
	 
	//Go through and allocate every block
	//Note -- these blocks are in order in the memory(block 1's pointer ends and block 2's pointer begins)
	for(u_int32_t offset = 0; offset < num_blocks; offset++){
		//Reserve space for the block metadata
		current = (struct mem_block_t*)malloc(sizeof(struct mem_block_t));
		//"Allocate" the memory as an offset of the pool start pointer
		current->ptr = mempool->memory_pool_aligned + offset * mempool->block_size; 
		//Initially everything has the same block size
		current->size = mempool->block_size;
		//Set to be NULL
		current->next = NULL;

		//Very first allocation
		if(mempool->free_list == NULL){
			//Set the free list head
			mempool->free_list = current;
			//Set the tail
			free_list_tail = current;
		} else {
			//Generic case, attach to tail
			free_list_tail->next = current;
			free_list_tail = current;
		}
	}

	//Once we get here, every block will have been allocated
	mempool->mempool_size = size;

	//If thread safety was required
	if(thread_safe == THREAD_SAFE_REQ){
		//Initialize the mutexes
		pthread_mutex_init(&(mempool->free_mutex),  NULL);
		pthread_mutex_init(&(mempool->allocated_mutex),  NULL);
	}
	
	//Let the caller know all went well
	return mempool;
}


/**
 * Allocate a block or block(s) of size num_bytes
 * 
 * NOTE: A reminder that this memory allocator gives you the power to choose the block size. If you are consistently allocating
 * chunks of memory that are larger than the block size, you should consider upping the block size on creation.
 */
void* mempool_alloc(mempool_t* mempool, u_int32_t num_bytes){
	//Make sure we actually have blocks to give
	if(mempool->free_list == NULL){
		printf("MEMPOOL_ERROR: No available memory. You either have a memory leak, or you gave the memory pool too small an amount of memory on creation\n");
		return NULL;
	}

	//Keep this local, avoids the need to compare constantly
	register u_int8_t thread_safe_req = mempool->thread_safe == THREAD_SAFE_REQ;

	//Our allocated block
	register struct mem_block_t* allocated;

	//If this is the case, we don't need to coalesce any blocks. If the user was intelligent about how they chose the block size,
	//this should be the case the majority of the time
	if(num_bytes <= mempool->block_size){
		//Lock the free list
		if(thread_safe_req){
			pthread_mutex_lock(&(mempool->free_mutex));
		}

		//Grab the head
		allocated = mempool->free_list;

		//"Delete" this from the free list
		mempool->free_list = mempool->free_list->next;

		//Unlock the free list
		if(thread_safe_req){
			pthread_mutex_unlock(&(mempool->free_mutex));
			//Now we'll use the allocated list, so lock it
			pthread_mutex_lock(&(mempool->allocated_mutex));
		}

		
		//Attach it to the allocated list
		allocated->next = mempool->allocated_list;
		
		//modify the allocated list head
		mempool->allocated_list = allocated;
		
		//All done so unlock
		if(thread_safe_req){
			pthread_mutex_unlock(&(mempool->allocated_mutex));
		}

	} else {
		(mempool->num_coalesced)++;
		//If we get here, we're going to need to coalesce some blocks to have enough space

		//Figure out how many blocks we need to coalesce
		u_int32_t blocks_needed = num_bytes / mempool->block_size + ((num_bytes % mempool->block_size == 0) ? 0 : 1);
		
		//We will be modifying the free_list, so lock
		if(thread_safe_req){
			pthread_mutex_lock(&(mempool->free_mutex));
		}

		//We need contiguous blocks, i.e, their memory addresses have to be one after the other
		//This is not a guarantee in the free list, so we have to search until we find this
		register struct mem_block_t* cursor = (mempool->free_list);
		//Store the address of the previous pointer 
		u_int64_t previous_address = (u_int64_t)(cursor->ptr);
		//We already have 1 contiguous block by default
		u_int16_t contiguous_blocks = 1;
		//By default, the cursor start is the very start of our continuous blocks
		register struct mem_block_t* contiguous_chunk_head = cursor;

		//Advance the pointer since we already dealt with the first one
		cursor = cursor->next;

		//While the cursor isn't null and we don't have enough blocks
		while(cursor != NULL && contiguous_blocks < blocks_needed){
			//If the difference the cursor's pointer and the previous one is the block size, we have 
			//a contiguous block
			if((u_int64_t)(cursor->ptr) - previous_address == mempool->block_size){
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
			pthread_mutex_unlock(&(mempool->free_mutex));
			return NULL;
		}

		//If we get here though, this means that we do have enough space to merge "blocks_needed" blocks, starting at the pointer
		//contiguous_chunk_head
		
		//Let's find the tail so that we can easily remove
		register struct mem_block_t* contiguous_chunk_tail = contiguous_chunk_head;

		//First find the tail of the chunk
		for(u_int16_t i = 1; i < contiguous_blocks; i++){
			contiguous_chunk_tail = contiguous_chunk_tail->next;
		}

		//Once we have the tail, we need the block before the contiguous_chunk_head
		//SPECIAL CASE: The chunk_head is the head of the free list
		if((u_int64_t)(mempool->free_list->ptr) == (u_int64_t)(contiguous_chunk_head->ptr)){
			//Remove everything from the free list by setting the free_list to be after the tail
			mempool->free_list = contiguous_chunk_tail->next;
		} else {
			//Otherwise, we have to traverse to find the block before the head of the contiguous chunk
			register struct mem_block_t* previous = mempool->free_list;

			//Keep searching until we have the block directly before the chunk head
			while((u_int64_t)(previous->next->ptr) != (u_int64_t)(contiguous_chunk_head->ptr)){
				previous = previous->next;
			}

			//We now have the block directly before the chunk head, so we'll remove the entire chunk at once
			previous->next = contiguous_chunk_tail->next;
		}

		//Done modifying the free list, so unlock
		if(thread_safe_req){
			pthread_mutex_unlock(&(mempool->free_mutex));
		}

		//Set this for later
		contiguous_chunk_tail->next = NULL;

		//We've now remove everything from the free list, so all that's left to do is "coalesce"	
		
		//Increase this pointer's size
		contiguous_chunk_head->size *= blocks_needed;

		//Prepare to delete all of the other coalesced blocks
		register struct mem_block_t* needs_freeing = contiguous_chunk_head->next;
		register struct mem_block_t* temp;

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
		if(thread_safe_req){
			pthread_mutex_lock(&(mempool->allocated_mutex));
		}

		//Once we're here, we've successfully coalesced and removed everything, so we can add the head to the allocated_list
		contiguous_chunk_head->next = mempool->allocated_list;
		mempool->allocated_list = contiguous_chunk_head;

		//All done so unlock
		if(thread_safe_req){
			pthread_mutex_unlock(&(mempool->allocated_mutex));
		}

		//Set this for our return
		allocated = contiguous_chunk_head;
	}

	//Return the allocated block
	return allocated->ptr;
}


/**
 * "Free" the block pointed to by the mem_ptr. This isn't actually a free, all we do here is remove it from the allocated_list 
 * and attach it to the free list
 *
 * Free is thread safe
 */
void mempool_free(mempool_t* mempool, void* ptr){
	//Check we aren't freeing a null
	if(ptr == NULL){
		printf("MEMPOOL_ERROR: Attempt to free a null pointer\n");
		return;
	}

	//If nothing was allocated, then we can't free anything
	if(mempool->allocated_list == NULL){
		printf("MEMPOOL_ERROR: Attempt to free a nonexistent pointer. Potential double free detected\n");
		return;
	}

	//Keep this local, avoids the need to compare constantly
	register u_int8_t thread_safe_req = mempool->thread_safe == THREAD_SAFE_REQ;

	//A reference to the block that we want to free
	register struct mem_block_t* freed;

	//Since we are modifying the allocated list, we need to lock
	if(thread_safe_req){
		pthread_mutex_lock(&(mempool->allocated_mutex));
	}

	//Search through the allocated list to find the pointer directly previous to this one
	register struct mem_block_t* cursor = mempool->allocated_list;

	//SPECIAL CASE: the head of the allocated list is the one we want to free
	if((u_int64_t)(cursor->ptr) == (u_int64_t)(ptr)){
		//Save the guy we want to free
		freed = cursor;
		//"Delete" this from the allocated list
		mempool->allocated_list = mempool->allocated_list->next;

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
			//Unlock before returning
			pthread_mutex_unlock(&(mempool->allocated_mutex));
			return;
		}

		//If we get here, we know that we have the pointer to the block directly preceeding this one
	
		//The block we're going to free
		freed = cursor->next;

		//Remove this from the allocated list
		cursor->next = freed->next;
	}

	//Now we're done with the allocated list, so we can unlock it for someone else to use
	if(thread_safe_req){
		pthread_mutex_unlock(&(mempool->allocated_mutex));
	}

	//The "tail" of the freed allocation
	register struct mem_block_t* freed_tail;

	//If the block size is equal, then freed and freed_tail are the same thing
	if(freed->size == mempool->block_size){
		freed_tail = freed;
	} else {
		//The number of blocks that we'll need to make
		u_int32_t num_blocks = freed->size / mempool->block_size;

		//Freed will be our first block, and we need to reduce his size to the block size
		freed->size = mempool->block_size;

		//This means that we have a coalesced block, so we're going to need to "uncoalesce" it
		register struct mem_block_t* intermediate = freed;

		//We now need to make new blocks
		for(u_int32_t i = 1; i < num_blocks; i++){
			//Create a new block
			register struct mem_block_t* block = (struct mem_block_t*)malloc(sizeof(struct mem_block_t));
			block->size = mempool->block_size;
			//Assign the pointer to have the appropraite offset
			block->ptr = freed->ptr + mempool->block_size * i;
			
			//Attach to the linked list
			intermediate->next = block;
			intermediate = block;
		}

		//The very last intermediary is the tail
		freed_tail = intermediate;
	}

	//We will be modifying the free list, so we need to lock it
	if(thread_safe_req){
		pthread_mutex_lock(&(mempool->free_mutex));
	}

	//We now need to strategically add this back onto the free list. We want the free list to be as in order as possible according to
	//the memory addresses of the blocks, in case we ever need to coalesce blocks
	cursor = mempool->free_list;

	//Special case -- insert at head if the head's address is higher than the freeds
	if((u_int64_t)(cursor->ptr) > (u_int64_t)(freed->ptr)){
		//Insert at head
		freed_tail->next = mempool->free_list;
		mempool->free_list = freed;

	} else {
		//We want to keep going until the cursor's next pointer is not less than the freed's pointer
		while(cursor->next != NULL && (u_int64_t)(cursor->next->ptr) < (u_int64_t)(freed->ptr)){
			cursor = cursor->next;
		}

		//If we get here, the cursor's memory address is less than the freed's, but the one after the cursor is not, so we need to insert the freed
		//inbetween the cursor and what comes after it
	
		//Save the cursor's next 
		register struct mem_block_t* temp = cursor->next;

		//Insert freed inbetween
		cursor->next = freed;
		freed_tail->next = temp;
	}

	//All done so unlock
	if(thread_safe_req){
		pthread_mutex_unlock(&(mempool->free_mutex));
	}
}


/**
 * Allocate num_members members of size size each, all set to 0
 */
void* mempool_calloc(mempool_t* mempool, u_int32_t num_members, size_t size){
	//Check if we are trying to memset too much
	if(num_members * size == 0){
		printf("MEMPOOL_ERROR: Attempt to allocate 0 bytes\n");
		return NULL;
	}

	//Use mempool_alloc to give us the space
	void* allocated = mempool_alloc(mempool, num_members * size);

	//Set all to be 0	
	memset(allocated, 0, num_members * size);
	
	//Return the allocated pointer
	return allocated;
}


/**
 * Reallocate the pointer and copy over the contents of the previous pointer to the new memory space
 */
void* mempool_realloc(mempool_t* mempool, void* ptr, u_int32_t num_bytes){
	//If the allocated list is NULL, we can't realloc anything
	if(mempool->allocated_list == NULL){
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

	//Keep this local, avoids the need to compare constantly
	register u_int8_t thread_safe_req = mempool->thread_safe == THREAD_SAFE_REQ;

	//We will be searching through a list, so be sure to lock
	if(thread_safe_req){
		pthread_mutex_lock(&(mempool->allocated_mutex));
	}

	//We first need to find this value in the allocated list
	register struct mem_block_t* cursor = mempool->allocated_list;
	register struct mem_block_t* realloc_target = NULL;

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
	if(thread_safe_req){
		pthread_mutex_unlock(&(mempool->allocated_mutex));
	}

	//If the target is still NULL, that means we didn't find the block
	if(realloc_target == NULL){
		printf("MEMPOOL_ERROR: Attempt to realloc a nonexistent pointer. Potential use after free detected\n");
		return NULL;
	}

	//The user may have inadvertently tried to realloc when there already is enough space in the block. If this is the
	//case, just return the pointer they gave without any fanfare
	if(realloc_target->size >= num_bytes){
		return ptr;
	}

	//Otherwise, we actually do need to resize the entire thing
	//Allocate fresh space
	void* reallocated = mempool_alloc(mempool, num_bytes);

	//Copy over the entirety of the contents into the new pointer
	memcpy(reallocated, ptr, realloc_target->size);

	//Now that we have all of the contents copied over, we can free the old pointer
	mempool_free(mempool, ptr);

	//Return the realloc'd pointer
	return reallocated;
}


/**
 * Deallocate everything in our mempool. 
 *  
 * NOTE: This is a completely destructive process. Everything block allocated will be deallocated.
 */
int mempool_destroy(mempool_t* mempool){
	//Check to make sure there is actually something to destroy
	if(mempool->free_list == NULL && mempool->allocated_list == NULL){
		printf("MEMPOOL_ERROR: No memory pool was ever initialized. Invalid call to destroy.\n");
		return -1;
	}

	//First deallocate the entire free list
	register struct mem_block_t* current = mempool->free_list;
	register struct mem_block_t* temp;

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
	mempool->free_list = NULL;

	//Now deallocate the entire allocated_list
	current = mempool->allocated_list;

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
	mempool->allocated_list = NULL;

	//Free the entire mempool
	free(mempool->memory_pool_original);

	//Destroy the mutexes
	if(mempool->thread_safe == THREAD_SAFE_REQ){
		pthread_mutex_destroy(&(mempool->free_mutex));
		pthread_mutex_destroy(&(mempool->allocated_mutex));
	}

	//Free the overall mempool pointer
	free(mempool);

	//Let the caller know all went well
	return 1;
}
