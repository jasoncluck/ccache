/* A simple server in the internet domain using TCP
   The port number is passed as an argument */

#include <ccache.h>
#include <ccache_socket.h>

#define EPOLL_QUEUE_LEN 128 //max number of connections to manage at one time

void error(const char *msg)
{
    perror(msg);
    exit(1);
}


int main(){

    /* initialization code */
    cvect_t *dyn_vect = ccache_init();
    assert(dyn_vect);


    int epfd;
    epfd = epoll_create(EPOLL_QUEUE_LEN);

    /* add descriptors to epoll */
    static struct epoll_event ev, *events;
    
     /* socket code */
    int client_sock, portno;
    socklen_t clilen;
    
    struct sockaddr_in serv_addr, cli_addr;

    client_sock = socket(AF_INET, SOCK_STREAM, 0); //create the socket
    if (client_sock < 0) error("ERROR opening socket");
    bzero((char *) &serv_addr, sizeof(serv_addr));  //zero out the information
    portno = 11211; //use memcaached port
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = INADDR_ANY;
    serv_addr.sin_port = htons(portno);
    /* try to bind and listen to the socket */
    if (bind(client_sock, (struct sockaddr *) &serv_addr,
          sizeof(serv_addr)) < 0) 
          error("ERROR on binding");
    listen(client_sock,5);
    clilen = sizeof(cli_addr);
    
    // newsockfd = accept(client_sock, (struct sockaddr *) &cli_addr, &clilen); //accept a connection on a socket.  newsockfd is now the reference to this newly accepted connection
    // if (newsockfd < 0) error("ERROR on accept");


    ev.events = EPOLLIN | EPOLLERR | EPOLLHUP;    
    ev.data.fd = client_sock;
    int res; 
    res = epoll_ctl(epfd, EPOLL_CTL_ADD, client_sock, &ev);
    //printf("res: %d\n", res);

    /* Descriptors are now added to epoll so process can idle and do something with epoll'ed sockets */
    while (1) {
        // wait for something to do...
        int nfds, n;
        nfds = epoll_wait(epfd, &ev, 10, -1);
        /* for all of the events on the socket, accept a new connection */
        for(n = 0; n < nfds; ++n) { 

        }
    }
    return 0;

};