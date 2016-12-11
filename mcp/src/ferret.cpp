// build:
// gcc 5: g++ -std=c++11 ...
// gcc 4: g++  -Wall -o ferret ferret.cpp project_xml_node.o executor.o SimpleXMLStream.o parse_dep.o file_manager.o hash_map.o hash_set.o find_files.o engine.o platform_spec.o glob_utility.o extension.o script_template.o output_collector.o ...

#include <iostream>

#include <cassert>
#include <algorithm>
#include <unistd.h>

#include "project_xml_node.h"
#include "traverse_structure.h"
#include "find_files.h"
#include "executor.h"
#include "parse_dep.h"
#include "engine.h"
#include "platform_spec.h"
#include "glob_utility.h"
#include "build_props.h"
#include "extension.h"
#include "include_manager.h"
#include "file_cleaner.h"
#include "output_collector.h"

using namespace std;

static const string ferretVersion = "0.9.8";


void printXmlStructure( ProjectXmlNode *node, int level=0)
{
    if( level>50 )
    {
        cerr << "error: xml structure too deep.\n";
        return;
    }
    
    char buf[10];
    snprintf( buf, 10, "%02d", level);
    cout << buf << string( level*4, ' ') << node->getDir() << "\n";

    for( size_t i=0; i < node->childNodes.size(); i++)
    {
        ProjectXmlNode *d = node->childNodes[ i ];
        printXmlStructure( d, level + 1);
    }
}


// -----------------------------------------------------------------------------

static void show_usage()
{
    cout << "ferret [options] [base_dir]\n";
    cout <<
        "   --init  <base_dir>      write new file db, <base_dir> must contain build directory, compile mode is DEBUG\n"
        "   -M <mode> <base_dir>    init with compile mode <mode>\n"
        "   -p <n>                  use <n> processors, default is 1\n"
        "   --stop                  stop on first error, default is ignore\n"
        "   --deep                  follow dependencies of shared libraries\n"
        "   --prop <properties>     use <properties> file when init, default is build/build_$HOSTNAME.properties\n"
        "   --proj <project_dir>    use <project_dir> as root node, default is build (using build/project.xml)\n"
        "   -q                      quick mode, build project of the sub directory only\n"
        "   -c                      full clean\n"
        "   --eclean                object/library/executable clean\n"
        "   --oclean                object only clean\n"
        "   -d <n>                  recursion depth of <n> during clean, default is maximum\n"
        "   --ignhdr                write a list of headers to ignore\n"
        "   --scfs                  support crappy file systems, having imprecise time stamps\n"
        "   -t <target>             build only <target> (can be comma separated list), default is all\n"
        "   --make                  write Makefile\n"
        "   --html                  write HTML directory containing depedency structure\n"
        "   --info                  show info level messages\n"
        "   -v,-vv,-vvv             verbosity, more verbosity, incredible verbosity\n"
        "   --times                 show various time consumptions\n\n"
        "Version " + ferretVersion + "\n";
    
    exit(2);
}

static void emergency_exit( int exc )
{
    cerr << "ferret can't continue.\n";
    exit( exc );
}


static string walk_for_filesdb( string &projDir )
{
    struct stat fst;
    int k=0;
    string up;
    bool found = false;

    string  help, helpdir, proj;
    char  cwd[1025];
    getcwd( cwd, 1024);
    
    for( k = 0; k < 5; k++)
    {
        string path = up + "ferret_db";
        if( stat( path.c_str(), &fst) == 0 )
        {
            if( S_ISDIR( fst.st_mode ) )
            {
                found = true;
                if( up != "" )
                    cout << "Using the files db from " << up << "\n"; 
                break;
            }
        }

        helpdir = stackPath( cwd, up);
        help =  helpdir + "/project.xml";
        
        if( FindFiles::existsUncached( help ) )
            proj = helpdir;
        
        up += "../";
    }

    helpdir = stackPath( cwd, up);
    if( found && proj != "" )
    {
        if( proj.length() > helpdir.length() )
            proj = proj.substr( helpdir.length()+1 );
        
        projDir = proj;
    }
    
    if( found )
        return (k==0) ? "." : up;
    else
        return  "";
}


