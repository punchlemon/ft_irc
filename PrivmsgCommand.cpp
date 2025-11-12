#include "PrivmsgCommand.hpp"
#include "Server.hpp"
#include "Client.hpp"
#include "Channel.hpp"
#include "utils.hpp"
#include <sstream>
#include <set>

PrivmsgCommand::PrivmsgCommand() {}
PrivmsgCommand::~PrivmsgCommand() {}

bool PrivmsgCommand::requiresRegistration() const {
    return true;
}

void PrivmsgCommand::execute(Server& server, Client* client, const std::vector<std::string>& args) {
    if (args.size() == 1) {
        client->reply(411, " :No recipient given (" + args[0] + ")");
        return;
    }
    if (args.size() == 2) {
        client->reply(412, " :No text to send");
        return;
    }
    if (args.size() != 3) {
        client->reply(461, args[0]);
        return;
    }

    std::string receivers = args[1];
    std::string message = args[2];
    std::set<std::string> seenTargets;

    std::istringstream rss(receivers);
    std::string target;
    while (std::getline(rss, target, ',')) {
        if (target.empty()) continue;

        if (seenTargets.find(target) != seenTargets.end()) continue;
        seenTargets.insert(target);

        std::string out = ":" + client->getPrefix() + " PRIVMSG " + target + " :" + message + "\r\n";
        if (target[0] == '#' || target[0] == '&') {
            Channel* channel = server.getChannel(target);
            if (!isValidChannelName(target) || !channel) {
                client->reply(401, target + " :No such nick or channel name");
                continue;
            }
            channel->broadcast(out, client->getFd());
        }
        else {
            Client* dest = server.getClientByNickname(target);
            if (!dest) {
                client->reply(401, target + " :No such nick or channel name");
                continue;
            }
            dest->queueMessage(out);
        }
    }
}
