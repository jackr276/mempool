/**
 * Author: Jack Robbins
 * This header file contains all of the needed functions and structures for the mempool memory suballocation system
 */

#ifndef MEMPOOL_H
#define MEMPOOL_H

#include <sys/types.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>


//For user convenience
#define KILOBYTE 1024
#define MEGABYTE 1048576
#define GIGABYTE 1073741824

//A struct for our mempool
typedef struct mempool_t mempool_t;
typedef struct mem_block_t mem_block_t;

/**
 * Define a struct for a block of memory
 */
struct mem_block_t {
	//The pointer that is actually usable
	void* ptr;

	//------------ Block metadata --------------
	//For the linked list functionality
	struct mem_block_t* next;
	//The size may change if we coalesce
	u_int32_t size;
	//------------------------------------------
};


/**
 * A struct that holds all data needed for a mempool. This will be passed through
 * all mempool method
 */
struct mempool_t {
	//The size of the mempool: Max-size: 2^32 - 1 bytes
	u_int32_t mempool_size;

	//The default block size of the mempool
	u_int32_t block_size;

	//A list of all free blocks
	mem_block_t* free_list;
	
	//A list of all allocated blocks
	mem_block_t* allocated_list;

	//Keep track of coalesced blocks
	u_int32_t num_coalesced;
	
	//For thread safety
	pthread_mutex_t free_mutex;
	pthread_mutex_t allocated_mutex;

	//The entire monolithic memory pool
	void* memory_pool_original;
	void* memory_pool_aligned;
};


/**
 * Initialize the entire memory pool to be of overall size of "size" bytes, with chunks of size
 * default_block_size
 *
 * NOTE: It is up to the caller to intelligently determine what an appropriate defualt block size is. Using too small
 * a default size leads to excessive coalescing, and too large a size leads to wasted memory
 *
 * THREAD_SAFE: NO
 */
mempool_t* mempool_init(u_int32_t size, u_int32_t default_block_size);


/**
 * Free the entire memory pool that was allocated
 *
 * NOTE: This will make all mem_ptrs invalid and prone to segmentation faults. If you are
 * calling this function, be sure you do not have any active mem_ptrs
 *
 * THREAD_SAFE: NO
 */
int mempool_destroy(mempool_t* mempool);


/**
 * Allocate num_bytes bytes of memory in the memory pool. 
 *
 * NOTE: The memory that is allocated may contain junk values from previous allocations.
 * If this is an issue, use mempool_calloc for a clean wipe
 *
 * NOTE: A reminder that this memory allocator gives you the power to choose the block size. If you are consistently allocating
 * chunks of memory that are larger than the block size, you should consider upping the block size on creation.
 */
void* mempool_alloc(mempool_t* mempool, u_int32_t num_bytes);


/**
 * Allocate num_members elements of size size each, all initialized to be 0 
 */
void* mempool_calloc(mempool_t* mempool, u_int32_t num_members, size_t size);


/**
 * Reallocate the memory allocated in mem_ptr to be of new size num_bytes
 *
 * NOTE: num_bytes must be greater than the number of bytes previously allocated to the mem_ptr. If memory
 * smashing is detected, an error will be thrown
 */
void* mempool_realloc(mempool_t* mempool, void* ptr, u_int32_t num_bytes);


/**
 * Free all of the memory in reserved by the mem_ptr region, and destroy
 * the mem_ptr itself
 * THREAD_SAFE: YES
 */
void mempool_free(mempool_t* mempool, void* ptr);


#endif /* MEMPOOL_H */
