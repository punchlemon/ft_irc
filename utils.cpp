#include "utils.hpp"
#include <cstdlib>
#include <cctype>
#include <string>
#include <cerrno>
#include <climits>

static int parse_and_validate_port(const std::string& portStr) {
    if (portStr.length() != 4) {
        throw std::invalid_argument("Port must be 4 digits");
    }
    for (size_t i = 0; i < portStr.length(); ++i) {
        unsigned char ch = static_cast<unsigned char>(portStr[i]);
        if (!std::isdigit(ch)) {
            throw std::invalid_argument("Port must contain only digits");
        }
    }

    errno = 0;
    char* endptr = NULL;
    long portLong = std::strtol(portStr.c_str(), &endptr, 10);
    if (endptr == portStr.c_str() || *endptr != '\0') {
        throw std::invalid_argument("Port conversion failed: not a valid number");
    }
    if ((portLong == LONG_MIN || portLong == LONG_MAX) && errno == ERANGE) {
        throw std::out_of_range("Port number out of range");
    }
    if (portLong < 6665 || portLong > 6669) {
        throw std::out_of_range("Port number must be in range 6665-6669");
    }
    return static_cast<int>(portLong);
}

static void validate_password(const std::string& password) {
    if (password.length() < 8 || password.length() > 64) {
        throw std::invalid_argument("Password must be 8 to 64 characters long");
    }
    for (size_t i = 0; i < password.length(); ++i) {
        unsigned char ch = static_cast<unsigned char>(password[i]);
        if (!std::isgraph(ch)) {
            throw std::invalid_argument("Password contains invalid character; only printable ASCII excluding whitespace allowed");
        }
    }
}

std::pair<int, std::string> validateInput(const int& argc, const char**& argv) {
    if (argc != 3) {
        throw std::invalid_argument("Usage: <program> <port> <password>");
    }

    std::string portStr = argv[1];
    int port = parse_and_validate_port(portStr);

    std::string password = argv[2];
    validate_password(password);

    return std::make_pair(port, password);
}
