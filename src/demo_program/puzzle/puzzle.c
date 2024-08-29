/**
 * Author: Jack Robbins
 * This c file contains the implementations for functions in puzzle.h, that are used by the solver,
 * in order to minimize repeated code
 */

//Link to puzzle.h
#include "puzzle.h"


/**
 * The initialize_state function takes in a pointer to a state and reserves the appropriate space for the dynamic array
 * that holds the tiles 
 */
void initialize_state(struct state* statePtr, const int N){
	//Declare all of the pointers needed for each row
	statePtr->tiles = (short*)malloc(sizeof(short) * N * N);
	statePtr->predecessor = NULL;
	statePtr->next = NULL;
}


/**
 * The destroy_state function does the exact reverse of the initialize_state function to properly free memory
 */
void destroy_state(struct state* statePtr){
	//We only need to free the tile pointer in this case
	free(statePtr->tiles);
}


/**
 * Prints out a state by printing out the positions in the 4x4 grid. If option is 1, print the
 * state out in one line
 */
void print_state(struct state* statePtr, const int N, int option){
	//Go through tile by tile and print out
	for (int i = 0; i < N; i++) {
		for (int j = 0; j < N; j++){
			//Support printing of states with 2 or 3 digit max integers
			if(N < 11){	
				//With numbers less than 11, N^2 is at most 99, so only 2 digits needed
				printf("%2d ", *(statePtr->tiles + i * N + j));
			} else {
				//Ensures printing of large states will not be botched
				printf("%3d ", *(statePtr->tiles + i * N + j));
			}
		}
		//Support printing in a single line
		if(!option){
			//Print a newline to represent the row change
			printf("\n");
		}
	}
	printf("\n");
}


/**
 * Performs a "deep copy" from the predecessor to the successor
 */
void copy_state(struct state* predecessor, struct state* successor, const int N){
	//Copy over the tiles array
	for(int i = 0; i < N; i++){
		for(int j = 0; j < N; j++){
			//Copy tile by tile
			*(successor->tiles + i * N + j) = *(predecessor->tiles + i * N + j);
		}
	}

	//Initialize the current travel to the predecessor travel + 1
	successor->current_travel = predecessor->current_travel+1;
	//Copy the zero row and column position
	successor->zero_row = predecessor->zero_row;
	successor->zero_column = predecessor->zero_column;
	//Initialize the successor's next to be null
	successor->next = NULL;
	//Set the successors predecessor
	successor->predecessor = predecessor;
}


/**
 * A simple function that swaps two tiles in the provided state
 * Note: The swap function assumes all row positions are valid, this must be checked by the caller
 */
static void swap_tiles(int row1, int column1, int row2, int column2, struct state* statePtr, const int N){
	//Store the first tile in a temp variable
	short tile = *(statePtr->tiles + row1 * N + column1);
	//Put the tile from row2, column2 into row1, column1
	*(statePtr->tiles + row1 * N + column1) = *(statePtr->tiles + row2 * N + column2);
	//Put the temp in row2, column2
	*(statePtr->tiles + row2 * N + column2) = tile;
}


/**
 * Move the 0 slider down by 1 row
 */
void move_down(struct state* statePtr, const int N){
	//Utilize the swap function, move the zero_row down by 1
	swap_tiles(statePtr->zero_row, statePtr->zero_column, statePtr->zero_row+1, statePtr->zero_column, statePtr, N);
	//Increment the zero_row to keep the position accurate
	statePtr->zero_row++;
}


/**
 * Move the 0 slider right by 1 column
 */
void move_right(struct state* statePtr, const int N){
	//Utilize the swap function, move the zero_column right by 1
	swap_tiles(statePtr->zero_row, statePtr->zero_column, statePtr->zero_row, statePtr->zero_column+1, statePtr, N);
	//Increment the zero_column to keep the position accurate
	statePtr->zero_column++;
}


/**
 * Move the 0 slider up by 1 row
 */
