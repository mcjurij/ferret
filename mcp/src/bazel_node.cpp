
#include <iostream>
#include <cassert>

#include "platform_spec.h"
#include "glob_utility.h"
#include "extension.h"
#include "engine.h"
#include "platform_defines.h"
#include "script_template.h"
#include "bazel_node.h"

using namespace std;


BazelNode::BazelNode( const string &d )
    : BaseNode( d, "BAZEL"), vis_state(V_PRIVATE)
{
}


void BazelNode::setName( const string &n )
{
    name = n;
    nodeName = module + "*" + name;
    nameToNodeMap[ nodeName ] = this;

    bazelPath = getPackage() + ':' + name;
}


vector<string> BazelNode::traverseStructureForChildren( int level )
{
    vector<string> r;
    assignBuildProperties();
    
    for( size_t i = 0; i < childNodes.size(); i++)
    {
        BazelNode *dep = childNodes[i];

        closureNodes.push_back( dep );

        if( dep->closureNodes.size() > 0 )
            closureNodes.insert( closureNodes.end(), dep->closureNodes.begin(), dep->closureNodes.end());
    }
    
    
    for( size_t i = 0; i < closureNodes.size(); i++)
    {
        BazelNode *sub = closureNodes[i];
        string subsrcdir = sub->getDir();
        searchIncDirs.push_back( subsrcdir );

        if( sub->getType() == "library" /* || sub->getType() == "staticlib"  || sub->getType() == "static" */ )
        {
            string t = sub->getTarget();
            libs.push_back( t );
            
            libsMap[ t ] = sub->getLibDir();
            searchLibDirs.push_back( sub->getLibDir() );
        }
    }
    // FIXME ....missing 
    
    return r;
}


void BazelNode::setSrcs( const vector<string> &l )
{
    srcs = l;
}


void BazelNode::setHdrs( const vector<string> &l )
{
    hdrs = l;
}


void BazelNode::setDeps( const vector<string> &l )
{
    deps = l;
}


void BazelNode::setVisibility( const vector<string> &v )
{
    visibility = v;

    vis_state = V_OTHER;
    for( size_t i = 0; i < visibility.size(); i++)
    {
        string v = visibility[i];
        
        string dir, nodeName;
        breakBazelDep( v, dir, nodeName);
        
        if( dir == "//visibility" )
        {
            if( nodeName == "public" )
                vis_state = V_PUBLIC;
            else if( nodeName == "private" )
                vis_state = V_PRIVATE;
        }
    }
}


int BazelNode::checkForNewFiles( FileManager &fileMan )
{
    size_t i;
    int cnt = 0;

    vector<string> allSrcs = srcs;
    allSrcs.insert( allSrcs.end(), hdrs.begin(), hdrs.end());
    
    vector<File> sources = nodeFiles.getSourceFiles( allSrcs );
    
    for( i=0; i < sources.size(); i++)
    {
        const File &f = sources[i];
        
        if( !fileMan.hasFileName( f.getPath() ) )   // already in files db?
        {
            sourceIds.push_back( fileMan.addNewFile( f.getPath(), this) );
            cnt++;
        }
        else
            sourceIds.push_back( fileMan.getIdForFile( f.getPath() ) );
    }

    return cnt;
}


bool BazelNode::checkVisibility( BazelNode *fromNode )
{
    assert( fromNode );

    if( vis_state == V_PUBLIC )
        return true;
    else if( vis_state == V_PRIVATE )
    {
        return getPackage() == fromNode->getPackage();   // same directory same package FIXME?
    }
    else
    {
        for( size_t i = 0; i < visibility.size(); i++)
        {
            string v = visibility[i];

            if( v.length() > 2  && v[0] == '/' && v[1] == '/' )
                v = v.substr(2);                             // FIXME for now we remove //
            
            string pkg, nodeName;
            breakBazelDep( v, pkg, nodeName);

            if( nodeName == "__pkg__" && fromNode->getPackage() == pkg )
                return true;
            else if( pkg == fromNode->getPackage() && nodeName == fromNode->getName() )
                return true;
        }        
    }

    return false;
}
