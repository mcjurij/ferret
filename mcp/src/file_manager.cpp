
#include <iostream>
#include <cassert>
#include <cstdio>
#include <cstring>
#include <cstdlib>

#include "file_manager.h"
#include "file_map.h"
//#include "find_files.h"
#include "project_xml_node.h"
#include "glob_utility.h"

using namespace std;


static int idc=0;

static int idgen()
{
    return ++idc;
}

static FileMap allDeps( 10007 );


file_id_t FileManager::addFile( const string &fn, ProjectXmlNode *node)
{
    file_id_t id;

    if( allDeps.hasFileName( fn ) == 0 )
    {
        id = idgen();
        
        allDeps.add( id, fn, node, "D", data_t::NEW);
    }
    else
    {
        id = allDeps.getIdForFileName( fn );
    }
    
    return id;
}


file_id_t FileManager::addNewFile( const string &fn, ProjectXmlNode *node)
{
    file_id_t id;
    
    assert( allDeps.hasFileName( fn ) == 0 );
    
    id = idgen();
    allDeps.add( id, fn, node, "D", data_t::NEW);
    
    return id;
}


bool FileManager::removeFile( const string &fn )
{
    file_id_t id;

    id = allDeps.getIdForFileName( fn );
    assert( id != -1 );
    
    string cmd = allDeps.getCmdForFileName( fn );

    bool rem = false;
    if( cmd == "D" )
    {
        allDeps.markByDeletion( id );
        allDeps.remove( id );
        
        rem = true;
    }
    return rem;
}


void FileManager::addDependency( const string &from_fn, const string &to_fn)
{
    assert( from_fn != to_fn );
    file_id_t from_id, to_id;
    assert( allDeps.hasFileName( from_fn ) );
    assert( allDeps.hasFileName( to_fn ) );
    
    from_id = allDeps.getIdForFileName( from_fn );
    to_id = allDeps.getIdForFileName( to_fn );
        
    allDeps.addDependency( from_id, to_id);
}


void FileManager::addDependency( file_id_t from_id, const string &to_fn)
{
    file_id_t to_id;
    assert( allDeps.hasFileName( to_fn ) );
    
    to_id = allDeps.getIdForFileName( to_fn );
    
    allDeps.addDependency( from_id, to_id);
}


void FileManager::addDependency( file_id_t from_id, file_id_t to_id)
{
    allDeps.addDependency( from_id, to_id);
}


/*
void FileManager::removeDependencies( const string &fn )
{
    if( allDeps.hasFileName( fn ) )
        allDeps.removeDependencies( getIdForFile( fn ) );
}
*/

void FileManager::addWeakDependency( file_id_t from_id, file_id_t to_id)
{
    assert( allDeps.getCmdForId( to_id ) == "W" );   // to node must be wait node
    
    allDeps.addWeakDependency( from_id, to_id);
}


bool FileManager::hasBlockedDependency( file_id_t from_id, file_id_t to_id)
{
    assert( allDeps.hasId( from_id ) );

    return allDeps.hasBlockedDependency( from_id, to_id);
}


bool FileManager::hasNext( FileMap::Iterator &it ) const
{
    return allDeps.hasNext( it );
}


bool FileManager::compareAndReplaceDependencies( file_id_t id, hash_set_t *against)
{
    assert( allDeps.hasId( id ) );
    
    int diff = allDeps.compareDependencies( id, against);

    if( diff )
        allDeps.replaceDependencies( id, against);
    
    return diff != 0;
}


file_id_t FileManager::addCommand( const string &fn, const string &cmd, ProjectXmlNode *node)
{
    file_id_t id;
    
    assert( node );
    assert( cmd != "D" );  // D is for static/source files _only_
    
    if( allDeps.hasFileName( fn ) == 0 )
    {
        id = idgen();
        allDeps.add( id, fn, node, cmd, data_t::RESULT);
    }
    else
    {
        id = allDeps.getIdForFileName( fn );
        
        string existing_cmd = allDeps.getCmdForFileName( fn );
        if( cmd != existing_cmd )
        {
            cerr << "internal error: attempt to change command for " << fn
                 << " (id = " << id << "; existing cmd = " << existing_cmd << ", new = " << cmd << ").\n";
        }
    }
    
    return id;
}


file_id_t FileManager::addExtensionCommand( const string &fn, const std::string &cmd, ProjectXmlNode *node)
{
    assert( node );
    assert( cmd != "D" && cmd != "Cpp" && cmd != "C" && cmd != "L" && cmd != "X" && cmd != "A" );
    
    return addCommand( fn, cmd, node);
}


