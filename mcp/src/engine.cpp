#include <cstdio>
#include <cassert>
#include <cstring>
#include <cstdlib>
#include <iostream>
#include <sstream>
#include <algorithm>

#include "engine.h"
#include "executor.h"
#include "project_xml_node.h"
#include "file_manager.h"
#include "glob_utility.h"
#include "script_template.h"
#include "find_files.h"
#include "output_collector.h"

using namespace std;


static int fe_b( int file_id )
{
    return file_id % FILE_TABLE_SIZE;
}


static string genScript( const string &file_name, file_id_t file_id)
{
    return ScriptManager::getTheScriptManager()->write( file_id, file_name);
}

// -----------------------------------------------------------------------------
void EngineBase::indicateDone( int file_id, long long curr_time)
{
    // default is to do nothing
}

// -----------------------------------------------------------------------------
void DepEngine::addDepCommand( ExecutorCommand ec )
{
    commands.push_back( ec );
}


int DepEngine::doWork( ExecutorBase &executor, bool printTime, const set<string> &userTargets)
{
    pos = 0;
    
    if( commands.size() >  0 )
        executor.processCommands( *this );
       
    ofstream osh( "last_run.html" );
    OutputCollector::getTheOutputCollector()->html( osh );

    ofstream ost( "last_run.txt" );
    OutputCollector::getTheOutputCollector()->text( ost );
    
    return 0;
}


ExecutorCommand DepEngine::nextCommand()
{
    if( pos < commands.size() )
        return commands[ pos++ ];
    else
        return ExecutorCommand( "FINALIZE" );
}

// -----------------------------------------------------------------------------

void SyncEngine::addSyncCommand( ExecutorCommand ec )
{
    commands.push_back( ec );
}


int SyncEngine::doWork( ExecutorBase &executor, bool printTime, const set<string> &userTargets)
{
    pos = 0;
    
    if( commands.size() >  0 )
        executor.processCommands( *this );
       
    ofstream osh( "last_sync_run.html" );
    OutputCollector::getTheOutputCollector()->html( osh );

    ofstream ost( "last_sync_run.txt" );
    OutputCollector::getTheOutputCollector()->text( ost );
    
    return 0;
}


ExecutorCommand SyncEngine::nextCommand()
{
    if( pos < commands.size() )
        return commands[ pos++ ];
    else
        return ExecutorCommand( "FINALIZE" );
}

// -----------------------------------------------------------------------------

Engine::Engine( int table_size, const std::string &dbProjDir, bool doScfs)
    : EngineBase(), dbProjDir(dbProjDir),
      round(0), scfs(doScfs), stopOnError(false)
{
    int i;
    for( i=0; i<FILE_TABLE_SIZE; i++)
    {
        files[ i ].entry = 0;
        files[ i ].next = 0;
    }

    all_targets = new_hash_set( 701 );
    targets_left = new_hash_set( 701 );
    
    target_root_ids = new_hash_set( 233 );
    wavefront_ids = new_hash_set( 499 );
    
    to_do_set = new_hash_set( 1051 );
    in_work_set = new_hash_set( 47 );
    done_set = new_hash_set( 701 );
    failed_set = new_hash_set( 179 );

    to_do_cmd_ids = 0;    // array that contains all from to_do_set
    to_do_cmd_pos = 0;
}


Engine::~Engine()
{
    delete_hash_set( all_targets );
    delete_hash_set( targets_left );
    
    delete_hash_set( target_root_ids );
    delete_hash_set( wavefront_ids );
    
    delete_hash_set( to_do_set );
    delete_hash_set( in_work_set );
    delete_hash_set( done_set );
    delete_hash_set( failed_set );

    delete to_do_cmd_ids;
}


