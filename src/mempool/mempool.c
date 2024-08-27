/**
 * Author: Jack Robbins
 * This file contains the implementation and functionality for mempool as described in 
 * mempool.h
 */

#include "mempool.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>

/**
 * Define a struct for a block of memory
 */
struct block {
	//------------ Block metadata --------------
	//For the linked list functionality
	struct block* next;
	//------------------------------------------
	
	//The pointer that is actually usable
	void* ptr;
};


//Overall size of the mempool
static u_int64_t mempool_size;

//The default block size
static u_int64_t block_size;

//A list of all free blocks
static struct block* free_list = NULL;

//A list of all allocated blocks
static struct block* allocated_list = NULL;

//The entire monolithic memory pool
static void* memory_pool = NULL;


/**
 * Initialize the memory pool to be of "size" bytes
 */
int mempool_init(u_int64_t size, u_int64_t default_block_size){
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
	u_int64_t num_blocks = size / block_size; 

	//Current block pointer
	struct block* current;

	struct block* free_list_tail = NULL;
	 
	//Go through and allocate every block
	//Note -- these blocks are in order in the memory(block 1's pointer ends and block 2's pointer begins)
	for(u_int64_t offset = 0; offset < num_blocks; offset++){
		//Reserve space for the block metadata
		current = (struct block*)malloc(sizeof(struct block));
		//"Allocate" the memory as an offset of the pool start pointer
		current->ptr = memory_pool + offset * block_size; 
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

	//Let the caller know all went well
	return 1;
}


/**
 * Allocate a block or block(s) of size num_bytes
 * 
 * NOTE: A reminder that this memory allocator gives you the power to choose the block size. If you are consistently allocating
 * chunks of memory that are larger than the block size, you should consider upping the block size on creation.
 */
void* mempool_alloc(u_int64_t num_bytes){
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

	//Our allocated block
	struct block* allocated;

	//If this is the case, we don't need to coalesce any blocks. If the user was intelligent about how they chose the block size,
	//this should be the case the majority of the time
	if(num_bytes <= block_size){
		//Grab the head
		allocated = free_list;

		//"Delete" this from the free list
		free_list = free_list->next;

		//Attach it to the allocated list
		allocated->next = allocated_list;
		
		//modify the allocated list head
		allocated_list = allocated;

	} else {
		//If we get here, we're going to need to coalesce some blocks to have enough space

		//Figure out how many blocks we need to coalesce
		u_int32_t blocks_needed = num_bytes / block_size + ((num_bytes % block_size == 0) ? 0 : 1);
		
		
		//We need to grab this many blocks off of the free list
		u_int32_t i = 0;

		//So long as we need more blocks and we have stuff on the free list
		while(i < blocks_needed && free_list != NULL){
			
			i++;
		}
		
	}

	//Return the allocated block
	return allocated->ptr;
}


/**
 * "Free" the block pointed to by the mem_ptr. This isn't actually a free, all we do here is remove it from the allocated_list 
 * and attach it to the free list
 */
void mempool_free(void* mem_ptr){
	//Check we aren't freeing a null
	if(mem_ptr == NULL){
		printf("MEMPOOL_ERROR: Attempt to free a null pointer\n");
		return;
	}

	if(allocated_list == NULL){
		printf("MEMPOOL_ERROR: Attempt to free a nonexistent pointer. Potential double free detected\n");
		return;
	}

	//A reference to the block that we want to free
	struct block* freed;

	//Search through the allocated list to find the pointer directly previous to this one
	struct block* cursor = allocated_list;

	//SPECIAL CASE: the head of the allocated list is the one we want to free
	if((u_int64_t)(cursor->ptr) == (u_int64_t)(mem_ptr)){
		//Save the guy we want to free
		freed = cursor;
		//"Delete" this from the allocated list
		allocated_list = allocated_list->next;

	} else {
		//Case -- we are in the middle of the list
		//Keep searching so long as the blocks aren't null
		while(cursor->next != NULL && (u_int64_t)(cursor->next->ptr) != (u_int64_t)mem_ptr){
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

	//We now need to strategically add this back onto the free list. We want the free list to be as in order as possible according to
	//the memory addresses of the blocks, in case we ever need to coalesce blocks
	cursor = free_list;

	//Special case -- insert at head if the head's address is higher than the freeds
	if((u_int64_t)(cursor->ptr) > (u_int64_t)(freed->ptr)){
		//Insert at head
		freed->next = free_list;
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
		freed->next = temp;
	}
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
		printf("%p\n", current->ptr);
		//Save the address of current
		temp = current; 
		//Advance the pointer
		current = current->next;
		//Free the current block stored in temp
		//TESTING
		free(temp);
	}

	printf("\n");
	
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

	//Reset these values
	mempool_size = 0;
	
	//Let the caller know all went well
	return 1;
}
