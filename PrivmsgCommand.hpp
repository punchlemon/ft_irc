#pragma once
#include "ICommand.hpp"

class Server;
class Client;

class PrivmsgCommand : public ICommand {
public:
    PrivmsgCommand();
    virtual ~PrivmsgCommand();

    virtual bool requiresRegistration() const;
    virtual void execute(Server& server, Client* client, const std::vector<std::string>& args);

private:
    PrivmsgCommand(const PrivmsgCommand& other);
    PrivmsgCommand& operator=(const PrivmsgCommand& other);
};
