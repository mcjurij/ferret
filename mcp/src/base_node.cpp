
#include <iostream>
#include <sstream>
#include <cassert>

#include "base_node.h"
#include "platform_spec.h"
#include "glob_utility.h"
#include "extension.h"
#include "engine.h"
#include "platform_defines.h"
#include "script_template.h"

using namespace std;


map<string, BaseNode *> BaseNode::nameToNodeMap;


BaseNode::BaseNode( const string &d, const string &module)
    : dir(d), module(module),
      madeObjDir(false), madeBinDir(false), madeLibDir(false),
      target_file_id(-1)
{
    srcdir = dir;
    collectFiles();
}


BaseNode::BaseNode( const string &d, const string &module, const string &name,
                    const string &target, const string &type,
                    const vector<string> &cppflags, const vector<string> &cflags,
                    const vector<string> &usetools, const vector<string> &localLibs)
    : dir(d), target(target), type(type), cppflags(cppflags), cflags(cflags), localLibs(localLibs),
      module(module), name(name), madeObjDir(false), madeBinDir(false), madeLibDir(false), usetools(usetools),
      target_file_id(-1)
{
    srcdir = stackPath( dir, "src");
    collectFiles();
}


void BaseNode::assignBuildProperties()
{
    prjBuildProps = BuildProps::getTheBuildProps()->getPrjBuildProps( this );
}


string BaseNode::getObjDir()
{
    if( !madeObjDir )
    {
        assert( prjBuildProps.getObjDir() != "" );
        mkdir_p( prjBuildProps.getObjDir() );
        madeObjDir = true;
    }
    
    return prjBuildProps.getObjDir();
}


string BaseNode::getBinDir()
{
    if( !madeBinDir )
    {
        assert( prjBuildProps.getBinDir() != "" );
        mkdir_p( prjBuildProps.getBinDir() );
        madeBinDir = true;
    }
    
    return prjBuildProps.getBinDir();
}


string BaseNode::getLibDir()
{
    if( !madeLibDir )
    {
        assert( prjBuildProps.getLibDir() != "" );
        mkdir_p( prjBuildProps.getLibDir() );
        madeLibDir = true;
    }
    
    return prjBuildProps.getLibDir();
}


void BaseNode::collectFiles()
{
    if( FindFiles::exists( srcdir ) )
        nodeFiles.traverse( srcdir,  false /* not deep */);
}


vector<File> BaseNode::getFiles() const
{
    return nodeFiles.getFiles();
}


void BaseNode::traverseIncdir( const string &incdir )
{
    if( FindFiles::exists( incdir ) )
        incdirFiles.appendTraverse( incdir, false /* not deep */);
    else
        cerr << "warning: incdir '" << incdir << "' not found\n";
}


int BaseNode::manageDeletedFiles( FileManager &fileMan )
{
    size_t i;
    int cnt = 0;
    
    for( i = 0; i < databaseFiles.size(); i++)
    {
        const string &fn = databaseFiles[ i ];
        
        if( !FindFiles::exists( fn ) )
        {
            file_id_t id = fileMan.getIdForFile( fn );
            assert( id != -1 );
            set<file_id_t> prereqs = fileMan.prerequisiteFor( id );
            
            if( fileMan.removeFile( fn ) )  // will only remove "D" type files from database
            {
                cout << "Removing file " << fn << " from db if D type.\n";
                
                IncludeManager::getTheIncludeManager()->removeFile( fileMan, id, fn, this, prereqs);
                cnt++;
            }
        }
    }
    
    return cnt;
}


int BaseNode::checkForNewIncdirFiles( FileManager &fileMan )
{
    size_t i;
    int cnt = 0;
    vector<File> headers = incdirFiles.getHeaderFiles();       // files referred to with <incdir> tag
    
    for( i=0; i < headers.size(); i++)
    {
        const File &f = headers[i];
        
        if( !fileMan.hasFileName( f.getPath() ) )              // already in files db?
        {
            fileMan.addNewFile( f.getPath(), 0); // this is done for the rare cases where the node being referred to has not been visited already
                                                               // currently files from core_as/ips_txn/src
            cnt++;
        }
    }
    
    return cnt;
}


