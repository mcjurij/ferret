
#include "traverse_structure.h"
#include "file_manager.h"

using namespace std;

template<class NodeT>
TraverseStructure<NodeT>::TraverseStructure( FileManager &filesDb, NodeT *rootNode, bool bazelMode)
    : filesDb(filesDb), rootNode(rootNode), bazelMode(bazelMode), cntRemoved(0), cntNew(0)
{
}


template<class NodeT>
void TraverseStructure<NodeT>::traverseStructureForChildren()
{
    if( !bazelMode )
        rootNode->traverseStructureForChildren();
    else
    {
        reset();
        traverser( FOR_CHILDREN, rootNode, 0);
    }
}


template<class NodeT>
void TraverseStructure<NodeT>::traverseStructureForDeletedFiles()
{
    reset();
    traverser( FOR_DELETED_FILES, rootNode, 0);
}


template<class NodeT>
void TraverseStructure<NodeT>::traverseStructureForNewFiles()
{
    reset();
    traverser( FOR_NEW_FILES, rootNode, 0);
}


template<class NodeT>
void TraverseStructure<NodeT>::traverseStructureForNewIncdirFiles()
{
    reset();
    traverser( FOR_NEW_INCDIR_FILES, rootNode, 0);
}


template<class NodeT>
void TraverseStructure<NodeT>::traverseStructureForExtensions()
{
    reset();
    traverser( FOR_EXTENSIONS, rootNode, 0);
}


template<class NodeT>
void TraverseStructure<NodeT>::traverseStructureForTargets()
{
    reset();
    traverser( FOR_TARGETS, rootNode, 0);
}


template<class NodeT>
void TraverseStructure<NodeT>::traverser( for_what_t for_what, NodeT *node, int level)
{   
    if( !bazelMode )
    {
        map<string,bool>::const_iterator it = dirTargetMap.find( node->getDir() );
        if( it != dirTargetMap.end() )
            return;

        vector<NodeT *> childNodes = node->getChildNodes();
        for( size_t i = 0; i < childNodes.size(); i++)
        {
            NodeT *d = childNodes[ i ];
            traverser( for_what, d, level + 1);
        }

        what( for_what, node);
        
        dirTargetMap[ node->getDir() ] = true;
    }
    else
    {
        map<string,bool>::const_iterator it = bazelTargetMap.find( node->getNodePath() );
        if( it != bazelTargetMap.end() )
            return;
        
        NodeT *sibling = node;
        do {
            vector<NodeT *> childNodes = sibling->getChildNodes();
            for( size_t i = 0; i < childNodes.size(); i++)
            {
                NodeT *child = childNodes[ i ];
                traverser( for_what, child, level + 1);
            }

            what( for_what, sibling);
            bazelTargetMap[ sibling->getNodePath() ] = true;
            
            sibling = sibling->getSibling();
        }
        while( sibling );
    }
}


template<class NodeT>
void TraverseStructure<NodeT>::what( for_what_t for_what, NodeT *node)
{
    switch( for_what )
    {
        case FOR_CHILDREN:         
            node->traverseStructureForChildren();             // called here only in bazel mode
            break;
        case FOR_DELETED_FILES:
            cntRemoved += node->manageDeletedFiles( filesDb );
            break;
        case FOR_NEW_FILES:
            cntNew += node->checkForNewFiles( filesDb );
            break;
        case FOR_NEW_INCDIR_FILES:
            cntNew += node->checkForNewIncdirFiles( filesDb );
            break;
        case FOR_EXTENSIONS:
            // setup arguments for compiler/linker (also usetool...)
            node->createCommandArguments( filesDb.getCompileMode() );
            
            // do these bevore other commands, so that blocked file ids are known
            node->createExtensionCommands( filesDb );
            break;
        case FOR_TARGETS:
            //  create all commands to produce target (if there is a target and a type)
            node->createCommands( filesDb );
            break;
    }
}
