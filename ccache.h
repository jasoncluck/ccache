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

/* cvect stuff */
#define COS_LINUX_ENV
#define LINUX_TEST
#include "cvect.h"


struct pair {
	long id;
	void *val;
};

//enumeration for the basic commands
typedef enum { CSET = 1, CGET = 2, CDELETE = 3 } ccmd_t;

/* Structure for the request and the response.
   They are combined so no additional memory allocation is neccessary */

typedef struct creq {
	ccmd_t type;
	char key[250];
	char *data;
	uint32_t flags, exp_time, bytes;

	struct cresp { 
		char *header, *footer;
		int head_sz, foot_sz;
		int errcode; //returns 0 if successful
	} resp;

} creq_t;

/* Constructor for initialization.  For cvect initialization mainly */
cvect_t *ccache_init(void);

/* Hashing function: Takes in an value and produces a key for that value to be mapped to. Key < 2^20 due to cvect contraints.
Hashing function is djb: http://www.cse.yorku.ca/~oz/hash.html*/
long hash(char *str);

/* Function to parse the string and put it into the structure above */
creq_t *ccache_req_parse(char *cmd, int cmdlen);


/* Populate the data, flags, and bytes */
int ccache_get(creq_t *creq);

int ccache_set(creq_t *creq);

int ccache_delete(creq_t *creq);


/* Create the headers and footers to be sent (populating cresp) */
int ccache_resp_synth(creq_t *creq);

/* Actually serialize the headers/footers/and the data */
int ccache_resp_send(creq_t *creq);

/* This might need to decrement reference counts in the actual data itself */
int ccache_req_free(creq_t *creq); 

/* Function to free up the cvect data structure - only called at the end of main */
void cvect_struct_free(cvect_t *dyn_vect);