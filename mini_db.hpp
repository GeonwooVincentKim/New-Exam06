#include <iostream>
#include <stdexcept>
#include <cstring>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>
#include <map>
#include <sstream>


//copy all the main.c in the subject
class Socket
{
private:
struct sockaddr_in _servaddr;

public:
	int _sockfd;
	Socket(int port) :
			_sockfd(socket(AF_INET, SOCK_STREAM, 0))
		{
			if(_sockfd == -1){
				throw std::runtime_error("Socket creation failed");
			}

			memset(&_servaddr, 0, sizeof(_servaddr));
			_servaddr.sin_family = AF_INET;
			_servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
			_servaddr.sin_port = htons(port);
		}

	~Socket()
		{
			if (_sockfd != -1)
			{
				close(_sockfd);
			}
		}
	void bindAndListen()
		{
			if(bind(_sockfd, (struct sockaddr *)&_servaddr, sizeof(_servaddr)) < 0)
			{
				throw std::runtime_error("Socket bind failed");
			}

			if (listen(_sockfd, 10) < 0)
			{
				throw std::runtime_error("Socket listen failed");
			}
		}
	int accept(struct sockaddr_in& clientAddr)
		{
			socklen_t clientLen = sizeof(clientAddr);
			int clientSocketFd = ::accept(_sockfd, (struct sockaddr*)&clientAddr, &clientLen);
			if(clientSocketFd < 0)
			{
				throw std::runtime_error("Failed to accept connection");
			}
			return clientSocketFd;
		}
		//change pull message to retrive the message add int clientFd as param
		std::string pullMessage(int clientFd)
		{
			char buf[1024];
			int byte_read = recv(clientFd, buf, 1000, 0);
			if(byte_read <= 0)
				return std::string("");
			buf[byte_read] = '\0';
			std::string msg(buf);
			return msg;
		}
};

//ad ad db reference to constructor
class Server
{
private: 
	Socket _listeningSocket;
	std::map<std::string, std::string> &db;
	fd_set rfds, wfds, afds;
	int max_fd = 0;
public:
	Server(int port, std::map<std::string, std::string>&database) :
		_listeningSocket(port), db(database)
		{
			FD_ZERO(&afds);
		}
	//add handler function
	void handlemessage(int clientFd, std::string message){
		std::istringstream msg(message);
		std::string command, key, value;
		msg >> command >> key >> value;
		if(command == "POST" && !value.empty()){
			db[key] = value;
			send(clientFd, "0\n", 2, 0);
		}
		else if(command == "GET" && value.empty()){
			auto it = db.find(key);
			if(it != db.end()){
				std::string ret = "0 " + it->second + '\n';
				send(clientFd, ret.c_str(), ret.size(), 0);
			}
			else
				send(clientFd, "1\n", 2 ,0);

		}else if(command == "DELETE" && value.empty()){
			auto it = db.find(key);
			if(it != db.end()){
				db.erase(it);
				send(clientFd, "0\n", 2, 0);
			}
			else
				send(clientFd, "1\n", 2 ,0);
		}else{
			send(clientFd, "2\n", 2, 0);
		}
	}
	int run()
		{
			try
			{
				_listeningSocket.bindAndListen();
				max_fd = _listeningSocket._sockfd; 
				FD_SET(max_fd, &afds);
				std::cout << "ready" << std::endl;
				while(true){
					std::cout << "AAAAAAAAAAAAAA" << std::endl;
					sockaddr_in clientAddr;
					rfds = wfds = afds;
					if (select(max_fd + 1, &rfds, &wfds, NULL, NULL) < 0)
						throw std::runtime_error("error in select");
					for (int fd = 0; fd <= max_fd; fd++){
						std::cout << "BBBBBBBB" << std::endl;
						if (!FD_ISSET(fd, &rfds))
							continue;
						if (fd == _listeningSocket._sockfd){
							int clientFd = _listeningSocket.accept(clientAddr);
							FD_SET(clientFd, &afds);
							max_fd = clientFd > max_fd ? clientFd : max_fd;
						}
						else{
							std::string message = _listeningSocket.pullMessage(fd);
							if(message.empty()){
								close(fd);
								FD_CLR(fd, &afds);
								break;
							}
							handlemessage(fd, message);
						}
					}
				}
				return 0; //Success
			}
			catch(const std::exception& e)
			{
				std::cerr << "Error during server run: " << e.what() << std::endl;
				return 1; // Return error code if server fails to start
			}
		}
};