/*
	file: mythreads.c
	purpose: Implementation of process threading library using ucontext
		and list.h (a circular queue lib)
		
	author: Mark Todd
*/
#ifndef _mythreads
#include "mythreads.h"
#define _mythreads
#endif

#ifndef _listh
#include "list.h"
#define _listh
#endif
#include <stdio.h>
#include <stdlib.h>


//threadInit starts entire library, ignored on repeat calls


/* Global vars:
@var 	readyList: list of active threads, activated in round robin
		allLocks: stores all mutexes, max is NUM_LOCKS
		idInc: creates unique thread identifier for each thread
		interruptsAreDisabled: 1 if in library call, intended to prevent reentrancy
		swapAndYielded: temporarily holds a thread that's been moved out of readyList
		lib_inited: used by threadInit to ensure ignore repeated init calls
*/
List *readyList, *exitList, *lockWaiting, *conditionWaiting;
tlock allLocks[NUM_LOCKS];

int idInc = 0;
int interruptsAreDisabled = 0;
tblock *swapAndYielded = NULL;
int lib_inited = 0;

/* 	helper functions:
*	setupcontext: returns memory allocated, but empty, ucontext. Used for child threads
*	childFuncHandler: handler that calls threadExit on function return
*	myLock: functionally the code for threadLock, allows threadWait to call it "atomically"
*	myUnlock: functionally the code for threadUnlock, allows threadSignal to call it "atomically"
*/
ucontext_t *_setupcontext();
void *_childFuncHandler(tblock *thread, thFuncPtr funcPtr, void *argPtr);
void _myLock(int lockNum);
void _myUnlock(int lockNum);

extern void threadInit () {
	interruptsAreDisabled = 1;
	if(lib_inited) return;
	
	//create the lists
	readyList = init_List();
	exitList = init_List();
	lockWaiting = init_List();
	conditionWaiting = init_List();
	
	//create initial thread
	ucontext_t *main_context;
	tblock *main_thread = malloc(sizeof(tblock));
	main_thread->parent = NULL;
	main_context = malloc(sizeof(ucontext_t));
	if(getcontext(main_context)) perror("problem setting init context");
	main_thread->context = main_context;
	main_thread->tid = idInc;
	idInc++;
	enqueue_List(readyList, main_thread);
	setCurrent(readyList, main_thread);
	int j;
	for(j = 0; j < NUM_LOCKS; j++){
		allLocks[j]=init_Lock();
	}
	interruptsAreDisabled = 0;
}

// thread management functions
extern int threadCreate ( thFuncPtr funcPtr , void* argPtr ) {
	interruptsAreDisabled = 1;
	//create tblock, add to list
	ucontext_t *newContext = _setupcontext();
	tblock *newThread = malloc(sizeof(tblock));
	newThread->context = newContext;
	newThread->tid = idInc;
	newThread->lockWait = -1;
	newThread->conditionWait = -1;
	idInc++;
	makecontext(newContext, (void (*) (void))_childFuncHandler, 3, newThread, funcPtr, argPtr);
	
	//printf("Created thread with id: %d\n", newThread->tid);
	//we can safely assume "current thread" is parent
	newThread->parent = current(readyList);
	enqueue_List(readyList, newThread);
	//readyList->cur = newThread;
	threadYield();
	interruptsAreDisabled = 0;
	return newThread->tid;
}
extern void threadYield () {
	interruptsAreDisabled = 1;
	tblock *oldCur, *newCur;
	if(swapAndYielded){
		oldCur = swapAndYielded;
		swapAndYielded = NULL;
	} else {
		oldCur = current(readyList);	
		swapAndYielded = NULL;
	}
	newCur = moveToNext(readyList);
	if(oldCur != newCur){
		swapcontext(oldCur->context, newCur->context);
	}
	interruptsAreDisabled = 0;
}
extern void threadJoin ( int thread_id , void** result ) {
	interruptsAreDisabled = 1;
	tblock *waitFor= findThread(exitList, thread_id);
	
	while(waitFor == NULL ){
		interruptsAreDisabled = 0;
		threadYield();
		interruptsAreDisabled = 1;
		waitFor = findThread(exitList, thread_id);
	}
	*result = waitFor->result;
	if(!removeThread_List(exitList, waitFor)) {
		perror("problem in removing thread from list\n");
	}
	free(waitFor->context);
	free(waitFor);
	interruptsAreDisabled = 0;
}
extern void threadExit ( void* result ) {
	interruptsAreDisabled = 1;
	tblock *thread = readyList->cur;
	//have to handle situations where main thread calls threadExit()
	if(!removeThread_List(readyList, thread)) {
		perror("problem in removing thread from list\n");
	}
	if(thread->tid == 0){
		removeThread_List(readyList, thread);
		emptyAndFree_List(readyList);
		emptyAndFree_List(exitList);
		free(thread);
		int a = *(int*) result;
		exit(a);
	}
	thread->result = result;
	enqueue_List(exitList, thread);
	//tblock *next = thread->parent;
	//printf("swapping from :%d to parent :%d\n", thread->tid, thread->parent->tid);
	readyList->cur = thread->parent;
	swapcontext(thread->context, thread->parent->context);
	interruptsAreDisabled = 0;
}

