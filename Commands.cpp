#include <unistd.h>
#include <string.h>
#include <iostream>
#include <vector>
#include <signal.h>
#include <sstream>
#include <sys/wait.h>
#include <time.h>
#include <fcntl.h>
#include <algorithm>
#include "Commands.h"

using namespace std;

#if 0
#define FUNC_ENTRY()  \
  cout << __PRETTY_FUNCTION__ << " --> " << endl;

#define FUNC_EXIT()  \
  cout << __PRETTY_FUNCTION__ << " <-- " << endl;
#else
#define FUNC_ENTRY()
#define FUNC_EXIT()
#endif

string _ltrim(const std::string& s)
{
  size_t start = s.find_first_not_of(WHITESPACE);
  return (start == std::string::npos) ? "" : s.substr(start);
}

string _rtrim(const std::string& s)
{
  size_t end = s.find_last_not_of(WHITESPACE);
  return (end == std::string::npos) ? "" : s.substr(0, end + 1);
}

string _trim(const std::string& s)
{
  return _rtrim(_ltrim(s));
}

int _parseCommandLine(const char* cmd_line, char** args) {
  FUNC_ENTRY()
  int i = 0;
  std::istringstream iss(_trim(string(cmd_line)).c_str());
  for(std::string s; iss >> s; ) {
    args[i] = (char*)malloc(s.length()+1);
    memset(args[i], 0, s.length()+1);
    strcpy(args[i], s.c_str());
    args[++i] = NULL;
  }
  return i;

  FUNC_EXIT()
}

bool _isBackgroundComamnd(const char* cmd_line) {
  const string str(cmd_line);
  return str[str.find_last_not_of(WHITESPACE)] == '&';
}

void _removeBackgroundSign(char* cmd_line) {
  const string str(cmd_line);
  // find last character other than spaces
  unsigned int idx = str.find_last_not_of(WHITESPACE);
  // if all characters are spaces then return
  if (idx == string::npos) {
    return;
  }
  // if the command line does not end with & then return
  if (cmd_line[idx] != '&') {
    return;
  }
  // replace the & (background sign) with space and then remove all tailing spaces.
  cmd_line[idx] = ' ';
  // truncate the command line string up to the last non-space character
  cmd_line[str.find_last_not_of(WHITESPACE, idx) + 1] = 0;
}

Command::Command(const char *cmd_line,CommandType type) {
    string cmd = _trim(string(cmd_line));
    Arguments = *(new vector<string>);
    cmdSyntax = cmd;
    this->Type = type;
    FillInArguments(cmd);
    this->processPID = getpid();
}

Command::~Command() noexcept {
    //TODO:Delete Vector
}

Command::Command(Command &C) {
    cmdSyntax = C.cmdSyntax;
    Type = C.Type;
    processPID = C.processPID;
    Arguments = *(new vector<string>);
    FillInArguments(cmdSyntax);
}

void Command::FillInArguments(const string& cmdline) {
    char** Args = new char*[20];
    int size = _parseCommandLine(cmdline.c_str(),Args);
    for(int i = 0; i < size; i++){
        Arguments.insert(std::next(Arguments.begin(),i),Args[i]);
    }
    delete Args;
}

BuiltInCommand::BuiltInCommand(const char *cmd_line): Command(cmd_line,BUILTIN) { }

ExternalCommand::ExternalCommand(const char *cmd_line): Command(cmd_line,FGEXTERNAL){

    if(_isBackgroundComamnd(cmd_line)){
        Type = BGEXTERNAL;
    }

}

ExternalCommand::ExternalCommand(ExternalCommand &E): Command(E) {
    Type = E.Type;
}

