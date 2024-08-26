/**
 * Author: Jack Robbins
 * This header file contains all of the needed functions and structures for the mempool memory suballocation system
 */

#ifndef MEMPOOL_H
#define MEMPOOL_H

#include <cstddef>
#include <sys/types.h>
#include <stdlib.h>


typedef struct {
	void* region_start;
	void* region_end;
	size_t size;
	size_t used;
} mem_ptr;


/**
 * Initialize the entire memory pool to be of overall size mempool_size bytes
 */
void mempool_init(u_int64_t mempool_size);


/**
 * Free the entire memory pool that was allocated
 *
 * NOTE: This will make all mem_ptrs invalid and prone to segmentation faults. If you are
 * calling this function, be sure you do not have any active mem_ptrs
 */
void mempool_destroy();


/**
 * Allocate num_bytes bytes of memory in the memory pool. 
 *
 * NOTE: The memory that is allocated may contain junk values from previous allocations.
 * If this is an issue, use mempool_calloc
 */
mem_ptr* mempool_alloc(u_int64_t num_bytes);


/**
 * Allocate num_bytes bytes of memory in the memory pool, and then set "n" of those 
 * bytes to be the value of "value". This avoids the need for calling "memset" manually
 */
mem_ptr* mempool_calloc(u_int64_t num_bytes, u_int8_t value, u_int64_t n);


/**
 * Reallocate the memory allocated in mem_ptr to be of new size num_bytes
 *
 * NOTE: num_bytes must be greater than the number of bytes previously allocated to the mem_ptr. If memory
 * smashing is detected, an error will be thrown
 */
mem_ptr* mempool_ralloc(mem_ptr* mem_ptr, u_int64_t num_bytes);


/**
 * Free all of the memory in reserved by the mem_ptr region, and destroy
 * the mem_ptr itself
 */
void mempool_free(mem_ptr* mem_ptr);


#endif /* MEMPOOL_H */
