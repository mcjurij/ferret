#include <stdlib.h>
#include <assert.h>
#include <stdio.h>
#include <iostream>
#include <cstring>
#include <algorithm>

#include "file_map.h"
#include "hash_set.h"
#include "engine.h"
#include "project_xml_node.h"
#include "glob_utility.h"

using namespace std;

#define DEP_HASH_SET_SIZE 29
#define WEAK_HASH_SET_SIZE 3

// a hash function for integers. easy.
static unsigned int hash_id( hash_map_t *hm, int file_id)
{
    return file_id % hm->size;
}


// this is a very fast hash for strings
static unsigned int hash_s( hash_map_t *hm, const string &s)
{
    const char *key = s.c_str();
    int len = s.length();
    size_t loop;
    unsigned int h;

#define HASH4 h = (h << 5) + h + *key++;

    h = 0;
    loop = len >> 3;
    switch (len & (8 - 1)) {
    case 0:
        break;
    case 7:
        HASH4;
        /* FALLTHROUGH */
    case 6:
        HASH4;
        /* FALLTHROUGH */
    case 5:
        HASH4;
        /* FALLTHROUGH */
    case 4:
        HASH4;
        /* FALLTHROUGH */
    case 3:
        HASH4;
        /* FALLTHROUGH */
    case 2:
        HASH4;
        /* FALLTHROUGH */
    case 1:
        HASH4;
    }
    while (loop--) {
        HASH4;
        HASH4;
        HASH4;
        HASH4;
        HASH4;
        HASH4;
        HASH4;
        HASH4;
    }
    return h % hm->size;
}


static bucket_t *hash_map_find( hash_map_t *hm, int file_id)
{
    assert( file_id != -1 );
    bucket_t *h = hm->buckets[ hash_id( hm, file_id) ];
    
    if( !h )
        return 0;
    
    assert( h->data != 0 );
    
    do
    {
        if( h->data->file_id == file_id )
            return h;
        h = h->next;
    }
    while( h );
    
    return 0;
}


static bucket_t *hash_map_find_s( hash_map_t *hm, const string &fn)
{
    bucket_t *h = hm->buckets[ hash_s( hm, fn) ];

    if( !h )
        return 0;
    
    assert( h->data != 0 );
    
    do
    {
        if( h->data->file_name == fn )
            return h;
        h = h->next;
    }
    while( h );
    
    return 0;
}


string FileMap::Iterator::getFile() const
{
    assert( current_next != 0 );
    assert( current_next->data != 0 );
    
    return current_next->data->file_name;
}

int FileMap::Iterator::getId() const
{
    assert( current_next != 0 );
    assert( current_next->data != 0 );
    
    return current_next->data->file_id;
}

string FileMap::Iterator::getCmd() const
{
    assert( current_next != 0 );
    assert( current_next->data != 0 );
    
    return current_next->data->cmd;
}

const ProjectXmlNode *FileMap::Iterator::getXmlNode() const
{
    assert( current_next != 0 );
    assert( current_next->data != 0 );
    
    return current_next->data->xmlNode;
}

data_t::structural_state_t FileMap::Iterator::getStructuralState() const
{
    assert( current_next != 0 );
    assert( current_next->data != 0 );
    
    return current_next->data->structural_state;
}

const hash_set_t *FileMap::Iterator::getDepsSet() const
{
    assert( current_next != 0 );
    assert( current_next->data != 0 );
    
    return current_next->data->deps_set;
}

const hash_set_t *FileMap::Iterator::getWeakDepsSet() const
{
    assert( current_next != 0 );
    assert( current_next->data != 0 );
    
    return current_next->data->weak_set;
    
}

const hash_set_t *FileMap::Iterator::getBlockedDepsSet() const
{
    assert( current_next != 0 );
    assert( current_next->data != 0 );
    
    return current_next->data->blocked_deps_set;
}


FileMap::FileMap( int size )
{
    int i;
    if( size < 5 )
        size = 5;
    
    hashmap = (hash_map_t *)malloc( sizeof(hash_map_t) );
    hashmap->size = size;
    hashmap->buckets = (bucket_t **)malloc( sizeof(bucket_t *) * hashmap->size );
    
    hashmap_s = (hash_map_t *)malloc( sizeof(hash_map_t) );
    hashmap_s->size = size;
    hashmap_s->buckets = (bucket_t **)malloc( sizeof(bucket_t *) * hashmap_s->size );
    
    for( i=0; i<hashmap->size; i++)
    {
        hashmap->buckets[i] = 0;
        hashmap_s->buckets[i] = 0;
    }
}