Engine::command_t *Engine::init_entry( data_t *cmd )
{
    command_t *e = (command_t *)malloc( sizeof(command_t) );
    e->file_id = cmd->file_id;
    e->file_name = strdup( cmd->file_name.c_str() );
    strncpy( e->dep_type, cmd->cmd.c_str(), 15);
    e->dep_type[14] = 0;
    e->state = cmd->state;
    e->structural_state = cmd->structural_state;
    e->xmlNode = cmd->xmlNode;
    
    unsigned int deps_s;
    e->deps = (int *)hash_set_get_as_array( cmd->deps_set, &deps_s);
    e->deps_size = deps_s;
    e->upwards = 0;
    
    unsigned int weak_s;
    e->weak = (int *)hash_set_get_as_array( cmd->weak_set, &weak_s);
    e->weak_size = weak_s;
    
    e->downward_cap = 32;
    e->downwards = (command_t **)malloc( sizeof(command_t *) * 32 );
    memset( e->downwards, 0, sizeof(command_t *) * 32);  // so I can sleep better...
    e->downward_size = 0;
    e->downward_deep = true;

    if( e->xmlNode && !e->xmlNode->doDeep() && strcmp( e->dep_type, "L") == 0 )  // do not go deep for shared libraries
        e->downward_deep = false;
    
    e->marked_by_deps_changed = cmd->mark_by_deps_changed;
    
    if( hash_set_has_id( global_mbd_set, e->file_id) )
        e->marked_by_deletion = true;
    else
        e->marked_by_deletion = false;
    
    e->dominator_set = 0;

    e->in_to_do = e->is_target = e->is_done = e->has_failed = false;
    e->dominating_time = 0;
    e->user_selected = false;
    e->find_target_root_visited = false;

    e->scfs_time = 0;
    
    return e;
}


void Engine::add_downward_ptr( command_t *c, command_t *up)
{
    assert( c );
    assert( up );
    
    int i;
    for( i = 0;  i < c->downward_size; i++)
        if( c->downwards[ i ] == up )
        {
            cerr << "downward pointer added 2nd time\n";
            return;
        }
    
    if( c->downward_size == c->downward_cap )
    {
        int new_cap = c->downward_cap * 2;
        c->downwards = (command_t **)realloc( c->downwards, sizeof(command_t *) * new_cap);
        c->downward_cap = new_cap;
    }

    c->downwards[ c->downward_size ] = up;
    c->downward_size++;
}


void Engine::addCommand( data_t *cmd )
{
    command_t *c = init_entry( cmd );
    int b = fe_b( cmd->file_id );
    
    file_ids.push_back( c->file_id );
    
    if( files[ b ].entry == 0 )
    {
        files[ b ].entry = c;
        files[ b ].next = 0;
    }
    else
    {
        fe_bucket_t *be = (fe_bucket_t *)malloc( sizeof( fe_bucket_t ) );
        be->entry = c;
        be->next = 0;
    
        fe_bucket_t *h = &(files[ b ]);
        assert( h );
        while( h->next != 0 )
            h = h->next;
        h->next = be;
    }
}


Engine::command_t *Engine::find_command( int file_id )
{
    int b = fe_b(file_id);
    fe_bucket_t *h = &(files[ b ]);
    
    if( h->entry == 0 )
        return 0;
    
    do
    {
        if( h->entry->file_id == file_id )
            return h->entry;
        h = h->next;
    }
    while( h );

    return 0;
}


char *Engine::find_filename( int file_id )
{
    return find_command( file_id )->file_name;
}


void Engine::doPointers()
{
    size_t i;
    for( i = 0;  i < file_ids.size(); i++)
    {
        int id = file_ids[i];
        
        command_t *c = find_command( id );     // upward is from c to d (=c needs d)
        c->upwards = (command_t **)malloc( sizeof(command_t *) * c->deps_size );
        
        int k;
        for( k = 0; k < c->deps_size; k++)
        {
            command_t *d = find_command( c->deps[k] );
            c->upwards[k] = d;

            add_downward_ptr( d, c);        // downward is from d to c (=d is a prerequisite for c)
        }

        if( c->deps_size == 0 )
            hash_set_add( wavefront_ids, c->file_id);
    }
}


static const unsigned char typeind_id = 0x01;
static const unsigned char typeind_ts = 0x05;
void Engine::writeScfsTimes()
{
    if( !scfs )
        return;

    string scfsfn = stackPath( dbProjDir, "ferret_scfs");
    size_t i;
    FILE *fp = fopen( scfsfn.c_str(), "w");
    if( !fp )
        return;

    int k = 0;
    for( i = 0;  i < file_ids.size(); i++)
    {
        command_t *c = find_command( file_ids[i] );
        
        if( c->scfs_time > 0 )
        {
            fwrite( &(typeind_id), sizeof(unsigned char), 1, fp);
            fwrite( &(c->file_id), sizeof(int), 1, fp);
            fwrite( &(typeind_ts), sizeof(unsigned char), 1, fp);
            fwrite( &(c->scfs_time), sizeof(long long), 1, fp);

            k++;
        }
    }
    
    if( verbosity )
        cout << "Scfs wrote " << k << " entries\n";
    
    fclose( fp );
}


