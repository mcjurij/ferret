#include <iostream>
#include <cassert>
#include <cstdlib>
#include <sstream>

#include "include_manager.h"
#include "glob_utility.h"
#include "engine.h"
#include "base_node.h"
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
    : fileRemoved(false)
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
        const BaseNode *node = it.getBaseNode();
        if( node )
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
                    string depfn = tempDepDir + "/" + node->getDir() + "/" + bn + ".d";

                    if( uniqueBasenames.find( bn ) == uniqueBasenames.end() )
                        uniqueBasenames[ bn ] = it.getId();
                    else
                        uniqueBasenames[ bn ] = -1;  // found it twice
                    
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
                        mkdir_p( tempDepDir + "/" + node->getDir() );
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
                        
                        hash_set_add( unsat_set, it.getId());
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
    if( !writeIgnHdr )
    {
        FILE *fp = fopen( "ferret_ignhdr", "r");
        if( fp )
        {
            while( !feof( fp ) )
            {
                char buf[512];
                if( fscanf( fp, "%511s\n", buf) != EOF )
                    ignoreHeaders.insert( string(buf) );
                else
                    break;
            }

            fclose( fp );
        }
    }
    
    if( initMode )
        readDepFiles( fileDb, writeIgnHdr);
    
    if( hash_set_get_size( unsat_set ) > 0 )
    {
        unsigned int i, s;
        int *a = hash_set_get_as_array( unsat_set, &s);
        for( i = 0; i < s; i++)
        {
            file_id_t from_id = a[i];
            
            if( fileDb.hasId( from_id ) && blockedIds.find( from_id ) == blockedIds.end() )
            {
                string from_fn = fileDb.getFileForId( from_id );
                BaseNode *node = fileDb.getBaseNodeFor( from_id );
                if( node )
                {
                    string dummy, bn, ext;
                    breakPath( from_fn, dummy, bn, ext);
                    string depfn = tempDepDir + "/" + node->getDir() + "/" + bn + ".d";
                    
                    const map<string,Seeker>::const_iterator &mit = seekerMap.find( from_fn );

                    int notFound = 0;
                    if( mit != seekerMap.end() )
                    {
                        if( mit->second.lookingFor.size() > 0 )
                        {
                            resolveFile( fileDb, from_id, mit->second, writeIgnHdr, notFound);
                        }
                        else
                            hash_set_remove( unsat_set, from_id);
                    }
                    else
                        if( readDepFile( from_fn, depfn, node) > 0 )
                            resolveFile( fileDb, from_id, seekerMap.at( from_fn ), writeIgnHdr, notFound);
                    
                    if( notFound == 0 )
                        hash_set_remove( unsat_set, from_id);
                }
            }
            else
                hash_set_remove( unsat_set, from_id);
        }
        free( a );
    }
    
    if( writeIgnHdr )
    {
        FILE *fp = fopen( "ferret_ignhdr", "w");
        
        if( fp )
        {
            set<string>::const_iterator ihit = ignoreHeaders.begin();
            for( ; ihit != ignoreHeaders.end(); ihit++)
                fprintf( fp, "%s\n", ihit->c_str());
            fclose( fp );
        }
    }
    
    FILE *fp = fopen( unsat_set_fn.c_str(), "w");
    if( fp )
    {
        hash_set_write( fp, unsat_set);
        fclose( fp );
    }

    const unsigned int treshold = 10;
    stringstream fw;
    if( hash_set_get_size( unsat_set ) > 0 )
    {
        unsigned int i, s;
        int *a = hash_set_get_as_array( unsat_set, &s);
        
        fw << "File(s) with unmet dependencies (listing at most " << treshold << "):\n";
        for( i = 0; i < s && i < treshold; i++)
        {
            file_id_t id = a[i];
            
            if( fileDb.hasId( id ) )
                fw << "  " << fileDb.getFileForId( id ) << "\n";
            else
                fw << "  ???" << "\n";
        }

        free( a );
    }
    
    if( hash_set_get_size( unsat_set ) > (int)treshold )
    {
        fw << "\nYou have more than " << treshold << " source files with unsatisfied dependencies. So, either\n"
           << "these are real and have to be fixed, or use --ignhdr once.\n"
           << "Using --ignhdr every time you call ferret will break the dependency analysis.\n\n";
    }

    finalWords = fw.str();
}


