#include <iostream>
#include <stdexcept>
#include <cstring>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>
#include <map>
#include <sstream>
#include <fstream>
#include <signal.h>

std::string path;
std::map<std::string, std::string> db;

void handler(int signalNum) {
	std::ofstream file(path);
	if (!file.is_open()) {
		std::cout << "Invalid file path" << std::endl;
		exit(signalNum);
	}
	for (auto const& it : db) {
		file << it.first << " " << it.second << std::endl;
	}
	file.close();
	exit(signalNum);
}

void readDb() {
	std::ifstream file(path);
	if (!file.is_open())
		return;
	std::string key, value;
	while (file >> key >> value) {
		db[key] = value;
	}
}

class Socket {
private:
	struct sockaddr_in _servaddr;
public:
	int _sockfd;
	Socket(int port) : _sockfd(socket(AF_INET, SOCK_STREAM, 0)) {
		if (_sockfd == -1) {
			throw std::runtime_error("Socket creation failed");
		}
		memset(&_servaddr, 0, sizeof(_servaddr));
		_servaddr.sin_family = AF_INET;
		_servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
		_servaddr.sin_port = htons(port);
	}
	~Socket() {
		if (_sockfd != -1) {
			close(_sockfd);
		}
	}
	void bindAndListen() {
		if (bind(_sockfd, (struct sockaddr *)&_servaddr, sizeof(_servaddr)) < 0) {
			throw std::runtime_error("Socket bind failed");
		}
		if (listen(_sockfd, 10) < 0) {
			throw std::runtime_error("Socket listen failed");
		}
	}
	int accept(struct sockaddr_in& clientAddr) {
		socklen_t clientLen = sizeof(clientAddr);
		int clientSocketFd = ::accept(_sockfd, (struct sockaddr*)&clientAddr, &clientLen);
		if (clientSocketFd < 0) {
			throw std::runtime_error("Failed to accept connection");
		}
		return clientSocketFd;
	}
	std::string pullMessage(int clientFd) {
		char buf[1024];
		int byte_read = recv(clientFd, buf, 1000, 0);
		if (byte_read <= 0)
			return std::string("");
		buf[byte_read] = '\0';
		return std::string(buf);
	}
};

class Server {
private:
	Socket _listeningSocket;
	std::map<std::string, std::string> &db;
	fd_set rfds, wfds, afds;
	int max_fd = 0;
public:
	Server(int port, std::map<std::string, std::string>& database) : _listeningSocket(port), db(database) {
		FD_ZERO(&afds);
	}
	void handlemessage(int clientFd, std::string message) {
		std::istringstream msg(message);
		std::string command, key, value;
		msg >> command >> key >> value;
		if (command == "POST" && !value.empty()) {
			db[key] = value;
			send(clientFd, "0\n", 2, 0);
		} else if (command == "GET" && value.empty()) {
			auto it = db.find(key);
			if (it != db.end()) {
				std::string ret = "0 " + it->second + '\n';
				send(clientFd, ret.c_str(), ret.size(), 0);
			} else {
				send(clientFd, "1\n", 2, 0);
			}
		} else if (command == "DELETE" && value.empty()) {
			auto it = db.find(key);
			if (it != db.end()) {
				db.erase(it);
				send(clientFd, "0\n", 2, 0);
			} else {
				send(clientFd, "1\n", 2, 0);
			}
		} else {
			send(clientFd, "2\n", 2, 0);
		}
	}
	int run() {
		try {
			_listeningSocket.bindAndListen();
			max_fd = _listeningSocket._sockfd;
			FD_SET(max_fd, &afds);
			std::cout << "ready" << std::endl;
			while (true) {
				sockaddr_in clientAddr;
				rfds = wfds = afds;
				if (select(max_fd + 1, &rfds, &wfds, NULL, NULL) < 0) {
					throw std::runtime_error("error in select");
				}
				for (int fd = 0; fd <= max_fd; fd++) {
					if (!FD_ISSET(fd, &rfds))
						continue;
					if (fd == _listeningSocket._sockfd) {
						int clientFd = _listeningSocket.accept(clientAddr);
						FD_SET(clientFd, &afds);
						max_fd = clientFd > max_fd ? clientFd : max_fd;
					} else {
						std::string message = _listeningSocket.pullMessage(fd);
						if (message.empty()) {
							close(fd);
							FD_CLR(fd, &afds);
							break;
						}
						handlemessage(fd, message);
					}
				}
			}
			return 0;
		} catch (const std::exception& e) {
			std::cerr << "Error during server run: " << e.what() << std::endl;
			return 1;
		}
	}
};

int main(int ac, char **av) {
	if (ac != 3) {
		std::cout << "Wrong number of argument" << std::endl;
		return 1;
	}
	signal(SIGINT, handler);
	path = av[2];
	readDb();
	Server server(atoi(av[1]), db);
	server.run();
	return 0;
}
