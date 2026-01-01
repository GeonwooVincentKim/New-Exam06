#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <netinet/in.h>
#include <stdlib.h>
#include <stdio.h>

int count = 0, max_fd = 0, sockfd = 0;
int ids[65536];
char *msgs[65536];
fd_set active, read_fds;
char buf_read[424242], msg_buf[424242];

void fatal() {
    write(2, "Fatal error\n", 12);
    exit(1);
}

int extract_message(char **buf, char **msg) {
    char *newbuf;
    int i;
    if (!*buf) return (0);
    for (i = 0; (*buf)[i]; i++) {
        if ((*buf)[i] == '\n') {
            newbuf = calloc(1, sizeof(char) * (strlen(*buf + i + 1) + 1));
            if (!newbuf) return (-1);
            strcpy(newbuf, *buf + i + 1);
            *msg = *buf;
            (*msg)[i + 1] = 0;
            *buf = newbuf;
            return (1);
        }
    }
    return (0);
}

char *str_join(char *buf, char *add) {
    char *newbuf;
    int len = buf ? strlen(buf) : 0;
    newbuf = malloc(sizeof(char) * (len + strlen(add) + 1));
    if (!newbuf) return (0);
    newbuf[0] = 0;
    if (buf) strcat(newbuf, buf);
    free(buf);
    strcat(newbuf, add);
    return (newbuf);
}

void notify(int sender, char *s) {
    for (int fd = 0; fd <= max_fd; fd++) {
        if (FD_ISSET(fd, &active) && fd != sender && fd != sockfd)
            send(fd, s, strlen(s), 0);
    }
}

int main(int ac, char **av) {
    if (ac != 2) { write(2, "Wrong number of arguments\n", 26); exit(1); }
    
    sockfd = socket(AF_INET, SOCK_STREAM, 0); 
    if (sockfd == -1) fatal();
    
    // setsockopt 제거됨 (Subject 허용 함수 아님)

    struct sockaddr_in addr; 
    bzero(&addr, sizeof(addr)); 
    addr.sin_family = AF_INET; 
    addr.sin_addr.s_addr = htonl(2130706433); // 127.0.0.1
    addr.sin_port = htons(atoi(av[1]));

    if ((bind(sockfd, (struct sockaddr *)&addr, sizeof(addr))) != 0) fatal();
    if (listen(sockfd, 10) != 0) fatal();

    FD_ZERO(&active);
    FD_SET(sockfd, &active);
    max_fd = sockfd;

    while (1) {
        read_fds = active;
        // write_fds는 NULL 처리하여 CPU 점유율 방지
        if (select(max_fd + 1, &read_fds, NULL, NULL, NULL) < 0) continue;

        for (int fd = 0; fd <= max_fd; fd++) {
            if (!FD_ISSET(fd, &read_fds)) continue;

            if (fd == sockfd) {
                int conn = accept(sockfd, NULL, NULL);
                if (conn >= 0) {
                    FD_SET(conn, &active);
                    if (conn > max_fd) max_fd = conn;
                    ids[conn] = count++;
                    msgs[conn] = NULL;
                    sprintf(msg_buf, "server: client %d just arrived\n", ids[conn]);
                    notify(conn, msg_buf);
                }
            } else {
                int ret = recv(fd, buf_read, 1000, 0);
                if (ret <= 0) {
                    sprintf(msg_buf, "server: client %d just left\n", ids[fd]);
                    notify(fd, msg_buf);
                    free(msgs[fd]);
                    msgs[fd] = NULL;
                    FD_CLR(fd, &active);
                    close(fd);
                } else {
                    buf_read[ret] = 0;
                    msgs[fd] = str_join(msgs[fd], buf_read);
                    if (!msgs[fd]) fatal();
                    char *s = NULL;
                    int extract_ret;
                    while ((extract_ret = extract_message(&msgs[fd], &s))) {
                        if (extract_ret == -1) fatal();
                        sprintf(msg_buf, "client %d: %s", ids[fd], s);
                        notify(fd, msg_buf);
                        free(s);
                        s = NULL;
                    }
                }
            }
        }
    }
    return (0);
}
