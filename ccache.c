#include "ccache.h"



/* globals */
/* cvect */
struct pair pairs[1<<10];
cvect_t *dyn_vect;
int pairs_counter;
CB_t *cb;

/* Global linked list for least recently used list */
struct creq_linked_list *lru_ll;


/* See if an id is already in a pair */
int 
in_pairs(struct pair *ps, int len, long id)
{
	for (; len >= 0 ; len--) {
		if (ps[len].id == id) return 1;
	}
	return 0;
}

/* Separate this out so that we can easily confirm that the compiler
 * is doing the proper optimizations. */
void *
do_lookups(struct pair *ps, cvect_t *v)
{
	return cvect_lookup(v, ps->id);	
}

/* Initialization code */
cvect_t *
ccache_init(void){
	
	/* Initialize cvect stuff and global lru list*/
	dyn_vect = cvect_alloc();
	assert(dyn_vect);
	cvect_init(dyn_vect);
	lru_ll = malloc(sizeof(struct creq_linked_list));

	return dyn_vect;
}


int 
command_buffer_init(){
	/* socket buffer */
	cb = malloc(sizeof(CB_t));
	assert(cb);
	buffer_init(cb, BUFFER_LENGTH, sizeof(char *));
	

	return 0;
}


/* Hashing function: Takes in an value and produces a key for that value to be mapped to. Key < 2^20 due to cvect contraints.
Hashing function is djb: http://www.cse.yorku.ca/~oz/hash.html*/
long 
hash(char *str){
	unsigned long hash = 5381;
    int c;

    while ((c = *str++)){
        hash = ((hash << 5) + hash) + c; /* hash * 33 + c */
    }
    //modulo for cvect
    hash %= CVECT_MAX_ID;
    return hash;
}


/* The Following three functons populate the data. Initial parsing has already occurred */
creq_t * 
ccache_get(creq_t *creq){

	//create a new pair for the cvect call and put the key in it.
	struct pair getpair;
	long hashedkey = hash(creq->key);
	getpair.id = hashedkey;
	void * lookup_result;


	if((lookup_result = do_lookups(&getpair, dyn_vect)) != 0){
		struct creq_linked_list *cvect_list = (struct creq_linked_list *) lookup_result; 
		creq->resp.errcode = NOERROR;
		while(1){
			if(!(strncmp(cvect_list->head->key, creq->key, KEY_SIZE))) {
				/* set this creq to be returned */
				creq = cvect_list->head; 
				creq->type = CGET;
				/* update pointers for cvects ll and lru ll*/
				creq->next = cvect_list->head; //set the current head to be the second node in the list
				cvect_list->head = creq; //now change the head to be the new node (most recent)


				/* rearrange the pointers and put the new data at the front of the global list */
				creq_t *temp_creq = creq;
				creq->next = creq->next->next;
				creq = creq->next;
				temp_creq->next = lru_ll->head;

				//TODO: DO this for the local lists as well				

				break;
			}
			else if(cvect_list->head->next == NULL) {
				creq->resp.errcode = RERROR;
				break;
			} 
			else cvect_list->head = cvect_list->head->next;
		}
	}
	else{

		creq->resp.errcode = RERROR;
	}

	creq = ccache_resp_synth(creq);
	//ccache_resp_send(creq);

	return creq;
}
creq_t * 
ccache_set(creq_t *creq){

	long hashedkey = hash(creq->key);

	/* create the new pair and the new node that is the pairs value */
	pairs[pairs_counter].id = hashedkey; //set the id to be the key returned from the hash function
	pairs[pairs_counter].val = malloc(sizeof(struct creq_linked_list)); //malloc space for the pointer to the node object
	//init linked lists, one for inserting the node, one for the cvect to map to
	struct creq_linked_list *insert_dll = malloc(sizeof(struct creq_linked_list));
	
	

	add_creq(insert_dll, creq);
	memcpy(&pairs[pairs_counter].val, &insert_dll, sizeof(struct creq_linked_list)); //store the linked list in the cvect

	/* check to see if this key already exists, if it doesn't: add it. If it does exist, resolve linked node collision */
	void *lookup_result;
	if((lookup_result = do_lookups(&pairs[pairs_counter], dyn_vect)) != 0) //string exists
	{ 
		struct creq_linked_list *cvect_dll = (struct creq_linked_list *) lookup_result;
		while(1){
			if(!(strcmp(insert_dll->head->key, cvect_dll->head->key))) {
				//memcpy(head, insert_node, sizeof(node_t)); //just copy over the memory overwriting all the old fields
				cvect_dll->head = insert_dll->head;
				break;
			}
			if(cvect_dll->head->next == NULL){
				add_creq(cvect_dll, insert_dll->head); //add the insert node to the end of the cvect's list
				break;	
			} 
			else cvect_dll->head = cvect_dll->head->next;
		}
		//at this point head.next == null so set head.next to the insert node
		
	}
	else
	{
		/* add the pair to the cvect and update the LRU list */
		creq->resp.errcode = cvect_add_id(dyn_vect, pairs[pairs_counter].val, pairs[pairs_counter].id);
		add_creq(lru_ll, creq);

	}	

	assert(creq->resp.errcode == NOERROR); //if no errors: creq->resp.errcode == 0;
	pairs_counter++;
		
	creq = ccache_resp_synth(creq);
	//ccache_resp_send(creq);
	
	return creq;

}


