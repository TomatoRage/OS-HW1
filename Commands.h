#ifndef SMASH_COMMAND_H_
#define SMASH_COMMAND_H_

#include <vector>
#include "iostream"
#include <time.h>

#define COMMAND_ARGS_MAX_LENGTH (200)
#define COMMAND_MAX_ARGS (20)
#define WHITESPACE " "

using namespace std;

enum CommandType{
    FGEXTERNAL,
    BGEXTERNAL,
    BUILTIN,
    SPECIAL,
    PIPE,
    REDIRECTION
};

class Command {
public:

    string cmdSyntax;
    vector<string> Arguments;
    CommandType Type;
    pid_t processPID;

    Command(const char* cmd_line,CommandType type);
    virtual ~Command();
    Command(Command& C);
    virtual void execute() = 0;
    void FillInArguments(const string& cmdline);
    //virtual void prepare();
    //virtual void cleanup();
};

class BuiltInCommand : public Command {
public:
    BuiltInCommand(const char* cmd_line);
    virtual ~BuiltInCommand() = default;
};

class ExternalCommand : public Command {
public:
    ExternalCommand(const char* cmd_line);
    virtual ~ExternalCommand() = default;
    void execute() override;
};

class PipeCommand : public Command {
public:
    PipeCommand(const char* cmd_line);
    virtual ~PipeCommand() {}
    void execute() override;

};

class RedirectionCommand : public Command {
public:
    explicit RedirectionCommand(const char* cmd_line);
    virtual ~RedirectionCommand() {}
    void execute() override;
    //void prepare() override;
    //void cleanup() override;
};

class ChangeDirCommand : public BuiltInCommand {
public:
    string lastdir;
    ChangeDirCommand(const char* cmd_line, string* plastPwd);
    virtual ~ChangeDirCommand() {}
    void execute() override;
};

class GetCurrDirCommand : public BuiltInCommand {
public:
    GetCurrDirCommand(const char* cmd_line);
    virtual ~GetCurrDirCommand() {}
    void execute() override;
};

class ShowPidCommand : public BuiltInCommand {
public:
    ShowPidCommand(const char* cmd_line);
    virtual ~ShowPidCommand() {}
    void execute() override;
};

class chpromptCommand : public BuiltInCommand {
public:
    chpromptCommand(const char *cmd_line);
    virtual ~chpromptCommand() = default;
    void execute() override;
};


class JobsList;
class QuitCommand : public BuiltInCommand {
public:
    JobsList* jobs;
    QuitCommand(const char* cmd_line, JobsList* jobs);
    virtual ~QuitCommand() {}
    void execute() override;
};




class JobsList {
public:
    class JobEntry {
    public:
        enum STATUS{
            STOPPED,
            RUNNING,
            FINISHED
        };
        Command* cmd;
        int jobID;
        STATUS state;
        time_t StartTime;
    };
    int Total;
    int MaxJob;
    vector<JobEntry*> Jobs;
public:
    JobsList();
    ~JobsList();
    void addJob(Command* cmd, bool isStopped = false);
    void printJobsList();
    void printJobswithpid();
    void killAllJobs();
    void removeFinishedJobs();
    JobEntry * getJobById(int jobId);
    void removeJobById(int jobId);
    JobEntry * getLastJob(int* lastJobId);
    JobEntry *getLastStoppedJob(int *jobId);
    JobEntry *getMaxJobId();
    JobEntry *getJobByPID(pid_t pid);
};

class JobsCommand : public BuiltInCommand {
    JobsList* Jobs;
public:
    JobsCommand(const char* cmd_line, JobsList* jobs);
    virtual ~JobsCommand() {}
    void execute() override;
};

class KillCommand : public BuiltInCommand {
    JobsList* Jobs;
public:
    KillCommand(const char* cmd_line, JobsList* jobs);
    virtual ~KillCommand() {}
    void execute() override;
};

class ForegroundCommand : public BuiltInCommand {
    JobsList* Jobs;
public:
    ForegroundCommand(const char* cmd_line, JobsList* jobs);
    virtual ~ForegroundCommand() {}
    void execute() override;
};

class BackgroundCommand : public BuiltInCommand {
    JobsList* Jobs;
public:
    BackgroundCommand(const char* cmd_line, JobsList* jobs);
    virtual ~BackgroundCommand() {}
    void execute() override;
};

class HeadCommand : public BuiltInCommand {
public:
    HeadCommand(const char* cmd_line);
    virtual ~HeadCommand() {}
    void execute() override;
};


class SmallShell {
private:
    SmallShell();
public:
    Command* currCommand;
    char* smashName;
    JobsList* jobList;
    string oldDirname;
    pid_t smashPID;
    Command *CreateCommand(const char* cmd_line);
    SmallShell(SmallShell const&)      = delete; // disable copy ctor
    void operator=(SmallShell const&)  = delete; // disable = operator
    static SmallShell& getInstance() // make SmallShell singleton
    {
        static SmallShell instance; // Guaranteed to be destroyed.
        // Instantiated on first use.
        return instance;
    }
    ~SmallShell();
    void executeCommand(const char* cmd_line);
};

#endif //SMASH_COMMAND_H_