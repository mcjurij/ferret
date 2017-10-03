
#include <iostream>
#include <sstream>
#include <cassert>

#include "project_xml_node.h"
#include "simple_xml_stream.h"
#include "platform_spec.h"
#include "glob_utility.h"
#include "extension.h"
#include "engine.h"
#include "platform_defines.h"
#include "script_template.h"

using namespace std;


bool ProjectXmlNode::xmlHasErrors = false;

ProjectXmlNode::ProjectXmlNode( const string &d, const string &module, const string &name,
                                const string &target, const string &type,
                                const vector<string> &cppflags, const vector<string> &cflags,
                                const vector<string> &usetools, const vector<string> &localLibs)
    : BaseNode( d, module, name, target, type, cppflags, cflags, usetools, localLibs)
{
}


static map<string,ProjectXmlNode *> dirToNodeMap;
static string nodeStack[50];

ProjectXmlNode *ProjectXmlNode::traverseXml( const string &start, ProjectXmlTimestamps &projXmlTs, int level)
{
    if( xmlHasErrors )
        return 0;
    
    if( level >= 50 )
    {
        cerr << "error: xml structure too deep.\n";
        xmlHasErrors = true;
        return 0;
    }
    
    for( int p = 0; p < level; p++)
    {
        if( nodeStack[ p ] == start )
        {
            cerr << "error: xml structure contains a cycle. visited " << start << " already. via:\n";
            for( int k = p; k < level; k++)
                cerr << "       " << nodeStack[ k ] << "\n";
            xmlHasErrors = true;
            return 0;
        }
    }
    
    map<string,ProjectXmlNode *>::const_iterator it = dirToNodeMap.find( start );
    if( it != dirToNodeMap.end() )
        return it->second;
    
    nodeStack[ level ] = start;
    
    string prjxml = start + "/project.xml";
    FILE *fpXml = fopen( prjxml.c_str(), "r");
    if( fpXml == 0 )
    {
        cerr << "error: xml structure file " << prjxml << " not found\n";
        return 0;
    }

    projXmlTs.addFile( prjxml );
    
    SimpleXMLStream *xmls = new SimpleXMLStream( fpXml );
    if( verbosity > 0 )
        cout << "level " << level << " - XML node: " << start << " -------------------------------------\n";
    
    vector<string> subDirs;
    string module;
    string name;
    string target;
    string type;
    vector<string> incdirs;    // incdir tag allows direct setting of additional include directory
    vector<string> cppflags;   // c++ flags valid only for this node
    vector<string> cflags;     // c flags valid only for this node
    vector<string> localLibs;  // lib tag allows adding external libs
    vector<string> usetools;
    vector<Extension> extensions;
    
    while( !xmls->atEnd() )
    {
        xmls->readNext();

        if( xmls->hasError() )
            break;
        
        if( xmls->isStartElement() )  // start or self closing
        {
            if( xmls->path() == "/project" )
            {
                 SimpleXMLAttributes attr = xmls->attributes();

                 if( attr.hasAttribute( "module" ) )
                 {
                     module = attr.value( "module" );
                 }

                 if( attr.hasAttribute( "name" ) )
                 {
                     name = attr.value( "name" );
                 }
                 
                 if( attr.hasAttribute( "target" ) )
                 {
                     target = attr.value( "target" );
                 }

                 if( attr.hasAttribute( "type" ) )
                 {
                     type = attr.value( "type" );
                 }
            }
            else if( xmls->path() == "/project/sub" )
            {
                SimpleXMLAttributes attr = xmls->attributes();
                
                if( attr.hasAttribute( "name" ) )
                {
                    subDirs.push_back( attr.value( "name" ) );
                }
            }
            else if( xmls->path() == "/project/incdir" )
            {
                SimpleXMLAttributes attr = xmls->attributes();
                
                if( attr.hasAttribute( "value" ) )
                    incdirs.push_back( PlatformDefines::getThePlatformDefines()->replace( attr.value( "value" ) ) );
            }
            else if( xmls->path() == "/project/cppflags" )
            {
                SimpleXMLAttributes attr = xmls->attributes();
                
                if( attr.hasAttribute( "value" ) )
                    cppflags.push_back( PlatformDefines::getThePlatformDefines()->replace( attr.value( "value" ) ) );
            }
            else if( xmls->path() == "/project/cflags" )
            {
                SimpleXMLAttributes attr = xmls->attributes();
                
                if( attr.hasAttribute( "value" ) )
                    cflags.push_back( PlatformDefines::getThePlatformDefines()->replace( attr.value( "value" ) ) );
            }
            else if( xmls->path() == "/project/lib" )
            {
                SimpleXMLAttributes attr = xmls->attributes();
                
                if( attr.hasAttribute( "value" ) )
                    localLibs.push_back( PlatformDefines::getThePlatformDefines()->replace( attr.value( "value" ) ) );
            }
            else if( xmls->path() == "/project/usetool" )
            {
                SimpleXMLAttributes attr = xmls->attributes();
                
                if( attr.hasAttribute( "name" ) )
                    usetools.push_back( attr.value( "name" ) );
            }
        }
        else if( xmls->isEndElement() )
        {
            if( xmls->path() == "/project" && xmls->name() != "sub" && xmls->name() != "incdir" &&
                xmls->name() != "cppflags" && xmls->name() != "cflags" && xmls->name() != "lflags" && xmls->name() != "eflags" &&
                xmls->name() != "usetool"  && xmls->name() != "lib"  )
            {
                map<string,string> attribs;
                SimpleXMLAttributes attr = xmls->attributes();
                size_t i;
                for( i = 0; i < attr.size(); i++)
                    attribs[ attr[i].name() ] = attr[i].value();
                
                extensions.push_back( Extension( xmls->name(), attribs) );
            }
        }
    }
    
    ProjectXmlNode *node = 0;
    if( !xmls->hasError() )
    {
        node = new ProjectXmlNode( start, module, name, target, type,
                                   cppflags, cflags, usetools, localLibs);
        size_t i;
        for( i = 0; i < subDirs.size(); i++)
        {
            string subDir = subDirs[i];
            
            if( !FindFiles::exists( subDir ) )
            {
                xmlHasErrors = true;
                cerr << "error: directory '" << subDir << "' referenced by " << prjxml << " not found.\n";
                delete xmls;
                fclose( fpXml );
                return 0;
            }
        }
        node->subDirs = subDirs;

        node->addExtensions( extensions );
    }
    else
    {
        xmlHasErrors = true;
        cerr << "error: xml structure file " << prjxml << " has errors.\n";
        delete xmls;
        fclose( fpXml );
        return 0;
    }
    
    delete xmls;
    fclose( fpXml );

    if( node )
    {
        node->traverseSrc();
        
        node->incdirs = incdirs;
        
        for( size_t i=0; i < node->subDirs.size(); i++)
        {
            string sub_start = join( "/", split( '/', node->subDirs[ i ]), false);
            
            ProjectXmlNode *d = traverseXml( sub_start, projXmlTs, level + 1);
            if( d )
            {
                node->workingSubDirs.push_back( node->subDirs[ i ] );
                node->childNodes.push_back( d );
            }
        }
    }
    
    dirToNodeMap[ start ] = node;
    return node;
}


