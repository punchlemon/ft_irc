#include "Command.hpp"
#include "Server.hpp"
#include "Client.hpp"

void Command::pass(Server& server, Client* client, const std::vector<std::string>& args) {
    if (args.size() != 2) {
        client->reply(461, "PASS"); // ERR_NEEDMOREPARAMS
        std::cout << "[Socket " << client->getFd() << "] PASS command received with incorrect number of parameters." << std::endl;
        return;
    }
    if (client->isAuthenticated()) {
        client->reply(462, ""); // ERR_ALREADYREGISTERED
        std::cout << "[Socket " << client->getFd() << "] PASS command received but client is already authenticated." << std::endl;
        return;
    }
    const std::string& password = args[1];
    if (password == server.getPassword()) {
        client->setAuthenticated(true);
        std::cout << "[Socket " << client->getFd() << "] Authentication successful." << std::endl;
        client->queueMessages("Authenticated!\r\n");
    } else {
        client->reply(464, ""); // ERR_PASSWDMISMATCH
        std::cout << "[Socket " << client->getFd() << "] Authentication failed: incorrect password." << std::endl;
    }
}
