#include "Client.hpp"
#include "Server.hpp"
#include <unistd.h>
#include <iostream>
#include <sstream>

Client::Client(int fd, const std::string& hostname, Server* server, uint32_t epollEvents):
    _fd(fd),
    _recvBuffer(""),
    _sendBuffer(""),
    _password(""),
    _nickname(""),
    _username(""),
    _hostname(hostname),
    _hasRegistered(false),
    _server(server),
    _epollEvents(epollEvents)
{}

Client::~Client() {}

int Client::getFd() const {
    return _fd;
}

const std::string& Client::getPassword() const {
    return _password;
}

const std::string& Client::getNickname() const {
    return _nickname;
}

const std::string& Client::getUsername() const {
    return _username;
}

const std::string& Client::getRealname() const {
    return _realname;
}

const std::string& Client::getHostname() const {
    return _hostname;
}

bool Client::hasRegistered() const {
    return _hasRegistered;
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
        case 451: // ERR_NOTREGISTERED
            replyMsg = "Connection not registered";
            break;
        case 461: // ERR_NEEDMOREPARAMS
            replyMsg = "Syntax error";
            break;
        case 462: // ERR_ALREADYREGISTERED
            replyMsg = "Connection already registered";
            break;
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

void Client::setPassword(const std::string& password) {
    _password = password;
}

void Client::setNickname(const std::string& nickname) {
    _nickname = nickname;
}

void Client::setUsername(const std::string& username) {
    _username = username;
}

void Client::setRealname(const std::string& realname) {
    _realname = realname;
}

void Client::setHasRegistered(bool val) {
    _hasRegistered = val;
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
