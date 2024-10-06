/**
 * Author: Jack Robbins
 * This file represents the header that will point to the solve_multi_threaded file
 */

#ifndef SOLVER_H
#define SOLVER_H

//Link to puzzle header file 
#include "../puzzle/puzzle.h"
#include "../../../mempool/mempool.h"

typedef struct thread_params_t thread_params_t;


/**
 * Define a structure for holding all of our thread parameters. We will only be using the multithreaded 
 * version of the solver
 */
struct thread_params_t {
	//The mempool that we are using
	mempool_t* mempool;
	//The predecessor state
	state_t* predecessor;
	//0 = leftMove, 1 = rightMove, 2 = downMove, 3 = upMove
	int option;
	//The size of the N puzzle
	int N;
	//The successors array that we will store the states in
	state_t** successors;
	//The closed and fringe
	fringe_t* fringe;
	closed_t* closed;
};


//The solve function. In theory, this is the only thing that we should need to see from solver
state_t* solve(mempool_t* mempool, const int N, state_t* start_state, state_t* goal_state, int solver_mode);

#endif /* SOLVER_H */
