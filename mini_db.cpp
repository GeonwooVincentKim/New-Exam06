#include "mini_db.hpp"
#include <iostream>
#include <map>
#include <fstream>
#include <signal.h>


std::string path;
std::map<std::string, std::string> db;


void handler(int signalNum){
	std::ofstream file(path);
	if(!file.is_open())
	{
		std::cout << "Invalid file path" << std::endl;
		exit(signalNum); 
	}
	for (auto const& it : db){
		file << it.first << " " << it.second << std::endl;
	}
	file.close();
	exit(signalNum);
}

void readDb(){
	std::ifstream file(path);
	if(!file.is_open())
		return;
	std::string key, value;
	while(file >> key >> value){
		db[key] = value;
	}
}

int main(int ac, char **av)
{
	if(ac != 3)
	{
		std::cout << "Wrong number of argument" << std::endl;
		return (1); 
	}
	signal(SIGINT, handler);
	path = av[2];
	readDb();
	Server server(atoi(av[1]), db);
	server.run();
	return 0;
}