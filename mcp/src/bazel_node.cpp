
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

static map<string,BazelNode *> bazelPathToNodeMap;


BazelNode::BazelNode( const string &d )
    : BaseNode( d, "BAZEL")
{
}


void BazelNode::setName( const string &n )
{
    name = n;
    nodeName = module + "*" + name;
    nameToNodeMap[ nodeName ] = this;

    bazelPath = getDir() + ':' + name;
    bazelPathToNodeMap[ bazelPath ] = this;
    cout << "bazel   new path = " << bazelPath << "\n";
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
    BazelNode *node, *startNode;
    
    parser.parse();

    startNode = parser.getStartNode();
    // for( node = startNode; node; node = node->getSibling())
    //{
    //    bazelPathToNodeMap[ start + ":" + node->getName() ] = node;
    //}
    
    for( node = startNode; node; node = node->getSibling())
    {
        const vector<string> &deps = node->getDeps();
        vector<string>::const_iterator dit = deps.begin();

        for( ; dit != deps.end(); dit++)
        {
            string dep = *dit;

            if( dep.length() == 0 )
                continue;
            
            if( dep[0] == ':' )
                dep = start + dep; 

            map<string,BazelNode *>::const_iterator nit = bazelPathToNodeMap.find( dep );
            if( nit != bazelPathToNodeMap.end() )
            {
                cout << "bazel   found dep from " << node->getNodePath() << " to " << nit->second->getNodePath() << "\n";
                node->addChildNode( nit->second );
            }
            else
            {
                string dir, name;
                breakBazelDep( dep, dir, name);

                BazelNode *subNode, *subStartNode;

                subStartNode = traverseBUILD( dir, level+1);
                nit = bazelPathToNodeMap.find( dep );
                
                if( nit != bazelPathToNodeMap.end() )
                {
                    cout << "bazel   found dep from " << node->getNodePath() << " to " << nit->second->getNodePath() << " (in dir " << dir << ")\n";
                    node->addChildNode( nit->second );
                }
            }
        }
    }
    
    dirToNodeMap[ start ] = startNode;
    
    return startNode;
}


static map<string,vector<string> > subsMap;
vector<string> BazelNode::traverseStructureForChildren( int level )
{
    vector<string> r;
    assignBuildProperties();
    
    // for( size_t i = 0; i < childNodes.size(); i++)   // collect all deps of this node
    // {
    //     BazelNode *d = childNodes[ childNodes.size() - 1 - i ];
    //     vector<string> subs = d->traverseStructureForChildren( level + 1 );
        
    //     vector<string>::iterator sit = subs.begin();
    //     for( ; sit != subs.end(); sit++)
    //         allSubDirs.push_back( *sit );
    // }

    for( size_t i = 0; i < childNodes.size(); i++)
    {
        BazelNode *dep = childNodes[i];
        if( dep->getType() == "library" || dep->getType() == "staticlib"  || dep->getType() == "static" )
        {
            string t = dep->getTarget();
            libs.push_back( t );
            
            libsMap[ t ] = dep->getLibDir();
            searchLibDirs.push_back( dep->getLibDir() );
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
