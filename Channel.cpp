#include "Channel.hpp"
#include "Client.hpp"
#include <sstream>
#include <algorithm>
#include <cstring>

Channel::Channel(const std::string& name)
    : _name(name), _topic(""), _topicSetter(""), _key(""), _userLimit(0), _createdAt(time(NULL)) {
}

Channel::~Channel() {
    _members.clear();
    _operators.clear();
    _inviteList.clear();
}

std::string Channel::getName() const {
    return _name;
}

std::string Channel::getTopic() const {
    return _topic;
}

std::string Channel::getTopicSetter() const {
    return _topicSetter;
}

size_t Channel::getMemberCount() const {
    return _members.size();
}

void Channel::addClient(Client* client) {
    if (!client) return;

    int fd = client->getFd();
    _members[fd] = client;

    if (_members.size() == 1) {
        _operators.insert(fd);
    }
}

void Channel::removeClient(Client* client) {
    if (!client) return;
    removeClientByFd(client->getFd());
}

void Channel::removeClientByFd(int fd) {
    _members.erase(fd);
    _operators.erase(fd);
    _inviteList.erase(fd);
}

Client* Channel::findClientByNickname(const std::string& nickname) {
    for (std::map<int, Client*>::iterator it = _members.begin();
         it != _members.end(); ++it) {
        if (it->second && it->second->getNickname() == nickname) {
            return it->second;
        }
    }
    return NULL;
}

bool Channel::isMember(int clientFd) const {
    return _members.find(clientFd) != _members.end();
}

void Channel::broadcast(const std::string& message, int senderFd) {
    for (std::map<int, Client*>::iterator it = _members.begin();
         it != _members.end(); ++it) {
        if (it->first != senderFd && it->second) {
            it->second->queueMessage(message);
        }
    }
}

void Channel::broadcastToAll(const std::string& message) {
    for (std::map<int, Client*>::iterator it = _members.begin();
         it != _members.end(); ++it) {
        if (it->second) {
            it->second->queueMessage(message);
        }
    }
}

bool Channel::isOperator(int clientFd) const {
    return _operators.find(clientFd) != _operators.end();
}

void Channel::addOperator(int clientFd) {
    if (isMember(clientFd)) {
        _operators.insert(clientFd);
    }
}

void Channel::removeOperator(int clientFd) {
    _operators.erase(clientFd);
}

void Channel::setTopic(const std::string& newTopic, const std::string& setterNickname) {
    _topic = newTopic;
    _topicSetter = setterNickname;
}

bool Channel::canSetTopic(int clientFd) const {
    if (hasMode('t')) {
        return isOperator(clientFd);
    }
    return isMember(clientFd);
}

void Channel::inviteClient(int clientFd) {
    _inviteList.insert(clientFd);
}

bool Channel::isInvited(int clientFd) const {
    return _inviteList.find(clientFd) != _inviteList.end();
}

void Channel::removeInvite(int clientFd) {
    _inviteList.erase(clientFd);
}

Channel::JoinError Channel::canClientJoin(int clientFd, const std::string& key) const {
    if (hasMode('i') && !isInvited(clientFd)) {
        return ERR_INVITEONLYCHAN;
    }
    if (hasMode('k') && key != _key) {
        return ERR_BADCHANNELKEY;
    }
    if (hasMode('l') && _members.size() >= _userLimit) {
        return ERR_CHANNELISFULL;
    }
    return JOIN_SUCCESS;
}

bool Channel::hasMode(char mode) const {
    return _modes.find(mode) != _modes.end();
}

void Channel::addMode(char mode) {
    _modes.insert(mode);
}

void Channel::removeMode(char mode) {
    _modes.erase(mode);

    if (mode == 'k') {
        _key.clear();
    } else if (mode == 'l') {
        _userLimit = 0;
    } else if (mode == 'i') {
        _inviteList.clear();
    }
}

std::string Channel::getModeString() const {
    if (_modes.empty()) {
        return "+";
    }

    std::string result = "+";
    for (std::set<char>::const_iterator it = _modes.begin();
         it != _modes.end(); ++it) {
        result += *it;
    }
    return result;
}

void Channel::setKey(const std::string& key) {
    _key = key;
    if (!key.empty()) {
        addMode('k');
    } else {
        removeMode('k');
    }
}

std::string Channel::getKey() const {
    return _key;
}

bool Channel::hasKey() const {
    return hasMode('k') && !_key.empty();
}

void Channel::setUserLimit(size_t limit) {
    _userLimit = limit;
    if (limit > 0) {
        addMode('l');
    } else {
        removeMode('l');
    }
}

size_t Channel::getUserLimit() const {
    return _userLimit;
}

bool Channel::hasUserLimit() const {
    return hasMode('l') && _userLimit > 0;
}

std::vector<std::string> Channel::getMemberList() const {
    std::vector<std::string> memberList;

    for (std::map<int, Client*>::const_iterator it = _members.begin();
         it != _members.end(); ++it) {
        if (it->second) {
            std::string nickname = it->second->getNickname();
            if (isOperator(it->first)) {
                nickname = "@" + nickname;
            }
            memberList.push_back(nickname);
        }
    }

    return memberList;
}

std::string Channel::getMemberListString() const {
    std::vector<std::string> members = getMemberList();

    if (members.empty()) {
        return "";
    }

    std::ostringstream oss;
    for (size_t i = 0; i < members.size(); ++i) {
        if (i > 0) {
            oss << " ";
        }
        oss << members[i];
    }

    return oss.str();
}

const std::map<int, Client*>& Channel::getMembers() const {
    return _members;
}

const std::string Channel::getCreationTimeString() const {
    std::ostringstream oss;
    oss << static_cast<long>(_createdAt);
    return oss.str();
}
