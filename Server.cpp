#include "Server.hpp"
#include "Client.hpp"
#include "ICommand.hpp"
#include "PassCommand.hpp"
#include "NickCommand.hpp"
#include "UserCommand.hpp"

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

extern volatile sig_atomic_t g_shutdown_requested;

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
}

void Server::_initCommands() {
    _commands["PASS"] = new PassCommand();
    _commands["NICK"] = new NickCommand();
    _commands["USER"] = new UserCommand();
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

    std::cout << "Server started on port " << _port << std::endl;
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
            std::cerr << "accept error" << std::endl;
            break;
        }

        if (fcntl(new_socket, F_SETFL, O_NONBLOCK) < 0) {
            std::cerr << "[Socket " << new_socket << "] fcntl() error" << std::endl;
            close(new_socket);
            continue;
        }

        std::string hostname = inet_ntoa(client_addr.sin_addr);
        std::cout << "[Socket " << new_socket << "] New connection from " << hostname << std::endl;

        Client* new_client = new Client(new_socket, hostname, this, EPOLLIN);
        _clients[new_socket] = new_client;

        struct epoll_event ev;
        ev.events = EPOLLIN;
        ev.data.fd = new_socket;
        if (epoll_ctl(_epollFd, EPOLL_CTL_ADD, new_socket, &ev) < 0) {
            std::cerr << "epoll_ctl add client failed for fd " << new_socket << std::endl;
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

void Server::_handleClientRecv(int fd) {
    std::cout << "[Socket " << fd << "] Handling client data" << std::endl;
    if (!_clients.count(fd)) return;
    Client* client = _clients[fd];

    while (true) {
        char read_buf[BUFFER_SIZE];
        ssize_t bytes_read = recv(fd, read_buf, sizeof(read_buf), 0);

        if (bytes_read < 0) {
            if (errno == EAGAIN || errno == EWOULDBLOCK) {
                break;
            }
            std::cerr << "[Socket " << fd << "] recv error" << std::endl;
            _handleClientDisconnect(fd);
            return;
        }
        if (bytes_read == 0) {
            std::cout << "[Socket " << fd << "] Client disconnected (read 0)." << std::endl;
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
        std::cerr << "[Socket " << fd << "] send error" << std::endl;
        _handleClientDisconnect(fd);
        return;
    }
    client->clearSendBuffer(bytes_sent);

    if (client->getSendBuffer().empty()) {
        this->disableEpollOut(fd);
    }
}

void Server::_handleClientDisconnect(int fd) {
    if (_epollFd >= 0) {
        if (epoll_ctl(_epollFd, EPOLL_CTL_DEL, fd, NULL) < 0) {
            std::cerr << "epoll_ctl del failed for fd " << fd << std::endl;
        }
    }
    close(fd);
    if (_clients.count(fd)) {
        delete _clients[fd];
        _clients.erase(fd);
    }
}

void Server::_processCommand(int fd, const std::string& commandLine) {
    std::vector<std::string> args;
    std::istringstream iss(commandLine);
    std::string token;
    while (iss >> token) {
        args.push_back(token);
    }

    if (args.empty()) {
        std::cerr << "[Socket " << fd << "] Empty command received" << std::endl;
        return;
    }

    if (!_clients.count(fd)) {
        std::cerr << "[Socket " << fd << "] Client not found in _processCommand" << std::endl;
        return;
    }

    Client* client = _clients[fd];
    std::string cmdName = args[0];

    for (size_t i = 0; i < cmdName.length(); ++i) {
        cmdName[i] = std::toupper(cmdName[i]);
    }

    if (_commands.count(cmdName) == 0) {
        std::cout << "[Socket " << fd << "] Unknown command: " << args[0] << std::endl;
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
                std::cout << "[Socket " << fd << "] Client provided wrong password: " << client->getPrefix() << std::endl;
                _handleClientDisconnect(fd);
                return;
            }
            client->setHasRegistered(true);
            std::cout << "[Socket " << fd << "] Client registered: " << client->getPrefix() << std::endl;
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
            std::cout << "\nShutdown signal received, cleaning up..." << std::endl;
            shutdown();
            break;
        }

        int n = epoll_wait(_epollFd, _events.data(), _events.size(), EPOLL_TIMEOUT_MS);
        if (n < 0) {
            if (errno == EINTR) continue; // interrupted by signal, retry
            std::cerr << "epoll_wait error: " << std::strerror(errno) << std::endl;
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
    std::cout << "Starting graceful shutdown..." << std::endl;

    // 1. Stop accepting new connections - close server socket first
    if (_serverFd >= 0) {
        std::cout << "Closing server socket (fd=" << _serverFd << ")" << std::endl;
        close(_serverFd);
        _serverFd = -1;
    }

    // 2. Notify all clients about shutdown and close their connections
    std::cout << "Disconnecting " << _clients.size() << " client(s)..." << std::endl;
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
        std::cout << "Closing epoll instance (fd=" << _epollFd << ")" << std::endl;
        close(_epollFd);
        _epollFd = -1;
    }

    std::cout << "Graceful shutdown complete." << std::endl;
}