void move_up(struct state* statePtr, const int N){
	//Utilize the swap function, move the zero_row up by 1
	swap_tiles(statePtr->zero_row, statePtr->zero_column, statePtr->zero_row-1, statePtr->zero_column, statePtr, N);
	//Decrement the zero_row to keep the position accurate
	statePtr->zero_row--;
}


/**
 * Move the 0 slider left by 1 column
 */
void move_left(struct state* statePtr, const int N){
	//Utilize the swap function, move the zero_column left by 1
	swap_tiles(statePtr->zero_row, statePtr->zero_column, statePtr->zero_row, statePtr->zero_column-1, statePtr, N);
	//Decrement the zero_column to keep the position accurate
	statePtr->zero_column--;
}


/**
 * A simple helper function that will tell if two states are the same. To be used for filtering
 */
int states_same(struct state* a, struct state* b, const int N){
	//Efficiency speedup -- if zero row and column aren't equal, return false
	if(a->zero_row != b->zero_row || a->zero_column != b->zero_column){
		return 0;
	}

	//Go through each row in the dynamic tile matrix in both states
	for(int i = 0; i < N * N; i++){
		if(*(a->tiles + i) != *(b->tiles + i)){ 
			return 0;
		}
	}

	//Return 1 if same	
	return 1;
}


/**
 * Update the prediction function for the state pointed to by succ_states[i]. If this pointer is null, simply skip updating
 * and return. This is a generic algorithm, so it will work for any size N
 */ 
