/**
 * Author: Jack Robbins
 * A main program that tests/demonstrates the functionality of mempool
 */

//Include using the relative path, or include using cmake
#include <pthread.h>
#include <sys/types.h>
#include "demo_program/solver/solve.h"
#include "demo_program/puzzle/puzzle.h"
#include "mempool/mempool.h"


int main(){
	mempool_init(50 * MEGABYTE, sizeof(struct state));

	/*
	int* ints = mempool_alloc(sizeof(int) * 15);

	
	for(int i = 0; i < 15; i++){
		ints[i] = i;
	}

	printf("Before realloc\n");

	for(int i = 0; i < 15; i++){
		printf("%d\n", ints[i]);
	}

	ints = (int*)mempool_realloc(ints, sizeof(int) * 30);

	for(int i = 15; i < 30; i++){
		ints[i] = i;
	}

	printf("After realloc\n");

	for(int i = 0; i < 30; i++){
		printf("%d\n", ints[i]);
	}

	*/
	struct state* initial = generate_start_config(200, 4);
	struct state* goal = initialize_goal(4);

	solve(4, initial, goal);
	
	mempool_destroy();
}
