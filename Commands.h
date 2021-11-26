#ifndef SMASH_COMMAND_H_
#define SMASH_COMMAND_H_

#include <vector>
#include "iostream"

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
    REDIRECTION,
};

class Command {
    public:

        string cmdSyntax;
        vector<string> Arguments;
        CommandType Type;

        Command(const char* cmd_line,CommandType type);
        virtual ~Command();
        virtual void execute() = 0;
        static vector<string> FillInArguments(const string& cmdline);
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
  // TODO: Add your data members
    public:
        PipeCommand(const char* cmd_line);
        virtual ~PipeCommand() {}
        void execute() override;
};

class RedirectionCommand : public Command {
 // TODO: Add your data members
    public:
        explicit RedirectionCommand(const char* cmd_line);
        virtual ~RedirectionCommand() {}
        void execute() override;
        //void prepare() override;
        //void cleanup() override;
};

class ChangeDirCommand : public BuiltInCommand {
    char **lastdir;
    ChangeDirCommand(const char* cmd_line, char** plastPwd);
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

class JobsList;
class QuitCommand : public BuiltInCommand {
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
            pid_t jobPID;
            int jobID;
            STATUS state;
            //TODO: add memebers fields
        };
        int Total;
        int MaxJob;
        vector<JobEntry> Jobs;
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
        // TODO: Add extra methods or modify exisitng ones as needed
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
        //TODO:Implement This
};


class SmallShell {
    private:
        SmallShell();
    public:
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
        // TODO: add extra methods as needed
};

#endif //SMASH_COMMAND_H_
