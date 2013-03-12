#include "ccache.h"

#define DEBUG 0

/* globals */
struct pair pairs[1<<10];
cvect_t *dyn_vect;
int pairs_counter;

/* function to see if an id is already in a pair */
int in_pairs(struct pair *ps, int len, long id)
{
	for (; len >= 0 ; len--) {
		if (ps[len].id == id) return 1;
	}
	return 0;
}

/* I separate this out so that we can easily confirm that the compiler
 * is doing the proper optimizations. */
void *do_lookups(struct pair *ps, cvect_t *v)
{
	return cvect_lookup(v, ps->id);	
}

/* Constructor */
cvect_t *ccache_init(void){
	dyn_vect = cvect_alloc();
	assert(dyn_vect);
	cvect_init(dyn_vect);
	return dyn_vect;
}


/* Hashing function: Takes in an value and produces a key for that value to be mapped to. Key < 2^20 due to cvect contraints.
Hashing function is djb: http://www.cse.yorku.ca/~oz/hash.html*/
long hash(char *str){
	unsigned long hash = 5381;
    int c;

    while ((c = *str++)){
        hash = ((hash << 5) + hash) + c; /* hash * 33 + c */
    }
    //modulo for cvect
    hash %= CVECT_MAX_ID;
    return hash;
}


/* The Following three functons populate the data. Initial parsing as already occurred */
int ccache_get(creq_t *creq){
	/* Each item sent by the server will look like: VALUE <key> <flags> <bytes> [<cas unique>]\r\n
<data block>\r\n */

	
	//create a new pair and put the key in it, the cvect call will fill in the value
	struct pair getpair;
	getpair.id = hash(creq->key);	
	//now typecast the void pointer returned by the cvect
	//TODO:shouldn't need to lock here, just reading but double check this
	void * lookup_result;
	if((lookup_result = do_lookups(&getpair, dyn_vect)) != 0){
		node_t *head = (node_t *) lookup_result; //if non-zero we can typecast this without seg faulting	
		creq->resp.errcode = 0;
		while(1){
			//if the keys are equal, we have the element so stop iteration
			if(!(strcmp(head->creq->key, creq->key))){
				creq = head->creq; //overwrite the passed in creq with the data from the head node's cvect
				creq->type = CGET; //just set the type back to GET
				break;
			}
			else if(head->next == NULL){
				creq->resp.errcode = -1;
				break;
			} 
			else head = head->next;
		}
		//if head->creq->key != creq->key, we didn't find the value so return an error
		
	}
	else{
		//Data was not found so set the error code flag and nothing will be sent back
		creq->resp.errcode = -1;
	}	

	#if DEBUG
		printf("\nDEBUG SECTION OF GET\n");
		printf("type: %i\n", creq->type);
		printf("key: %s\n", creq->key);
		printf("flags: %i\n", creq->flags);
		printf("exp_time: %i\n", creq->exp_time);
		printf("bytes: %i\n", creq->bytes);
		printf("data: %s\n", creq->data);
		printf("END GET DEBUG\n\n");
	#endif /* DEBUG */

	ccache_resp_synth(creq);
	ccache_resp_send(creq);
	
	return 0;
}

int ccache_set(creq_t *creq){
	printf("Data line to be cached: ");
	//use temp string to hold scanf data then copy it to the struct
	char *temp_data = (char * ) malloc(creq->bytes);
	int input_success = scanf("%s", temp_data);
	if(!input_success){
		printf("Input error - quitting");
		exit(1);
	}
	strcpy(creq->data, temp_data);
	
	/* Get the hash result of the key */
	long hashedkey = hash(creq->key);

	//TODO: Store the data here in some type of data structure
	//Lock goes here before adding data to the cvect - needed for cvect_counter and dyn_vect
	pairs[pairs_counter].id = hashedkey; //set the id to be the key returned from the hash function
	pairs[pairs_counter].val = malloc(sizeof(node_t)); //malloc space for the pointer to the node object
	/* create the node, add the data to the node, and store it in the cvect */
	node_t *insert_node = (node_t *) malloc(sizeof(node_t));
	node_t *head = (node_t *) malloc(sizeof(node_t));
	insert_node->creq = creq;
	insert_node->next = NULL;
	memcpy(&pairs[pairs_counter].val, &insert_node, sizeof(node_t)); //store the entire creq in the cvect

	/* check to see if this key already exists, if it doesn't: add it. If it does exist, resolve linked node collision */
	void *lookup_result;
	if((lookup_result = do_lookups(&pairs[pairs_counter], dyn_vect)) != 0){
		head = (node_t *) lookup_result; //if non-zero we can now typecast
		while(1){
			if(!(strcmp(insert_node->creq->key, head->creq->key))) {
				creq->resp.errcode = cvect_add_id(dyn_vect, pairs[pairs_counter].val, pairs[pairs_counter].id);
			}
			if(head->next == NULL) break;
			else head = head->next;

		}
		//at this point head.next == null so set head.next to the insert node
		head->next = insert_node;
	}
	else{
		//add the pair to the cvect
		creq->resp.errcode = cvect_add_id(dyn_vect, pairs[pairs_counter].val, pairs[pairs_counter].id);
	}	

	assert(!creq->resp.errcode); //if no errors: creq->resp.errcode == 0;
	pairs_counter++;
	//Release Lock goes here
	

	ccache_resp_synth(creq);
	ccache_resp_send(creq);
	
	return 0;

}

