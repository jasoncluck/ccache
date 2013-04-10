#include "circBuffer.h"
/**
  * Constructor for the buffer:
  * Takes in the buffer object, length of the buffer (BUFFER_LENGTH)
  * and the size of the elements (int in this case)
**/
void buffer_init(CB_t *cb, int length, int sz){
	cb->length = length;
	cb->head = 0;
	cb->tail = 0;
	if(!(cb->buffer = (void*) calloc(length, sz))) {
		printf("Memory allocation failed for read/write buffer");
	}
}

/* push an integer on the data structure.  Appends it to the end of of the buffer.
Returns 1 if successful and 0 if the queue is full and the last index is being overwritten.
Not the most elegant solution but fixes the problem where head==tail is true initially so can't test on that for fullness */
int push(char *str, CB_t *cb){
	
	//check to see if buffer is full or empty.
	if((cb->head+1) % cb->length == cb->tail){
		//This means we are allocating data in the last available slot so allocate the data then show error message
		cb->buffer[cb->head] = str;		
		//printf("Queue is full, returning error value");
		return 0;
	}

	cb->buffer[cb->head] = str;		//add the new element to the buffer
	cb->head++; 					//increment head pointer
	cb->head %= cb->length;			//loop back around to index 0 if larger than array length

	return 1;
}

/* Pop the oldest element off the buffer.
Returns the oldest integer if successful and 0 if the queue is empty and erroneous data is being read */

char * pop(CB_t *cb){
	
	//check to see if the buffer is empty.
	if(cb->tail == cb->head){
		return "empty";
	}

	char * returnval = cb->buffer[cb->tail];
	//increment tail and loop around if index > array length
	cb->tail++;
	cb->tail %= cb->length;;
	return returnval;
}

//helper function to see if the queue is empty.  Should ONLY be used after a worker thread processes a request
int isEmpty(CB_t *cb){
	if(cb->head == cb->tail){
		return 1;
	}
	else return 0;
}