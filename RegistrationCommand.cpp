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
    if (args.size() != 2) {
        client->reply(461, "PASS"); // ERR_NEEDMOREPARAMS
        std::cout << "[Socket " << client->getFd() << "] PASS: insufficient parameters" << std::endl;
        return;
    }

    if (client->isAuthenticated()) {
        client->reply(462, ""); // ERR_ALREADYREGISTERED
        std::cout << "[Socket " << client->getFd() << "] PASS: already authenticated" << std::endl;
        return;
    }

    const std::string& password = args[1];
    if (password == server.getPassword()) {
        client->setAuthenticated(true);
        std::cout << "[Socket " << client->getFd() << "] PASS: authentication successful" << std::endl;
        client->queueMessage("PASS accepted\r\n");
    } else {
        client->reply(464, ""); // ERR_PASSWDMISMATCH
        std::cout << "[Socket " << client->getFd() << "] PASS: incorrect password" << std::endl;
    }
}

void RegistrationCommand::executeNick(Server& server, Client* client, const std::vector<std::string>& args) {
    (void)server; // unused for now

    if (args.size() < 2) {
        client->reply(431, ""); // ERR_NONICKNAMEGIVEN
        std::cout << "[Socket " << client->getFd() << "] NICK: no nickname given" << std::endl;
        return;
    }

    const std::string& nickname = args[1];

    std::string oldNick = client->getNickname();
    client->setNickname(nickname);

    std::cout << "[Socket " << client->getFd() << "] NICK: changed from '"
              << oldNick << "' to '" << nickname << "'" << std::endl;

    if (!oldNick.empty()) {
        std::string msg = ":" + oldNick + " NICK :" + nickname + "\r\n";
        client->queueMessage(msg);
    }
}

void RegistrationCommand::executeUser(Server& server, Client* client, const std::vector<std::string>& args) {
    (void)server; // unused for now

    if (args.size() < 5) {
        client->reply(461, "USER"); // ERR_NEEDMOREPARAMS
        std::cout << "[Socket " << client->getFd() << "] USER: insufficient parameters" << std::endl;
        return;
    }

    if (!client->getUsername().empty()) {
        client->reply(462, ""); // ERR_ALREADYREGISTERED
        std::cout << "[Socket " << client->getFd() << "] USER: already registered" << std::endl;
        return;
    }

    const std::string& username = args[1];

    std::string realname = args[4];
    for (size_t i = 5; i < args.size(); ++i) {
        realname += " " + args[i];
    }

    client->setUsername(username);

    std::cout << "[Socket " << client->getFd() << "] USER: username='" << username
              << "' realname='" << realname << "'" << std::endl;
}