void ExternalCommand::execute() {
    pid_t son = fork();
    if(son == -1) {
        perror("smash error: fork failed");
        return;
    }
    if(Type == FGEXTERNAL){
        if(son == 0){
            setpgrp();
            char *Args[] = {"bin/bash","-c", const_cast<char *>(cmdSyntax.c_str()), NULL};
            if(execv("/bin/bash",Args) == -1) {
                perror("smash error: exec failed");
                return;
            }
        }else{
            processPID = son;
            waitpid(son, nullptr,WUNTRACED);
        }
    }else{
        if(son == 0){
            setpgrp();
            char *Args[] = {"bin/bash","-c",const_cast<char *>(cmdSyntax.substr(0, cmdSyntax.size() - 1).c_str()), NULL};
            if(execv("/bin/bash",Args) == -1) {
                perror("smash error: exec failed");
                return;
            }
        }else
            processPID = son;
    }
}

PipeCommand::PipeCommand(const char *cmd_line): Command(cmd_line,PIPE) { }

void PipeCommand::execute() {
    int idx = cmdSyntax.find("|&");
    string inp;
    string outp;
    int myPipe[2];
    int Stream;

    if(idx == string::npos){
        inp = cmdSyntax.substr(cmdSyntax.find("|")+1);
        outp = cmdSyntax.substr(0,cmdSyntax.find("|"));
        Stream = 1;
    }else{
        string inp = cmdSyntax.substr(cmdSyntax.find("|")+2);
        string outp = cmdSyntax.substr(0,cmdSyntax.find("|"));
        Stream = 2;
    }
    pid_t PPid = processPID;
    pid_t son = fork();
    if(son == -1){
        perror("smash error: fork failed");
        return;
    }
    if(son == 0) {
        setpgrp();
        processPID = getppid();
        if (pipe(myPipe) == -1) {
            perror("smash error: pipe failed");
            exit(1);
        }

        pid_t Grandson = fork();
        if (Grandson == -1) {
            perror("smash error: fork failed");
            exit(1);
        }
        if (Grandson == 0) {
            if (close(myPipe[1]) == -1) {
                perror("smash error: close failed");
                exit(1);
            }
            if (dup2(myPipe[0], 0) == -1) {
                perror("smash error: dup2 failed");
                exit(1);
            }
            if (close(myPipe[0]) == -1) {
                perror("smash error: close failed");
                exit(1);
            }
            Command *CMD = SmallShell::getInstance().CreateCommand(inp.c_str());
            CMD->processPID = PPid;
            CMD->execute();
            exit(0);
        }
        pid_t Grandson2 = fork();
        if (Grandson2 == -1) {
            perror("smash error: fork failed");
            exit(1);
        }
        if (Grandson2 == 0) {
            if (close(myPipe[0]) == -1) {
                perror("smash error: close failed");
                exit(1);
            }
            if (dup2(myPipe[1], Stream) == -1) {
                perror("smash error: dup2 failed");
                exit(1);
            }
            if (close(myPipe[1]) == -1) {
                perror("smash error: close failed");
                exit(1);
            }
            Command *CMD = SmallShell::getInstance().CreateCommand(outp.c_str());
            CMD->processPID = PPid;
            CMD->execute();
            exit(0);
        }
        if (close(myPipe[0] == -1)) {
            perror("smash error: close failed");
            return;
        }
        if (close(myPipe[1]) == -1) {
            perror("smash error: close failed");
            return;
        }
        waitpid(Grandson, nullptr,WUNTRACED);
        waitpid(Grandson2, nullptr,WUNTRACED);
        exit(0);
    }
    waitpid(son, nullptr,WUNTRACED);
}

RedirectionCommand::RedirectionCommand(const char *cmd_line): Command(cmd_line,REDIRECTION) {
    if(_isBackgroundComamnd(cmd_line))
        cmdSyntax = cmdSyntax.substr(0,cmdSyntax.size()-1);
}

