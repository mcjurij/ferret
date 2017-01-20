
#include "job.h"

#include <cassert>

using namespace std;


static unsigned int job_idc = 0;

static unsigned int job_idgen()
{
    return ++job_idc;
}

// -----------------------------------------------------------------------------
string Job::getStateAsString()
{
    string s;
    
    switch( state )
    {
        case INVALID:
            s = "INVALID";
            break;
        case QUEUED:
            s = "QUEUED";
            break;
        case DONE:
            s = "DONE";
            break;
        case FAILED:
            s = "FAILED";
            break;
        case SPECIAL:
            s = "SPECIAL";
            break;
    }
    
    return s;
}


string Job::getOutput() const
{
    return output;
}


string Job::getStdOut() const
{
    return std_output;
}


string Job::getErrOut() const
{
    return err_output;
}


void Job::setError( bool err )
{
    state =  err ? FAILED : DONE;
}


bool Job::hasError() const
{
    return state != DONE && state != SPECIAL;
}

// -----------------------------------------------------------------------------

static size_t jobidx( unsigned int job_id )
{
    return job_id - 1;
}


JobStorer::JobStorer()
{
}


unsigned int JobStorer::createJob( const string &fn, const vector<string> &args, int file_id)
{
    unsigned int job_id = job_idgen();
    size_t i = jobidx( job_id );
    
    Job::state_t st = Job::QUEUED;
    if( file_id < 0 )
        st = Job::SPECIAL;
    
    if( i >= jobs.size() )
        jobs.resize( i + 1 );
    
    jobs[ i ] = Job( fn, args, file_id, st);
    
    return job_id;
}


Job::state_t JobStorer::getState( unsigned int job_id ) const
{
    size_t i = jobidx( job_id );
    assert( i < jobs.size() );
    
    return jobs[ i ].getState();
}


string JobStorer::getStateAsString( unsigned int job_id )
{
    size_t i = jobidx( job_id );
    assert( i < jobs.size() );
    
    return jobs[ i ].getStateAsString();
}


string JobStorer::getFileName( unsigned int job_id ) const
{
    size_t i = jobidx( job_id );
    assert( i < jobs.size() );
    
    return jobs[ i ].getFileName();
}


void JobStorer::append( unsigned int job_id, const string &s)
{
    size_t i = jobidx( job_id );
    assert( i < jobs.size() );
    
    jobs[ i ].appendOutput( s );
}


void JobStorer::appendStd( unsigned int job_id, const string &s)
{
    size_t i = jobidx( job_id );
    assert( i < jobs.size() );
    
    jobs[ i ].appendStdOut( s );
}


void JobStorer::appendErr( unsigned int job_id, const string &s)
{
    size_t i = jobidx( job_id );
    assert( i < jobs.size() );
    
    if( s.length() > 0  )
        jobs[ i ].appendErrOut( s );
}


string JobStorer::getOutput( unsigned int job_id ) const
{
    size_t i = jobidx( job_id );
    if( i < jobs.size() )
        return jobs[ i ].getOutput();
    else
        return "";
}


string JobStorer::getStdOut( unsigned int job_id ) const
{
    size_t i = jobidx( job_id );
    if( i < jobs.size() )
        return jobs[ i ].getStdOut();
    else
        return "";
}


string JobStorer::getErrOut( unsigned int job_id ) const
{
    size_t i = jobidx( job_id );
    if( i < jobs.size() )
        return jobs[ i ].getErrOut();
    else
        return "";
}


bool JobStorer::hasOutput( unsigned int job_id ) const
{
    assert( job_id >= 1 && job_id <= jobs.size() );
    
    size_t i = jobidx( job_id );
    if( i < jobs.size() )
        return jobs[ i ].hasOutput();
    else
        return false;
}


void JobStorer::setError( unsigned int job_id, bool err)
{
    assert( job_id >= 1 && job_id <= jobs.size() );
    
    size_t i = jobidx( job_id );
    if( i < jobs.size() )
        jobs[ i ].setError( err );
}


bool JobStorer::hasError( unsigned int job_id )
{
    assert( job_id >= 1 && job_id <= jobs.size() );
    
    size_t i = jobidx( job_id );
    if( i < jobs.size() )
        return jobs[ i ].hasError();
    else
        return true;
}


int JobStorer::getFileId( unsigned int job_id ) const
{
    assert( job_id >= 1 && job_id <= jobs.size() );
    
    size_t i = jobidx( job_id );
    if( i < jobs.size() )
        return jobs[ i ].getFileId();
    else
        return -1;
}


void JobStorer::clear()
{
    job_idc = 0;
    jobs.clear();
}
