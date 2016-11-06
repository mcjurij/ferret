
#include <iostream>
#include <sstream>
#include <cassert>

#include "project_xml_node.h"
#include "simple_xml_stream.h"
#include "platform_spec.h"
#include "glob_utility.h"
#include "extension.h"
#include "engine.h"

using namespace std;

//#define USE_CPP_RELATIVE_TO_SRC_HACK

#ifdef USE_CPP_RELATIVE_TO_SRC_HACK
static string join_hack( const string &sep1, const vector<string> &a, bool beforeFirst)
{
    size_t i;
    string s;
    int n = 0;
    for( i=0; i < a.size(); i++)
    {
        if( a[i].length() > 0 )
        {
            if( n==0 && beforeFirst )
                s.append( sep1 );
            else if( n>0 )
                s.append( sep1 );
            n++;
            s.append( "../../../" + a[i] );
        }
    }
    
    return s;
}
#endif

static map<string, ProjectXmlNode *> nameToNodeMap;

bool ProjectXmlNode::xmlHasErrors = false;

ProjectXmlNode::ProjectXmlNode( const string &d, const string &module, const string &name,
                                const string &target, const string &type,
                                const vector<string> &cppflags, const vector<string> &cflags,
                                const vector<string> &usetools)
    : dir(d), target(target), type(type), cppflags(cppflags), cflags(cflags), module(module), name(name),
      madeObjDir(false), madeBinDir(false), madeLibDir(false), usetools(usetools), target_file_id(-1)
{
    nodeName = module + "*" + name;
    nameToNodeMap[ nodeName ] = this;

    srcdir = stackPath( dir, "src");
}


static map<string,ProjectXmlNode *> dirToNodeMap;
static string nodeStack[50];

ProjectXmlNode *ProjectXmlNode::traverseXml( const string &start, int level)
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
    
    SimpleXMLStream *xmls = new SimpleXMLStream( fpXml );
    if( verbosity > 0 )
        cout << "level " << level << " - XML node: " << start << " -------------------------------------\n";
    
    vector<string> subDirs;
    string module;
    string name;
    string target;
    string type;
    vector<string> incdirs;  // incdir tag allows direct setting of additional include directory
    vector<string> cppflags; // c++ flags valid only for this node
    vector<string> cflags;   // c flags valid only for this node
    vector<string> usetools;
    vector<Extension> extensions;
    
    while( !xmls->AtEnd() )
    {
        xmls->ReadNext();

        if( xmls->HasError() )
            break;
        
        if( xmls->IsStartElement() )  // start or self closing
        {
            if( xmls->Path() == "/project" )
            {
                 SimpleXMLAttributes attr = xmls->Attributes();

                 if( attr.HasAttribute( "module" ) )
                 {
                     module = attr.Value( "module" );
                 }

                 if( attr.HasAttribute( "name" ) )
                 {
                     name = attr.Value( "name" );
                 }
                 
                 if( attr.HasAttribute( "target" ) )
                 {
                     target = attr.Value( "target" );
                 }

                 if( attr.HasAttribute( "type" ) )
                 {
                     type = attr.Value( "type" );
                 }
            }
            else if( xmls->Path() == "/project/sub" )
            {
                SimpleXMLAttributes attr = xmls->Attributes();
                
                if( attr.HasAttribute( "name" ) )
                {
                    subDirs.push_back( attr.Value( "name" ) );
                }
            }
            else if( xmls->Path() == "/project/incdir" )
            {
                SimpleXMLAttributes attr = xmls->Attributes();
                
                if( attr.HasAttribute( "value" ) )
                    incdirs.push_back( attr.Value( "value" ) );
            }
            else if( xmls->Path() == "/project/cppflags" )
            {
                SimpleXMLAttributes attr = xmls->Attributes();
                
                if( attr.HasAttribute( "name" ) )
                {
                    cppflags.push_back( attr.Value( "name" ) );
                }
            }
            else if( xmls->Path() == "/project/cflags" )
            {
                SimpleXMLAttributes attr = xmls->Attributes();
                
                if( attr.HasAttribute( "name" ) )
                {
                    cflags.push_back( attr.Value( "name" ) );
                }
            }
            else if( xmls->Path() == "/project/usetool" )
            {
                SimpleXMLAttributes attr = xmls->Attributes();
                
                if( attr.HasAttribute( "name" ) )
                    usetools.push_back( attr.Value( "name" ) );
            }
        }
        else if( xmls->IsEndElement() )
        {
            if( xmls->Path() == "/project" && xmls->Name() != "sub" && xmls->Name() != "incdir" &&
                xmls->Name() != "cppflags" && xmls->Name() != "cflags" && xmls->Name() != "lflags" && xmls->Name() != "eflags" &&
                xmls->Name() != "usetool"  )
            {
                map<string,string> attribs;
                SimpleXMLAttributes attr = xmls->Attributes();
                size_t i;
                for( i = 0; i < attr.size(); i++)
                    attribs[ attr[i].Name() ] = attr[i].Value();
                
                extensions.push_back( Extension( xmls->Name(), attribs) );
            }
        }
    }
    
    ProjectXmlNode *node = 0;
    if( !xmls->HasError() )
    {
        node = new ProjectXmlNode( start, module, name, target, type,
                                   cppflags, cflags, usetools);
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
            string entrypoint = node->subDirs[ i ];
            
            ProjectXmlNode *d = traverseXml( entrypoint, level + 1);
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
            
            if( dep->getType() == "library" )
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
        nodeFiles.traverse( srcdir );
}


