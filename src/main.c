/**
 * Author: Jack Robbins
 * A main program that tests/demonstrates the functionality of mempool
 */

//Include using the relative path, or include using cmake
#include "mempool/mempool.h"
#include <stdio.h>
#include <sys/types.h>


int main(){
	printf("Creating a memory pool of 4GB\n");
	//Create a memory pool of size 4 gigabyte
	mempool_init(4l * GIGABYTE, 128);

	int* ex = mempool_calloc(20, sizeof(int));

	for(int i = 0; i < 20; i++){
		printf("%c\n", ex[i]);
	}

	mempool_free(ex);
	mempool_free(ex);

	
	printf("Deallocating the entire memory pool\n");
	//Destroy the mempool
	mempool_destroy();
}