file_id_t FileManager::getIdForFile( const string &fn )
{
    return allDeps.getIdForFileName( fn );
}


string FileManager::getFileForId( file_id_t fid )
{
    return allDeps.getFileNameForId( fid );
}


string FileManager::getCmdForId( file_id_t fid )
{
    return allDeps.getCmdForId( fid );
}


bool FileManager::hasFileName( const string &fn )
{
    return allDeps.hasFileName( fn ) != 0;
}


bool FileManager::hasId( file_id_t fid )
{
    return allDeps.hasId( fid ) != 0;
}


bool FileManager::isTargetCommand( file_id_t id )
{
    string cmd = allDeps.getCmdForId( id );
    return cmd != "D" && cmd != "W";
}


set<file_id_t> FileManager::getDependencies( file_id_t id )
{
    unsigned int i,s;
    int *a = hash_set_get_as_array( allDeps.getDependencies( id ), &s);
    set<file_id_t> deps;
    
    for( i = 0; i < s; i++)
        deps.insert( a[i] );
    
    free( a );
    return deps;
}


set<file_id_t> FileManager::prerequisiteFor( file_id_t id )
{
    unsigned int i,s;
    int *a = hash_set_get_as_array( allDeps.prerequisiteFor( id ), &s);
    set<file_id_t> down_deps;
    
    for( i = 0; i < s; i++)
        down_deps.insert( a[i] );
    
    free( a );
    return down_deps;
}


ProjectXmlNode *FileManager::getXmlNodeFor( file_id_t id )
{
    return allDeps.getXmlNodeFor( id );
}


bool FileManager::readDb( const string &projDir, bool ignoreXmlNodes)    // ignoreXmlNodes => true for file cleaning
{
    char linebuf[2048];
    int line=0;
    int no_of_files=-1;
    FILE *fp;
    int in_edges = 0;
    int file_cnt = 0;

    string fn = stackPath( stackPath( ferretDbDir, projDir), "ferret_files");
    fp = fopen( fn.c_str(), "r");
    if( !fp )
    {
        cerr << "error: No ferret db '" << fn << "' found.\n";
        return false;
    }
    
    allDeps.setDbReadMode( true );
    
    int max_file_id = 0;
    int errors = 0;
    
    while( fgets( linebuf, 2048, fp) ) {
        line++;
        
        int len = strlen(linebuf);
        if( len>0 && linebuf[len-1]=='\n' )
            linebuf[len-1]=0;
        
        if( line==1 )
        {
            if( strcmp( "FERRET", linebuf) != 0 )
            {
                cerr << "error: line "<< line << ": not a ferret file.\n";
                errors++;
                break;
            }
        }
        else if( line==2 )
        {
            char cm[51];
            sscanf( linebuf, "%d %d %50s", &no_of_files, &idc, cm);
            if( verbosity > 1 )
                cout << "Number of files in files db = " << no_of_files << " / idc = " << idc << "\n";
            
            compileMode = cm;
        }
        else if( line==3 )
        {
            char pf[1024];
            sscanf( linebuf, "%1023s", pf);
            if( verbosity > 1 )
                cout << "Properties from files db = '" << pf << "'\n";
            
            propertiesFile = pf;            
        }
        else if( line==4 )
        {
            // separator
        }
        else if( line > 5 && !in_edges && strcmp( "--------------", linebuf)==0 ) {
            in_edges = 1;
        }
        else if( line >= 5 && !in_edges) {
            int file_id;
            char file_name[1025];
            char node_name[256];
            char dep_type[15];
            sscanf( linebuf, "%d %255s %14s %1024s", &file_id, node_name, dep_type, file_name);
            
            file_cnt++;
            if( file_cnt > no_of_files )
            {
                cerr << "error: line "<< line << ": more files than expected.\n";
                errors++;
                break;
            }
            else if( allDeps.hasId( file_id ) == 0 )
            {
                ProjectXmlNode *node = 0;
                
                if( !ignoreXmlNodes )
                {
                    if( strcmp( node_name, "*") != 0 )
                    {
                        node = ProjectXmlNode::getNodeByName( node_name );
                        if( node == 0 )
                        {
                            cerr << "error: line "<< line << ": module or name '" << node_name << "' unknown. do --init run.\n";
                            errors++;
                            break;
                        }
                        else
                            node->addDatabaseFile( file_name );
                    }
                }
                
                if( strcmp( dep_type, "D" ) != 0 )
                    allDeps.add( file_id, file_name, node, dep_type, data_t::RESULT);
                else
                    if( FindFiles::exists( file_name ) )
                        allDeps.add( file_id, file_name, node, dep_type, data_t::UNCHANGED);
                    else
                        allDeps.add( file_id, file_name, node, dep_type, data_t::GONE);
                
                if( file_id > max_file_id )
                    max_file_id = file_id;
            }
            else
            {
                cerr << "error: line "<< line << ": file id " << file_id << " already used.\n";
                errors++;
                break;
            }
        }
        else if( in_edges )
        {
            int fid_from, fid_to;
            char t;
            sscanf( linebuf, "%d-%c%d", &fid_from, &t,  &fid_to);
            
            if( allDeps.hasId( fid_from ) != 0 && allDeps.hasId( fid_to ) != 0 )
            {
                if( t == '>' )
                    allDeps.addDependency( fid_from, fid_to);
                else if( t == '<' )
                    allDeps.addWeakDependency( fid_from, fid_to);
                else if( t == 'X' )
                    allDeps.addBlockedDependency( fid_from, fid_to);
            }
            else
            {
                cerr << "error: line "<< line << ": unknown file id(s).\n";
                errors++;
            }
        }
    }

    fclose(fp);

    if( idc < max_file_id )  // huh?!?
        idc = max_file_id;
    
    allDeps.setDbReadMode( false );
    
    return errors == 0;
}


