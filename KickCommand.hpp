#pragma once
#include "ICommand.hpp"

class Server;
class Client;

class KickCommand : public ICommand {
public:
    KickCommand();
    virtual ~KickCommand();

    virtual bool requiresRegistration() const;
    virtual void execute(Server& server, Client* client, const std::vector<std::string>& args);

private:
    KickCommand(const KickCommand& other);
    KickCommand& operator=(const KickCommand& other);
};