vector<File> ProjectXmlNode::getFiles() const
{
    return nodeFiles.getFiles();
}


void ProjectXmlNode::traverseIncdir( const string &incdir )
{
    if( FindFiles::exists( incdir ) )
        incdirFiles.appendTraverse( incdir );
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

#ifdef USE_CPP_RELATIVE_TO_SRC_HACK
    incss << join_hack( " -I", searchIncDirs, true); // hack to prepend ../../
#else
    incss << join( " -I", searchIncDirs, true);     // includes from this node
#endif
    includesArg = incss.str();
    toolIncArg = toolincss.str();
    
    cppflagss << join( " ", cppflags, true);       // c++ flags from this node
    cppflagsArg = cppflagss.str();
    
    cflagss << join( " ", cflags, true);           // c flags from this node
    cflagsArg = cflagss.str();
    
    lflagsArg = lflagss.str();
    eflagsArg = eflagss.str();

    toolLibArg    = toolLibss.str();
    toolLibDirArg = toolLibDirss.str();
    
    if( type == "library" )
        scriptTarget = ps->getSoFileName( target );            
    else if( type == "executable" )
        scriptTarget = target;
}


void ProjectXmlNode::createCommands( FileManager &fileMan )
{
    PlatformSpec *ps = PlatformSpec::getThePlatformSpec();
    string compileMode = fileMan.getCompileMode();
    
    if( type == "library" || type == "executable" )
    {
        string l_outputdir = getLibDir();
        string b_outputdir = getBinDir();
        
        if( type == "library" )
        {
            string t = l_outputdir + scriptTarget;
            target_file_id = fileMan.addCommand( t, "L", this);
            
            setExtensionScriptTemplNameForId( target_file_id, "ferret_l.sh.templ", "ferret_l__#__" + compileMode + ".sh");
            setReplKeyValueForId( target_file_id, "F_LFLAGS", lflagsArg);
            setReplKeyValueForId( target_file_id, "F_OUT", t);
        }
        else if( type == "executable" )
        {
            string t = b_outputdir + scriptTarget;
            target_file_id = fileMan.addCommand( t, "X", this);

            setExtensionScriptTemplNameForId( target_file_id, "ferret_x.sh.templ", "ferret_x__#__" + compileMode + ".sh");
            setReplKeyValueForId( target_file_id, "F_EFLAGS", eflagsArg);
            setReplKeyValueForId( target_file_id, "F_OUT", t);
        }
        
        setReplKeyValueForId( target_file_id, "F_ID", target_file_id);
        setReplKeyValueForId( target_file_id, "F_TARGET", scriptTarget);
        setReplKeyValueForId( target_file_id, "F_LIBS", join( " -l", libs, true));
        setReplKeyValueForId( target_file_id, "F_LIBDIRS", joinUniq( " -L", searchLibDirs, true));
        setReplKeyValueForId( target_file_id, "F_PLTF_LIBS", pltfLibArg);
        setReplKeyValueForId( target_file_id, "F_PLTF_LIBDIRS", pltfLibDirArg);
        setReplKeyValueForId( target_file_id, "F_TOOL_LIBS", toolLibArg);       
        setReplKeyValueForId( target_file_id, "F_TOOL_LIBDIRS", toolLibDirArg);
    }
    
    
    if( type == "library" || type == "executable" )
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
                
#ifdef USE_CPP_RELATIVE_TO_SRC_HACK
                pegCppScript( fid, bn, o, compileMode);
#else
                pegCppScript( fid, srcfn, o, compileMode);
#endif
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
                
                pegCScript( fid, srcfn, o, compileMode);
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
        setReplKeyValueForId( target_file_id, "F_IN", join( " ", objs, false));   // linker input files
        
        for( i = 0; i < libs.size(); i++)
        {
            string dir = libsMap[ libs[i] ];
                
            file_id_t tid = fileMan.addCommand( stackPath( dir, ps->getSoFileName( libs[i] )), "L", this);
            fileMan.addDependency( target_file_id, tid);
        }
    }
}


