#include <iostream>
#include <cassert>
#include <cstdlib>

#include "include_manager.h"
#include "glob_utility.h"
#include "engine.h"
#include "project_xml_node.h"
#include "hash_set.h"

using namespace std;

static bool isCppFile( const string &ext )
{
    return ext == ".cpp" || ext == ".c" ||
        ext == ".h" || ext == ".hpp" ||
        ext == ".C" || ext == ".cxx"; // ||
//        ext == ".nsmap";
}

// -----------------------------------------------------------------------------
IncludeManager *IncludeManager::theIncludeManager = 0;

IncludeManager *IncludeManager::getTheIncludeManager()
{
    if( theIncludeManager == 0 )
        theIncludeManager = new IncludeManager;
    
    return theIncludeManager;
}

 
IncludeManager::IncludeManager()
    : fileRemoved(false), ignHdrFp(0)
{
    unsat_set = new_hash_set( 101 );
}


void IncludeManager::readUnsatSet( bool initMode )
{
    unsat_set_fn = stackPath( dbProjDir, "ferret_unsat");

    if( !initMode )
    {
        FILE *fp = fopen( unsat_set_fn.c_str(), "r");
        if( fp )
        {
            hash_set_read( fp, unsat_set);
            fclose( fp );
        }
    }    
}


void IncludeManager::createMissingDepFiles( FileManager &fileDb, Executor &executor, bool printTimes)
{
    FileMap::Iterator it;
    
    while( fileDb.hasNext( it ) )
    {
        const ProjectXmlNode *xmlNode = it.getXmlNode();
        if( xmlNode )
        {
            string dummy, bn, ext;
            breakPath( it.getFile(), dummy, bn, ext);
        
            if( isCppFile( ext ) )
            {
                if( blockedIds.find( it.getId() ) != blockedIds.end() )
                {
                    if( verbosity > 0 )
                        cout << "inc mananger  dep file for '" << it.getFile()  << "' blocked.\n";
                }
                else
                {
                    string depfn = tempDepDir + "/" + xmlNode->getDir() + "/" + bn + ".d";
            
                    bool do_dep = false;
                    if( FindFiles::exists( depfn ) )
                    {
                        if( !FindFiles::exists( it.getFile() ) )
                            do_dep = false;
                        else
                        {
                            File f = FindFiles::getCachedFile( it.getFile() );
                            File depFile = FindFiles::getCachedFile( depfn );
                    
                            if( f.isNewerThan( depFile ) )
                                do_dep = true;
                        }
                    }
                    else if( FindFiles::exists( it.getFile() ) )
                    {
                        mkdir_p( tempDepDir + "/" + xmlNode->getDir() );
                        do_dep = true;
                    }
                    else
                        do_dep = false;
            
                    if( do_dep )
                    {
                        vector<string> args;
                        args.push_back( it.getFile() );
                        args.push_back( depfn );
                    
                        ExecutorCommand ec = ExecutorCommand( args );
                        ec.addFileToRemoveAfterSignal( depfn );
                        FindFiles::remove( depfn.c_str() );
                    
                        depEngine.addDepCommand( ec );
                    
                        filesWithUpdate.push_back( it.getFile() );
                        depFilesWithUpdate.push_back( depfn );
                    }
                }
            }
        }
    }

    if( filesWithUpdate.size() > 0 )
    {
        if( verbosity > 0 )
            cout << "inc mananger  " << filesWithUpdate.size() << " dep file missing or needs update.\n";
        depEngine.doWork( executor, printTimes);  // create all missing .d files
    }
}


void IncludeManager::resolve( FileManager &fileDb, bool initMode, bool writeIgnHdr)
{
    resolve_normal( fileDb, initMode, writeIgnHdr);
    
    if( hash_set_get_size( unsat_set ) > 0 )
    {
        unsigned int i, s;
        int *a = hash_set_get_as_array( unsat_set, &s);
        for( i = 0; i < s; i++)
        {
            file_id_t from_id = a[i];
            
            if( fileDb.hasId( from_id ) && blockedIds.find( from_id ) == blockedIds.end() )
            {
                string fn = fileDb.getFileForId( from_id );
                ProjectXmlNode *xmlNode = fileDb.getXmlNodeFor( from_id );
                if( xmlNode )
                {
                    string dummy, bn, ext;
                    breakPath( fn, dummy, bn, ext);
                    string depfn = tempDepDir + "/" + xmlNode->getDir() + "/" + bn + ".d";
                    
                    const map<string,Seeker>::const_iterator &mit = seekerMap.find( fn );
                
                    if( mit != seekerMap.end()  )
                    {
                        if( mit->second.lookingFor.size() > 0 )
                        {
                            if( resolveFile( fileDb, from_id, seekerMap.at( fn ), false) )
                                hash_set_remove( unsat_set, from_id);
                        }
                        else
                            hash_set_remove( unsat_set, from_id);
                    }
                    else
                        if( readDepFile( fn, depfn, xmlNode) > 0 )
                            if( resolveFile( fileDb, from_id, seekerMap.at( fn ), false) )
                                hash_set_remove( unsat_set, from_id);
                }
            }
        }
        free( a );
    }
    
    FILE *fp = fopen( unsat_set_fn.c_str(), "w");
    if( fp )
    {
        hash_set_write( fp, unsat_set);
        fclose( fp );
    }
}


