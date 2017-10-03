#include <cstdio>
#include <iostream>
#include <sstream>
#include <cstdlib>

#include "build_props.h"
#include "glob_utility.h"
#include "base_node.h"

using namespace std;

// expand stuff in $...$
// OBJDIR=/nfss/as/Private/$user$/FO_FIFO_F05_05_00/obj/$host$/$compiler_version$/$compile_mode$/$proj_dir$
// BINDIR=/nfss/as/Private/$user$/FO_FIFO_F05_05_00/bin/$host$/$compiler_version$/$compile_mode$

// -----------------------------------------------------------------------------
BuildProps *BuildProps::theBuildProps = 0;

BuildProps *BuildProps::getTheBuildProps()
{
    if( theBuildProps == 0 )
        theBuildProps = new BuildProps;

    return theBuildProps;
}


static bool is_allowed_key( char c )
{
    return (c>='A' && c<='Z') || c=='_' || (c>='0' && c<='9') || (c>='a' && c<='z') || c=='.';
}

bool BuildProps::read( const string &fn )
{
    FILE *fpBp = fopen( fn.c_str(), "r");
    if( fpBp == 0 )
    {
        cerr << "error: build properties file " << fn << " not found\n";
        return false;
    }
    
    stringstream ss;
    
    int c;
    while( (c = fgetc( fpBp )) > 0 )
        ss << (char)c;
    
    string content = ss.str();

    fclose( fpBp );

    size_t i = 0, len = content.length();

    while( i < len )
    {
        string key;
        
        while( i < len && (content[i] == ' ' || content[i] == '\t' || content[i] == '\n') )        // skip white
            i++;

        if( i == len )
            break;
        
        if( content[i]=='#' )                      // comment
            i++;
        else
        {
            while( i < len && is_allowed_key( content[i] ) )
                key += content[i++];
            while( i < len && (content[i] == ' ' || content[i] == '\t') )
                i++;
            if( i < len && content[i] == '=' )
            {
                i++;
                if( i < len )
                {
                    while( i < len && (content[i] == ' ' || content[i] == '\t') )
                        i++;
                    
                    string val;
                    while( i < len && content[i] != '\n' )
                        val += content[i++];

                    if( val != "" )
                        val.erase( val.find_last_not_of( " \t" ) + 1 );        // right trim
                    
                    if( key != "" )
                        keyValMap[ key ] = val;
                }
            }
        }
        
        while( i < len && content[i] != '\n' )
            i++;
        if( i < len && content[i] == '\n' )
            i++;
    }

    
    return true;
}


bool BuildProps::hasKey( const string &key ) const
{
    map<string,string>::const_iterator it = keyValMap.find( key );
    
    return it != keyValMap.end();
}


string BuildProps::getValue( const string &key ) const
{
    map<string,string>::const_iterator it = keyValMap.find( key );

    if( it != keyValMap.end() )
        return it->second;
    else
        return "";
}


int BuildProps::getIntValue( const string &key ) const
{
    string v = getValue( key );
    if( v == "" )
        return 0;
    return atoi( v.c_str() );
}


bool BuildProps::getBoolValue( const string &key ) const
{
    string v = getValue( key );
    if( v == "" )
        return false;
    return v[0] == 'Y' || v[0] == 'y';
}


void BuildProps::setValue( const string &key, const string &value)
{
    keyValMap[ key ] = value;
}


void BuildProps::setIntValue( const string &key, int value)
{
    stringstream ss;
    ss << value;
    setValue( key, ss.str());
}


void BuildProps::setBoolValue( const std::string &key, bool value)
{
    setValue( key, value ? "Y" : "N");
}


void BuildProps::setStaticProp( const string &key, const string &val)
{
    if( key != "" )
        staticPropMap[ key ] = val;
}


static string getPrjValue( const map<string,string> &prjPropMap, const string &key)
{
    map<string,string>::const_iterator it = prjPropMap.find( key );
    if( it == prjPropMap.end() )
    {
        cerr << "error: no value for key '" << key << "' in build properties.\n";
        return "";
    }
    else
        return it->second;
}


static string replKeysWithPrjValues( const string &rs, const map<string,string> &prjPropMap)
{
    string prjval;
    size_t i = 0, len = rs.length();

    while( i < len )
    {
        while( i < len && rs[i] != '$' )
            prjval += rs[i++];

        if( i < len && rs[i] == '$' )
        {
            i++;
            string key;
            
            while( i < len && rs[i] != '$' )
                key += rs[i++];

            if( i < len && rs[i] == '$' )
            {
                i++;

                if( key != "" )
                    prjval += getPrjValue( prjPropMap, key);
            }
        }
    }

    return prjval;
}


PrjBuildProps BuildProps::getPrjBuildProps( const BaseNode *node )
{
    map<string,string> prjPropMap = staticPropMap;
    prjPropMap[ "proj_dir" ] = node->getDir();
    
    map<string,string>::const_iterator kvit = keyValMap.begin();   // key/value pairs from file

    string odir, bdir, ldir;
    
    for( ; kvit != keyValMap.end(); kvit++)
    {
        if( kvit->first == "OBJDIR" )
            odir = cleanPath( replKeysWithPrjValues( kvit->second, prjPropMap) ) + '/';
        else if( kvit->first == "BINDIR" )
            bdir = cleanPath( replKeysWithPrjValues( kvit->second, prjPropMap) ) + '/';
        else if( kvit->first == "LIBDIR" )
            ldir = cleanPath( replKeysWithPrjValues( kvit->second, prjPropMap) ) + '/';
    }

    if( ldir == "" )
        ldir = bdir;   // for people who do not set LIBDIR
    
    return PrjBuildProps( odir, bdir, ldir);
}
