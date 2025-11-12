#include "JoinCommand.hpp"
#include "Server.hpp"
#include "Client.hpp"
#include "Channel.hpp"
#include "utils.hpp"
#include <sstream>

JoinCommand::JoinCommand() {}

JoinCommand::~JoinCommand() {}

bool JoinCommand::requiresRegistration() const {
    return true;
}

void JoinCommand::execute(Server& server, Client* client, const std::vector<std::string>& args) {
    if (args.size() != 2 && args.size() != 3) {
        client->reply(461, args[0]);
        return;
    }

    const std::string& channelList = args[1];
    std::string keyList = (args.size() == 3) ? args[2] : "";

    std::vector<std::string> channels;
    std::istringstream channelStream(channelList);
    std::string channelName;
    while (std::getline(channelStream, channelName, ',')) {
        if (!channelName.empty()) {
            channels.push_back(channelName);
        }
    }

    std::vector<std::string> keys;
    std::istringstream keyStream(keyList);
    std::string key;
    while (std::getline(keyStream, key, ',')) {
        if (!key.empty()) {
            keys.push_back(key);
        }
    }

    for (size_t i = 0; i < channels.size(); ++i) {
        std::string currentKey = (i < keys.size()) ? keys[i] : "";
        _joinSingleChannel(server, client, channels[i], currentKey);
    }
}

void JoinCommand::_joinSingleChannel(Server& server, Client* client, const std::string& channelName, const std::string& key) {
    if (!isValidChannelName(channelName)) {
        client->reply(403, channelName);
        return;
    }

    int clientFd = client->getFd();

    Channel* channel = server.getOrCreateChannel(channelName);

    if (channel->isMember(clientFd)) {
        return;
    }

    Channel::JoinError joinError = channel->canClientJoin(clientFd, key);
    if (joinError != Channel::JOIN_SUCCESS) {
        client->reply(joinError, channelName);
        return;
    }

    channel->addClient(client);

    if (channel->isInvited(clientFd)) {
        channel->removeInvite(clientFd);
    }

    std::string joinMsg = ":" + client->getPrefix() + " JOIN " + channelName + "\r\n";
    channel->broadcastToAll(joinMsg);

    if (!channel->getTopic().empty()) {
        client->reply(332, channelName + " :" + channel->getTopic());
        client->reply(333, channelName + " " + channel->getTopicSetter() + " " + channel->getCreationTimeString());
    }

    // Send NAMES list
    // RPL_NAMREPLY
    std::string memberList = channel->getMemberListString();
    client->reply(353, "= " + channelName + " :" + memberList);

    // RPL_ENDOFNAMES
    client->reply(366, channelName + " :End of /NAMES list");
}