FileMap::~FileMap()
{
    int i;
    for( i=0; i<hashmap->size; i++)
    {
        bucket_t *h = hashmap->buckets[i];

        while( h )
        {
            bucket_t *tmp = h;
            delete_hash_set( h->data->deps_set );
            delete_hash_set( h->data->downward_deps_set );
            delete_hash_set( h->data->weak_set );
            delete_hash_set( h->data->downward_weak_set );
            delete_hash_set( h->data->blocked_deps_set );
            delete h->data;
            h = h->next;
            free( tmp );
        }
    }
    
    free( hashmap->buckets );
    hashmap->buckets = 0;
    free( hashmap );

    for( i=0; i<hashmap_s->size; i++)
    {
        bucket_t *h = hashmap_s->buckets[i];
        
        while( h )
        {
            bucket_t *tmp = h;
            h->data = 0;    // already deleted above
            h = h->next;
            free( tmp );
        }
    }

    free( hashmap_s->buckets );
    hashmap_s->buckets = 0;
    free( hashmap_s );
}


void FileMap::add( int file_id, const string &file_name, ProjectXmlNode *node, const string &cmd, data_t::structural_state_t ss)
{
    // node can be null
    
    int b = hash_id( hashmap, file_id);
    bucket_t *h = hashmap->buckets[b];
      
    if( h && h->data->file_id == file_id )
        return;  // do nothing, file id already in set
    
    int b_s = hash_s( hashmap_s, file_name);
    bucket_t *h_s = hashmap_s->buckets[b_s];
  
    data_t *nd = 0;
    if( h == 0 )
    {
        bucket_t *n = (bucket_t *)malloc( sizeof(bucket_t) );
        n->next = 0;
        nd = new data_t;
        n->data = nd;
        hashmap->buckets[b] = n;
    }
    else
    {
        while( h->next != 0 )
        {
            h = h->next;
            if( h->data->file_id == file_id )
                return;                // do nothing, file id already in set
        }
        
        bucket_t *n = (bucket_t *)malloc( sizeof(bucket_t) );
        h->next = n;
        nd = new data_t;       
        n->data = nd;
        n->next = 0;
    }

    if( nd )
    {
        file_ids.push_back( file_id );
        
        nd->file_id = file_id;
        nd->file_name = file_name;
        nd->deps_set = new_hash_set( DEP_HASH_SET_SIZE );
        nd->downward_deps_set = new_hash_set( DEP_HASH_SET_SIZE );
        nd->weak_set = new_hash_set( WEAK_HASH_SET_SIZE );
        nd->downward_weak_set = new_hash_set( WEAK_HASH_SET_SIZE );
        nd->blocked_deps_set = new_hash_set( 5 );
        nd->state = (ss == data_t::NEW || ss == data_t::DEP_CHANGED) ? data_t::TOUCHED : data_t::UNTOUCHED;
        nd->structural_state = ss;
        nd->cmd = cmd;
        nd->xmlNode = node;
        if( node )
            node->addFileId( file_id );
        nd->mark_by_deps_changed = false;
        nd->marked_by_delete = false;
        nd->marked_by_delete_visited = false;
        nd->find_root_visited = 0;
        nd->find_root_visited_cycle = 0;
        
        if( h_s == 0 )
        {
            bucket_t *n = (bucket_t *)malloc( sizeof(bucket_t) );
            n->next = 0;
            n->data = nd;
            hashmap_s->buckets[b_s] = n;
        }
        else
        {
            while( h_s->next != 0 )           // find end of list
                h_s = h_s->next;
            
            bucket_t *n = (bucket_t *)malloc( sizeof(bucket_t) );
            h_s->next = n;
            n->data = nd;
            n->next = 0;
        }
    }
}


