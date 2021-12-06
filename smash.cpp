#include <iostream>
#include <unistd.h>
#include <sys/wait.h>
#include <signal.h>
#include "Commands.h"
#include <time.h>
#include "signals.h"

int main(int argc, char* argv[]) {
    if (signal(SIGTSTP, ctrlZHandler) == SIG_ERR) {
        perror("smash error: failed to set ctrl-Z handler");
    }
    if (signal(SIGINT, ctrlCHandler) == SIG_ERR) {
        perror("smash error: failed to set ctrl-C handler");
    }

    //TODO: setup sig alarm handler
    struct sigaction sig;
    sig.sa_handler = alarmHandler;
    sig.sa_flags = SA_RESTART;
    if (sigaction(SIGALRM, &sig, NULL) < 0) {
        perror("smash error: failed to set alarm handler");
    }
    SmallShell &smash = SmallShell::getInstance();
    while (true) {
        std::cout << SmallShell::getInstance().smashName << "> ";
        std::string cmd_line;
        if (!std::getline(std::cin, cmd_line))
            std::cin.clear();
        smash.executeCommand(cmd_line.c_str());
    }
    return 0;
}