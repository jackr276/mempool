# Author: Jack Robbins
# Testing script for the memory pool

#!/bin/bash

# make the out directory
if [[ ! -d ./out ]]; then 
	mkdir out
fi

# wipe it clean
rm -r out/*


gcc -pthread -Wall -Wextra ./src/main.c ./src/mempool/mempool.c -o ./out/main

./out/main
