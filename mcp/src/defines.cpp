#include <cstdlib>
#include <iostream>

#include "defines.h"

using namespace std;

void Defines::set( const string &name, const string &value)
{
    if( name.length() > 0 )
        definesMap[ name ] = value;
}


string Defines::getValue( const string &name )
{
    map<string,string>::const_iterator it = definesMap.find( name );
    
    if( it != definesMap.end() )
        return it->second;
    else
    {
        char *env = getenv( name.c_str() );
        if( env )
        {
            string val(env);
            set( name, val);
            return val;
        }
        else
        {
            cout << "error: define named '" << name << "' not found\n";
            return "UNSET";
        }
    }
}


string Defines::replace( const string &in )
{
    string out;
    size_t i = 0;

    while( i < in.length() )
    {
        if( in[i] != '$' )
        {
            out += in[i];
            i++;
        }
        else if( (i+1) < in.length() && (in[i+1] == '{' || in[i+1] == '(') )  // start of key?
        {
            i++;  // consume $
            char o = in[i], c;
            i++;  // consume { or (
            
            if( o == '{' )
                c = '}';
            else
                c = ')';
            
            string key;
            while( i < in.length() && in[i] != c )   // consume key
            {
                key += in[i];
                i++;
            }
            
            if( i < in.length() && in[i] == c )
            {
                i++; // consume } or )
                
                if( key.length() > 0 )
                    out += getValue( key );
                else
                    cout << "error: define name empty for '" << in << "'\n";
            }
            else
                out += string("$") + o + key;    // key didn't end properly
        }
        else
        {
            out += in[i];
            i++;
        }
    }
    
    return out;
}


void Defines::copyInto( Defines &d )
{
    map<string,string>::iterator it = definesMap.begin();

    for( ; it != definesMap.end(); it++)
        d.set( it->first, it->second);
}
