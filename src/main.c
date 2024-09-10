/**
 * Author: Jack Robbins This tester/demo program demonstrates a simple way of using mempool
 */

//Include the header file
#include "./mempool/mempool.h"
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>

/**
 * A sample struct for us to use in testing.
 */
typedef struct {
	int array[2];
	double d;
} mempool_sample_struct_t;



/**
 * A whole demo program that shows the use of all of the mempool API functions
 */
int main(){
	printf("Testing Mempool\n");
	
	printf("Initializing a memory pool of size 500KB\n");

	/**
	 * Initialization
	 *
	 * Since we will be storing mempool_sample_struct_t's I want to make the block size the 
	 * size of that struct. Remember, the entire point of using this tool is to allow you to choose
	 * the block size, so if you aren't doing this, you will not see any performance boost over malloc
	 */
	mempool_init(500*KILOBYTE, sizeof(mempool_sample_struct_t));

	printf("Testng mempool alloc. Allocating 500 sample structs.\n");

	//Hold our pointers
	mempool_sample_struct_t* structs[500];

	/**
	 * Using mempool_alloc in a loop
	 */
	for(u_int16_t i = 0; i < 500; i++){
		/**
		 * Mempool allocate works the exact same way as regular malloc from a user's perspective. However, I will once again
		 * reiterate that the "num_bytes" field should be less than or equal to the default_block_size in MOST cases. If this is not
		 * the case, mempool will coalesce blocks, which is an expensive operation
		 *
		 * Notice how we are saving the pointers here. This is the exact same as malloc, where you must keep track of what you allocated, 
		 * or you will suffer a memory leak. Mempool is not garbage collected, so all memory allocated is your responsibility until mempool free is called
		 */
		structs[i] = (mempool_sample_struct_t*)mempool_alloc(sizeof(mempool_sample_struct_t));

		//Store some junk data to demonstrate
		structs[i]->array[0] = 3;
		structs[i]->array[1] = 5;
		structs[i]->d = 4e10;
	}

	//Let's look at a random struct to make sure its actually there
	srand(54);
	u_int16_t r = rand() % 54;
	printf("Randomly viewing a struct at index: %u", r);

	printf("Struct at index: %u\n \tstruct->array[0] = 3 == struct->array[0] = %u\n", r, structs[r]->array[0]);
	printf("\tstruct->array[1] = 5 == struct->array[1] = %u\n", structs[r]->array[1]);
	printf("\tstruct->d = 4e10 == struct->d = %e\n", structs[r]->d);

	printf("Freeing all sample structs.\n");

	/**
	 * Using mempool_free in a loop
	 */
	for(u_int16_t i = 0; i < 500; i++){
		/**
		 * Here we can see why we must keep track of the pointers. To free them, we can simply call mempool_free in the
		 * same way that we'd call regular free
		 */
		mempool_free(structs[i]);
	}


	printf("Demonstrating mempool_calloc() and coalescing of blocks.\n");
	/**
	 * To demonstrate mempool_calloc and the nature of coalescing blocks all at once, I will call mempool_calloc 
	 * and reserve a size that is larger than the default block size. As a reminder, this is allowed and supported,
	 * but if you find yourself doing this a lot, you're not using mempool correctly
	 */
	u_int16_t* int_arr = (u_int16_t*)mempool_calloc(40, sizeof(u_int16_t));

	printf("Array after mempool_calloc() and initializing:\n");
	//Fill it up for the sake of demonstration
	for(u_int16_t i = 0; i < 40; i++){
		*(int_arr + i) = i;
		printf("\tint_arr[%d]: %d\n", i, int_arr[i]);
	}

	/**
	 * To demonstrate mempool_realloc(), we will realloc() this block to have 10 additional ints in it. Mempool_realloc()
	 * works the same way externally as realloc
	 */
	int_arr = (u_int16_t*)mempool_realloc(int_arr, 50 * sizeof(u_int16_t));

	//Add the other ints in
	for(u_int16_t i = 40; i < 50; i++){
		*(int_arr + i) = i;
	}

	printf("Realloc'd array:\n");

	//Print out the array to demonstrate mempool_realloc()
	for(u_int16_t i = 0; i < 50; i++){
		printf("\tint_arr[%d]: %d\n", i, int_arr[i]);
	}

	/**
	 * When we're done, we do have to free the block. Mempool supports 
	 * freeing coalesced blocks as well, so again we just use this like regular
	 * free
	 */
	mempool_free(int_arr);

	printf("Destroying the mempool\n");
	/**
	 * Teardown
	 *
	 * When we're done, we have to destroy the mempool. This process is COMPLETELY DESTRUCTIVE. If you still
	 * had any pointers that came from mempool, they are now "dangling" and a hazard, so be careful
	 */
	mempool_destroy();

	return 0;
}