void BaseNode::createCommandArguments( const string &compileMode )
{
    size_t i;
    stringstream incss, toolincss, cppflagss, cflagss, lflagss, eflagss;
    stringstream toolLibss, toolLibDirss;
    
    PlatformSpec *ps = PlatformSpec::getThePlatformSpec();

    // global settings from platform spec
    pltfIncArg = ps->getIncCommandArgs();
    cppflagss << ps->getCppFlagArgs();
    cflagss   << ps->getCFlagArgs();
    lflagss   << ps->getLinkerFlagArgs();
    eflagss   << ps->getLinkExecutableFlagArgs();
    pltfLibArg    = ps->getLibArgs();
    pltfLibDirArg = ps->getLibDirArgs();

    const CompileMode &cm = ps->getCompileMode( compileMode );
    cppflagss << " " << cm.getCppFlagArgs();
    cflagss   << " " << cm.getCFlagArgs();
    lflagss   << " " << cm.getLinkerFlagArgs();
    eflagss   << " " << cm.getLinkExecutableFlagArgs();

    if( ps->hasCompileTrait( type ) )
    {
        const CompileTrait &ct = ps->getCompileTrait( type );
        cppflagss << " " << ct.getCppFlagArgs();
        cflagss   << " " << ct.getCFlagArgs();
        lflagss   << " " << ct.getLinkerFlagArgs();
        eflagss   << " " << ct.getLinkExecutableFlagArgs();
    }
    
    for( i = 0; i < usetools.size(); i++)    // tools used in this node
    {
        const ToolSpec &ts = ps->getTool( usetools[i] );
        
        if( ts.getName() == "INVALID" )
        {
            cerr << "warning: you are trying to use an unconfigured tool '" << usetools[i] << "' in "
                 << getDir() << ". add tool to platform specification.\n";
            continue;
        }
        
        toolincss << ts.getIncCommandArgs();
        cppflagss << " " << ts.getCppFlagArgs();
        cflagss   << " " << ts.getCFlagArgs();
        lflagss   << " " << ts.getLinkerFlagArgs();
        eflagss   << " " << ts.getLinkExecutableFlagArgs();
        toolLibss    << " " << ts.getLibArgs();
        toolLibDirss << " " << ts.getLibDirArgs();
    }

    incss << join( " -I", searchIncDirs, true);     // includes from this node
    includesArg = incss.str();
    toolIncArg = toolincss.str();
    
    cppflagss << join( " ", cppflags, true);       // c++ flags from this node
    cppflagsArg = cppflagss.str();
    
    cflagss << join( " ", cflags, true);           // c flags from this node
    cflagsArg = cflagss.str();
    
    lflagsArg = lflagss.str();
    aflagsArg = "";                      // TODO
    eflagsArg = eflagss.str();

    toolLibArg    = toolLibss.str();
    toolLibDirArg = toolLibDirss.str();
    
    if( type == "library" )
        scriptTarget = ps->getSoFileName( target );
    else if( type == "executable" )
        scriptTarget = target;
    else if( type == "staticlib" || type == "static" )
        scriptTarget = ps->getStaticlibFileName( target );
}


