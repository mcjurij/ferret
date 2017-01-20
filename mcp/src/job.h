#ifndef FERRET_JOB_H_
#define FERRET_JOB_H_

#include <string>
#include <vector>


class Job {
public:
    typedef enum { INVALID=0, QUEUED, DONE, FAILED, SPECIAL} state_t;
    
    Job()
        : jobid(-1), file_id(-1), state(INVALID)
    {
    }
    
    Job( const std::string &fn, const std::vector<std::string> &args, int file_id, state_t st)
        : jobid(-1), fileName(fn), args(args), file_id(file_id), state(st)
    {
    }
    

public:
    unsigned int getJobId() const
    { return jobid; }
    state_t getState() const
    { return state; }
    std::string getStateAsString();
    std::string getFileName() const
    { return fileName; }
    std::vector<std::string> getArgs() const
    { return args; }
    int getFileId() const
    { return file_id; }
    
    void appendOutput( const std::string &s )
    { output += s; };
    void appendStdOut( const std::string &s )
    { std_output += s; };
    void appendErrOut( const std::string &s )
    { err_output += s; };
    
    bool hasOutput() const
    { return output.length()>0; }
    
    std::string getOutput() const;
    std::string getStdOut() const;
    std::string getErrOut() const;
    
    void setError( bool err );
    bool hasError() const;
    
private:
    unsigned int jobid;
    
    std::string fileName;
    std::vector<std::string> args;
    int file_id;
    state_t state;

    std::string output, std_output, err_output;
};


class JobStorer {
    
public:
    JobStorer();
    
public:
    unsigned int createJob( const std::string &fn, const std::vector<std::string> &args, int file_id);
    Job::state_t getState( unsigned int job_id ) const;
    std::string getStateAsString( unsigned int job_id );
    std::string getFileName( unsigned int job_id ) const;
    
    void append( unsigned int job_id, const std::string &s);
    void appendStd( unsigned int job_id, const std::string &s);
    void appendErr( unsigned int job_id, const std::string &s);
    void cursesAppend( unsigned int job_id, const std::string &s);
    void cursesEnd( unsigned int job_id );
    
    std::string getOutput( unsigned int job_id ) const;
    std::string getStdOut( unsigned int job_id ) const;
    std::string getErrOut( unsigned int job_id ) const;
    
    bool hasOutput( unsigned int job_id ) const;

    void setError( unsigned int job_id, bool err);
    bool hasError( unsigned int job_id );
    
    int getFileId( unsigned int job_id ) const;

    size_t getSize() const
    { return jobs.size(); }

    void clear();
    
private:
    std::vector<Job> jobs;
};


#endif
