/**
 * Author: Jack Robbins
 * This program implements an A* search algorithm to find the shortest solve path for the 15-puzzle problem
 * game. It takes in an N-puzzle problem starting configuration in row-major order as a command line argument, following
 * a number N that represents the NxN size of the puzzle, and prints out the full solution path to the problem,
 * step by step, if such a solution exists.
 *
 * NOTE: This is the multi-threaded version of the solver, using pthreads
 */

//For timing
#include <time.h>
//For multi-threading functionality
#include <pthread.h>
#include "solve.h"

/**
 * This worker thread function generates and checks the validity of a successor that is made by 
 * moving up, down, left or right based on the option given. It will also update the prediction function
 * of the successor if the successor is valid
 */
static void* generator_worker(void* thread_params){
	//Initialize a state pointer to be null by default
	struct state* moved = NULL;	
	//Make an appropriate cast to the parameter input struct
	struct thread_params* parameters = (struct thread_params*)thread_params;
	//Grab the option for convenience
	int option = parameters->option;
	int N = parameters->N;

	//Perform a left move if option is 0 and if possible
	if(option == 0 && parameters->predecessor->zero_column > 0){
		//Create the new state
		moved = (struct state*)malloc(sizeof(struct state));
		//Dynamically allocate the space needed in the state
		initialize_state(moved, N);
		//Perform a deep copy from predecessor to successor
		copy_state(parameters->predecessor, moved, N);
		//Use helper function to move left
		move_left(moved, N);

	//Perform a right move if option is 1 and if possible
	} else if(option == 1 && parameters->predecessor->zero_column < N-1){
		//Create the new state
		moved = (struct state*)malloc(sizeof(struct state));
		//Dynamically allocate the space needed in the state
		initialize_state(moved, N);
		//Perform a deep copy from predecessor to successor
		copy_state(parameters->predecessor, moved, N);
		//Use helper function to move right 
		move_right(moved, N);

	//Perform a down move if option is 2 and if possible
	} else if(option == 2 && parameters->predecessor->zero_row < N-1){
		//Create the new state
		moved = (struct state*)malloc(sizeof(struct state));
		//Dynamically allocate the space needed in the state
		initialize_state(moved, N);
		//Perform a deep copy from predecessor to successor
		copy_state(parameters->predecessor, moved, N);
		//Use helper function to move down	
		move_down(moved, N);

	//Perform an up move if option is 3 and if possible
	} else if(option == 3 && parameters->predecessor->zero_row > 0){
		//Create the new state
		moved = (struct state*)malloc(sizeof(struct state));
		//Dynamically allocate the space needed in the state
		initialize_state(moved, N);
		//Perform a deep copy from predecessor to successor
		copy_state(parameters->predecessor, moved, N);
		//Use helper function to move up	
		move_up(moved, N);
	}
	
	//Whether it's null or not, place the pointer into moved
	parameters->successors[option] = moved;

	//Only perform the checks if moved is not null
	if(moved != NULL){
		//Now we must check for repeating
		//Important -- we need to modify the state in successors, not the local copy "moved"
		check_repeating_closed(parameters->closed, &(parameters->successors[option]), N);
		check_repeating_fringe(parameters->fringe, &(parameters->successors[option]), N);
		//Update prediction function
		update_prediction_function(parameters->successors[option], N);
	}

	//Threadwork done, no return value will be used
	pthread_exit(NULL);
}


/**
 * This multi-threaded version of successor generation and validation spawns an individual thread for
 * each of the 4 possible moves, potentially expediting the process of generating and checking successors
 */
static void generate_successors(struct fringe* fringe, struct closed* closed, struct state* predecessor, struct state** successors, int N){
	//We will create 4 threads, once for each successor potential successor
	pthread_t thread_arr[4];
	//We also need 4 thread_param structures 
	struct thread_params* param_arr[4];

	//Create all 4 threads
	for(int i = 0; i < 4; i++){
		//Reserve space and allocate appropriate values in each thread_param
		param_arr[i] = (struct thread_params*)malloc(sizeof(struct thread_params));
		param_arr[i]->predecessor = predecessor;
		//The option will tell the thread function what move to make
		param_arr[i]->option = i;
		//Set the value of N
		param_arr[i]->N = N;
		//Save in successors for storage
		param_arr[i]->successors = successors;
		//Pass in refences to closed and fringe
		param_arr[i]->fringe = fringe;
		param_arr[i]->closed = closed;	

		//Spawn our worker threads, generator_worker is the thread funtion, and paramArr[i]
		//is the needed struct input
		pthread_create(&thread_arr[i], NULL, generator_worker, param_arr[i]);
	}

	//rejoin all the threads
	for(int i = 0; i < 4; i++){
		pthread_join(thread_arr[i], NULL);
		//Free the memory from the parameter structure
		free(param_arr[i]);
	}
}


/**
 * A simple helper function that will perform all of the printing when we are in debug mode in our solver
 */