void FileMap::removeAllDeps( data_t *n )
{
    unsigned int i,s;
    int *a = hash_set_get_as_array( n->downward_deps_set, &s);
    
    // step #1 downwards  - tell all that point to us, that we'll go away
    for( i = 0; i < s; i++)
    {
        int other_id = a[ i ];
        removeDependency( other_id, n->file_id);
    }
    free( a );
    
    // step #2 upwards    - remove dependencies to all others
    a = hash_set_get_as_array( n->deps_set, &s);
    for( i = 0; i < s; i++)
    {
        int other_id = a[ i ];
        removeDependency( n->file_id, other_id);
    }
    free( a );

    hash_set_clear( n->blocked_deps_set );

    n->mark_by_deps_changed = true;
}


void FileMap::remove( int file_id )
{
    assert( hasId( file_id ) );
    unsigned int b = hash_id( hashmap, file_id);
    bucket_t *h = hashmap->buckets[ b ];
    
    if( h == 0 )
        return;
    
    string save = getFileNameForId( file_id );

    if( h->data->file_id == file_id )
    {
        removeAllDeps( h->data );
        removeAllWeakDeps( h->data );
        
        if( h->data->xmlNode )
            h->data->xmlNode->removeFileId( file_id );
        
        h->data = 0;
        hashmap->buckets[ b ] = h->next;
        free( h );
    }
    else
    {
        bucket_t *del = 0;
        while( h->next != 0 )
        {
            if( h->next->data->file_id == file_id )
            {
                del = h->next;
                break;  // found
            }
            h = h->next;            
        }
        
        if( del )
        {
            removeAllDeps( del->data );
            removeAllWeakDeps( del->data );

            if( del->data->xmlNode )
                del->data->xmlNode->removeFileId( file_id );
        }
        else
            return;     // not found
        
        h->next = del->next;
        free( del );
    }
    
    vector<int>::iterator p = find( file_ids.begin(), file_ids.end(), file_id);
    file_ids.erase( p );

    unsigned int b_s = hash_s( hashmap_s, save);
    bucket_t *h_s = hashmap_s->buckets[ b_s ];

    assert( h_s );
    
    if( h_s->data->file_name == save )
    {
        data_t *tmp = h_s->data;
        h_s->data = 0;
        hashmap_s->buckets[ b_s ] = h_s->next;
        delete_hash_set( tmp->deps_set );
        delete_hash_set( tmp->downward_deps_set );
        delete_hash_set( tmp->weak_set );
        delete_hash_set( tmp->downward_weak_set );
        delete_hash_set( tmp->blocked_deps_set );
        delete tmp;
        free( h_s );
    }
    else
    {
        bucket_t *del = 0;
        while( h_s->next != 0 )
        {
            if( h_s->next->data->file_name == save )
            {
                del = h_s->next;
                break;  // found
            }
            h_s = h_s->next;
        }
        
        assert( del != 0 );  // must be there
        
        delete_hash_set( del->data->deps_set );
        delete_hash_set( del->data->downward_deps_set );
        delete_hash_set( del->data->weak_set );
        delete_hash_set( del->data->downward_weak_set );
        delete_hash_set( del->data->blocked_deps_set );
        delete del->data;
        
        h_s->next = del->next;
        free( del );
    }
}


void FileMap::setDbReadMode( bool dbmode )
{
    dbReadMode = dbmode;
}


static void setSturcturalStateWhenDepsChanged( data_t *d )
{
    if( d->structural_state != data_t::NEW )        // new is stronger
    {
        if( d->cmd == "D" )
            d->structural_state = data_t::DEP_CHANGED;
        else
            d->structural_state = data_t::RESULT_DEP_CHANGED;
    }
}


void FileMap::addDependency( int from_id, int to_id)
{
    assert( hasId( from_id ) );
    assert( hasId( to_id ) );

    bucket_t *b = hash_map_find( hashmap, from_id);

    if( !hash_set_has_id( b->data->deps_set, to_id) && !hash_set_has_id( b->data->blocked_deps_set, to_id) )
    {
        hash_set_add( b->data->deps_set, to_id);
        
        bucket_t *other = hash_map_find( hashmap, to_id);
        hash_set_add( other->data->downward_deps_set, from_id);
        
        if( !dbReadMode )
        {
            setSturcturalStateWhenDepsChanged( b->data );
            b->data->mark_by_deps_changed = true;
        }
    }
}


