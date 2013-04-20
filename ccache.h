/**	ccache.h 
  *	Cloud cache (cc) header file
  * Written by Jason Cluck (jcluck@gwmail.gwu.edu) and Gabe Parmer
  * 
**/
#pragma once

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <stdint.h>
#include <string.h>
#include <pthread.h>
#include <semaphore.h>
#include <unistd.h>
#include <time.h>
/* ccache native */
#define MAX_CMD_SZ (1<<10)

#include "creq.h"

/* cvect stuff */
#define COS_LINUX_ENV
#define LINUX_TEST
#include "cvect.h"

/* Concurrency */
#define MAX_CONCURRENCY 1

/* linked list */
#include "creqlinkedlist.h"

/* Command buffer */
#include "circBuffer.h"
#define BUFFER_LENGTH 256

/* Turn debugging on */
#define DEBUG 0




/* Constructor for initialization.  For cvect initialization mainly */
cvect_t *
ccache_init(void);

/* Hashing function: Takes in an value and produces a key for that value to be mapped to. Key < 2^20 due to cvect contraints.
Hashing function is djb: http://www.cse.yorku.ca/~oz/hash.html*/
long 
hash(char *str);

/* Function to parse the string and put it into the structure above */
creq_t *
ccache_req_parse(char *cmd);


/* Populate the data, flags, and bytes */
int 
ccache_get(creq_t *creq);

int 
ccache_add_set_data(char *creq_data);


int 
ccache_set(creq_t *creq);

int 
ccache_delete(creq_t *creq);


/* Create the headers and footers to be sent (populating cresp) */
int 
ccache_resp_synth(creq_t *creq);

/* Actually serialize the headers/footers/and the data */
int 
ccache_resp_send(creq_t *creq);

/* This might need to decrement reference counts in the actual data itself */
int 
ccache_req_free(creq_t *creq); 

/* Function to free up the cvect data structure - only called at the end of main */
void 
cvect_struct_free(cvect_t *dyn_vect);


/* cvect function */
int 
in_pairs(struct pair *ps, int len, long id);

void *
do_lookups(struct pair *ps, cvect_t *v);

/* concurrency functions */
void *
thread_start(void *);

/* add the request from the main thread to a buffer for a worker thread to pick up */
int 
add_req_to_buffer(int socketfd, char * str);

/* initialize thread pool, mutexes, semaphores */
int 
thread_pool_init();

/* initialize circular buffer for command */
int 
command_buffer_init();


