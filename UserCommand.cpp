#include "UserCommand.hpp"
#include "Server.hpp"
#include "Client.hpp"

UserCommand::UserCommand() {}

UserCommand::~UserCommand() {}

bool UserCommand::requiresRegistration() const {
    return false;  // USER is used during registration
}

void UserCommand::execute(Server& server, Client* client, const std::vector<std::string>& args) {
    (void)server;  // unused for now

    if (client->hasRegistered()) {
        client->reply(462, "");
        return;
    }

    if (args.size() != 5) {
        client->reply(461, args[0]);
        return;
    }

    if (!client->getUsername().empty()) {
        client->reply(451, "");
        return;
    }

    client->setUsername(args[1]);
    client->setRealname(args[4]);
}
