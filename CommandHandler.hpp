#pragma once
#include <string>
#include <vector>

class Server;
class Client;

typedef void (*CommandHandler)(Server& server, Client* client, const std::vector<std::string>& args);
