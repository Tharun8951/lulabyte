#include "communicate.h"
#include <string.h>
#include <sys/socket.h>
#include <stdio.h>
#include <stdint.h>
#include <unistd.h>

int read_query(int client_sock, char *rbuf, size_t n) {
    while (n > 0) {
        ssize_t bytes_read = read(client_sock, rbuf, n);
        if (bytes_read <= 0) {
            if (bytes_read < 0) {
                perror("read_query");
            } else {
                printf("Client disconnected\n");
            }
            return -1;
        }
        n -= (size_t)bytes_read;
        rbuf += bytes_read;
    }
    printf("Exiting read_query()\n");
    return 0;
}

int write_response(int client_sock, const char *wbuf, size_t n) {
    while (n > 0) {
        ssize_t bytes_written = write(client_sock, wbuf, n);
        if (bytes_written <= 0) {
            if (bytes_written < 0) {
                perror("write_response");
            } else {
                printf("Client disconnected\n");
            }
            return -1;
        }
        n -= (size_t)bytes_written;
        wbuf += bytes_written;
    }
    printf("Exiting write_response()\n");
    return 0;
}

int handle_query(int client_sock) {
    printf("\n\nInside the handle_query function\n");
    printf("max_msg_length: %d\n", max_message_length);

    char rbuf[4 + max_message_length + 1];
    int err = read_query(client_sock, rbuf, 4);
    if (err < 0) {
        printf("read_query() error!\n");
        return -1;
    }

    uint32_t req_message_length = 0;
    memcpy(&req_message_length, rbuf, 4);
    if (req_message_length > max_message_length) {
        printf("Request Message too long!\n");
        const char *wbuf = "Request Message too long!\n";
        write_response(client_sock, wbuf, strlen(wbuf));
        return -1;
    }

    err = read_query(client_sock, &rbuf[4], req_message_length);
    if (err < 0) {
        printf("read_query() error!\n");
        return -1;
    }

    rbuf[4 + req_message_length] = '\0';
    printf("Client message: %s and length is %d\n", &rbuf[4], req_message_length);

    const char *reply = &rbuf[4];
    int res_message_length = (int)strlen(reply);
    char wbuf[4 + res_message_length];
    memcpy(wbuf, &res_message_length, 4);
    memcpy(&wbuf[4], reply, res_message_length);

    int err1 = write_response(client_sock, wbuf, 4 + res_message_length);
    if (err1 < 0) {
        printf("write_response() error!\n");
        return -1;
    }
    return 0;
}
