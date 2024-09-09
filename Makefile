# Author: Jack Robbins
# This make file is for the uses of testing and also serves as a demo for how to 
# use makefile with mempool

CC = gcc
PROGS = mempool_demo
CFLAGS = -Wall -Wextra
# You need to pull in mempool to the compilation
INCLUDE = ./src/mempool/mempool.c
# Compilation directory
OUT = ./out

all: $(PROGS)

mempool_demo:
	$(CC) $(CFLAGS) ./src/main.c $(INCLUDE) -o $(OUT)/test

test:
	$(OUT)/test

clean:
	rm -r $(OUT)/*