void FileMap::removeDependency( int from_id, int to_id)
{
    assert( hasId( from_id ) );
    assert( hasId( to_id ) );
    
    data_t *f = hash_map_find( hashmap, from_id)->data;
    
    if( hash_set_has_id( f->deps_set, to_id ) )
    {
        data_t *other_n = hash_map_find( hashmap, to_id)->data;
        
        hash_set_remove( other_n->downward_deps_set, f->file_id);
        hash_set_remove( f->deps_set, other_n->file_id);
        
        setSturcturalStateWhenDepsChanged( f );

        hash_set_remove( f->blocked_deps_set, to_id);

        f->mark_by_deps_changed = true;
    }
}


void FileMap::addWeakDependency( int from_id, int to_id)
{
    assert( hasId( from_id ) );
    assert( hasId( to_id ) );
    
    data_t *b = hash_map_find( hashmap, from_id)->data;
    
    if( !hash_set_has_id( b->weak_set, to_id ) )
    {
        hash_set_add( b->weak_set, to_id);

        bucket_t *other = hash_map_find( hashmap, to_id);
        hash_set_add( other->data->downward_weak_set, from_id);
    }
}


void FileMap::removeWeakDependency( int from_id, int to_id)
{
    assert( hasId( from_id ) );
    assert( hasId( to_id ) );
    
    data_t *f = hash_map_find( hashmap, from_id)->data;
    
    if( hash_set_has_id( f->weak_set, to_id ) )
    {
        data_t *other_n = hash_map_find( hashmap, to_id)->data;
        
        hash_set_remove( other_n->downward_weak_set, f->file_id);
        hash_set_remove( f->weak_set, other_n->file_id);
    }
}


void FileMap::addBlockedDependency( int from_id, int to_id)
{
    assert( hasId( from_id ) );
    
    data_t *f = hash_map_find( hashmap, from_id)->data;
    
    if( !dbReadMode )
        hash_set_add( f->blocked_deps_set, to_id);
    else if( hasId( to_id ) )
        hash_set_add( f->blocked_deps_set, to_id);
}


bool FileMap::hasBlockedDependency( int from_id, int to_id)
{
    assert( hasId( from_id ) );
    data_t *f = hash_map_find( hashmap, from_id)->data;
    
    if( hash_set_has_id( f->blocked_deps_set, to_id) )
        return true;
    else
        return false;
}


bool FileMap::hasNext( Iterator &it ) const
{
    if( it.current_bucket < hashmap->size )
    {
        if( it.current_next != 0 && it.current_next->next != 0 )
        {
            it.current_next = it.current_next->next;
            return true;
        }
        else
        {
            it.current_bucket++;
            while( it.current_bucket < hashmap->size && hashmap->buckets[ it.current_bucket ] == 0 )
                it.current_bucket++;
            
            if( it.current_bucket < hashmap->size && hashmap->buckets[ it.current_bucket ] != 0 )
            {
                it.current_next = hashmap->buckets[ it.current_bucket ];
                return true;
            }
            else
                return false;
        }
    }
    else
        return false;
}


void FileMap::removeAllWeakDeps( data_t *n )
{
    unsigned int i,s;
    int *a = hash_set_get_as_array( n->downward_weak_set, &s);
    
    // step #1 downwards  - tell all that point to us, that we'll go away
    for( i = 0; i < s; i++)
    {
        int other_id = a[ i ];
        removeWeakDependency( other_id, n->file_id);
    }
    free( a );
    
    // step #2 upwards    - remove weak dependencies to all others
    a = hash_set_get_as_array( n->weak_set, &s);
    for( i = 0; i < s; i++)
    {
        int other_id = a[ i ];
        removeWeakDependency( n->file_id, other_id);
    }
    free( a );
}


bucket_t *FileMap::rmDeps( int file_id )
{
    bucket_t *b = hash_map_find( hashmap, file_id);
    assert( b );

    unsigned int i,s;
    int *a = hash_set_get_as_array( b->data->deps_set, &s);
    // tell all that we point to, that we remove all dependencies
    for( i = 0; i < s; i++)
    {
        int other_id = a[ i ];
        data_t *other_n = hash_map_find( hashmap, other_id)->data;
        
        hash_set_remove( other_n->downward_deps_set, b->data->file_id);
    }
    free( a );
    
    hash_set_clear( b->data->deps_set );
    
    setSturcturalStateWhenDepsChanged( b->data );
    b->data->mark_by_deps_changed = true;
    
    return b;
}


