/**
 * Author: Jack Robbins
 * This tester/demo program demonstrates a simple way of using mempool
 */

//Include the header file
#include "./mempool/mempool.h"
#include <stdio.h>
#include <sys/types.h>

/**
 * A sample struct for us to use in testing.
 */
typedef struct {
	int array[3];
	double d;
} mempool_sample_struct_t;


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


	/**
	 * Teardown
	 *
	 * When we're done, we have to destroy the mempool. This process is COMPLETELY DESTRUCTIVE. If you still
	 * had any pointers that came from mempool, they are now "dangling" and a hazard, so be careful
	 */
	mempool_destroy();

	return 0;
}


