#include <cassert>
#include <iostream>

#include "extension.h"
#include "project_xml_node.h"
#include "glob_utility.h"
#include "include_manager.h"
#include "simple_xml_stream.h"
#include "platform_defines.h"

using namespace std;


void setupExtensions()
{
    ExtensionManager *em = ExtensionManager::getTheExtensionManager();

    em->addExtension( new FlexExtension(0) );
    
    em->addExtension( new BisonExtension(0) );
    
    em->addExtension( new MsgExtension(0) );

    // em->read( "ferret_extensions.xml" );
}


class ExtensionEntry      // for XML extensions
{

public:
    ExtensionEntry( const std::string &name )
        : type(name)
    {}

    std::string getType() const
    { return type; }
    
private:
    std::string type;

public:
    std::vector< std::pair< std::string, std::string> > defs;
    
    std::string selector_param;
    std::string selector_fileext;
    
    std::vector< std::pair< std::string, std::string> > selector_assigns;
    std::vector< std::pair< std::string, std::string> > param_defs;
    
    struct Node {
        typedef enum { INVALID, FILE_NODE /* = D type */, EXTENSION_NODE, WEAK_NODE, CPP_NODE, C_NODE} node_t;
        node_t node_type;
        std::string name;
        
        std::string file_name;
        std::string file_name_append;
        std::string assign_file_name_to;
        
        std::string block_extent;
    };
    
    //std::map< std::string, Node>  nodeMap;            // maps from node name to node
    std::vector< Node > nodes;
    
    std::vector< std::pair< std::string, std::string> > dependencies;
    
    std::string script_file_name;
    std::string script_stencil;
    
    std::vector< std::pair< std::string, std::string> > script_repl;

    std::vector< std::string > print_vals;
};

// -----------------------------------------------------------------------------
static string getAttrib( const map<string,string> &xmlAttribs, const string &key)
{
    map<string,string>::const_iterator it = xmlAttribs.find( key );
    if( it != xmlAttribs.end() )
        return it->second;
    else
        return "";
}


// -----------------------------------------------------------------------------
ProjectXmlNode *ExtensionBase::getNode()
{
    return node;
}


void ExtensionBase::addBlockedFileId( file_id_t id )
{
    blockedIds.insert( id );
    IncludeManager::getTheIncludeManager()->addBlockedId( id );
}


void ExtensionBase::addBlockIncludesFileId( file_id_t id )
{
    IncludeManager::getTheIncludeManager()->addBlockedId( id );
}


// -----------------------------------------------------------------------------

string FlexExtension::getType() const
{
    return "flex";
}


ExtensionBase *FlexExtension::createExtensionDriver( ProjectXmlNode *node )
{
    return new FlexExtension( node );
}


map<file_id_t, map<string,string> > FlexExtension::createCommands( FileManager &fileMan, const map<string,string> &xmlAttribs)
{
    assert( getNode() != 0 );
    
    map<file_id_t, map<string,string> > replacements;
    
    size_t i;
    if( verbosity > 0 )
        cout << "FLEX Extension for node " << getNode()->getDir() << ":\n";
    if( getAttrib( xmlAttribs, "prefix").length() > 0 )
        cerr << "FLEX Extension: internal error: 'prefix' attribute with value '" << getAttrib( xmlAttribs, "prefix") << "' currently ignored.\n";
    
    string outputdir = getNode()->getSrcDir() + "/";
    string o_outputdir = getNode()->getObjDir();
    
    file_id_t fid, tid;
    vector<File> sources = getNode()->getFiles();
    
    for( i = 0; i < sources.size(); i++)
    {
        map<string,string> r;
        
        if( sources[i].extension() == ".l" )
        {
            string flex_o = outputdir  + sources[i].woExtension() + ".cpp";
            string cpp_o = o_outputdir + sources[i].woExtension() + ".o";
            
            if( verbosity > 0 )
                cout << "Ext cmd for node " << getNode()->getDir() << ": flex " << flex_o << "\n";
            
            fid = fileMan.addExtensionCommand( flex_o, getType(), getNode());    // call flex
            tid = fileMan.addFile( sources[i].getPath(), getNode());             // input to flex
            fileMan.addDependency( fid, tid);

            file_id_t cpp_id = fileMan.addCommand( cpp_o, "Cpp", getNode());     // compile flex' output
            fileMan.addDependency( cpp_id, fid);
            
            r[ "F_IN" ]     = sources[i].getBasename();  // fileMan.getFileForId( tid );
            r[ "F_OUT" ]    = sources[i].woExtension() + ".cpp"; // fileMan.getFileForId( fid );
            r[ "F_OUTDIR" ] = outputdir;
            
            addBlockedFileId( fid );
            addBlockedFileId( tid );
            addBlockedFileId( cpp_id );
            getNode()->objs.push_back( cpp_o );
            
            replacements[ fid ] = r;
            getNode()->setExtensionScriptTemplNameForId( fid, getScriptTemplName(), getScriptName());
            getNode()->pegCppScript( cpp_id, flex_o, cpp_o, fileMan.getCompileMode());
        }
    }
    
    return replacements;
}