int ccache_delete(creq_t *creq){

	struct pair deletepair;
	deletepair.id = hash(creq->key);	
	
	//add lock here
	assert(!cvect_del(dyn_vect, deletepair.id));
	//release lock
	//search for creq->key in the hash table, if it exists delete it
	ccache_resp_synth(creq);
	ccache_resp_send(creq);
	ccache_req_free(creq);

	return 0;
}


//Commands look like: <command name> <key> <flags> <exptime> <bytes> [noreply]\r\n

/* Function to parse the string and put it into the structure above */
creq_t *ccache_req_parse(char *cmd, int cmdlen){
	//Command is passed as a string along with its length
	//Can start by tokenizing this data, and determining what type of command it is.

	creq_t *creq = (creq_t *) malloc(sizeof(creq_t));
	printf("%s\n", cmd);
	char * pch;
	pch = strtok(cmd, " ");
	int counter = 0;
	while(pch != NULL){
		
		/* first find out what command the first token is */
		if(counter == 0){

			if(strcmp(pch, "get") == 0) creq->type = CGET;
			if(strcmp(pch, "set") == 0) creq->type = CSET;
			if(strcmp(pch, "delete") == 0) creq->type = CDELETE;
		}
		/* second token will always be the key */
		else if(counter == 1){
			//key is an array, pch is a pointer so must use strcopy here
			strcpy(creq->key, pch);
		}
		else{
			switch(creq->type){
				/* get <key>*\r\n */
				case CGET:
					/* if we end up here there are more than one get requests
					so process the current one and assemble the new one */
					ccache_get(creq);
					strcpy(creq->key, pch); //update request with the next key
					break;
				/* set <key> <flags> <exptime> <bytes> [noreply]\r\n */
				case CSET:
		 			if(counter == 2){
		 				creq->flags = atoi(pch);
		 			}
		 			else if(counter == 3){
						creq->exp_time = atoi(pch);
					}
		 			else if(counter == 4){
		 				creq->bytes = atoi(pch);
		 				creq->data = (char *) malloc(creq->bytes);
		 			}
		 		/* delete <key> [noreply]\r\n */
				case CDELETE:
					break;
				default:
					printf("Invalid command type");
					break;
			}
		}
		// //get the next token and update the token counter
		pch = strtok(NULL, " ");
		counter++;
		//TODO: Now call ccache_req_process(creq_t *r) to fill in the data portion and the rest of the struct
	}

	/* Debug info */
	#if DEBUG
	switch(creq->type){
		case CGET:
			printf("type: %i\n", creq->type);
			printf("key: %s\n", creq->key);
			break;	
		case CSET:
			printf("type: %i\n", creq->type);
			printf("key: %s\n", creq->key);
			printf("flags: %i\n", creq->flags);
			printf("exp_time: %i\n", creq->exp_time);
			printf("bytes: %i\n", creq->bytes);
			break;
		case CDELETE:
			printf("type: %i\n", creq->type);
			printf("key: %s\n", creq->key);
			break;
	}
	#endif /* DEBUG */
	

	/* Now that the tokens are done - continue the processing by calling the respective functions */
	if(creq->type == CGET){
		ccache_get(creq);
		//This is definitely the last GET to be processed regardless of # of input tokens so send END
		printf("END\r\n");
	}
	else if(creq->type == CSET) {
		ccache_set(creq);
	}
	else if(creq->type == CDELETE){ 
		ccache_delete(creq);
	}

	printf("\n\n");

	return creq;
}

int ccache_resp_synth(creq_t *creq){
	switch(creq->type){
		case CSET:
			// Configure the header based on whether the data was saved successfully or not 
			creq->resp.header = (char * ) malloc(16);
			if(!creq->resp.errcode) creq->resp.head_sz = sprintf(creq->resp.header, "STORED\r\n");
			else creq->resp.head_sz = sprintf(creq->resp.header, "NOT_STORED\r\n");
			creq->resp.footer = "";
			creq->resp.foot_sz = 0;
			break;
		case CGET:
			// Header should be: VALUE <key> <flags> <bytes> [<cas unique>]\r\n 
			// Footer should be the data block
			
			creq->resp.header = (char * ) malloc(1<<8);
			creq->resp.footer = (char * ) malloc(creq->bytes);
			//only populate the fields if no errors were encountered
			if(!creq->resp.errcode){
				creq->resp.head_sz = sprintf(creq->resp.header, "VALUE %s %d %d \r\n", creq->key, creq->flags, creq->bytes);
				creq->resp.foot_sz = sprintf(creq->resp.footer, "%s\r\n", creq->data);
			}
			break;
		case CDELETE:
			creq->resp.header = (char * ) malloc(16);	
			if(!creq->resp.errcode) creq->resp.head_sz = sprintf(creq->resp.header, "DELETED\r\n");
			else creq->resp.head_sz = sprintf(creq->resp.header, "NOT_FOUND\r\n");
			creq->resp.footer = "";
			creq->resp.foot_sz = 0;
			break;
	}
	return 0;
}


/* Actually serialize the headers/footers/and the data */
int ccache_resp_send(creq_t *creq){
	//TESTING CODE - just going to print out the values for now - should eventually send data through a socket
	printf("%s", creq->resp.header);
	printf("%s", creq->resp.footer);

	//TODO: After response is sent call ccache_req_free and destroy the data structure
	return 0;
}


int ccache_req_free(creq_t *creq){
	free(creq);
	return 0;
}

void cvect_struct_free(cvect_t *dyn_vect){
	cvect_free(dyn_vect);
}


