
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


static map<string, ProjectXmlNode *> nameToNodeMap;

bool ProjectXmlNode::xmlHasErrors = false;

ProjectXmlNode::ProjectXmlNode( const string &d, const string &module, const string &name,
                                const string &target, const string &type,
                                const vector<string> &cppflags, const vector<string> &cflags,
                                const vector<string> &usetools, const vector<string> &localLibs)
    : dir(d), target(target), type(type), cppflags(cppflags), cflags(cflags), localLibs(localLibs),
      module(module), name(name), madeObjDir(false), madeBinDir(false), madeLibDir(false), usetools(usetools),
      target_file_id(-1)
{
    nodeName = module + "*" + name;
    nameToNodeMap[ nodeName ] = this;

    srcdir = stackPath( dir, "src");
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


void ProjectXmlNode::assignBuildProperties()
{
    prjBuildProps = BuildProps::getTheBuildProps()->getPrjBuildProps( this );
}


string ProjectXmlNode::getObjDir()
{
    if( !madeObjDir )
    {
        assert( prjBuildProps.getObjDir() != "" );
        mkdir_p( prjBuildProps.getObjDir() );
        madeObjDir = true;
    }
    
    return prjBuildProps.getObjDir();
}

string ProjectXmlNode::getBinDir()
{
    if( !madeBinDir )
    {
        assert( prjBuildProps.getBinDir() != "" );
        mkdir_p( prjBuildProps.getBinDir() );
        madeBinDir = true;
    }
    
    return prjBuildProps.getBinDir();
}

string ProjectXmlNode::getLibDir()
{
    if( !madeLibDir )
    {
        assert( prjBuildProps.getLibDir() != "" );
        mkdir_p( prjBuildProps.getLibDir() );
        madeLibDir = true;
    }
    
    return prjBuildProps.getLibDir();
}


void ProjectXmlNode::traverseSrc()
{
    if( FindFiles::exists( srcdir ) )
        nodeFiles.traverse( srcdir,  false /* not deep */);
}


vector<File> ProjectXmlNode::getFiles() const
{
    return nodeFiles.getFiles();
}


void ProjectXmlNode::traverseIncdir( const string &incdir )
{
    if( FindFiles::exists( incdir ) )
        incdirFiles.appendTraverse( incdir, false /* not deep */);
    else
        cerr << "warning: incdir '" << incdir << "' not found\n";
}


int ProjectXmlNode::manageDeletedFiles( FileManager &fileMan )
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
            
            if( fileMan.removeFile( fn ) )  // will only remove "D" type files from database
            {
                cout << "Removing file " << fn << " from db if D type.\n";
                
                IncludeManager::getTheIncludeManager()->removeFile( fn, this);
                cnt++;
            }
        }
    }
    
    return cnt;
}


int ProjectXmlNode::checkForNewFiles( FileManager &fileMan )
{
    size_t i;
    int cnt = 0;
    vector<File> sources = nodeFiles.getSourceFiles();
    
    for( i=0; i < sources.size(); i++)
    {
        const File &f = sources[i];
        
        if( !fileMan.hasFileName( f.getPath() ) )   // already in files db?
        {
            sourceIds.push_back( fileMan.addNewFile( f.getPath(), this) );
            cnt++;
        }
        else
            sourceIds.push_back( fileMan.getIdForFile( f.getPath() ) );
    }

    return cnt;
}


int ProjectXmlNode::checkForNewIncdirFiles( FileManager &fileMan )
{
    size_t i;
    int cnt = 0;
    vector<File> headers = incdirFiles.getHeaderFiles();       // files referred to with <incdir> tag
    
    for( i=0; i < headers.size(); i++)
    {
        const File &f = headers[i];
        
        if( !fileMan.hasFileName( f.getPath() ) )              // already in files db?
        {
            fileMan.addNewFile( f.getPath(), 0);               // this is done for the rare cases where the XML node being referred to has not been visited already
                                                               // currently files from core_as/ips_txn/src
            cnt++;
        }
    }
    
    return cnt;
}


void ProjectXmlNode::createCommandArguments( const string &compileMode )
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


void ProjectXmlNode::createCommands( FileManager &fileMan )
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
            
            if( ext == ".cpp" || ext == ".C" )
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


void ProjectXmlNode::pegCppScript( file_id_t cpp_id, const string &in, const string &out)
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


void ProjectXmlNode::pegCScript( file_id_t c_id, const string &in, const string &out)
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


bool ProjectXmlNode::doDeep()       // wether to follow library, via downward pointers, to all downward nodes or not
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


ProjectXmlNode *ProjectXmlNode::getNodeByName( const string &nodeName )
{
    if( nodeName.length() == 0 || nodeName == "*" )
        return 0;
    
    if( nameToNodeMap.find( nodeName ) != nameToNodeMap.end() )
        return nameToNodeMap[ nodeName ];
    else
        return 0;
}


void ProjectXmlNode::addExtensions( const vector<Extension> &ext )
{
    extensions = ext;
}


void ProjectXmlNode::setExtensionUsed( const string &ext, const string &fileName)   // prevent using the same extension twice for same file
{
    extensionUsed[ fileName ] = ext;
}


bool ProjectXmlNode::isExtensionUsed( const string &ext, const string &fileName)
{
    map<string,string>::iterator it = extensionUsed.find( fileName );
    if( it != extensionUsed.end() )
        return it->second == ext;
    else
        return false;
}


void ProjectXmlNode::createExtensionCommands( FileManager &fileMan )
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


/*
void ProjectXmlNode::addExtensionSourceId( file_id_t id )
{
    sourceIds.push_back( id );
}
*/
