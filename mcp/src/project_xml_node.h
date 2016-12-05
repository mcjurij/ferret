#ifndef FERRET_PROJECT_XML_NODE_H_
#define FERRET_PROJECT_XML_NODE_H_

#include <string>
#include <vector>
#include <map>
#include <set>

#include "parse_dep.h"
#include "find_files.h"
#include "engine.h"
#include "file_manager.h"
#include "include_manager.h"
#include "build_props.h"


// -----------------------------------------------------------------------------
// XML traversal
// -----------------------------------------------------------------------------

struct Extension
{
    std::string name;
    std::map<std::string,std::string> attribs;

    Extension( const std::string &name, const std::map<std::string,std::string> &attr)
        : name(name), attribs(attr)
    {}
};


class ProjectXmlNode {
    
public:
    std::vector<std::string> incdirs;        // from XML
    std::vector<std::string> subDirs;
    std::vector<ProjectXmlNode *> childNodes;
    std::vector<std::string> workingSubDirs;

    std::vector<std::string> allSubDirs;
    
    std::vector<std::string> searchIncDirs;    // include search paths

    std::vector<std::string> objs;             // .o files for linker
    std::vector<std::string> libs;             // libraries for linker
    std::map<std::string,std::string> libsMap; // store directories of libs
    std::vector<std::string> searchLibDirs;    // libraries search paths
    
public:
    ProjectXmlNode( const std::string &d, const std::string &module, const std::string &name,
                    const std::string &target, const std::string &type,
                    const std::vector<std::string> &cppflags, const std::vector<std::string> &cflags,
                    const std::vector<std::string> &usetools);
    
    static ProjectXmlNode *traverseXml( const std::string &start, int level=0);
    static bool hasXmlErrors();
    std::vector<std::string> traverseXmlStructureForChildren( int level=0 );
    
    void assignBuildProperties();
    
    std::string getDir() const
    { return dir; }
    
    std::string getSrcDir() const
    { return srcdir; }
    
    std::string getModule() const
    { return module; }
    std::string getName() const
    { return name; }
   
    std::string getTarget() const
    { return target; }
    
    std::string getType() const
    { return type; }

    std::string getObjDir();
    
    std::string getBinDir();
    
    std::string getLibDir();
    
    void traverseSrc();

    std::vector<File> getFiles() const;
    
    void traverseIncdir( const std::string &incdir );
    
    //std::vector<File> getIncdirFiles() const;
    
    void addDatabaseFile( const std::string &fn )
    { databaseFiles.push_back( fn ); }
    
    void addFileId( file_id_t id )
    { fileIds.insert( id ); }
    
    void removeFileId( file_id_t id )
    { fileIds.erase( id ); }
    
public:
    int  manageDeletedFiles( FileManager &fileMan );
    int  checkForNewFiles( FileManager &fileMan );
    int  checkForNewIncdirFiles( FileManager &fileMan );
    
    void readExistingDepFiles();
    void manageChangedDependencies( FileManager &fileMan, std::set<file_id_t> &blockedIds);
    void createCommandArguments( const std::string &compileMode );
    void createCommands( FileManager &fileMan );
    void pegCppScript( file_id_t cpp_id, const std::string &in, const std::string &out, const std::string &compileMode);
    void pegCScript( file_id_t cpp_id, const std::string &in, const std::string &out, const std::string &compileMode);
    bool doDeep();   // wether to follow library from this node, via downward pointers, to all downward nodes or not
    
    //file_id_t getTargetFileId() const
    //{ return target_file_id; }
    
    std::string getNodeName() const
    { return nodeName; }
    
    static ProjectXmlNode *getNodeByName( const std::string &name );

    void addExtensions( const std::vector<Extension> &ext );

    void setExtensionUsed( const std::string &ext, const std::string &fileName);  // prevent using the same extension twice for same file
    bool isExtensionUsed( const std::string &ext, const std::string &fileName);
    void createExtensionCommands( FileManager &fileMan );
    
private:
    std::set<file_id_t> blockedIds;
    
public:
    //void addExtensionSourceId( file_id_t id );
    void setReplMapForId( const std::map<file_id_t,std::map<std::string,std::string> > &re);
    void setReplKeyValueForId( file_id_t id, const std::string &key, const std::string &val);
    void setReplKeyValueForId( file_id_t id, const std::string &key, int val);
    std::map<std::string,std::string> getReplMapForId( file_id_t id );
    void        setExtensionScriptTemplNameForId( file_id_t id, const std::string &scriptTempl, const std::string &script);
    std::string getExtensionScriptTemplNameForId( file_id_t id );
    std::string getExtensionScriptNameForId( file_id_t id );

    std::string getScriptTarget() const
    { return scriptTarget; }
    
private:
    static bool xmlHasErrors;
    std::string dir;
    std::string srcdir;
    FindFiles nodeFiles, incdirFiles;
    std::vector<file_id_t>   sourceIds;
    std::set<file_id_t>      fileIds;
    std::string target, type;          // from XML
    std::vector<std::string> cppflags;
    std::vector<std::string> cflags;
    std::vector<std::string> databaseFiles;  // files and commands in the ferret file database
    
    std::string              module;   // from XML
    std::string              name;     // from XML
    std::string              nodeName;
    
    std::string              includesArg;
    std::string              pltfIncArg;
    std::string              toolIncArg;         // from usetool
    std::string              cppflagsArg;
    std::string              cflagsArg;
    std::string              lflagsArg;
    std::string              aflagsArg;          // flags for static lib (ar), currently unused
    std::string              eflagsArg;
    std::string              pltfLibArg;
    std::string              pltfLibDirArg;
    std::string              toolLibArg;         // from usetool
    std::string              toolLibDirArg;      // from usetool

    PrjBuildProps  prjBuildProps;     // from build_<machine>.properties file
    bool madeObjDir, madeBinDir, madeLibDir;
    
    std::vector<std::string> usetools;
    std::vector<Extension>   extensions;
    std::map<std::string,std::string> extensionUsed;
    
    std::map<file_id_t,std::map<std::string,std::string> > replacements;
    std::map<file_id_t,std::string>  extensionScriptTemplNames;
    std::map<file_id_t,std::string>  extensionScriptNames;
    
    file_id_t   target_file_id;
    std::string scriptTarget;   // the "human readable" form of the target, to be displayed in the echo in the scripts
};

#endif
