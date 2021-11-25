#include <unistd.h>
#include <string.h>
#include <iostream>
#include <vector>
#include <sstream>
#include <sys/wait.h>
#include <iomanip>
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
    cmdSyntax = cmd;
    this->Type = type;
    Arguments = FillInArguments(cmd);
}

vector<string> Command::FillInArguments(const string& cmdline) {
    vector<string> Vicky;
    char **Args;
    int size = _parseCommandLine(cmdline.c_str(),Args);
    for(int i = 0; i < size; i++){
        Vicky.emplace_back(Args[i]);
    }
    return Vicky;
}

BuiltInCommand::BuiltInCommand(const char *cmd_line): Command(cmd_line,BUILTIN) { }

ExternalCommand::ExternalCommand(const char *cmd_line): Command(cmd_line,FGEXTERNAL){

    if(_isBackgroundComamnd(cmd_line)){
        Type = BGEXTERNAL;
        int size = Arguments.size();
        if(Arguments[size-1] == "&"){
            Arguments.pop_back();
        }
    }

}

void ExternalCommand::execute() {
    pid_t son = fork();
    if(son == -1)
        perror("smash error: fork failed");
    if(Type == FGEXTERNAL){
        if(son == 0){
            setpgrp();
            char *Args[] = {"-c", const_cast<char *>(cmdSyntax.c_str()), NULL};
            if(execv("/bin/bash",Args) == -1)
                perror("smash error: exec failed");
        }else{
            int status,wstatus;

            waitpid(son,&wstatus,0);
            status = WEXITSTATUS(wstatus);
        }
    }else{
        if(son == 0){
            setpgrp();
            char *Args[] = {"-c", const_cast<char *>(cmdSyntax.substr(0, cmdSyntax.size() - 1).c_str()), NULL};
            if(execv("/bin/bash",Args) == -1)
                perror("smash error: exec failed");
        }
    }
}

ChangeDirCommand::ChangeDirCommand(const char *cmd_line, char **plastPwd): BuiltInCommand(cmd_line),lastdir(plastPwd) {}

void ChangeDirCommand::execute() {
    int result;
    if(Arguments.size() > 2) {
        cerr << "smash error: cd: too many arguments" << endl;
        return;
    }else if(Arguments[1] == "-" && lastdir == nullptr){
        cerr << "smash error: cd: OLDPWD not set" << endl;
        return;
    }
    if(Arguments[2] == "-"){
        result = chdir(*lastdir);
    }else{
        result = chdir(Arguments[2].c_str());
    }
    if(result == -1)
        perror("smash error: chdir failed");
}

GetCurrDirCommand::GetCurrDirCommand(const char *cmd_line): BuiltInCommand(cmd_line) {}

void GetCurrDirCommand::execute() {
    char* Dir;
    getwd(Dir);

    if (Dir == nullptr){
        perror("smash error: getcwd failed");
        return;
    }
    cout << Dir << endl;
}

ShowPidCommand::ShowPidCommand(const char *cmd_line): BuiltInCommand(cmd_line) {}

void ShowPidCommand::execute() {
    pid_t currPid = getpid();

    cout << "smash pid is " << currPid << endl;
}

QuitCommand::QuitCommand(const char *cmd_line, JobsList *jobs): BuiltInCommand(cmd_line),jobs(jobs) {}

void QuitCommand::execute() {
    if(Arguments[1] == "kill"){
        cout << "sending SIGKILL signal to " << jobs->Total << " jobs:" << endl;
        jobs->printJobswithpid();
        jobs->killAllJobs();
    }
    exit(0);
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
    }
    if(Jobs->getJobById(job) == nullptr){
        cerr << "smash error: kill: job-id " << job << " does not exist" << endl;
        return;
    }

    pid_t son = fork();
    pid_t ToKill = Jobs->getJobById(job)->jobPID;
    if(son == 0){
        char* Args[] = {"-c", "kill -s", const_cast<char *>(to_string(sig).c_str()),const_cast<char *>(to_string(ToKill).c_str())};
        setpgrp();
        execv("/bin/bash",Args);
    }else{
        waitpid(son, nullptr,0);
        cout << "signal number " << sig << " was sent to pid " << ToKill << endl;
    }
    //TODO:CHECK IF GOOD IMPLEMENTATION
}


SmallShell::SmallShell() {
// TODO: add your implementation
}

SmallShell::~SmallShell() {
// TODO: add your implementation
}

/**
* Creates and returns a pointer to Command class which matches the given command line (cmd_line)
*/
Command * SmallShell::CreateCommand(const char* cmd_line) {
	// For example:
/*
  string cmd_s = _trim(string(cmd_line));
  string firstWord = cmd_s.substr(0, cmd_s.find_first_of(" \n"));

  if (firstWord.compare("pwd") == 0) {
    return new GetCurrDirCommand(cmd_line);
  }
  else if (firstWord.compare("showpid") == 0) {
    return new ShowPidCommand(cmd_line);
  }
  else if ...
  .....
  else {
    return new ExternalCommand(cmd_line);
  }
  */
  return nullptr;
}

void SmallShell::executeCommand(const char *cmd_line) {
  // TODO: Add your implementation here
  // for example:
  // Command* cmd = CreateCommand(cmd_line);
  // cmd->execute();
  // Please note that you must fork smash process for some commands (e.g., external commands....)
}