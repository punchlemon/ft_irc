#include "ModeCommand.hpp"
#include "Server.hpp"
#include "Client.hpp"
#include "Channel.hpp"
#include "utils.hpp"
#include <sstream>
#include <iostream>
#include <string>
#include <cstdlib>
#include <cerrno>

ModeCommand::ModeCommand() {}

ModeCommand::~ModeCommand() {}

bool ModeCommand::requiresRegistration() const {
    return true;
}

static void parseModeChanges(Client* client, const std::string& target, const std::string& modestr, std::vector<std::pair<char,bool> >& outModes) {
    bool add = true;
    for (size_t i = 0; i < modestr.size(); ++i) {
        char c = modestr[i];
        if (c == '+') { add = true; continue; }
        if (c == '-') { add = false; continue; }
        if (c == 'i' || c == 't' || c == 'k' || c == 'o' || c == 'l') {
            outModes.push_back(std::make_pair(c, add));
        } else {
            client->reply(472, std::string(1, c) + " :is unknown mode char for " + target);
        }
    }
}

void ModeCommand::execute(Server& server, Client* client, const std::vector<std::string>& args) {
    (void)server;
    if (args.size() < 2) {
        client->reply(461, args[0]);
        return;
    }

    const std::string target = args[1];
    Channel* channel = server.getChannel(target);
    if (isValidChannelName(target) && channel) {

        if (args.size() < 3) {
            std::string modes = channel->getModeString();
            client->reply(324, target + " " + modes);
            client->reply(329, target + " " + channel->getCreationTimeString());
            return;
        }

        int clientFd = client->getFd();
        if (!channel->isMember(clientFd)) {
            client->reply(442, target + " :You are not on that channel");
            return;
        }
        if (!channel->isOperator(clientFd)) {
            client->reply(482, target + " :You are not channel operator");
            return;
        }

        std::string modestr = args[2];
        std::vector<std::pair<char,bool> > modeChanges;
        parseModeChanges(client, target, modestr, modeChanges);

        size_t argIndex = 3;
        std::string appliedModes = "";
        std::vector<std::string> appliedArgs;
        char currentSign = 0;

        for (size_t i = 0; i < modeChanges.size(); ++i) {
            char mode = modeChanges[i].first;
            bool add = modeChanges[i].second;

            std::string param;
            bool hasParam = false;
            if ((add && (mode == 'k' || mode == 'l')) || mode == 'o') {
                if (argIndex >= args.size()) {
                    continue;
                }
                param = args[argIndex++];
                hasParam = true;
            }

            bool applied = false;
            if (mode == 'k') {
                if (add) {
                    channel->setKey(param);
                    applied = true;
                } else {
                    if (channel->hasKey()) {
                        channel->setKey("");
                        applied = true;
                    }
                }
            }
            else if (mode == 'l') {
                if (add) {
                    size_t limit = 0;
                    char* endptr = NULL;
                    errno = 0;
                    unsigned long val = std::strtoul(param.c_str(), &endptr, 10);
                    if (endptr == param.c_str() || *endptr != '\0' || errno == ERANGE) {
                        continue;
                    }
                    limit = static_cast<size_t>(val);
                    channel->setUserLimit(limit);
                    applied = true;
                } else {
                    if (channel->hasUserLimit()) {
                        channel->setUserLimit(0);
                        applied = true;
                    }
                }
            }
            else if (mode == 'i') {
                if (add) {
                    if (!channel->hasMode('i')) { channel->addMode('i'); applied = true; }
                } else {
                    if (channel->hasMode('i')) { channel->removeMode('i'); applied = true; }
                }
            }
            else if (mode == 't') {
                if (add) {
                    if (!channel->hasMode('t')) { channel->addMode('t'); applied = true; }
                } else {
                    if (channel->hasMode('t')) { channel->removeMode('t'); applied = true; }
                }
            }
            else if (mode == 'o') {
                Client* targetClient = channel->findClientByNickname(param);
                if (!targetClient) {
                    client->reply(401, param + " :No such nick or channel name");
                } else {
                    if (add) {
                        if (!channel->isOperator(targetClient->getFd())) { channel->addOperator(targetClient->getFd()); applied = true; }
                    } else {
                        if (channel->isOperator(targetClient->getFd())) { channel->removeOperator(targetClient->getFd()); applied = true; }
                    }
                }
            }

            if (applied) {
                char sign = add ? '+' : '-';
                if (currentSign == 0) {
                    currentSign = sign;
                    appliedModes.push_back(currentSign);
                } else if (currentSign != sign) {
                    currentSign = sign;
                    appliedModes.push_back(currentSign);
                }
                appliedModes.push_back(mode);
                if (hasParam) appliedArgs.push_back(param);
            }
        }

        if (appliedModes.empty()) {
            return;
        }

        std::ostringstream oss;
        oss << ":" << client->getPrefix() << " MODE " << target << " " << appliedModes;
        for (size_t i = 0; i < appliedArgs.size(); ++i) {
            oss << " " << appliedArgs[i];
        }
        oss << "\r\n";
        channel->broadcastToAll(oss.str());
    }
    else {
        client->reply(401, target + " :No such nick or channel name");
    }
}
