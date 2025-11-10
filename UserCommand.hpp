#pragma once
#include "ICommand.hpp"

class Server;
class Client;

// Handles the USER command for setting username and realname
class UserCommand : public ICommand {
public:
    UserCommand();
    virtual ~UserCommand();

    virtual bool requiresRegistration() const;
    virtual void execute(Server& server, Client* client, const std::vector<std::string>& args);

private:
    UserCommand(const UserCommand& other);
    UserCommand& operator=(const UserCommand& other);
};
