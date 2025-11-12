#include "PassCommand.hpp"
#include "Server.hpp"
#include "Client.hpp"

PassCommand::PassCommand() {}

PassCommand::~PassCommand() {}

bool PassCommand::requiresRegistration() const {
    return false;
}

void PassCommand::execute(Server& server, Client* client, const std::vector<std::string>& args) {
    (void)server;

    if (!client->getPassword().empty() || !client->getNickname().empty() || !client->getUsername().empty()) {
        client->reply(462, " :Connection already registered");
        return;
    }

    if (args.size() != 2) {
        client->reply(461, args[0]);
        return;
    }

    client->setPassword(args[1]);
}
