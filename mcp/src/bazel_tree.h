#ifndef FERRET_BAZEL_TREE_H_
#define FERRET_BAZEL_TREE_H_

#include <string>


class BazelTree {

public:
    BazelTree();

    std::string findWorkspaceBase();
    
private:
    std::string cwd, bzlCwd;
};


#endif
