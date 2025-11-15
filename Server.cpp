#include "Server.hpp"
#include "Client.hpp"
#include "Channel.hpp"
#include "ICommand.hpp"
#include "PassCommand.hpp"
#include "NickCommand.hpp"
#include "UserCommand.hpp"
#include "JoinCommand.hpp"
#include "ModeCommand.hpp"
#include "InviteCommand.hpp"
#include "PartCommand.hpp"
#include "KickCommand.hpp"
#include "TopicCommand.hpp"
#include "PrivmsgCommand.hpp"

#include <iostream>
#include <cstring>
#include <cstdlib>
#include <cerrno>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <sys/epoll.h>
#include <sstream>
#include <signal.h>
#include <ctime>
#include <cstdio>

extern volatile sig_atomic_t g_shutdown_requested;

// Simple ngircd-like logger compatible with C++98
static void ngircd_log(const std::string& level, const std::string& msg) {
    time_t t = time(NULL);
    struct tm* tm_info = localtime(&t);
    char timebuf[64];
    if (tm_info) {
        strftime(timebuf, sizeof(timebuf), "%b %d %H:%M:%S", tm_info);
    } else {
        std::strncpy(timebuf, "0000-00-00 00:00:00", sizeof(timebuf));
        timebuf[sizeof(timebuf)-1] = '\0';
    }
    std::cout << "[" << timebuf << "] : " << level << ": " << msg << std::endl;
}

#define BACKLOG 10
#define BUFFER_SIZE 512

Server::Server(int port, const std::string& password):
    _serverName("ft_irc"),
    _port(port),
    _password(password),
    _serverFd(-1),
    _epollFd(-1),
    _startTimeString(_generateTimeString(time(NULL)))
{
    _initCommands();
}

Server::~Server() {
    _cleanupCommands();

    if (_serverFd >= 0) close(_serverFd);
    if (_epollFd >= 0) close(_epollFd);

    for (std::map<int, Client*>::iterator it = _clients.begin(); it != _clients.end(); ++it) {
        close(it->first);
        delete it->second;
    }

    for (std::map<std::string, Channel*>::iterator it = _channels.begin(); it != _channels.end(); ++it) {
        delete it->second;
    }
}

void Server::_initCommands() {
    _commands["PASS"] = new PassCommand();
    _commands["NICK"] = new NickCommand();
    _commands["USER"] = new UserCommand();
    _commands["JOIN"] = new JoinCommand();
    _commands["MODE"] = new ModeCommand();
    _commands["INVITE"] = new InviteCommand();
    _commands["PART"] = new PartCommand();
    _commands["KICK"] = new KickCommand();
    _commands["TOPIC"] = new TopicCommand();
    _commands["PRIVMSG"] = new PrivmsgCommand();
}

void Server::_cleanupCommands() {
    for (std::map<std::string, ICommand*>::iterator it = _commands.begin(); it != _commands.end(); ++it) {
        delete it->second;
    }
    _commands.clear();
}