void RedirectionCommand::execute() {
    int idx = cmdSyntax.find(">>");
    if(idx == string::npos){
        string File = cmdSyntax.substr(cmdSyntax.find(">")+1);
        string cmd = cmdSyntax.substr(0, cmdSyntax.find(">"));
        int FD = open(_trim(File).c_str(),O_TRUNC | O_CREAT | O_WRONLY,0666);
        if(FD == -1){
            perror("smash error: open failed");
            return;
        }
        int oldFD = dup(1);
        if(dup2(FD,1) == -1){
            perror("smash error: dup2 failed");
            return;
        }
        Command* COMMAND = SmallShell::getInstance().CreateCommand(cmd.c_str());
        COMMAND->execute();

        if(close(FD) == -1){
            perror("smash error: close failed");
            return;
        }
        if(dup2(oldFD,1) == -1){
            perror("smash error: dup2 failed");
            return;
        }
        if(close(oldFD) == -1){
            perror("smash error: close failed");
            return;
        }
    }else{
        string File = cmdSyntax.substr(idx+2);
        string cmd = cmdSyntax.substr(0,idx);
        int FD = open(_trim(File).c_str(),O_APPEND | O_CREAT | O_WRONLY,0666);
        if(FD == -1){
            perror("smash error open failed");
            return;
        }
        int oldFD = dup(1);
        if(dup2(FD,1) == -1){
            perror("smash error: dup2 failed");
            return;
        }

        Command* COMMAND = SmallShell::getInstance().CreateCommand(cmd.c_str());
        COMMAND->execute();

        if(close(FD) == -1){
            perror("smash error: close failed");
            return;
        }
        if(dup2(oldFD,1) == -1){
            perror("smash error: dup2 failed");
            return;
        }
        if(close(oldFD) == -1){
            perror("smash error: close failed");
            return;
        }
    }
}

ChangeDirCommand::ChangeDirCommand(const char *cmd_line, string* plastPwd): BuiltInCommand(cmd_line),lastdir(*plastPwd) {}

void ChangeDirCommand::execute() {
    int result;
    if(Arguments.size() > 2) {
        cerr << "smash error: cd: too many arguments" << endl;
        return;
    }else if(Arguments[1] == "-" && lastdir.empty()){
        cerr << "smash error: cd: OLDPWD not set" << endl;
        return;
    }

    char* Dir;
    int Size = 50;
    Dir = getcwd(NULL,Size);
    while(!Dir) {
        Size += 20;
        Dir = getcwd(NULL, Size);
    }

    if(Arguments[1] == "-"){
        result = chdir((lastdir).c_str());
    }else{
        result = chdir(Arguments[1].c_str());
    }
    if(result == -1) {
        perror("smash error: chdir failed");
        delete Dir;
        return;
    }

    SmallShell::getInstance().oldDirname = Dir;
}

GetCurrDirCommand::GetCurrDirCommand(const char *cmd_line): BuiltInCommand(cmd_line) {}

void GetCurrDirCommand::execute() {
    char* Dir;
    int Size = 50;
    Dir = getcwd(NULL,Size);
    while(!Dir){
        Size += 20;
        Dir = getcwd(NULL,Size);
    }

    cout << Dir << endl;
}

ShowPidCommand::ShowPidCommand(const char *cmd_line): BuiltInCommand(cmd_line) {}

void ShowPidCommand::execute() {
    cout << "smash pid is " << processPID << endl;
}

QuitCommand::QuitCommand(const char *cmd_line, JobsList *jobs): BuiltInCommand(cmd_line),jobs(jobs) {}

void QuitCommand::execute() {
    if(Arguments.size() == 2 && Arguments[1] == "kill"){
        cout << "smash: sending SIGKILL signal to " << jobs->Total << " jobs:" << endl;
        jobs->printJobswithpid();
        jobs->killAllJobs();
    }
    exit(0);
}

JobsList::JobsList(){
    Total = 0;
}

JobsList::~JobsList() = default;

void JobsList::addJob(Command *cmd, bool isStopped) {
    auto* NewJob = new JobEntry;
    NewJob->cmd = cmd;
    if(!isStopped)
        NewJob->state = JobEntry::RUNNING;
    else
        NewJob->state = JobEntry::STOPPED;

    if(getMaxJobId())
        NewJob->jobID = getMaxJobId()->jobID + 1;
    else
        NewJob->jobID = 1;

    NewJob->StartTime = time(nullptr);

    if(NewJob->StartTime == -1){
        perror("smash error: time failed");
        delete NewJob;
        return;
    }
    Jobs.emplace_back(NewJob);

    Total++;

}

