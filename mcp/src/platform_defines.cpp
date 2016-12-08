
#include "platform_defines.h"

#include <iostream>

using namespace std;


PlatformDefines *PlatformDefines::thePlatformDefines = 0;

PlatformDefines *PlatformDefines::getThePlatformDefines()
{
    if( thePlatformDefines == 0 )
        thePlatformDefines = new PlatformDefines;

    return thePlatformDefines;
}


void PlatformDefines::add( const string &name, const string &value)
{
    if( name.length() > 0 )
        definesMap[ name ] = value;
}


string PlatformDefines::getValue( const string &name ) const
{
    map<string,string>::const_iterator it = definesMap.find( name );
    if( it != definesMap.end() )
        return it->second;
    else
    {
        cout << "error: define named '" << name << "' not found\n";
        return "UNSET";
    }
}


string PlatformDefines::replace( const string &in )
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
        else if( (i+1) < in.length() && (in[i+1] == '{' || in[i+1] == '(') )
        {
            i++;  // consume $
            char o = in[i], c;
            i++;  // consume { or (

            if( o == '{' )
                c = '}';
            else
                c = ')';
                    
            string key;
            while( i < in.length() && in[i] != c )
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
                out += string("$") + o + key;
        }
        else
        {
            out += in[i];
            i++;
        }
    }

    return out;
}