void Engine::readScfsTimes()
{
    string scfsfn = stackPath( dbProjDir, "ferret_scfs");
    if( !scfs )
    {
        ::remove( scfsfn.c_str() );
        return;
    }
    
    FILE *fp = fopen( scfsfn.c_str(), "r");
    if( !fp )
        return;

    int k = 0;
    bool err = false;
    while( !feof( fp ) && !err )
    {
        unsigned char typeind;
        int file_id;
        long long timestamp;
        size_t n;
        
        n = fread( &typeind, sizeof(unsigned char), 1, fp);
        if( n == 0 )
            break;
        
        if( typeind == typeind_id )
            fread( &file_id, sizeof(int), 1, fp);
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
            command_t *c = find_command( file_id );
            
            if( c )
            {
                c->scfs_time = timestamp;
                k++;
            }
        }
    }
    
    if( verbosity )
        cout << "Scfs read " << k << " entries\n";
    if( err )
        cerr << "warning: ferret_scfs corrupt.\n";
    fclose( fp );
}


hash_set_t *Engine::traverse_union_tree( command_t *e, int level)
{
    if( level>=50 )
    {
        cerr << "union traversal: level too deep\n";
        return 0;
    }

    assert( e );

    if( e->dominator_set != 0 )
        return e->dominator_set;
    
    hash_set_t *hs = 0;
    
    if( e->downward_deep )
    {
        int k;
        
        hs = new_hash_set( 307 );  // difficult to guess here, headers dominate much more than object files for example
        hash_set_add( hs, e->file_id);
        
        for( k=0; k < e->downward_size; k++)
        {
            command_t *d = e->downwards[ k ];
            hash_set_t *sub = traverse_union_tree( d, level+1);
            
            if( sub )
            {
                hash_set_union( hs, sub);
            }
        }
    }
    else
    {
        hs = new_hash_set( 3 );
        hash_set_add( hs, e->file_id);  // this node dominates no other nodes (for shared libraries)
    }
    
    e->dominator_set = hs;  // all nodes this node dominates
    
    return hs;
}


void Engine::traverse_for_dominator_sets()
{
    size_t i;
    
    for( i = 0;  i < file_ids.size(); i++)         // find dominator sets for all files
    {
        command_t *c = find_command( file_ids[i] );
        
        /* hash_set_t *hs = */ traverse_union_tree( c, 0);
        //printf( "UNION FIND result for start %d:", file_ids[i]);
        //hash_set_print( hs );
        //printf( "\n" );
    }
}


bool Engine::prereqs_done( command_t *c )
{
    int i;
    
    for( i = 0; i < c->deps_size; i++)
    {
        if( !c->upwards[i]->is_done )
            return false;
    }
    
    return true;
}


string Engine::generateScript( command_t *c )
{
    return genScript( c->file_name, c->file_id);
}


bool Engine::make_target_by_wait( command_t *c )
{
    bool make_target = false;
    
    for( int w = 0; w < c->weak_size && !make_target; w++)
    {
        command_t *wait = find_command( c->weak[w] );
        
        if( FindFiles::exists( wait->file_name ) /* && FindFiles::exists( c->file_name ) */ )
        {
            const File waitf = FindFiles::getCachedFile( wait->file_name );
            
            for( int i = 0; i < c->deps_size && !make_target; i++)
            {
                command_t *dep = find_command( c->deps[i] );
                
                const File depf = FindFiles::getCachedFile( dep->file_name );

                if( scfs && c->scfs_time > 0 && dep->scfs_time > 0 )
                {
                    if( c->scfs_time < dep->scfs_time )
                        make_target = true;
                }
                else
                    if( depf.isNewerThan( waitf ) )
                        make_target = true;
            }
        }
        else
            make_target = true;
    }
    
    return make_target;
}


void Engine::make_targets_by_dom_set( command_t *c )    // make all nodes dominated by c a target
{
    unsigned int i,s;
    int *a = hash_set_get_as_array( c->dominator_set, &s);
    
    for( i = 0; i < s; i++)
    {
        command_t *subt = find_command( a[i] );

        if( subt->file_id != c->file_id && !subt->is_target && subt->user_selected &&
            strcmp( subt->dep_type, "D") != 0 && strcmp( subt->dep_type, "W") != 0 )
        {
            subt->is_target = true;
            hash_set_add( all_targets, subt->file_id);
            
            if( subt->downward_size == 0 )
                final_targets.push_back( subt->file_id );
            
            if( verbosity > 1 )
                cout << "file " << subt->file_name << " (" << subt->file_id << ") also becomes target, dominated by "
                     << c->file_name << " (" << c->file_id << ")\n";
        }
    }
    
    free( a );
}