static set<string> blockDoubleIncMiss;
int IncludeManager::readDepFile( const string &fn, const string &depfn, const ProjectXmlNode *xmlNode)
{
    int entries = 0;
    
    FILE *fp = fopen( depfn.c_str(), "r");
    if( fp )
    {
        ParseDep pd( fp );
        if( verbosity > 0 )
            cout << "Reading dependency file " << depfn << " ...\n";
        pd.parse();
        
        fclose( fp );
        
        const vector<DepFileEntry> dfe = pd.getDepEntries();
                        
        size_t j;
        for( j = 0; j < dfe.size(); j++)   // should be only one
        {
            const DepFileEntry &df = dfe[j];
            
            if( fn != df.target )
                cerr << "warning: unexpected target file name in dependency file '" << df.target << "', expected '" << fn << "'\n";
            
            addSeeker( fn, xmlNode->getSrcDir(), xmlNode->searchIncDirs,   // it.getFile() should be equal to df.target
                       df.depIncludes);
            entries++;
        }
    }
    else
        cerr << "error: could not open dependency file '" << depfn << "'\n";

    return entries;
}


bool IncludeManager::readDepFiles( FileManager &fileDb, bool initMode, bool writeIgnHdr)
{
    if( !writeIgnHdr )
    {
        FILE *fp = fopen( "ferret_ignhdr", "r");
        if( fp )
        {
            while( !feof( fp ) )
            {
                char buf[512];
                if( fscanf( fp, "%511s\n", buf) != EOF )
                    blockDoubleIncMiss.insert( string(buf) );
                else
                    break;
            }

            fclose( fp );
        }
    }
    
    size_t i;
    bool check_all = initMode || fileRemoved;
    
    for( i = 0; i < filesWithUpdate.size() && !check_all; i++)
    {
        const string &fn = filesWithUpdate[i];
        file_id_t from_id  = fileDb.getIdForFile( fn );
        
        if( blockedIds.find( from_id ) == blockedIds.end() )
        {
            if( from_id != -1 )
            {
                const string &depfn = depFilesWithUpdate[i];
                ProjectXmlNode *xmlNode = fileDb.getXmlNodeFor( from_id );
                assert( xmlNode );
                
                if( readDepFile( fn, depfn, xmlNode) > 0 )
                    check_all = resolveFile( fileDb, from_id, seekerMap.at( fn ), false);
            }
            else
                check_all = true;   // new file
        }
    }

    if( !check_all )
    {
        if( verbosity > 0 )
            cout << "No include dependencies have changed.\n";
        return false;
    }    
    
    FileMap::Iterator it;
    
    while( fileDb.hasNext( it ) )
    {
        const ProjectXmlNode *xmlNode = it.getXmlNode();
        if( xmlNode )
        {
            string dummy, bn, ext;
            breakPath( it.getFile(), dummy, bn, ext);
            //string cmd = it.getCmd();
            
            if( isCppFile( ext ) )
            {
                if( blockedIds.find( it.getId() ) != blockedIds.end() )
                {
                    if( verbosity > 0 )
                        cout << "Includes from '" << it.getFile()  << "' blocked.\n";
                }
                else if( seekerMap.find( it.getFile() ) == seekerMap.end() )
                {
                    string depfn = tempDepDir + "/" + xmlNode->getDir() + "/" + bn + ".d";
                    
                    if( FindFiles::existsUncached( depfn ) )
                    {
                        readDepFile( it.getFile(), depfn, xmlNode);
                    }
                    else
                        cerr << "warning: file " << depfn << " must exist at this point.\n";
                }
            }
        }
    }

    return true;
}


void IncludeManager::addSeeker( const string &from, const string &localDir, const vector<string> &searchIncDirs,
                                const vector<string> &lookingFor)
{
    assert( from != "" );
    
    map<string,Seeker>::iterator mit = seekerMap.find( from );
    
    if( mit != seekerMap.end() )
    {
        Seeker &s = mit->second;
        
        if( verbosity > 1 )
            cout << "inc mananger  seeker '" << from << "' additionally looking for " << join( ", ", lookingFor, false) << ".\n";
        
        for( size_t i = 0; i < lookingFor.size(); i++)
            if( localDir != searchIncDirs[i] )
                s.lookingFor.push_back( lookingFor[i] );
    }
    else
    {
        Seeker s;
        s.from = from;
        s.lookingFor = lookingFor;
        
        s.searchIncDirs.push_back( localDir );
        for( size_t i = 0; i < searchIncDirs.size(); i++)
            if( localDir != searchIncDirs[i] )
                s.searchIncDirs.push_back( searchIncDirs[i] );
        
        seekerMap[ from ] = s;
        
        if( verbosity > 1 )
        {
            cout << "inc mananger  adding new seeker '" << s.from << "'.\n";
            cout << "inc mananger  seeker '" << s.from << "' looking for " << join( ", ", s.lookingFor, false) << ".\n";
            
            if( verbosity > 2 )
            {
                cout << "inc mananger  '" << s.from << "' seeking in";
                cout << " " << join( ", ", s.searchIncDirs, false);
                cout << "\n";
            }
        }
    }
}


