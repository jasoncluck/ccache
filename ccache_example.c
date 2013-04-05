/* A simple server in the internet domain using TCP
   The port number is passed as an argument */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h> 
#include <sys/socket.h>
#include <netinet/in.h>

#include <ccache.h>

#define BUFFER_SIZE 1025

void error(const char *msg)
{
    perror(msg);
    exit(1);
}

int main(int argc, char *argv[])
{
    /* initialization code */
    cvect_t *dyn_vect = ccache_init();
    assert(dyn_vect);
    if(thread_pool_init()) goto mem_error;
    if(command_buffer_init()) goto mem_error;

    if(argc < 2){
        fprintf(stderr, "ERROR, no port provided\n");
        exit(1);
    }


    /* socket code */
    int sockfd, newsockfd, portno;
    socklen_t clilen;
    char buffer[BUFFER_SIZE];
    struct sockaddr_in serv_addr, cli_addr;
    int n;

    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) 
    error("ERROR opening socket");
    bzero((char *) &serv_addr, sizeof(serv_addr));
    portno = atoi(argv[1]);
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = INADDR_ANY;
    serv_addr.sin_port = htons(portno);
    if (bind(sockfd, (struct sockaddr *) &serv_addr,
          sizeof(serv_addr)) < 0) 
          error("ERROR on binding");
    listen(sockfd,5);
    clilen = sizeof(cli_addr);
    newsockfd = accept(sockfd, 
             (struct sockaddr *) &cli_addr, 
             &clilen);
    if (newsockfd < 0) error("ERROR on accept");
    while(1){
        bzero(buffer,BUFFER_SIZE);
        n = read(newsockfd,buffer,BUFFER_SIZE-1);
        if (n < 0) error("ERROR reading from socket");
        printf("Here is the message: %s\n",buffer);
        n = write(newsockfd,"I got your message",18);

        /* process the command */
        add_req_to_buffer(newsockfd, buffer);
        sleep(1);
    }

    if (n < 0) error("ERROR writing to socket");

    return 0;

    mem_error:
        printf("Couldn't allocate memory for data structure - quitting\n");
        return 0;
    // done:
    //     close(newsockfd);
    //     close(sockfd);
    //     cvect_struct_free(dyn_vect);
    //     return 0;
    


}