void BaseNode::createCommands( FileManager &fileMan )
{
    PlatformSpec *ps = PlatformSpec::getThePlatformSpec();
    string compileMode = fileMan.getCompileMode();
    ScriptManager *sm = ScriptManager::getTheScriptManager();
    
    if( type == "library" || type == "executable" || type == "staticlib" || type == "static" )
    {
        string l_outputdir = getLibDir();
        string b_outputdir = getBinDir();
        map<string,string> repl;
        
        if( type == "library" )
        {
            string t = l_outputdir + scriptTarget;
            target_file_id = fileMan.addCommand( t, "L", this);
            
            sm->setTemplateFileName( target_file_id, "ferret_l.sh.templ", "ferret_l__#.sh");

            repl[ "F_LFLAGS" ] = lflagsArg;
            repl[ "F_OUT" ] = t;
        }
        else if( type == "executable" )
        {
            string t = b_outputdir + scriptTarget;
            target_file_id = fileMan.addCommand( t, "X", this);

            sm->setTemplateFileName( target_file_id, "ferret_x.sh.templ", "ferret_x__#.sh");
            
            repl[ "F_EFLAGS" ] = eflagsArg;
            repl[ "F_OUT" ] = t;
        }
        else if( type == "staticlib" || type == "static" )
        {
            string t = l_outputdir + scriptTarget;
            target_file_id = fileMan.addCommand( t, "A", this);

            sm->setTemplateFileName( target_file_id, "ferret_a.sh.templ", "ferret_a__#.sh");
            
            repl[ "F_AFLAGS" ] = aflagsArg;
            repl[ "F_OUT" ] = t;
        }
        
        stringstream con;
        con << target_file_id;
        repl[ "F_ID" ] = con.str();
        repl[ "F_TARGET" ] = scriptTarget;
        repl[ "F_LIBS" ] = join( " -l", libs, true) + join( " -l", localLibs, true);
        repl[ "F_LIBDIRS" ] = joinUniq( " -L", searchLibDirs, true);
        repl[ "F_PLTF_LIBS" ] = pltfLibArg;
        repl[ "F_PLTF_LIBDIRS" ] = pltfLibDirArg;
        repl[ "F_TOOL_LIBS" ] = toolLibArg;
        repl[ "F_TOOL_LIBDIRS" ] = toolLibDirArg;
        
        sm->addReplacements( target_file_id, repl);
    }
    
    
    if( type == "library" || type == "executable" || type == "staticlib" || type == "static" )
    {
        size_t i;
        
        if( verbosity > 1 )
            cout << "Cmds for node " << getDir() << ". Creating commands for target " << target << "\n";
        
        string o_outputdir = getObjDir();
        
        file_id_t fid = 0;

        set<file_id_t>::const_iterator fileit = fileIds.begin();  // file ids controlled by FileMap via addFileId
        
        for( ; fileit != fileIds.end(); fileit++)   // iterates over the file ids from *this* node
        {
            file_id_t tid = *fileit;
            string srcfn = fileMan.getFileForId( tid );
            assert( srcfn != "?" );
            
            if( blockedIds.find( tid ) != blockedIds.end() )
            {
                if( verbosity > 1 )
                    cout << "File " << srcfn << " already used by an extension.\n";
                
                continue;
            }
            
            string dummy, bn, ext, bext;
            
            breakPath( srcfn, dummy, bn);
            breakFileName( bn, bext, ext);
            
            if( ext == ".cpp" || ext == ".C" || ext == ".cc" )
            {
                string o = o_outputdir + bext + ".o";
                
                if( fileMan.hasFileName( o ) )
                {
                    file_id_t obj_id = fileMan.getIdForFile( o );
                    
                    if( blockedIds.find( obj_id ) != blockedIds.end() )
                    {
                        if( verbosity > 1 )
                            cout << "Object file " << o << " already used by an extension.\n";
                        fileMan.addDependency( target_file_id, obj_id);
                        continue;
                    }
                }
                
                if( verbosity > 1 )
                    cout << "Cmd for node " << getDir() << ": Cpp " << o << "\n";
                fid = fileMan.addCommand( o, "Cpp", this);
                fileMan.addDependency( fid, tid);
                
                pegCppScript( fid, srcfn, o);
                objs.push_back( o );
            }
            else if( ext == ".c" )
            {
                string o = o_outputdir + bext + ".o";
                
                if( fileMan.hasFileName( o ) )
                {
                    file_id_t obj_id = fileMan.getIdForFile( o );
                    
                    if( blockedIds.find( obj_id ) != blockedIds.end() )
                    {
                        if( verbosity > 1 )
                            cout << "Object file " << o << " already used by an extension.\n";
                        fileMan.addDependency( target_file_id, obj_id);
                        continue;
                    }
                }
                
                if( verbosity > 1 )
                    cout << "Cmd for node " << getDir() << ": C " << o << "\n";
                fid = fileMan.addCommand( o, "C", this);
                fileMan.addDependency( fid, tid);
                
                pegCScript( fid, srcfn, o);
                objs.push_back( o );
            }
        }
        
        for( i = 0; i < objs.size(); i++)
        {
            file_id_t obj_id = fileMan.getIdForFile( objs[i] );
            assert( obj_id != -1 );
                
            /*  if( blockedIds.find( obj_id ) != blockedIds.end() )
            {
                if( verbosity > 1 )
                    cout << "Referenced object file " << objs[i] << " (" << obj_id << ") already used by an extension.\n";
                continue;
                }*/
            fileMan.addDependency( target_file_id, obj_id);
        }
        sm->addReplacement( target_file_id, "F_IN", join( " ", objs, false));   // linker input files
        
        for( i = 0; i < libs.size(); i++)
        {
            string dir = libsMap[ libs[i] ];
                
            file_id_t tid = fileMan.addCommand( stackPath( dir, ps->getSoFileName( libs[i] )), "L", this);
            fileMan.addDependency( target_file_id, tid);
        }
    }
}


