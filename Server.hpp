#pragma once
#include <vector>
#include <map>
#include <string>
#include <sys/epoll.h>

class Client;
class ICommand;
class Channel;

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

    // Channel management (basic operations)
    Channel* getChannel(const std::string& channelName);
    Channel* getOrCreateChannel(const std::string& channelName);
    void removeChannel(const std::string& channelName);
    Client* getClientByFd(int fd);
    Client* getClientByNickname(const std::string& nickname);

    // Client-Channel operations (high-level helpers)
    void addClientToChannel(Client* client, Channel* channel);
    void removeClientFromChannel(Client* client, Channel* channel);
    void removeClientFromAllChannels(Client* client);

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
    std::map<std::string, Channel*> _channels;

    void _initCommands();
    void _cleanupCommands();
    void _initServer();
    const std::string _generateTimeString(time_t startTime) const;
    void _handleNewConnection();
    void _handleClientRecv(int fd);
    void _handleClientSend(int fd);
    void _handleClientDisconnect(int fd);
    void _processCommand(int fd, const std::string& commandLine);
    std::vector<std::string> _splitArgs(const std::string& commandLine);
};
