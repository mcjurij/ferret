#ifndef FERRET_FILE_MAP_H_
#define FERRET_FILE_MAP_H_

#include <string>
#include <vector>
#include <set>


class Engine;
class MakefileEngine;

struct hash_set;
class BaseNode;

typedef struct data {
    int file_id;
    std::string file_name;
    std::string cmd;
    typedef enum { UNTOUCHED = 0, TOUCHED} file_state_t;
    typedef enum { UNCHANGED = 0, NEW, GONE, DEP_CHANGED, RESULT, RESULT_DEP_CHANGED} structural_state_t;
    file_state_t state;
    structural_state_t structural_state;
    
    BaseNode *baseNode;
    
    struct hash_set *deps_set;              // upward, same as in file db
    struct hash_set *downward_deps_set;     // downward, meaning: all files that point to this file
    
    struct hash_set *weak_set;              // upward weak dependency, example: msg node -> header node
    struct hash_set *downward_weak_set;     // downward, example: header node -> yac node
    
    struct hash_set *blocked_deps_set;

    bool mark_by_deps_changed;
    
    bool marked_by_delete;
    bool marked_by_delete_visited;
    
    int find_root_visited, find_root_visited_cycle;
} data_t;

typedef struct bucket {
    data_t *data;
    struct bucket *next;
} bucket_t;

typedef struct hash_map {
    bucket_t **buckets;
    int  size;
} hash_map_t;


class FileMap
{
public:
    class Iterator {
        friend class FileMap;
        
    public:
        Iterator()
            : current_bucket(-1), current_next(0)
        {}

        std::string getFile() const;
        int getId() const;
        std::string getCmd() const;
        const BaseNode *getBaseNode() const;
        data_t::structural_state_t getStructuralState() const;
        
        const struct hash_set *getDepsSet() const;
        const struct hash_set *getWeakDepsSet() const;
        const struct hash_set *getBlockedDepsSet() const;
        
    private:
        int current_bucket;
        struct bucket *current_next;
    };
    
public:
    FileMap( int size );  // always use a prime number for size

    ~FileMap();
    
    void add( int file_id, const std::string &file_name, BaseNode *baseNode, const std::string &cmd = "D", data_t::structural_state_t ss = data_t::UNCHANGED);
    void remove( int file_id );

private:
    void removeAllDeps( data_t *n );
    void removeAllWeakDeps( data_t *n );
    
public:
    void setDbReadMode( bool dbmode );
    void addDependency( int from_id, int to_id);
    void removeDependency( int from_id, int to_id);
    
    void addWeakDependency( int from_id, int to_id);
    void removeWeakDependency( int from_id, int to_id);
    void addBlockedDependency( int from_id, int to_id);
    bool hasBlockedDependency( int from_id, int to_id);
    
    bool hasNext( Iterator &it ) const;    // iterator
    
private:
    bucket_t *rmDeps( int file_id );
public:
    // void removeDependencies( int file_id );
    void replaceDependencies( int file_id, struct hash_set *new_deps_set);
    int  compareDependencies( int file_id, struct hash_set *against);
    
    int  hasId( int file_id );
    int  hasFileName( const std::string &fn );
    int  getIdForFileName( const std::string &fn );
    std::string getCmdForFileName( const std::string &fn );
    std::string getFileNameForId( int file_id );
    std::string getCmdForId( int file_id );
    
    hash_set *getDependencies( int id );
    hash_set *prerequisiteFor( int id );
    BaseNode *getBaseNodeFor( int id );
    
    bool hasDependency( int from_id, int to_id);
    int whatsChanged();
    int cleanupDeletions();
    void persistMarkByDeletions( struct hash_set *mbd_set );
    void markByDeletion( int id, int level = 0);
    
    int  getSize() const;
    void setState( const std::string &fn, data_t::file_state_t s);
    data_t::file_state_t getState( const std::string &fn );
    data_t::structural_state_t getStructuralState( const std::string &fn );
    
    //void writeFiles( FILE *fp );
    //void writeDeps( FILE *fp );
    
private:
    void markByDeletions( const std::set<int> &gone_set );
    void findRoots( data_t *c, int level = 0);

public:
    struct hash_set *findRoots();
    
    void sendToEngine( Engine &engine );
    void sendToMakefileEngine( MakefileEngine &engine );
    
    void print();

private:
    hash_map_t *hashmap,           // map from id to data
               *hashmap_s;         // map from string to data
    bool dbReadMode;               // true while reading the files db
    std::vector<int>  file_ids;    // to preserve order
};

#endif
