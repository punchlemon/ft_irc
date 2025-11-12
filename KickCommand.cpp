#include "KickCommand.hpp"
#include "Server.hpp"
#include "Client.hpp"
#include "Channel.hpp"
#include "utils.hpp"
#include <sstream>

KickCommand::KickCommand() {}
KickCommand::~KickCommand() {}

bool KickCommand::requiresRegistration() const {
    return true;
}

void KickCommand::execute(Server& server, Client* client, const std::vector<std::string>& args) {
    if (args.size() != 3 && args.size() != 4) {
        client->reply(461, args[0]);
        return;
    }

    std::string channelName = args[1];
    std::string targetList = args[2];
    std::string comment = (args.size() == 4) ? args[3] : client->getNickname();

    Channel* channel = server.getChannel(channelName);
    if (!isValidChannelName(channelName) || !channel) {
        client->reply(403, channelName + " :No such channel");
        return;
    }

    std::vector<std::string> targets;
    std::istringstream tss(targetList);
    std::string tnick;
    while (std::getline(tss, tnick, ',')) {
        if (!tnick.empty()) targets.push_back(tnick);
    }

    int issuerFd = client->getFd();
    if (!channel->isMember(issuerFd)) {
        client->reply(442, channelName + " :You are not on that channel");
        return;
    }

    if (!channel->isOperator(issuerFd)) {
        client->reply(482, channelName + " :You are not channel operator");
        return;
    }

    for (size_t i = 0; i < targets.size(); ++i) {
        const std::string& targetNick = targets[i];
        Client* targetClient = server.getClientByNickname(targetNick);
        if (!targetClient) {
            client->reply(401, targetNick + " :No such nick or channel name");
            continue;
        }

        if (!channel->isMember(targetClient->getFd())) {
            client->reply(441, targetNick + " " + channelName + " :They aren't on that channel");
            continue;
        }

        std::string kickMsg = ":" + client->getPrefix() + " KICK " + channelName + " " + targetNick + " :" + comment;
        kickMsg += "\r\n";
        channel->broadcastToAll(kickMsg);

        server.removeClientFromChannel(targetClient, channel);
    }
}
