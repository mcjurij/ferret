#ifndef FERRET_PLATFORM_DEFINES_H_
#define FERRET_PLATFORM_DEFINES_H_

#include <string>

#include "defines.h"


// singleton wrapper around defines
class PlatformDefines {
    
public:
    static PlatformDefines *getThePlatformDefines();
    
    PlatformDefines()
    {}

    void set( const std::string &name, const std::string &value);
    std::string getValue( const std::string &name );
    
    std::string replace( const std::string &in );
    
private:
    static PlatformDefines *thePlatformDefines;
    
    Defines defines;    
};

#endif
