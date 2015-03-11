/*
	file: list.c
	purpose: Definitions for Circular queue list
*/
#ifndef _listh
#include "list.h"
#define _listh
#endif
#include <stdio.h>
#include <stdlib.h>

List *init_List()
{
	List *l = malloc(sizeof(List));
	tblock *head = malloc(sizeof(tblock));
	tblock *tail = malloc(sizeof(tblock));
	head->tid = -1;
	tail->tid = -2;
	head->next = tail;
	tail->prev = head;
	l->head = head;
	l->tail = tail;
	l->cur = l->head;
	
	return l;
}

tlock init_Lock(){
	tlock myL;
	myL.curVal = 1;
	int i;
	for(i = 0; i < CONDITIONS_PER_LOCK; i++){
		myL.conditionVal[i] = -1;
	}
	return myL;
}


//add to end of list
void enqueue_List(List *l, tblock *t)
{
	if(isEmpty_List(l)){
		l->cur = t;
	}
	tblock *end;
	end = l->tail->prev;
	end->next = t;
	t->prev = end;
	t->next = l->tail;
	l->tail->prev = t;
}

tblock *dequeue_List(List *l)
{
	tblock *ret;
	
	if(isEmpty_List(l)){
		return NULL;
	}
	
	ret = l->head->next;
	ret->next->prev = l->head;
	l->head->next = ret->next;
	
	return ret;
}

void emptyAndFree_List(List *l){
	tblock *torem;
	while(!isEmpty_List(l)){
		l->cur = l->head->next;
		torem = l->cur;
		removeThread_List(l, l->cur);
		freeTBlock(torem);
	}
}

void freeTBlock(tblock *t){
	free(t->context);
	free(t->result);
	free(t);
}

int isEmpty_List(List *l){
	if(l->head->next == l->tail){
		return 1;
	}
	return 0;
}

tblock *current(List *l){
	if(isEmpty_List(l)) l->cur = NULL;
	return l->cur;
}

tblock *moveToNext(List *l){
	if(isEmpty_List(l)){
		return NULL;
	}
	//in theory, if just init, pointing at head
	//it will move to head->next always.
	if(l->cur == l->tail->prev || l->cur == l->tail
		|| l->cur == l->head) {
		l->cur = l->head->next;
	} else {
		l->cur = l->cur->next;
	}
	return l->cur;
}

tblock *findThread(List *l, int tid){
	tblock *cur = l->head->next;
	while(cur != NULL && cur->tid != tid){
		cur = cur->next;
	}
	return cur;
}

tblock *findThreadByLock(List *l, int lock){
	return findThreadByCondition(l, lock, -1);
}

tblock *findThreadByCondition(List *l, int lock, int cond){
	tblock *cur = l->head->next;
	while(cur != NULL && cur->lockWait != lock && cur->conditionWait != cond){
		cur = cur->next;
	}
	return cur;
}

tblock *findThreadSignaledCondition(List *l, tlock lock){
	tblock *cur = l->head->next;
	while(cur != NULL){
		if(cur->lockWait == lock.curVal){
			if(lock.conditionVal[cur->conditionWait] == 1){
				return cur;
			}
		}
		cur = cur->next;
	}
	return cur;
}

int removeThread_List(List *l, tblock *t){
	if(findThread(l, t->tid) == NULL) return 0;
	if(l->cur == t) moveToNext(l);
	if(isEmpty_List(l)) return 0;
	
	t->prev->next = t->next;
	t->next->prev = t->prev;
	
	t->prev = NULL;
	t->next = NULL;
	return 1;
}

void setCurrent(List *l, tblock *t){
	l->cur = t;
}

void switchList(List *from, List *to, tblock *t){
	if(!removeThread_List(from, t)) {
		perror("problem in removing thread from list\n");
	}
	enqueue_List(to, t);
}

// end list