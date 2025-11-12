#pragma once
#include "ICommand.hpp"

class Server;
class Client;

class ModeCommand : public ICommand {
public:
    ModeCommand();
    virtual ~ModeCommand();

    virtual bool requiresRegistration() const;
    virtual void execute(Server& server, Client* client, const std::vector<std::string>& args);

private:
    ModeCommand(const ModeCommand& other);
    ModeCommand& operator=(const ModeCommand& other);
};
