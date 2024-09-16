# mempool
Author: [Jack Robbins](https://www.github.com/jackr276)

# Overview
**mempool** is a thread-safe, dynamic memory suballocation system that I made for my personal uses in an A* solver. It was made to address the slowdowns and heap fragmentation that repetitive calls to `malloc` and `free` can cause when called *tens or hundreds of thousands of times*, as happens in my A* solver. I figured that other people may have the same issues and need for a memory suballocation system, so I've decided to put the code up here for anyone to use in their own projects. I want to be very clear that this is not a `malloc` clone, it is not identical to the way which `malloc` works, and should not be used as a replacement to it. It is designed for a specific use case that satisfies the following conditions:
1. You will be allocating/freeing blocks that are all around the same size
2. You will be doing this tens or hundreds of thousands of times, to the point where the costs of `malloc` and `free` begin to buildup and cause slowdowns
3. You are not operating in a constrained memory environment, and potential memory waste is an acceptable tradeoff for faster performance
4. You know approximately how much overall memory at most your program will need during runtime

If these issues sound like something that your project is dealing with, then **mempool** may work well for you.

## mempool API details
There are 6 functions exposed to the user by the `malloc.h` header file. Each of these functions is detailed below:

1.) mempool_init
```c
mempool_t* mempool_init(u_int32_t size, u_int32_t default_block_size)
```
THREAD SAFE: NO

The initializer function takes in the overall `size` of the mempool and the `default_block_size` to facilitate the initial creation of the memory pool. It is very important that you choose both of these parameters wisely. If you make the `size` too small, you will run out of space. **mempool does not resize once created.** This is something that I may change later on, but as of right now, once you make the mempool, you are stuck with that overall size. It is recommended that you be very very liberal with the size. The macros `KILOBYTE`, `MEGABYTE`, and `GIGABYTE` are included for convenience. The `default_block_size` is also very important to the overall performance of the memory pool. This size is meant to be the size of the blocks that you'll be allocating over and over again. If this size is too small, the memory pool won't crash, but it will **coalesce** blocks to get the size that you need. This operation is expensive, and the entire point of this tool is to avoid the need to do this. This function will return a pointer to the mempool that was created. **If you lose this pointer, you will have a memory leak as you will not be able to destroy this mempool**. This `mempool_t*` is essential as it will be passed into all the other functions so that you can specify the specific mempool that you want to allocate to/free from.

2.) mempool_destroy
```c
int mempool_destroy(mempool_t* mempool)
```
THREAD SAFE: NO

The destructor function destroys the entire memory pool. Once down, any pointers that were malloc'd become invalid because the large pool that they point to is now gone, so it is important that you only call this function when you are absolutely sure that you want to get rid of the entire memory pool. **mempool_destroy** is guaranteed to completely clean up the memory allocated by **mempool_init**, so it is impossible to memory leak with mempool. The paramter is the pointer to the mempool struct that you would like to teardown.

3.) mempool_alloc
```c
void* mempool_alloc(mempool_t* mempool, u_int32_t num_bytes)
```
THREAD SAFE: YES

The allocation function allocates a chunk of memory of size `num_bytes` and returns a pointer to the start of that allocated region to the user. If you run out of space, mempool_alloc will return `NULL`. This function is thread safe with regards to the mempool pointer.

4.) mempool_calloc
```c
void* mempool_calloc(mempool_t* mempool, u_int32_t num_members, size_t size)
```
THREAD SAFE: YES

The `mempool_calloc` function allocates a chunk of memory that can hold `num_members`, each being `size` bytes. `mempool_calloc` also sets all bytes to 0 in the allocated block. Just like `mempool_alloc`, it returns a pointer to the first byte of the allocated region. This function is thread safe with regards to the mempool pointer.

5.) mempool_realloc
```c
void* mempool_realloc(mempool_t* mempool, void* ptr, u_int32_t num_bytes)
```
THREAD SAFE: YES

The `mempool_realloc` function reallocates a chunk of previously allocated memory pointed to by `ptr` to be of size `num_bytes`. `mempool_realloc` does not support downsizing, so `num_bytes` must always be larger than the previous size of the allocated block. This function is thread safe.

6.) mempool_free
```c
void mempool_free(mempool_t* mempool, void* ptr)
```
THREAD SAFE: YES

The `mempool_free` function releases the memory chunk that is pointed to by `ptr`. If `ptr` was never allocated, the function will print a debug message alerting the user about a potential double free. `mempool_free` automatically defragments the allocated memory, leading to improvements over regular free. This function is thread safe with regards to the mempool pointer. 

## Efficiency of mempool
**mempool** is not *that* different from how malloc works, but it does reduce system calls. As said in the introduction, the use of this tool will only cause a noticeable speedup if you are malloc'ing and free'ing tens or hundreds of thousands of times. The main advantage of this tool is in the reduction of system calls/memory page access, and the elimination of heap fragmentation. I've included the asymptotic time complexity below:

- Allocating: O(1) for allocating a block less than or equal to `default_block_size`, O(n) for allocating a block greater than `default_block_size`, because of the need for coalescing of blocks.
- Releasing: O(n) for freeing blocks. This O(n) comes from the automatic defragmentation of the block allocated when `malloc_free` is called.

## Using mempool
**mempool** is designed to be used intelligently by a user who has a deep understanding of the memory needs and characteristics of their project, and gives you all the power. As such, making the right choices when it comes to default block size is very important. There is a [demo branch](https://github.com/jackr276/mempool/tree/demo) that shows the use of mempool in an A* solver. I would encourage anyone wishing to use it to look at the specific places where mempool is used, and the places where it is not used. The main branch just contains the header file `mempool.h` and the implementation file `mempool.c` that need to be included in your project if you wish to use mempool. `mempool.c` will need to be on the build path if you have it in your project. The [Makefile](https://github.com/jackr276/mempool/blob/main/Makefile) shows examples of how to include the program with make, and the example program [main.c](https://github.com/jackr276/mempool/blob/main/src/main.c) shows some very simple examples on usage.
