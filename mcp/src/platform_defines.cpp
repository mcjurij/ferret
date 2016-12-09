#include "platform_defines.h"

using namespace std;


PlatformDefines *PlatformDefines::thePlatformDefines = 0;

PlatformDefines *PlatformDefines::getThePlatformDefines()
{
    if( thePlatformDefines == 0 )
        thePlatformDefines = new PlatformDefines;

    return thePlatformDefines;
}


void PlatformDefines::set( const string &name, const string &value)
{
    defines.set( name, value);
}


string PlatformDefines::getValue( const string &name )
{
    return defines.getValue( name );
}


string PlatformDefines::replace( const string &in )
{
    return defines.replace( in );
}
