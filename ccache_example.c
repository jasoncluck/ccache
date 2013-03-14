#include <stdio.h>
#include <stdlib.h>
#include <malloc.h>
#include <string.h>
#include <assert.h>
#include <unistd.h>

#include "ccache.h"



int main(){
	cvect_t *dyn_vect = ccache_init();
	assert(dyn_vect);
	if(thread_pool_init()) goto mem_error;
	if(command_buffer_init()) goto mem_error;
	
	char *cmd = malloc(MAX_CMD_SZ);
	while(1){
		/* Get a command */
		fgets(cmd, MAX_CMD_SZ, stdin);
		
		cmd[strlen(cmd)-1] = '\0'; //remove any trailing newline character

		//can quit with 'q'
		if(!(strcmp(cmd, "q"))) {
			printf("Quitting\n");
			goto done;
		}

		add_req_to_buffer(cmd);
		sleep(1); //allow the workers to go
	}
	done:
		free(cmd);
		cvect_struct_free(dyn_vect);
		return 0;
	mem_error:
		printf("Couldn't allocate memory for data structure - quitting\n");
		exit(1);
}