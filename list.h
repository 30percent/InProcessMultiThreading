/*
	file: list.h
	purpose: Circular queue list for thread nodes.
	IMPORTANT: this is *definitely* not reentrancy *or* thread safe. It is specifically
		designed for single call access. Expect errors if threads are not called
		in mutual exclusion.
		
	author: Mark Todd
*/

#include <ucontext.h>
#ifndef _mythreads
#include "mythreads.h"
#define _mythreads
#endif

/* tlock - mutual exclusion lock
*@var	curVal: 1 if available, 0 if unavailable
*		conditionVal: array of "conditions", 1 is signaled, 0 if not
*/
typedef struct t_lock{
	int curVal;
	int conditionVal[CONDITIONS_PER_LOCK];
} tlock;

/* tblock - thread management block
*DEPENDS: entirely built around list functionality
*NOTES: could move "list" variables to holder for more modal object
*@var	prev,next: stores previous and next node in list respectively
*		lockWait: id of tlock it is waiting on
*		conditionWait: id of condition within lockWait it is waiting on
*		parent: thread of creator thread (currently unused)
*		tid: unique thread id
*		context: current "ucontext" where thread was last yielded
*		result: holds 'return' value, main thread must be int value
*/
typedef struct thread_a{
	struct thread_a *prev;
	struct thread_a *next;
	
	int lockWait;
	int conditionWait;
	
	struct thread_a *parent;
	int tid;
	
	ucontext_t *context;
	void *result;
} tblock;

/* List - a circular list with null head and tail nodes
* DEPENDS: tblock struct as node
* NOTES: if node struct was abstracted, this could easily be modal
*@var	head: empty node with no previous
*		tail: empty node with no next node
*		cur: node (last accessed), unless list is empty, will always 
*			point to valued node
*/
typedef struct a_list{
	tblock *head;
	tblock *tail;
	tblock *cur;
} List;

List *init_List();
tlock init_Lock();

/* enqueue_List - adds node to tail of list, if list is empty, sets node to cur
*@vars	t: node to be added to end of list
*/
void enqueue_List(List *l, tblock *t);

/* dequeue_List - removes and returns first node of list
*@return	returns first node in list, if list is empty returns NULL
*/
tblock *dequeue_List(List *l);

/* isEmpty_List - returns 1 if list is empty, 0 if not
*/
int isEmpty_List(List *l);
tblock *current(List *l);
void setCurrent(List *l, tblock *t);

/* moveToNext - sets current to the current's next entry in list
*@return	the new current of the list:
*				if at end of list, returns the start of list
*				if list empty returns NULL,
*				if only one node, returns that node
*/
tblock *moveToNext(List *l);

/* emptyAndFree_List - frees the contents of the list
* NOTES: *does not* free head and tail nodes
* DEPENDS: freeTBlock for freeing contents
*/
void emptyAndFree_List(List *l);
/* freeTBlock - frees context, result, then finally the struct
*/
void freeTBlock(tblock *t);

/* removeThread_List - safely removes node from list
*@var	t: thread to be removed
*@return	returns 0 if thread was not in list or if list is empty
*/
int removeThread_List(List *l, tblock *t);
/* switchList - safely moves node from a list to another
*DEPENDS: removeThread_List, enqueue_List
*@var	from: list node is to be removed from
*		to: list node is to be moved to
*		t: node to be moved
*/
void switchList(List *from, List *to, tblock *t);

/* list searcher functions
* Thread uses node's unique id
* ThreadByLock searches list for node "waiting" on lock, requires no conditionWaiting
* ThreadByCondition searches list for node "waiting" on a lock's specific condition
* findThreadSignaledCondition searches list for any node waiting on a lock where
*		the condition has been "signaled"
*/
tblock *findThread(List *l, int tid);
tblock *findThreadByLock(List *l, int lock);
tblock *findThreadByCondition(List *l, int lock, int cond);
tblock *findThreadSignaledCondition(List *l, tlock lock);