/*
void FileMap::removeDependencies( int file_id )
{
    rmDeps( file_id );
}
*/

void FileMap::replaceDependencies( int file_id, hash_set_t *new_deps_set)
{
    bucket_t *b = rmDeps( file_id );
    
    unsigned int i, sa;
    int *na = hash_set_get_as_array( new_deps_set, &sa);
    for( i = 0; i < sa; i++)
        if( !hash_set_has_id( b->data->blocked_deps_set, na[i]) )
            hash_set_add( b->data->deps_set, na[i]);
    free( na );
    
    unsigned int s;
    int *a = hash_set_get_as_array( b->data->deps_set, &s);
    // tell all that we point to, that we added all dependencies from new_deps_set
    for( i = 0; i < s; i++)
    {
        int other_id = a[ i ];
        data_t *other_n = hash_map_find( hashmap, other_id)->data;
        
        hash_set_add( other_n->downward_deps_set, b->data->file_id);
    }
    free( a );
}


int FileMap::compareDependencies( int file_id, hash_set_t *against)
{
    bucket_t *b = hash_map_find( hashmap, file_id);
    assert( b );

    // cout << "comparing " << b->data->file_id << ": "; hash_set_print( b->data->deps_set ); cout << " against ";  hash_set_print( against ) ; cout << "\n";
    return hash_set_compare( b->data->deps_set, against);
}


int FileMap::hasId( int file_id )
{
    return (hash_map_find( this->hashmap, file_id) != 0 ) ? 1 : 0;
}


int FileMap::hasFileName( const string &fn )
{
    return (hash_map_find_s( this->hashmap_s, fn) != 0 ) ? 1 : 0;
}


int FileMap::getIdForFileName( const string &fn )
{
    bucket_t *b = hash_map_find_s( this->hashmap_s, fn);
    if( b )
        return b->data->file_id;
    else
        return -1;
}


string FileMap::getCmdForFileName( const std::string &fn )
{
    bucket_t *b = hash_map_find_s( this->hashmap_s, fn);
    if( b )
        return b->data->cmd;
    else
        return "?";
}


string FileMap::getFileNameForId( int file_id )
{
    bucket_t *b = hash_map_find( this->hashmap, file_id);
    if( b )
        return b->data->file_name;
    else
        return "?";
}


string FileMap::getCmdForId( int file_id )
{
    bucket_t *b = hash_map_find( this->hashmap, file_id);
    if( b )
        return b->data->cmd;
    else
        return "?";
}

hash_set_t *FileMap::getDependencies( int id )
{
    bucket_t *b = hash_map_find( this->hashmap, id);
    if( b )
        return b->data->deps_set;
    else
        return 0;
}


hash_set_t *FileMap::prerequisiteFor( int id )
{
    bucket_t *b = hash_map_find( this->hashmap, id);
    if( b )
        return b->data->downward_deps_set;
    else
        return 0;
}


ProjectXmlNode *FileMap::getXmlNodeFor( int id )
{
    bucket_t *b = hash_map_find( this->hashmap, id);
    if( b )
        return b->data->xmlNode;
    else
        return 0;
}


bool FileMap::hasDependency( int from_id, int to_id)
{
    bucket_t *b = hash_map_find( this->hashmap, from_id);
    if( !b )
        return false;

    assert( b->data );
    if( hash_set_has_id( b->data->deps_set, to_id) != 0 )
        return true;
    else
        return false;
}


int FileMap::whatsChanged()
{
    int i;
    int cnt = 0;   // count all structural changes without changed depedencies
    
    for( i=0; i < hashmap->size; i++)
    {
        bucket_t *th = hashmap->buckets[ i ];
        
        while( th )
        {
            if( th->data->state == data_t::TOUCHED )
              cout << "Touched file id: " << th->data->file_id << "\n";
            
            if( th->data->structural_state == data_t::GONE )
            {
                cout << "Gone file id: " << th->data->file_id << "\n";
                cnt++;
            }
            else if( th->data->structural_state == data_t::NEW )
            {
                cout << "New file id: " << th->data->file_id << "\n";
                cnt++;
            }
            else if( th->data->structural_state == data_t::DEP_CHANGED )
            {
                cout << "Changed dependencies in: " << th->data->file_id << "\n";
            }
            
            th = th->next;
        }
    }

    return cnt;
}


