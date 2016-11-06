#ifndef FERRET_BUILD_PROPS_H_
#define FERRET_BUILD_PROPS_H_

#include <string>
#include <map>

class ProjectXmlNode;

class PrjBuildProps
{
public:

    PrjBuildProps()
    {}
    
    PrjBuildProps( const std::string &odir, const std::string &bdir, const std::string &ldir)
        : objdir( odir ), bindir( bdir ), libdir( ldir )
    {}

    std::string getObjDir() const
    { return objdir; }

    std::string getBinDir() const
    { return bindir; }
 
    std::string getLibDir() const
    { return libdir; }

private:
    std::string objdir, bindir, libdir;
};


class BuildProps
{
    
public:
    static BuildProps *getTheBuildProps();
    
    BuildProps()
    {}

    bool read( const std::string &fn );
    
    bool hasKey( const std::string &key ) const;
    std::string getValue( const std::string &key ) const;  // not for OBJDIR, BINDIR, LIBDIR
    int getIntValue( const std::string &key ) const;
    bool getBoolValue( const std::string &key ) const;
    void setValue( const std::string &key, const std::string &value);  // not for OBJDIR, BINDIR, LIBDIR
    void setIntValue( const std::string &key, int value);
    void setBoolValue( const std::string &key, bool value);
    
    void setStaticProp( const std::string &key, const std::string &val);
    
    PrjBuildProps getPrjBuildProps( const ProjectXmlNode *node );

    
private:
    static BuildProps *theBuildProps;

    std::map<std::string,std::string> keyValMap;

    std::map<std::string,std::string> staticPropMap;
};

#endif
