#include <unistd.h>
#include <sys/stat.h>
#include <iostream>

#include "find_files.h"
#include "glob_utility.h"
#include "bazel_tree.h"

using namespace std;


BazelTree::BazelTree()
{
    
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
