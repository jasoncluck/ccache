#define COS_LINUX_ENV 1

#include "ccache.h"

int main(){
	//create a test command and pass it to parse request
	char str[] = "set foobar 230 02 01";
	ccache_req_parse(str, strlen(str));

	char str2[] = "get foobar";
	ccache_req_parse(str2, strlen(str2));

	char str3[] = "delete foobar";
	ccache_req_parse(str3, strlen(str3));
	

	return 0;
}
