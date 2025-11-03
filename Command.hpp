#pragma once
#include "CommandHandler.hpp"
#include <iostream>

class Command {
public:
    static void pass(Server& server, Client* client, const std::vector<std::string>& args);
    // static void nick(Server& server, Client* client, const std::vector<std::string>& args);
    // static void user(Server& server, Client* client, const std::vector<std::string>& args);

private:
    Command();
    Command(const Command& other);
    Command& operator=(const Command& other);
    ~Command();
};
