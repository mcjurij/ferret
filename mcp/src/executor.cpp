#include <sys/wait.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <iostream>
#include <cassert>
#include <time.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include <cmath>

#include "executor.h"
#include "engine.h"
#include "glob_utility.h"
#include "output_collector.h"

using namespace std;

static const string depsh = "build/ferret_dep.sh";

static bool terminate_by_signal = false;
static int  terminate_signal;

extern "C" {
void termination_handler( int signum )
{
    terminate_by_signal = true;
    terminate_signal = signum;
}
}

static struct sigaction old_action_int, old_action_hup, old_action_term;

static void set_signal_handler()
{
    struct sigaction new_action;
    
    // set up the structure to specify the new action.
    new_action.sa_handler = termination_handler;
    sigemptyset( &new_action.sa_mask );
    new_action.sa_flags = 0;
    
    sigaction( SIGINT, NULL, &old_action_int);
    if( old_action_int.sa_handler != SIG_IGN )
        sigaction( SIGINT, &new_action, NULL);
    
    sigaction( SIGHUP, NULL, &old_action_hup);
    if( old_action_hup.sa_handler != SIG_IGN )
        sigaction( SIGHUP, &new_action, NULL);
    
    sigaction( SIGTERM, NULL, &old_action_term);
    if( old_action_term.sa_handler != SIG_IGN )
        sigaction( SIGTERM, &new_action, NULL);


    sigset_t blockMask;
    sigemptyset(&blockMask);
    sigaddset(&blockMask, SIGCHLD);
    if( sigprocmask(SIG_SETMASK, &blockMask, NULL) == -1 )
        cerr << "warning: could not block SIGCHLD\n";

}


static void restore_signal_handler()
{
    if( old_action_int.sa_handler != SIG_IGN )
        sigaction( SIGINT, &old_action_int, NULL);
    
    if( old_action_hup.sa_handler != SIG_IGN )
        sigaction( SIGHUP, &old_action_hup, NULL);
    
    if( old_action_term.sa_handler != SIG_IGN )
        sigaction( SIGTERM, &old_action_term, NULL);
}


// ------------------------------------------------------------
void ExecutorCommand::setJobId( unsigned int id )
{
    jobid = id;
}


string ExecutorCommand::getStateAsString()
{
    string s;
    
    switch( state )
    {
        case INVALID:
            s = "INVALID";
            break;
        case WAITING:
            s = "WAITING";
            break;
        case PROCESSING:
            s = "PROCESSING";
            break;
        case DONE:
            s = "DONE";
            break;            
        case FAILED:
            s = "FAILED";
            break;
    }

    return s;
}

// -----------------------------------------------------------------------------

static pid_t execCmd( const string &ex, ExecutorCommand &cmd, vector<string> args)
{
    pid_t child_pid;
    int out_filedes[2], err_filedes[2];
    
    if( pipe( out_filedes ) == -1 )
    {
        perror("pipe");
        exit(1);
    }
    
    if( pipe( err_filedes ) == -1 )
    {
        perror("pipe");
        exit(1);
    }
    
    if( (child_pid = fork()) < 0 )
    {
        perror("fork failure");
        return -1;
    }
    
    if( child_pid == 0 )
    {
        while( (dup2( out_filedes[1], STDOUT_FILENO) == -1) && (errno == EINTR) )
        {
        }
        close( out_filedes[1] );
        close( out_filedes[0] );
        
        while( (dup2( err_filedes[1], STDERR_FILENO) == -1) && (errno == EINTR) )
        {
        }
        close( err_filedes[1] );
        close( err_filedes[0] );
        
        char **newargv = (char **)malloc( sizeof(char*) * (args.size()+2) );
        unsigned int i, k=0;
        newargv[k++] = strdup( ex.c_str() );
        for( i=0; i<args.size(); i++)
            newargv[k++] = strdup( args[i].c_str() );
        newargv[k] = 0;
        
        execv( ex.c_str(), newargv);
        perror("exec");   // exec..() returns only on error

        _exit(1);
    }
    
    close( out_filedes[1] );
    close( err_filedes[1] );

    cmd.setStdoutFiledes( out_filedes[0] );
    cmd.setStderrFiledes( err_filedes[0] );
    
    int flags = fcntl( out_filedes[0], F_GETFL, 0);
    if( fcntl( out_filedes[0], F_SETFL, flags | O_NONBLOCK))
    {
        perror( "fcntl" );
        exit(1);
    }
    
    flags = fcntl( err_filedes[0], F_GETFL, 0);
    if( fcntl( err_filedes[0], F_SETFL, flags | O_NONBLOCK))
    {
        perror( "fcntl" );
        exit(1);
    }
    
    return child_pid;
}


