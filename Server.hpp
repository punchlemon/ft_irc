#pragma once
#include <vector>
#include <map>
#include <string>
#include <sys/epoll.h>

class Client;
class ICommand;

class Server {
public:
    Server(int port, const std::string& password);
    ~Server();

    void run();
    int getPort() const;
    void enableEpollOut(int fd);
    void disableEpollOut(int fd);
    const std::string& getPassword() const;
    const std::string getServerName() const;
    const std::string getStartTimeString() const;
    void shutdown();

private:
    Server();
    Server(const Server& other);
    Server& operator=(const Server& other);

    std::string _serverName;
    int _port;
    std::string _password;
    int _serverFd;
    int _epollFd;
    std::string _startTimeString;
    std::vector<struct epoll_event> _events;
    std::map<int, Client*> _clients;
    std::map<std::string, ICommand*> _commands;

    void _initCommands();
    void _cleanupCommands();
    void _initServer();
    const std::string _generateTimeString(time_t startTime) const;
    void _handleNewConnection();
    void _handleClientRecv(int fd);
    void _handleClientSend(int fd);
    void _handleClientDisconnect(int fd);
    void _processCommand(int fd, const std::string& commandLine);
};
