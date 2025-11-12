#include "TopicCommand.hpp"
#include "Server.hpp"
#include "Client.hpp"
#include "Channel.hpp"
#include "utils.hpp"
#include <sstream>

TopicCommand::TopicCommand() {}
TopicCommand::~TopicCommand() {}

bool TopicCommand::requiresRegistration() const {
    return true;
}

void TopicCommand::execute(Server& server, Client* client, const std::vector<std::string>& args) {
    if (args.size() != 2 && args.size() != 3) {
        client->reply(461, args[0]);
        return;
    }

    std::string channelName = args[1];
    Channel* channel = server.getChannel(channelName);
    if (!isValidChannelName(channelName) || !channel) {
        client->reply(403, channelName + " :No such channel");
        return;
    }

    int fd = client->getFd();
    if (!channel->isMember(fd)) {
        client->reply(442, channelName + " :You're not on that channel");
        return;
    }

    if (args.size() == 2) {
        if (channel->getTopic().empty()) {
            client->reply(331, channelName + " :No topic is set");
        } else {
            client->reply(332, channelName + " :" + channel->getTopic());
            client->reply(333, channelName + " " + channel->getTopicSetter() + " " + channel->getCreationTimeString());
        }
        return;
    }

    if (!channel->isMember(fd)) {
        client->reply(442, channelName + " :You're not on that channel");
        return;
    }
    if (channel->hasMode('t') && !channel->isOperator(fd)) {
        client->reply(482, channelName + " :You are not channel operator");
        return;
    }

    std::string newTopic = args[2];
    channel->setTopic(newTopic, client->getNickname());

    std::string topicMsg = ":" + client->getPrefix() + " TOPIC " + channelName + " :" + newTopic + "\r\n";
    channel->broadcastToAll(topicMsg);
}
