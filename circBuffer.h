#pragma once

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Circular FIFO buffer data structure for storing char objects.  Supports push and pop operands. Uses head and tail pointers to determine
   if the queue is empty or full.  Should probably be made more generic for future use*/

//buffer code
typedef struct cb{
	int head;
	int tail;
	char ** buffer;	
	int length;
} CB_t;


void buffer_init(CB_t *cb, int length, int sz); //constructor
int push(char * str, CB_t *cb); 					//push element onto end of busffer using tail
char * pop(CB_t *cb); 								//pop the oldest element off the buffer using head
int isEmpty(CB_t *cb);							//return true if head==tail, note: this is also the condition for full so need to call this only after processing a request