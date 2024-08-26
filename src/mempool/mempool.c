/**
 * Author: Jack Robbins
 * This file contains the implementation and functionality for mempool as described in 
 * mempool.h
 */

#include "mempool.h"
#include <stdio.h>

//Overall size of the mempool
static u_int64_t mempool_size;

//The amount of memory currently used
static u_int64_t mempool_used;

//The pointer to the start of the mempool region
static void* mempool_start = NULL;


/**
 * Initialize the memory pool to be of "size" bytes
 */
int mempool_init(u_int64_t size){
	//Input checking
	if(size == 0){
		printf("Invalid size for memory pool, memory pool will not be initialized\n");
		return -1;
	}

	//Check if a memory pool already exists
	if(mempool_start != NULL){
		printf("A memory pool has already been created. If you wish to create a new one, you must first call mempool_destroy()\n");
		return -1;
	}

	//Allocate our mempool
	mempool_start = malloc(size);

	//Set the size and initially used values
	mempool_size = size;
	mempool_used = 0;

	//Let the caller know all went well
	return 0;
}



int mempool_destroy(){
	//Check to make sure there is actually something to destroy
	if(mempool_start == NULL){
		printf("No memory pool was ever initialized. Invalid call to destroy.\n");
		return -1;
	}

	//Deallocate the mempool
	free(mempool_start);
	mempool_start = NULL;
	
	//Reset these values
	mempool_size = 0;
	mempool_used = 0;
	
	//Let the caller know all went well
	return 0;
}