pid_t Executor::processCommand( ExecutorCommand &cmd )
{
    pid_t pid = -1;
    
    if( cmd.getCmdType() == "DEP" )
    {
        pid = execCmd( depsh, cmd, cmd.getArgs());
    }
    else if( cmd.getCmdType() == "EXSH" )
    {
        vector<string> args = cmd.getArgs();
        args.insert( args.begin(), cmd.getFileName());
        pid = execCmd( "/bin/sh", cmd, args);
    }
    else if( cmd.getCmdType() == "BARRIER" )
    {
        if( verbosity > 0 )
            cout << "BARRIER... waiting for all sub processes to finish before fetching next commands\n";
        pid = 0;
        barrierMode = true;
    }
    else if( cmd.getCmdType() == "FINALIZE" )
    {
        if( verbosity > 0 )
            cout << "FINALIZE... waiting for all sub processes to finish\n";
        pid = 0;
        finalizeMode = true;
    }
    else
    {
        cerr << "internal error: '" << cmd.getCmdType() << "' command type not implemented.\n";
        pid = -1;
    }
    
    return pid;
}


void Executor::readOutput( const ExecutorCommand &cmd )
{
    OutputCollector *oc = OutputCollector::getTheOutputCollector();
    int count;
    char buffer[1025];
    string b;
            
    while( (count = read( cmd.getStdoutFiledes(), buffer, 1024)) > 0 )
    {
        buffer[count] = 0;
        b = buffer;
        
        oc->appendJobOut( cmd.getJobId(), b);
        oc->appendJobStd( cmd.getJobId(), b);
        if( curses )
            oc->cursesAppend( cmd.getJobId(), b);
    }
    
    while( (count = read( cmd.getStderrFiledes(), buffer, 1024)) > 0 )
    {
        buffer[count] = 0;
        b = buffer;
        
        oc->appendJobOut( cmd.getJobId(), b);
        oc->appendJobErr( cmd.getJobId(), b);
        if( curses )
        {
            oc->cursesAppend( cmd.getJobId(), b);
            oc->cursesSetStderr( cmd.getJobId() );
        }
    }
}


void Executor::readOutputs()
{
    map<pid_t,ExecutorCommand>::const_iterator pit = pidToCmdMap.begin();
    
    for( pit = pidToCmdMap.begin(); pit != pidToCmdMap.end(); pit++)
    {
        const ExecutorCommand &cmd = pit->second;
        if( cmd.state == ExecutorCommand::PROCESSING && cmd.getCmdType() != "BARRIER" && cmd.getCmdType() != "FINALIZE" )
        {
            readOutput( cmd );
        }
    }
}


long Executor::delaySampler()   // return nsec
{
    curr_time = get_curr_time_ms();
    long diff = curr_time - last_time_ms;
    last_time_ms = curr_time;

    /*sample_calls++;
    double avg = (double)(curr_time - start_time_ms) / (double)sample_calls;
    cout << "                          avg = " << avg << "\n";
    */
    double wait = exp( -((double)diff*.5) );
    return (long)round(2. * 1000. * 1000. * wait);    // 2ms max
}


