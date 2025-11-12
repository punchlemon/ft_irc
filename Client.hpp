#pragma once
#include <string>
#include <set>
#include <sys/types.h>
#include <stdint.h>

class Server;
class Channel;

class Client {
private:
    int _fd;
    std::string _recvBuffer;
    std::string _sendBuffer;
    std::string _password;
    std::string _nickname;
    std::string _username;
    std::string _realname;
    std::string _hostname;
    bool _hasRegistered;
    std::set<char> _modes;
    std::set<Channel*> _joinedChannels;
    Server* _server;
    uint32_t _epollEvents;

    Client();
    Client(const Client& other);
    Client& operator=(const Client& other);

public:
    explicit Client(int fd, const std::string& hostname, Server* server, uint32_t epollEvents);
    ~Client();

    int getFd() const;
    const std::string& getPassword() const;
    const std::string& getNickname() const;
    const std::string& getUsername() const;
    const std::string& getHostname() const;
    const std::string& getRealname() const;
    bool hasRegistered() const;
    bool hasMode(char mode) const;
    std::string getPrefix() const;

    const std::string& getRecvBuffer() const;
    const std::string& getSendBuffer() const;

    uint32_t getEpollEvents() const;

    void queueMessage(const std::string& message);
    void reply(int replyCode, const std::string& message);

    void setPassword(const std::string& password);
    void setNickname(const std::string& nickname);
    void setUsername(const std::string& username);
    void setRealname(const std::string& realname);
    void setHasRegistered(bool val);

    void appendRecvBuffer(const char* buf, ssize_t len);
    void clearRecvBuffer(size_t len);
    void appendSendBuffer(const char* buf, ssize_t len);
    void clearSendBuffer(size_t len);

    void setEpollEvents(uint32_t events);

    void addMode(char mode);
    void removeMode(char mode);

    void addChannel(Channel* channel);
    void removeChannel(Channel* channel);
    const std::set<Channel*>& getJoinedChannels() const;
    bool isInChannel(Channel* channel) const;
};
