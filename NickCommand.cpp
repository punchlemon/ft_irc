#include "NickCommand.hpp"
#include "Server.hpp"
#include "Client.hpp"

NickCommand::NickCommand() {}

NickCommand::~NickCommand() {}

bool NickCommand::requiresRegistration() const {
    return false;
}

void NickCommand::execute(Server& server, Client* client, const std::vector<std::string>& args) {
    (void)server;

    if (args.size() != 2) {
        client->reply(461, args[0]);
        return;
    }

    Client* existing = server.getClientByNickname(args[1]);
    if (existing && existing != client) {
        client->reply(433, args[1] + " :Nickname already in use");
        return;
    }

    client->setNickname(args[1]);
}
