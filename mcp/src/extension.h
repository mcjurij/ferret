#ifndef FERRET_EXTENSION_H_
#define FERRET_EXTENSION_H_

#include <string>
#include <map>
#include <set>

#include "file_manager.h"

void setupExtensions();


class ProjectXmlNode;

class ExtensionBase
{
    ExtensionBase();
    
public:
    ExtensionBase( ProjectXmlNode *node )
        : node(node)
    {}
    virtual ~ExtensionBase()
    {}
    
    ProjectXmlNode *getNode();
    
    virtual std::string getType() const = 0;   // msg, yac, flex

    virtual ExtensionBase *createExtensionDriver( ProjectXmlNode *node ) = 0;

    virtual std::map<file_id_t, std::map<std::string,std::string> > createCommands( FileManager &fileMan, const std::map<std::string,std::string> &xmlAttribs) = 0;

    virtual std::string getScriptTemplName() const = 0;
    virtual std::string getScriptName() const = 0;

    void addBlockedFileId( file_id_t id );

    void addBlockIncludesFileId( file_id_t id );
    
    std::set<file_id_t> getBlockedIds() const
    { return blockedIds; }
    
private:
    ProjectXmlNode *node;
    std::set<file_id_t> blockedIds;
    int groupId;
};


class CxxExtension : public ExtensionBase   // test & example
{

public:
    CxxExtension( ProjectXmlNode *node )
        : ExtensionBase(node)
    {}
    
    virtual std::string getType() const;

    virtual ExtensionBase *createExtensionDriver( ProjectXmlNode *node );

    virtual std::map<file_id_t, std::map<std::string,std::string> > createCommands( FileManager &fileMan, const std::map<std::string,std::string> &xmlAttribs);

    virtual std::string getScriptTemplName() const
    { return "ferret_cxx.sh.templ"; }
    
    virtual std::string getScriptName() const
    { return "ferret_cxx__#.sh"; }
};


class FlexExtension : public ExtensionBase
{

public:
    FlexExtension( ProjectXmlNode *node )
        : ExtensionBase(node)
    {}
    
    virtual std::string getType() const;
    virtual ExtensionBase *createExtensionDriver( ProjectXmlNode *node );
    
    virtual std::map<file_id_t, std::map<std::string,std::string> > createCommands( FileManager &fileMan, const std::map<std::string,std::string> &xmlAttribs);

    virtual std::string getScriptTemplName() const
    { return "ferret_flex.sh.templ"; }
    
    virtual std::string getScriptName() const
    { return "ferret_flex__#.sh"; }
};


class BisonExtension : public ExtensionBase
{
public:
    BisonExtension( ProjectXmlNode *node )
        : ExtensionBase(node)
    {}
    
    virtual std::string getType() const;
    virtual ExtensionBase *createExtensionDriver( ProjectXmlNode *node );

    virtual std::map<file_id_t, std::map<std::string,std::string> > createCommands( FileManager &fileMan, const std::map<std::string,std::string> &xmlAttribs);

    virtual std::string getScriptTemplName() const
    { return "ferret_bison.sh.templ"; }
    
    virtual std::string getScriptName() const
    { return "ferret_bison__#.sh"; }
};


class MsgExtension : public ExtensionBase  // for *.msg files
{

public:
    MsgExtension( ProjectXmlNode *node )
        : ExtensionBase(node)
    {}
    
    virtual std::string getType() const;
    virtual ExtensionBase *createExtensionDriver( ProjectXmlNode *node );

    virtual std::map<file_id_t, std::map<std::string,std::string> > createCommands( FileManager &fileMan, const std::map<std::string,std::string> &xmlAttribs);

    virtual std::string getScriptTemplName() const
    { return "ferret_msg.sh.templ"; }
    
    virtual std::string getScriptName() const
    { return "ferret_msg__#.sh"; }
};


class ExtensionManager
{
    
public:
    static ExtensionManager *getTheExtensionManager();
    
    ExtensionManager()
    {}
    
    void addExtension( ExtensionBase *e );

    ExtensionBase *createExtensionDriver( const std::string &type, ProjectXmlNode *node);
    
private:
    std::map<std::string,ExtensionBase *> extMap;
};

#endif
