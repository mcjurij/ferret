#include <unistd.h>
#include <sys/stat.h>
#include <iostream>
#include <sstream>
#include <fstream>

#include "find_files.h"
#include "glob_utility.h"
#include "bazel_tree.h"
#include "bazel_node.h"
#include "mini_bazel_parser.h"

using namespace std;


BazelTree *BazelTree::theBazelTree = 0;

BazelTree::BazelTree()
{
    baseDir = findWorkspaceBase();
}


BazelTree *BazelTree::getTheBazelTree()
{
    if( theBazelTree == 0 )
        theBazelTree = new BazelTree;

    return theBazelTree;
}


BazelNode *BazelTree::traverse( const string &start )
{
    return traverseBUILD( start, 0);
}

    
static string appendParts( const vector<string> &v )
{
    string s;
    for( size_t i = 0; i < v.size(); i++)
        s += '/' + v[i];
    return s;
}


string BazelTree::findWorkspaceBase()
{
    struct stat fst;
    int k=0;
    bool found = false;
    
    string dir, help;
    char  ccwd[1025];
    getcwd( ccwd, 1024);
    cwd = ccwd;
    vector<string> cwd_parts = split( '/', cwd);
    
    for( k = 0; k < 8 && cwd_parts.size() > 0; k++)
    {
        dir = appendParts( cwd_parts );
        string path = dir + "/WORKSPACE";
        cout << "looking for " << path << "\n";
        
        if( stat( path.c_str(), &fst) == 0 )
        {
            if( S_ISREG( fst.st_mode ) )
            {
                found = true;
                if( k > 0 )
                    cout << "Using the WORKSPACE anchor from " << path << "\n"; 
                break;
            }
        }
        
        help = cwd_parts.back() + ((k > 0) ? "/" + help : "");
        cwd_parts.pop_back();
    }
    
    if( found )
    {
        bzlCwd = help;
        return dir;
    }
    else
        return  "";
}


static map<string,BazelNode *> dirToNodeMap;
static string nodeStack[50];

BazelNode *BazelTree::traverseBUILD( const string &start, int level)
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

    for( node = startNode; node; node = node->getSibling())
    {
        bazelPathToNodeMap[ node->getNodePath() ] = node;
        cout << "bazel   new path = " << node->getNodePath() << "\n";
    }
    
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

            if( dep.length() > 2  && dep[0] == '/' && dep[1] == '/' )
            {
                dep = dep.substr(2);                             // FIXME for now we remove //
                cout  << "bazel   using " << dep << " without // in front\n";
            }
            
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

                traverseBUILD( dir, level+1);
                nit = bazelPathToNodeMap.find( dep );
                
                if( nit != bazelPathToNodeMap.end() )
                {
                    cout << "bazel   found dep from " << node->getNodePath() << " to " << nit->second->getNodePath() << " (in dir " << dir << ")\n";

                    if( nit->second->checkVisibility( node ) )
                        node->addChildNode( nit->second );
                    else
                        cout << "bazel   found dep from " << node->getNodePath() << " to " << nit->second->getNodePath() << " node is not visible!\n";
                }
            }
        }
    }
    
    dirToNodeMap[ start ] = startNode;
    
    return startNode;
}
