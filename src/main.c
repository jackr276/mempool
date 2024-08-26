/**
 * Author: Jack Robbins
 * A main program that tests/demonstrates the functionality of mempool
 */

//Include using the relative path, or include using cmake
#include "mempool/mempool.h"
#include <bits/time.h>
#include <stdio.h>
#include <sys/types.h>



int main(){
	printf("Creating a memory pool of 4GB\n");
	//Create a memory pool of size 4 gigabyte
	u_int64_t num_gbs = 4;
	mempool_init(num_gbs * GIGABYTE, 512);

	
	printf("Deallocating the entire memory pool\n");
	//Destroy the mempool
	mempool_destroy();
}