// -----------------------------------------------------------------------------

string BisonExtension::getType() const
{
    return "yac";
}

ExtensionBase *BisonExtension::createExtensionDriver( ProjectXmlNode *node )
{
    return new BisonExtension( node );
}


map<file_id_t, map<string,string> > BisonExtension::createCommands( FileManager &fileMan, const map<string,string> &xmlAttribs)
{
    assert( getNode() != 0 );
    map<file_id_t, map<string,string> > replacements;
    
    size_t i;
    if( verbosity > 0 )
        cout << "BISON Extension for node " << getNode()->getDir() << ":\n";
    if( getAttrib( xmlAttribs, "name").length() > 0 )
        cerr << "BISON Extension: internal error: 'name' attribute with value '" << getAttrib( xmlAttribs, "name") << "' currently ignored.\n";
    
    string outputdir = getNode()->getSrcDir() + "/";
    string o_outputdir = getNode()->getObjDir();
    
    file_id_t fid, tid;
    vector<File> sources = getNode()->getFiles();
    
    for( i = 0; i < sources.size(); i++)
    {
        map<string,string> r;
        
        if( sources[i].extension() == ".y" )
        {
            string bison_o = outputdir + sources[i].woExtension() + ".cpp";  // bison -d produces 2 output files, a .cpp
            string bison_o2 = outputdir + sources[i].woExtension() + ".h";   // and a header
            string cpp_o = o_outputdir + sources[i].woExtension() + ".o";
            
            if( verbosity > 0 )
                cout << "Ext cmd for node " << getNode()->getDir() << ": bison " << bison_o << "\n";
            
            fid = fileMan.addExtensionCommand( bison_o, getType(), getNode());   // call bison
            tid = fileMan.addFile( sources[i].getPath(), getNode());             // input to bison
            fileMan.addDependency( fid, tid);

            file_id_t cpp_id = fileMan.addCommand( cpp_o, "Cpp", getNode());     // compile bison's output
            fileMan.addDependency( cpp_id, fid);
            
            file_id_t header_id = fileMan.addCommand( bison_o2, "W", getNode());
            fileMan.addDependency( cpp_id, header_id);                           /* string a wait node to the compile node, to make sure we do not continue
                                                                                    without the header being generated */
            fileMan.addWeakDependency( fid, header_id);
            
            r[ "F_IN" ]     = sources[i].getBasename();           // fileMan.getFileForId( tid );
            r[ "F_INTERM_OUT" ]  = sources[i].woExtension() + ".tab.c";
            r[ "F_INTERM_OUT2" ] = sources[i].woExtension() + ".tab.h";
            r[ "F_OUT" ]    = sources[i].woExtension() + ".cpp";  // fileMan.getFileForId( fid );
            r[ "F_OUT2" ]   = sources[i].woExtension() + ".h";    // fileMan.getFileForId( header_id );
            r[ "F_OUTDIR" ] = outputdir;
            
            addBlockedFileId( fid );
            addBlockedFileId( tid );
            addBlockedFileId( header_id );
            addBlockedFileId( cpp_id );
            getNode()->objs.push_back( cpp_o );
            
            replacements[ fid ] = r;
            getNode()->setExtensionScriptTemplNameForId( fid, getScriptTemplName(), getScriptName());
            getNode()->pegCppScript( cpp_id, bison_o, cpp_o, fileMan.getCompileMode());
        }
    }
    
    return replacements;
}

