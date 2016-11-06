#ifndef FERRET_GLOB_UTILITY_H_
#define FERRET_GLOB_UTILITY_H_

#include <string>
#include <vector>

#include "hash_set.h"

extern std::string scriptTemplDir;

extern std::string tempDir;  // where ferret can put all its temporary files
extern std::string tempScriptDir;
extern std::string tempDepDir;
extern std::string ferretDbDir;

extern long long startTimeMs;
extern int verbosity;
extern bool show_info;

extern hash_set_t *global_mbd_set;

void init_timer();
std::string get_diff();  // for --times option
void set_start_time();
long long get_curr_time_ms();

std::string join( const std::string &sep1, const std::vector<std::string> &a, bool beforeFirst, const std::string &sep2 = "");
std::string joinUniq( const std::string &sep1, const std::vector<std::string> &a, bool beforeFirst);
std::vector<std::string> split( char where, const std::string &s);

std::string cleanPath( const std::string &in );
std::string stackPath( const std::string &p1, const std::string &p2);
void breakPath( const std::string &path, std::string &dir, std::string &bn);
void breakPath( const std::string &path, std::string &dir, std::string &bn, std::string &ext);
void breakFileName( const std::string &fn, std::string &bext, std::string &ext);

bool mkdir_p( const std::string &path );

#endif
