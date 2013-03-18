/* Doubly linked list implementation */

#include "dlinkedlist.h"

int
add_node(struct dlinked_list *dll, node_t * new_node){
	if(dll->head == NULL){
		dll->head = new_node;
		dll->tail = new_node;
	}
	else{
		new_node->next = dll->head; //set the current head to be the second node in the list
		dll->head = new_node; //now change the head to be the new node (most recent)
	}
	return 0;
}

int
delete_node(struct dlinked_list *dll, creq_t * creq){
	while(1){
		if(dll->head->next != NULL && !strcmp(dll->head->next->creq->key, creq->key)){
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
get_node(struct dlinked_list *dll, creq_t * creq){
	while(1){
			if(!(strcmp(dll->head->creq->key, creq->key))){
				creq = dll->head->creq; 
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