// -----------------------------------------------------------------------------

string MsgExtension::getType() const
{
    return "msg";
}


ExtensionBase *MsgExtension::createExtensionDriver( ProjectXmlNode *node )
{
    return new MsgExtension( node );
}


map<file_id_t, map<string,string> > MsgExtension::createCommands( FileManager &fileMan, const map<string,string> &xmlAttribs)
{
    assert( getNode() != 0 );
    
    map<file_id_t, map<string,string> > replacements;
    
    size_t i;

    if( verbosity > 0 )
        cout << "MSG Extension for node " << getNode()->getDir() << ":\n";
    
    string msgfn = getAttrib( xmlAttribs, "name");
    if( msgfn.length() == 0 )
        cerr << "MSG Extension: error: 'name' attribute missing.\n";
    
    string m_outputdir = getNode()->getDir() + "/";
    string o_outputdir = getNode()->getObjDir();
    
    file_id_t fid, tid;
    vector<File> sources = getNode()->getFiles();
    
    for( i = 0; i < sources.size(); i++)
    {
        if( sources[i].extension() == ".msg" && sources[i].getPath() == getNode()->getDir() + "/" + msgfn )
        {
            map<string,string> r;
            
            string msg_o = m_outputdir + "src/" + sources[i].woExtension() + ".cpp";  // msg produces 2 output files, a .cpp
            string msg_o2 = m_outputdir + "src/" + sources[i].woExtension() + ".h";   // and a header
            //string cpp_o = o_outputdir + sources[i].woExtension() + ".o";

            if( verbosity > 0 )
                cout << "Ext cmd for node " << getNode()->getDir() << ": msg " << msg_o << " and " << msg_o2 << "\n";
            fid = fileMan.addExtensionCommand( msg_o, getType(), getNode());     // call msg, generates the .h and .cpp file
            tid = fileMan.addFile( sources[i].getPath(), getNode());             // input to msg
            fileMan.addDependency( fid, tid);
            
            file_id_t header_id = fileMan.addCommand( msg_o2, "W", getNode());
            //file_id_t cpp_id = fileMan.addCommand( cpp_o, "Cpp", getNode());     // compile msg output
            //fileMan.addDependency( cpp_id, fid);
            //fileMan.addDependency( cpp_id, header_id);                           
            fileMan.addWeakDependency( fid, header_id);  /* string a wait node to the msg node, to make sure we do not continue
                                                                                    without the header being generated */
            
            r[ "F_IN" ] = sources[i].getPath();
            r[ "F_MSG_IN" ] = sources[i].getBasename();
            r[ "F_MSG_NODEDIR" ] = getNode()->getDir() + "/";
            // r[ "F_OUT" ] =    not needed. Output is fixed

            //getNode()->addExtensionSourceId( fid );
            addBlockIncludesFileId( fid );
            //addBlockedFileId( fid );
            addBlockedFileId( tid );
            //addBlockedFileId( header_id );
            //addBlockedFileId( cpp_id );
            
            replacements[ fid ] = r;
            getNode()->setExtensionScriptTemplNameForId( fid, getScriptTemplName(), getScriptName());
            //getNode()->pegCppScript( cpp_id, msg_o, cpp_o, fileMan.getCompileMode());
        }
    }
    
    return replacements;
}

// -----------------------------------------------------------------------------
// XML extensions configurable using XML file
string XmlExtension::getType() const
{
    assert( entry );
    return entry->getType();
}


ExtensionBase *XmlExtension::createExtensionDriver( ProjectXmlNode *node )
{
    assert( entry );
    return new XmlExtension( node, entry);
}