void IncludeManager::addBlockedId( file_id_t id )
{
    blockedIds.insert( id );
}


void IncludeManager::addBlockedIds( const set<file_id_t> &blids )
{
    blockedIds.insert( blids.begin(), blids.end());
}


void IncludeManager::removeFile( FileManager &fileDb, const string &fn, const ProjectXmlNode *xmlNode, const set<file_id_t> &prereqs)
{
    string dummy, bn, ext;
    breakPath( fn, dummy, bn, ext);
    string depfn = tempDepDir + "/" + xmlNode->getDir() + "/" + bn + ".d";
    fileRemoved = true;
    
    if( verbosity > 0 )
        cout << "Removing dep file " << depfn << "\n";
    ::remove( depfn.c_str() );
    
    set<file_id_t>::const_iterator sit = prereqs.begin();
    for( ; sit != prereqs.end(); sit++)
    {
        file_id_t id = *sit;
        if( fileDb.getCmdForId( id ) == "D" )
        {
            hash_set_add( unsat_set, id);

        }
    }
}


bool IncludeManager::resolveFile( FileManager &fileDb, file_id_t from_id, const Seeker &s, bool writeIgnHdr)
{
    hash_set_t *new_deps_set = new_hash_set( 37 );
    size_t i,j;
    for( i = 0; i < s.lookingFor.size(); i++)
    {
        if( s.lookingFor[i] != "" && s.lookingFor[i][0] != '/' )
        {
            int found = 0;
            for( j = 0; j < s.searchIncDirs.size(); j++)     // order matters
            {
                string tryfn = stackPath( s.searchIncDirs[j], s.lookingFor[i]);

                file_id_t to_id = fileDb.getIdForFile( tryfn );
                
                if( to_id != -1  )
                {
                    if( !fileDb.hasBlockedDependency( from_id, to_id) && found == 0 )
                        hash_set_add( new_deps_set, to_id);
                    found++;
                    
                    if( verbosity > 1 )
                        cout << "inc mananger  resolved dep " << fileDb.getFileForId( from_id ) << " (" << from_id << ") -> "
                             << tryfn << " (" << to_id << ")\n";
                    
                    if( !show_info )
                        break;                         // first wins
                }                    
            }
            
            if( show_info && found > 1 )
                cout << "info: include '" << s.lookingFor[i] << "' found more than once, for " << fileDb.getFileForId( from_id ) << "\n";
            
            if( found == 0 )
            {
                if( blockDoubleIncMiss.find( s.lookingFor[i] ) == blockDoubleIncMiss.end() )
                {
                    cerr << "warning: " << s.from << " includes " << s.lookingFor[i]
                         << ", but ferret wasn't able to find it.\n";
                    blockDoubleIncMiss.insert( s.lookingFor[i] );
                    if( writeIgnHdr && ignHdrFp )
                        fprintf( ignHdrFp, "%s\n", s.lookingFor[i].c_str());
                }
            }
        }
    }
    
    bool repl = fileDb.compareAndReplaceDependencies( from_id, new_deps_set);
    if( repl )
        cout << "Replaced dependencies for '" << s.from << "'.\n";
    delete_hash_set( new_deps_set );

    return repl;
}


void IncludeManager::resolve_normal( FileManager &fileDb, bool initMode, bool writeIgnHdr)
{
    if( !readDepFiles( fileDb, initMode, writeIgnHdr) )
        return;

    if( writeIgnHdr )
        ignHdrFp = fopen( "ferret_ignhdr", "w");
    
    
    FileMap::Iterator it;
    
    while( fileDb.hasNext( it ) )
    {
        string file_name = it.getFile();
        string cmd = it.getCmd();

        if( cmd != "Cpp" && cmd != "L" && cmd != "X" && cmd != "C" )
        {
            //string dummy, bn, ext;
            //breakPath( file_name, dummy, bn, ext);
            
            if( blockedIds.find( it.getId() ) == blockedIds.end() )
            {
                const map<string,Seeker>::const_iterator &mit = seekerMap.find( file_name );
                
                if( mit != seekerMap.end() && mit->second.lookingFor.size() > 0 )
                {
                    if( verbosity > 1 )
                        cout << "inc mananger  checking " << file_name << " ...\n";
                    resolveFile( fileDb, it.getId(), mit->second, writeIgnHdr);
                }
                else if( verbosity > 1 )
                    cout << "inc mananger  checking " << file_name << " => not seeking\n";
            }
            else if( verbosity > 1 )
                cout << "inc mananger  checking " << file_name << " => BLOCKED\n";
        }
    }

    if( ignHdrFp )
        fclose( ignHdrFp );
}