void FileManager::writeDb( const string &projDir )
{
    string dir = stackPath( ferretDbDir, projDir);
    mkdir_p( dir );
    string fn = stackPath( dir, "ferret_files");
    
    FILE *fp = fopen( fn.c_str(), "w");
    assert(fp);
    
    fprintf( fp, "FERRET\n");
    fprintf( fp, "%d %d %s\n", allDeps.getSize(), idc, compileMode.c_str());
    fprintf( fp, "%s\n", propertiesFile.c_str());
    fprintf( fp, "--------------\n");
    
    FileMap::Iterator itFile, itDeps;
    
    while( hasNext( itFile ) )
    {
        if( itFile.getStructuralState() != data_t::GONE )
            fprintf( fp, "%d %s %s %s\n", itFile.getId(), itFile.getXmlNode() ? itFile.getXmlNode()->getNodeName().c_str() : "*",
                     itFile.getCmd().c_str(), itFile.getFile().c_str());
    }
    
    fprintf( fp, "--------------\n");
    
    while( hasNext( itDeps ) )
    {
        if( itDeps.getStructuralState() != data_t::GONE )
        {
            hash_set_print_deps( fp, itDeps.getId(), itDeps.getDepsSet(), '>');           // dependency
            hash_set_print_deps( fp, itDeps.getId(), itDeps.getWeakDepsSet(), '<');       // weak dependency
            hash_set_print_deps( fp, itDeps.getId(), itDeps.getBlockedDepsSet(), 'X');    // blocked dependency
        }
    }
    fclose(fp);
}


void FileManager::removeCycles()
{
    hash_set_t *root_ids = allDeps.findRoots();

    if( verbosity > 0 )
    {
        cout << hash_set_get_size( root_ids ) << " root ids: ";
        hash_set_print( root_ids );
        cout << "\n";
    }
    
    delete_hash_set( root_ids );
}


bool FileManager::seeWhatsNewOrGone()
{
    int cntStructChanges = allDeps.whatsChanged();
    
    while( allDeps.cleanupDeletions() )
        ;

    return cntStructChanges > 0;
}


void FileManager::printWhatsChanged()
{
    allDeps.whatsChanged();
}


void FileManager::persistMarkByDeletions( hash_set_t *mbd_set )
{
    unsigned int i,s;
    int *a = hash_set_get_as_array( mbd_set, &s);
    
    for( i = 0; i < s; i++)
        if( !allDeps.hasId( a[i] ) )
            hash_set_remove( mbd_set, a[i]);
    free( a );
    
    allDeps.persistMarkByDeletions( mbd_set );
}


void FileManager::setCompileMode( const string &cm )
{
    compileMode = cm;
}


string FileManager::getCompileMode() const
{
    return compileMode;
}


void FileManager::setPropertiesFile( const string &pf )
{
    assert( pf != "" );
    propertiesFile = pf;
}


string FileManager::getPropertiesFile() const
{
    return propertiesFile;
}


void FileManager::sendToEngine( Engine &engine )
{
    allDeps.sendToEngine( engine );
}


void FileManager::sendToMakefileEngine( MakefileEngine &engine )
{
    allDeps.sendToMakefileEngine( engine );
}


void FileManager::print()
{
    allDeps.print();
}
