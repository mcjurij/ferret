#ifndef FERRET_FILE_CLEANER_H_
#define FERRET_FILE_CLEANER_H_

#include <string>
#include <list>
#include <set>

#include "file_manager.h"


class FileCleaner {

public:
    FileCleaner()
        : full(false), objClean(false), libexeClean(false), depth( 50 )
    {}
    
    bool readDb( const std::string &projDir );
    
    void clean( const std::set<std::string> &userTargets );

    void setDepth( int d )
    { if( d>=0 ) depth = d; }

    void setFullClean();
    void setObjClean();
    void setLibExeClean();
    
private:
    void traverseUserTargets( file_id_t id, int level = 0);
    void checkUserTargets( const std::set<std::string> &userTargets );
    
private:
    bool full, objClean, libexeClean;
    
    FileManager  filesdb;
    int depth;
    
    class Cmd {
    public:
        Cmd( const std::string &dt, const std::string &fn)
            : dep_type(dt), file_name(fn)
        {}
        
        std::string dep_type, file_name;
    };

    std::list<Cmd> cleanableFiles;   // all generated/intermediate files, i.e. all non-D command files
};

#endif
