#include <string.h>
#include <stdio.h>

#include "ccache.h"

//Commands look like: <command name> <key> <flags> <exptime> <bytes> [noreply]\r\n



/* Function to parse the string and put it into the structure above */
struct creq *ccache_req_parse(char *cmd, int cmdlen){
	//Command is passed as a string along with its length
	//Can start by tokenizing this data, and determining what type of command it is.

	char * pch;
	printf("Tokenizing the command into tokens");
	pch = strtok(cmd, " ");
	while (pch != NULL)
	{
		printf ("%s\n", pch);
		pch = strtok(NULL, " ");
	}

}


int main(){
	//create a test command and pass it to parse request
	char str[] = "This is a test string that should tokenize";
	ccache_req_parsed(str, strlen(str))
}