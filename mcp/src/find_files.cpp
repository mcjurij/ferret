// read file states and cache them

//#define _XOPEN_SOURCE 500
//#define _XOPEN_SOURCE 700
#include <ftw.h>  // probably a Linux thing only

#include <cstring>
#include <iostream>
#include <cstdio>
#include <cstdlib>

#include "find_files.h"
#include "glob_utility.h"

using namespace std;

static bool nano_diff_too_small( long a, long b)
{
    return labs( a-b ) < 100000000;  // 0.1s
}


bool File::isOlderThan( const File& other ) const
{
    if( last_modification.tv_sec < other.last_modification.tv_sec )
        return true;
    else
        if( last_modification.tv_sec == other.last_modification.tv_sec )
            if( !nano_diff_too_small( last_modification.tv_nsec, other.last_modification.tv_nsec) &&
                last_modification.tv_nsec < other.last_modification.tv_nsec )
                return true;
    return false;
}


bool File::isNewerThan( const File& other ) const
{
    if( last_modification.tv_sec > other.last_modification.tv_sec )
        return true;
    else
        if( last_modification.tv_sec == other.last_modification.tv_sec )
            if( !nano_diff_too_small( last_modification.tv_nsec, other.last_modification.tv_nsec) &&
                last_modification.tv_nsec > other.last_modification.tv_nsec )
                return true;
    return false;
}


string File::getDepFileName() const
{
    return getBasename() + ".d";
}


string File::extension() const
{
   const string &bn = getBasename();
   size_t pos = bn.rfind( "." );
   
   if( pos != string::npos )
       return bn.substr( pos );
   else
       return "";
}


string File::woExtension() const
{
   const string &bn = getBasename();
   size_t pos = bn.rfind( "." );
   
   if( pos != string::npos )
       return bn.substr( 0, pos);
   else
       return "";
}


long long File::getTimeMs() const
{
    long long s  = last_modification.tv_sec;
    long long ms = last_modification.tv_nsec / 1000000;

    return s*1000 + ms;
}


// -----------------------------------------------------------------------------
map<string,File> FindFiles::allFiles;
vector<File> FindFiles::ftwFiles;
set<string> FindFiles::noexistingFiles;


void FindFiles::traverse( const string &start_dir, bool deep) 
{
    int flags = 0;
    
    ftwFiles.clear();
    files.clear();

    if( deep )
    {
        if( nftw( start_dir.c_str(), FindFiles::handle_entry, 20, flags) == -1 ) // see http://linux.die.net/man/3/ftw
        {
            perror("nftw");
            cerr << "error: " + start_dir + " not found or not readable\n";
        }

        files = ftwFiles;
        ftwFiles.clear();
    }
    else
    {
        vector<File> dir_files = readDirectory( start_dir );

        files = dir_files;
        for( size_t i = 0; i < dir_files.size(); i++)
            allFiles[ dir_files[i].getPath() ] = dir_files[i];
    }
}


void FindFiles::appendTraverse( const string &start_dir, bool deep)
{
    int flags = 0;
    
    ftwFiles.clear();

    if( deep )
    {
        if( nftw( start_dir.c_str(), FindFiles::handle_entry, 20, flags) == -1 ) // see http://linux.die.net/man/3/ftw
        {
            cerr << "error: " + start_dir + " not found or not readable\n";
            perror("nftw");
        }

        files.reserve( files.size() + ftwFiles.size() );
        files.insert( files.end(), ftwFiles.begin(), ftwFiles.end());  
    
        ftwFiles.clear();
    }
    else
    {
        vector<File> dir_files = readDirectory( start_dir );
        
        files.reserve( files.size() + dir_files.size() );
        
        for( size_t i = 0; i < dir_files.size(); i++)
        {
            files.push_back( dir_files[i] );
            allFiles[ dir_files[i].getPath() ] = dir_files[i];
        }
    }
}


vector<File> FindFiles::getSourceFiles() const
{
    vector<File> sources;
    vector<File>::const_iterator it = files.begin();
    
    for( ; it != files.end(); it++)
    {
        const string &f = it->getBasename();
        size_t pos = f.rfind( "." );
        
        if( pos != string::npos )
        {
            string ext = f.substr( pos );
            
            if( ext == ".cpp" || ext == ".c" ||
                ext == ".h" || ext == ".hpp" || ext == ".C" )
                sources.push_back( *it );
        }
    }
    
    return sources;
}


