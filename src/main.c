#include <errno.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

#include "eventLoop.h"
// #include "communicate.h"

Conn **fd2conn;
pollfd *poll_args;

long long max_clients = 1024;
long long connected_clients = 0;

long long sf = 0;

#define BUFFER_SIZE 1024

int main() {
    setbuf(stdout, NULL);

    printf("Logs from your program will appear here!\n");

    // int server_fd, client_addr_len;
    int server_fd;
    // struct sockaddr_in client_addr;

    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd == -1) {
        printf("Socket creation failed: %s...\n", strerror(errno));
        return 1;
    }

    int reuse = 1;
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse)) <
        0) {
        printf("SO_REUSEADDR failed: %s \n", strerror(errno));
        return 1;
    }

    struct sockaddr_in serv_addr = {
        .sin_family = AF_INET,
        .sin_port = htons(6379),
        .sin_addr = {htonl(INADDR_ANY)},
    };

    if (bind(server_fd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) !=
        0) {
        printf("Bind failed: %s \n", strerror(errno));
        return 1;
    }

    int connection_backlog = 5;
    if (listen(server_fd, connection_backlog) != 0) {
        printf("Listen failed: %s \n", strerror(errno));
        return 1;
    }

    // setting server_fs to non blocking mode
    set_fd_nb(server_fd);

    Conn **fd2conn = (Conn **)calloc(max_clients, sizeof(Conn *));
    if (!fd2conn) die("calloc");

    struct pollfd *poll_args =
        (struct pollfd *)malloc(max_clients * sizeof(struct pollfd));
    if (!poll_args) die("malloc");

    while (1) {
        printf("inside event_loop while loop\n");
        poll_args[0].fd = server_fd;
        poll_args[0].events = POLLIN;
        int nfds = 1;

        for (int i = 0; i < max_clients; i++) {
            if (fd2conn[i] != NULL) {
                poll_args[nfds].fd = fd2conn[i]->fd;
                poll_args[nfds].events =
                    (fd2conn[i]->state == STATE_REQ) ? POLLIN : POLLOUT;
                poll_args[nfds].events |= POLLERR;
                nfds++;
            } 
        }

        int poll_sockets = poll(poll_args, nfds, 1000);
        if (poll_sockets < 0) die("poll");

        if (poll_args[0].revents) {
            accept_new_conn(&fd2conn, server_fd, &connected_clients,
                            &max_clients);
        }

        for (int i = 1; i < nfds; i++) {
            if (poll_args[i].revents) {
                Conn *conn = conn_get(fd2conn, poll_args[i].fd);
                connection_io(conn);
                if (conn->state == STATE_END) {
                    conn_remove(&fd2conn, conn->fd, &connected_clients);
                    close(conn->fd);
                }
            }
        }
    }

    free(poll_args);
    free(fd2conn);
    close(server_fd);  // Closing the server socket outside the loop
    return 0;
}