static string selectPlatformSpec( const string &buildDir )
{
    char *user = getenv( "USER" );
    char *host = getenv( "HOSTNAME" );
    string fn;
    
    if( user )
    {
        fn = stackPath( buildDir, "platform_"  + string(user) + ".xml");
        if( FindFiles::existsUncached( fn ) )
            return fn;
    }
    else if( host == 0 )
    {
        cerr << "error: HOSTNAME not set.\n";
        return "";
    }
    
    return stackPath( buildDir, "platform_" + string(host) + ".xml");
}


static void set_up_mbd_set( const string &dbProjDir )
{
    global_mbd_set = new_hash_set( 19 );
    
    string mbdfn = stackPath( dbProjDir, "ferret_mbd");
    FILE *mbdfp = fopen( mbdfn.c_str(), "r");
    
    if( mbdfp )
    {
        hash_set_read( mbdfp, global_mbd_set);

        if( hash_set_get_size( global_mbd_set ) > 0 )
        {
            cout << "Marked by deletion: ";
            hash_set_print( global_mbd_set );
            cout << "\n";
        }

        fclose( mbdfp );
    }
}


static void remove_mbd( const string &dbProjDir )
{
    string mbdfn = stackPath( dbProjDir, "ferret_mbd");
    if( FindFiles::existsUncached( mbdfn ) )
        ::remove( mbdfn.c_str() );
}


static void store_mbd_set( const string &dbProjDir )
{
    assert( global_mbd_set );
    
    string mbdfn = stackPath( dbProjDir, "ferret_mbd");
    if( hash_set_get_size( global_mbd_set ) > 0 )
    {
        FILE *mbdfp = fopen( mbdfn.c_str(), "w");
        
        if( mbdfp )
        {
            hash_set_write( mbdfp, global_mbd_set);
            
            fclose( mbdfp );
        }
        else
            cerr << "error: could not save marked by deletion set.\n";
    }
    else
        remove_mbd( dbProjDir );
}


static string selectBuildProperties( const string &buildDir )
{
    char *user = getenv( "USER" );
    char *host = getenv( "HOSTNAME" );
    string fn;
    
    if( user )
    {
        fn = "build_" + string(user) + ".properties";
        if( FindFiles::existsUncached( fn ) )
            return fn;
        else
        {
            fn = stackPath( buildDir, "build_" + string(user) + ".properties");
            if( FindFiles::existsUncached( fn ) )
                return fn;
        }
    }
    else if( host == 0 )
    {
        cerr << "error: HOSTNAME not set.\n";
        return "";
    }
    
    return stackPath( buildDir, "build_" + string(host) + ".properties");
}


static bool checkBuildPropsTime( const string &propsfn, const string &projDir)
{
    string filesdb = stackPath( stackPath( ferretDbDir, projDir), "ferret_files");
    if( !FindFiles::existsUncached( filesdb.c_str() ) )
        return false;
    
    const File f = FindFiles::getUncachedFile( propsfn );
    const File fdb = FindFiles::getUncachedFile( filesdb.c_str() );
    
    return f.isOlderThan( fdb );
}


bool doScfs = false;
bool hasUseCpu = false;   
int use_cpus = 1; 
bool compileModeSet = false;
string compileMode;
bool stopOnErr = false;
bool downwardDeep = false;