vector<File> FindFiles::getHeaderFiles() const
{
    vector<File> sources;
    vector<File>::const_iterator it = files.begin();
    
    for( ; it != files.end(); it++)
    {
        const string &f = it->getBasename();
        size_t pos = f.rfind( "." );
        
        if( pos != string::npos )
        {
            string ext = f.substr( pos );
            
            if( ext == ".h" || ext == ".hpp" )
                sources.push_back( *it );
        }
    }
    
    return sources;
}


bool FindFiles::exists( const string &path )
{
    struct stat fst;

    if( allFiles.find( path ) != allFiles.end() )
        return true;
    else
    {
        if( noexistingFiles.find( path ) != noexistingFiles.end() )
            return false;
        
        if( stat( path.c_str(), &fst) == 0 )
        {
            if( S_ISREG( fst.st_mode ) )
            {
                string dir, bn;
                
                breakPath( path, dir, bn);
            
                allFiles[ path ] = File( path, dir, bn, &fst);
            }
            else
                allFiles[ path ] = File( path, &fst);
            
            return true;
        }
        else
        {
            noexistingFiles.insert( path );
            return false;
        }
    }
}


File FindFiles::getCachedFile( const string &path )
{
    map<string,File>::iterator it = allFiles.find( path );
    
    if( it != allFiles.end() )
        return it->second;
    else
    {
        cerr << "internal error: file '" << path << "' requested without prior exists check.\n";
        return File();
    }
}


File FindFiles::getUncachedFile( const string &path )
{
    struct stat fst;
    File f;
    
    if( stat( path.c_str(), &fst) == 0 )
    {
        if( S_ISREG( fst.st_mode ) )
        {
            string dir, bn;
            
            breakPath( path, dir, bn);
            
            f = File( path, dir, bn, &fst);
        }
    }
    
    return f;
}


bool FindFiles::existsUncached( const string &path )
{
    struct stat fst;
    if( stat( path.c_str(), &fst) == 0 )
    {
        if( S_ISREG( fst.st_mode ) )
            return true;
        else
            return false;
    }

    return false;
}


void FindFiles::clearCache()
{
    allFiles.clear();
    noexistingFiles.clear();
}


int FindFiles::handle_entry( const char *fpath, const struct stat *sb,
                             int tflag, struct FTW *ftwbuf)
{
    if( tflag == FTW_F )
    {
        const char *bn = fpath + ftwbuf->base;
        if( bn[0] == 0 || bn[0] == '.' )
            return 0;

        char d[1024];
        strncpy( d, fpath, ftwbuf->base);
        d[ftwbuf->base]=0;
        if( ftwbuf->base>0 && d[ftwbuf->base-1]=='/' )
            d[ftwbuf->base-1]=0;

        ftwFiles.push_back( File( fpath, d, bn, sb) );
        allFiles[ fpath ] = File( fpath, d, bn, sb);
    }
    
    return 0;           // tell nftw() to continue
}


vector<File> FindFiles::readDirectory( const string &dir )
{
    DIR *dirp = opendir( dir.c_str() );
    struct dirent *dp;
    vector<File> dir_files;
    
    if( dirp == 0 )
    {
        perror("opendir");
        cerr << "error: " + dir + " not found or not readable\n";
        return dir_files;
    }
    
    do
    {
        errno = 0;
        if( (dp = readdir(dirp)) != 0 )
        {
            if( dp->d_name[0] == 0 || dp->d_name[0] == '.' || dp->d_type != DT_REG )
                continue;
            
            struct stat fst;
            
            string bn = dp->d_name;
            string fpath = dir + "/" + bn;
            
            if( stat( fpath.c_str(), &fst) == 0 )
                dir_files.push_back( File( fpath, dir, bn, &fst) );
            else
                cerr << "error: can't stat '" << fpath << "'\n";
        }
    }
    while( dp != 0 );
    
    if (errno != 0)
        perror("readdir: error reading directory");
    closedir(dirp);

    return dir_files;
}


// ----------------------------------------------------------------------------
QueryFiles::QueryFiles()
{
}

bool QueryFiles::exists( const string &path )
{
    if( path.size() == 0 )
        return false;
    
    File f = FindFiles::getUncachedFile( path );
    if( f.getPath() != "" )
        return true;
    
    return false;
}


File QueryFiles::getFile( const string &path )
{
    return FindFiles::getUncachedFile( path );
}

