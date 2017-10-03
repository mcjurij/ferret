
#include "traverse_structure.h"
#include "file_manager.h"

using namespace std;

template<class NodeT>
TraverseStructure<NodeT>::TraverseStructure( FileManager &filesDb, NodeT *xmlRootNode)
    : filesDb(filesDb), rootNode(xmlRootNode), cntRemoved(0), cntNew(0)
{
}


template<class NodeT>
void TraverseStructure<NodeT>::traverseStructureForChildren()
{
    rootNode->traverseStructureForChildren();
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
    map<string,bool>::const_iterator it = dirTargetMap.find( node->getDir() );
    if( it != dirTargetMap.end() )
        return;
    
    for( size_t i = 0; i < node->childNodes.size(); i++)
    {
        NodeT *d = node->childNodes[ i ];
        traverser( for_what, d, level + 1);
    }

    switch( for_what )
    {
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

    dirTargetMap[ node->getDir() ] = true;
}
