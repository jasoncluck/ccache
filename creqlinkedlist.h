/* Doubly linked list declaration */
#pragma once
#include <string.h>
#include "creq.h" /* for struct creq */

/* simple doubly linked list */
struct creq_linked_list {
	creq_t * head;
	creq_t * tail;
};

int
add_creq(struct creq_linked_list * dll, creq_t * new_creq);

/* Iterate through the list starting at the head and delete the node */
int
delete_creq(struct creq_linked_list * dll, creq_t * creq);

int
get_creq(struct creq_linked_list *dll, creq_t * creq);
