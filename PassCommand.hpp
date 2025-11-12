#pragma once
#include "ICommand.hpp"

class Server;
class Client;

// Handles the PASS command for password authentication
class PassCommand : public ICommand {
public:
    PassCommand();
    virtual ~PassCommand();

    virtual bool requiresRegistration() const;
    virtual void execute(Server& server, Client* client, const std::vector<std::string>& args);

private:
    PassCommand(const PassCommand& other);
    PassCommand& operator=(const PassCommand& other);
};
