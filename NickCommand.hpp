#pragma once
#include "ICommand.hpp"

class Server;
class Client;

class NickCommand : public ICommand {
public:
    NickCommand();
    virtual ~NickCommand();

    virtual bool requiresRegistration() const;
    virtual void execute(Server& server, Client* client, const std::vector<std::string>& args);

private:
    NickCommand(const NickCommand& other);
    NickCommand& operator=(const NickCommand& other);
};