int FileMap::cleanupDeletions()
{
    int i, cnt = 0;
    
    set<int> d;
    cout << " --------------------------------------------------\n";
    for( i=0; i < hashmap->size; i++)
    {
        bucket_t *th = hashmap->buckets[ i ];
        
        while( th )
        {
            if( th->data->structural_state == data_t::GONE )
            {
                cout << "can delete id: " << th->data->file_id << "\n";
                cnt++;
                
                d.insert( th->data->file_id );
            }
            else if( th->data->structural_state == data_t::RESULT || th->data->structural_state == data_t::RESULT_DEP_CHANGED )
            {                
                if( hash_set_get_size( th->data->deps_set ) == 0 )
                {
                    cout << "can delete id: " << th->data->file_id << " (unlinked result file)\n";
                    cnt++;

                    if( verbosity > 0 )
                        cout << "Removing stale file '" << getFileNameForId( th->data->file_id ) << "'\n";
                    ::remove( getFileNameForId( th->data->file_id ).c_str() );
                    
                    d.insert( th->data->file_id );
                }
            }
            
            th = th->next;
        }
    }

    markByDeletions( d );
    
    if( cnt )
    {
        set<int>::iterator it;
        
        for( i=0; i < hashmap->size; i++)
        {
            bucket_t *th = hashmap->buckets[ i ];
            
            while( th )
            {
                for( it = d.begin(); it != d.end(); it++)
                {
                    if( hash_set_has_id( th->data->deps_set, *it ) )
                    {
                        cout << "can delete dependency from " << th->data->file_id << " to " << *it << ".\n";

                        removeDependency( th->data->file_id, *it);
                    }
                }
                                
                th = th->next;
            }
        }

        for( it = d.begin(); it != d.end(); it++)
        {
            remove( *it );
            hash_set_remove( global_mbd_set, *it);
        }
    }
    
    return cnt;
}


void FileMap::persistMarkByDeletions( hash_set_t *mbd_set )
{
    if( hash_set_get_size( mbd_set ) == 0 )
        return;
    
    unsigned int i,s;
    int *a = hash_set_get_as_array( mbd_set, &s);
    
    for( i = 0; i < s; i++)
    {
        bucket_t *b = hash_map_find( this->hashmap, a[i]);

        if( b )
        {
            data_t *n = b->data;
            cout << "Mark " << n->file_id << " by old deletion.\n";
            
            n->marked_by_delete = true;
        }
        else
            cerr << "warning: could not mark " << a[i] << " by deletion.\n";
    }
    
    free( a );
}


void FileMap::markByDeletion( int id, int level)
{
    bucket_t *b = hash_map_find( this->hashmap, id);
    assert( b );
    data_t *n = b->data;
    
    if( n->marked_by_delete_visited || n->marked_by_delete )   // to prevent loops
        return;
    
    if( level >= 50 )
    {
        cerr << "error: mark by deletion recursion too deep.\n";
        return;
    }
    
    cout << "Mark " << n->file_id << " by deletion.\n";

    n->marked_by_delete = true;
    hash_set_add( global_mbd_set, id);
    
    unsigned int i,s;
    hash_set_t *prereq = prerequisiteFor( id );
    assert( prereq );
    int *a = hash_set_get_as_array( prereq, &s);
    
    for( i = 0; i < s; i++)
        markByDeletion( a[i], level+1);
    
    free( a );
    n->marked_by_delete_visited = true;
}


void FileMap::markByDeletions( const set<int> &gone_set )
{
    set<int>::const_iterator it;
    for( it = gone_set.begin(); it != gone_set.end(); it++)
        markByDeletion( *it );
}


int FileMap::getSize() const
{
    int i;
    int cnt = 0;
    
    for( i=0; i < hashmap->size; i++)
    {
        bucket_t *h = hashmap->buckets[ i ];
        
        while( h  )
        {
            h = h->next;
            cnt++;
        }
    }
    
    return cnt;
}


