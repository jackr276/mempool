# Author: Jack Robbins
# This make file is for the uses of testing and also serves as a demo for how to 
# use makefile with mempool

CC = gcc
PROGS = mempool_demo
CFLAGS = -pthread -Wall -Wextra
# You need to pull in mempool to the compilation
INCLUDE = ./src/mempool/mempool.c ./src/demo-program/npuzzle/puzzle/puzzle.c ./src/demo-program/npuzzle/solver/solve_multi_threaded.c
# Compilation directory
OUT = ./out

all: $(PROGS)

mempool_demo: # FIXME
	$(CC) $(CFLAGS) ./src/demo-program/demo.c $(INCLUDE) -o $(OUT)/test

test:
	$(OUT)/test < ./test/test_args.txt

clean:
	rm -r $(OUT)/*
