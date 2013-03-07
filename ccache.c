#include "ccache.h"


/* The Following three funcitons populate the data. Initial parsing as already occurred */
int ccache_get(creq_t *creq){
	/* Each item sent by the server will look like: VALUE <key> <flags> <bytes> [<cas unique>]\r\n
<data block>\r\n */

	//TODO: Get the item from the hash table with the key - then overwrite creq with all the data
	//creq = hashtable.get(creq->key);
	
	
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
	//strcpy(creq->data, temp_data);
	/* Copy over the data input using memcpy since we know how many bytes it is */
	memcpy(&creq->data, &temp_data, creq->bytes);

	printf("Data is: %s\n", creq->data);
	
	//TODO: Actually store the data here in some type of data structure
	creq->resp.errcode = 0;

	ccache_resp_synth(creq);
	ccache_resp_send(creq);
	return 0;

}

int ccache_delete(creq_t *creq){
	//search for creq->key in the hash table, if it exists delete it
	ccache_resp_synth(creq);
	ccache_resp_send(creq);
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

	/* Now that the tokens are done - continue the processing by calling the respective functions */
	if(creq->type == CGET){ 
		ccache_get(creq);
		printf("END\r\n");
	}
	else if(creq->type == CSET) {
		ccache_set(creq);
	}
	else if(creq->type == CDELETE){ 
		ccache_delete(creq);
	}
	

	/* Debug stuff */
	printf("type: %i\n", creq->type);
	printf("key: %s\n", creq->key);
	printf("flags: %i\n", creq->flags);
	printf("exp_time: %i\n", creq->exp_time);
	printf("bytes: %i\n", creq->bytes);

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
			creq->resp.head_sz = sprintf(creq->resp.header, "VALUE %s %d %d \r\n", creq->key, creq->flags, creq->bytes);
			creq->resp.foot_sz = sprintf(creq->resp.footer, "%s", creq->data);
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

