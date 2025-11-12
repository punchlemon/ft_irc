#pragma once
#include "ICommand.hpp"

class Server;
class Client;

class JoinCommand : public ICommand {
public:
    JoinCommand();
    virtual ~JoinCommand();

    virtual bool requiresRegistration() const;
    virtual void execute(Server& server, Client* client, const std::vector<std::string>& args);

private:
    void _joinSingleChannel(Server& server, Client* client, const std::string& channelName, const std::string& key);
    bool _isValidChannelName(const std::string& name) const;

    JoinCommand(const JoinCommand& other);
    JoinCommand& operator=(const JoinCommand& other);
};
