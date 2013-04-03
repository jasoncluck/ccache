/* Doubly linked list declaration */
#pragma once

#include "ccache.h" /* for struct creq */
#include "llnode.h"

/* simple doubly linked list */
struct dlinked_list {
	node_t * head;
	node_t * tail;
};

int
add_node(struct dlinked_list * dll, node_t * new_node);

/* Iterate through the list starting at the head and delete the node */
int
delete_node(struct dlinked_list * dll, creq_t * creq);

int
get_node(struct dlinked_list *dll, creq_t * creq);

void remove_oldest_creq(struct creq_linked_list *dll);