void JobsList::printJobsList() {
    removeFinishedJobs();
    for(auto & Job : Jobs){
        cout << "[" << Job->jobID << "] " << Job->cmd->cmdSyntax << " : " << Job->cmd->processPID << " "
        << (int) difftime(time(nullptr),Job->StartTime) << " secs";
        if(Job->state == JobEntry::STOPPED)
            cout << " (stopped)";
        cout << endl;
    }
}

void JobsList::printJobswithpid() {
    for(auto & Job : Jobs){
        cout << Job->cmd->processPID << ": " << Job->cmd->cmdSyntax <<  endl;
    }
}

void JobsList::killAllJobs() {
    for(auto & Job : Jobs){
        if (kill(Job->cmd->processPID,SIGKILL) == -1){
            perror("smash error: kill failed");
            return;
        }
    }
}

void JobsList::removeFinishedJobs() {
    pid_t p;
    p = waitpid(WAIT_ANY, NULL, WNOHANG);
    while (p > 0) {
        JobEntry* J = getJobByPID(p);
        if(J)
            SmallShell::getInstance().jobList->removeJobById(J->jobID);
        p = waitpid(WAIT_ANY, NULL, WNOHANG);

    }

}

JobsList::JobEntry *JobsList::getJobById(int jobId) {
    if(Jobs.empty())
        return nullptr;
    for(auto & Job : Jobs){
        if(Job->jobID == jobId)
            return Job;
    }
    return nullptr;
}

void JobsList::removeJobById(int jobId) {
    for(unsigned int i = 0; i < Jobs.size();i++){
        if(Jobs[i]->jobID == jobId){
            JobEntry* toDelete = Jobs[i];
            Jobs.erase(std::next(Jobs.begin(),i));
            Total--;
            delete toDelete;
            return;
        }
    }
}

JobsList::JobEntry *JobsList::getLastJob(int *lastJobId) {
    if(Jobs.empty())
        return nullptr;
    return Jobs[Jobs.size()-1];
}

JobsList::JobEntry *JobsList::getLastStoppedJob(int *jobId) {
    if(Jobs.empty())
        return nullptr;
    for(int i = Jobs.size()-1; i >= 0; i--){
        if(Jobs[i]->state == JobEntry::STOPPED){
            *jobId = Jobs[i]->jobID;
            return Jobs[i];
        }
    }
    return nullptr;
}

JobsList::JobEntry *JobsList::getMaxJobId() {
    int max = 0;
    JobEntry* Job;
    if(Jobs.empty())
        return nullptr;
    for(int i = Jobs.size()-1; i >= 0; i--){
        if(Jobs[i]->jobID > max){
            max = Jobs[i]->jobID;
            Job = Jobs[i];
        }
    }
    return Job;
}

JobsList::JobEntry *JobsList::getJobByPID(pid_t pid) {
    if(Jobs.empty())
        return nullptr;
    for(auto & Job : Jobs){
        if(Job->cmd->processPID == pid)
            return Job;
    }
    return nullptr;
}

JobsCommand::JobsCommand(const char *cmd_line, JobsList *jobs): BuiltInCommand(cmd_line),Jobs(jobs) {}

void JobsCommand::execute() {
    Jobs->printJobsList();
}

KillCommand::KillCommand(const char *cmd_line, JobsList *jobs): BuiltInCommand(cmd_line),Jobs(jobs) {}

void KillCommand::execute() {
    int sig,job;
    if(Arguments.size() != 3){
        cerr << "smash error: kill: invalid arguments" << endl;
        return;
    }
    try{
        sig = stoi(Arguments[1]);
        job = stoi(Arguments[2]);
    }catch(...){
        cerr << "smash error: kill: invalid arguments" << endl;
        return;
    }if(sig > 0){
        cerr << "smash error: kill: invalid arguments" << endl;
        return;
    }
    if(Jobs->getJobById(job) == nullptr){
        cerr << "smash error: kill: job-id " << job << " does not exist" << endl;
        return;
    }
    sig *= -1;
    int result = kill(Jobs->getJobById(job)->cmd->processPID,sig);

    if (result == -1){
        perror("smash error: kill failed");
        return;
    }
    cout << "signal number " << sig << " was sent to pid " << Jobs->getJobById(job)->cmd->processPID << endl;
}

