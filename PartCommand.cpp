#include "PartCommand.hpp"
#include "Server.hpp"
#include "Client.hpp"
#include "Channel.hpp"
#include "utils.hpp"
#include <sstream>

PartCommand::PartCommand() {}
PartCommand::~PartCommand() {}

bool PartCommand::requiresRegistration() const {
    return true;
}

void PartCommand::execute(Server& server, Client* client, const std::vector<std::string>& args) {
    if (args.size() != 2 && args.size() != 3) {
        client->reply(461, args[0]);
        return;
    }

    std::string channelList = args[1];
    std::istringstream ss(channelList);
    std::string chName;
    while (std::getline(ss, chName, ',')) {
        if (chName.empty()) continue;

        Channel* channel = server.getChannel(chName);
        if (!isValidChannelName(chName) || !channel) {
            client->reply(403, chName + " :No such channel");
            continue;
        }

        int fd = client->getFd();
        if (!channel->isMember(fd)) {
            client->reply(442, chName + " :You're not on that channel");
            continue;
        }

        std::string partMsg = ":" + client->getPrefix() + " PART " + chName + " :";
        if (args.size() == 3) {
            partMsg += args[2];
        }
        partMsg += "\r\n";
        channel->broadcastToAll(partMsg);

        server.removeClientFromChannel(client, channel);
    }
}
