#ifndef COMMUNICATE_H
#define COMMUNICATE_H

#include <unistd.h>
#include <stdio.h>
#include <stdint.h>
#define max_message_length 4096

int handle_query(int client_sock);

#endif