ForegroundCommand::ForegroundCommand(const char *cmd_line, JobsList *jobs): BuiltInCommand(cmd_line),Jobs(jobs) {}

void ForegroundCommand::execute() {
    int jobid = -1;
    if(Arguments.size() > 2){
        cerr << "smash error: fg: invalid arguments" << endl;
        return;
    }
    try{
        if(Arguments.size() == 2)
            jobid = stoi(Arguments[1]);
    }catch(...){
        cerr << "smash error: fg: invalid arguments" << endl;
        return;
    }
    if(Jobs->Jobs.empty()){
        cerr << "smash error: fg: job list is empty" << endl;
        return;
    }else if(jobid != -1 && Jobs->getJobById(jobid) == nullptr){
        cerr << "smash error: fg: job-id " << jobid << " does not exist" << endl;
        return;
    }
    if(jobid == -1)
        jobid = Jobs->getMaxJobId()->jobID;
    if(kill(Jobs->getJobById(jobid)->cmd->processPID,SIGCONT) == -1){
        perror("smash error: kill failed");
        return;
    }
    cout << Jobs->getJobById(jobid)->cmd->cmdSyntax << " : " << Jobs->getJobById(jobid)->cmd->processPID << endl;
    pid_t PID = Jobs->getJobById(jobid)->cmd->processPID;
    SmallShell::getInstance().currCommand = new ExternalCommand(*(ExternalCommand*)(Jobs->getJobById(jobid)->cmd));
    SmallShell::getInstance().currCommand->Type = FGEXTERNAL;
    Jobs->removeJobById(jobid);
    waitpid(PID, nullptr,WUNTRACED);
}

BackgroundCommand::BackgroundCommand(const char *cmd_line, JobsList *jobs): BuiltInCommand(cmd_line),Jobs(jobs) {}

void BackgroundCommand::execute() {
    int jobid = -1;
    if(Arguments.size() > 2){
        cerr << "smash error: bg: invalid arguments" << endl;
        return;
    }
    try{
        if(Arguments.size() == 2)
            jobid = stoi(Arguments[1]);
    }catch(...){
        cerr << "smash error: bg: invalid arguments" << endl;
        return;
    }if(jobid == -1 && Jobs->getLastStoppedJob(&jobid) == nullptr){
        cerr << "smash error: bg: there is no stopped jobs to resume" << endl;
        return;
    }if(jobid != -1 && Jobs->getJobById(jobid) == nullptr){
        cerr << "smash error: bg: job-id " << jobid << " does not exist" << endl;
        return;
    }if(jobid != -1 && Jobs->getJobById(jobid)->state == JobsList::JobEntry::RUNNING) {
        cerr << "smash error: bg: job-id " << jobid << " is already running in the background" << endl;
        return;
    }
    if(kill(Jobs->getJobById(jobid)->cmd->processPID,SIGCONT) == -1){
        perror("smash error: kill failed");
        return;
    }
    Jobs->getJobById(jobid)->state = JobsList::JobEntry::RUNNING;
    cout << Jobs->getJobById(jobid)->cmd->cmdSyntax << " : " << Jobs->getJobById(jobid)->cmd->processPID << endl;
}

chpromptCommand::chpromptCommand(const char *cmd_line): BuiltInCommand(cmd_line) {}

void chpromptCommand::execute() {
    if(Arguments.size() == 1)
        strcpy(SmallShell::getInstance().smashName,"smash");
    else {
        strcpy(SmallShell::getInstance().smashName,const_cast<char *>(Arguments[1].c_str()));
    }
}


HeadCommand::HeadCommand(const char *cmd_line) : BuiltInCommand(cmd_line) {}

