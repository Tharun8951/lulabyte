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

    int epoll_fd = epoll_create1(0);
    if (epoll_fd == -1) die("epoll_create1");

    struct epoll_event event;
    event.data.fd = server_fd;
    event.events = EPOLLIN | EPOLLET;
    if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, server_fd, &event) == -1) {
        die("epoll_ctl");
    }

    struct epoll_event *events =
        (struct epoll_event *)calloc(MAX_EVENTS, sizeof(struct epoll_event));
    if (!events) die("calloc");

    while (1) {
        printf("inside epoll eventloop\n");
        int n = epoll_wait(epoll_fd, events, MAX_EVENTS, -1);
        if (n == -1) {
            if (errno == EINTR) continue;
            die("epoll_wait");
        }

        for (int i = 0; i < n; i++) {
            if ((events[i].events & EPOLLERR) ||
                (events[i].events & EPOLLHUP) ||
                (!(events[i].events & EPOLLIN))) {
                fprintf(stderr, "epoll error\n");
                close(events[i].data.fd);
                continue;
            } else if (server_fd == events[i].data.fd) {
                while (accept_new_conn(server_fd, event, epoll_fd, &fd2conn,
                                       &connected_clients, &max_clients)) {
                }
                continue;
            } else {
                Conn *conn = conn_get(fd2conn, events[i].data.fd);
                if (!conn) continue;
                connection_io(conn);
                if (conn->state == STATE_END) {
                    conn_remove(&fd2conn, conn->fd, &connected_clients);
                    printf("\n\n\t\t\tDISCONNECTED CLIENT\t\t\t\n\n");
                    close(conn->fd);
                }
            }
        }
    }
    free(events);
    free(fd2conn);
    close(server_fd);  // Closing the server socket outside the loop
    return 0;
}