/* Doubly linked list implementation */

#include "creqlinkedlist.h"

void
init_linked_list(struct creq_linked_list *dll){
	dll->head = NULL;
	dll->tail = NULL;
}

int
add_creq(struct creq_linked_list * dll, creq_t * new_creq){
	if(dll->head == NULL){
		dll->head = new_creq;
		dll->tail = new_creq;
	}
	else{
		new_creq->next = dll->head; //set the current head to be the second node in the list
		dll->head = new_creq; //now change the head to be the new node (most recent)
	}
	return 0;
}

int
delete_creq(struct creq_linked_list *dll, creq_t * creq){
	while(1){
		if(dll->head->next != NULL && !strcmp(dll->head->next->key, creq->key)){
			if(dll->head->next->next != NULL){
				dll->head->next = dll->head->next->next; //change the pointers
			}
			else{
				//this is the last node in the list
				dll->head->next = NULL;
			}
			break;
		}
		else if(dll->head->next != NULL) dll->head = dll->head->next;
		else{
			return -1;
		}
	}
	return 0;
}

int
get_creq(struct creq_linked_list *dll, creq_t * creq){
	while(1){
			if(!(strcmp(dll->head->key, creq->key))){
				creq = dll->head; 
				creq->type = CGET;
				break;
			}
			else if(dll->head->next == NULL){
				creq->resp.errcode = RERROR;
				break;
			} 
			else dll->head = dll->head->next;
		}
	return 0;
}