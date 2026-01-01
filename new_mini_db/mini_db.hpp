#include <iostream>
#include <string>
#include <map>
#include <fstream>
#include <sstream>
#include <vector>
#include <netinet/in.h>
#include <unistd.h>
#include <signal.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <cstring>
#include <cstdlib>

using namespace std;

string g_filename;
map<string, string> g_db;
int g_sockfd = -1;

void save_db_and_exit(int) {
    ofstream file(g_filename.c_str());
    if (file.is_open()) {
        for (map<string, string>::iterator it = g_db.begin(); it != g_db.end(); ++it)
            file << it->first << " " << it->second << "\n";
        file.close();
    }
    if (g_sockfd != -1) close(g_sockfd);
    exit(0);
}

void send_res(int fd, const string &msg) { send(fd, msg.c_str(), msg.length(), 0); }

int main(int ac, char **av) {
    if (ac != 3) return 1;
    g_filename = av[2];
    ifstream infile(g_filename.c_str());
    if (infile.is_open()) {
        string key, val;
        while (infile >> key >> val) g_db[key] = val;
        infile.close();
    }

    g_sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (g_sockfd == -1) return 1;

    struct sockaddr_in servaddr;
    bzero(&servaddr, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    servaddr.sin_port = htons(atoi(av[1]));

    if (bind(g_sockfd, (struct sockaddr *)&servaddr, sizeof(servaddr)) != 0 || listen(g_sockfd, 100) != 0)
        return (close(g_sockfd), 1);

    signal(SIGINT, save_db_and_exit);
    
    cout << "ready" << endl;  

    fd_set current_sockets, ready_sockets;
    FD_ZERO(&current_sockets);
    FD_SET(g_sockfd, &current_sockets);
    int max_fd = g_sockfd;
    map<int, string> buffers;

    while (true) {
        ready_sockets = current_sockets;
        if (select(max_fd + 1, &ready_sockets, NULL, NULL, NULL) < 0) continue;

        for (int i = 0; i <= max_fd; i++) {
            if (!FD_ISSET(i, &ready_sockets)) continue;

            if (i == g_sockfd) {
                int client_fd = accept(g_sockfd, NULL, NULL);
                if (client_fd >= 0) {
                    FD_SET(client_fd, &current_sockets);
                    if (client_fd > max_fd) max_fd = client_fd;
                    buffers[client_fd] = "";
                }
            } else {
                char buf[1025];
                int bytes = recv(i, buf, 1000, 0);
                if (bytes <= 0) {
                    close(i);
                    FD_CLR(i, &current_sockets);
                    buffers.erase(i);
                } else {
                    buf[bytes] = 0;
                    buffers[i] += buf;
                    size_t pos = 0;
                    while ((pos = buffers[i].find('\n')) != string::npos) {
                        string line = buffers[i].substr(0, pos);
                        buffers[i].erase(0, pos + 1);
                        if (!line.empty() && line[line.length() - 1] == '\r') line.erase(line.length() - 1);
                        
                        stringstream ss(line);
                        string cmd, key, val;
                        ss >> cmd >> key >> val;

                        if (cmd == "POST" && !key.empty() && !val.empty()) {
                            g_db[key] = val;
                            send_res(i, "0\n");
                        } else if (cmd == "GET" && !key.empty() && val.empty()) {
                            send_res(i, g_db.count(key) ? "0 " + g_db[key] + "\n" : "1\n");
                        } else if (cmd == "DELETE" && !key.empty() && val.empty()) {
                            send_res(i, g_db.erase(key) ? "0\n" : "1\n");
                        } else {
                            send_res(i, "2\n");
                        }
                    }
                }
            }
        }
    }
    return 0;
}