void Engine::fill_target_set()
{
    int activity;
    hash_set_t *front_ids;
    size_t n;
    front_ids = new_hash_set( 499 );
    
    for( n = 0; n < file_ids.size(); n++)
    {
        command_t *c = find_command( file_ids[n] );

        if( c->deps_size == 0 )  // root node?
        {
            if( strcmp( c->dep_type, "D") == 0 || FindFiles::exists( c->file_name ) )
            {
                if( !(c->marked_by_deletion || c->marked_by_deps_changed) )
                    hash_set_add( front_ids, c->file_id);
                
                if( scfs && c->scfs_time > 0 )
                    c->dominating_time = c->scfs_time;
                else
                {
                    const File f = FindFiles::getCachedFile( c->file_name );
                    c->dominating_time = f.getTimeMs();
                }
            }
        }
        
        if( c->marked_by_deletion || c->marked_by_deps_changed )
        {
            if( !c->is_target )
            {
                if( strcmp( c->dep_type, "D") != 0 && strcmp( c->dep_type, "W") != 0 )
                {
                    c->is_target = true;
                    hash_set_add( all_targets, c->file_id);
                }
                if( verbosity > 1 )
                    cout << "target planner   making targets from " << c->file_name << " (" << c->file_id << ") on, since marked\n";
                make_targets_by_dom_set( c );
            }
        }
        else if( c->user_selected && c->weak_size > 0 && make_target_by_wait( c ) ) // weak_size > 0 only true for extension nodes
        {
            c->is_target = true;
            hash_set_add( all_targets, c->file_id);
            
            if( verbosity > 0 )
                cout << "target planner   adding "  << c->file_name << " (" << c->file_id << ") to target set (trough wait node)\n";
            make_targets_by_dom_set( c );
        }
    }
    
    do{
        activity = 0;
        if( verbosity > 1 )
            cout << "target planner   wavefront size = "  << hash_set_get_size( front_ids ) << "\n";
        
        unsigned int i, s;
        int *a = hash_set_get_as_array( front_ids, &s);
        
        for( i = 0; i < s; i++)
        {
            command_t *c = find_command( a[i] );
            
            int k;
            for( k = 0; k < c->downward_size; k++)
            {
                command_t *d = c->downwards[ k ];
                
                char *down_fn = d->file_name;
                bool dominates = false;
                
                if( d->marked_by_deletion || d->marked_by_deps_changed )
                {
                    d->dominating_time = (long long)365*24*60*60*1000 * 100;    // many years into the future to force domination of all that come after
                    dominates = true;
                }
                else if( strcmp( d->dep_type, "D") == 0 || FindFiles::exists( down_fn ) )
                {
                    if( d->dominating_time == 0 )
                    {
                        if( scfs && d->scfs_time > 0 )
                            d->dominating_time = d->scfs_time;
                        else
                        {
                            const File down_f = FindFiles::getCachedFile( down_fn );
                            d->dominating_time = down_f.getTimeMs();
                        }
                    }
                    
                    if( c->dominating_time > d->dominating_time )  // propagate dominating time
                    {
                        d->dominating_time = c->dominating_time;
                        dominates = true;
                    }
                }
                else
                {
                    d->dominating_time = (long long)365*24*60*60*1000 * 100;    // many years into the future to force domination of all that come after
                    dominates = true;
                }
                
                if( strcmp( d->dep_type, "D") == 0 )
                {
                    hash_set_add( front_ids, d->file_id);
                    if( verbosity > 2 )
                        cout << "target planner   moving on from " << c->file_name << " (" << c->file_id << ") to "
                             << d->file_name << " (" << d->file_id << ") (D) propagating time\n";
                    activity++;
                }
                else
                {
                    if( !d->is_target )
                    {
                        activity++;
                        
                        if( dominates && d->user_selected )
                        {
                            d->is_target = true;
                            hash_set_add( all_targets, d->file_id);
                            
                            if( verbosity > 0 )
                                cout << "target planner   adding "  << d->file_name << " (" << d->file_id << ") to target set\n";
                            
                            make_targets_by_dom_set( d );              // propagate target state to all dependend nodes
                            
                            if( d->downward_size == 0 )
                                final_targets.push_back( d->file_id );
                        }
                        else  // nothing to do here, so we move on
                        {
                            hash_set_add( front_ids, d->file_id);
                            if( verbosity > 2 )
                                cout << "target planner   moving on from " << c->file_name << " (" << c->file_id << ") to "
                                     << d->file_name << " (" << d->file_id << ") (non D) propagating time\n";
                        }
                    }
                }
            }  // for( k = 0; k < c->downward_size; k++)
            
            hash_set_remove( front_ids, c->file_id);
        }
        free( a );
        
        if( verbosity > 2 )
            cout << "target planner   activity = "  << activity << " ...\n";
    }
    while( activity > 0 );
    
    delete_hash_set( front_ids );

    validCmdsLastRound = 0;
    errors = 0;
}


