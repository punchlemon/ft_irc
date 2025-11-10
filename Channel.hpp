#pragma once
#include <string>
#include <map>
#include <set>
#include <vector>

class Client;

class Channel {
public:
    explicit Channel(const std::string& name);
    ~Channel();

    std::string getName() const;
    std::string getTopic() const;
    std::string getTopicSetter() const;
    size_t getMemberCount() const;

    void addClient(Client* client);
    void removeClient(Client* client);
    void removeClientByFd(int fd);
    Client* findClientByNickname(const std::string& nickname);
    bool isMember(int clientFd) const;

    void broadcast(const std::string& message, int senderFd);
    void broadcastToAll(const std::string& message);

    bool isOperator(int clientFd) const;
    void addOperator(int clientFd);
    void removeOperator(int clientFd);

    void setTopic(const std::string& newTopic, const std::string& setterNickname);
    bool canSetTopic(int clientFd) const;

    void inviteClient(int clientFd);
    bool isInvited(int clientFd) const;
    void removeInvite(int clientFd);

    bool canClientJoin(int clientFd, const std::string& key) const;

    bool hasMode(char mode) const;
    void addMode(char mode);
    void removeMode(char mode);
    std::string getModeString() const;

    void setKey(const std::string& key);
    std::string getKey() const;
    bool hasKey() const;

    void setUserLimit(size_t limit);
    size_t getUserLimit() const;
    bool hasUserLimit() const;

    std::vector<std::string> getMemberList() const;
    std::string getMemberListString() const;

private:
    Channel();
    Channel(const Channel& other);
    Channel& operator=(const Channel& other);

    std::string _name;
    std::string _topic;
    std::string _topicSetter;
    std::string _key;
    size_t _userLimit;

    std::set<char> _modes;
    std::map<int, Client*> _members;
    std::set<int> _operators;
    std::set<int> _inviteList;
};
