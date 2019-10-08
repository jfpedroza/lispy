#include <csignal>
#include <cstdlib>
#include <iostream>
#include "Lispy.hpp"

using std::cout;
using std::endl;

void exit_handler(int);

int main(int argc, char *argv[]) {
    Lispy lispy;

    struct sigaction sig_handler;
    sig_handler.sa_handler = exit_handler;
    sigemptyset(&sig_handler.sa_mask);
    sig_handler.sa_flags = 0;
    sigaction(SIGINT, &sig_handler, nullptr);

    return lispy.run(argc, argv);
}

void exit_handler(int) {
    linenoiseHistoryFree();
    cout << "\nBye" << endl;
    exit(0);
}