void HeadCommand::execute() {
    int NumOfRows;
    int file;
    char * buff=NULL;
    if (Arguments.size() == 1) {
        cerr << "smash error: head: invalid arguments" << endl;
    } else if (Arguments.size() == 2) {
        if (Arguments[1][0] == '-') {
            try {
                NumOfRows = stoi(Arguments[1]);
            } catch (...) {
                cerr << "smash error: head: invalid arguments" << endl;
                return;
            }
            file = open(Arguments[1].c_str(), O_RDONLY, 0666);
            if (file < 0) {
                perror("smash error: open failed");
                return;
            }
        } else {
            NumOfRows = 10;
        }
        file = open(Arguments[1].c_str(), O_RDONLY, 0666);
        if (file < 0) {
            perror("smash error: open failed");
            return;
        }
        buff = new char[1];
        size_t size = 1;
        ssize_t text = read(file, (void*)buff, size);
        if (text < 0) {
            perror("smash error: read failed");
            return;
        }
        if(text==0) return;

        while (NumOfRows>0){

            cout << *buff;
            buff=NULL;
            buff = new char[1];
            text = read(file, (void*)buff, size);
            if (text < 0) {
                perror("smash error: read failed");
                return;
            }

            if(text==0) return;

            if(*buff == '\n'){
                NumOfRows--;
            }

        }
        if(!NumOfRows)
            cout << '\n';

    } else {
        if (Arguments[1][0] == '-') {
            try {
                NumOfRows = stoi(Arguments[1]);
                NumOfRows = -1 * NumOfRows;
            } catch (...) {
                cerr << "smash error: head: invalid arguments" << endl;
                return;
            }
        } else{
            cerr << "smash error: head: invalid arguments" << endl;
            return;
        }
        file = open(Arguments[2].c_str(), O_RDONLY, 0666);
        if (file < 0) {
            perror("smash error: open failed");
            return;
        }

        buff = new char[1];
        size_t size = 1;
        ssize_t text = read(file, (void*)buff, size);

        if (text < 0) {
            perror("smash error: read failed");
            return;
        }

        if(text==0) return;

        while (NumOfRows>0){

        cout << *buff;
        buff=NULL;
        buff = new char[1];
        text = read(file, (void*)buff, size);
        if (text < 0) {
            perror("smash error: read failed");
            return;
        }

        if(text==0) return;

        if(*buff == '\n')
             NumOfRows--;
        }

        if(!NumOfRows)
            cout << '\n';
    }

    delete buff;
    SmallShell::getInstance().currCommand=NULL;
}

SmallShell::SmallShell() {

    currCommand = NULL;
    oldDirname.clear();
    smashName = new char[6];
    jobList = new JobsList;
    strcpy(smashName, "smash");
    smashPID = getpid();

}

SmallShell::~SmallShell() {
    delete smashName;
    delete jobList;
}

void FreeArgs(int N,char **args) {
    for (int i = 0; i < N; i++) {
        free(args[i]);
        args[i] = NULL;
    }
    delete[] args;
    args = NULL;
}



