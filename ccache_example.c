/* A simple server in the internet domain using TCP
   The port number is passed as an argument 
   Socket code from: https://banu.com/blog/2/how-to-use-epoll-a-complete-example-in-c/*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/epoll.h>
#include <errno.h>
#include <assert.h>
#include <string.h>

#include <ccache.h>
#include <creq.h>

#define MAXEVENTS 64
#define BUFFER_SIZE 512

/* Check to see if the request is actually complete. Also handles special cases
depending on the command issued.*/
static ccmd_t
is_complete_request(char *cmd){
    /* first find out how many tokens there are in the command */
    ccmd_t cmd_type;
    char * pch;
    pch = strtok(cmd, " ");
    int counter = 0;
    while(pch != NULL){
        /* first find out what command the first token is */
        if(counter == 0){
            if(strcmp(pch, "get") == 0) cmd_type = CGET;
            else if(strcmp(pch, "set") == 0) cmd_type = CSET;
            else if(strcmp(pch, "delete") == 0) cmd_type = CDELETE;
            else{
                cmd_type = INVALID;
            }
        }
        pch = strtok(NULL, " ");
        counter++;
    }


    /* check counter for the type - client errors */
    if((cmd_type == CGET && counter <= 1) || (cmd_type == CSET && counter != 5 )
        || (cmd_type == CDELETE && counter != 2)){
        cmd_type = INVALID;
    }
    return cmd_type;
}


static int
create_and_bind (char *port)
{
  struct addrinfo hints;
  struct addrinfo *result, *rp;
  int s, sfd;

  memset (&hints, 0, sizeof (struct addrinfo));
  hints.ai_family = AF_UNSPEC;     /* Return IPv4 and IPv6 choices */
  hints.ai_socktype = SOCK_STREAM; /* We want a TCP socket */
  hints.ai_flags = AI_PASSIVE;     /* All interfaces */

  s = getaddrinfo (NULL, port, &hints, &result);
  if (s != 0)
    {
      fprintf (stderr, "getaddrinfo: %s\n", gai_strerror (s));
      return -1;
    }

  for (rp = result; rp != NULL; rp = rp->ai_next)
    {
      sfd = socket (rp->ai_family, rp->ai_socktype, rp->ai_protocol);
      if (sfd == -1)
        continue;

      s = bind (sfd, rp->ai_addr, rp->ai_addrlen);
      if (s == 0)
        {
          /* We managed to bind successfully! */
          break;
        }

      close (sfd);
    }

  if (rp == NULL)
    {
      fprintf (stderr, "Could not bind\n");
      return -1;
    }

  freeaddrinfo (result);

  return sfd;
}

static int
make_socket_non_blocking (int sfd)
{
  int flags, s;

  flags = fcntl (sfd, F_GETFL, 0);
  if (flags == -1)
    {
      perror ("fcntl");
      return -1;
    }

  flags |= O_NONBLOCK;
  s = fcntl (sfd, F_SETFL, flags);
  if (s == -1)
    {
      perror ("fcntl");
      return -1;
    }

  return 0;
}