void Engine::move_wavefront()
{
    int activity;
    
    do{
        activity = 0;
        if( verbosity > 1 )
            cout << "work planner   wavefront size = "  << hash_set_get_size( wavefront_ids ) << "\n";
        
        unsigned int i, s;
        int *a = hash_set_get_as_array( wavefront_ids, &s);
        
        for( i = 0; i < s; i++)
        {
            command_t *c = find_command( a[i] );
            
            if( !c->is_done )
            {
                if( prereqs_done( c ) )
                {
                    if( c->is_target )
                    {
                        if( !c->has_failed )
                        {
                            if(  !c->in_to_do && 
                                 !hash_set_has_id( in_work_set, c->file_id)  )
                            {
                                c->in_to_do = true;
                                if( verbosity > 0 )
                                    cout << "work planner   adding "  << c->file_name << " (" << c->file_id << ") to to-do set\n";
                                hash_set_add( target_root_ids, c->file_id);
                                activity++;
                            }
                        }
                        else
                        {
                            hash_set_remove( wavefront_ids, c->file_id);
                            activity++;
                        }
                    }
                    else
                    {
                        if( strcmp( c->dep_type, "W") != 0 )
                        {
                            c->is_done = true;
                            hash_set_add( done_set, c->file_id);
                            activity++;
                            if( verbosity > 1 )
                                cout << "work planner   setting "  << c->file_name << " (" << c->file_id << ") done\n";
                        }
                    }
                }
            }
            else
            {
                for( int w = 0; w < c->weak_size; w++)
                {
                    command_t *wait = find_command( c->weak[w] );
                    wait->is_done = true;
                }
                hash_set_remove( global_mbd_set, c->file_id);
                
                hash_set_remove( wavefront_ids, c->file_id);  // towards the end this will remove all final targets
                activity++;

                if( c->downward_deep )
                {
                    for( int k = 0; k < c->downward_size; k++)
                    {
                        command_t *down = c->downwards[k];
                        
                        if( verbosity > 2 )
                            cout << "work planner   moving on from " << c->file_name << " (" << c->file_id << ") to "
                                 << down->file_name << " (" << down->file_id << ")\n";
                        hash_set_add( wavefront_ids, down->file_id);
                    }
                }
                else
                {
                    if( verbosity > 1 )
                         cout << "work planner   not going deep at " << c->file_name << " (" << c->file_id << ")\n";
                }
            }
        }
        free( a );
        
        if( verbosity > 2 )
            cout << "work planner   activity = "  << activity << " ...\n";
    }
    while( activity > 0 );
}


void Engine::build_to_do_array()
{
    int i = 0, s = hash_set_get_size( to_do_set );
    
    to_do_cmd_pos = 0;
    delete to_do_cmd_ids;
    to_do_cmd_ids = new command_t *[ s ];
    
    size_t j;
    for( j = 0;  j < file_ids.size(); j++)
    {
        if( hash_set_has_id( to_do_set, file_ids[j]) )
        {
            assert( i < s );
            
            to_do_cmd_ids[ i++ ] = find_command( file_ids[j] );
        }
    }

    to_do_cmd_size = i;
}


