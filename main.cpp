#include "Server.hpp"
#include "utils.hpp"
#include <iostream>
#include <cstdlib>
#include <signal.h>

// Global flag for graceful shutdown
volatile sig_atomic_t g_shutdown_requested = 0;

void signalHandler(int signum) {
    (void)signum;
    g_shutdown_requested = 1;
}

void setupSignalHandlers() {
    signal(SIGPIPE, SIG_IGN);
    signal(SIGINT, signalHandler);
    signal(SIGTERM, signalHandler);
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
