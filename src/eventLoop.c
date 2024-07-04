#include "eventLoop.h"

void set_fd_nb(int fd) {
    int flags = fcntl(fd, F_GETFL, 0);
    if (flags < 0) die("fcntl F_GETFL");
    if (fcntl(fd, F_SETFL, flags | O_NONBLOCK) < 0)
        die("fcntl F_SETFL O_NONBLOCK");
}

void die(const char *msg) {
    perror(msg);
    exit(EXIT_FAILURE);
}

void state_req(Conn *conn) {
    while (try_fill_buffer(conn)) {
    };
}

void state_res(Conn *conn) {
    while (try_flush_buffer(conn)) {
    };
}

void add_conn_fd2conn(Conn ***fd2conn, long long *connected_clients,
                      long long *max_clients, int connfd, Conn *conn) {
    if (connfd >= *max_clients) {
        *max_clients *= 2;
        *fd2conn = realloc(*fd2conn, sizeof(Conn *) * (*max_clients));
        if (!*fd2conn) die("add_conn_fd2conn() realloc error\n");
    }
    (*fd2conn)[connfd] = conn;
    (*connected_clients)++;
}

void accept_new_conn(Conn ***fd2conn, int server_fd,
                     long long *connected_clients, long long *max_clients) {
    printf("accepting new client\n");
    struct sockaddr_in client_addr = {};
    socklen_t sockelen = sizeof(client_addr);
    int connfd = accept(server_fd, (struct sockaddr *)&client_addr, &sockelen);
    if (connfd < 0) die("accept_new_conn() accept() err\n");

    set_fd_nb(connfd);
    Conn *conn = (Conn *)malloc(sizeof(Conn));
    if (!conn) {
        close(connfd);
        die("Conn *conn memory alocation error\n");
        return;
    }

    conn->fd = connfd;
    conn->state = STATE_REQ;
    conn->rbuf_size = 0;
    conn->wbuf_sent = 0;
    conn->wbuf_size = 0;

    add_conn_fd2conn(fd2conn, connected_clients, max_clients, connfd, conn);
    printf("accept_new_client() ok\n");
}

Conn *conn_get(Conn **fd2conn, int fd) { return fd2conn[fd]; }

bool try_one_req(Conn *conn) {
    if (conn->rbuf_size < 4) {
        printf("try_one_req() message too small\n");
        return false;
    }

    uint32_t len = 0;
    memcpy(&len, conn->rbuf, 4);
    if (len > max_message_length) {
        die("try_one_req() message too long\n");
        conn->state = STATE_END;
        return false;
    }

    {
        printf("client says: %.*s\n", len, &conn->rbuf[4]);
    }

    // memcpy(&conn->wbuf[0], &len, 4);
    // memcpy(&conn->wbuf[4], &conn->rbuf[4], len);
    memcpy(conn->wbuf, conn->rbuf, 4 + len);
    conn->wbuf_size = 4 + len;

    size_t remain = conn->rbuf_size - 4 - len;
    if (remain) {
        memmove(conn->rbuf, &conn->rbuf[4 + len], remain);
    }
    conn->rbuf_size = remain;

    conn->state = STATE_RES;
    state_res(conn);
    return (conn->state == STATE_REQ);
}

bool try_fill_buffer(Conn *conn) {
    ssize_t rv;
    do {
        size_t cap = sizeof(conn->rbuf) - conn->rbuf_size;
        rv = read(conn->fd, &conn->rbuf[conn->rbuf_size], cap);
    } while (rv < 0 && errno == EINTR);
    if (rv < 0 && errno == EAGAIN) {
        return false;
    }
    if (rv < 0) {
        printf("state_req() read error\n");
        conn->state = STATE_END;
        return false;
    }
    if (rv == 0) {
        if (conn->rbuf_size > 0) {
            printf("state_req() read Unexpected EOF\n");
        } else {
            printf("state_req() read EOF\n");
        }
        conn->state = STATE_END;
        return false;
    }

    conn->rbuf_size += (size_t)rv;
    assert(conn->rbuf_size <= sizeof(conn->rbuf));

    while (try_one_req(conn)) {
    };
    return (conn->state == STATE_END);
}

bool try_flush_buffer(Conn *conn) {
    ssize_t rv;
    do {
        size_t remain = conn->wbuf_size - conn->wbuf_sent;
        rv = write(conn->fd, &conn->wbuf[conn->wbuf_sent], remain);
    } while (rv < 0 && errno == EINTR);
    if (rv < 0 && errno == EAGAIN) {
        return false;
    }
    if (rv < 0) {
        printf("try_flush_buffer() write error\n");
        conn->state = STATE_END;
        return false;
    }

    conn->wbuf_sent += (size_t)rv;
    if (conn->wbuf_sent == conn->wbuf_size) {
        conn->state = STATE_REQ;
        conn->wbuf_sent = 0;
        conn->wbuf_size = 0;
        return false;
    }

    return true;
}

void conn_remove(Conn ***fd2conn, int fd, long long *connected_clients) {
    free((*fd2conn)[fd]);
    (*fd2conn)[fd] = NULL;
    (*connected_clients)--;
}

void connection_io(Conn *conn) {
    if (conn->state == STATE_REQ) {
        state_req(conn);
    } else if (conn->state == STATE_RES) {
        state_res(conn);
    } else {
        assert(0);
    }
}