ExecutorCommand Engine::nextCommand()
{
    if( stopOnError && hash_set_get_size( failed_set ) > 0 )
    {
        return ExecutorCommand( "FINALIZE" );
    }
    
    if( hash_set_get_size( to_do_set ) < 30 )
    {
        if( verbosity > 0 )
            cout << "TARGETS LEFT: " << hash_set_get_size( targets_left )<<"\n";

        move_wavefront();
    }
    
    if( hash_set_get_size( target_root_ids ) > 0 )
    {
        if( verbosity > 0 )
            cout << " NEW TARGETS: " << hash_set_get_size( target_root_ids ) << "\n";
        hash_set_union( to_do_set, target_root_ids);
        hash_set_clear( target_root_ids );
        
        build_to_do_array();
    }
    
    if( hash_set_get_size( targets_left ) > 0 && hash_set_get_size( to_do_set ) == 0 )
    {
        if( hash_set_get_size( in_work_set ) > 0 )
        {
            round++;
            validCmdsLastRound = 0;
            cout << "-------------- ROUND " << round << " --------------\n";
            return ExecutorCommand( "BARRIER" );
        }
        else
            return ExecutorCommand( "FINALIZE" );
    }
    else if( hash_set_get_size( targets_left ) == 0 )
    {
        cout << "\nNo targets left.\n";
        return ExecutorCommand( "FINALIZE" );
    }
    
    ExecutorCommand ec;
    
    int validCmds = 0;

    if( hash_set_get_size( to_do_set ) > 0 )
    {
        assert( to_do_cmd_pos < to_do_cmd_size );
        command_t *c = to_do_cmd_ids[ to_do_cmd_pos++ ];
        
        char *dep_type = c->dep_type;
        
        if( prereqs_done( c ) == 0 )
        {
            cerr << "internal error: tried to reach unreachable " << c->file_name << " target\n";
            exit( 11 );
        }
        
        if( strcmp( dep_type, "W") == 0 )             // a wait command from .msg (or other extension that creates more than one file)
        {
            if( verbosity > 0 )
                cout << "# waiting for " << c->file_name << "\n";
        }
        else
        {
            string script = generateScript( c );
            if( script.length() == 0 )
            {
                c->has_failed = true;
                hash_set_add( failed_set, c->file_id);
                cerr << "Target " << c->file_name << " has failed through internal error\n";
                errors++;
            }
            else
            {
                if( verbosity > 0 )
                    cout << "using " << script << " to produce " << c->file_name << "\n";
                ec = ExecutorCommand( script, c->file_id);
                validCmds++;

                ec.addFileToRemoveAfterSignal( c->file_name );
                for( int w = 0; w < c->weak_size; w++)
                {
                    command_t *wait = find_command( c->weak[w] );
                    ec.addFileToRemoveAfterSignal( wait->file_name );
                }
                
                hash_set_remove( to_do_set, c->file_id);
                hash_set_add( in_work_set, c->file_id);
            }
        }
    }
    
    cout << "To do: " << hash_set_get_size( to_do_set )
         << " / in work: " << hash_set_get_size( in_work_set )
         << " / failed: " << hash_set_get_size( failed_set ) << "\n";
    
    validCmdsLastRound += validCmds;
    
    if( validCmds > 0 )
    {
        return ec;
    }
    else if( errors > 0 )
    {
        cout << "\nerrors. sending finalize.\n";
        return ExecutorCommand( "FINALIZE" );
    }
    else
    {
        round++; cout << " sending BARRRIER 2\n";
        validCmdsLastRound = 0;
        return ExecutorCommand( "BARRIER" );
    }
}


void Engine::indicateDone( int file_id, long long curr_time)           // called by executor
{
    command_t *c = find_command( file_id );
    assert( c );
    
    hash_set_remove( in_work_set, c->file_id);
    hash_set_remove( targets_left, c->file_id);
    
    bool exists = queryFiles.exists( c->file_name );
    bool proper = false;    
    if( exists )
    {
        const File f = queryFiles.getFile( c->file_name );
        if( f.getTimeMs() > (startTimeMs - (scfs ? 1000 : 0) ) )            // file must be newer than ferret's start
            proper = true;

        if( scfs && proper )
            c->scfs_time = curr_time;
    }
    
    bool all_proper = false;
    if( proper )
    {
        int wait_fails = 0;
        
        // check all wait nodes via weak dependencies (only very few have one, like msg)
        if( c->weak_size > 0 )
        {
            for( int i = 0; i < c->weak_size; i++)
            {
                command_t *w = find_command( c->weak[i] );
                assert( w );
                
                bool w_exists = queryFiles.exists( w->file_name );
                bool w_proper = false;
                if( w_exists )
                {
                    const File wf = queryFiles.getFile( w->file_name );
                    if( wf.getTimeMs() > (startTimeMs - (scfs ? 1000 : 0))  )       // wait file must be newer than ferret's start
                        w_proper = true;
                } 
                
                if( w_proper )
                {
                    w->is_done = true;
                    hash_set_add( done_set, w->file_id);

                    if( scfs )
                        w->scfs_time = curr_time;
                }
                else
                {
                    w->has_failed = true;
                    wait_fails++;
                    // hash_set_add( failed_set, w->file_id);
                    cerr << "Waiting for " << w->file_name << " has FAILED\n";
                    OutputCollector::getTheOutputCollector()->setError( w->file_id, true);
                }
            }
        }

        if( wait_fails == 0 )
        {
            c->is_done = true;
            hash_set_add( done_set, c->file_id);
            all_proper = true;
        }
    }
    
    if( !all_proper )
    {
        c->has_failed = true;
        hash_set_add( failed_set, c->file_id);
        cerr << "Target " << c->file_name << " has FAILED\n";
        OutputCollector::getTheOutputCollector()->setError( c->file_id, true);
    }
    else
        hash_set_remove( global_mbd_set, c->file_id);
}


