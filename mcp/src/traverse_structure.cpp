
#include "traverse_structure.h"

using namespace std;


TraverseStructure::TraverseStructure( FileManager &filesDb, ProjectXmlNode *xmlRootNode)
    : filesDb(filesDb), rootNode(xmlRootNode), cntRemoved(0), cntNew(0)
{
}


void TraverseStructure::traverseXmlStructureForChildren()
{
    rootNode->traverseXmlStructureForChildren();
}


void TraverseStructure::traverseXmlStructureForDeletedFiles()
{
    reset();
    traverser( FOR_DELETED_FILES, rootNode, 0);
}


void TraverseStructure::traverseXmlStructureForNewFiles()
{
    reset();
    traverser( FOR_NEW_FILES, rootNode, 0);
}


void TraverseStructure::traverseXmlStructureForNewIncdirFiles()
{
    reset();
    traverser( FOR_NEW_INCDIR_FILES, rootNode, 0);
}


void TraverseStructure::traverseXmlStructureForExtensions()
{
    reset();
    traverser( FOR_EXTENSIONS, rootNode, 0);
}


void TraverseStructure::traverseXmlStructureForTargets()
{
    reset();
    traverser( FOR_TARGETS, rootNode, 0);
}


void TraverseStructure::traverser( for_what_t for_what, ProjectXmlNode *node, int level)
{
    map<string,bool>::const_iterator it = dirTargetMap.find( node->getDir() );
    if( it != dirTargetMap.end() )
        return;
    
    for( size_t i = 0; i < node->childNodes.size(); i++)
    {
        ProjectXmlNode *d = node->childNodes[ i ];
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
