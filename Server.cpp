#include "Server.hpp"
#include "Client.hpp"

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

#define BACKLOG 10
#define BUFFER_SIZE 512

Server::Server() : _port(0), _password(""), _serverFd(-1), _epollFd(-1) {}

Server::Server(int port, const std::string& password):
    _port(port),
    _password(password),
    _serverFd(-1),
    _epollFd(-1)
{}

Server::~Server() {
    if (_serverFd >= 0) close(_serverFd);
    if (_epollFd >= 0) close(_epollFd);

    for (std::map<int, Client*>::iterator it = _clients.begin(); it != _clients.end(); ++it) {
        close(it->first);
        delete it->second;
    }
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

        Client* new_client = new Client(new_socket, hostname);
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

void Server::_handleClientData(int fd) {
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
    std::cout << "[Socket " << fd << "] CMD: " << commandLine << std::endl;
}

void Server::run() {
    _initServer();
    while (true) {
        int n = epoll_wait(_epollFd, _events.data(), _events.size(), -1);
        if (n < 0) {
            if (errno == EINTR) continue;
            std::cerr << "epoll_wait error" << std::endl;
            break;
        }

        for (int i = 0; i < n; ++i) {
            int fd = _events[i].data.fd;
            if (_events[i].events & EPOLLIN) {
                if (fd == _serverFd) {
                    _handleNewConnection();
                } else {
                    _handleClientData(fd);
                }
            }
        }
    }
}
