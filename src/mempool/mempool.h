/**
 * Author: Jack Robbins
 * This header file contains all of the needed functions and structures for the mempool memory suballocation system
 */

#ifndef MEMPOOL_H
#define MEMPOOL_H

#include <sys/types.h>
#include <stdlib.h>
#include <stdio.h>


//For user convenience
#define KILOBYTE 1024
#define MEGABYTE 1048576
#define GIGABYTE 1073741824

/**
 * Define a struct for a block of memory
 */
struct block {
	//------------ Block metadata --------------
	u_int64_t block_id;	
	//For the linked list functionality
	struct block* next;
	//------------------------------------------
	
	//The pointer that is actually usable
	void* ptr;
};


/**
 * Initialize the entire memory pool to be of overall size of "size" bytes, with chunks of size
 * default_block_size
 *
 * NOTE: It is up to the caller to intelligently determine what an appropriate defualt block size is. Using too small
 * a default size leads to excessive coalescing, and too large a size leads to wasted memory
 */
int mempool_init(u_int64_t size, u_int64_t default_block_size);


/**
 * Free the entire memory pool that was allocated
 *
 * NOTE: This will make all mem_ptrs invalid and prone to segmentation faults. If you are
 * calling this function, be sure you do not have any active mem_ptrs
 */
int mempool_destroy();


/**
 * Allocate num_bytes bytes of memory in the memory pool. 
 *
 * NOTE: The memory that is allocated may contain junk values from previous allocations.
 * If this is an issue, use mempool_calloc
 */
struct block* mempool_alloc(u_int64_t num_bytes);


/**
 * Allocate num_bytes bytes of memory in the memory pool, and then set "n" of those 
 * bytes to be the value of "value". This avoids the need for calling "memset" manually
 */
struct block* mempool_calloc(u_int64_t num_bytes, u_int8_t value, u_int64_t n);


/**
 * Reallocate the memory allocated in mem_ptr to be of new size num_bytes
 *
 * NOTE: num_bytes must be greater than the number of bytes previously allocated to the mem_ptr. If memory
 * smashing is detected, an error will be thrown
 */
struct block* mempool_ralloc(struct block* mem_ptr, u_int64_t num_bytes);


/**
 * Free all of the memory in reserved by the mem_ptr region, and destroy
 * the mem_ptr itself
 */
void mempool_free(struct block* mem_ptr);


#endif /* MEMPOOL_H */
