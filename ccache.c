#include "ccache.h"



/* globals */
/* cvect */
struct pair pairs[1<<10];
cvect_t *dyn_vect;
int pairs_counter;

CB_t *cb;

/* threads */
pthread_t workerThreads[MAX_CONCURRENCY];
//mutex for accessing the global cvect data structure
pthread_mutex_t cvect_mutex;
pthread_mutex_t cvect_pairs_mutex;
pthread_mutex_t cvect_counter_mutex;
pthread_mutex_t buffer_mutex;
sem_t buffer_sem;



/* function for the main thread to pass inputs over to threads */
void *thread_start(void * thread_num){
	//while there is something to do... ie: semaphore isn't 0
	//lock the request structure
	//pop off the most recent request
	//unlock the request structure
	//start parsing command
	while(1){
		//printf("thread %i here\n", (int) thread_num);
		sem_wait(&buffer_sem); //will block until semaphore is non-negative
		pthread_mutex_lock(&buffer_mutex);
		char * cmd = pop(cb);
		pthread_mutex_unlock(&buffer_mutex);

		ccache_req_parse(cmd);
	}
	//should never reach here - threads don't join
}

/* 	Function for adding a request to the circular buffer.
	Only called by the main thread */
int add_req_to_buffer(char * str){
	//make sure the buffer isn't full
	pthread_mutex_lock(&buffer_mutex);
	push(str, cb);
	sem_post(&buffer_sem); //increment the semaphore because a new command is in the buffer
	pthread_mutex_unlock(&buffer_mutex);
	
	return 0;
}

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
	
	/* Initialize cvect stuff */
	dyn_vect = cvect_alloc();
	assert(dyn_vect);
	cvect_init(dyn_vect);

	/* socket buffer */
	cb = malloc(sizeof(*cb));
	buffer_init(cb, BUFFER_LENGTH, sizeof(int));

	/* initialize thread pool */
	int i, rc;
	for(i = 0; i < MAX_CONCURRENCY; i++){
		//init thread
		//printf("Creating thread: %i\n", i);
		rc = pthread_create(&workerThreads[i], NULL, thread_start, (void *) i);
		//error check
		if(rc){
			fprintf(stderr, "Error, returned value of response is %d\n", rc);
			exit(-1);
		}
	}

	/* initialize mutexes and semaphores */
	pthread_mutex_init(&cvect_mutex, NULL);
	pthread_mutex_init(&cvect_counter_mutex, NULL);
	pthread_mutex_init(&cvect_pairs_mutex, NULL);
	pthread_mutex_init(&buffer_mutex, NULL);
	sem_init(&buffer_sem, 0, -1); //turn off forks - can't do this in Linux anyways.  Initial value of -1, non-negative values stop threads from blocking

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
	long hashedkey = hash(creq->key);
	getpair.id = hashedkey;
	//TODO:shouldn't need to lock here, just reading but double check this
	void * lookup_result;
	pthread_mutex_lock(&cvect_mutex);
	if((lookup_result = do_lookups(&getpair, dyn_vect)) != 0){
		node_t *head = (node_t *) lookup_result; //if non-zero we can typecast this without seg faulting	
		creq->resp.errcode = 0;
		while(1){
			//if the keys are equal, we have the element so stop iteration
			if(!(strcmp(head->creq->key, creq->key))){
				creq = head->creq; 
				creq->type = CGET; //set command back to GET
				break;
			}
			else if(head->next == NULL){
				creq->resp.errcode = -1;
				break;
			} 
			else head = head->next;
		}
	}
	else{
		//Data was not found so set the error code flag and nothing will be sent back
		creq->resp.errcode = -1;
	}
	pthread_mutex_unlock(&cvect_mutex);

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

	/* create the new pair and the new node that is the pairs value */
	pthread_mutex_lock(&cvect_pairs_mutex);
	pairs[pairs_counter].id = hashedkey; //set the id to be the key returned from the hash function
	pairs[pairs_counter].val = malloc(sizeof(node_t)); //malloc space for the pointer to the node object
	node_t *insert_node = (node_t *) malloc(sizeof(node_t));
	node_t *head = (node_t *) malloc(sizeof(node_t));
	insert_node->creq = creq;
	insert_node->next = NULL;
	memcpy(&pairs[pairs_counter].val, &insert_node, sizeof(node_t)); //store the entire creq in the cvect

	/* check to see if this key already exists, if it doesn't: add it. If it does exist, resolve linked node collision */
	void *lookup_result;
	pthread_mutex_lock(&cvect_mutex);
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
	pthread_mutex_lock(&cvect_counter_mutex);
	pairs_counter++;

	#if DEBUG
		printf("Checking at the end of SET to see if data was stored correctly:\n");
		lookup_result = do_lookups(&pairs[pairs_counter], dyn_vect);
		node_t *check_node = (node_t *) lookup_result;
		printf("check set key: %s\n", check_node->creq->key);
		printf("check set data: %s\n", check_node->creq->data);
	#endif /* DEBUG */
	
	//release all the locks: cvect structure, the pairs structure, the pairs counter
	pthread_mutex_unlock(&cvect_counter_mutex);
	pthread_mutex_unlock(&cvect_pairs_mutex);
	pthread_mutex_unlock(&cvect_mutex);
	
	ccache_resp_synth(creq);
	ccache_resp_send(creq);
	
	return 0;

}