void print_solution_path(struct state* solution_path, const int N, int pathlen, int num_unique_configs, double time_spent_CPU){
	//Print out the solution path first	
	printf("\nSolution found! Now displaying solution path\n");
	//Display the path length for the user
	printf("Path Length: %d\n\n", pathlen); 

	//Print out the solution path in order
	while(solution_path != NULL){
		print_state(solution_path, N, 0);
		solution_path = solution_path->next;
	}	

	//Print out all running statistics
	printf("================ Program Running Statistics ===============\n\n");
	//Print out the path length
	printf("Optimal solution path length: %d\n", pathlen);
	//Print out the number of unique configurations generated
	printf("Unique configurations generated by solver: %d\n", num_unique_configs);
	//Print out total memory consumption in Megabytes
	printf("Memory consumed: %.2f MB\n", (sizeof(struct state) + N*N*sizeof(short)) * num_unique_configs / 1048576.0);
	//Print out CPU time(NOT wall time) spent
	printf("Total CPU time spent: %.7f seconds\n\n", time_spent_CPU);	
	printf("===========================================================\n\n");
}


/**
 * Use an A* search algorithm to solve the 15-puzzle problem by implementing the A* main loop. If the solve function 
 * is successful, it will print the resulting solution path to the console as well.  
 * For mode: 0 equals web client solve, 1 equals debug(CLI) mode
 */
struct state* solve(int N, struct state* start_state, struct state* goal_state, int solver_mode){
	//If we are in debug mode, we will start off by printing to the console
	if(solver_mode == 1){
		printf("\nInitial State:\n");
		print_state(start_state, N, 0);
		printf("Goal state\n");
		print_state(goal_state, N, 0);
	} 
	

	//Create the fringe and closed structues
	struct fringe* fringe = initialize_fringe();
	struct closed* closed = initialize_closed();

	//We will keep track of the time taken to execute
	clock_t begin_CPU = clock();

	//We will keep track of the number of iterations as a sanity check for large problems
	int iteration = 0;
	//We will also keep track of the number of unique configurations
	int num_unique_configs = 0;
	//Define an array for holding successor states. We can generate at most 4 each time
	struct state* successors[4];

	//Put the start_state into fringe to begin the search
	priority_queue_insert(fringe, start_state);

	//Maintain a pointer for the current state in the search
	struct state* curr_state;

	//Algorithm main loop -- while there are still states to be expanded, keep iterating until we find a solution
	while (!fringe_empty(fringe)){
		//Remove or "pop" the head of the fringe linked list -- because fringe is a priority queue, this is the most
		//promising state to explore next
		curr_state = dequeue(fringe);

		//Check to see if we have found the solution. If we did, we will print out the solution path and stop
		if(states_same(curr_state, goal_state, N)){
			//Stop the clock if we find a solution
			clock_t end_CPU = clock();

			//Determine the time spent on the CPU
			double time_spent_CPU = (double)(end_CPU - begin_CPU) / CLOCKS_PER_SEC;

			//Now find the solution path by working backwords
			//Keep track of how long the path is	
			int pathlen = 0;
			//Keep a linked list for our solution path
			struct state* solution_path = NULL;
		
			//Put the states into the solution path in reverse order(insert at the head) using their predecessor
			while(curr_state != NULL){
				//Insert the current state at the head of solution path
				curr_state->next = solution_path;
				solution_path = curr_state;
				//Go back up the solution chain using predecessor
				curr_state = curr_state->predecessor;
				//Increment the path length
				pathlen++;
			}

			//Cleanup the fringe and closed arrays
			cleanup_fringe_closed(fringe, closed, solution_path, N);

			//If we are in debug mode, print this path to the console
			if(solver_mode == 1){
				//Print the path
				print_solution_path(solution_path, N, pathlen, num_unique_configs, time_spent_CPU);
				//Cleanup the path
				cleanup_solution_path(solution_path);
				//Return nothing, as it isn't used
				return NULL;
			}

			//We've found a solution, so the function should exit 
			return solution_path;	
		}
		
		/**
		 * This part can benefit from multi-threading, generate_valid_successors implements
		 * multiple threads for creating and checking the validity of successors
		 */

		//Generate successors to the current state once we know it isn't a solution
		generate_successors(fringe, closed, curr_state, successors, N);
		
		/* End multi-threading */

		//Add all necessary states to fringe now that we have checked for repeats and updated predictions 
		//Additionally, we need to update the num_unique_configs, this will be done in merge_to_fringe
		num_unique_configs += merge_to_fringe(fringe, successors); 
	
		//Add to closed
		merge_to_closed(closed, curr_state);

		//For very complex problems, print the iteration count to the console for a sanity check
		if(solver_mode == 1 && iteration > 1 && iteration % 1000 == 0) {
			printf("Iteration: %6d, %6d total unique states generated\n", iteration, num_unique_configs);
		}
		
		//End of one full iteration
		iteration++;
	}
	
	//If we end up here, fringe became NULL with no goal configuration found, so there is no solution
	printf("No solution.\n");

	//Cleanup the fringe and closed arrays
	cleanup_fringe_closed(fringe, closed, NULL, N);
	return NULL;
}