creq_t * 
ccache_delete(creq_t *creq){

	struct pair delete_pair;
	long hashedkey = hash(creq->key);
	delete_pair.id = hashedkey;	
	
	void * lookup_result;
	if((lookup_result = do_lookups(&delete_pair, dyn_vect)) != 0){
		struct creq_linked_list *cvect_list = (struct creq_linked_list *) lookup_result; //if non-zero we can typecast this without seg faulting	
		creq->resp.errcode = NOERROR;
		
		//check the first node
		if(!(strcmp(cvect_list->head->key, creq->key))){
			//if there is nothing after the first node, just delete the whole cvect bucket
			if(cvect_list->head->next == NULL){
				creq->resp.errcode = cvect_del(dyn_vect, delete_pair.id);	
				if(creq->resp.errcode == NOERROR){
					//counter is always inside of cvect lock so this probably isn't needed...
					pairs_counter--;	
				} 
			} 
			else{
				//if there is another node after the first one - have to change the cvect pointer. also get rid of cvect_list->head's data
				memcpy(cvect_list->head, cvect_list->head->next, sizeof(creq_t));
				free(cvect_list->head->next);
			}			
		}
		else{
			creq->resp.errcode = delete_creq(cvect_list, creq);
		}
	}		
	else{
		//Data not found in cvect, return error
		creq->resp.errcode = RERROR;
	}	
	
	//search for creq->key in the hash table, if it exists delete it
	creq = ccache_resp_synth(creq);
	//ccache_resp_send(creq);
	//ccache_req_free(creq); TODO: FIX THIS CLEANUP

	return creq;
}

//Commands look like: <command name> <key> <flags> <exptime> <bytes> [noreply]\r\n
/* Function to parse the string and put it into the structure above */
creq_t *
ccache_req_parse(char *cmd){
	//Command is passed as a string along with its length
	//Can start by tokenizing this data, and determining what type of command it is.

	creq_t *creq = (creq_t *) malloc(sizeof(creq_t));
	
	char * pch;
	pch = strtok(cmd, " ");
	int counter = 0;
	creq->resp.errcode = NOERROR;
	
	while(pch != NULL){
		/* first find out what command the first token is */
		if(counter == 0){
			if(strcmp(pch, "get") == 0) creq->type = CGET;
			else if(strcmp(pch, "set") == 0) creq->type = CSET;
			else if(strcmp(pch, "delete") == 0) creq->type = CDELETE;
			else{
				creq->resp.errcode = 1;
				break;
			}
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
		 			else{
		 				break;
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


	/* check counter for the type - client errors */
	if((creq->type == CGET && counter <= 1) || (creq->type == CSET && counter != 5 )
		|| (creq->type == CDELETE && counter != 2)){
		creq->resp.errcode = CERROR;
	}

	/* deal with any parsing errors */
	if(creq->resp.errcode == ERROR || creq->resp.errcode == CERROR){
		creq = ccache_resp_synth(creq);

		//ccache_resp_send(creq);
	}
	else{
		/* Now that the tokens are done - continue the processing by calling the respective functions */
		if(creq->type == CGET){

			creq = ccache_get(creq);
		}
		else if(creq->type == CSET) {
			creq = ccache_set(creq);
		}
		else if(creq->type == CDELETE){ 
			creq = ccache_delete(creq);
		}
	}
	return creq;
}

creq_t *
ccache_resp_synth(creq_t *creq){
	if(creq->resp.errcode > 0 && creq->resp.errcode < 3){
		creq->resp.header = (char * ) malloc(16); //send errcode as a header - looks the same to the client
		
		switch(creq->resp.errcode){
			case ERROR: 
				sprintf(creq->resp.header, "ERROR That command was not recognized - must be get, set, or delete\r\n");
				break;
			case CERROR:
				sprintf(creq->resp.header, "CLIENT ERROR Incorrect arguments detected\r\n");
				break;
			case SERROR:
				sprintf(creq->resp.header, "SERVER ERROR Server is unresponsive, try again in a bit\r\n");
				break;
			default:
				break;
		}
	}
	else{
		switch(creq->type){
			case CSET:
				// Configure the header based on whether the data was saved successfully or not 
				creq->resp.header = (char * ) malloc(16);
				
				if(creq->resp.errcode == NOERROR) creq->resp.head_sz = sprintf(creq->resp.header, "STORED\r\n");
				else if(creq->resp.errcode == RERROR) creq->resp.head_sz = sprintf(creq->resp.header, "NOT_STORED\r\n");
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
					creq->resp.head_sz = snprintf(creq->resp.header, 1<<8, "VALUE %s %d %d \r\n", creq->key, creq->flags, creq->bytes);
					creq->resp.foot_sz = snprintf(creq->resp.footer, creq->bytes, "%s\r\n", creq->data);
				}

				break;
			case CDELETE:
				creq->resp.header = (char * ) malloc(16);
				
				if(!creq->resp.errcode) creq->resp.head_sz = sprintf(creq->resp.header, "DELETED\r\n");
				else if(creq->resp.errcode == RERROR) creq->resp.head_sz = sprintf(creq->resp.header, "NOT_FOUND\r\n");
				creq->resp.footer = "";
				creq->resp.foot_sz = 0;
				break;
			default:
				break;
		}
	}
	return creq;
}


/* Actually serialize the headers/footers/and the data */
int 
ccache_resp_send(creq_t *creq){	
	//TESTING CODE - just going to print out the values for now - should eventually send data through a socket
	printf("%s", creq->resp.header);
	if(creq->resp.errcode == RERROR || creq->resp.errcode == 0){
		printf("%s", creq->resp.footer); //only print footer if no errors - gets rid of some of the gibberish
		
	}
	return 0;
}


int 
ccache_req_free(creq_t *creq){
	free(creq);
	return 0;
}

void 
cvect_struct_free(cvect_t *dyn_vect){
	cvect_free(dyn_vect);
}

