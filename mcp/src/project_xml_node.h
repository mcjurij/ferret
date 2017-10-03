#ifndef FERRET_PROJECT_XML_NODE_H_
#define FERRET_PROJECT_XML_NODE_H_

#include <string>
#include <vector>
#include <map>
#include <set>


#include "project_xml_timestamps.h"
#include "base_node.h"

// -----------------------------------------------------------------------------
// XML traversal
// -----------------------------------------------------------------------------


class ProjectXmlNode : public BaseNode {
    
public:
    ProjectXmlNode( const std::string &d, const std::string &module, const std::string &name,
                    const std::string &target, const std::string &type,
                    const std::vector<std::string> &cppflags, const std::vector<std::string> &cflags,
                    const std::vector<std::string> &usetools, const std::vector<std::string> &localLibs);
    
    static ProjectXmlNode *traverseXml( const std::string &start, ProjectXmlTimestamps &projXmlTs, int level=0);
    static bool hasXmlErrors();
    
    void traverseStructureForChildren();
    
private:
    std::vector<std::string> traverseXmlStructureForChildren( int level );
    
public:
    static ProjectXmlNode *getNodeByName( const std::string &name );

    std::vector<ProjectXmlNode *> childNodes;
    
private:
    static bool xmlHasErrors;
};

#endif
