#pragma once
#include "ICommand.hpp"

class Server;
class Client;

class PartCommand : public ICommand {
public:
    PartCommand();
    virtual ~PartCommand();

    virtual bool requiresRegistration() const;
    virtual void execute(Server& server, Client* client, const std::vector<std::string>& args);

private:
    PartCommand(const PartCommand& other);
    PartCommand& operator=(const PartCommand& other);
};
