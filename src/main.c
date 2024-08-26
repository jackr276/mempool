/**
 * Author: Jack Robbins
 * A main program that tests/demonstrates the functionality of mempool
 */

//Include using the relative path, or include using cmake
#include "mempool/mempool.h"
#include <stdio.h>



int main(){
	printf("Creating a memory pool of 1GB\n");
	//Create a memory pool of size 1 gigabyte
	mempool_init(1 * GIGABYTE, 512);

	
	printf("Deallocating the entire memory pool\n");
	//Destroy the mempool
	mempool_destroy();
}