void Engine::traverseUserTargets( command_t *c, int level)
{
    if( c->find_target_root_visited )
        return;
    
    c->user_selected = true;
    
    int k;
    for( k = 0; k < c->deps_size; k++)
    {
        command_t *d = c->upwards[k];
        if( strcmp( d->dep_type, "D") != 0 && strcmp( d->dep_type, "W") != 0 )
            traverseUserTargets( d, level+1);
    }
    
    c->find_target_root_visited = true;
}


void Engine::checkUserTargets( const set<string> &userTargets )
{
    size_t i;
    
    if( userTargets.size() > 0 )
    {
        int n = 0;
        
        for( i = 0; i < file_ids.size(); i++)
        {
            command_t *c = find_command( file_ids[i] );

            set<string>::const_iterator it = userTargets.begin();
            for( ; it != userTargets.end(); it++)
            {
                const string &userTarget = *it;
                
                if( string( c->file_name ).find( userTarget ) != string::npos
                    && strcmp( c->dep_type, "D") != 0 && strcmp( c->dep_type, "W") != 0 )
                {
                    traverseUserTargets( c );
                    n++;
                }
            }
        }
        
        cout << n << " user given target(s) found.\n";
    }
    else
    {
        for( i = 0; i < file_ids.size(); i++)
        {
            command_t *c = find_command( file_ids[i] );
            
            if( strcmp( c->dep_type, "D") != 0 && strcmp( c->dep_type, "W") != 0 )
                c->user_selected = true;
        }
    }
    
    //cout << "user selected " << hash_set_get_size( all_targets ) << " of " << all << " possible targets.\n";
}


void Engine::analyzeResults()
{
    cerr << hash_set_get_size( all_targets ) - hash_set_get_size( failed_set ) << " target(s) not reachable.\n";
    //cerr << "Missing targets:\n";
    unsigned int i,s;
    int *a = hash_set_get_as_array( all_targets, &s);
            
    for( i = 0; i < s; i++)
    {
        command_t *c = find_command( a[i] );

        if( hash_set_has_id( failed_set, c->file_id) )
            cerr << "Failed     : " << c->file_name << "\n";
        else
        {
            // cerr << "  Unreachable: ";
            OutputCollector::getTheOutputCollector()->appendErrHtml( c->file_id, string( "Target " ) + c->file_name);
            OutputCollector::getTheOutputCollector()->setError( c->file_id, true);
        }
        
        if( !c->has_failed )
        {
            //cerr << ". Prerequisites missing:\n";
            OutputCollector::getTheOutputCollector()->appendErrHtml( c->file_id, " not reachable, as prerequisites are missing:\n<table>");
                    
            int k;
            for( k = 0; k < c->deps_size; k++)
            {
                command_t *d = find_command( c->deps[k] );
                        
                if( !d->is_done )
                {
                    OutputCollector::getTheOutputCollector()->appendErrHtml( c->file_id, "<tr class=\"target\"><td>");
                    //cerr << "      " << d->file_name << ", ";
                    OutputCollector::getTheOutputCollector()->appendErrHtml( c->file_id, string( d->file_name ) + "</td><td>\n" );
                            
                    stringstream ss;
                    ss << d->file_id;
                    if( d->has_failed )
                    {
                        //cerr << "which has failed";
                        OutputCollector::getTheOutputCollector()->appendErrHtml( c->file_id, "which has failed ");
                        OutputCollector::getTheOutputCollector()->appendErrHtml( c->file_id, "<a href=\"#" + ss.str() + "\">errors</a>");
                    }
                    else
                    {
                        //cerr << "which is not reachable";
                        OutputCollector::getTheOutputCollector()->appendErrHtml( c->file_id, "which is not reachable");
                        OutputCollector::getTheOutputCollector()->appendErrHtml( c->file_id, " <a href=\"#" + ss.str() + "\">follow</a>");
                    }
                            
                    if( !d->is_target )
                    {
                        //cerr << " but surprisingly is not a target";
                        OutputCollector::getTheOutputCollector()->appendErrHtml( c->file_id, " but surprisingly is not a target");
                    }
                            
                    //cerr << "\n";
                    OutputCollector::getTheOutputCollector()->appendErrHtml( c->file_id, "</td></tr>\n");
                }
            }
            OutputCollector::getTheOutputCollector()->appendErrHtml( c->file_id, "</table>\n");
        }
    }
    free( a );
}


