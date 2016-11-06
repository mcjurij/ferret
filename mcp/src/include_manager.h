#ifndef FERRET_INCLUDE_MANAGER_H_
#define FERRET_INCLUDE_MANAGER_H_

#include <string>
#include <vector>
#include <set>
#include <map>

#include "file_manager.h"
#include "parse_dep.h"


class Seeker {
public:
    std::string  from;                       // file that seeks
    std::vector<std::string> searchIncDirs;  // include dirs from XML node
    std::vector<std::string> lookingFor;     // includes being looked for  
};


// The include manager takes care of all dependencies coming from includes
class IncludeManager
{
    
public:
    static IncludeManager *getTheIncludeManager();
    
    IncludeManager()
        : fileRemoved(false), ignHdrFp(0)
    {}

    //void announceNewFile( const NewFile &nf );
    
    void createMissingDepFiles( FileManager &fileDb, Executor &executor, bool printTimes);
    
    void resolve( FileManager &fileDb, bool initMode, bool writeIgnHdr);
    
    void addBlockedId( file_id_t id );
    void addBlockedIds( const std::set<file_id_t> &blids );

    void removeFile( const std::string &fn, const ProjectXmlNode *xmlNode);
    
private:
    int readDepFile( const std::string &fn, const std::string &depfn, const ProjectXmlNode *xmlNode);
    bool readDepFiles( FileManager &fileDb, bool initMode, bool writeIgnHdr);
    void addSeeker( const std::string &from, const std::string &localDir, const std::vector<std::string> &searchIncDirs,
                    const std::vector<std::string> &lookingFor);
    
    bool resolveFile( FileManager &fileDb, file_id_t from_id, const Seeker &s, bool writeIgnHdr);
    
private:
    static IncludeManager *theIncludeManager;
    DepEngine depEngine;

    std::vector<std::string> filesWithUpdate, depFilesWithUpdate;
    bool fileRemoved;
    
    std::map<std::string,Seeker> seekerMap;

    std::set<file_id_t> blockedIds;

    FILE *ignHdrFp;
};

#endif
