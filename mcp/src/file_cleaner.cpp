
#include <cstdio>
#include <iostream>
#include <cstring>
#include <sstream>

#include "file_cleaner.h"
#include "glob_utility.h"
#include "file_map.h"

using namespace std;


static string withS( int n )
{
    return n == 1 ? "" : "s";
}


static bool removeFile( const string &fn )
{
    if( verbosity > 0 )
        cout << "removing file '" + fn + "'\n";
    if( ::remove( fn.c_str() ) == 0 )
        return true;
    else
        return false;
}


bool FileCleaner::readDb( const string &projDir )
{    
    if( !filesdb.readDb( projDir, true) )   // true => ignore Xml nodes during read
    {
        return false;
    }
    
    return true;
}


void FileCleaner::clean( const set<string> &userTargets )
{
    checkUserTargets( userTargets );
    
    list<Cmd>::const_iterator it = cleanableFiles.begin();
    int cnt_obj=0, cnt_lib=0, cnt_bin=0, cnt_interm=0, cnt_wait=0;
    
    for( ; it != cleanableFiles.end(); it++)
    {
        const Cmd &cmd = *it;

        if( cmd.dep_type == "Cpp" || cmd.dep_type == "C" )
        {
            if( objClean && removeFile( cmd.file_name ) )
                cnt_obj++;
        }
        else if( cmd.dep_type == "L" || cmd.dep_type == "A" )
        {
            if( libexeClean && removeFile( cmd.file_name ) )
                cnt_lib++;
        }
        else if( cmd.dep_type == "X" )
        {
            if( libexeClean && removeFile( cmd.file_name ) )
                cnt_bin++;
        }
        else if( cmd.dep_type == "W" )
        {
            if( full )
            {
                if( removeFile( cmd.file_name ) )
                    cnt_wait++;
            }
        }
        else  // everything else must come from an extension
        {
            if( full )
            {
                if( removeFile( cmd.file_name ) )
                    cnt_interm++;
            }
        }
    }

    vector<string> outputHelper;
    stringstream ss;
    if( cnt_obj > 0 )
    {
        ss << cnt_obj << " object file" << withS( cnt_obj );
        outputHelper.push_back( ss.str() );
        ss.str("");
    }
    if( cnt_lib > 0 )
    {
        ss << cnt_lib << (cnt_lib == 1 ? " library" : " libraries");
        outputHelper.push_back( ss.str() );
        ss.str("");
    }
    if( cnt_bin > 0 )
    {
        ss << cnt_bin << (cnt_bin == 1 ? " binary" : " binaries");
        outputHelper.push_back( ss.str() );
        ss.str("");
    }
    if( cnt_interm > 0 )
    {
        ss << cnt_interm << " file" << withS( cnt_interm ) << " from extensions";
        outputHelper.push_back( ss.str() );
        ss.str("");
    }
    if( cnt_wait > 0 )
    {
        ss << cnt_wait << " wait file" << withS( cnt_wait );
        outputHelper.push_back( ss.str() );
        ss.str("");
    }
 
    if( outputHelper.size() > 0 )
    {
        cout << "ferret removed ";

        if( outputHelper.size() == 1 )
        {
            cout << outputHelper[0];
        }
        else
        {
            for( size_t i = 0; i < outputHelper.size(); i++)
            {
                if( i > 0 )
                {
                    if( i < outputHelper.size()-1 )
                        cout << ", ";
                    else
                        cout << " and ";
                }
                cout << outputHelper[i];
            }
        }
        cout << ".\n";
    }
    else
    {
        cout << "No files were removed. Nothing to do.\n";
    }
}


void FileCleaner::setFullClean()
{
    full = libexeClean = objClean = true;
}

void FileCleaner::setObjClean()
{
    objClean = true;
}

void FileCleaner::setLibExeClean()
{
    libexeClean = true;
}


void FileCleaner::traverseUserTargets( file_id_t id, int level)
{
    if( level >= 50 )
    {
        cerr << "error: level too deep.\n";
        return;
    }
    
    cleanableFiles.push_back( Cmd( filesdb.getCmdForId( id ), filesdb.getFileForId( id )) );
    
    if( level > depth )
        return;
     
    set<file_id_t> d = filesdb.getDependencies( id );
    set<file_id_t>::const_iterator dit = d.begin();
    for( ; dit != d.end(); dit++)
    {
        string cmd = filesdb.getCmdForId( *dit );
        
        if( cmd != "D" )
            traverseUserTargets( *dit, level+1);
    }
}


void FileCleaner::checkUserTargets( const set<string> &userTargets )
{
    FileMap::Iterator itFile;
    
    if( userTargets.size() > 0 )
    {
        int n = 0;
        
        while( filesdb.hasNext( itFile ) )
        {
            set<string>::const_iterator it = userTargets.begin();
            for( ; it != userTargets.end(); it++)
            {
                const string &userTarget = *it;
                
                if( itFile.getFile().find( userTarget ) != string::npos
                    && itFile.getCmd() != "D" )
                {
                    if( verbosity > 0 )
                        cout << "User given target matches " << itFile.getFile() << "\n";
                    traverseUserTargets( itFile.getId(), 1);
                    n++;
                }
            }
        }

        if( verbosity > 0 )
            cout << n << " user given target(s) found.\n";
    }
    else
    {
        while( filesdb.hasNext( itFile ) )
        {
            if( itFile.getCmd() != "D" )
                cleanableFiles.push_back( Cmd( itFile.getCmd(), itFile.getFile()) );
        }
    }
    
}

