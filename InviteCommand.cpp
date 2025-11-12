#include "InviteCommand.hpp"
#include "Server.hpp"
#include "Client.hpp"
#include "Channel.hpp"
#include "utils.hpp"
#include <sstream>

InviteCommand::InviteCommand() {}
InviteCommand::~InviteCommand() {}

bool InviteCommand::requiresRegistration() const {
    return true;
}

void InviteCommand::execute(Server& server, Client* client, const std::vector<std::string>& args) {
    if (args.size() != 3) {
        client->reply(461, args[0]);
        return;
    }

    const std::string& nick = args[1];
    const std::string& channelName = args[2];

    Channel* channel = server.getChannel(channelName);
    Client* targetClient = server.getClientByNickname(nick);
    if (!isValidChannelName(channelName) || !channel || !targetClient) {
        client->reply(401, nick + " :No such nick or channel name");
        return;
    }

    int inviterFd = client->getFd();
    if (!channel->isMember(inviterFd)) {
        client->reply(442, channelName + " :You are not on that channel");
        return;
    }

    if (!channel->isOperator(inviterFd)) {
        client->reply(482, channelName + " :You are not channel operator");
        return;
    }

    if (channel->isMember(targetClient->getFd())) {
        client->reply(443, targetClient->getNickname() + " " + channelName + " :is already on channel");
        return;
    }

    channel->inviteClient(targetClient->getFd());

    std::string inviteMsg = ":" + client->getPrefix() + " INVITE " + targetClient->getNickname() + " " + channelName + "\r\n";
    targetClient->queueMessage(inviteMsg);

    std::string inviteReply = ":" + client->getPrefix() + " 341 " + client->getNickname() + " " + targetClient->getNickname() + " " + channelName + "\r\n";
    client->queueMessage(inviteReply);
}