map<file_id_t, map<string,string> > XmlExtension::createCommands( FileManager &fileMan, const map<string,string> &xmlAttribs)
{
    assert( getNode() != 0 );
    
    PlatformDefines::getThePlatformDefines()->copyInto( defines );
    
    defines.set( "PROJ_TARGET", getNode()->getScriptTarget());
    defines.set( "PROJ_DIR",    getNode()->getDir());
    defines.set( "SRC_DIR",     getNode()->getSrcDir());
    defines.set( "OBJ_DIR",     getNode()->getObjDir());
    
    size_t i;
    for( i = 0; i < entry->param_defs.size(); i++)  // defines from the XML attributes
    {
        string param = entry->param_defs[i].first;
        string to    = entry->param_defs[i].second;
        defines.set( to, getAttrib( xmlAttribs, param));
    }
    
    map<file_id_t, map<string,string> > replacements;
    string selectorfn = getAttrib( xmlAttribs, entry->selector_param);
    if( selectorfn.length() == 0 )
    {
        cerr << entry->getType() << " extension (XML): error: '" << entry->selector_param << "' attribute missing.\n";
        return replacements;
    }
    
    for( i = 0; i < entry->defs.size(); i++)  // regular defines
    {
        string name  = entry->defs[i].first;
        string value = entry->defs[i].second;
        defines.set( name, defines.replace( value ));
    }
    
    vector<File> sources = getNode()->getFiles();
    
    for( i = 0; i < sources.size(); i++)
    {
        if( sources[i].extension() == entry->selector_fileext && sources[i].getPath() == getNode()->getSrcDir() + "/" + selectorfn )
        {
            map<string,file_id_t>  nameToFileId;
            
            size_t a;
            for( a = 0; a < entry->selector_assigns.size(); a++)
            {
                string part = entry->selector_assigns[a].first;
                string to   = entry->selector_assigns[a].second;
                
                if( part == "full" )
                    defines.set( to, sources[i].getPath());
                else if( part == "bn_wo_ext" )
                    defines.set( to, sources[i].woExtension());
                else if( part == "bn" )
                    defines.set( to, sources[i].getBasename());
            }
            
            size_t n;
            file_id_t extension_fid = -1;
            
            for( n = 0; n < entry->nodes.size(); n++)
            {
                const ExtensionEntry::Node &node = entry->nodes[n];
                
                file_id_t fid;
                string base_fn = defines.replace( node.file_name );
                string fn = base_fn + node.file_name_append;
                string out_obj_fn;
                
                switch( node.node_type )
                {
                    case ExtensionEntry::Node::INVALID:
                        fid = -1;
                        break;
                        
                    case ExtensionEntry::Node::FILE_NODE:
                        fid = fileMan.addFile( fn, getNode());
                        break;
                        
                    case ExtensionEntry::Node::EXTENSION_NODE:
                        extension_fid = fid = fileMan.addExtensionCommand( fn, entry->getType(), getNode());    
                        break;
                        
                    case ExtensionEntry::Node::WEAK_NODE:
                        fid = fileMan.addCommand( fn, "W", getNode());
                        break;
                        
                    case ExtensionEntry::Node::CPP_NODE:
                        fid = fileMan.addCommand( fn, "Cpp", getNode());
                        
                        out_obj_fn = base_fn + ".o";
                        getNode()->objs.push_back( out_obj_fn );
                        getNode()->pegCppScript( fid, fn, out_obj_fn, fileMan.getCompileMode());
                        break;
                        
                    case ExtensionEntry::Node::C_NODE:
                        fid = fileMan.addCommand( fn, "C", getNode());
                        
                        out_obj_fn = base_fn + ".o";
                        getNode()->objs.push_back( out_obj_fn );
                        getNode()->pegCScript( fid, fn, out_obj_fn, fileMan.getCompileMode());
                        break;
                }
                
                if( fid != -1 )
                {
                    nameToFileId[ node.name ] = fid;
                    
                    if( node.block_extent.length() > 0 )
                    {
                        if( node.block_extent == "full" )
                            addBlockedFileId( fid );
                        else if( node.block_extent == "include" || node.block_extent == "includes" )
                            addBlockIncludesFileId( fid );                    
                    }

                    if( node.assign_file_name_to.length() > 0 )
                        defines.set( node.assign_file_name_to, fn);
                }
            }
            
            size_t d;
            for( d = 0; d < entry->dependencies.size(); d++)
            {
                file_id_t from_id = -1, to_id = -1;
                string from_name = entry->dependencies[d].first;
                string to_name   = entry->dependencies[d].second;
                map<string,file_id_t>::iterator from_it = nameToFileId.find( from_name );
                map<string,file_id_t>::iterator to_it   = nameToFileId.find( to_name );
                
                if( from_it != nameToFileId.end() )
                    from_id = from_it->second;
                if( to_it != nameToFileId.end() )
                    to_id = to_it->second;
                
                if( from_id != -1 && to_id != -1 )
                {
                    string to_cmd = fileMan.getCmdForId( to_id );

                    if( to_cmd != "W" )
                        fileMan.addDependency( from_id, to_id);
                    else
                        fileMan.addWeakDependency( from_id, to_id);   // only a weak dependency is allowed to connect to a weak node
                }
                else
                    cerr << entry->getType() << " extension (XML): error: dependency from '" << from_name << "' to '" << to_name << "' is broken.\n";
            }
            
            if( extension_fid != -1 )
            {
                size_t r;
                map<string,string> repl;
                for( r = 0; r < entry->script_repl.size(); r++)
                {
                    string name  = entry->script_repl[r].first;
                    string value = entry->script_repl[r].second;
                    
                    repl[ name ] = defines.replace( value );
                }
                
                replacements[ extension_fid ] = repl;

                string stencil;
                vector<string> p = split( '#', entry->script_stencil);

                if( p.size() == 2 )
                    stencil = p[0] + "#__" + fileMan.getCompileMode() + p[1];
                else
                    stencil = entry->script_stencil;
                
                getNode()->setExtensionScriptTemplNameForId( extension_fid, entry->script_file_name, stencil);
            }
            else
                cerr << entry->getType() << " extension (XML): error: no extension command node created\n";            
            
            for( size_t p = 0; p < entry->print_vals.size(); p++)  // prints, useful for debugging
            {
                cout << entry->getType() << " extension (XML): prints: " << defines.replace( entry->print_vals[p] ) << "\n";
            }
        }
    }
    
    return replacements;
}


