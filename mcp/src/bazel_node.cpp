
#include <iostream>
#include <sstream>
#include <fstream>
#include <cassert>

#include "platform_spec.h"
#include "glob_utility.h"
#include "extension.h"
#include "engine.h"
#include "platform_defines.h"
#include "script_template.h"
#include "bazel_node.h"
#include "mini_bazel_parser.h"

using namespace std;



BazelNode::BazelNode( const string &d )
    : BaseNode( d, "BAZEL")
{
}


static map<string,BazelNode *> dirToNodeMap;
static string nodeStack[50];

BazelNode *BazelNode::traverseBUILD( const string &start, int level)
{
    // if( xmlHasErrors )
    //    return 0;
    
    if( level >= 50 )
    {
        cerr << "error: BUILD structure too deep.\n";
        //xmlHasErrors = true;
        return 0;
    }
    
    for( int p = 0; p < level; p++)
    {
        if( nodeStack[ p ] == start )
        {
            cerr << "error: BUILD structure contains a cycle. visited " << start << " already. via:\n";
            for( int k = p; k < level; k++)
                cerr << "       " << nodeStack[ k ] << "\n";
            // xmlHasErrors = true;
            return 0;
        }
    }
    
    map<string,BazelNode *>::const_iterator it = dirToNodeMap.find( start );
    if( it != dirToNodeMap.end() )
        return it->second;
    
    nodeStack[ level ] = start;
    
    string buildfile = start + "/BUILD";
    ifstream fp( buildfile );
    stringstream buffer;
    if( fp )
    {
        buffer << fp.rdbuf();  // slurp file in 

        fp.close();
    }
    else
    {
        cerr << "error: BUILD structure file " << buildfile << " not found\n";
        return 0;
    }
    
    
    MiniBazelParser parser( buffer.str(), start);
    BazelNode *node = 0;
    
    parser.parse();
    
    node = parser.getStartNode();
    
    dirToNodeMap[ start ] = node;
    return node;
}


static map<string,vector<string> > subsMap;
vector<string> BazelNode::traverseStructureForChildren( int level )
{
    map<string,vector<string> >::iterator it = subsMap.find( getDir() );
    if( it != subsMap.end() )
        return it->second;
    
    assignBuildProperties();
    
    for( size_t i = 0; i < childNodes.size(); i++)   // collect all deps of this node
    {
        BazelNode *d = childNodes[ childNodes.size() - 1 - i ];
        vector<string> subs = d->traverseStructureForChildren( level + 1 );
        
        vector<string>::iterator sit = subs.begin();
        for( ; sit != subs.end(); sit++)
            allSubDirs.push_back( *sit );
    }
    
    allSubDirs.push_back( getDir() );

    // FIXME ....missing 
    subsMap[ getDir() ] = allSubDirs;
    
    return allSubDirs;
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
    