int ccache_delete(creq_t *creq){

	struct pair deletepair;
	long hashedkey = hash(creq->key);
	deletepair.id = hashedkey;	
	
	void * lookup_result;
	pthread_mutex_lock(&cvect_mutex);
	if((lookup_result = do_lookups(&deletepair, dyn_vect)) != 0){
		node_t *head = (node_t *) lookup_result; //if non-zero we can typecast this without seg faulting	
		creq->resp.errcode = 0;
		
		//check the first node
		if(!(strcmp(head->creq->key, creq->key))){
			//if there is nothing after the first node, just delete the whole cvect bucket
			if(head->next == NULL){
				creq->resp.errcode = cvect_del(dyn_vect, deletepair.id);	
				if(!creq->resp.errcode){
					//counter is always inside of cvect lock so this probably isn't needed...
					pthread_mutex_lock(&cvect_counter_mutex);
					pairs_counter--;	
					pthread_mutex_unlock(&cvect_counter_mutex);
				} 
			} 
			else{
				//if there is another node after the first one - have to change the cvect pointer. also get rid of head's data
				memcpy(head, head->next, sizeof(node_t));
				free(head->next);
			}			
		}
		else{
			while(1){
				if(head->next != NULL && !strcmp(head->next->creq->key, creq->key)){
					free(head->next); //garbage collect the node's references
					if(head->next->next != NULL) head->next = head->next->next; //change the pointers
					break;
				}
				if(head->next != NULL) head = head->next;
				else{
					creq->resp.errcode = -2;
					break;
				}

			}

		}
	}		
	else{
		//Data not found in cvect, return error
		creq->resp.errcode = -1;
	}	
	pthread_mutex_unlock(&cvect_mutex);
	
	//search for creq->key in the hash table, if it exists delete it
	ccache_resp_synth(creq);
	ccache_resp_send(creq);
	ccache_req_free(creq);

	return 0;
}

//Commands look like: <command name> <key> <flags> <exptime> <bytes> [noreply]\r\n
/* Function to parse the string and put it into the structure above */
creq_t *ccache_req_parse(char *cmd){
	//Command is passed as a string along with its length
	//Can start by tokenizing this data, and determining what type of command it is.

	creq_t *creq = (creq_t *) malloc(sizeof(creq_t));
	//printf("%s\n", cmd);
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
	return 0;
}


int ccache_req_free(creq_t *creq){
	free(creq);
	return 0;
}

void cvect_struct_free(cvect_t *dyn_vect){
	cvect_free(dyn_vect);
}