void update_prediction_function(struct state* statePtr, const int N){
	//If statePtr is null, this state was a repeat and has been freed, so don't calculate anything
	if(statePtr == NULL){
		return;
	}

	//The current_travel of the state has already been updated by stateCopy, so we only need to find the heuristic_cost
	statePtr->heuristic_cost = 0;

	/**
	* For heuristic_cost, we will use the manhattan distance from each tile to where it should be.
	* Conveniently, each tile 1-15 should be in position 0-14, so we can find manhattan distance by
	* doing the sum of the absolute difference in coordinates from a number's current state position
	* to its goal state position
	*/

	//Declare all needed variables
	short selected_num, goal_rowCor, goal_colCor;
	//Keep track of the manhattan distance
	int manhattan_distance;
	
	//Go through each tile in the state and calculate the heuristic_cost
	for(int i = 0; i < N; i++){
		for(int j = 0; j < N; j++){
			//grab the number to be examined
			selected_num = *(statePtr->tiles + i * N + j);

			//We do not care about 0 as it can move, so skip it
			if(selected_num == 0){
				continue;
			}

			//Otherwise mathematically find the needed position
			//Goal row coordinate is the index of the number divided by number of rows
			goal_rowCor = (selected_num - 1) / N;
			//Goal column coordinate is the index modulated by column length 
			goal_colCor = (selected_num - 1) % N;
				

			//Manhattan distance is the absolute value of the x distance and the y distance
			manhattan_distance = abs(i - goal_rowCor) + abs(j - goal_colCor);	
		
			//Add manhattan distance for each tile
			statePtr->heuristic_cost += manhattan_distance;
		}
	}
		
	/**
	 * Now we must calculate the linear conflict heuristic. This heuristic takes two tiles in their goal row
	 * or goal column and accounts for the fact that for each tile to be moved around, it actually takes
	 * at least 2 additional moves. Given two tiles in their goal row, 2 additional vertical moves are added
	 * to manhattan distance for each row/column amount that they have to move
	 * 
	 */

	//We initially have no linear conflicts
	int linear_conflicts = 0;
	//Declare for convenience
	short left, right, above, below;
	//Also declare goal row coordinates for convenience
	short goal_rowCor_left, goal_rowCor_right, goal_colCor_above, goal_colCor_below;

	//Check each row for linear conflicts
	for(int i = 0; i < N; i++){
		for(int j = 0; j < N-1; j++){
			//Grab the leftmost tile that we'll be comparing to
			left = *(statePtr->tiles + i * N + j);  

			//If this tile is 0, it's irrelevant so do not explore further
			if(left == 0){
				continue;
			}
			
			//Now go through every tile in the row after left, this is what makes this generalized linear conflict
			for(int k = j+1; k < N; k++){
				//Grab right tile for convenience
				right = *(statePtr->tiles + i * N + k);

				//Again, if the tile is 0, no use in wasting cycles with it
				if(right == 0){
					continue;
				}
				//Check if both tiles are in their goal row 
				goal_rowCor_left = (left - 1) / N;
				goal_rowCor_right = (right - 1) / N;

				//If these tiles are not BOTH their goal row, linear conflict does not apply
				//This is conterintuitive, but as it turns out makes an enormous difference
				if(goal_rowCor_left != goal_rowCor_right || goal_rowCor_right != i){
					continue;
				}
			
				//If the tiles are swapped, we have a linear conflict
				if(left > right){
					//To be more informed, we should add 2 moves for EACH time we have to swap
					linear_conflicts++;
				}
			}	
		}
	}

	//Now check each column for linear conflicts 
	for(int i = 0; i < N-1; i++){
		for(int j = 0; j < N; j++){
			//Grab the abovemost tile that we'll be comparing to
			above = *(statePtr->tiles + i * N + j);

			//If this tile is 0, it's irrelevant so do not explore further
			if(above == 0){
				continue;
			}

			//Now go through every tile in the column below "above", this is what makes it generalized linear conflict
			for(int k = i+1; k < N; k++){
				//Grab the below tile for convenience
				below = *(statePtr->tiles + k * N + j);

				//We don't care about the 0 tile, skip if we find it
				if(below == 0){
					continue;
				}

				//Check if both tiles are in their goal column
				goal_colCor_above = (above - 1) % N;
				goal_colCor_below = (below - 1) % N;

				//If these tiles are not BOTH in their goal column, linear conflict does not apply
				//This is counterintutive, but as it turns out makes an enormous difference
				if(goal_colCor_below != goal_colCor_above || goal_colCor_above != j){
					continue;
				}

				//If above is more than below, we have a linear conflict
				if(above > below){
					//To be more informed, we should add 2 moves for EACH time we have to swap
					linear_conflicts++;				
				}
			}
		}
	}

	//Once we have calculated the number of linear conflicts, we add it into the heuristic cost
	//For each linear conflict, a minimum of 2 additional moves are required to swap tiles, so add 2 to the heuristic_cost
	statePtr->heuristic_cost += linear_conflicts * 2;

	//Once we have the heuristic_cost, update the total_cost
	statePtr->total_cost = statePtr->heuristic_cost + statePtr->current_travel;
}



/**
 * This initialization function mathematically creates a goal state for a given 
 * size N
 */
struct state* initialize_goal(const int N){
	//Initial allocation
	struct state* goal_state = (struct state*)malloc(sizeof(struct state));
	//Dynamically allocate the memory needed in the goal_state
	initialize_state(goal_state, N);	

	int row, col;
	//To create the goal state, place the numbers 1-15 in the appropriate locations
	for(short num = 1; num < N * N; num++){
		//We can mathematically find row and column positions for inorder numbers
		row = (num - 1) / N;
		col = (num - 1) % N;
		*(goal_state->tiles + row * N + col) = num;
	}

	//0 is always at the last spot in the goal state
	*(goal_state->tiles + N * N - 1) = 0;

	//Initialize everything else in the goal state
	goal_state->zero_row = (goal_state)->zero_column = N-1;
	goal_state->total_cost = 0;
	goal_state->current_travel = 0;
	goal_state->heuristic_cost = 0;
	goal_state->next=NULL; 
	
	return goal_state;
}


/**
 * A simple helper function that allocates memory for fringe
 */
struct fringe* initialize_fringe(){
	//Allocate memory for the fringe struct
	struct fringe* fringe = (struct fringe*)malloc(sizeof(struct fringe));