void ProjectXmlNode::pegCppScript( file_id_t cpp_id, const string &in, const string &out, const string &compileMode)
{
    PlatformSpec *ps = PlatformSpec::getThePlatformSpec();
    
    setExtensionScriptTemplNameForId( cpp_id, "ferret_cpp.sh.templ", "ferret_cpp__#__" + compileMode + ".sh");

    setReplKeyValueForId( cpp_id, "F_COMPILER", ps->getCppCompiler());
    setReplKeyValueForId( cpp_id, "F_ID", cpp_id);
    setReplKeyValueForId( cpp_id, "F_CPPFLAGS", cppflagsArg);
    setReplKeyValueForId( cpp_id, "F_PLTF_INCDIRS", pltfIncArg);
    setReplKeyValueForId( cpp_id, "F_INCLUDES", includesArg);
    setReplKeyValueForId( cpp_id, "F_TOOL_INCDIRS", toolIncArg);
    setReplKeyValueForId( cpp_id, "F_IN", in);
    setReplKeyValueForId( cpp_id, "F_OUT", out);
    setReplKeyValueForId( cpp_id, "F_TARGET", scriptTarget);
    setReplKeyValueForId( cpp_id, "F_SRCDIR", getSrcDir());
}


void ProjectXmlNode::pegCScript( file_id_t c_id, const string &in, const string &out, const string &compileMode)
{
    PlatformSpec *ps = PlatformSpec::getThePlatformSpec();
    
    setExtensionScriptTemplNameForId( c_id, "ferret_c.sh.templ", "ferret_c__#__" + compileMode + ".sh");
    
    setReplKeyValueForId( c_id, "F_COMPILER", ps->getCCompiler());
    setReplKeyValueForId( c_id, "F_ID", c_id);
    setReplKeyValueForId( c_id, "F_CFLAGS", cflagsArg);
    setReplKeyValueForId( c_id, "F_PLTF_INCDIRS", pltfIncArg);
    setReplKeyValueForId( c_id, "F_INCLUDES", includesArg);
    setReplKeyValueForId( c_id, "F_TOOL_INCDIRS", toolIncArg);
    setReplKeyValueForId( c_id, "F_IN", in);
    setReplKeyValueForId( c_id, "F_OUT", out);
    setReplKeyValueForId( c_id, "F_TARGET", scriptTarget);
    setReplKeyValueForId( c_id, "F_SRCDIR", getSrcDir());
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
            map<file_id_t, map<string,string> > re = e->createCommands( fileMan, extensions[i].attribs);
            setReplMapForId( re );
            
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

void ProjectXmlNode::setReplMapForId( const map<file_id_t,map<string,string> > &re)
{
    map<file_id_t, map<string,string> >::iterator it;
    map<file_id_t, map<string,string> >::const_iterator itn;
    map<string,string>::const_iterator in;
    
    for( itn = re.begin(); itn != re.end(); itn++)
    {
        file_id_t id = itn->first;
        it = replacements.find( id );

        if( it != replacements.end() )
        {
            for( in = re.at( id ).begin(); in != re.at( id ).end(); in++)
            {
                replacements[ id ][ in->first ] = in->second;  // merge stuff with same file id
            }
        }
        else
            replacements[ id ] = re.at( id );
    }
}


void ProjectXmlNode::setReplKeyValueForId( file_id_t id, const string &key, const string &val)
{
    map<string,string> m;
    m[ key ] = val;
    map<int,map<string,string> > re;
    re[ id ] = m;
    
    setReplMapForId( re );
}


void ProjectXmlNode::setReplKeyValueForId( file_id_t id, const string &key, int val)
{
    stringstream vss;
    vss << val;
    map<string,string> m;
    m[ key ] = vss.str();
    map<int,map<string,string> > re;
    re[ id ] = m;
    
    setReplMapForId( re );
}


map<string,string> ProjectXmlNode::getReplMapForId( file_id_t id )
{
    map<file_id_t, map<string,string> >::iterator it = replacements.find( id );
    
    if( it != replacements.end() )
        return it->second;
    else
        return map<string,string>();
}


void ProjectXmlNode::setExtensionScriptTemplNameForId( file_id_t id, const string &scriptTempl, const string &script)
{
    extensionScriptTemplNames[ id ] = scriptTempl;
    extensionScriptNames[ id ] = script;
}


string ProjectXmlNode::getExtensionScriptTemplNameForId( file_id_t id )
{
    map<file_id_t,string>::iterator it = extensionScriptTemplNames.find( id );
    if( it != extensionScriptTemplNames.end() )
        return it->second;
    else
        return "MISSING";
}


string ProjectXmlNode::getExtensionScriptNameForId( file_id_t id )
{
    map<file_id_t,string>::iterator it = extensionScriptNames.find( id );
    if( it != extensionScriptNames.end() )
        return it->second;
    else
        return "MISSING";
}