int Engine::doWork( ExecutorBase &executor, bool printTimes, const set<string> &userTargets)
{
    init_timer();
    
    doPointers();    
    readScfsTimes();
    
    traverse_for_dominator_sets();       // for this, the dependency graph needs to be cycle free (i.e. must be a DAG)
    if( printTimes )
        cout << "traverse dependencies for dominators took " << get_diff() << "s\n";
    
    checkUserTargets( userTargets );
    
    fill_target_set();
    
    FindFiles::clearCache();
    if( printTimes )
        cout << "checking file state and timestamps took " << get_diff() << "s\n";
    round = 1;
    hash_set_union( targets_left, all_targets);
    
    if( hash_set_get_size( all_targets ) > 0 )
    {
        cout << final_targets.size() << " final target(s).\n";
        
        cout << "-------------- ROUND 1 --------------\n";
        
        executor.processCommands( *this );   // do it!

        // print out what's missing
        if( hash_set_get_size( failed_set ) > 0 )
            cerr << "\n" << hash_set_get_size( failed_set ) << " target(s) failed.\n";
        
        hash_set_minus( all_targets, done_set);         // does all_targets = all_targets MINUS done_set, giving all that could not be done
        
        if( hash_set_get_size( all_targets ) > 0 )
        {
            analyzeResults();
        }
        
        writeScfsTimes();
    }
    else
        cout << "Target set empty. Nothing to do for our beloved ferret.\n";
    
    if( printTimes )
        cout << "build took " << get_diff() << "s\n";

    if( !executor.isInterruptedBySignal() )
    {
        ofstream osh( "last_run.html" );
        OutputCollector::getTheOutputCollector()->html( osh );
        
        ofstream ost( "last_run.txt" );
        OutputCollector::getTheOutputCollector()->text( ost );
    }
    
    return 0;
}
 
// -----------------------------------------------------------------------------

void MakefileEngine::addMakeCommand( data_t *cmd )
{
    files.push_back( cmd );
}


int MakefileEngine::doWork( ExecutorBase &executor, bool printTimes, const set<string> &userTargets)
{
    init_timer();
    
    fp = fopen( fileName.c_str(), "w");
    if( !fp )
    {
        cerr << "error: could not open output file.\n";
        errors++;
        return -1;
    }
    
    all_final_targets();
    writeCommands();

    fclose( fp );
    
    if( printTimes )
        cout << "makefile generation and write took " << get_diff() << "s\n";
    
    return 0;
}


ExecutorCommand MakefileEngine::nextCommand()
{
    assert( 0 );
}


void MakefileEngine::all_final_targets()
{
    fputs( "# ferret generated makefile\n\nall:", fp);
    
    for( pos = 0; pos < files.size(); pos++)
    {
        data_t *c = files[ pos ];
        
        if( c->cmd != "D" && hash_set_get_size( c->downward_deps_set ) == 0 )
            fprintf( fp, " %s", c->file_name.c_str());
    }
    
    fputs( "\n\n", fp);
}


void MakefileEngine::writeCommands()
{
    for( pos = 0; pos < files.size(); pos++)
    {
        data_t *c = files[ pos ];
        
        if( c->cmd == "D" )      // write out make dependency for .h/.cpp
        {
            if( hash_set_get_size( c->deps_set ) > 0 )
            {
                fprintf( fp, "\n%s:", c->file_name.c_str());
                unsigned int i,s;
                int *a = hash_set_get_as_array( c->deps_set, &s);
                
                for( i = 0; i < s; i++)
                    fprintf( fp, " %s", fileMan.getFileForId( a[i] ).c_str());
                fputs( "\n", fp);
                free( a );
            }
        }
        else if( c->cmd == "W" )             // a wait command
        {
            fprintf( fp, "\n# waiting for %s\n", c->file_name.c_str());
        }
        else
        {
            writeCommand( c );
        }        
    }
}


string MakefileEngine::generateScript( data_t *c )
{
    return genScript( c->file_name, c->file_id);
}


void MakefileEngine::writeCommand( data_t *cmd )
{
    string script = generateScript( cmd );

    if( script.length() == 0 )
    {
        errors++;
        return;
    }
    
    // write out a make rule for target cmd->file_name
    fprintf( fp, "\n%s:", cmd->file_name.c_str());
    
    if( hash_set_get_size( cmd->deps_set ) > 0 )
    {
        unsigned int i,s;
        int *a = hash_set_get_as_array( cmd->deps_set, &s);
        
        for( i = 0; i < s; i++)
            fprintf( fp, " %s", fileMan.getFileForId( a[i] ).c_str());
        
        free( a );
    }
    fprintf( fp, "\n\t/bin/sh %s\n\n", script.c_str());  // command to build target
}
