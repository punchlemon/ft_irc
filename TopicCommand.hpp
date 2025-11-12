#pragma once
#include "ICommand.hpp"

class Server;
class Client;

class TopicCommand : public ICommand {
public:
    TopicCommand();
    virtual ~TopicCommand();

    virtual bool requiresRegistration() const;
    virtual void execute(Server& server, Client* client, const std::vector<std::string>& args);

private:
    TopicCommand(const TopicCommand& other);
    TopicCommand& operator=(const TopicCommand& other);
};
