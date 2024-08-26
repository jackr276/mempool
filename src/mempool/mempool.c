/**
 * Author: Jack Robbins
 * This file contains the implementation and functionality for mempool as described in 
 * mempool.h
 */

#include "mempool.h"
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>


//Overall size of the mempool
static u_int64_t mempool_size;

//The amount of memory currently used
static u_int64_t mempool_used;

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
		printf("Invalid size for memory pool, memory pool will not be initialized\n");
		return -1;
	}

	//Check for default_block_size validity
	if(default_block_size == 0 || default_block_size >= size){
		printf("Invalid default block size. Block size must be strictly less than overall size. Memory pool will not be initialized.\n");
		return -1;
	}

	//Check if a memory pool already exists
	if(free_list != NULL && allocated_list != NULL){
		printf("A memory pool has already been created. If you wish to create a new one, you must first call mempool_destroy()\n");
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
		//Assign an ID for reference
		current->block_id = num_blocks - i;
		//Assign the default block size
		current->block_size = block_size;

		//Dynamically allocate the block's space
		current->ptr = malloc(block_size);

		//Attach to the free linked list
		current->next = free_list;
		
		//Set the head to be current
		free_list = current;
	}

	//Once we get here, every block will have been allocated

	//Let the caller know all went well
	return 1;
}



int mempool_destroy(){
	//Check to make sure there is actually something to destroy
	if(free_list == NULL && allocated_list == NULL){
		printf("No memory pool was ever initialized. Invalid call to destroy.\n");
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
	mempool_used = 0;
	
	//Let the caller know all went well
	return 1;
}