	//Initialize these values
	fringe->fringe_max_size = ARRAY_START_SIZE;
	fringe->next_fringe_index = 0;

	//Allocate space for the heap
	fringe->heap = (struct state**)malloc(sizeof(struct state*) * fringe->fringe_max_size);

	//Return a pointer to our fringe in memory
	return fringe;
}


/**
 * A simple helper function that allocates memory for closed
 */
struct closed* initialize_closed(){
	//Allocate memory for closed
	struct closed* closed = (struct closed*)malloc(sizeof(struct closed));
	
	//Initialize these values
	closed->closed_max_size = ARRAY_START_SIZE;
	closed->next_closed_index = 0;

	//Reserve space for the internal array
	closed->array = (struct state**)malloc(sizeof(struct state*) * closed->closed_max_size);

	//Return the closed pointer
	return closed;
}


/**
 * A helper function that merges the given statePtr into closed. This function also automatically
 * resizes closed, so the caller does not have to maintain the array
 */
void merge_to_closed(struct closed* closed, struct state* statePtr){
	//If we run out of space, we can expand
	if(closed->next_closed_index == closed->closed_max_size){
		//Double closed max size
		closed->closed_max_size *= 2;
		//Reallocate space for closed
		closed->array = (struct state**)realloc(closed->array, sizeof(struct state*) * closed->closed_max_size);
	}

	//Put curr_state into closed
	closed->array[closed->next_closed_index] = statePtr; 
	//Keep track of the next closed index
	(closed->next_closed_index)++;

}


/**
 * A simple helper function that will swap two pointers in our minHeap
 */
static void swap(struct state** a, struct state** b){
	struct state* temp = *a;
	*a = *b;
	*b = temp;
}


/**
 * A simple helper function that calculates the parent of a certain index in the minHeap
 */
static int parent_index(int index){
	return (index - 1) / 2;
}


/**
 * States will be merged into fringe according to their priority values. The lower the total cost,
 * the higher the priority. Since fringe is a minHeap, we will insert accordingly
 */
void priority_queue_insert(struct fringe* fringe, struct state* statePtr){
	//Automatic resize
	if(fringe->next_fringe_index == fringe->fringe_max_size){
		//Just double this value
		fringe->fringe_max_size *= 2;
		//Reallocate fringe memory	
		fringe->heap = (struct state**)realloc(fringe->heap, sizeof(struct state*) * fringe->fringe_max_size);
	}

	//Insert value at the very end
	fringe->heap[fringe->next_fringe_index] = statePtr;
	//Increment the next fringe index
	(fringe->next_fringe_index)++;

	//The current index will be used to reheapify after this addition
	int current_index = fringe->next_fringe_index - 1;

	//As long as we're in valid bounds, and the priorities of parent and child are backwards
	while (current_index > 0 && fringe->heap[parent_index(current_index)]->total_cost > fringe->heap[current_index]->total_cost){
		//Swap the two values
		swap(&(fringe->heap[parent_index(current_index)]), &(fringe->heap[current_index]));

		//Set the current index to be it's parent, and repeat the process
		current_index = parent_index(current_index);
	}
}


/**
 * A recursive function that will heapify fringe following any delete operations. It takes in
 * the index to be min heapified. This is a "down heapify" because we start at the front
 */
static void min_heapify(struct fringe* fringe, int index){
	//Initialize the smallest as the index
	int smallest = index;
	//Right and left children of current index
	int left_child = index * 2 + 1;
	int right_child = index * 2 + 2;

	//If the left child has a lower priority than the index
	if(left_child < fringe->next_fringe_index && fringe->heap[left_child]->total_cost < fringe->heap[index]->total_cost){
		smallest = left_child;
	}

	//If the right child has a lower priority than the smallest, we want to make sure we have the absolute smallest 
	if(right_child < fringe->next_fringe_index && fringe->heap[right_child]->total_cost < fringe->heap[smallest]->total_cost){
		smallest = right_child;
	}

	//If we found something smaller than index, we must swap
	if(smallest != index){
		//Swap the two values
		swap(&(fringe->heap[index]), &(fringe->heap[smallest]));
		//Recursively call 
		min_heapify(fringe, smallest);
	}
}


