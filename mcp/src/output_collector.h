#ifndef FERRET_OUTPUT_COLLECTOR_H_
#define FERRET_OUTPUT_COLLECTOR_H_

#include <vector>
#include <map>
#include <sstream>
#include <fstream>
#include <cassert>

#include "file_manager.h"
#include "job.h"
#include "curses_screen.h"


class BaseNode;

class OutputCollector {

private:    
    OutputCollector();

public:
    static OutputCollector *getTheOutputCollector();
    
    unsigned int createJob( const std::string &fn, const std::vector<std::string> &args, int file_id);
    unsigned int createJob( const std::string &psuedo_fn );
    
    void cursesEnable( unsigned int parallel );
    void cursesDisable();
    void cursesEventHandler();
    void cursesUpdate();
    
    void appendJobOut( unsigned int job_id, const std::string &s);
    void appendJobStd( unsigned int job_id, const std::string &s);
    void appendJobErr( unsigned int job_id, const std::string &s);
    bool hasJobOut( unsigned int job_id ) const;
    
    void cursesAppend( unsigned int job_id, const std::string &s);
    void cursesEnd( unsigned int job_id );
    void cursesSetStderr( unsigned int job_id );
    
    void cursesSetTopLine( int y, const std::string &l);
    void cursesTopShowJob( unsigned int job_id );
    
    std::string getJobOut( unsigned int job_id ) const;
    std::string getJobOutHtml( unsigned int job_id  ) const;
    std::string getJobStd( unsigned int job_id  ) const;
    std::string getJobStdHtml( unsigned int job_id ) const;
    std::string getJobErr( unsigned int job_id ) const;
    std::string getJobErrHtml( unsigned int job_id ) const;
    
    void text( std::ofstream &os );
    void html( std::ofstream &os );
    void addHtml( unsigned int job_id, const std::string &s);
    std::string getHtml( unsigned int job_id ) const;
    void addFreeHtml( const std::string &s )
    { freeHtml.append( s ); }
    void resetHtml( std::ofstream &os );
    
    void setJobError( unsigned int job_id, bool err);
    bool hasJobError( unsigned int job_id );
    int  getJobFileId( unsigned int job_id );
    
    void clear();
    
    void setProjectNodesDir( const std::string &dir );
    void htmlProjectNodes( BaseNode *node, FileManager &fileMan, int level = 0);
    
private:
    void htmlProjectNode( bool index, BaseNode *node, FileManager &fileMan, int level);
    void htmlProjectFile( bool index, file_id_t id, BaseNode *node, FileManager &fileMan);
    
private:
    static OutputCollector *theOutputCollector;
    
    JobStorer jobStorer;
    std::map<unsigned int,std::string> jobHtml;
    std::string freeHtml;
    
    std::string projectdir;
    
#ifdef  USE_CURSES
    CursesScreen *screen;
#endif
    
};

#endif
