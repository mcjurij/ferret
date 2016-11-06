#include <stdlib.h>
#include <assert.h>
#include <stdio.h>
#include <iostream>

#include "hash_set.h"

using std::cout;

static int hash( hash_set_t *hs, int file_id)
{
    return file_id % hs->table_size;
}

hash_set_t *new_hash_set( int table_size )
{
    int i;
    if( table_size < 5 )
        table_size = 5;
    
    hash_set_t *hs = (hash_set_t *)malloc( sizeof(hash_set_t) );
    hs->table_size = table_size;
    hs->buckets = (hash_set_bucket_t *)malloc( sizeof(hash_set_bucket_t) * hs->table_size );
    hs->size = 0;
    
    for( i=0; i<hs->table_size; i++)
    {
        hs->buckets[i].file_id = -1;
        hs->buckets[i].next = 0;
    }
    
    return hs;
}

void delete_hash_set( hash_set_t *hs )
{
    int i;
    for( i=0; i<hs->table_size; i++)
    {
        hash_set_bucket_t *h = &( hs->buckets[i] );
        if( h->next != 0 )
        {
            h = h->next;
            do {
                hash_set_bucket_t *tmp = h;
                h = h->next;
                free( tmp );
            }
            while( h != 0 );
        }
    }
    
    free( hs->buckets );
    hs->buckets = 0;
    free( hs );
}


void hash_set_add( hash_set_t *hs, int file_id)
{
    int b = hash( hs, file_id);
    hash_set_bucket_t *h = &( hs->buckets[b] );
    
    if( h->file_id == file_id )
        return;  // do nothing, file id already in set
    
    if( h->file_id == -1 )
    {
        h->file_id = file_id;
        hs->size++;
    }
    else
    {
        while( h->next != 0 )
        {
            h = h->next;
            if( h->file_id == file_id )
                return;  // do nothing, file id already in set
        }
        
        hash_set_bucket_t *n = (hash_set_bucket_t *)malloc( sizeof(hash_set_bucket_t) );
        h->next = n;
        n->file_id = file_id;
        n->next = 0;
        hs->size++;
    }
}


void hash_set_remove( hash_set_t *hs, int file_id)
{
    int b = hash( hs, file_id);
    hash_set_bucket_t *h = &( hs->buckets[b] );
    
    if( h->file_id == file_id )
    {
        h->file_id = -1;
        hs->size--;
    }
    else
    {
        hash_set_bucket_t *del = 0;
        while( h->next != 0 )
        {
            if( h->next->file_id == file_id )
            {
                del = h->next;
                break;  // found
            }
            h = h->next;            
        }

        if( del == 0 )
            return;     // not found
        h->next = del->next;

        hs->size--;
        free( del );
    }
}


static hash_set_bucket_t *hash_set_find( hash_set_t *hs, int file_id)
{
    hash_set_bucket_t *h = &( hs->buckets[ hash( hs, file_id) ] );
    assert( file_id != -1 );

    if( h->file_id == file_id )
        return h;
    else
    {
        while( h->next != 0 )
        {
            h = h->next;
            if( h->file_id == file_id )
                return h;
        }
    }
    
    return 0;
}


int hash_set_has_id( hash_set_t *hs, int file_id)
{
    return (hash_set_find( hs, file_id) != 0 ) ? 1 : 0;
}


void hash_set_clear( hash_set_t *hs )
{
    int i;
    for( i=0; i<hs->table_size; i++)
    {
        hash_set_bucket_t *h = &( hs->buckets[i] );
        if( h->next != 0 )
        {
            h = h->next;
            do {
                hash_set_bucket_t *tmp = h;
                h = h->next;
                free( tmp );
            }
            while( h != 0 );
        }

        hs->buckets[i].file_id = -1;
        hs->buckets[i].next = 0;
    }
    
    hs->size = 0;
}


void hash_set_union( hash_set_t *hs_union, hash_set_t *hs_add) // copy all entries from hs_add to hs_union
{
    int i;
    assert( hs_union );
    assert( hs_add );
    if( hs_union == hs_add )
        return;
    
    for( i=0; i < hs_add->table_size; i++)   // sure, the bigger hs_add, the slower
    {
        hash_set_bucket_t *h = &( hs_add->buckets[ i ] );
        
        if( h->file_id != -1 )
        {
            hash_set_add( hs_union, h->file_id);
        }
        
        while( h->next != 0 )
        {
            h = h->next;
            hash_set_add( hs_union, h->file_id);
        }
    }
}