/**
 * Dequeues by removing from the minHeap datastructure. This involves removing value at index 0,
 * replacing it with the very last value, and calling minHeapify()
 */
struct state* dequeue(struct fringe* fringe){
	//Save the pointer, we always take from the front
	struct state* dequeued = fringe->heap[0];
	
	//Put the last element in the front to "prime" the heap
	fringe->heap[0] = fringe->heap[fringe->next_fringe_index - 1];

	//Decrement this value
	(fringe->next_fringe_index)--;
	
	//Call minHeapify on index 0 to maintain the heap properly
	min_heapify(fringe, 0);

	//Give the dequeued pointer back
	return dequeued;
}


/**
 * This function generates a starting configuration of appropriate complexity by moving the 0
 * slider around randomly, for an appropriate number of moves
 */
struct state* generate_start_config(const int complexity, const int N){
	//Create the simplified state that we will use for generation
	struct state* statePtr = (struct state*)malloc(sizeof(struct state));
	//Iniitialize the state with helper function
	initialize_state(statePtr, N);

	int row, col;
	//Now generate the goal state. Once we create the goal state, we will "mess it up" according to the input number
	for(short index = 1; index < N*N; index++){
		//Mathematically generate row position for goal by integer dividing the number by N
		row = (index - 1) / N;
		//Mathematically generate column position for goal by finding remainder of row division
		col = (index - 1) % N;
		//Put the index in the correct position
		*(statePtr->tiles + row * N + col) = index;
	}
	
	//Now that we have generated and placed numbers 1-15, we will put the 0 slider in the very last slot
	*(statePtr->tiles + N * N - 1) = 0;
	//Initialize the zero_row and zero_column position for use later
	statePtr->zero_row = N-1;
	statePtr->zero_column = N-1;

	//Set the seed for our random number generation
	srand(time(NULL));

	//Counter for while loop
	int i = 0;

	//A variable to store our random move numbers in
	int random_move;
	//The main loop of our program. Keep randomly messing up the goal config as many times as specified
	//In theory -- higher number inputted = more complex config
	while(i < complexity){
		//Get a random number from 0 to 4
		random_move = rand() % 4;	

		//We will keep the same convention as in the solver
		// 0 = left move, 1 = right move, 2 = down move , 3 = up move
		
		//Move left if possible and random_move is 0
		if(random_move == 0 && statePtr->zero_column > 0){
			move_left(statePtr, N);
		}

		//Move right if possible and random move is 1
		if(random_move == 1 && statePtr->zero_column < N-1){
			move_right(statePtr, N);
		}

		//Move down if possible and random move is 2
		if(random_move == 2 && statePtr->zero_row < N-1){
			move_down(statePtr, N);
		}

		//Move up if possible and random move is 3
		if(random_move == 3 && statePtr->zero_row > 0){
			move_up(statePtr, N);
		}

		//Increment i
		i++;
	}

	return statePtr;
}


/**
 * A very simple helper function that lets solve know if the fringe is empty
 */
int fringe_empty(struct fringe* fringe){
	return fringe->next_fringe_index == 0;
}


/**
 * Check to see if the state at position i in the fringe is repeating. If it is, free it and
 * set the pointer to be null
 * NOTE: since we may modify the memory address of statePtr, we need a reference to that address 
 */
void check_repeating_fringe(struct fringe* fringe, struct state** statePtr, const int N){ 	
	//If succ_states[i] is NULL, no need to check anything
	if(*statePtr == NULL){
		return;
	}

	//Go through the heap, if we ever find an element that's the same, break out and free the pointer
	for(int i = 0; i < fringe->next_fringe_index; i++){
		//If the states match, we free the pointer and exit the loop
		if(states_same(*statePtr, fringe->heap[i], N)){
			//Properly tear down the dynamic array in the state to avoid memory leaks
			destroy_state(*statePtr);
			//Free the pointer to the state
			free(*statePtr);
			//Set the pointer to be null as a warning
			*statePtr = NULL;
			break;
		}
	}
	//If we get here, we know that the state was not repeating
}


