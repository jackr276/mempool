/**
 * Author: Jack Robbins
 * This C file contains code to run the demo N-Puzzle solver program for mempool
 */


/**
 * Makes the need calls to the solver to generate a random puzzle and solve
 */
#include <stdio.h>
#include <sys/types.h>

//Link to the NPuzzle solver and puzzle generator
#include "./npuzzle/puzzle/puzzle.h"
#include "./npuzzle/solver/solve.h"


/**
 * Get appropriate user input and make calls to the needed APIs in puzzle and solver
 */
int main(void){
	//Store N and the comlexity
	u_int32_t N;
	u_int32_t complexity;

	//Grab N from the user
	printf("Enter a value for N: ");
	scanf("%u", &N);
	printf("\n");

	//Grab the intial complexity
	printf("Enter the initial complexity: ");
	scanf("%u", &complexity);
	printf("\n");

	printf("Generating an N-Puzzle with N = %u and initial complexity = %u\n", N, complexity);

	//Generate the random puzzle
	
}