bool ProjectXmlNode::hasXmlErrors()
{
    return xmlHasErrors;
}


void ProjectXmlNode::traverseStructureForChildren()
{
    traverseXmlStructureForChildren( 0 );
}

static map<string,vector<string> > subsMap;
vector<string> ProjectXmlNode::traverseXmlStructureForChildren( int level )
{
    map<string,vector<string> >::iterator it = subsMap.find( getDir() );
    if( it != subsMap.end() )
        return it->second;
    
    assignBuildProperties();
    
    for( size_t i = 0; i < childNodes.size(); i++)   // collect all <sub> tags of this node
    {
        ProjectXmlNode *d = childNodes[ childNodes.size() - 1 - i ];
        vector<string> subs = d->traverseXmlStructureForChildren( level + 1 );
        
        vector<string>::iterator sit = subs.begin();
        for( ; sit != subs.end(); sit++)
            allSubDirs.push_back( *sit );
    }
    
    allSubDirs.push_back( getDir() );
    set<string> rmDuplicates;
    vector<string> help;
    
    size_t i;
    for( i = 0; i < allSubDirs.size(); i++)
    {
        const string &subdir = allSubDirs[ i ];
        
        if( rmDuplicates.find( subdir ) == rmDuplicates.end() )
        {
            rmDuplicates.insert( subdir );
            help.push_back( subdir );
        }
    }

    set<string> subIncDirs;
    for( i = 0; i < help.size(); i++)
    {
        const string &subdir = help[ help.size() - 1 - i ];
        string subsrcdir = stackPath( subdir, "src");
        
        searchIncDirs.push_back( subsrcdir );
        subIncDirs.insert( subsrcdir );
    }

    string nodesrcdir = getSrcDir(); // stackPath( getDir(), "src");
    for( i = 0; i < incdirs.size(); i++)
    {
        string d = incdirs[i];
        if( d.length() > 0 )
        {
            if( d[0] != '/' )
                d = stackPath( nodesrcdir, d);
            
            if( subIncDirs.find( d ) == subIncDirs.end() )
            {
                searchIncDirs.push_back( d );
                traverseIncdir( d );
            }
            else
                if( show_info )
                    cout << "info: project.xml for target '" << getTarget() << "' contains superfluous incdir tag '"
                         << incdirs[i] << "'. include directory already in the sub tag set. ignored.\n";
        }
    }
    
    for( i = 0; i < help.size(); i++)
    {
        const string &subdir = help[ help.size() - 1 - i ];
        
        if( getDir() != subdir )
        {
            assert( dirToNodeMap.find( subdir ) != dirToNodeMap.end() );
            
            ProjectXmlNode *dep = dirToNodeMap.at( subdir );
            
            if( dep->getType() == "library" || dep->getType() == "staticlib"  || dep->getType() == "static" )
            {
                string t = dep->getTarget();
                libs.push_back( t );
                
                libsMap[ t ] = dep->getLibDir();
                searchLibDirs.push_back( dep->getLibDir() );
            }
        }
    }
    
    subsMap[ getDir() ] = allSubDirs;
    
    return allSubDirs;
}