/**
 * Check for repeats in the closed array. Since we don't need any priority queue functionality,
 * using closed as an array is a major speedup for us
 * NOTE: since we may modify the memory address of statePtr, we need a reference to that address 
 */
void check_repeating_closed(struct closed* closed, struct state** statePtr, const int N){
	//If this has already been made null, simply return
	if(*statePtr == NULL){
		return;
	}

	//Go through the entire populated closed array
	for(int i = closed->next_closed_index - 1; i > -1; i--){
		//If at any point we find that the states are the same
		if(states_same(closed->array[i], *statePtr, N)){
			//Free both the internal memory and the state pointer itself
			destroy_state(*statePtr);
			free(*statePtr);
			//Set to null as a warning
			*statePtr = NULL;
			//Break out of the loop and exit
			break;
		}
	}
	//If we get here, we know that the state was not repeating
}


/**
 * This function simply iterates through successors, passing the appropriate states along to priority_queue_insert if the pointers
 * are not null
 */
int merge_to_fringe(struct fringe* fringe, struct state* successors[4]){ 
	//Keep track of how many valid(not null) successors that we merge in
	int valid_successors = 0;

	//Iterate through succ_states, if the given state is not null, call the priority_queue_insert function on it
	for(int i = 0; i < 4; i++){
		if(successors[i] != NULL){
			//If it isn't null, we also know that we have one more unique config, so increment our counterS
			valid_successors++;
			//Insert into queue
			priority_queue_insert(fringe, successors[i]);
		}
	}
	//Return how many valid successors that we had
	return valid_successors;
}


/**
 * A helper function to tell us if state_ptr is in the solution path linked list
 */
static int in_solution_path(struct state* state_ptr, struct state* solution_path, const int N){
	//If the solution is null, it can't contain it so return false
	if(solution_path == NULL){
		return 0;
	}

	//Maintain a cursor
	struct state* cursor = solution_path;

	//Iterate over our linked list
	while(cursor != NULL){
		//If we have a match, return 1
		if(states_same(state_ptr, cursor, N) == 1){
			return 1;
		}

		//Advance the cursor
		cursor = cursor->next;
	}
	
	//If we get here, we didn't find it
	return 0;
}


/**
 * Cleanup the fringe and closed lists when we're done
 */
void cleanup_fringe_closed(struct fringe* fringe, struct closed* closed, struct state* solution_path, const int N){
	//cleanup fringe
	for(int i = 0; i < fringe->next_fringe_index; i++){
		destroy_state(fringe->heap[i]);
		free(fringe->heap[i]);
	}

	//Free the fringe array
	free(fringe->heap);
	//Free the fringe struct
	free(fringe);

	//cleanup closed
	for(int i = 0; i < closed->next_closed_index; i++){
		//NOTE: some of the stuff is fringe is in our solution path,
		//for obvious reasons, if we destroy those states we will have issues
		//so we must check here
		if(in_solution_path(closed->array[i], solution_path, N) == 0){
			destroy_state(closed->array[i]);
			free(closed->array[i]);
		}
	}

	//Free the array of pointers
	free(closed->array);
	//Free the close struct
	free(closed);
}


/**
 * Define a way of cleaning up the solution path once done with it
 */
void cleanup_solution_path(struct state* solution_path){
	//Assign a cursor for list traversal
	struct state* cursor = solution_path;
	struct state* temp;	

	//Iterate over the linked list
	while(cursor != NULL){
		//Assign temp to be cursor
		temp = cursor;

		//Advance cursor for the next round
		cursor = cursor->next;

		//Free the tiles in cursor
		destroy_state(temp);
		
		//Free temp altogether
		free(temp);
	}
}


