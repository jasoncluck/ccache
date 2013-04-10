#pragma once

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h> 
#include <sys/socket.h>
#include <netinet/in.h>
#include <semaphore.h>

#define BUFFER_SIZE 1025

int newsockfd;
sem_t input_buffer_sem;
char buffer[BUFFER_SIZE];
CB_t *output_cb;


/* Write to the open socket. Returns < 0 if an error occured*/
int write_to_socket(char *str, int sz);

char *read_from_socket();
