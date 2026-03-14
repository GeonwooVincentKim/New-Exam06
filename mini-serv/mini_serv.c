#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <unistd.h>

int extract_message(char **buf, char **msg) {
  char *newbuf;
  int i;
  if (!*buf)
    return (0);
  for (i = 0; (*buf)[i]; i++) {
    if ((*buf)[i] == '\n') {
      newbuf = calloc(1, sizeof(char) * (strlen(*buf + i + 1) + 1));
      if (!newbuf)
        return (-1);
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
  if (!newbuf)
    return (0);
  newbuf[0] = 0;
  if (buf)
    strcat(newbuf, buf);
  free(buf);
  strcat(newbuf, add);
  return (newbuf);
}

void err_msg(char *msg) {
  write(2, msg, strlen(msg));
  exit(1);
}

void fatal() { err_msg("Fatal error\n"); }

void broadcast(char *msg, int *client_fds, int count, int ignfd){
    int len = strlen(msg);
    for (int i = 0; i < count; i++) {
      if (client_fds[i] > 0 && client_fds[i] != ignfd) {
        int sent = 0;
        while (sent < len) {
          ssize_t n = send(client_fds[i], msg + sent, len - sent, 0);
          if (n <= 0)
            break;
          sent += n;
        }
      }
    }
}

int main(int argc, char **argv) {
  if (argc != 2) {
    err_msg("Wrong number of arguments\n");
  }

  int sockfd = socket(AF_INET, SOCK_STREAM, 0);
  if (sockfd == -1)
    fatal();

  struct sockaddr_in addr;
  bzero(&addr, sizeof(addr));
  addr.sin_family = AF_INET;
  addr.sin_addr.s_addr = htonl(2130706433); // 127.0.0.1
  addr.sin_port = htons(atoi(argv[1]));

  if ((bind(sockfd, (struct sockaddr *)&addr, sizeof(addr))) != 0)
    fatal();
  if (listen(sockfd, 10) != 0)
    fatal();

  char recvbuff[1024];
  int recvbuff_size = 1024;

  char *client_msgs[256] = {};
  int clients[256] = {};

  int appear_clients = 0;
  int maxfd = sockfd;

  for (;;) {
    int ready;
    ssize_t nbytes;
    fd_set readfds;

    memset(recvbuff, 0, recvbuff_size);

    FD_ZERO(&readfds);
    FD_SET(sockfd, &readfds);
    for (int i = 0; i < appear_clients; i++) {
      if (clients[i] > 0) {
        FD_SET(clients[i], &readfds);
      }
    }

    ready = select(maxfd + 1, &readfds, NULL, NULL, NULL);
    if (ready == -1) {
      fatal();
    }

    int fd;
    if (FD_ISSET(sockfd, &readfds)) {
      socklen_t addrlen;
      struct sockaddr_in client_addr;

      addrlen = sizeof(client_addr);
      memset(&client_addr, 0, addrlen);
      fd = accept(sockfd, (struct sockaddr *)&client_addr, &addrlen);
      if (fd == -1) {
        fatal();
      } else {
        clients[appear_clients] = fd;
        if (fd > maxfd)
          maxfd = fd;
        char sendbuff[64];
        sprintf(sendbuff, "server: client %d just arrived\n", appear_clients);
        broadcast(sendbuff, clients, appear_clients + 1, fd);
        appear_clients++;
        continue;
      }
    }

    for (int i = 0; i < appear_clients; i++) {
      fd = clients[i];
      if (fd > 0 && FD_ISSET(fd, &readfds)) {
        nbytes = recv(fd, recvbuff, recvbuff_size - 1, 0);
        if (nbytes < 1) {
          close(fd);
          clients[i] = 0;
          free(client_msgs[i]);
          client_msgs[i] = NULL;
          char sendbuff[64];
          sprintf(sendbuff, "server: client %d just left\n", i);
          broadcast(sendbuff, clients, appear_clients, 0);

        } else {
          recvbuff[nbytes] = '\0';
          client_msgs[i] = str_join(client_msgs[i], recvbuff);
          if (client_msgs[i] == NULL) {
            fatal();
          }
          char *each_msg = NULL;
          int ret;
          while ((ret = extract_message(&client_msgs[i], &each_msg)) == 1) {
            int msglen = strlen(each_msg) + 32;
            char *sendbuff = malloc(msglen);
            if (!sendbuff) fatal();
            sprintf(sendbuff, "client %d: %s", i, each_msg);
            broadcast(sendbuff, clients, appear_clients, fd);
            free(sendbuff);
            free(each_msg);
            each_msg = NULL;
          }
          if (ret == -1) {
            fatal();
          }
        }
      }
      // loop end
    }
  }
}