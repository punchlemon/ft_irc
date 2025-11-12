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
    return _nickname + "!" + (_username.empty() ? "*" : "~" +  _username) + "@" + _hostname;
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
    std::string nickname = _nickname.empty() ? "*" : _nickname;
    std::string msg;
    if (!message.empty()) {
        msg = " " + message;
    }
    std::string replyMsg = ":" + _server->getServerName() + " " + replyCodeStr + " " + nickname + msg;
    switch (replyCode) {
        case 001: // RPL_WELCOME
            replyMsg += " :Welcome to the Internet Relay Server " + getPrefix();
            break;
        case 002:
            replyMsg += " :Your host is " + _server->getServerName();
            break;
        case 003:
            replyMsg += " :This server has been started " + _server->getStartTimeString();
            break;
        case 421: // ERR_UNKNOWNCOMMAND
            replyMsg += " :Unknown command";
            break;
        case 451: // ERR_NOTREGISTERED
            replyMsg += " :Connection not registered";
            break;
        case 461: // ERR_NEEDMOREPARAMS
            replyMsg += " :Syntax error";
            break;
        case 462: // ERR_ALREADYREGISTERED
            replyMsg += " :Connection already registered";
            break;
        case 471: // ERR_CHANNELISFULL
            replyMsg += " :Cannot join channel (+l) -- Channel is full, try later";
            break;
        case 473: // ERR_INVITEONLYCHAN
            replyMsg += " :Cannot join channel (+i) -- Invited users only";
            break;
        case 475: // ERR_BADCHANNELKEY
            replyMsg += " :Cannot join channel (+k) -- Wrong channel key";
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

void Client::addChannel(Channel* channel) {
    if (channel) {
        _joinedChannels.insert(channel);
    }
}

void Client::removeChannel(Channel* channel) {
    if (channel) {
        _joinedChannels.erase(channel);
    }
}

const std::set<Channel*>& Client::getJoinedChannels() const {
    return _joinedChannels;
}

bool Client::isInChannel(Channel* channel) const {
    return channel && _joinedChannels.find(channel) != _joinedChannels.end();
}