void Executor::checkExitState( ExecutorCommand &cmd, int status, EngineBase &engine)
{
    if( WIFEXITED(status) )
    {
        if( verbosity > 2 )
            cout << "checking exit state of job " << cmd.getJobId() << "\n";
        cmd.returnCode = WEXITSTATUS(status);
        if( cmd.returnCode != 0 )
        {
            errors++;
            cmd.state = ExecutorCommand::FAILED;

            vector<string> ftorem = cmd.getFilesToRemoveAfterSignal();
            
            for( size_t i = 0; i < ftorem.size(); i++)
                removeUnfinished.push_back( ftorem[i] );
        }
        else
            cmd.state = ExecutorCommand::DONE;
        
        engine.indicateDone( cmd.getFileId(), cmd.getJobId(), curr_time);
    }
    else
    {
        if( WIFSIGNALED(status) )
        {
            if( !terminate_by_signal )
                cerr << "error: a child was interrupted by a signal (pid = " << cmd.pid << ")\n";
            
            if( WCOREDUMP(status) )
                cerr << "error: a child dumped core (pid = " << cmd.pid << ")\n";
        }
        
        cmd.state = ExecutorCommand::FAILED;
        
        if( terminate_by_signal )
        {
            OutputCollector::getTheOutputCollector()->appendJobOut( cmd.getJobId(), "\nProcess terminated by signal from parent");
            OutputCollector::getTheOutputCollector()->appendJobErr( cmd.getJobId(), "\nProcess terminated by signal from parent");
        }
        
        if( cmd.getFilesToRemoveAfterSignal().size() > 0 )
        {
            size_t i ;
            vector<string> ftorem = cmd.getFilesToRemoveAfterSignal();
            
            for( i = 0; i < ftorem.size(); i++)
            {
                removeUnfinished.push_back( ftorem[i] );
                /* if( FindFiles::existsUncached( ftorem[i] ) )
                {
                    cout << "Removing unfinished output '" << ftorem[i] << "'\n";
                    ::remove( ftorem[i].c_str() );
                }*/
            }
        }
    }
}


void Executor::checkStates( EngineBase &engine )
{
    struct timespec t;
    t.tv_sec = 0;
    readOutputs();

#if 0
    map<pid_t,ExecutorCommand>::iterator pit;
    
    if( pidToCmdMap.size() == 1 )
    {
        pit = pidToCmdMap.begin();
        ExecutorCommand &cmd = pit->second;
        if( cmd.getCmdType() == "BARRIER" )   // last entry is barrier?
        {
            pidToCmdMap.clear();              // remove it!
            barrierMode = false;
        }
    }
    if( pidToCmdMap.size() == 0 )
        return;
#endif
    
    // wait a bit to prevent unnecessary waitpid calls
    t.tv_nsec = delaySampler();
    if( t.tv_nsec > 0 )
        nanosleep( &t, 0);
    
    int status;
    pid_t p = waitpid( -1, &status, WNOHANG);

    if( p<0 )
    {
        cerr << "error: waitpid returned -1\n";
        return;
    }
            
    if( p == 0 )
        return;  // ok, nothing to do
    
    map<pid_t,ExecutorCommand>::iterator pit, it = pidToCmdMap.find( p );
    if( it == pidToCmdMap.end() )
    {
        cerr << "error: a child exited that we do not know (pid = " << p << ")\n";
        return;
    }
    
    if( verbosity > 2 )
    {
        cout << "no of sub processes: " << pidToCmdMap.size() << " -----------------------\n";
        pit = pidToCmdMap.begin();
        for( ; pit != pidToCmdMap.end(); pit++)
        {
            ExecutorCommand &cmd = pit->second;
            cout << "   " << cmd.getJobId() << "  " << cmd.getCmdType() << "    " << cmd.getStateAsString() << " pid: " << cmd.pid << "\n";
        }
    }
    
    if( it != pidToCmdMap.end() )
    {
        ExecutorCommand &cmd = it->second;
        OutputCollector *oc = OutputCollector::getTheOutputCollector();
        
        readOutput( cmd );
        
        if( curses )
            oc->cursesEnd( cmd.getJobId() );
        
        if( close( cmd.getStdoutFiledes() ) < 0 )
        {
            perror( "close" );
            exit(1);
        }
        
        if( close( cmd.getStderrFiledes() ) < 0 )
        {
            perror( "close" );
            exit(1);
        }
        
        if( !curses && oc->hasJobOut( cmd.getJobId() ) )
        {
            string out = oc->getJobOut( cmd.getJobId() );
            
            cout << "-----------------------------------\n";
            cout << out;
        }
        
        checkExitState( cmd, status, engine);
    }
    
    map<pid_t,ExecutorCommand> help;  // warning: std::map.erase() behaviour has changed with gcc 5 onwards. Don't use!
    
    // erase all who are not in processing state to make room for new ones
    for( pit = pidToCmdMap.begin(); pit != pidToCmdMap.end(); pit++)
    {
        ExecutorCommand &cmd = pit->second;
        
        if( cmd.state == ExecutorCommand::PROCESSING )
            help[ pit->first ] = pit->second;
    }
    pidToCmdMap = help;

    if( (curr_time - html_timer) > 20000 )
    {
        html_timer = curr_time;

        ofstream osh( "last_run.html" );
        OutputCollector::getTheOutputCollector()->html( osh );
    }
    
    if( pidToCmdMap.size() == 1 )
    {
        pit = pidToCmdMap.begin();
        ExecutorCommand &cmd = pit->second;
        if( cmd.getCmdType() == "BARRIER" )   // last entry is barrier?
        {
            pidToCmdMap.clear();              // remove it!
            barrierMode = false;
        }
    }
}


