#ifndef FERRET_EXECUTOR_H_
#define FERRET_EXECUTOR_H_

#include <vector>
#include <string>
#include <map>
#include <set>
#include <list>

class EngineBase;

class ExecutorBase {
public:
    ExecutorBase()
        : errors(0)
    {}
    ~ExecutorBase()
    {}
    
    virtual unsigned int getMaxParallel() const
    { return 1; }
    
    virtual void processCommands( EngineBase &engine ) = 0;

    virtual int getErrors() const
    { return errors; }
    
    virtual bool isInterruptedBySignal() const
    { return false; }
    
protected:
    int errors;
};


class ExecutorCommand {
public:
    typedef enum { INVALID=0, WAITING, PROCESSING, DONE, FAILED} state_t;
    
    ExecutorCommand()
        : jobid(-1), cmdType("invalid"), state(INVALID), pid(-1)
    {
    }
    
    ExecutorCommand( unsigned int id, const std::string &type)
        : jobid(id), cmdType(type), file_id(-1), state(WAITING), pid(-1)
    {
    }
    
    ExecutorCommand( const std::string &fn, int file_id)
        : jobid(-1), cmdType("EXSH"), fileName(fn), file_id(file_id), state(WAITING), pid(-1)
    {
    }
    
    ExecutorCommand( const std::string &fn, const std::vector<std::string> &args)
        : jobid(-1), cmdType("EXSH"), fileName(fn), args(args), file_id(-1), state(WAITING), pid(-1)
    {
    }
    
    ExecutorCommand( const std::vector<std::string> &args )    // for dependency file generation
        : jobid(-1), cmdType("DEP"), args(args), file_id(-1), state(WAITING), pid(-1)
    {
    }

public:
    void setJobId( unsigned int id );
    unsigned int getJobId() const
    { return jobid; }
    state_t getState() const
    { return state; }
    std::string getStateAsString();
    std::string getCmdType() const
    { return cmdType; }
    std::string getFileName() const
    { return fileName; }
    std::vector<std::string> getArgs() const
    { return args; }
    int getFileId() const
    { return file_id; }

    void addFileToRemoveAfterSignal( const std::string &fn )
    { filesToRemoveAfterSignal.push_back( fn ); }
    
    std::vector<std::string> getFilesToRemoveAfterSignal() const
    { return filesToRemoveAfterSignal; }
    
    void setStdoutFiledes( int fd )
    { stdout_filedes = fd; }
    void setStderrFiledes( int fd )
    { stderr_filedes = fd; }
    int getStdoutFiledes() const
    { return stdout_filedes; }
    int getStderrFiledes() const
    { return stderr_filedes; }

private:
    unsigned int jobid;
    std::string cmdType;   // DEP = dependencies, EXSH = execute sh script, BARRIER, FINALIZE
    std::string fileName;
    std::vector<std::string> args;
    int file_id;
    std::vector<std::string> filesToRemoveAfterSignal;
    int stdout_filedes;
    int stderr_filedes;
    
public:
    state_t state;
    pid_t pid;
    int returnCode;
};


class Executor : public ExecutorBase {
public:
    Executor()
        : maxParallel(4), curses(false), barrierMode(false), finalizeMode(false)
    {}
    
    Executor( unsigned int maxParallel, bool curses)
        : maxParallel(maxParallel), curses(curses), barrierMode(false), finalizeMode(false)
    {}

    virtual unsigned int getMaxParallel() const
    { return maxParallel; }
    
    pid_t processCommand( ExecutorCommand &cmd );
    
    virtual void processCommands( EngineBase &engine );
    virtual bool isInterruptedBySignal() const;
    
private:
    void cleanUpAfterSignal( int signum, EngineBase &engine);
    void readOutput( const ExecutorCommand &cmd );
    void readOutputs();
    long delaySampler();
    void checkExitState( ExecutorCommand &cmd, int status, EngineBase &engine);
    void checkStates( EngineBase &engine );
    
private:
    unsigned int maxParallel;
    bool curses;
    std::map<pid_t,ExecutorCommand> pidToCmdMap;
    
    bool barrierMode, finalizeMode;
    long long start_time_ms, last_time_ms, curr_time;
    unsigned long sample_calls;
    long long html_timer;
    std::list<std::string> removeUnfinished;
};


class MakefileExec : public ExecutorBase {
public:
    virtual void processCommands( EngineBase &engine );

private:
};


#endif
