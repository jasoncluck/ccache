/* struct for the ccache requests (creqs) */
#pragma once

#include <stdint.h>
#include <stdlib.h>
#define KEY_SIZE 255
//enumeration for the basic commands
typedef enum { CSET = 1, CGET = 2, CDELETE = 3 } ccmd_t;
//errors: reference error, no error, error, client error, server error
typedef enum { RERROR = -1, NOERROR = 0, ERROR = 1, CERROR = 2, SERROR = 3 } error_t;

/* Structure for the request and the response.
   They are combined so no additional memory allocation is neccessary */

typedef struct creq {
	ccmd_t type;
	char key[KEY_SIZE];
	char *data;
	uint32_t flags, exp_time, bytes;

	struct cresp { 
		char *header, *footer;
		int head_sz, foot_sz;
		error_t errcode; //0 for successful, -1 for normal errors like element not found, 1 for command errors, 2 for client errors, 3 for server errors
	} resp;

	struct creq *next; //linked list node pointer

} creq_t;