// end of management
// synchronization functions

extern void threadLock (int lockNum ) {
	interruptsAreDisabled = 1;
	_myLock(lockNum);
	interruptsAreDisabled = 0;
}
extern void threadUnlock (int lockNum ) {
	interruptsAreDisabled = 1;
	_myUnlock(lockNum);
	interruptsAreDisabled = 0;
	
}
extern void threadWait (int lockNum , int conditionNum ) {
	interruptsAreDisabled = 1;
	_myLock(lockNum);
	while(allLocks[lockNum].conditionVal[conditionNum] <= 0){
		tblock* thread = readyList->cur;
		thread->lockWait = lockNum;
		thread->conditionWait = conditionNum;
		swapAndYielded = thread;
		switchList(readyList, conditionWaiting, thread);
		_myUnlock(lockNum);
		threadYield();
		interruptsAreDisabled = 1;
		_myLock(lockNum);
	}
	allLocks[lockNum].conditionVal[conditionNum] = -1;
	_myUnlock(lockNum);
	interruptsAreDisabled = 0;
}
extern void threadSignal (int lockNum , int conditionNum ) {
	interruptsAreDisabled = 1;
	_myLock(lockNum);
	
	if(allLocks[lockNum].conditionVal[conditionNum] <= 0){
		tblock *swapBackThread;
		allLocks[lockNum].conditionVal[conditionNum] = 1;
		swapBackThread = findThreadByCondition(conditionWaiting, lockNum, conditionNum);
		if(swapBackThread != NULL){
			swapBackThread->lockWait = -1;
			swapBackThread->conditionWait = -1;
			switchList(conditionWaiting, readyList, swapBackThread);
		}
	}
	
	//threadUnlock
	_myUnlock(lockNum);
	interruptsAreDisabled = 0;
}

// end of synchronization
// Thread Library's helper functions
ucontext_t *_setupcontext(){
	ucontext_t *ret = malloc(sizeof(ucontext_t));
	char *a_new_stack = malloc(STACK_SIZE);
	if(getcontext(ret)) perror("problem setting new context");

	ret->uc_stack.ss_sp = a_new_stack;
	ret->uc_stack.ss_size = STACK_SIZE;
	return ret;
}

// Function that handles child exits that return rather than exit
void *_childFuncHandler(tblock *thread, thFuncPtr funcPtr, void *argPtr){
	thread->result = funcPtr(argPtr);
	threadExit(thread->result);
	return NULL;
}

void _myLock(int lockNum) {
	while(allLocks[lockNum].curVal == 0){
		//add to waiting list
		tblock* thread = readyList->cur;
		thread->lockWait = lockNum;
		swapAndYielded = thread;
		switchList(readyList, lockWaiting, readyList->cur);
		threadYield(); //whatever thread it enters should always reset interrupts
		interruptsAreDisabled = 1; 
	}
	if(allLocks[lockNum].curVal > 0){
		allLocks[lockNum].curVal = 0;
	}
}

void _myUnlock(int lockNum) {
	if(allLocks[lockNum].curVal <= 0){
		tblock *swapBackThread;
		allLocks[lockNum].curVal = 1;
		swapBackThread = findThreadByLock(lockWaiting, lockNum);
		if(swapBackThread != NULL){
			swapBackThread->lockWait = -1;
			switchList(lockWaiting, readyList, swapBackThread);
		} else{
			swapBackThread = findThreadSignaledCondition(conditionWaiting, allLocks[lockNum]);
			if(swapBackThread != NULL){
				swapBackThread->lockWait = -1;
				swapBackThread->conditionWait = -1;
				switchList(conditionWaiting, readyList, swapBackThread);
			}
		}
	}
}
