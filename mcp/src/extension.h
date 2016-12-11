#ifndef FERRET_EXTENSION_H_
#define FERRET_EXTENSION_H_

#include <string>
#include <map>
#include <set>

#include "file_manager.h"
#include "defines.h"

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
    
    virtual std::string getType() const = 0;           // yac, flex, anything from the xml configuration

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


class ExtensionEntry;
class XmlExtension : public ExtensionBase  // for XML extensions
{
    
public:
    XmlExtension( ProjectXmlNode *node, const ExtensionEntry *entry)
        : ExtensionBase(node), entry(entry)
    {}
    
    virtual std::string getType() const;
    virtual ExtensionBase *createExtensionDriver( ProjectXmlNode *node );

    virtual std::map<file_id_t, std::map<std::string,std::string> > createCommands( FileManager &fileMan, const std::map<std::string,std::string> &xmlAttribs);

    virtual std::string getScriptTemplName() const;
        
    virtual std::string getScriptName() const;
    
private:
    const ExtensionEntry *entry;
    Defines defines;
};


class ExtensionManager
{
    
public:
    static ExtensionManager *getTheExtensionManager();
    
    ExtensionManager()
    {}
    
    void addExtension( ExtensionBase *e );
    void addXmlExtension( const ExtensionEntry *entry );
    
    ExtensionBase *createExtensionDriver( const std::string &type, ProjectXmlNode *node);
    
    bool read( const std::string &fn );
    
private:
    std::map<std::string,ExtensionBase *> extMap;
};

#endif
