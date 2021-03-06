#ifndef FERRET_ENGINE_H_
#define FERRET_ENGINE_H_

#include <vector>
#include <set>

#include "hash_set.h"
#include "file_map.h"
#include "executor.h"
#include "find_files.h"

class FileManager;

class EngineBase
{
    
public:
    EngineBase()
        : errors(0), barrierMode(false)
    {}
    
    virtual int doWork( ExecutorBase &executor, bool printTimes, const std::set<std::string> &userTargets = std::set<std::string>()) = 0;
    virtual ExecutorCommand nextCommand() = 0;
    virtual void indicateDone( int file_id, unsigned int job_id, long long curr_time);
    
protected:
    int errors;
    bool barrierMode;
};


// calls ferret_dep.sh to produce *.d files
class DepEngine : public EngineBase
{
public:
    DepEngine()
        : EngineBase()
    {}
    
    void addDepCommand( ExecutorCommand &ec );
    
    virtual int doWork( ExecutorBase &executor, bool printTimes, const std::set<std::string> &userTargets = std::set<std::string>());
    virtual ExecutorCommand nextCommand();
    virtual void indicateDone( int file_id, unsigned int job_id, long long curr_time);
    
private:
    std::vector<ExecutorCommand> commands;
    size_t pos;
};


// sync tag in platform XML, /platform/tool/sync
class SyncEngine : public EngineBase
{
public:
    SyncEngine()
        : EngineBase()
    {}
    
    void addSyncCommand( ExecutorCommand ec );
    
    virtual int doWork( ExecutorBase &executor, bool printTimes, const std::set<std::string> &userTargets = std::set<std::string>());
    virtual ExecutorCommand nextCommand();
    virtual void indicateDone( int file_id, unsigned int job_id, long long curr_time);
    
    bool hasCommands() const
    { return commands.size(); }
    
private:
    std::vector<ExecutorCommand> commands;
    size_t pos;
};


class BaseNode;

// build engine
class Engine : public EngineBase
{
    typedef struct command
    {
        int file_id;
        unsigned int job_id;
        char dep_type[15];
        char *file_name;
        
        BaseNode *baseNode;
        
        int *deps;
        int deps_size;
        struct command **upwards;
        
        int *weak;
        int weak_size;
        
        int downward_cap;
        struct command **downwards;
        int downward_size;
        bool downward_deep;          /* if true (default) all downward deps are created and thus followed,
                                        if false no downward deps are created and thus this node becomes a final target
                                     */
        bool marked_by_deletion, marked_by_deps_changed;
        
        hash_set_t *dominator_set;   // downwards, not to be confused with "dominating set"
        
        bool find_target_root_visited;
        bool in_to_do;
        bool is_target;
        long long dominating_time;
        bool is_done;
        bool has_failed;
        bool user_selected;         // true for all targets if no user targets given
        long long scfs_time;        // only valid if scfs is active
        
    } command_t;
    

private:
    Engine()
    {}
    
public:
    Engine( int table_size /* unused */, const std::string &dbProjDir, const std::string &compileMode, bool scfs);
    ~Engine();
    
    void addCommand( data_t *cmd );
        
    virtual int doWork( ExecutorBase &executor, bool printTimes, const std::set<std::string> &userTargets = std::set<std::string>());

    void cursesEnable( bool en )
    { curses = en; }
    
private:
    void analyzeResults();
    
    command_t *init_entry( data_t *cmd );
    void add_downward_ptr( command_t *c, command_t *up);
    command_t *find_command( int file_id );
    char *find_filename( int file_id );
    
    void doPointers();
    void writeScfsTimes();
    void readScfsTimes();
    
    hash_set_t *traverse_union_tree( command_t *e, int level);
    void traverse_for_dominator_sets();
    
    bool prereqs_done( command_t *c );
    std::string generateScript( command_t *c );
    
    bool make_target_by_wait( command_t *c );
    void make_targets_by_dom_set( command_t *c );
    void fill_target_set();

    void move_wavefront();
    
    void build_to_do_array();
    
public:
    virtual ExecutorCommand nextCommand();
    virtual void indicateDone( int file_id, unsigned int job_id, long long curr_time);

    void setStopOnError( bool stop )
    { stopOnError = stop; }
    
private:
    void traverseUserTargets( command_t *c, int level = 0);
    void checkUserTargets( const std::set<std::string> &userTargets );
    
private:
    std::vector<int>  file_ids;   // to preserve order
    std::string dbProjDir;
    std::string compileMode;
    bool curses;
    
    typedef struct fe_bucket {
        command_t *entry;
        struct fe_bucket *next;
    } fe_bucket_t;
    
#define FILE_TABLE_SIZE 977
    fe_bucket_t files[ FILE_TABLE_SIZE ];

    std::vector<int> final_targets;
    hash_set_t *all_targets, *targets_left;
    hash_set_t *to_do_set, *in_work_set, *done_set, *failed_set;
    int traverse_targets_run;
    hash_set_t *target_root_ids;
    hash_set_t *wavefront_ids;
    command_t **to_do_cmd_ids;    // array that contains all from to_do_set
    int to_do_cmd_pos, to_do_cmd_size;
    
    int round, validCmdsLastRound;
    bool scfs, stopOnError;

    QueryFiles queryFiles;
};


// Makefile engine for --make
class MakefileEngine : public EngineBase {
 
public:
    MakefileEngine( FileManager &fileMan, const std::string &fn)
        : EngineBase(), fileMan(fileMan), fileName(fn), fp(0)
    {}
    
    void addMakeCommand( data_t *cmd );
    
    virtual int doWork( ExecutorBase &executor, bool printTimes, const std::set<std::string> &userTargets = std::set<std::string>());
    virtual ExecutorCommand nextCommand();

    
private:
    void all_final_targets();
    void writeCommands();
    std::string generateScript( data_t *c );
    void writeCommand( data_t *cmd );
    
private:
    FileManager &fileMan;
    std::string  fileName;
    std::vector<data_t *> files;
    size_t pos;
    FILE *fp;
};

#endif

