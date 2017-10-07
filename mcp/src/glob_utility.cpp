#include <sstream>
#include <cstdio>
#include <time.h>    // for clock_gettime()
#include <sys/stat.h>
#include <iostream>

#include "glob_utility.h"
#include "find_files.h"

using namespace std;

string scriptTemplDir;
string tempScriptDir;
string tempDepDir;
string ferretDbDir;

hash_set_t *global_mbd_set;

long long startTimeMs;
int verbosity;     // -v,-vv or -vvv flag
bool show_info;   // --info flag

string tempDir;

static struct timespec lastt;

void init_timer()
{
    clock_gettime( CLOCK_REALTIME, &lastt);
}


string get_diff()
{
    struct timespec curr;
    clock_gettime( CLOCK_REALTIME, &curr);
    
    double diff = (double)(curr.tv_sec - lastt.tv_sec) +
                  (double)(curr.tv_nsec - lastt.tv_nsec) * 1.e-9;
    lastt.tv_sec = curr.tv_sec;
    lastt.tv_nsec = curr.tv_nsec;

    char buf[10];
    sprintf( buf, "%.3f", diff);
    
    return buf;
}


void set_start_time()
{
    long    ms; // milliseconds
    time_t  s;  // seconds
    
    clock_gettime( CLOCK_REALTIME, &lastt);

    s  = lastt.tv_sec;
    ms = lastt.tv_nsec / 1.0e6; // nanoseconds to milliseconds

    startTimeMs = (long long)s*1000 + (long long)ms;
}


long long get_curr_time_ms()
{
    long    ms; // milliseconds
    time_t  s;  // seconds
    struct timespec spec;
    clock_gettime( CLOCK_REALTIME, &spec);

    s  = spec.tv_sec;
    ms = spec.tv_nsec / 1.0e6; // nanoseconds to milliseconds

    return (long long)s*1000 + (long long)ms;
}


string join( const string &sep1, const vector<string> &a, bool beforeFirst, const string &sep2)
{
    size_t i;
    string s;
    int n = 0;
    for( i=0; i < a.size(); i++)
    {
        if( a[i].length() > 0 )
        {
            if( n==0 && beforeFirst )
                s.append( sep1 );
            else if( n>0 )
                s.append( sep1 );
            n++;
            s.append( a[i] );
            s.append( sep2 );
        }
    }
    
    return s;
}


string joinUniq( const string &sep1, const vector<string> &a, bool beforeFirst)
{
    size_t i;
    set<string> s;
    vector<string> uniq;
    
    for( i=0; i < a.size(); i++)
    {
        if( a[i].length() > 0 )
        {
            set<string>::iterator it = s.find( a[i] );
            if( it == s.end() )
            {
                s.insert( a[i] );
                uniq.push_back( a[i] );
            }
        }
    }
    
    return join( sep1, uniq, beforeFirst);
}


vector<string> split( char where, const string &s)
{
   vector<string> r;
   
   size_t len = s.length();
   size_t i;
   
   for( i = 0; i < len; i++)
   {
       while( i < len && s[i] == where )
           i++;
       string part;
       
       while( i < len && s[i] != where )
       {
           part += s[i];
           i++;
       }
       
       if( part.length() > 0 )
           r.push_back( part );
   }
   
   return r;
}


string cleanPath( const string &in )
{
    if( in.length() == 0 )
        return "";
    string slashAtStart = in[0] == '/' ? "/" : "";
    
    vector<string> parts, sp = split( '/', in);
    parts.reserve( sp.size() );
    parts.resize( sp.size() );
    size_t i, stackpos = 0;
    for( i = 0; i < sp.size(); i++)
    {
        const string &p = sp[i];
        
        if( p != "." && p != ".." )
            parts[stackpos++] = p;
        else if( p == ".." && stackpos>0)
            stackpos--;
    }

    if( stackpos == 0 && slashAtStart == "" )
    {
        parts.resize( 1 );
        parts[0] = ".";
    }
    else
        parts.resize( stackpos );
    
    return slashAtStart + join( "/", parts, false);
}


string stackPath( const string &p1, const string &p2)
{
    if( p2.length() == 0 )
        return p1;
    
    if( p2[0] == '/' )
        return p2;
    
    if( p1.length() == 0 )
        return p2;
    
    return cleanPath( p1 + '/' + p2 );
}


void breakPath( const string &path, string &dir, string &bn)
{
    size_t pos = path.rfind( "/" );  // I know of dirname/basename but they are difficult to use
    
    if( pos == string::npos )
        bn = path;
    else
    {
        dir = (pos>0) ? path.substr( 0, pos) : "";
        bn = (pos+1)<path.length() ? path.substr( pos+1 ) : "";
    }
}


void breakPath( const string &path, string &dir, string &bn, string &ext)
{
    breakPath( path, dir, bn);

    size_t pos = bn.rfind( "." );
   
    if( pos != string::npos )
        ext = bn.substr( pos );
    else
        ext = "";
}


void breakFileName( const string &fn, string &bext, string &ext)
{
    size_t pos = fn.rfind( "." );
   
    if( pos != string::npos )
    {
        if( pos > 0 )
            bext = fn.substr( 0, pos);
        else
            bext = "";
        ext = fn.substr( pos );
    }
    else
    {
        bext = fn;
        ext = "";
    }
}


bool mkdir_p( const string &path )
{
    size_t lpos = 0;
    string d;
    string p_d;

    if( FindFiles::exists( path ) )
        return true;
    
    do
    {
        if( lpos < path.length() && path.at( lpos ) == '/' )
        {
            p_d += '/';
            lpos++;
        }

        if( lpos == path.length() )
            break;
        
        d = "";
        while( lpos < path.length() && path.at( lpos ) != '/' )
            d += path.at( lpos++ );
        
        if( d.length() > 0 )
        {
            p_d += d;
            if( p_d != "." )
                mkdir( p_d.c_str(),  S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
        }
    } while( lpos < path.length() );
    
    return true;
}


void breakBazelDep( const string &dep, string &dir, string &name)
{
    vector<string> parts = split( ':', dep);  // hopefully not more than one :

    if( parts.size() >= 2 )
    {
        dir = parts[0];
        name = parts[1];
    }
    else if( parts.size() == 1 )
    {
         dir = parts[0];
         name = "";
    }
    else
        dir = name = "";
}
