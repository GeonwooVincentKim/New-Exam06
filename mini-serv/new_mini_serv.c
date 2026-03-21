
```
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <netinet/in.h>

typedef struct s_client {
    int id;
    char msg[100000];
} t_client;

t_client c[2048];
fd_set rd, wr, curr;
int maxfd = 0, gid = 0;
char s_buf[120000], r_buf[110000];

void err(char *msg) {
    if (msg) write(2, msg, strlen(msg));
    else write(2, "Fatal error", 11);
    write(2, "\n", 1);
    exit(1);
}

void send_all(int except) {
    for (int fd = 0; fd <= maxfd; fd++) {
        if (FD_ISSET(fd, &wr) && fd != except)
            send(fd, s_buf, strlen(s_buf), 0);
    }
}

int main(int ac, char **av) {
    if (ac != 2) err("Wrong number of arguments");

    int sfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sfd == -1) err(NULL);
    maxfd = sfd;

    FD_ZERO(&curr);
    FD_SET(sfd, &curr);

    // struct sockaddr_in sa;
    // memset(&sa, 0, sizeof(sa));
    // sa.sin_family = AF_INET;
    // sa.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    // sa.sin_port = htons(atoi(av[1]));
    struct sockaddr_in serv_addr;
    bzero(&serv_addr, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    ((unsigned char )&serv_addr.sin_addr.s_addr)[0] = 127;
    ((unsigned char)&serv_addr.sin_addr.s_addr)[1] = 0;
    ((unsigned char )&serv_addr.sin_addr.s_addr)[2] = 0;
    ((unsigned char)&serv_addr.sin_addr.s_addr)[3] = 1;
    serv_addr.sin_port = (atoi(av[1])<<8) | (atoi(av[1]) >> 8);

    if(bind(serverfd, (const struct sockaddr *)&serv_addr, sizeof(serv_addr)) == -1 || listen(serverfd, 128) == -1)
        err(NULL); 

    while (1) {
        rd = wr = curr;
        if (select(maxfd + 1, &rd, &wr, 0, 0) == -1) continue;

        for (int fd = 0; fd <= maxfd; fd++) {
            if (!FD_ISSET(fd, &rd)) continue;

            if (fd == sfd) {
                struct sockaddr_in c_sa;
                socklen_t len = sizeof(c_sa);
                int cfd = accept(sfd, (struct sockaddr *)&c_sa, &len);
                
                if (cfd == -1) continue;
                if (cfd > maxfd) maxfd = cfd;
                c[cfd].id = gid++;
                c[cfd].msg[0] = '\0';
                FD_SET(cfd, &curr);
                sprintf(s_buf, "server: client %d just arrived\n", c[cfd].id);
                send_all(cfd);
                break;
            } else {
                int n = recv(fd, r_buf, 100000, 0);
                if (n <= 0) {
                    sprintf(s_buf, "server: client %d just left\n", c[fd].id);
                    send_all(fd);
                    FD_CLR(fd, &curr);
                    close(fd);
                    break;
                }
                r_buf[n] = '\0';
                for (int i = 0; i < n; i++) {
                    int l = strlen(c[fd].msg);
                    c[fd].msg[l] = r_buf[i];
                    c[fd].msg[l+1] = '\0';
                    if (c[fd].msg[l] == '\n') {
                        sprintf(s_buf, "client %d: %s", c[fd].id, c[fd].msg);
                        send_all(fd);
                        c[fd].msg[0] = '\0';
                    }
                }
            }
        }
    }
}
