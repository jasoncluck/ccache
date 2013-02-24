/**	ccache.h 
  *	Cloud cache (cc) header file
  * Written by Jason Cluck (jcluck@gwmail.gwu.edu) and Gabe Parmer
  * 
**/

#include <stdio.h>
#include <assert.h>
#include <stdint.h>

//enumeration for the basic commands
typedef enum { CSET, CGET, CDELETE } ccmd_t;

/* Structure for the request and the response.
   They are combined so no additional memory allocation is neccessary */

struct creq {
	ccmd_t type;
	char key[250];
	char *data;
	uint32_t flags, exp_time, bytes;

	struct cresp { 
		char *header, *footer;
		int head_sz, foot_sz;
		int errcode; //returns 0 if successful
	};

};

/* Function to parse the string and put it into the structure above */
struct creq *ccache_req_parse(char *cmd, int cmdlen);

/* Fill in the rest of the creq struct */
int ccache_req_process(struct creq *r);

/* Populate the data, flags, and bytes */
int ccache_get(struct creq *r);

int ccache_set(struct creq *r);

int ccache_delete(struct creq *r);


/* Create the headers and footers to be sent (populating cresp) */
int ccache_resp_synth(struct creq *req);

/* Actually serialize the headers/footers/and the data */
int ccache_resp_send(struct creq *req);

/* This might need to decrement reference counts in the actual data itself */
int ccache_req_free(struct creq *r) 