string XmlExtension::getScriptTemplName() const
{
    assert( entry );
    return entry->script_file_name;
}


string XmlExtension::getScriptName() const
{
    assert( entry );
    return entry->script_stencil;
}

// -----------------------------------------------------------------------------

ExtensionManager *ExtensionManager::getTheExtensionManager()
{
    static ExtensionManager *em = 0;
    
    if( em == 0 )
        em = new ExtensionManager;

    return em;
}


void ExtensionManager::addExtension( ExtensionBase *e )
{
    if( extMap.find( e->getType() ) == extMap.end() )
    {
        if( verbosity > 0 )
            cout << "Extension " << e->getType() << " added, using script template " << e->getScriptTemplName() << "\n";
        extMap[ e->getType() ] = e;
    }
    else
    {
        cerr << "error: extension of type '" << e->getType() << "' comes across twice.\n";
        delete e;
    }
}


void ExtensionManager::addXmlExtension( const ExtensionEntry *entry )
{
    ExtensionBase *e = new XmlExtension( 0, entry);
    
    addExtension( e );
}


ExtensionBase *ExtensionManager::createExtensionDriver( const string &type, ProjectXmlNode *node)
{
    map<string,ExtensionBase *>::iterator it = extMap.find( type );

    if( it != extMap.end() )
    {
        ExtensionBase *e = it->second->createExtensionDriver( node );
        return e;
    }
    else
        return 0;
}


