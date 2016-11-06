#ifndef FERRET_HASH_SET_H_
#define FERRET_HASH_SET_H_

typedef struct hs_bucket {
    int file_id;
    struct hs_bucket *next;
} hash_set_bucket_t;

typedef struct hash_set {
    hash_set_bucket_t *buckets;
    int  table_size;
    unsigned int size;
} hash_set_t;


hash_set_t *new_hash_set( int size );  // always use a prime number for size

void delete_hash_set( hash_set_t *hs );

void hash_set_add( hash_set_t *hs, int file_id);
void hash_set_remove( hash_set_t *hs, int file_id);

int hash_set_has_id( hash_set_t *hs, int file_id);

void hash_set_clear( hash_set_t *hs );

void hash_set_union( hash_set_t *hs_union, hash_set_t *hs_add); // add all entries from hs_add to hs_union

void hash_set_minus( hash_set_t *hs, hash_set_t *hs_minus);

int hash_set_get_size( hash_set_t *hs );

int *hash_set_get_as_array( hash_set_t *hs, unsigned int *size);

int hash_set_get_first( hash_set_t *hs );   // removes first that it finds

int hash_set_compare( hash_set_t *hs, hash_set_t *other);

void hash_set_print( hash_set_t *hs );

void hash_set_print_deps( FILE *fp, int from_id, const hash_set_t *hs, char t);

void hash_set_write( FILE *fp, hash_set_t *hs);
void hash_set_read( FILE *fp, hash_set_t *hs );

#endif
