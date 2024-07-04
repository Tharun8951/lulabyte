#ifndef EVENTLOOP_H
#define EVENTLOOP_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <errno.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <stdbool.h>
#include <poll.h>


#define max_message_length 4096
#define STATE_REQ 0
#define STATE_RES 1
#define STATE_END 2

extern long long max_clients;
extern long long connected_clients;

typedef struct pollfd pollfd;

typedef struct Conn {
    int fd;
    uint32_t state;
    size_t rbuf_size;
    uint8_t rbuf[4 + max_message_length];
    size_t wbuf_size;
    size_t wbuf_sent;
    uint8_t wbuf[4 + max_message_length];
} Conn;


extern Conn **fd2conn;
extern pollfd *poll_args;

void connection_io(Conn *conn);
void set_fd_nb(int);
void die(const char *msg);
bool try_one_req(Conn *conn);
Conn *conn_get(Conn **fd2conn, int fd);
void accept_new_conn(Conn ***fd2conn, int server_fd, long long *connected_clients, long long *max_clients);
void add_conn_fd2conn(Conn ***fd2conn, long long *connected_clients,long long *max_clients, int connfd, Conn *conn);
void state_res(Conn *conn);
void state_req(Conn *conn);
bool try_fill_buffer(Conn *conn);
bool try_flush_buffer(Conn *conn);
void conn_remove(Conn ***fd2conn, int fd, long long *connected_clients);

#endif