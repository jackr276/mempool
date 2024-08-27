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
	mempool_init(1 * MEGABYTE, sizeof(int));

	int* pointers[5000];

	for(int i = 0; i < 5000; i++){
		pointers[i] = (int*)mempool_alloc(sizeof(int));
		*(pointers[i]) = 5;
	}

	for(int i = 0; i < 5000; i++){
		mempool_free(pointers[i]);
	}


	
	printf("Deallocating the entire memory pool\n");
	//Destroy the mempool
	mempool_destroy();
}
