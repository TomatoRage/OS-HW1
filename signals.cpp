#include <iostream>
#include <signal.h>
#include "signals.h"
#include "Commands.h"

using namespace std;

void ctrlZHandler(int sig_num) {

    cout << "smash: got ctrl-Z" << endl;
    if (SmallShell::getInstance().currCommand == NULL) return;
    if (SmallShell::getInstance().currCommand->Type == FGEXTERNAL){
        pid_t proID = SmallShell::getInstance().currCommand->processPID;
        if (kill(proID, SIGSTOP) == -1) {
            perror("smash error: kill failed");
            return;
        }
        cout << "smash: process " << proID << " was stopped" << endl;
        SmallShell::getInstance().jobList->addJob(SmallShell::getInstance().currCommand, true);
    }

    SmallShell::getInstance().currCommand = NULL;
}

void ctrlCHandler(int sig_num) {
    cout << "smash: got ctrl-C" << endl;
    if (SmallShell::getInstance().currCommand == NULL) return;
    if (SmallShell::getInstance().currCommand->Type == FGEXTERNAL){
        int proID = SmallShell::getInstance().currCommand->processPID;
        if (kill(proID, SIGKILL) == -1) {
            perror("smash error: kill failed");
            return;
        }
        cout << "smash: process " << proID << " was killed" << endl;
    }

    SmallShell::getInstance().currCommand = NULL;
}

void alarmHandler(int sig_num) {
    // TODO: Add your implementation
}