/**
* Creates and returns a pointer to Command class which matches the given command line (cmd_line)
*/
Command * SmallShell::CreateCommand(const char* cmd_line) {

    char **Args = new char *[22];
    Command *cmd;
    for (int i = 0; i < 22; i++) {
        Args[i] = NULL;
    }
    int NumOfArgs = _parseCommandLine(cmd_line, Args);

    if (NumOfArgs == 0)
        return NULL;
    if (strchr(cmd_line, '|')) {
        cmd = new PipeCommand(cmd_line);
        cmd->Type=PIPE;
        currCommand = cmd;
        FreeArgs(NumOfArgs,Args );
        return cmd;
    }
    if (strchr(cmd_line, '>')) {
        cmd = new RedirectionCommand(cmd_line);
        cmd->Type=REDIRECTION;
        currCommand = cmd;
        FreeArgs(NumOfArgs, Args);
        return cmd;
    }
    char *tmpCmd = new char[strlen(cmd_line) + 1];
    strcpy(tmpCmd, cmd_line);
    _removeBackgroundSign(tmpCmd);
    char **newArgs = new char *[22];
    for (int i = 0; i < 22; i++) {
        newArgs[i] = NULL;
    }
    int NewnumOfArgs = _parseCommandLine(tmpCmd, newArgs);
    if (strcmp(newArgs[0], "chprompt\0") == 0) {

        cmd = new chpromptCommand(tmpCmd);
        currCommand = cmd;
        FreeArgs(NewnumOfArgs,newArgs );
        FreeArgs(NumOfArgs, Args);
        delete[] tmpCmd;
        return cmd;
    }
    else if(strcmp(newArgs[0], "head\0")==0){
        cmd= new HeadCommand(tmpCmd);
        currCommand=cmd;
        FreeArgs(NewnumOfArgs,newArgs );
        FreeArgs(NumOfArgs,Args );
        delete[] tmpCmd;
        return cmd;
    }
    if (strcmp(newArgs[0], "pwd\0") == 0) {
        cmd = new GetCurrDirCommand(tmpCmd);
        currCommand = cmd;
        FreeArgs(NewnumOfArgs,newArgs );
        FreeArgs(NumOfArgs,Args );
        delete[] tmpCmd;
        return cmd;
    }

    if (strcmp(newArgs[0], "cd\0") == 0) {

        cmd = new ChangeDirCommand(tmpCmd, &oldDirname);
        currCommand = cmd;
        FreeArgs(NewnumOfArgs,newArgs );
        FreeArgs(NumOfArgs,Args);
        delete[] tmpCmd;
        return cmd;
    }

    if (strcmp(newArgs[0], "kill\0") == 0) {
        cmd = new KillCommand(tmpCmd, jobList);
        currCommand = cmd;
        FreeArgs(NewnumOfArgs,newArgs );
        FreeArgs(NumOfArgs,Args );
        delete[] tmpCmd;
        return cmd;
    }

    if (strcmp(newArgs[0], "jobs\0") == 0) {
        cmd = new JobsCommand(tmpCmd, jobList);
        currCommand = cmd;
        FreeArgs(NewnumOfArgs,newArgs );
        FreeArgs(NumOfArgs,Args );
        delete[] tmpCmd;
        return cmd;
    }


    if (strcmp(newArgs[0], "fg\0") == 0) {
        cmd = new ForegroundCommand(tmpCmd, jobList);
        currCommand = cmd;
        FreeArgs(NewnumOfArgs,newArgs );
        FreeArgs(NumOfArgs,Args );
        delete[] tmpCmd;
        return cmd;
    }

    if (strcmp(newArgs[0], "showpid\0") == 0) {
        cmd = new ShowPidCommand(tmpCmd);
        currCommand = cmd;
        FreeArgs(NewnumOfArgs,newArgs );
        FreeArgs(NumOfArgs,Args );

        delete[] tmpCmd;
        return cmd;
    }

    if (strcmp(newArgs[0], "bg\0") == 0) {
        cmd = new BackgroundCommand(tmpCmd, jobList);
        currCommand = cmd;
        FreeArgs(NewnumOfArgs,newArgs );
        FreeArgs(NumOfArgs,Args );
        delete[] tmpCmd;
        return cmd;
    }
    if (strcmp(newArgs[0], "quit\0") == 0) {
        cmd = new QuitCommand(tmpCmd, jobList);
        currCommand = cmd;
        FreeArgs(NewnumOfArgs,newArgs );
        FreeArgs(NumOfArgs,Args );
        delete[] tmpCmd;
        return cmd;
    }

    cmd = new ExternalCommand(cmd_line);
    currCommand = cmd;
    FreeArgs(NewnumOfArgs,newArgs );
    FreeArgs(NumOfArgs,Args );
    delete[] tmpCmd;
    return cmd;

}

void SmallShell::executeCommand(const char *cmd_line) {

    currCommand = CreateCommand(cmd_line);
    if (currCommand) {

        jobList->removeFinishedJobs();
        if (currCommand->Type == BGEXTERNAL) {
            jobList->addJob(currCommand);
        }

        currCommand->execute();
    }

    currCommand = nullptr;
}