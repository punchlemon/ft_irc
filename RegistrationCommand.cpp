#include "RegistrationCommand.hpp"
#include "Server.hpp"
#include "Client.hpp"
#include <iostream>

RegistrationCommand::RegistrationCommand(CommandType type) : _type(type) {}

RegistrationCommand::~RegistrationCommand() {}

void RegistrationCommand::execute(Server& server, Client* client, const std::vector<std::string>& args) {
    switch (_type) {
        case PASS:
            executePass(server, client, args);
            break;
        case NICK:
            executeNick(server, client, args);
            break;
        case USER:
            executeUser(server, client, args);
            break;
    }
}

std::string RegistrationCommand::getName() const {
    switch (_type) {
        case PASS: return "PASS";
        case NICK: return "NICK";
        case USER: return "USER";
        default: return "UNKNOWN";
    }
}

void RegistrationCommand::executePass(Server& server, Client* client, const std::vector<std::string>& args) {
    (void)server; // unused for now

    if (!client->getPassword().empty() || !client->getNickname().empty() || !client->getUsername().empty()) {
        client->reply(462, ""); // ERR_ALREADYREGISTERED
        return;
    }

    if (args.size() != 2) {
        client->reply(461, "PASS"); // ERR_NEEDMOREPARAMS
        return;
    }

    client->setPassword(args[1]);
}

void RegistrationCommand::executeNick(Server& server, Client* client, const std::vector<std::string>& args) {
    (void)server; // unused for now

    if (args.size() != 2) {
        client->reply(461, "NICK"); // ERR_NONICKNAMEGIVEN
        return;
    }

    client->setNickname(args[1]);
}

void RegistrationCommand::executeUser(Server& server, Client* client, const std::vector<std::string>& args) {
    (void)server; // unused for now

    if (client->hasRegistered()) {
        client->reply(462, ""); // ERR_ALREADYREGISTERED
        return;
    }

    if (args.size() != 5) {
        client->reply(461, "USER"); // ERR_NEEDMOREPARAMS
        return;
    }

    if (!client->getUsername().empty()) {
        client->reply(451, ""); // ERR_NOTREGISTERED
    }

    client->setUsername(args[1]);
    client->setRealname(args[4]);
}