static void readBuildProperties( bool initMode, const string &build_properies, const string &projDir)
{
    if( verbosity > 0 )
        cout << "Using properties from '" << build_properies << "'\n";
    
    if( !BuildProps::getTheBuildProps()->read( build_properies ) )
    {
        cerr << "error: error(s) in '" << build_properies << "'. check your setup.\n";
        emergency_exit( 2 );
    }

    if( !initMode && !checkBuildPropsTime( build_properies, projDir) )
    {
        cerr << "error: build properties " << build_properies << " newer than file db, rerun using --init.\n";
        emergency_exit( 5 );
    }
    
    string host = getenv( "HOSTNAME" ) ? string( getenv( "HOSTNAME" ) ) : "unknown_host";
    string user = getenv( "USER" ) ? string( getenv( "USER" ) ) : "unknown_user";
    string home = getenv( "HOME" ) ? string( getenv( "HOME" ) ) : "unknown_home";
    
    BuildProps::getTheBuildProps()->setStaticProp( "host", host);
    BuildProps::getTheBuildProps()->setStaticProp( "user", user);
    BuildProps::getTheBuildProps()->setStaticProp( "home", home);
    BuildProps::getTheBuildProps()->setStaticProp( "compiler_version", PlatformSpec::getThePlatformSpec()->getCompilerVersion());
    
    if( !doScfs )
    {
        if( BuildProps::getTheBuildProps()->hasKey( "FERRET_SCFS" ) )
        {
            doScfs = BuildProps::getTheBuildProps()->getBoolValue( "FERRET_SCFS" );
            if( doScfs && verbosity > 0 )
                cout << "Crappy file system support turned on by build properties setting.\n";
        }
    }
    else
    {
        if( verbosity > 0 )
            cout << "Crappy file system support turned on by command line argument.\n";
        BuildProps::getTheBuildProps()->setBoolValue( "FERRET_SCFS", true);
    }
    
    if( !hasUseCpu )
    {
        if( BuildProps::getTheBuildProps()->hasKey( "FERRET_P" ) )
        {
            int p = BuildProps::getTheBuildProps()->getIntValue( "FERRET_P" );
            if( p > 0  )
            {
                if( verbosity > 0 )
                    cout << "P set to " << p << " by build properties setting.\n";
                use_cpus = p;
            }
        }
    }
    else if( verbosity > 0 )
        cout << "P set to " << use_cpus << " by command line argument.\n";

    if( use_cpus > 24 )
        use_cpus = 24;
    else if( use_cpus < 1 )
        use_cpus = 1;
    BuildProps::getTheBuildProps()->setIntValue( "FERRET_P", use_cpus);
    
    if( initMode )
    {
        if( !compileModeSet )
        {
            if( BuildProps::getTheBuildProps()->hasKey( "FERRET_M" ) )
            {
                string v = BuildProps::getTheBuildProps()->getValue( "FERRET_M" );
                if( v.length() > 0  )
                {
                    compileMode = v;
                    
                    if( verbosity > 0 )
                        cout << "Compile mode set to '" << compileMode << "' by build properties setting.\n";
                }
            }
        }
        else if( verbosity > 0 )
            cout << "Compile mode set to '" << compileMode << "' by command line argument.\n";
        
        if( !PlatformSpec::getThePlatformSpec()->hasCompileMode( compileMode ) )
        {
            cerr << "error: compile mode '" << compileMode << "' not found.\n";
            emergency_exit( 2 );
        }
        else
             BuildProps::getTheBuildProps()->setValue( "FERRET_M", compileMode);
    }

    if( !stopOnErr )
    {
        if( BuildProps::getTheBuildProps()->hasKey( "FERRET_STOP" ) )
        {
            stopOnErr = BuildProps::getTheBuildProps()->getBoolValue( "FERRET_STOP" );
            if( stopOnErr && verbosity > 0 )
                cout << "Stop on first error turned on by build properties setting.\n";
        }
    }
    else
    {
        if( verbosity > 0 )
            cout << "Stop on first error turned on by command line argument.\n";
        BuildProps::getTheBuildProps()->setBoolValue( "FERRET_STOP", true);
    }

    if( !downwardDeep )
    {
        if( BuildProps::getTheBuildProps()->hasKey( "FERRET_DEEP" ) )
        {
            downwardDeep = BuildProps::getTheBuildProps()->getBoolValue( "FERRET_DEEP" );
            if( downwardDeep && verbosity > 0 )
                cout << "Deep mode turned on by build properties setting.\n";
        }
    }
    else
    {
        if( verbosity > 0 )
            cout << "Deep mode turned on by command line argument.\n";
        BuildProps::getTheBuildProps()->setBoolValue( "FERRET_DEEP", true);
    }
}


