#include <cstdio>
#include <sstream>
#include <iostream>
#include <cassert>
#include <cstdlib>

#include "script_template.h"
#include "glob_utility.h"
#include "find_files.h"

using namespace std;


static map<string,string> fileMap;

class ScriptTemplate
{
public:
    ScriptTemplate( const string &fn )
        : fn(fn)
    {}

    bool readTemplate();

    void replace( const map<string,string> &replMap);

    string write( const string &scriptfn, int file_id, const string &compile_mode);

private:
    bool hasNextChar() const
    { return pos < templContent.length(); }
    
    char consume();
    
    char getc() const
    { return templContent[pos]; }
    
private:
    string fn;
    string templContent;  // we slurp the file in
    string newContent;
    size_t pos;
};


static string findTemplate( const string &fn )
{
    vector<string> tries;
    string host;
    string user;
    
    if( getenv("HOSTNAME") )
        host = getenv("HOSTNAME");
    if( getenv("USER") )
        user = getenv("USER");

    if( user.length() > 0 )
        tries.push_back( scriptTemplDir + "/" + user + "/" + fn );
    if( host.length() > 0 )
        tries.push_back( scriptTemplDir + "/" + host + "/" + fn );
    tries.push_back( scriptTemplDir + "/" + fn );
    
    for( size_t i = 0; i < tries.size(); i++)
    {
        if( FindFiles::existsUncached( tries[i] ) )
            return tries[i];
    }
    
    return "";
}


bool ScriptTemplate::readTemplate()
{
    map<string,string>::iterator it = fileMap.find( fn );
    
    if( it == fileMap.end() )
    {
        stringstream ss;

        string templfn = findTemplate( fn );
        if( templfn == "" )
        {
            cerr << "error: did not find script template '" << fn << "'.\n";
            return false;
        }

        if( verbosity > 0 )
            cout << "Will be using '" << templfn << "' for script template " << fn << "\n";
        
        FILE *in_fp = fopen( templfn.c_str(), "r");
        
        if( !in_fp )
        {
            cerr << "error: could not open script template '" << templfn << "'.\n";
            return false;
        }
        
        int c;
        while( (c = fgetc( in_fp )) > 0 )
            ss << (char)c;                          // slurp it in, they aren't big
    
        templContent = ss.str();
        fclose( in_fp );
        
        fileMap[ fn ] = templContent;
    }
    else
        templContent = it->second;
    
    
    return true;
}


#define is_white(c) ((c)==' ' || (c)=='\t' || (c)=='\n')
/*
#define is_delim(c) ((c)=='/' || (c)=='-' || (c)=='+' || (c)=='<' || (c)=='>' || (c)=='$' || (c)=='\\' || (c)=='|' || (c)==',' || \
                     (c)=='\'' || (c)=='`' || (c)=='"' || (c)=='#' || (c)==';'|| (c)=='(' || (c)==')' || (c)=='{' || (c)=='}' || \
                     (c)=='.' || (c)=='@' || (c)=='~' || (c)=='%' || (c)=='?' || (c)=='&' || (c)=='!')
*/

static bool is_allowed( char c )
{
    return (c>='A' && c<='Z') || c=='_' || (c>='0' && c<='9') || (c>='a' && c<='z');
}


void ScriptTemplate::replace( const map<string,string> &replMap )
{
    newContent = "";
    stringstream out;
    string key;
    map<string, string>::const_iterator it;

    pos = 0;
    
    do
    {
        if( getc() == '$' )
        {
            consume();           // consume '$'
            key = "";
            
            while( hasNextChar() && !is_white(getc()) && is_allowed(getc()) )
                key += consume();           // consume the key
            
            if( key.length() > 2 )          // must start with F_
            {
                it = replMap.find( key );
                if( it != replMap.end() )
                    out << it->second;      // insert the value at the position of key
                else
                    out << '$' << key;   // ignore
            }
            else
                out << '$' << key;     // ignore
        }
        else
            out << consume();
    }
    while( hasNextChar() );

    newContent = out.str();
}


string ScriptTemplate::write( const string &scriptfn, int file_id, const string &compile_mode)
{
    string fn;
    size_t i;
    stringstream idss;

    if( file_id > 0 )
        idss << file_id;
    idss << "__" << compile_mode;
    
    for( i = 0; i < scriptfn.length(); i++)
        if( scriptfn[ i ] == '#' )
            fn.append( idss.str() );
        else
            fn += scriptfn[ i ];

    fn = tempScriptDir + "/" + fn;

    FILE *out_fp = fopen( fn.c_str(), "w");
    
    if( !out_fp )
    {
        cerr << "error: could not write script '" << fn << "'.\n";
        return "";
    }

    for( i = 0; i < newContent.length(); i++)
        fputc( newContent[i], out_fp);
    
    fclose( out_fp );

    return fn;
}


char ScriptTemplate::consume()
{
    assert( pos < templContent.length() );
    return templContent[pos++];
}


void ScriptInstance::addReplacements( const map<string,string> &repl )
{
    if( replacements.size() == 0 )
        replacements = repl;
    else
    {
        map<string,string>::const_iterator it = repl.begin();
        for( ; it != repl.end(); it++)
            replacements[ it->first ] = it->second;
    }
}


void ScriptInstance::addReplacement( const string &k, const string &v)
{
    replacements[ k ] = v;
}


string ScriptInstance::write( const string &target_fn, const string &compile_mode)
{    
    ScriptTemplate st( templ_name );
    if( !st.readTemplate() )
    {
        cerr << "error: command for file " << target_fn << " (" << file_id << "): error while reading script template.\n";
        return "";
    }
    
    st.replace( replacements );
    
    return st.write( stencil, file_id, compile_mode);  // returns file name of the written script
}


ScriptManager *ScriptManager::theScriptManager = 0;

ScriptManager *ScriptManager::getTheScriptManager()
{
    if( theScriptManager == 0 )
        theScriptManager = new ScriptManager;

    return theScriptManager;
}


void ScriptManager::setTemplateFileName( file_id_t file_id, const string &fn, const string &st)
{
    map<file_id_t,ScriptInstance>::iterator  it = instances.find( file_id );

    if( it == instances.end() )
        instances[ file_id ] = ScriptInstance( file_id, fn, st);
    else
    {
        cerr << "internal error: attempt to change script template file name\n";
    }
}


void ScriptManager::addReplacements( file_id_t file_id, const map<string,string> &repl)
{
    map<file_id_t,ScriptInstance>::iterator it = instances.find( file_id );
    
    if( it != instances.end() )
        it->second.addReplacements( repl );
    else
    {
        cerr << "internal error: unknown script template for adding replacements\n";
    }
}


void ScriptManager::addReplacement( file_id_t file_id, const string &k, const string &v)
{
    map<file_id_t,ScriptInstance>::iterator it = instances.find( file_id );
    
    if( it != instances.end() )
        it->second.addReplacement( k, v);
    else
    {
        cerr << "internal error: unknown script template for adding replacement\n";
    }
}


string ScriptManager::write( file_id_t file_id, const string &target_fn)
{
    map<file_id_t,ScriptInstance>::iterator it = instances.find( file_id );
    
    if( it != instances.end() )
        return it->second.write( target_fn, compile_mode);
    else
    {
        cerr << "internal error: command for file " << target_fn << " (" << file_id << ") does not have a script template attached to it.\n";
        return "";
    }
}
