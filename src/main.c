/**
 * Author: Jack Robbins
 * A main program that tests/demonstrates the functionality of mempool
 */

//Include using the relative path, or include using cmake
#include "mempool/mempool.h"
#include <pthread.h>
#include <stdio.h>
#include <sys/types.h>

//TEST CODE
void* thread_allocate(void* params){
	int* ints = mempool_alloc(sizeof(int) * 50);

	for(int i = 0; i < 50; i++){
		ints[i] = i;
	}

	for(int i = 0; i < 50; i++){
		printf("%d\n", ints[i]);
	}

	mempool_free(ints);

	return NULL;
}

int main(){
	printf("Creating a memory pool of 4GB\n");
	//Create a memory pool of size 4 gigabyte
	mempool_init(1 * MEGABYTE, sizeof(int));

	//Attempt to coalesce
	int* ints = (int*)mempool_alloc(sizeof(int) * 50);

	int* pointers[50];

	pthread_t threads[20];

	for(int i = 0; i < 20; i++){
		pthread_t thread;
		threads[i] = thread; 
		pthread_create(&threads[i], NULL, thread_allocate, NULL);
	}

	for(int i = 0; i < 50; i++){
		pointers[i] = (int*)mempool_alloc(sizeof(int));
		*(pointers[i]) = 5;
	}

	for(int i = 0; i < 50; i++){
		mempool_free(pointers[i]);
	}

	for(int i = 0; i < 50; i++){
		ints[i] = i;
	}

	for(int i = 0; i < 50; i++){
		printf("%d\n", ints[i]);
	}

	for(int i = 0; i < 20; i++){
		pthread_join(threads[i],NULL);
	}

	mempool_free(ints);
	
	printf("Deallocating the entire memory pool\n");
	//Destroy the mempool
	mempool_destroy();
}
