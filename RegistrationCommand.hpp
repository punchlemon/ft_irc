#pragma once
#include "ICommand.hpp"

class Server;
class Client;

// Handles IRC registration commands (PASS, NICK, USER)
class RegistrationCommand : public ICommand {
public:
    enum CommandType {
        PASS,
        NICK,
        USER
    };

    explicit RegistrationCommand(CommandType type);
    virtual ~RegistrationCommand();

    virtual void execute(Server& server, Client* client, const std::vector<std::string>& args);
    virtual std::string getName() const;

private:
    CommandType _type;

    void executePass(Server& server, Client* client, const std::vector<std::string>& args);
    void executeNick(Server& server, Client* client, const std::vector<std::string>& args);
    void executeUser(Server& server, Client* client, const std::vector<std::string>& args);

    RegistrationCommand();
    RegistrationCommand(const RegistrationCommand& other);
    RegistrationCommand& operator=(const RegistrationCommand& other);
};
