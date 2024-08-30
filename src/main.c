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
	mempool_init(500000 * sizeof(struct state), sizeof(struct state));

	struct state* initial = generate_start_config(2090, 4);
	struct state* goal = initialize_goal(4);

	solve(4, initial, goal);
	
	mempool_destroy();
}
