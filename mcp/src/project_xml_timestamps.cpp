#include <cstdio>
#include <cstdlib>
#include <iostream>

#include "project_xml_timestamps.h"
#include "glob_utility.h"

using namespace std;


void ProjectXmlTimestamps::addFile( const string &path )
{
    files.push_back( FindFiles::getUncachedFile( path ) );
}


static const unsigned char typeind_fn = 0x42;
static const unsigned char typeind_ts = 0x47;
void ProjectXmlTimestamps::writeTimes()
{
    string xmltsfn = stackPath( dbProjDir, "ferret_xmlts");
    FILE *fp = fopen( xmltsfn.c_str(), "w");
    if( !fp )
        return;

    int k = 0;
    list<File>::const_iterator it = files.begin();
    
    for( ; it != files.end(); it++)
    {
        const File &f = *it;
        unsigned int len = (unsigned int)f.getPath().length();
        
        if( len > 0 )
        {
            long long ts = f.getTimeMs();
            
            fwrite( &typeind_fn, sizeof(unsigned char), 1, fp);
            fwrite( &len, sizeof(unsigned int), 1, fp);
            fwrite( f.getPath().c_str(), sizeof(char), len, fp);
            fwrite( &typeind_ts, sizeof(unsigned char), 1, fp);
            fwrite( &ts, sizeof(long long), 1, fp);
            k++;
        }
    }
    
    if( verbosity )
        cout << "XML timestamps wrote " << k << " entries\n";
    
    fclose( fp );
}


bool ProjectXmlTimestamps::readTimes()
{
    string xmltsfn = stackPath( dbProjDir, "ferret_xmlts");
    FILE *fp = fopen( xmltsfn.c_str(), "r");
    if( !fp )
        return false;

    int k = 0;
    bool err = false;
    char *fn = 0;
    unsigned int max_len = 0;
    while( !feof( fp ) && !err )
    {
        unsigned char typeind;
        string  path;
        long long timestamp;
        size_t n;
        
        n = fread( &typeind, sizeof(unsigned char), 1, fp);
        if( n == 0 )
            break;
        
        if( typeind == typeind_fn )
        {
            unsigned int len;
            n = fread( &len, sizeof(unsigned int), 1, fp);
            if( n == 0 || len == 0)
            {
                err = true;
                break;
            }
            
            if( fn == 0 )
            {
                max_len = len;
                fn = (char *)malloc( (len+1) * sizeof(char) );
            }
            else if( len > max_len )
            {
                max_len = len * 2;
                free( fn );
                fn = (char *)malloc( (max_len+1) * sizeof(char) );
            }
            
            n = fread( fn, sizeof(char), len, fp);
            if( n == 0 )
            {
                err = true;
                break;
            }
            
            fn[len] = 0;
            path = fn;
        }
        else
            err = true;
        
        if( !err )
        {
            n = fread( &typeind, sizeof(unsigned char), 1, fp);
            if( n == 0)
                err = true;
            else if( typeind == typeind_ts )
                fread( &timestamp, sizeof(long long), 1, fp);
            else
                err = true;
        }
        
        if( !err )
        {
            compare_files.push_back( make_pair( path, timestamp) );
            k++;
        }
    }

    if( fn )
        free( fn );
    
    if( verbosity )
        cout << "XML timestamps read " << k << " entries\n";
    if( err )
        cerr << "error: ferret_xmlts corrupt.\n";
    fclose( fp );

    return !err;
}


bool ProjectXmlTimestamps::compare()
{
    list< pair<string, long long> >::const_iterator it = compare_files.begin();
    
    bool one_newer = false;
    for( ; it != compare_files.end() && !one_newer; it++)
    {
        string path = it->first;
        long long timestamp = it->second;
        
        if( FindFiles::exists( path ) )
        {
            File f = FindFiles::getCachedFile( path );
            one_newer = f.getTimeMs() > timestamp;

            if( one_newer && verbosity > 0 )
                cout << "xml file '" << f.getPath() << "' modified\n";
        }
        else
            one_newer = true;  // for safety
    }
    
    return one_newer;
}
