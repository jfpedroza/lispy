#include <iostream>
#include <cstdlib>
#include <csignal>
#include "lispy.hpp"

using std::cout;
using std::endl;

void exit_handler(int);

int main(int argc, char* argv[]) {
    lispy lspy;

    struct sigaction sig_handler;
    sig_handler.sa_handler = exit_handler;
    sigemptyset(&sig_handler.sa_mask);
    sig_handler.sa_flags = 0;
    sigaction(SIGINT, &sig_handler, nullptr);

    return lspy.run(argc, argv);
}

void exit_handler(int) {
    linenoiseHistoryFree();
    cout << "\nBye" << endl;
    exit(0);
}

