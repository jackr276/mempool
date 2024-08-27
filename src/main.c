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

	int* ex = (int*)mempool_calloc(80, 0, 78);

	for(int i = 0; i < 78; i++){
		printf("%d\n", ex[i]);
	}

	mempool_free(ex);

	int b = 7;
	int* a = &b;
	mempool_free(a);
	
	printf("Deallocating the entire memory pool\n");
	//Destroy the mempool
	mempool_destroy();
}
