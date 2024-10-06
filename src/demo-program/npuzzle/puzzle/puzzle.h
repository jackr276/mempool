/**
 * Author: Jack Robbins
 * This header file contains state information and function prototypes that
 * are implemented in puzzle.c
 */


#ifndef PUZZLE_H
#define PUZZLE_H

#define ARRAY_START_SIZE 5000

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <time.h>
#include "../../../mempool/mempool.h"
typedef struct state_t state_t;
typedef struct closed_t closed_t;
typedef struct fringe_t fringe_t;


/**
* Defines a type of state, which is a structure, that represents a configuration in the gem puzzle game
*/
struct state_t {
   //Define a dynamic 2D array for the tiles since we have a variable puzzle size
   short* tiles;
   //For A*, define the total_cost, how far the tile has traveled, and heuristic cost int total_cost, current_travel, heuristic_cost;
   int total_cost, current_travel, heuristic_cost;
   //location (row and colum) of blank tile 0
   short zero_row, zero_column;
   //The next state in the linked list(fringe or closed), NOT a successor
   state_t* next;
   //The predecessor of the current state, used for tracing back a solution	
   state_t* predecessor;			
};


/**
 * Define a struct that holds everything that we need for the closed array
 */
struct closed_t {
	state_t** array;
	int next_closed_index;
	int closed_max_size;
};


/**
 * Define a struct that holds everything that we need for the fringe min heap
 */
struct fringe_t {
	state_t** heap;
	int next_fringe_index;
	int fringe_max_size;
};


/* Method Protoypes */
void initialize_state(mempool_t* mempool, state_t* state_ptr, const int N);
void destroy_state(mempool_t* mempool, state_t* state_ptr);
void cleanup_fringe_closed(mempool_t* mempool, fringe_t* fringe, closed_t* closed, state_t* state_ptr, const int N);
void cleanup_solution_path(mempool_t* mempool, state_t* solution);
void print_state(state_t* state_ptr, const int N, int option);
void copy_state(state_t* predecessor, state_t* successor, const int N);
void move_down(state_t* state_ptr, const int N);
void move_right(state_t* state_ptr, const int N);
void move_up(state_t* state_ptr, const int N);
void move_left(state_t* state_ptr, const int N);
int states_same(state_t* a, state_t* b, const int N);
void update_prediction_function(state_t* state_ptr, int N);
void priority_queue_insert(fringe_t* fringe, state_t* state_ptr);
state_t* initialize_goal(mempool_t* mempool, const int N);
state_t* generate_start_config(mempool_t* mempool, const int complexity, const int N);
closed_t* initialize_closed(void);
fringe_t* initialize_fringe(void);
void merge_to_closed(closed_t* closed, state_t* state_ptr);
state_t* dequeue(fringe_t* fringe);
int fringe_empty(fringe_t* fringe);
void check_repeating_fringe(mempool_t* mempool, fringe_t* fringe, state_t** state_ptr, const int N);
void check_repeating_closed(mempool_t* mempool, closed_t* closed, state_t** state_ptr, const int N);
int merge_to_fringe(fringe_t* fringe, state_t* successors[4]);

#endif /* PUZZLE_H */
