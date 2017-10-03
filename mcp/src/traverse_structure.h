#ifndef FERRET_TRAVERSE_STRUCTURE_H_
#define FERRET_TRAVERSE_STRUCTURE_H_

#include <map>
#include <string>

#include "project_xml_node.h"  // complete type needed for template instantiation
#include "bazel_node.h"

class FileManager;

template<class NodeT>
class TraverseStructure
{
private:
    TraverseStructure();
    
public:
    TraverseStructure( FileManager &filesDb, NodeT *xmlRootNode);
    
    void traverseStructureForChildren();
    void traverseStructureForDeletedFiles();
    void traverseStructureForNewFiles();
    void traverseStructureForNewIncdirFiles();
    void traverseStructureForExtensions();
    void traverseStructureForTargets();
    
    void reset()
    { dirTargetMap.clear(); }
    
    int getCntRemoved() const
    { return cntRemoved; }
    
    int getCntNew() const
    { return cntNew; }
    
private:
    typedef enum { FOR_DELETED_FILES, FOR_NEW_FILES, FOR_NEW_INCDIR_FILES, FOR_EXTENSIONS, FOR_TARGETS} for_what_t;
    void traverser( for_what_t for_what, NodeT *node, int level);

private:
    std::map<std::string,bool> dirTargetMap;
    FileManager  &filesDb;
    NodeT *rootNode;
    int cntRemoved;
    int cntNew;
};


template class TraverseStructure<ProjectXmlNode>;
template class TraverseStructure<BazelNode>;

#endif
