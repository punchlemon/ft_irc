#include "Server.hpp"
#include "utils.hpp"
#include <iostream>
#include <cstdlib>
#include <signal.h>

void setupSignalHandlers() {
    signal(SIGPIPE, SIG_IGN);
}

int main(const int argc, const char **argv) {
    std::pair<int, std::string> inputParams;
    try {
        inputParams = validateInput(argc, argv);
    } catch (const std::exception& e) {
        std::cerr << "Input validation error: " << e.what() << std::endl;
        return EXIT_FAILURE;
    }

    setupSignalHandlers();

    try {
        Server irc_server(inputParams.first, inputParams.second);
        irc_server.run();
    } catch (const std::exception& e) {
        std::cerr << "Server runtime error: " << e.what() << std::endl;
        return EXIT_FAILURE;
    }

    return 0;
}
