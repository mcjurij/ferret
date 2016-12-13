#ifndef FERRET_SCRIPT_TEMPLATE_H_
#define FERRET_SCRIPT_TEMPLATE_H_

#include <string>
#include <map>

#include "glob_utility.h"


class ScriptInstance {
public:
    ScriptInstance()
        : file_id(-1)
    {}
    
    ScriptInstance( file_id_t fid, const std::string &templ_fn,  const std::string &st)
        : file_id(fid), templ_name(templ_fn), stencil(st)
    {}
    
    // void setTemplateFileName( const std::string &fn )
    // { templ_name = fn; }

    // void setStencil( const std::string &st )
    // { stencil = st; }

    void addReplacements( const std::map<std::string,std::string> &repl );

    void addReplacement( const std::string &k, const std::string &v);

    std::string write( const std::string &target_fn );  // returns file name of the written script
    
private:
    file_id_t file_id;
    
    std::string templ_name;
    std::string stencil;
    
    std::map<std::string,std::string> replacements;
};


// singleton for scripts
class ScriptManager {
    
public:
    static ScriptManager *getTheScriptManager();
    
    ScriptManager()
    {}

    void setTemplateFileName( file_id_t file_id, const std::string &fn, const std::string &st);

    // void setStencil( file_id_t file_id, const std::string &st);
    
    void addReplacements( file_id_t file_id, const std::map<std::string,std::string> &repl);
    
    void addReplacement( file_id_t file_id, const std::string &k, const std::string &v);
    
    std::string write( file_id_t file_id, const std::string &target_fn);
    
private:
    static ScriptManager *theScriptManager;

    std::map<file_id_t,ScriptInstance> instances;
};

#endif
