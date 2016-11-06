#ifndef FERRET_TRAVERSE_STRUCTURE_H_
#define FERRET_TRAVERSE_STRUCTURE_H_

#include <map>
#include <string>

#include "project_xml_node.h"


class TraverseStructure
{
private:
    TraverseStructure();
    
public:
    TraverseStructure( FileManager &filesDb, ProjectXmlNode *xmlRootNode);
    
    void traverseXmlStructureForChildren();
    void traverseXmlStructureForDeletedFiles();
    void traverseXmlStructureForNewFiles();
    void traverseXmlStructureForNewIncdirFiles();
    void traverseXmlStructureForExtensions();
    void traverseXmlStructureForTargets();

    void reset()
    { dirTargetMap.clear(); }

    int getCntRemoved() const
    { return cntRemoved; }

    int getCntNew() const
    { return cntNew; }
    
private:
    typedef enum { FOR_DELETED_FILES, FOR_NEW_FILES, FOR_NEW_INCDIR_FILES, FOR_EXTENSIONS, FOR_TARGETS} for_what_t;
    void traverser( for_what_t for_what, ProjectXmlNode *node, int level);

private:
    std::map<std::string,bool> dirTargetMap;
    FileManager  &filesDb;
    ProjectXmlNode *rootNode;
    int cntRemoved ;
    int cntNew;
};


#endif
