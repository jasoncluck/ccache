/* A simple server in the internet domain using TCP
   The port number is passed as an argument */


#include <ccache.h>
#include <ccache_socket.h>

void error(const char *msg)
{
    perror(msg);
    exit(1);
}

int write_to_socket(char *str, int sz){
    int n;
    n = write(newsockfd, str, sz);
    return n;
}

char *read_from_socket(){
    int n;
    bzero(buffer,BUFFER_SIZE);
    n = read(newsockfd,buffer,BUFFER_SIZE-1);
    if (n < 0) error("ERROR reading from socket");
    return buffer;
}

int main(int argc, char *argv[])
{
    /* initialization code */
    cvect_t *dyn_vect = ccache_init();
    assert(dyn_vect);
    if(thread_pool_init()) goto mem_error;
    if(command_buffer_init()) goto mem_error;

    /* output buffer init */
    output_cb = malloc(sizeof(CB_t));
    assert(output_cb);
    buffer_init(output_cb, BUFFER_LENGTH, sizeof(char *));

    /* semaphore for accepting socket input */
    sem_init(&input_buffer_sem, 0, -1); //start at 1 for the first request

    if(argc < 2){
        fprintf(stderr, "ERROR, no port provided\n");
        exit(1);
    }


    /* socket code */
    int sockfd, portno;
    socklen_t clilen;
    
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
        /* check to see if anything needs to be outputted over the socket */
        char *output;
        output = pop(output_cb);

        
        /* read data and remove the newline */
        read_from_socket();
        buffer[strlen(buffer)-1] = '\0';

        /* process the command */
        add_req_to_buffer(newsockfd, buffer);
        //sem_wait(&input_buffer_sem);
        sleep(1);
        while(1){
            output = pop(output_cb);
            if(strcmp(output, "empty") == 0) break;
            write_to_socket(output, strlen(output));
            
        }
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