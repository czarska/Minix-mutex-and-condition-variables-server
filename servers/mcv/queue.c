#include "queue.h"

queue * make_queue(int mutex_number)
{
	queue * q = malloc(sizeof(queue));
	q->owner = mutex_number;
	q->start = malloc(sizeof(node));
	q->start->next = NULL;
	return q;
}

void enqueue(queue *q, int number)
{
	node *temp = q->start;
	while(temp->next != NULL)
		temp = temp->next;
	node *n = malloc(sizeof(node));
	temp->next = n ;
	n->next = NULL;
	n->number = number;

}

int dequeue(queue *q) 
{
	node *n = q->start->next;
	q->start->next = n->next;
	int r = n->number;
	free(n);
	return r;
}

void remove_queue(queue *q)
{
	node *temp = q->start->next;
	while (temp != NULL)
	{
		node *n = temp;
		temp = temp->next;
		free(n);
	}
	free(q);
}

int is_empty(queue *q)
{
	if (q->start->next == NULL)
		return 1;
	else
		return 0;
}
int remove_pr(queue *q, int proc)
{
 	node *prev = q->start;
	node *temp = q->start->next;
	while(temp != NULL)
	{
		if (temp->number == proc)
		{
			prev->next = temp->next;
			node *rem = temp;
			free(rem);
			return 1;
		}
		prev = temp;
		temp = temp->next;
	}
	return 0;
}
