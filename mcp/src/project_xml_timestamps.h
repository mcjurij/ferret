#ifndef FERRET_PROJECT_XML_TIMESTAMPS_H_
#define FERRET_PROJECT_XML_TIMESTAMPS_H_

#include <string>
#include <list>

#include "find_files.h"

// detect modified XML files
// as a single modified project XML file can affect the entire build (same for platform XML)
class ProjectXmlTimestamps {
    
public:
    ProjectXmlTimestamps( const std::string &dbProjDir )
        : dbProjDir(dbProjDir)
    {}
    
    void addFile( const std::string &path );
    void writeTimes();
    
    bool readTimes();
    bool compare();
    
private:
    std::string dbProjDir;
    
    std::list<File> files;                                           // used when writing
    std::list< std::pair< std::string, long long> > compare_files;   // used when reading
};

#endif
