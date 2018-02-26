#ifndef _QUEUE_H
#define _QUEUE_H

#include <string.h>

typedef struct {
	int number;
        struct node *next;
} node;

typedef struct{
	int owner; //mutex number
	node * start;
} queue;

queue* make_queue(int number);
void enqueue(queue *q, int number); //put on end
int dequeue(queue *q); //return first
void remove_queue(queue *q);
int is_empty(queue *q);
int remove_pr(queue *q, int proc);

#endif
