/**
 * Author: Jack Robbins
 * This file contains the implementation and functionality for mempool as described in 
 * mempool.h
 */

#include "mempool.h"

#include <stdio.h>
#include <stdlib.h>
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

	//Store the block size
	block_size = default_block_size;

	//Determine how many blocks we need to allocated
	u_int64_t num_blocks = size / block_size; 

	//Current block pointer
	struct block* current;
	 
	//Go through and allocate every block
	for(u_int64_t i = 1; i <= num_blocks; i++){
		//Reserve space for the block metadata
		current = (struct block*)malloc(sizeof(struct block));
		//Dynamically allocate the block's space
		current->ptr = malloc(block_size);

		//Attach to the free linked list
		current->next = free_list;
		
		//Set the head to be current
		free_list = current;
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
		//TODO implement coalescence algo
		allocated = NULL;
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

	//Search through the allocated list to find the pointer directly previous to this one
	struct block* previous = allocated_list;

	//SPECIAL CASE: the head of the allocated list is the one we want to free
	if((u_int64_t)(previous->ptr) == (u_int64_t)(mem_ptr)){
		//"Delete" this from the allocated list
		allocated_list = allocated_list->next;

		//Add back onto the free list		
		previous->next = free_list;
		free_list = previous;

	} else {
		//Case -- we are in the middle of the list
		//Keep searching so long as the blocks aren't null
		while(previous->next != NULL && (u_int64_t)(previous->next->ptr) != (u_int64_t)mem_ptr){
			//Advance the pointer
			previous = previous->next;
		}

		//If this somehow happened, the free is invalid
		if(previous == NULL){
			printf("MEMPOOL_ERROR: Attempt to free a nonexistent pointer. Potential double free detected\n");
			return;
		}

		//If we get here, we know that we have the pointer to the block directly preceeding this one
	
		//The block we're going to free
		struct block* freed = previous->next;

		//Remove this from the linked list
		previous->next = freed->next;

		//Add this back onto the free list
		freed->next = free_list;

		//Assign as the head
		free_list = freed;
	}
}


/**
 * Allocated a pointer returned by the memory pool and set "n" bytes of
 * it to be "value"
 */
void* mempool_calloc(u_int64_t num_bytes, u_int8_t value, u_int64_t n){
	//Check if we are trying to memset too much
	if(num_bytes < n){
		printf("MEMPOOL_ERROR: Attempt to set more bytes than allocated\n");
		return NULL;
	}

	//Use mempool_alloc to give us the space
	void* allocated = mempool_alloc(num_bytes);

	//Set n bytes of s to be the value
	memset(allocated, value, n);
	
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
		//Free the current pointer
		free(current->ptr);
		//Save the address of current
		temp = current; 
		//Advance the pointer
		current = current->next;
		//Free the current block stored in temp
		free(temp);
	}
	
	//Set to be null
	free_list = NULL;

	//Now deallocate the entire allocated_list
	current = allocated_list;

	//Walk the list
	while(current != NULL){
		//Free the current pointer
		free(current->ptr);
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
