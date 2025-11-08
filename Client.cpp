#include "Client.hpp"
#include "Server.hpp"
#include <unistd.h>
#include <iostream>
#include <sstream>

Client::Client(int fd, const std::string& hostname, Server* server, uint32_t epollEvents)
    : _fd(fd), _hostname(hostname), _isAuthenticated(false), _server(server), _epollEvents(epollEvents) {
}

Client::Client(const Client& other)
    : _fd(other._fd),
      _recvBuffer(other._recvBuffer),
      _sendBuffer(other._sendBuffer),
      _nickname(other._nickname),
      _username(other._username),
      _hostname(other._hostname),
      _isAuthenticated(other._isAuthenticated),
      _modes(other._modes) {
}

Client& Client::operator=(const Client& other) {
    if (this == &other) {
        return *this;
    }
    Client temp(other);
    this->swap(temp);
    return *this;
}

Client::~Client() {}

void Client::swap(Client& other) {
    std::swap(_fd, other._fd);
    std::swap(_isAuthenticated, other._isAuthenticated);
    _recvBuffer.swap(other._recvBuffer);
    _sendBuffer.swap(other._sendBuffer);
    _nickname.swap(other._nickname);
    _username.swap(other._username);
    _hostname.swap(other._hostname);
    _modes.swap(other._modes);
}

int Client::getFd() const {
    return _fd;
}

const std::string& Client::getNickname() const {
    return _nickname;
}

const std::string& Client::getUsername() const {
    return _username;
}

const std::string& Client::getHostname() const {
    return _hostname;
}

bool Client::isAuthenticated() const {
    return _isAuthenticated;
}

std::string Client::getPrefix() const {
    if (_nickname.empty()) {
        return "*";
    }
    return _nickname + "!" + (_username.empty() ? "*" : _username) + "@" + _hostname;
}

const std::string& Client::getRecvBuffer() const {
    return _recvBuffer;
}

const std::string& Client::getSendBuffer() const {
    return _sendBuffer;
}

uint32_t Client::getEpollEvents() const {
    return _epollEvents;
}

void Client::queueMessage(const std::string& message) {
    _sendBuffer += message;
    _server->enableEpollOut(_fd);
}

void Client::reply(int replyCode, const std::string& message) {
    std::ostringstream oss;
    oss << replyCode;
    std::string replyCodeStr = oss.str();
    if (replyCodeStr.length() < 3) {
        replyCodeStr = std::string(3 - replyCodeStr.length(), '0') + replyCodeStr;
    }
    std::string replyMsg = ":" + _server->getServerName() + " " + replyCodeStr + " " + getPrefix() + " " + message + " :";
    switch (replyCode) {
        case 401: // ERR_NOSUCHNICK
        case 402: // ERR_NOSUCHSERVER
        case 403: // ERR_NOSUCHCHANNEL
        case 404: // ERR_CANNOTSENDTOCHAN
        case 431: // ERR_NONICKNAMEGIVEN
        case 432: // ERR_ERRONEUSNICKNAME
        case 433: // ERR_NICKNAMEINUSE
        case 461: // ERR_NEEDMOREPARAMS
            replyMsg += "Syntax error";
        case 462: // ERR_ALREADYREGISTERED
        case 464: // ERR_PASSWDMISMATCH
            break;
        default:
            break;
    }
    replyMsg += "\r\n";
    queueMessage(replyMsg);
}

bool Client::hasMode(char mode) const {
    return _modes.count(mode) > 0;
}

void Client::setNickname(const std::string& nickname) {
    _nickname = nickname;
}

void Client::setUsername(const std::string& username) {
    _username = username;
}

void Client::setAuthenticated(bool val) {
    _isAuthenticated = val;
}

void Client::appendRecvBuffer(const char* buf, ssize_t len) {
    _recvBuffer.append(buf, len);
}

void Client::clearRecvBuffer(size_t len) {
    if (len >= _recvBuffer.length()) {
        _recvBuffer.clear();
    } else {
        _recvBuffer.erase(0, len);
    }
}

void Client::appendSendBuffer(const char* buf, ssize_t len) {
    _sendBuffer.append(buf, len);
}

void Client::clearSendBuffer(size_t len) {
    if (len >= _sendBuffer.length()) {
        _sendBuffer.clear();
    } else {
        _sendBuffer.erase(0, len);
    }
}

void Client::setEpollEvents(uint32_t events) {
    _epollEvents = events;
}

void Client::addMode(char mode) {
    _modes.insert(mode);
}

void Client::removeMode(char mode) {
    _modes.erase(mode);
}
