#ifndef FERRET_DEFINES_H_
#define FERRET_DEFINES_H_

#include <string>
#include <map>

// used by platfrom XML and extensions XML
class Defines {

public:
    Defines()
    {}

    void set( const std::string &name, const std::string &value);
    std::string getValue( const std::string &name );
    
    std::string replace( const std::string &in );
    
private:
    std::map<std::string, std::string>  definesMap;    
};

#endif