void hash_set_minus( hash_set_t *hs, hash_set_t *hs_minus)
{
    int i;
    assert( hs );
    assert( hs_minus );
    if( hs == hs_minus )
    {
        hash_set_clear( hs );
        return;
    }
    
    for( i = 0; i < hs_minus->table_size; i++)   // sure, the bigger hs_minus, the slower
    {
        hash_set_bucket_t *h = &( hs_minus->buckets[ i ] );
        
        if( h->file_id != -1 )
        {
            hash_set_remove( hs, h->file_id);
        }
        
        while( h->next != 0 )
        {
            h = h->next;
            hash_set_remove( hs, h->file_id);
        }
    }
}


int hash_set_get_size( hash_set_t *hs )
{
    return hs->size;
}


int *hash_set_get_as_array( hash_set_t *hs, unsigned int *s)
{
    *s = hs->size;
    int *a = (int *)malloc( sizeof(int) * *s );
    int i;
    unsigned int k=0;
    
    for( i=0; i < hs->table_size; i++)
    {
        hash_set_bucket_t *h = &( hs->buckets[ i ] );
        
        if( h->file_id != -1 )
        {
            a[ k ] = h->file_id;
            k++;
        }
        
        while( h->next != 0 )
        {
            h = h->next;
            a[ k ] = h->file_id;
            k++;
        }
    }
    assert( *s == k );
    
    return a;
}


int hash_set_get_first( hash_set_t *hs )   // removes first that it finds
{
    int i;
    int found_it = -1;

    if( hash_set_get_size( hs ) == 0 )
        return -1;

    bool done = false;
    for( i=0; !done && i < hs->table_size; i++)
    {
        hash_set_bucket_t *h = &( hs->buckets[ i ] );
        
        if( h->file_id != -1 )
        {
            found_it = h->file_id;
            done = true;
        }
        
        while( !done && h->next != 0 )
        {
            h = h->next;
            found_it = h->file_id;
            done = true;
        }
    }

    assert( found_it != -1 );
    
    hash_set_remove( hs, found_it);

    return found_it;
}


int hash_set_compare( hash_set_t *hs,  hash_set_t *other)
{
    int i;
    int diff = 0;
    
    for( i=0; i < other->table_size; i++)
    {
        hash_set_bucket_t *oh = &( other->buckets[ i ] );
        
        if( oh->file_id != -1 )
        {
            if( !hash_set_has_id( hs, oh->file_id) )
            {
                //printf( " New dep id: %d\n", oh->file_id);
                diff++;
            }
        }
        
        while( oh->next != 0 )
        {
            oh = oh->next;
            if( !hash_set_has_id( hs, oh->file_id) )
            {
                //printf( " New dep id: %d\n", oh->file_id);
                diff++;
            }
        }
    }

    for( i=0; i < hs->table_size; i++)
    {
        hash_set_bucket_t *h = &( hs->buckets[ i ] );
        
        if( h->file_id != -1 )
        {
            if( !hash_set_has_id( other, h->file_id) )
            {
                //printf( " Gone: %d\n", h->file_id);
                diff++;
            }
        }
        
        while( h->next != 0 )
        {
            h = h->next;
            if( !hash_set_has_id( other, h->file_id) )
            {
                //printf( " Gone: %d\n", h->file_id);
                diff++;
            }
        }
    }

    return diff;
}


void hash_set_print( hash_set_t *hs )
{
    int i;
    for( i=0; i < hs->table_size; i++)
    {
        hash_set_bucket_t *h = &( hs->buckets[ i ] );
        
        if( h->file_id != -1 )
            cout << " " << h->file_id;
        
        while( h->next != 0 )
        {
            h = h->next;
            cout << " " << h->file_id;
        }
    }
}


void hash_set_print_deps( FILE *fp, int from_id, const hash_set_t *hs, char t)       // t=='>' => dependency, t=='<' => weak dependency
{
    int i;
    for( i=0; i < hs->table_size; i++)
    {
        hash_set_bucket_t *h = &( hs->buckets[ i ] );
        
        if( h->file_id != -1 )
        {
            fprintf( fp, "%d-%c%d\n", from_id, t, h->file_id);
        }
        
        while( h->next != 0 )
        {
            h = h->next;
            fprintf( fp, "%d-%c%d\n", from_id, t, h->file_id);
        }
    }
}


void hash_set_write( FILE *fp, hash_set_t *hs)
{
    int i;
    for( i=0; i < hs->table_size; i++)
    {
        hash_set_bucket_t *h = &( hs->buckets[ i ] );
        
        if( h->file_id != -1 )
        {
            fprintf( fp, " %d", h->file_id);
        }
        
        while( h->next != 0 )
        {
            h = h->next;
            fprintf( fp, " %d", h->file_id);
        }
    }
}


void hash_set_read( FILE *fp, hash_set_t *hs )
{
    hash_set_clear( hs );

    while( !feof( fp ) )
    {
        int id;
        if( fscanf( fp, " %d", &id) == EOF )
            break;
        hash_set_add( hs, id);
    }
}