static void doBuild( FileManager &filesDb, set<string> &userTargets, const string &dbProjDir, bool printTimes)
{
    Executor executor( BuildProps::getTheBuildProps()->getIntValue( "FERRET_P" ) );
    bool doScfs = BuildProps::getTheBuildProps()->getBoolValue( "FERRET_SCFS" );
    Engine engine( 911, dbProjDir, doScfs);
    
    bool stopOnErr = BuildProps::getTheBuildProps()->getBoolValue( "FERRET_STOP" );
    engine.setStopOnError( stopOnErr );
    
    filesDb.sendToEngine( engine );
    
    engine.doWork( executor, printTimes, userTargets);
    
    store_mbd_set( dbProjDir );
}


//------------------------------------------------------------------------------
int main( int argc, char **argv)
{
    vector<string> args;     // non-option arguments
    int arg_err = 0;
    bool initMode = false;
    bool quickMode = false;
    bool doClean = false;
    bool doEClean = false;
    bool doObjOnlyClean = false;
    int clean_depth = 50;
    bool doWriteIgnHdr = false;
    bool printTimes = false;
    bool writeMakef = false;
    bool doProjHtml = false;
    bool initAndBuild = false;
    
    string targetArg, propertiesFileArg, startProjArg;
    set<string> userTargets;
    int i;

    compileMode = "DEBUG";
    verbosity = 0;
    show_info = false;
    
    for( i = 1; i<argc; i++)
    {
        string arg = argv[i];
        if( arg.at(0) == '-' )
        {
            if( arg == "--init" )
                initMode = true;
            else if( arg == "-q" )
                quickMode = true;
            else if( arg == "--times" )
                printTimes = true;
            else if( arg == "-p" )
            {
                if( (i+1)<argc )
                {
                    i++;
                    use_cpus = atoi( argv[i] );
                    hasUseCpu = true;
                }
                else
                {
                    cerr << "error: option p requires an argument. # of cpus\n";
                    arg_err++;
                }
            }
            else if( arg == "--stop" )
                stopOnErr = true;
            else if( arg == "--deep" )
                downwardDeep = true;
            else if( arg == "-t" )
            {
                if( (i+1)<argc )
                {
                    i++;
                    targetArg = argv[i];
                }
                else
                {
                    cerr << "error: option t requires an argument. list of targets\n";
                    arg_err++;
                }
            }
            else if( arg == "--make" )
                writeMakef = true;
            else if( arg == "--html" )
                doProjHtml = true;
            else if( arg == "-M" )
            {
                if( (i+1)<argc )
                {
                    i++;
                    compileMode = argv[i];
                    compileModeSet = true;
                }
                else
                {
                    cerr << "error: option M requires an argument. compile mode\n";
                    arg_err++;
                }
            }
            else if( arg == "-c" )
                doClean = true;
            else if( arg == "--eclean" )
                doEClean = true;
            else if( arg == "--oclean" )
                doObjOnlyClean = true;
            else if( arg == "-d" )
            {
                if( (i+1)<argc )
                {
                    i++;
                    clean_depth = atoi( argv[i] );
                }
                else
                {
                    cerr << "error: option d requires an argument. clean depth\n";
                    arg_err++;
                }
            }
            else if( arg == "--info" )
                show_info = true;
            else if( arg == "--ignhdr" )
                doWriteIgnHdr = true;
            else if( arg == "--scfs" )
                doScfs = true;
            else if( arg == "--prop" )
            {
                if( (i+1)<argc )
                {
                    i++;
                    propertiesFileArg = argv[i];
                }
                else
                {
                    cerr << "error: option prop requires an argument. build properties file name\n";
                    arg_err++;
                }
            }
            else if( arg == "--proj" )
            {
                if( (i+1)<argc )
                {
                    i++;
                    startProjArg = argv[i];
                }
                else
                {
                    cerr << "error: option proj requires an argument. project.xml directory\n";
                    arg_err++;
                }
            }
            else if( arg == "-v" )
                verbosity = 1;
            else if( arg == "-vv" )
                verbosity = 2;
            else if( arg == "-vvv" )
                verbosity = 3;
            else
            {
                cerr << "error: unknown option '" << arg << "'\n";
                arg_err++;
            }
        }
        else
            args.push_back( arg );
    }
    
    if( arg_err )
        show_usage();

    if( compileModeSet )
        initMode = true;
    
    if( initMode && args.size() == 0 )
        show_usage();

    if( !initMode && propertiesFileArg != "" )
        show_usage();

    if( quickMode && startProjArg != "" )
        show_usage();
    
    if( initMode && (doClean || doEClean || doObjOnlyClean) )
        show_usage();
    
    if( targetArg != "" )
    {
        vector<string> targets = split( ',', targetArg );
        for( vector<string>::iterator it = targets.begin(); it != targets.end(); it++)
            userTargets.insert( *it );
    }

    if( verbosity > 0 )
        cout << "ferret v" << ferretVersion << "\n";
    
    string startProjDir = "build";
    if( startProjArg != "" )
        startProjDir = startProjArg;
    
    ferretDbDir = "ferret_db";
    
    if( args.size() > 0 )
    {
        string chdirpath = args[0];
        
        if( chdirpath.length() == 0 )
        {
            cerr << "error: empty string as directory not possible.\n";
            emergency_exit( 5 );
        }
        else if( chdirpath != "." )
        {
            if( chdir( chdirpath.c_str() ) != 0 )
            {
                cerr << "error: can't change to '" << chdirpath << "'.\n";
                emergency_exit( 5 );
            }
        }
    }
    else if( !initMode )
    {
        string projDir;
        string chdirpath = walk_for_filesdb( projDir );
        
        if( chdirpath.length() == 0 )
        {
             cerr << "error: files db 'ferret_db' directory not found. No root.\n";
             emergency_exit( 5 );
        }
        else if( chdirpath != "." )
        {
            if( verbosity > 0 )
                cout << "Changing to '" << chdirpath << "' directory.\n";
            
            if( quickMode && startProjArg == "" )
            {
                cout << "Quick mode: Setting project directory to '" << projDir << "'\n";
                startProjDir = projDir;
            }
            
            if( chdir( chdirpath.c_str() ) != 0 )
            {
                cerr << "error: can't change to '" << chdirpath << "'.\n";
                emergency_exit( 5 );
            }
        }
        else if( quickMode )
        {
            cout << "Quick mode not sensible on top level. Ignored.\n";
            quickMode = false;
        }
        
        string fn = stackPath( stackPath( ferretDbDir, startProjDir), "ferret_files");
        if( !FindFiles::existsUncached( fn.c_str() ) )
        {
            cout << "Files db '" << fn << "' not found, switching to init mode.\n";
            initMode = initAndBuild = true;
        }
    }
    else
        mkdir_p( ferretDbDir );

    if( startProjDir.length() > 0 && startProjDir[0] == '/' )
    {
        cerr << "error: start project directory must be relative.\n";
        emergency_exit(5);
    }
    
    // at this point we are in the correct directory, which must contain the 'build' directory
    
    set_start_time();
    FileManager  filesDb;
    
    if( verbosity > 0 )
        cout << "Using '" << startProjDir << "' directory as project root.\n";
    string buildDir = "build";
    
    
    if( doClean || doEClean || doObjOnlyClean )
    {
        FileCleaner c;
        if( !c.readDb( startProjDir ) )
        {
            cerr << "error: error(s) in ferret file db, rerun using --init.\n";
            emergency_exit( 5 );
        }
        
        if( printTimes )
            cout << "reading files db took " << get_diff() << "s\n";

        if( clean_depth >= 0 )
            c.setDepth( clean_depth );

        if( doClean )
            c.setFullClean();
        else if( doEClean )
        {
            c.setLibExeClean();
            c.setObjClean();
        }
        else if( doObjOnlyClean )
            c.setObjClean();
        
        c.clean( userTargets );
        
        if( printTimes )
            cout << "cleaning took " << get_diff() << "s\n";
        
        exit(0);
    }
    
    string platform_spec = selectPlatformSpec( buildDir );
    string build_properies;

    if( initMode )
    {
        if( propertiesFileArg.length() > 0 )
            build_properies = propertiesFileArg;
        else
            build_properies = selectBuildProperties( buildDir );

        filesDb.setPropertiesFile( build_properies );
    }
    
    if( !PlatformSpec::getThePlatformSpec()->read( platform_spec ) )
    {
        cerr << "error: error(s) in '" << platform_spec << "'. check your setup.\n";
        emergency_exit( 2 );
    }

    string host = getenv( "HOSTNAME" ) ? string( getenv( "HOSTNAME" ) ) : "unknown_host";
    
    scriptTemplDir = buildDir + "/script_templ";
    tempDir = buildDir + "/temp/ferret/" + host;
    mkdir_p( tempDir );

    tempScriptDir = tempDir + "/scripts";
    tempDepDir = tempDir + "/deps";
    mkdir_p( tempScriptDir );
    mkdir_p( tempDepDir );
    
    setupExtensions();
    
    string dbProjDir = stackPath( ferretDbDir, startProjDir);
    mkdir_p( dbProjDir );
    set_up_mbd_set( dbProjDir );
    
    // start with reading all project xml files
    ProjectXmlNode *xmlRootNode = ProjectXmlNode::traverseXml( startProjDir );
    if( ProjectXmlNode::hasXmlErrors() )
        emergency_exit( 7 );
    
    if( xmlRootNode == 0 )
    {
        cerr << "error: no root node at '" << startProjDir << "'. check your setup.\n";
        emergency_exit( 5 );
    }
    
    if( printTimes )
        cout << "reading XML took " << get_diff() << "s\n";
    
    // printXmlStructure( xmlRootNode );
    TraverseStructure traverse( filesDb, xmlRootNode);
    
    if( !initMode )
    {
        if( !filesDb.readDb( startProjDir ) )
        {
            cerr << "error: error(s) in ferret file db, rerun using --init.\n";
            emergency_exit( 5 );
        }
        if( printTimes )
            cout << "reading files db took " << get_diff() << "s\n";
        
        readBuildProperties( false, filesDb.getPropertiesFile(), startProjDir);
        
        compileMode = filesDb.getCompileMode();
        BuildProps::getTheBuildProps()->setValue( "FERRET_M", compileMode);
        BuildProps::getTheBuildProps()->setStaticProp( "compile_mode", compileMode);
        
        traverse.traverseXmlStructureForChildren();
        if( printTimes )
            cout << "traversing structure for children " << get_diff() << "s\n";
        
        if( verbosity > 0 )
            cout << "Compile mode is '" << compileMode << "'\n";
        
        filesDb.persistMarkByDeletions( global_mbd_set );
        
        traverse.traverseXmlStructureForDeletedFiles();
        
        if( traverse.getCntRemoved() )
        {
            filesDb.seeWhatsNewOrGone();       // deletes gone files/commands
        }
        if( printTimes )
            cout << "checking for deleted files took " << get_diff() << "s\n";
    }
    else // initMode == true
    {
        readBuildProperties( true, build_properies, startProjDir);
        
        compileMode = BuildProps::getTheBuildProps()->getValue( "FERRET_M" );
        BuildProps::getTheBuildProps()->setStaticProp( "compile_mode", compileMode);
        filesDb.setCompileMode( compileMode );
        
        traverse.traverseXmlStructureForChildren();
        if( printTimes )
            cout << "traversing structure for children " << get_diff() << "s\n";
        
        if( verbosity > 0 )
            cout << "Compile mode is '" << compileMode << "'\n";
    }
            
    traverse.traverseXmlStructureForExtensions();
    if( printTimes )
        cout << "traversing structure for extensions took " << get_diff() << "s\n";
    
    traverse.traverseXmlStructureForNewFiles();
    traverse.traverseXmlStructureForNewIncdirFiles();
    if( !initMode && traverse.getCntNew() )
    {
        cout << "Found " << traverse.getCntNew() << " new file(s).\n";
    }
    if( printTimes )
        cout << "checking for new files took " << get_diff() << "s\n";
    
    if( !initMode && verbosity > 1 )
    {
        cout << " CHANGED -2: \n";
        filesDb.printWhatsChanged();
    }
    
    Executor incExecutor( BuildProps::getTheBuildProps()->getIntValue( "FERRET_P" ) );
    IncludeManager::getTheIncludeManager()->createMissingDepFiles( filesDb, incExecutor, printTimes);  // create all missing .d files
    if( incExecutor.isInterruptedBySignal() )
        emergency_exit(3);
    
    if( printTimes )
        cout << "updating dependency files took " << get_diff() << "s\n";

    if( initMode )
    {
        hash_set_clear( global_mbd_set );
        remove_mbd( dbProjDir );
        ::remove( stackPath( dbProjDir, "ferret_scfs").c_str() );
    }
    
    if( verbosity > 1 )
    {
        cout << " CHANGED 1: \n";
        filesDb.printWhatsChanged();
    }
    
    traverse.traverseXmlStructureForTargets();

    if( printTimes )
        cout << "traversing structure for targets took " << get_diff() << "s\n";
    
    if( verbosity > 1 )
    {
        cout << " CHANGED 2: \n";
        filesDb.printWhatsChanged();
    }

    IncludeManager::getTheIncludeManager()->resolve( filesDb, initMode, doWriteIgnHdr);
    if( printTimes )
        cout << "resolving dependencies took " << get_diff() << "s\n";
    
    filesDb.removeCycles();
    
    if( printTimes )
        cout << "removing cycles took " << get_diff() << "s\n";
    
    filesDb.writeDb( startProjDir );
    
    if( printTimes )
        cout << "writing files db took " << get_diff() << "s\n";
    
    if( !initMode )
    {
        if( verbosity > 1 )
        {
            cout << " CHANGED 3: \n";
            filesDb.printWhatsChanged();
        }
        
        if( doProjHtml )
        {
            mkdir_p( "ferret_html" );
            OutputCollector::getTheOutputCollector()->setProjectNodesDir( "ferret_html" );
            OutputCollector::getTheOutputCollector()->htmlProjectNodes( xmlRootNode, filesDb);
            
            if( printTimes )
                cout << "writing html files took " << get_diff() << "s\n";
        }
        else if( !writeMakef )
        {
            doBuild( filesDb, userTargets, dbProjDir, printTimes);
        }
        else
        {
            MakefileEngine engine( filesDb, "Makefile");
            filesDb.sendToMakefileEngine( engine );
            
            MakefileExec makeExec;
            engine.doWork( makeExec, printTimes);
        }
    }
    else if( initMode && initAndBuild )
        doBuild( filesDb, userTargets, dbProjDir, printTimes);
    
    return 0;
}

/*
  Brand: "What are you doing?"
  
  Cooper: "Docking."
  
  CASE: "Endurace's rotation is 67..68 RPM."
  
  Cooper: "Get ready to match up spin, retro thrust."
  
  CASE: "It's not possible."
  
  Cooper: "No, it's necessary."
*/
