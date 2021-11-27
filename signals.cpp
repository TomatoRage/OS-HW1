#include <iostream>
#include <signal.h>
#include "signals.h"
#include "Commands.h"

using namespace std;

void ctrlZHandler(int sig_num) {
    //SmallShell::getInstance().currCommand->isFinished = true;
    // TODO: Add your implementation
    cout << "smash: got ctrl-Z" << endl;
    if (SmallShell::getInstance().currCommand == NULL) return;
    int proID = SmallShell::getInstance().currCommand->processPID;
    if (SmallShell::getInstance().currCommand->Type == PIPE ||
        SmallShell::getInstance().currCommand->Type == REDIRECTION ||
        SmallShell::getInstance().currCommand->Type == PIPE) {//time o
        if (killpg(proID, SIGSTOP) == -1) {
            perror("smash error: kill failed");
            return;
        }
    } else if (kill(proID, SIGSTOP) == -1) {
        perror("smash error: kill failed");
        return;
    }
    cout << "smash: process " << proID << " was stopped" << endl;

    if () {


    } else {

    }


}

void ctrlCHandler(int sig_num) {
    // TODO: Add your implementation
}

void alarmHandler(int sig_num) {
    // TODO: Add your implementation
}

