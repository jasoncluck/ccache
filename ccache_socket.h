#pragma once

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h> 
#include <sys/socket.h>
#include <sys/epoll.h>
#include <netinet/in.h>
#include <semaphore.h>


#define BUFFER_SIZE 1025

char buffer[BUFFER_SIZE];
CB_t *output_cb;


