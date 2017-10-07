#ifndef FERRET_PROJECT_XML_NODE_H_
#define FERRET_PROJECT_XML_NODE_H_

#include <string>
#include <vector>
#include <map>
#include <set>


#include "project_xml_timestamps.h"
#include "base_node.h"

// -----------------------------------------------------------------------------
// XML node
// -----------------------------------------------------------------------------

class ProjectXmlNode : public BaseNode {
    
public:
    ProjectXmlNode( const std::string &d, const std::string &module, const std::string &name,
                    const std::string &target, const std::string &type,
                    const std::vector<std::string> &cppflags, const std::vector<std::string> &cflags,
                    const std::vector<std::string> &usetools, const std::vector<std::string> &localLibs);

    virtual ProjectXmlNode *getSibling() const    // never used, for compatibility in TraverseStructure
    { return 0; }
    
    virtual void setName( const std::string &n );

    void addChildNode( ProjectXmlNode *n )
    { childNodes.push_back( n ); }
    
    std::vector<ProjectXmlNode *> getChildNodes() const
    { return childNodes; }
    
    std::string getNodePath() const              // never used, for compatibility in TraverseStructure
    { return "!"; }
    
    static ProjectXmlNode *traverseXml( const std::string &start, ProjectXmlTimestamps &projXmlTs, int level=0);
    static bool hasXmlErrors();
    
    void traverseStructureForChildren();
    
private:
    std::vector<std::string> traverseXmlStructureForChildren( int level );
    
public:
    int checkForNewFiles( FileManager &fileMan );
    
private:
    std::vector<ProjectXmlNode *> childNodes;
    
    static bool xmlHasErrors;
};

#endif
