#ifndef FERRET_FILE_MANAGER_H_
#define FERRET_FILE_MANAGER_H_

#include <string>
#include <vector>
#include <set>
#include <map>

#include "hash_set.h"
#include "engine.h"
#include "glob_utility.h"


class ProjectXmlNode;

class FileManager {
public:
    typedef enum { UNKNOWN, UNCHANGED, NEW, GONE, DEP_CHANGED, TOUCHED} state_t;
    
    file_id_t addFile( const std::string &fn, ProjectXmlNode *node);
    file_id_t addNewFile( const std::string &fn, ProjectXmlNode *node);
    bool removeFile( const std::string &fn );
    
    void addDependency( const std::string &from_fn, const std::string &to_fn);
    void addDependency( file_id_t from_id, const std::string &to_fn);
    void addDependency( file_id_t from_id, file_id_t to_id);
    //void removeDependencies( const std::string &fn );
    void addWeakDependency( file_id_t from_id, file_id_t to_id);
    bool hasBlockedDependency( file_id_t from_id, file_id_t to_id);
    
    bool hasNext( FileMap::Iterator &it ) const;
    
    bool compareAndReplaceDependencies( file_id_t id, hash_set_t *against);
    
    file_id_t addCommand( const std::string &fn, const std::string &cmd, ProjectXmlNode *node);
    file_id_t addExtensionCommand( const std::string &fn, const std::string &cmd, ProjectXmlNode *node);
    
    file_id_t getIdForFile( const std::string &fn );
    std::string getFileForId( file_id_t fid );
    std::string getCmdForId( file_id_t fid );
    
    bool hasFileName( const std::string &fn );

    bool isTargetCommand( file_id_t );
    std::set<file_id_t> getDependencies( file_id_t id );
    std::set<file_id_t> prerequisiteFor( file_id_t id );
    ProjectXmlNode *getXmlNodeFor( file_id_t id );
    
    void touchFile( const std::string &fn );
    //state_t getFileState( const std::string &fn );
    
    bool readDb( const std::string &projDir, bool ignoreXmlNodes = false);
    void writeDb( const std::string &projDir );
    void removeCycles();
    bool seeWhatsNewOrGone();
    void printWhatsChanged();
    void persistMarkByDeletions( hash_set_t *mbd_set );

    void setCompileMode( const std::string &cm );
    std::string getCompileMode() const;
    void setPropertiesFile( const std::string &pf );  // only allowed during init
    std::string getPropertiesFile() const;
    
    void sendToEngine( Engine &engine );
    void sendToMakefileEngine( MakefileEngine &engine );
    
    void print();
    
private:
    std::string compileMode;
    std::string propertiesFile;
};

#endif