void BaseNode::pegCppScript( file_id_t cpp_id, const string &in, const string &out)
{
    PlatformSpec *ps = PlatformSpec::getThePlatformSpec();
    ScriptManager *sm = ScriptManager::getTheScriptManager();
    map<string,string> repl;
    
    sm->setTemplateFileName( cpp_id, "ferret_cpp.sh.templ", "ferret_cpp__#.sh");

    repl[ "F_COMPILER" ] = ps->getCppCompiler();
    stringstream con;
    con << cpp_id;
    repl[ "F_ID" ] = con.str();
    repl[ "F_CPPFLAGS" ] = cppflagsArg;
    repl[ "F_PLTF_INCDIRS" ] = pltfIncArg;
    repl[ "F_INCLUDES" ] = includesArg;
    repl[ "F_TOOL_INCDIRS" ] = toolIncArg;
    repl[ "F_IN" ] = in;
    repl[ "F_OUT" ] = out;
    repl[ "F_TARGET" ] = scriptTarget;
    repl[ "F_SRCDIR" ] = getSrcDir();
    
    sm->addReplacements( cpp_id, repl);
}


void BaseNode::pegCScript( file_id_t c_id, const string &in, const string &out)
{
    PlatformSpec *ps = PlatformSpec::getThePlatformSpec();
    ScriptManager *sm = ScriptManager::getTheScriptManager();
    map<string,string> repl;
    
    sm->setTemplateFileName( c_id, "ferret_c.sh.templ", "ferret_c__#.sh");
    
    repl[ "F_COMPILER" ] = ps->getCCompiler();
    stringstream con;
    con << c_id;
    repl[ "F_ID" ] = con.str();
    repl[ "F_CFLAGS" ] = cflagsArg;
    repl[ "F_PLTF_INCDIRS" ] = pltfIncArg;
    repl[ "F_INCLUDES" ] = includesArg;
    repl[ "F_TOOL_INCDIRS" ] = toolIncArg;
    repl[ "F_IN" ] = in;
    repl[ "F_OUT" ] = out;
    repl[ "F_TARGET" ] = scriptTarget;
    repl[ "F_SRCDIR" ] = getSrcDir();
    
    sm->addReplacements( c_id, repl);
}


bool BaseNode::doDeep()       // wether to follow library, via downward pointers, to all downward nodes or not
{
    if( type == "library" )
    {
        if( BuildProps::getTheBuildProps()->hasKey( "FERRET_DEEP" ) &&
            BuildProps::getTheBuildProps()->getBoolValue( "FERRET_DEEP" ) )
            return true;
        else
            return false;
    }
    else
        return true;
}


BaseNode *BaseNode::getNodeByName( const string &nodeName )
{
    if( nodeName.length() == 0 || nodeName == "*" )
        return 0;
    
    if( nameToNodeMap.find( nodeName ) != nameToNodeMap.end() )
        return nameToNodeMap[ nodeName ];
    else
        return 0;
}


void BaseNode::addExtensions( const vector<Extension> &ext )
{
    extensions = ext;
}


void BaseNode::setExtensionUsed( const string &ext, const string &fileName)   // prevent using the same extension twice for same file
{
    extensionUsed[ fileName ] = ext;
}


bool BaseNode::isExtensionUsed( const string &ext, const string &fileName)
{
    map<string,string>::iterator it = extensionUsed.find( fileName );
    if( it != extensionUsed.end() )
        return it->second == ext;
    else
        return false;
}


void BaseNode::createExtensionCommands( FileManager &fileMan )
{
    ExtensionManager *em = ExtensionManager::getTheExtensionManager();
    size_t i;
    for( i = 0; i < extensions.size(); i++)
    {
        string type = extensions[i].name;

        ExtensionBase *e = em->createExtensionDriver( type, this);
        if( !e )
        {
            cerr << "warning: extension type '" << type << "' not implemented.\n";
        }
        else
        {
            e->createCommands( fileMan, extensions[i].attribs);
            
            set<file_id_t> bids = e->getBlockedIds();            
            blockedIds.insert( bids.begin(), bids.end());
            
            delete e;
        }
    }
}
