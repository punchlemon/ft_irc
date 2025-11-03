#pragma once
#include <string>
#include <set>
#include <sys/types.h>

class Client {
private:
    int _fd;
    std::string _recvBuffer;
    std::string _sendBuffer;
    std::string _nickname;
    std::string _username;
    std::string _hostname;
    bool _isAuthenticated;
    std::set<char> _modes;

public:
    Client();
    explicit Client(int fd, const std::string& hostname);
    Client(const Client& other);
    Client& operator=(const Client& other);
    ~Client();

    void swap(Client& other);
    int getFd() const;
    const std::string& getNickname() const;
    const std::string& getUsername() const;
    const std::string& getHostname() const;
    bool isAuthenticated() const;
    bool hasMode(char mode) const;
    std::string getPrefix() const;

    const std::string& getRecvBuffer() const;
    const std::string& getSendBuffer() const;

    void setNickname(const std::string& nickname);
    void setUsername(const std::string& username);
    void setAuthenticated(bool val);

    void appendRecvBuffer(const char* buf, ssize_t len);
    void clearRecvBuffer(size_t len);
    void appendSendBuffer(const char* buf, ssize_t len);
    void clearSendBuffer(size_t len);

    void addMode(char mode);
    void removeMode(char mode);
};
