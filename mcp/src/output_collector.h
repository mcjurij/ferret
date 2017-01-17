#ifndef FERRET_OUTPUT_COLLECTOR_H_
#define FERRET_OUTPUT_COLLECTOR_H_

#include <vector>
#include <map>
#include <sstream>
#include <fstream>
#include <cassert>

#include "file_manager.h"
#include "curses_screen.h"

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

#ifdef  USE_CURSES
    void cursesAppend( const std::string &s );
    void cursesEnd();
    void appendTo( std::vector<std::string> &lines, int from);
    int  cursesNumLines() const
    { return (int)cursesLines.size(); }
#endif
    
private:
    int file_id;
    std::string o, so, soh, eo, eoh;
    bool error;
    
#ifdef  USE_CURSES
    std::vector<std::string> cursesLines;
    std::string cursesLast;
    int  startY;
#endif
};


class OutputCollector {

private:    
    OutputCollector();

public:
    static OutputCollector *getTheOutputCollector();
    
    void cursesEnable( unsigned int parallel );
    void cursesDisable();
    
    std::map<int,OutputEntry>::iterator getEntry( int file_id );
    
    void append( int file_id, const std::string &s);
    void appendStd( int file_id, const std::string &s);
    void appendStdHtml( int file_id, const std::string &s);
    void appendErr( int file_id, const std::string &s);
    void appendErrHtml( int file_id, const std::string &s);
    void cursesAppend( int file_id, const std::string &s);
    void cursesEnd( int file_id );
    
    void cursesSetTopLine( int y, const std::string &l);
#ifdef  USE_CURSES
    std::vector<std::string> cursesTop( int last );
    std::vector<std::string> cursesBottom( int last );
#endif
    
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
    void clear();
    
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
    
#ifdef  USE_CURSES
    CursesScreen *screen;
    std::vector<std::string> topLines;
#endif
    
};

#endif
