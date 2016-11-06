#include <cassert>
#include <iostream>

#include "extension.h"
#include "project_xml_node.h"
#include "glob_utility.h"
#include "include_manager.h"

using namespace std;


void setupExtensions()
{
    ExtensionManager *em = ExtensionManager::getTheExtensionManager();

    // em->addExtension( new CxxExtension(0) );  // experiment

    em->addExtension( new FlexExtension(0) );
    
    em->addExtension( new BisonExtension(0) );
    
    em->addExtension( new MsgExtension(0) );
}

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

string CxxExtension::getType() const
{
    return "cxx";
}


ExtensionBase *CxxExtension::createExtensionDriver( ProjectXmlNode *node )
{
    return new CxxExtension( node );
}


map<file_id_t, map<string,string> > CxxExtension::createCommands( FileManager &fileMan, const map<string,string> &xmlAttribs)
{
    assert( getNode() != 0 );
    
    map<file_id_t, map<string,string> > replacements;
    
    size_t i;
    cout << "Extension for node " << getNode()->getDir() << ":\n";
    
    string o_outputdir = getNode()->getDir() + "/";
    file_id_t fid = 0, tid;
    vector<File> sources = getNode()->getFiles();
    
    for( i = 0; i < sources.size(); i++)
    {
        file_id_t nl_id;
        map<string,string> r;
        
        if( sources[i].extension() == ".cxx" )
        {
            string o = o_outputdir + sources[i].woExtension() + ".o";
            
            cout << "Ext cmd for node " << getNode()->getDir() << ": cxx " << o << "\n";
            fid = fileMan.addExtensionCommand( o, "cxx", getNode());
            tid = fileMan.addFile( sources[i].getPath(), getNode());
            fileMan.addDependency( fid, tid);
            
            nl_id = fileMan.addExtensionCommand( sources[i].getPath() + ".nl", "W", getNode());
            fileMan.addDependency( nl_id, fid);

            r[ "F_IN" ] = fileMan.getFileForId( tid );
            r[ "F_OUT" ] = fileMan.getFileForId( fid );
            r[ "F_OUT2" ] = fileMan.getFileForId( nl_id );

            addBlockedFileId( tid );
            addBlockedFileId( fid );
            addBlockedFileId( nl_id );
            getNode()->objs.push_back( o );
            
            replacements[ fid ] = r;
            getNode()->setExtensionScriptTemplNameForId( fid, getScriptTemplName(), getScriptName());
        }
    }

    
    return replacements;
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

ExtensionManager *ExtensionManager::getTheExtensionManager()
{
    static ExtensionManager *em = 0;
    
    if( em == 0 )
        em = new ExtensionManager;

    return em;
}


void ExtensionManager::addExtension( ExtensionBase *e )
{
    if( verbosity > 0 )
        cout << "Extension " << e->getType() << " added, using script template " << e->getScriptTemplName() << "\n";
    extMap[ e->getType() ] = e;
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
