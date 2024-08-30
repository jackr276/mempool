# mempool
Author: [Jack Robbins](https://www.github.com/jackr276)

# Overview
**mempool** is a thread-safe, dynamic memory suballocation system that I made for my personal uses in an A* solver. It was made to address the slowdowns and heap fragmentation that repetitive calls to `malloc` and `free` can cause when called *tens or hundreds of thousands of times*, as happens in my A* solver. I figured that other people may have the same issues and need for a memory suballocation system, so I've decided to put the code up here for anyone to use in their own projects. I want to be very clear that this is not a `malloc` clone, and should not be used as a replacement to it. It is designed for a specific use case that satisfies the following conditions:
1. You will be allocating/freeing blocks that are all around the same size
2. You will be doing this tens or hundreds of thousands of times, to the point where the costs of `malloc` and `free` begin to buildup and cause slowdowns
3. You are not operating in a constrained memory environment, and potential memory waste is an acceptable tradeoff for faster performance
4. You know approximately how much overall memory at most your program will need during runtime

If these issues sound like something that your project is dealing with, then **mempool** may work well for you. 
