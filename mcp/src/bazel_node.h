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
    
    static BazelNode *traverseBUILD( const std::string &start, int level=0);
    
    void traverseStructureForChildren();
    std::vector<BazelNode *> childNodes;

    void setSibling( BazelNode *n )
    { sibling = n; }

    void setSrcs( const std::vector<std::string> &l );
    void setHdrs( const std::vector<std::string> &l );
    void setDeps( const std::vector<std::string> &l );
    
private:
    BazelNode *sibling;
    std::vector<std::string>  srcs, hdrs, deps;   // from BUILD file
};

#endif
