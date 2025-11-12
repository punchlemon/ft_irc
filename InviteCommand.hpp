#pragma once
#include "ICommand.hpp"

class Server;
class Client;

class InviteCommand : public ICommand {
public:
    InviteCommand();
    virtual ~InviteCommand();

    virtual bool requiresRegistration() const;
    virtual void execute(Server& server, Client* client, const std::vector<std::string>& args);

private:
    InviteCommand(const InviteCommand& other);
    InviteCommand& operator=(const InviteCommand& other);
};
