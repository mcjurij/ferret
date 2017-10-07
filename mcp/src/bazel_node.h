#ifndef FERRET_BAZEL_TARGET_H_
#define FERRET_BAZEL_TARGET_H_

#include <string>
#include <vector>
#include <map>

#include "parse_dep.h"
#include "find_files.h"
#include "engine.h"
#include "file_manager.h"
#include "include_manager.h"
#include "build_props.h"
#include "base_node.h"


// -----------------------------------------------------------------------------
// class to hold a bazel target
// -----------------------------------------------------------------------------

class BazelNode : public BaseNode {
    
public:
    BazelNode( const std::string &d );
    virtual void setName( const std::string &n );
    
    static BazelNode *traverseBUILD( const std::string &start, int level=0);

    void addChildNode( BazelNode *n )
    { childNodes.push_back( n ); }
    
    std::vector<BazelNode *> getChildNodes() const
    { return childNodes; }

    std::string getNodePath() const
    { return bazelPath; }
    
    std::vector<std::string> traverseStructureForChildren( int level = 0 );
    void setSibling( BazelNode *n )
    { sibling = n; }
    virtual BazelNode *getSibling() const
    { return sibling; }
    
    void setSrcs( const std::vector<std::string> &l );
    void setHdrs( const std::vector<std::string> &l );
    void setDeps( const std::vector<std::string> &l );
    std::vector<std::string> getDeps() const
    { return deps; }
    
    int checkForNewFiles( FileManager &fileMan );
    
private:
    std::string bazelPath;    // dir + ':' + name
    BazelNode *sibling;
    std::vector<std::string>  srcs, hdrs, deps;   // from BUILD file
    std::vector<BazelNode *> childNodes;
};

#endif