bool ExtensionManager::parseExtension( SimpleXMLStream *xmls )
{
    ExtensionEntry *entry = 0;
    
    while( !xmls->AtEnd() )
    {
        if( xmls->HasError() )
            break;
        
        if( xmls->IsStartElement() )
        {
            if( xmls->Path() == "/extension" )
            {
                SimpleXMLAttributes attr = xmls->Attributes();

                string type;
                if( attr.HasAttribute( "name" ) )
                     type = attr.Value( "name" );

                entry = new ExtensionEntry( type );
            }
            else if( xmls->Path() == "/extension/selector" )
            {
                SimpleXMLAttributes attr = xmls->Attributes();
                
                if( attr.HasAttribute( "param" ) )
                    entry->selector_param = attr.Value( "param" );
                
                if( attr.HasAttribute( "fileext" ) )
                    entry->selector_fileext = attr.Value( "fileext" );
            }
            else if( xmls->Path() == "/extension/selector/assign" )
            {
                SimpleXMLAttributes attr = xmls->Attributes();

                string part, to;
                if( attr.HasAttribute( "part" ) )
                    part = attr.Value( "part" );

                if( attr.HasAttribute( "to" ) )
                    to = attr.Value( "to" );
                
                if( part.length() > 0 && to.length() > 0 )
                    entry->selector_assigns.push_back( make_pair( part, to) );
            }
            else if( xmls->Path() == "/extension/file_node" ||
                     xmls->Path() == "/extension/extension_node" ||
                     xmls->Path() == "/extension/weak_node" ||
                     xmls->Path() == "/extension/cpp_node"  ||
                     xmls->Path() == "/extension/c_node")
            {
                ExtensionEntry::Node node;
                string save_path = xmls->Path();
                string wait_for;
                if( xmls->Path() == "/extension/file_node" )
                {
                    node.node_type = ExtensionEntry::Node::FILE_NODE;
                    wait_for = "file_node";
                }
                else if( xmls->Path() == "/extension/extension_node" )
                {
                    node.node_type = ExtensionEntry::Node::EXTENSION_NODE;
                    wait_for = "extension_node";
                }
                else if( xmls->Path() == "/extension/weak_node" )
                {
                    node.node_type = ExtensionEntry::Node::WEAK_NODE;
                    wait_for = "weak_node";
                }
                else if( xmls->Path() == "/extension/cpp_node" )
                {
                    node.node_type = ExtensionEntry::Node::CPP_NODE;
                    wait_for = "cpp_node";
                }
                else if( xmls->Path() == "/extension/weak_node" )
                {
                    node.node_type = ExtensionEntry::Node::C_NODE;
                    wait_for = "c_node";
                }
                
                SimpleXMLAttributes attr = xmls->Attributes();
                if( attr.HasAttribute( "name" ) )
                    node.name = attr.Value( "name" );
                
                while( !xmls->AtEnd() )
                {
                    xmls->ReadNext();
                    
                    if( xmls->HasError() )
                        break;
                    
                    if( xmls->IsStartElement() )
                    {
                         if( xmls->Path() == save_path + "/file_name" )
                         {
                             SimpleXMLAttributes attr = xmls->Attributes();
                             
                             if( attr.HasAttribute( "value" ) )
                                 node.file_name = attr.Value( "value" );
                             
                             if( attr.HasAttribute( "append" ) )
                                 node.file_name_append = attr.Value( "append" );
                         }
                         else if( xmls->Path() == save_path + "/assign_file_name" )
                         {
                             SimpleXMLAttributes attr = xmls->Attributes();
                             if( attr.HasAttribute( "to" ) )
                                 node.assign_file_name_to = attr.Value( "to" );
                         }
                         else if( xmls->Path() == save_path + "/block" )
                         {
                             SimpleXMLAttributes attr = xmls->Attributes();
                             if( attr.HasAttribute( "extent" ) )
                                 node.block_extent = attr.Value( "extent" );
                         }
                    }
                    else if( xmls->IsEndElement() )
                    {
                        if( xmls->Path() == "/extension" && xmls->Name() == wait_for )
                            break;
                    }
                }

                entry->nodes.push_back( node );
            }
            else if( xmls->Path() == "/extension/dependency" )
            {
                SimpleXMLAttributes attr = xmls->Attributes();

                string from_name, to_name;
                if( attr.HasAttribute( "from_node" ) )
                    from_name = attr.Value( "from_node" );
                
                if( attr.HasAttribute( "to_node" ) )
                    to_name = attr.Value( "to_node" );
                
                if( from_name.length() > 0 && to_name.length() > 0 )
                    entry->dependencies.push_back( make_pair( from_name, to_name) );
            }
            else if( xmls->Path() == "/extension/script_name" )
            {
                SimpleXMLAttributes attr = xmls->Attributes();
                
                if( attr.HasAttribute( "value" ) )
                    entry->script_file_name = attr.Value( "value" );
            }
            else if( xmls->Path() == "/extension/script_stencil" )
            {
                SimpleXMLAttributes attr = xmls->Attributes();
                
                if( attr.HasAttribute( "value" ) )
                    entry->script_stencil = attr.Value( "value" );                
            }
            else if( xmls->Path() == "/extension/replace" )
            {
                SimpleXMLAttributes attr = xmls->Attributes();
                
                string name, value;
                if( attr.HasAttribute( "name" ) )
                    name = attr.Value( "name" );
                if( attr.HasAttribute( "value" ) )
                    value = attr.Value( "value" );
                
                if( name.length() > 0 && value.length() > 0 )
                    entry->script_repl.push_back( make_pair( name, value) );
            }
            else if( xmls->Path() == "/extension/param_def" )
            {
                SimpleXMLAttributes attr = xmls->Attributes();
                
                string param, to;
                if( attr.HasAttribute( "param" ) )
                    param = attr.Value( "param" );
                if( attr.HasAttribute( "to" ) )
                    to = attr.Value( "to" );
                
                if( param.length() > 0 && to.length() > 0 )
                    entry->param_defs.push_back( make_pair( param, to) );
            }
            else if( xmls->Path() == "/extension/print" )
            {
                SimpleXMLAttributes attr = xmls->Attributes();
                
                string value;
                if( attr.HasAttribute( "value" ) )
                    value = attr.Value( "value" );
                
                entry->print_vals.push_back( value );
            }
            else if( xmls->Path() == "/extension/define" )
            {
                SimpleXMLAttributes attr = xmls->Attributes();
                
                string name, value;
                if( attr.HasAttribute( "name" ) )
                    name = attr.Value( "name" );
                if( attr.HasAttribute( "value" ) )
                    value = attr.Value( "value" );
                
                if( name.length() > 0 && value.length() > 0 )
                    entry->defs.push_back( make_pair( name, value) );
            }
            
/*            else if( xmls->Path() == "/platform/define" )
            {
                SimpleXMLAttributes attr = xmls->Attributes();

                if( attr.HasAttribute( "name" ) )
                {
                    string name = attr.Value( "name" );

                    if( name.length() > 0 )
                    {
                        string value;
                        if( attr.HasAttribute( "value" ) )
                            value = PlatformDefines::getThePlatformDefines()->replace( attr.Value( "value" ) );
                        
                        PlatformDefines::getThePlatformDefines()->set( name, value);
                    }
                }
            }
*/
        }
        else if( xmls->IsEndElement() )
        {
            if( xmls->Path() == "" && xmls->Name() == "extension" )
            {
                if( entry )
                {
                    addXmlExtension( entry );
                    entry = 0;
                }
                break;
            }
        }

        xmls->ReadNext();
    }

    bool ok = !xmls->HasError();

    return ok;
}


bool ExtensionManager::read( const string &fn )
{
    FILE *fpXml = fopen( fn.c_str(), "r");
    if( fpXml == 0 )
    {
        cerr << "error: xml extensions configuration file " << fn << " not found\n";
        return false;
    }
    
    SimpleXMLStream *xmls = new SimpleXMLStream( fpXml );

    while( !xmls->AtEnd() )
    {
        xmls->ReadNext();
        
        if( xmls->HasError() )
            break;
        
        if( xmls->IsStartElement() )
        {
            if( xmls->Path() == "/extensions/extension" )
            {
                xmls->setPathStart( 1 );
                parseExtension( xmls );
                xmls->setPathStart( 0 );
            }
        }
        else if( xmls->IsEndElement() )
        {
            if( xmls->Path() == "" && xmls->Name() == "extensions" )
                break;
        }
    }

    bool err = !xmls->HasError();
    
    delete xmls;
    fclose( fpXml );

    return err;
}