void FileMap::setState( const string &fn, data_t::file_state_t s)
{
    bucket_t *b = hash_map_find_s( hashmap_s, fn);
    assert( b );
    b->data->state = s;
}


data_t::file_state_t FileMap::getState( const string &fn )
{
    bucket_t *b = hash_map_find_s( hashmap_s, fn);
    assert( b );
    return b->data->state;
}


data_t::structural_state_t FileMap::getStructuralState( const string &fn )
{
    bucket_t *b = hash_map_find_s( hashmap_s, fn);
    assert( b );
    return b->data->structural_state;
}


static int froot_found_cycle;
static hash_set_t *froot_ids = 0;
static int froot_fid_stack[ 50 ];

void FileMap::findRoots( data_t *c, int level)
{
    if( level >= 50 )
    {
        cerr << "error: recursion level in find root too deep.\n";
        return;
    }
    
    froot_fid_stack[ level ] = c->file_id;
    
    c->find_root_visited_cycle++;

    if( c->find_root_visited_cycle>1 )
    {
        froot_found_cycle++;
        
        cout << "warning: visited " << getFileNameForId( c->file_id ) << " (" << c->file_id << ") already. via\n";
        int k;
        for( k=0; froot_fid_stack[ k ] != c->file_id; k++)
            ;
        for( ; k < level; k++)
            cout << "        " << getFileNameForId( froot_fid_stack[ k ] ) << " (" << froot_fid_stack[ k ] << ")\n";
        if( level > 0 )
        {
            cout << "        removing dependency from " << froot_fid_stack[ level-1 ] << " to " << c->file_id << "\n";
            removeDependency( froot_fid_stack[ level-1 ], c->file_id);
            addBlockedDependency( froot_fid_stack[ level-1 ], c->file_id);
        }
        
        return;
    }
    
    if( hash_set_get_size( c->deps_set ) == 0 )
    {
        hash_set_add( froot_ids, c->file_id);
    }
    else
    {
        unsigned int k, deps_size;
        int *deps_arr = hash_set_get_as_array( c->deps_set, &deps_size);
        
        for( k = 0; k < deps_size; k++)
        {
            data_t *d = hash_map_find( hashmap, deps_arr[k])->data;
            if( d->find_root_visited == 0 )
                findRoots( d, level+1);
        }

        free( deps_arr );
    }
    c->find_root_visited = 1;
}


hash_set_t *FileMap::findRoots()
{
    size_t i;
    froot_ids = new_hash_set( 499 );

    do {
        froot_found_cycle = 0;
        
        for( i = 0;  i < file_ids.size(); i++)
        {
            int id = file_ids[i];
            data_t *c = hash_map_find( hashmap, id)->data;
            c->find_root_visited = 0;
            c->find_root_visited_cycle = 0;
        }
        
        for( i = 0;  i < file_ids.size(); i++)
        {
            int id = file_ids[i];
            data_t *c = hash_map_find( hashmap, id)->data;
            
            if( c->find_root_visited == 0 )
            {
                memset( froot_fid_stack, 0, sizeof(int) * 50);
                findRoots( c );
            }
        }
    }
    while( froot_found_cycle>0 );
    
    return froot_ids;
}


void FileMap::sendToEngine( Engine &engine )
{
    int i;
    for( i=0; i < hashmap->size; i++)
    {
        bucket_t *h = hashmap->buckets[ i ];
        
        while( h )
        {
            engine.addCommand( h->data );
            h = h->next;
        }
    }
}

void FileMap::sendToMakefileEngine( MakefileEngine &engine )
{
    int i;
    for( i=0; i < hashmap->size; i++)
    {
        bucket_t *h = hashmap->buckets[ i ];
        
        while( h )
        {
            engine.addMakeCommand( h->data );
            h = h->next;
        }
    }
}


void FileMap::print()
{
    int i;
    for( i=0; i < hashmap->size; i++)
    {
        bucket_t *h = hashmap->buckets[ i ];
        
        while( h )
        {
            cout << " " << h->data->file_id << " " << h->data->file_name << " -> ";
            hash_set_print( h->data->deps_set );
            cout << "\n";
            
            h = h->next;
        }
    }
}