void Server::_initServer() {
    _serverFd = socket(AF_INET, SOCK_STREAM, 0);
    if (_serverFd < 0) {
        throw std::runtime_error("Error: socket() failed");
    }

    int opt = 1;
    if (setsockopt(_serverFd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
        throw std::runtime_error("Error: setsockopt() failed");
    }

    if (fcntl(_serverFd, F_SETFL, O_NONBLOCK) < 0) {
        throw std::runtime_error("Error: fcntl() failed");
    }

    struct sockaddr_in address;
    std::memset(&address, 0, sizeof(address));
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(_port);

    if (bind(_serverFd, (struct sockaddr *)&address, sizeof(address)) < 0) {
        throw std::runtime_error("Error: bind() failed");
    }

    if (listen(_serverFd, BACKLOG) < 0) {
        throw std::runtime_error("Error: listen() failed");
    }

    _epollFd = epoll_create1(0);
    if (_epollFd < 0) {
        throw std::runtime_error("Error: epoll_create1() failed");
    }

    struct epoll_event ev;
    ev.events = EPOLLIN;
    ev.data.fd = _serverFd;
    if (epoll_ctl(_epollFd, EPOLL_CTL_ADD, _serverFd, &ev) < 0) {
        throw std::runtime_error("Error: epoll_ctl(ADD) failed");
    }

    _events.resize(1024);

    {
        std::ostringstream oss;
        oss << "Server started on port " << _port;
        ngircd_log("info", oss.str());
    }
}

const std::string Server::_generateTimeString(time_t startTime) const {
    struct tm* timeinfo = gmtime(&startTime);
    char buffer[80];
    memset(buffer, 0, sizeof(buffer));
    const char* format = "%a %b %d %Y at %T (UTC)";
    strftime(buffer, sizeof(buffer), format, timeinfo);

    return std::string(buffer);
}

void Server::_handleNewConnection() {
    struct sockaddr_in client_addr;
    socklen_t client_len = sizeof(client_addr);

    while (true) {
        int new_socket = accept(_serverFd, (struct sockaddr *)&client_addr, &client_len);

        if (new_socket < 0) {
            if (errno == EAGAIN || errno == EWOULDBLOCK) {
                break;
            }
            ngircd_log("error", "accept error");
            break;
        }

        if (fcntl(new_socket, F_SETFL, O_NONBLOCK) < 0) {
            {
                std::ostringstream oss;
                oss << "[Socket " << new_socket << "] fcntl() error";
                ngircd_log("error", oss.str());
            }
            close(new_socket);
            continue;
        }

        std::string hostname = inet_ntoa(client_addr.sin_addr);
        {
            std::ostringstream oss;
            oss << "New connection from " << hostname << " (fd=" << new_socket << ")";
            ngircd_log("info", oss.str());
        }

        Client* new_client = new Client(new_socket, hostname, this, EPOLLIN);
        _clients[new_socket] = new_client;

        struct epoll_event ev;
        ev.events = EPOLLIN;
        ev.data.fd = new_socket;
        if (epoll_ctl(_epollFd, EPOLL_CTL_ADD, new_socket, &ev) < 0) {
            {
                std::ostringstream oss;
                oss << "epoll_ctl add client failed for fd " << new_socket;
                ngircd_log("error", oss.str());
            }
            close(new_socket);
            delete new_client;
            _clients.erase(new_socket);
            continue;
        }
    }
}

std::string visualizeCRLF(const std::string& input) {
    std::ostringstream oss;
    for (size_t i = 0; i < input.length(); ++i) {
        char ch = input[i];
        if (ch == '\r') {
            oss << "\\r";
        } else if (ch == '\n') {
            oss << "\\n";
        } else {
            oss << ch;
        }
    }
    return oss.str();
}

// Helper: split a raw command line into whitespace-separated arguments
std::vector<std::string> Server::_splitArgs(const std::string& commandLine) {
    std::vector<std::string> args;
    size_t i = 0;
    const size_t n = commandLine.size();

    // skip leading whitespace
    while (i < n && isspace((unsigned char)commandLine[i])) ++i;

    while (i < n) {
        // If token starts with ':', everything after ':' is one trailing parameter
        if (commandLine[i] == ':') {
            std::string trailing;
            if (i + 1 < n) trailing = commandLine.substr(i + 1);
            else trailing = "";
            args.push_back(trailing);
            break;
        }

        // Read next token until whitespace
        size_t start = i;
        while (i < n && !isspace((unsigned char)commandLine[i])) ++i;
        args.push_back(commandLine.substr(start, i - start));

        // Skip spaces to next token
        while (i < n && isspace((unsigned char)commandLine[i])) ++i;
    }

    return args;
}

void Server::_handleClientRecv(int fd) {
    if (!_clients.count(fd)) return;
    Client* client = _clients[fd];

    while (true) {
        char read_buf[BUFFER_SIZE];
        ssize_t bytes_read = recv(fd, read_buf, sizeof(read_buf), 0);

        if (bytes_read < 0) {
            if (errno == EAGAIN || errno == EWOULDBLOCK) {
                break;
            }
            {
                std::ostringstream oss;
                oss << "[Socket " << fd << "] recv error";
                ngircd_log("error", oss.str());
            }
            _handleClientDisconnect(fd);
            return;
        }
        if (bytes_read == 0) {
            {
                std::ostringstream oss;
                oss << "Client disconnected (fd=" << fd << ")";
                ngircd_log("info", oss.str());
            }
            _handleClientDisconnect(fd);
            return;
        }
        client->appendRecvBuffer(read_buf, bytes_read);
    }
}

void Server::_handleClientSend(int fd) {
    if (!_clients.count(fd)) return;
    Client* client = _clients[fd];

    const std::string& send_buf = client->getSendBuffer();
    if (send_buf.empty()) {
        this->disableEpollOut(fd);
        return;
    }
    ssize_t bytes_sent = send(fd, send_buf.c_str(), send_buf.length(), 0);
    if (bytes_sent < 0) {
        if (errno == EAGAIN || errno == EWOULDBLOCK) {
            return;
        }
        {
            std::ostringstream oss;
            oss << "[Socket " << fd << "] send error";
            ngircd_log("error", oss.str());
        }
        _handleClientDisconnect(fd);
        return;
    }
    client->clearSendBuffer(bytes_sent);

    if (client->getSendBuffer().empty()) {
        this->disableEpollOut(fd);
    }
}

void Server::_handleClientDisconnect(int fd) {
    {
        std::ostringstream _entry;
        _entry << "_handleClientDisconnect called for fd=" << fd << " (clients=" << _clients.size() << ")";
        ngircd_log("debug", _entry.str());
    }

    if (!_clients.count(fd)) {
        std::ostringstream _oss;
        _oss << "_handleClientDisconnect: fd=" << fd << " not found in _clients map, ensuring close()";
        ngircd_log("debug", _oss.str());
        close(fd);
        return;
    }

    Client* client = _clients[fd];
    {
        std::ostringstream _info;
        _info << "Disconnecting client fd=" << fd << " nick='" << client->getNickname() << "' prefix='" << client->getPrefix() << "'";
        ngircd_log("info", _info.str());
    }

    // Show how many channels the client is in before removal
    {
        std::ostringstream _chcount;
        _chcount << "Client fd=" << fd << " is in " << client->getJoinedChannels().size() << " channel(s) before removal";
        ngircd_log("debug", _chcount.str());
    }

    removeClientFromAllChannels(client);

    {
        std::ostringstream _after;
        _after << "After removeClientFromAllChannels: client fd=" << fd << " joinedChannels=" << client->getJoinedChannels().size();
        ngircd_log("debug", _after.str());
    }

    if (_epollFd >= 0) {
        std::ostringstream _oss_try;
        _oss_try << "Attempting epoll_ctl(DEL) for fd=" << fd << " on epoll_fd=" << _epollFd;
        ngircd_log("debug", _oss_try.str());

        if (epoll_ctl(_epollFd, EPOLL_CTL_DEL, fd, NULL) < 0) {
            int err = errno;
            // Ignore benign errors: invalid fd or already removed
            if (err != EBADF && err != EINVAL) {
                std::ostringstream oss;
                oss << "epoll_ctl del failed for fd " << fd << ": " << std::strerror(err);
                ngircd_log("error", oss.str());
            } else {
                std::ostringstream oss_ignore;
                oss_ignore << "epoll_ctl del for fd=" << fd << " returned benign errno=" << err << " (" << std::strerror(err) << ") - ignoring";
                ngircd_log("debug", oss_ignore.str());
            }
        } else {
            std::ostringstream oss_ok;
            oss_ok << "epoll_ctl(DEL) succeeded for fd=" << fd;
            ngircd_log("debug", oss_ok.str());
        }
    }

    {
        std::ostringstream _closemsg;
        _closemsg << "Closing socket fd=" << fd;
        ngircd_log("debug", _closemsg.str());
    }
    close(fd);
    {
        std::ostringstream _closed;
        _closed << "Socket fd=" << fd << " closed";
        ngircd_log("debug", _closed.str());
    }

    {
        std::ostringstream _del;
        _del << "Deleting client object for fd=" << fd << " nick='" << client->getNickname() << "'";
        ngircd_log("debug", _del.str());
    }
    delete client;
    _clients.erase(fd);

    {
        std::ostringstream _remaining;
        _remaining << "Client removed. remaining clients=" << _clients.size();
        ngircd_log("info", _remaining.str());
    }
}

void Server::_processCommand(int fd, const std::string& commandLine) {
    // Log the raw command line received from client (make CR/LF visible)
    {
        std::ostringstream _logoss;
        _logoss << "Command from fd=" << fd << " : [" << visualizeCRLF(commandLine) << "]";
        ngircd_log("info", _logoss.str());
    }

    // Parse command token first (commands must come first and must not start with ':')
    size_t pos = 0;
    const size_t n = commandLine.size();
    // skip leading whitespace
    while (pos < n && isspace((unsigned char)commandLine[pos])) ++pos;

    size_t cmd_start = pos;
    while (pos < n && !isspace((unsigned char)commandLine[pos])) ++pos;

    if (cmd_start == pos) {
        {
            std::ostringstream _oss;
            _oss << fd;
            ngircd_log("debug", std::string("Empty command received from fd=") + _oss.str());
        }
        return;
    }

    std::string cmdToken = commandLine.substr(cmd_start, pos - cmd_start);
    if (!cmdToken.empty() && cmdToken[0] == ':') {
        // Command beginning with ':' is invalid in this server's parsing expectations
        {
            std::ostringstream oss;
            oss << "Malformed command (starts with ':') from fd=" << fd << ": " << cmdToken;
            ngircd_log("info", oss.str());
        }
        return;
    }

    // Build args: first element is the command token, remaining are parsed from the rest of the line
    std::vector<std::string> args;
    args.push_back(cmdToken);

    // remainder (may include leading spaces) and let _splitArgs handle trailing ':' semantics
    std::string remainder;
    if (pos < n) remainder = commandLine.substr(pos);
    if (!remainder.empty()) {
        std::vector<std::string> more = _splitArgs(remainder);
        for (size_t i = 0; i < more.size(); ++i) args.push_back(more[i]);
    }

    if (args.empty()) {
        std::ostringstream _oss2;
        _oss2 << fd;
        ngircd_log("debug", std::string("Empty command received from fd=") + _oss2.str());
        return;
    }

    if (!_clients.count(fd)) {
        std::ostringstream _oss3;
        _oss3 << fd;
        ngircd_log("warning", std::string("Client not found in _processCommand for fd=") + _oss3.str());
        return;
    }

    Client* client = _clients[fd];
    std::string cmdName = args[0];

    for (size_t i = 0; i < cmdName.length(); ++i) {
        cmdName[i] = std::toupper(cmdName[i]);
    }

    if (_commands.count(cmdName) == 0) {
        {
            std::ostringstream oss;
            oss << "Unknown command from fd=" << fd << ": " << args[0];
            ngircd_log("info", oss.str());
        }
        if (client->hasRegistered()) {
            client->reply(421, args[0]);
        }
        return;
    }

    ICommand* cmd = _commands[cmdName];

    if (!client->hasRegistered()) {
        if (cmd->requiresRegistration()) {
            client->reply(451, "");
            return;
        }
        cmd->execute(*this, client, args);
        ///// これはいつか関数化して綺麗にする
        if (!client->hasRegistered() && !client->getNickname().empty() && !client->getUsername().empty()) {
            if (client->getPassword() != this->getPassword()) {
                client->queueMessage("ERROR :Access denied: Bad password?\r\n"); //// これを送信してからdisconnectする
                {
                    std::ostringstream oss;
                    oss << "Client provided wrong password: " << client->getPrefix();
                    ngircd_log("warning", oss.str());
                }
                _handleClientDisconnect(fd);
                return;
            }
            client->setHasRegistered(true);
            {
                std::ostringstream oss;
                oss << "Client registered: " << client->getPrefix();
                ngircd_log("info", oss.str());
            }
            client->reply(001, "");
            client->reply(002, "");
            client->reply(003, "");
        }
        ////////////////////////////////
    }
    else {
        cmd->execute(*this, client, args);
    }
}

void Server::run() {
    _initServer();
    const int EPOLL_TIMEOUT_MS = 1000; // 1 second timeout to avoid indefinite blocking
    while (true) {
        // Check for shutdown signal
        if (g_shutdown_requested) {
            ngircd_log("info", "Shutdown signal received, cleaning up...");
            shutdown();
            break;
        }

        int n = epoll_wait(_epollFd, _events.data(), _events.size(), EPOLL_TIMEOUT_MS);
        if (n < 0) {
            if (errno == EINTR) continue; // interrupted by signal, retry
            {
                std::ostringstream oss;
                oss << "epoll_wait error: " << std::strerror(errno);
                ngircd_log("error", oss.str());
            }
            continue;
        }
        if (n == 0) {
            continue;
        }

        for (int i = 0; i < n; ++i) {
            int fd = _events[i].data.fd;
            uint32_t events = _events[i].events;
            if (fd == _serverFd && (events & EPOLLIN)) {
                _handleNewConnection();
                continue;
            }
            if (events & (EPOLLHUP | EPOLLERR)) {
                _handleClientDisconnect(fd);
                continue;
            }
            if (events & EPOLLIN) {
                _handleClientRecv(fd);
            }
            if (events & EPOLLOUT) {
                _handleClientSend(fd);
            }


            if (!_clients.count(fd)) continue;
            Client* client = _clients[fd];
            while (true) {
                const std::string& buf = client->getRecvBuffer();
                // std::cout << "[Socket " << fd << "] Processing command line : ["
                //         << visualizeCRLF(buf) << "]" << std::endl;
                size_t crlf_pos = buf.find("\r\n");
                if (crlf_pos == std::string::npos) break;
                std::string command_line = buf.substr(0, crlf_pos);
                client->clearRecvBuffer(crlf_pos + 2);
                if (!command_line.empty()) {
                    _processCommand(fd, command_line);
                }
            }


        }
    }
}

int Server::getPort() const {
    return _port;
}

const std::string& Server::getPassword() const {
    return _password;
}

void Server::enableEpollOut(int fd) {
    if (!_clients.count(fd)) return;
    Client* client = _clients[fd];

    uint32_t events = client->getEpollEvents();
    if (!(events & EPOLLOUT)) {
        uint32_t new_events = events | EPOLLOUT;
        struct epoll_event ev;
        ev.events = new_events;
        ev.data.fd = fd;
        if (epoll_ctl(_epollFd, EPOLL_CTL_MOD, fd, &ev) == 0) {
            client->setEpollEvents(new_events);
        }
    }
}

void Server::disableEpollOut(int fd) {
    if (!_clients.count(fd)) return;
    Client* client = _clients[fd];
    uint32_t events = client->getEpollEvents();
    if (events & EPOLLOUT) {
        uint32_t new_events = events & ~EPOLLOUT;
        struct epoll_event ev;
        ev.events = new_events;
        ev.data.fd = fd;
        if (epoll_ctl(_epollFd, EPOLL_CTL_MOD, fd, &ev) == 0) {
            client->setEpollEvents(new_events);
        }
    }
}

const std::string Server::getServerName() const {
    return _serverName;
}

const std::string Server::getStartTimeString() const {
    return _startTimeString;
}

void Server::shutdown() {
    ngircd_log("info", "Starting graceful shutdown...");

    // 1. Stop accepting new connections - close server socket first
    if (_serverFd >= 0) {
        {
            std::ostringstream oss;
            oss << "Closing server socket (fd=" << _serverFd << ")";
            ngircd_log("info", oss.str());
        }
        // actually close the listening socket
        close(_serverFd);
        _serverFd = -1;
    }

    // 2. Notify all clients about shutdown and close their connections
    {
        std::ostringstream oss;
        oss << "Disconnecting " << _clients.size() << " client(s)...";
        ngircd_log("info", oss.str());
    }

    for (std::map<int, Client*>::iterator it = _clients.begin(); it != _clients.end(); ++it) {
        int client_fd = it->first;
        Client* client = it->second;

        // Send shutdown message (optional but polite)
        const char* shutdown_msg = "ERROR :Server is shutting down\r\n";
        send(client_fd, shutdown_msg, strlen(shutdown_msg), 0);

        // Close client socket
        close(client_fd);

        // Free client memory
        delete client;
    }
    _clients.clear();

    // 3. Close epoll instance
    if (_epollFd >= 0) {
        {
            std::ostringstream oss;
            oss << "Closing epoll instance (fd=" << _epollFd << ")";
            ngircd_log("info", oss.str());
        }
        close(_epollFd);
        _epollFd = -1;
    }

    ngircd_log("info", "Graceful shutdown complete.");
}

Channel* Server::getChannel(const std::string& channelName) {
    std::map<std::string, Channel*>::iterator it = _channels.find(channelName);
    if (it != _channels.end()) {
        return it->second;
    }
    return NULL;
}

Channel* Server::getOrCreateChannel(const std::string& channelName) {
    std::map<std::string, Channel*>::iterator it = _channels.find(channelName);
    if (it != _channels.end()) {
        return it->second;
    }

    // Create new channel
    Channel* newChannel = new Channel(channelName);
    _channels[channelName] = newChannel;
    ngircd_log("info", std::string("Created new channel: ") + channelName);
    return newChannel;
}

void Server::removeChannel(const std::string& channelName) {
    std::map<std::string, Channel*>::iterator it = _channels.find(channelName);
    if (it == _channels.end()) {
        return;
    }
    Channel* channel = it->second;
    const std::map<int, Client*>& members = channel->getMembers();
    std::vector<Client*> membersCopy;
    for (std::map<int, Client*>::const_iterator mit = members.begin();
         mit != members.end(); ++mit) {
        if (mit->second) {
            membersCopy.push_back(mit->second);
        }
    }

    for (std::vector<Client*>::iterator cit = membersCopy.begin();
         cit != membersCopy.end(); ++cit) {
        Client* client = *cit;
        if (client) {
            client->removeChannel(channel);
        }
    }

    delete channel;
    _channels.erase(it);
    ngircd_log("info", std::string("Removed channel: ") + channelName);
}

Client* Server::getClientByFd(int fd) {
    std::map<int, Client*>::iterator it = _clients.find(fd);
    if (it != _clients.end()) {
        return it->second;
    }
    return NULL;
}

Client* Server::getClientByNickname(const std::string& nickname) {
    for (std::map<int, Client*>::iterator it = _clients.begin(); it != _clients.end(); ++it) {
        if (it->second && it->second->getNickname() == nickname) {
            return it->second;
        }
    }
    return NULL;
}

void Server::addClientToChannel(Client* client, Channel* channel) {
    if (!client || !channel) return;

    channel->addClient(client);
    client->addChannel(channel);

    ngircd_log("info", std::string("[") + client->getNickname() + "] joined channel " + channel->getName());
}

void Server::removeClientFromChannel(Client* client, Channel* channel) {
    if (!client || !channel) return;

    const std::string& channelName = channel->getName();

    channel->removeClient(client);
    client->removeChannel(channel);

    ngircd_log("info", std::string("[") + client->getNickname() + "] left channel " + channelName);

    if (channel->getMemberCount() == 0) {
        removeChannel(channelName);
    }
}

void Server::removeClientFromAllChannels(Client* client) {
    if (!client) return;

    const std::set<Channel*>& joinedChannels = client->getJoinedChannels();

    std::set<Channel*> channelsCopy = joinedChannels;

    for (std::set<Channel*>::iterator it = channelsCopy.begin();
         it != channelsCopy.end(); ++it) {
        Channel* channel = *it;
        if (channel) {
            // Broadcast QUIT message to channel members before removing
            std::string quitMsg = ":" + client->getPrefix() + " QUIT :Client disconnected\r\n";
            channel->broadcast(quitMsg, client->getFd());
            ngircd_log("info", std::string("Client quit on channel ") + channel->getName() + ": " + client->getPrefix());

            removeClientFromChannel(client, channel);
        }
    }
}