void IncludeManager::printFinalWords() const
{
    cout << finalWords;
}


int IncludeManager::readDepFile( const string &fn, const string &depfn, const BaseNode *node)
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
            
            addSeeker( fn, node->getSrcDir(), node->searchIncDirs,   // it.getFile() should be equal to df.target
                       df.depIncludes);
            entries++;
        }
    }
    else
        cerr << "error: could not open dependency file '" << depfn << "'\n";

    return entries;
}


bool IncludeManager::readDepFiles( FileManager &fileDb, bool writeIgnHdr)
{
    FileMap::Iterator it;
    
    while( fileDb.hasNext( it ) )
    {
        const BaseNode *node = it.getBaseNode();
        if( node )
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
                    string depfn = tempDepDir + "/" + node->getDir() + "/" + bn + ".d";
                    
                    if( FindFiles::existsUncached( depfn ) )
                    {
                        readDepFile( it.getFile(), depfn, node);
                        hash_set_add( unsat_set, it.getId());
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


void IncludeManager::removeFile( FileManager &fileDb, file_id_t id, const string &fn, const BaseNode *node, const set<file_id_t> &prereqs)
{
    string dummy, bn, ext;
    breakPath( fn, dummy, bn, ext);
    string depfn = tempDepDir + "/" + node->getDir() + "/" + bn + ".d";
    fileRemoved = true;
    
    if( verbosity > 0 )
        cout << "Removing dep file " << depfn << "\n";
    ::remove( depfn.c_str() );
    hash_set_remove( unsat_set, id);
    
    set<file_id_t>::const_iterator sit = prereqs.begin();
    for( ; sit != prereqs.end(); sit++)
    {
        file_id_t pre_id = *sit;
        if( fileDb.getCmdForId( pre_id ) == "D" )
        {
            hash_set_add( unsat_set, pre_id);
        }
    }
}


set<string> blockDoubleWarn;

bool IncludeManager::resolveFile( FileManager &fileDb, file_id_t from_id, const Seeker &s, bool writeIgnHdr, int &notFound)
{
    hash_set_t *new_deps_set = new_hash_set( 37 );
    size_t i,j;

    notFound = 0;
    
    for( i = 0; i < s.lookingFor.size(); i++)
    {
        string lookingFor = s.lookingFor[i];
        if( lookingFor != "" && lookingFor[0] != '/' )
        {
            int found = 0;
            for( j = 0; j < s.searchIncDirs.size(); j++)     // order matters
            {
                string tryfn = stackPath( s.searchIncDirs[j], lookingFor);

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
                cout << "info: include '" << lookingFor << "' found more than once, for " << fileDb.getFileForId( from_id ) << "\n";

            if( found == 0 )
            {
                // try "guessing" the header in case the looked for file name is unique
                map<string,file_id_t>::const_iterator lgit = uniqueBasenames.find( lookingFor );
                if( lgit != uniqueBasenames.end() && lgit->second != -1 )
                {
                    file_id_t to_id = lgit->second;
                    
                    if( !fileDb.hasBlockedDependency( from_id, to_id) )
                        hash_set_add( new_deps_set, to_id);
                    
                    found++;
                    if( verbosity > 1 )
                        cout << "inc mananger  resolved dep " << fileDb.getFileForId( from_id ) << " (" << from_id << ") -> "
                             << fileDb.getFileForId( to_id ) << " (" << to_id << ") but only by using the unique basename technique\n";
                }
            }
            
            if( found == 0 )
            {
                if( ignoreHeaders.find( lookingFor ) == ignoreHeaders.end() )
                {
                    notFound++;

                    if( blockDoubleWarn.find( lookingFor ) == blockDoubleWarn.end() )
                    {
                        cerr << "warning: " << s.from << " includes " << lookingFor
                             << ", but ferret wasn't able to find it.\n";
                        blockDoubleWarn.insert( lookingFor );
                    }

                    if( writeIgnHdr )
                        ignoreHeaders.insert( lookingFor );
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
