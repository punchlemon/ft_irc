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

    client->setNickname(args[1]);
}
