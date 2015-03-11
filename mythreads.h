/*
	file: mythreads.h
	purpose: process threading library
	
	author: Mark Todd (well, this was provided file by Dr. Sorber)
*/

#include <ucontext.h>
#include <stdio.h>
#include <stdlib.h>

#define STACK_SIZE (16*1024)
#define NUM_LOCKS 10
#define CONDITIONS_PER_LOCK 10

// the type of function used to run your threads
typedef void *(* thFuncPtr ) ( void *) ;

extern void threadInit () ;

// thread management functions
extern int threadCreate ( thFuncPtr funcPtr , void* argPtr ) ;
extern void threadYield () ;
extern void threadJoin ( int thread_id , void** result ) ;
extern void threadExit ( void* result ) ;

// synchronization functions
extern void threadLock (int lockNum ) ;
extern void threadUnlock (int lockNum ) ;
extern void threadWait (int lockNum , int conditionNum ) ;
extern void threadSignal (int lockNum , int conditionNum ) ;

// control atomicity
extern int interruptsAreDisabled;