void Executor::processCommands( EngineBase &engine )
{
    bool done = false;
    errors = 0;
    
    barrierMode = false;
    finalizeMode = false;

    start_time_ms = last_time_ms = get_curr_time_ms();
    sample_calls = 0;
    html_timer = start_time_ms;
    
    ofstream osh( "last_run.html" );
    OutputCollector::getTheOutputCollector()->resetHtml( osh );
    osh.close();
    
    terminate_by_signal = false;
    set_signal_handler();
    
    do {
        if( curses )
        {
            OutputCollector::getTheOutputCollector()->cursesEventHandler();  // must not block
            OutputCollector::getTheOutputCollector()->cursesUpdate();
        }
        
        if( barrierMode )
        {
            while( pidToCmdMap.size() > 0 && !terminate_by_signal )
                checkStates( engine );
        }
        else
        {
            while( !done && !barrierMode && !finalizeMode && pidToCmdMap.size() < maxParallel && !terminate_by_signal )
            {
                ExecutorCommand cmd = engine.nextCommand();
                
                if( cmd.getState() == ExecutorCommand::INVALID )
                {
                    cerr << "internal error: got invalid command from engine.\n";
                    errors++;
                    done = true;
                    break;
                }

                if( terminate_by_signal || cmd.getCmdType() == "FINALIZE" )
                {
                    done = true;
                    finalizeMode = true;
                    break;
                }
                
                cmd.state = ExecutorCommand::PROCESSING;
                cmd.pid = processCommand( cmd );

                if( cmd.pid < 0 )
                {
                    cmd.state = ExecutorCommand::FAILED;
                    cerr << "error: could not start process.\n";
                    done = true;
                    break;
                }
                else
                {
                    pidToCmdMap[ cmd.pid ] = cmd;
                    
                    if( curses )
                        OutputCollector::getTheOutputCollector()->cursesTopShowJob( cmd.getJobId() );
                }
            }
        }

        if( terminate_by_signal )
        {
            done = true;
            finalizeMode = true;
        }
        
        if( !finalizeMode && pidToCmdMap.size() > 0 )
            checkStates( engine );
    } while( !done && errors == 0 && !finalizeMode );
    
    if( terminate_by_signal )
    {
        cleanUpAfterSignal( terminate_signal, engine);
        
        list<string>::iterator rit = removeUnfinished.begin();
        for( ; rit != removeUnfinished.end(); rit++)
        {
            string fn = *rit;
            if( FindFiles::existsUncached( fn ) )
            {
                cout << "Removing unfinished output '" << fn << "'\n";
                ::remove( fn.c_str() );
            }
        }
    }
    else
    {
        html_timer = curr_time;
        if( pidToCmdMap.size() > 0 )
        {
            cout << "Waiting for unfinished jobs...\n";
            
            while( pidToCmdMap.size() > 0 )
                checkStates( engine );
        }
    }
    
    restore_signal_handler();
}


bool Executor::isInterruptedBySignal() const
{
    return terminate_by_signal;
}


void Executor::cleanUpAfterSignal( int signum, EngineBase &engine)
{
    map<pid_t,ExecutorCommand>::const_iterator pit;
    
    if( pidToCmdMap.size() > 0 )
        cout << "Cleaning up unfinished jobs...\n";
    
    for( pit = pidToCmdMap.begin(); pit != pidToCmdMap.end(); pit++)
    {
        if( pit->second.pid > 0 && pit->second.state == ExecutorCommand::PROCESSING )
        {
            int status;
            pid_t p = waitpid( pit->second.pid, &status, 0);
            
            if( p < 0 )
            {
                cerr << "error: waitpid returned -1\n";
                return;
            }
            
            if( p > 0 )
            {
                map<pid_t,ExecutorCommand>::iterator it = pidToCmdMap.find( p );
                if( it != pidToCmdMap.end() )
                {
                    checkExitState( it->second, status, engine);
                }
                else
                    cerr << "error: a child exited that we do not know (pid = " << p << ")\n";
            }
        }
    }
}


// -----------------------------------------------------------------------------
void MakefileExec::processCommands( EngineBase &engine )
{
    assert( 0 );
}

