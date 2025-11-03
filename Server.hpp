#pragma once
#include <vector>
#include <map>
#include <string>
#include <sys/epoll.h>

class Client;

class Server {
public:
    Server();
    Server(int port, const std::string& password);
    ~Server();

    void run();

private:
    Server(const Server& other);
    Server& operator=(const Server& other);

    int _port;
    std::string _password;
    int _serverFd;
    int _epollFd;
    std::vector<struct epoll_event> _events;
    std::map<int, Client*> _clients;

    void _initServer();
    void _handleNewConnection();
    void _handleClientData(int fd);
    void _handleClientDisconnect(int fd);
    void _processCommand(int fd, const std::string& commandLine);
};
