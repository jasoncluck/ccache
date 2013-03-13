/* linked list node declaration.  Might use sentinel nodes later */
#pragma once 
#include <stdio.h>

#include "ccache.h" /* for struct creq */

typedef struct node {
	struct node *next;
	struct creq *creq;
} node_t;


