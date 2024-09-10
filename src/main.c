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
	int array[2];
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


