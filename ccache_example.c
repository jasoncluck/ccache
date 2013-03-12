#include <stdio.h>
#include <stdlib.h>
#include <malloc.h>
#include <string.h>
#include <assert.h>

#include "ccache.h"



int main(){
	cvect_t *dyn_vect = ccache_init();

	assert(dyn_vect);
	//create a test command and pass it to parse request
	char str[] = "set foobar 100 02 3";
	ccache_req_parse(str, strlen(str));

	char str2[] = "set alice 200 01 50";
	ccache_req_parse(str2, strlen(str2));

	char str3[] = "get foobar";
	ccache_req_parse(str3, strlen(str3));

	char str7[] = "set buff 400 03 20";
	ccache_req_parse(str7, strlen(str7));

	char str4[] = "set baz 300 02 40";
	ccache_req_parse(str4, strlen(str4));

	//char str4[] = "delete alice";
	//ccache_req_parse(str4, strlen(str4));

	char str5[] = "get alice";
	ccache_req_parse(str5, strlen(str5));

	char str6[] = "get buff alice foobar baz";
	ccache_req_parse(str6, strlen(str6));

	// char str3[] = "delete foobar";
	// ccache_req_parse(str3, strlen(str3));
	
	cvect_struct_free(dyn_vect);
	return 0;
}