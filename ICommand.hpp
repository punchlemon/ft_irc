#pragma once
#include <iostream>
#include <string>
#include <vector>

class Server;
class Client;

class ICommand {
public:
    virtual ~ICommand() {}

    virtual void execute(Server& server, Client* client, const std::vector<std::string>& args) = 0;

protected:
    ICommand() {}

private:
    ICommand(const ICommand& other);
    ICommand& operator=(const ICommand& other);
};