int
main (int argc, char *argv[])
{

    /* initialization code */
    cvect_t *dyn_vect = ccache_init();
    assert(dyn_vect);

    int sfd, s;
    int efd;
    struct epoll_event event;
    struct epoll_event *events;

    if (argc != 2)
    {
      fprintf (stderr, "Usage: %s [port]\n", argv[0]);
      exit (EXIT_FAILURE);
    }

    sfd = create_and_bind (argv[1]);
    if (sfd == -1) abort ();

    s = make_socket_non_blocking (sfd);
    if (s == -1) abort ();

    s = listen (sfd, SOMAXCONN);
    if (s == -1)
    {
        perror ("listen");
        abort ();
    }

    efd = epoll_create1 (0);
    if (efd == -1)
    {
      perror ("epoll_create");
      abort ();
    }

    event.data.fd = sfd;
    event.events = EPOLLIN | EPOLLET;
    s = epoll_ctl (efd, EPOLL_CTL_ADD, sfd, &event);
    if (s == -1)
    {
      perror ("epoll_ctl");
      abort ();
    }

    /* Buffer where events are returned */
    events = calloc (MAXEVENTS, sizeof event);

    /* The event loop */
    while (1)
    {
        int n, i;

        n = epoll_wait (efd, events, MAXEVENTS, -1);
        for (i = 0; i < n; i++)
        {   
        if ((events[i].events & EPOLLERR) || (events[i].events & EPOLLHUP) || (!(events[i].events & EPOLLIN)))
        {
              /* An error has occured on this fd, or the socket is not
                 ready for reading (why were we notified then?) */
            fprintf (stderr, "epoll error\n");
            close (events[i].data.fd);
            continue;
        }

        else if (sfd == events[i].data.fd)
        {
              /* We have a notification on the listening socket, which
                 means one or more incoming connections. */
                while (1)
                {
                    struct sockaddr in_addr;
                    socklen_t in_len;
                    int infd;
                    char hbuf[NI_MAXHOST], sbuf[NI_MAXSERV];

                    in_len = sizeof in_addr;
                    infd = accept (sfd, &in_addr, &in_len);
                    if (infd == -1)
                    {
                      if ((errno == EAGAIN) ||
                          (errno == EWOULDBLOCK))
                        {
                          /* We have processed all incoming
                             connections. */
                          break;
                        }
                      else
                        {
                          perror ("accept");
                          break;
                        }
                    }

                    s = getnameinfo (&in_addr, in_len,
                                   hbuf, sizeof hbuf,
                                   sbuf, sizeof sbuf,
                                   NI_NUMERICHOST | NI_NUMERICSERV);
                    if (s == 0)
                    {
                        printf("Accepted connection on descriptor %d "
                             "(host=%s, port=%s)\n", infd, hbuf, sbuf);
                    }

                  /* Make the incoming socket non-blocking and add it to the
                     list of fds to monitor. */
                    s = make_socket_non_blocking (infd);
                    if (s == -1) abort ();

                    event.data.fd = infd;
                    event.events = EPOLLIN | EPOLLET;
                    s = epoll_ctl (efd, EPOLL_CTL_ADD, infd, &event);
                    if (s == -1)
                    {
                        perror ("epoll_ctl");
                        abort ();
                    }
                }
                continue;
            }
            else
            {
              /* We have data on the fd waiting to be read. Read and
                 process it. We must read whatever data is available
                 completely, as we are running in edge-triggered mode
                 and won't get a notification again for the same
                 data. */
                int done = 0;
                char buf[BUFFER_SIZE];
                while (1)
                {
                    ssize_t count;
                    bzero(buf,BUFFER_SIZE); //zero out the buffer every time 


                    count = read (events[i].data.fd, buf, sizeof buf);
                    if (count == -1)
                    {
                      /* If errno == EAGAIN, that means we have read all
                         data. So go back to the main loop. */
                      if (errno != EAGAIN)
                        {
                          perror ("read");
                          done = 1;
                        }
                      break;
                    }
                    else if (count == 0)
                    {
                      /* End of file. The remote has closed the
                         connection. */
                      done = 1;
                      break;
                    }

                    


                    /* Process the buffered request and write the results to the output */
                    creq_t *creq = (creq_t *) sizeof(creq_t);

                    /* Make sure this request is complete - can do special handling based on creq_type then */ 
                    // if((creq->type = is_complete_request(buf))) {
                    //     creq = ccache_req_parse(buf);
                    // }
                    /* remove trailing newline */
                    buf[strlen(buf)-1] = '\0';
                    creq = ccache_req_parse(buf);

                    /* Write Header and Footer to socket */
                    printf("before header: %s, strlen: %i\n", creq->resp.header, strlen(creq->resp.header));
                    s = write (events[i].data.fd, creq->resp.header, strlen(creq->resp.header));
                    if (s == -1)
                    {
                        perror ("write");
                        abort ();
                    }

                    if(creq->resp.errcode == RERROR || creq->resp.errcode == 0){
                        //printf("%s", creq->resp.footer); //only print footer if no errors - gets rid of some of the gibberish
                        s = write (events[i].data.fd, creq->resp.footer, strlen(creq->resp.footer));
                        if (s == -1)
                        {
                            perror ("write");
                            abort ();
                        } 
                    }

                    /* write the END line to the socket if request was a GET */
                    if(creq->type == CGET){
                        s = write (events[i].data.fd, "END\r\n", strlen("END\r\n"));
                        if (s == -1)
                        {
                            perror ("write");
                            abort ();
                        }
                    }

                    // s = write (events[i].data.fd, buf, count);
                    // if (s == -1)
                    // {
                    //     perror ("write");
                    //     abort ();
                    // }
                }

                if (done)
                {
                    printf ("Closed connection on descriptor %d\n",
                          events[i].data.fd);

                    /* Closing the descriptor will make epoll remove it
                     from the set of descriptors which are monitored. */
                    close (events[i].data.fd);
                }
            }
        }
    }

  free (events);

  close (sfd);

  return EXIT_SUCCESS;
}