#ifndef FERRET_OUTPUT_COLLECTOR_H_
#define FERRET_OUTPUT_COLLECTOR_H_

#include <vector>
#include <map>
#include <sstream>
#include <fstream>
#include <cassert>

#include "file_manager.h"

class ProjectXmlNode;


class OutputEntry {

public: 
    OutputEntry()
        : file_id( -1 ), error(false)
    {}
    
    OutputEntry( int file_id )
        : file_id( file_id ), error(false)
    {}

    ~OutputEntry()
    {}

    void append( const std::string &s )
    { o.append( s ); }
    
    void appendStd( const std::string &s )
    { so.append( s ); }
    void appendStdHtml( const std::string &s )
    { soh.append( s ); }
    
    void appendErr( const std::string &s )
    { eo.append( s ); }
    void appendErrHtml( const std::string &s )
    { eoh.append( s ); }
    
    std::string getOutput() const;
    std::string getStdOut() const;
    std::string getStdOutHtml() const;
    std::string getErrOut() const;
    std::string getErrOutHtml() const;
    
    void setError( bool err );
    bool hasError() const
    { return error; }
    
private:
    int file_id;
    std::string o, so, soh, eo, eoh;
    bool error;
};


class OutputCollector {

private:    
    OutputCollector()
    {}

public:
    static OutputCollector *getTheOutputCollector();
    
    void append( int file_id, const std::string &s);
    void appendStd( int file_id, const std::string &s);
    void appendStdHtml( int file_id, const std::string &s);
    void appendErr( int file_id, const std::string &s);
    void appendErrHtml( int file_id, const std::string &s);
    
    std::string getOutput( int file_id ) const;
    std::string getOutputHtml( int file_id ) const;
    std::string getStdOut( int file_id ) const;
    std::string getStdOutHtml( int file_id ) const;
    std::string getErrOut( int file_id ) const;
    std::string getErrOutHtml( int file_id ) const;
    
    void text( std::ofstream &os );
    void html( std::ofstream &os );
    void resetHtml( std::ofstream &os );

    void setError( int file_id, bool err);

    void setProjectNodesDir( const std::string &dir );
    void htmlProjectNodes( ProjectXmlNode *node, FileManager &fileMan, int level = 0);

private:
    void htmlProjectNode( bool index, ProjectXmlNode *node, FileManager &fileMan, int level);
    void htmlProjectFile( bool index, file_id_t id, ProjectXmlNode *node, FileManager &fileMan);

private:
    static OutputCollector *theOutputCollector;

    std::vector<int> file_ids;
    std::map<int,OutputEntry>  entryMap;

    std::string projectdir;
};


#endif
