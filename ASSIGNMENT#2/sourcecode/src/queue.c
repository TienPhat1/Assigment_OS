#include <stdio.h>
#include <stdlib.h>
#include "queue.h"

int empty(struct queue_t * q) {
	return (q->size == 0);
}

void enqueue(struct queue_t * q, struct pcb_t * proc) {
	/* TODO: put a new process to queue [q] */
	if(q->size != MAX_QUEUE_SIZE){
		q->proc[q->size] = proc;
		q->size++;
	}	
}

struct pcb_t * dequeue(struct queue_t * q) {
	if(q->size != 0){
		struct pcb_t * temp;
		for (int i = 0; i<=(q->size -2); i++){
			if(q->proc[i]->priority > q->proc[i+1]->priority){
				temp = q->proc[i];
				q->proc[i] = q-> proc[i+1];
				q->proc[i+1] = temp;
			}
		}
		temp = q->proc[q->size-1];
		q->size --;
		return temp;
	}
	else	return NULL;
}

