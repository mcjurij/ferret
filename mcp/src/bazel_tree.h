#ifndef FERRET_BAZEL_TREE_H_
#define FERRET_BAZEL_TREE_H_

#include <string>
#include <map>


class BazelNode;

class BazelTree {
    BazelTree();
    
public:
    static BazelTree *getTheBazelTree();

    BazelNode *traverse( const std::string &start );

    std::string getBaseDir() const
    { return baseDir; }
    
    std::string convertBazelToFs( const std::string &p );
    
private:
    std::string findWorkspaceBase();
       
    BazelNode *traverseBUILD( const std::string &start, int level);
    
private:
    static BazelTree *theBazelTree;
    
    std::string cwd, bzlCwd, baseDir;
    std::map<std::string,BazelNode *> bazelPathToNodeMap;
};


#endif
