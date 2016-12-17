#include <cstdio>
#include <iostream>
#include <sstream>

#include "platform_spec.h"
#include "simple_xml_stream.h"
#include "glob_utility.h"
#include "platform_defines.h"
#include "extension.h"
#include "output_collector.h"

using namespace std;


void ToolSpec::addCppFlag( const std::string &flag )
{
    cppflags.push_back( flag );
}


void ToolSpec::addCFlag( const std::string &flag )
{
    cflags.push_back( flag );
}


string ToolSpec::getIncCommandArgs() const
{
    return join( " -I", incdirs, true);
}


string ToolSpec::getCppFlagArgs() const
{
    return join( " ", cppflags, false);
}


string ToolSpec::getCFlagArgs() const
{
    return join( " ", cflags, false);
}


string ToolSpec::getLinkerFlagArgs() const
{
    return join( " ", lflags, false);
}


string ToolSpec::getLinkExecutableFlagArgs() const
{
    return join( " ", eflags, false);
}


string ToolSpec::getLibArgs() const
{
    return join( " -l", libs, true);
}


string ToolSpec::getLibDirArgs() const
{
    return join( " -L", libDirs, true);
}


bool ToolSpec::parseToolSpec( SimpleXMLStream *xmls )
{
    while( !xmls->atEnd() )
    {
        xmls->readNext();
        
        if( xmls->hasError() )
            break;
        
        if( xmls->isStartElement() )
        {
            if( xmls->path() == "/tool/incdir" )
            {
                SimpleXMLAttributes attr = xmls->attributes();
                
                if( attr.hasAttribute( "value" ) )
                    addIncDir( PlatformDefines::getThePlatformDefines()->replace( attr.value( "value" ) ) );
            }
            else if( xmls->path() == "/tool/cppflags" )
            {
                SimpleXMLAttributes attr = xmls->attributes();
                
                if( attr.hasAttribute( "value" ) )
                    addCppFlag( PlatformDefines::getThePlatformDefines()->replace( attr.value( "value" ) ) );
            }
            else if( xmls->path() == "/tool/cflags" )
            {
                SimpleXMLAttributes attr = xmls->attributes();
                
                if( attr.hasAttribute( "value" ) )
                    addCFlag( PlatformDefines::getThePlatformDefines()->replace( attr.value( "value" ) ) );
            }
            else if( xmls->path() == "/tool/lflags" )
            {
                SimpleXMLAttributes attr = xmls->attributes();
                
                if( attr.hasAttribute( "value" ) )
                    addLFlag( PlatformDefines::getThePlatformDefines()->replace( attr.value( "value" ) ) );
            }
            else if( xmls->path() == "/tool/eflags" )
            {
                SimpleXMLAttributes attr = xmls->attributes();
                
                if( attr.hasAttribute( "value" ) )
                    addLEFlag( PlatformDefines::getThePlatformDefines()->replace( attr.value( "value" ) ) );
            }
            else if( xmls->path() == "/tool/lib" )
            {
                SimpleXMLAttributes attr = xmls->attributes();
                
                if( attr.hasAttribute( "value" ) )
                    addLib( PlatformDefines::getThePlatformDefines()->replace( attr.value( "value" ) ) );
            }
            else if( xmls->path() == "/tool/libdir" )
            {
                SimpleXMLAttributes attr = xmls->attributes();
                
                if( attr.hasAttribute( "value" ) )
                    addLibDir( PlatformDefines::getThePlatformDefines()->replace( attr.value( "value" ) ) );
            }
            else if( xmls->path() == "/tool/sync" )
            {
                SimpleXMLAttributes attr = xmls->attributes();
                
                if( attr.hasAttribute( "use" ) )
                    sync_use = PlatformDefines::getThePlatformDefines()->replace( attr.value( "use" ) );
                if( attr.hasAttribute( "source" ) )
                    sync_source = PlatformDefines::getThePlatformDefines()->replace( attr.value( "source" ) );
                if( attr.hasAttribute( "target" ) )
                    sync_target = PlatformDefines::getThePlatformDefines()->replace( attr.value( "target" ) );
            }
        }
        else if( xmls->isEndElement() )
        {
            if( xmls->path() == "" && xmls->name() == "tool" )   // closing tool
                break;
        }
    }

    return !xmls->hasError();
}

// -----------------------------------------------------------------------------
void CompileMode::addCppFlag( const std::string &flag )
{
    cppflags.push_back( flag );
}


void CompileMode::addCFlag( const std::string &flag )
{
    cflags.push_back( flag );
}


void CompileMode::addLFlag( const std::string &flag )
{
    lflags.push_back( flag );
}


void CompileMode::addLEFlag( const std::string &flag )
{
    eflags.push_back( flag );
}


string CompileMode::getCppFlagArgs() const
{
    return join( " ", cppflags, false);
}


string CompileMode::getCFlagArgs() const
{
    return join( " ", cflags, false);
}


string CompileMode::getLinkerFlagArgs() const
{
    return join( " ", lflags, false);
}


string CompileMode::getLinkExecutableFlagArgs() const
{
    return join( " ", eflags, false);
}

// -----------------------------------------------------------------------------
void CompileTrait::addCppFlag( const string &flag )
{
    cppflags.push_back( flag );
}


void CompileTrait::addCFlag( const string &flag )
{
    cflags.push_back( flag );
}


void CompileTrait::addLFlag( const string &flag )
{
    lflags.push_back( flag );
}


void CompileTrait::addLEFlag( const string &flag )
{
    eflags.push_back( flag );
}


string CompileTrait::getCppFlagArgs() const
{
    return join( " ", cppflags, false);
}


string CompileTrait::getCFlagArgs() const
{
    return join( " ", cflags, false);
}


string CompileTrait::getLinkerFlagArgs() const
{
    return join( " ", lflags, false);
}


string CompileTrait::getLinkExecutableFlagArgs() const
{
    return join( " ", eflags, false);
}

// -----------------------------------------------------------------------------
PlatformSpec *PlatformSpec::thePlatformSpec = 0;

PlatformSpec *PlatformSpec::getThePlatformSpec()
{
    if( thePlatformSpec == 0 )
        thePlatformSpec = new PlatformSpec;

    return thePlatformSpec;
}


bool PlatformSpec::read( const string &fn )
{
    FILE *fpXml = fopen( fn.c_str(), "r");
    if( fpXml == 0 )
    {
        cerr << "error: xml platform specification file " << fn << " not found\n";
        return false;
    }
    
    SimpleXMLStream *xmls = new SimpleXMLStream( fpXml );
    CompileMode currCm;
    CompileTrait currCt;
    
    while( !xmls->atEnd() )
    {
        xmls->readNext();
        
        if( xmls->hasError() )
            break;
        
        if( xmls->isStartElement() )
        {
            //cout << "  -- START Line: " << xmls->LineNumber() << ":";
            
            //cout << " START  Current path: >" << xmls->path()  << "< ";
            if( xmls->path() == "/platform/compiler_version" )
            {
                SimpleXMLAttributes attr = xmls->attributes();
                    
                if( attr.hasAttribute( "value" ) )
                    compilerVersion = attr.value( "value" );
            }
            else if( xmls->path() == "/platform/compile_mode" )
            {
                SimpleXMLAttributes attr = xmls->attributes();
                    
                if( attr.hasAttribute( "value" ) )
                    currCm = CompileMode( attr.value( "value" ) );
            }
            else if( xmls->path() == "/platform/compile_mode/cppflags" )
            {
                SimpleXMLAttributes attr = xmls->attributes();
                
                if( attr.hasAttribute( "value" ) )
                    currCm.addCppFlag( PlatformDefines::getThePlatformDefines()->replace( attr.value( "value" ) ) );
            }
            else if( xmls->path() == "/platform/compile_mode/cflags" )
            {
                SimpleXMLAttributes attr = xmls->attributes();
                
                if( attr.hasAttribute( "value" ) )
                    currCm.addCFlag( PlatformDefines::getThePlatformDefines()->replace(  attr.value( "value" ) ) );
            }
            else if( xmls->path() == "/platform/compile_mode/lflags" )
            {
                SimpleXMLAttributes attr = xmls->attributes();
                
                if( attr.hasAttribute( "value" ) )
                    currCm.addLFlag( PlatformDefines::getThePlatformDefines()->replace( attr.value( "value" ) ) );
            }
            else if( xmls->path() == "/platform/compile_mode/eflags" )
            {
                SimpleXMLAttributes attr = xmls->attributes();
                
                if( attr.hasAttribute( "value" ) )
                    currCm.addLEFlag( PlatformDefines::getThePlatformDefines()->replace( attr.value( "value" ) ) );
            }
            else if( xmls->path() == "/platform/compile_trait" )
            {
                SimpleXMLAttributes attr = xmls->attributes();
                
                if( attr.hasAttribute( "type" ) )
                    currCt = CompileTrait( attr.value( "type" ) );
            }
            else if( xmls->path() == "/platform/compile_trait/cppflags" )
            {
                SimpleXMLAttributes attr = xmls->attributes();
                
                if( attr.hasAttribute( "value" ) )
                    currCt.addCppFlag( PlatformDefines::getThePlatformDefines()->replace( attr.value( "value" ) ) );
            }
            else if( xmls->path() == "/platform/compile_trait/cflags" )
            {
                SimpleXMLAttributes attr = xmls->attributes();
                
                if( attr.hasAttribute( "value" ) )
                    currCt.addCFlag( PlatformDefines::getThePlatformDefines()->replace(  attr.value( "value" ) ) );
            }
            else if( xmls->path() == "/platform/compile_trait/lflags" )
            {
                SimpleXMLAttributes attr = xmls->attributes();
                
                if( attr.hasAttribute( "value" ) )
                    currCt.addLFlag( PlatformDefines::getThePlatformDefines()->replace( attr.value( "value" ) ) );
            }
            else if( xmls->path() == "/platform/compile_trait/eflags" )
            {
                SimpleXMLAttributes attr = xmls->attributes();
                
                if( attr.hasAttribute( "value" ) )
                    currCt.addLEFlag( PlatformDefines::getThePlatformDefines()->replace( attr.value( "value" ) ) );
            }
            else if( xmls->path() == "/platform/tool" )
            {
                SimpleXMLAttributes attr = xmls->attributes();
                    
                if( attr.hasAttribute( "name" ) )
                {
                    ToolSpec currTool( attr.value( "name" ) );
                    
                    xmls->setPathStart( 1 );
                    currTool.parseToolSpec( xmls );
                    xmls->setPathStart( 0 );

                    addTool( currTool );
                }
            }
            else if( xmls->path() == "/platform/compiler" )
            {
                SimpleXMLAttributes attr = xmls->attributes();
                
                if( attr.hasAttribute( "value" ) )
                    compiler = PlatformDefines::getThePlatformDefines()->replace( attr.value( "value" ) );  // c++ compiler
            }
            if( xmls->path() == "/platform/compilerc" )
            {
                SimpleXMLAttributes attr = xmls->attributes();
                
                if( attr.hasAttribute( "value" ) )
                    compilerc = PlatformDefines::getThePlatformDefines()->replace( attr.value( "value" ) );  // c compiler
            }
            else if( xmls->path() == "/platform/incdir" )
            {
                SimpleXMLAttributes attr = xmls->attributes();
                
                if( attr.hasAttribute( "value" ) )
                    incdirs.push_back( PlatformDefines::getThePlatformDefines()->replace( attr.value( "value" ) ) );
            }
            else if( xmls->path() == "/platform/cppflags" )
            {
                SimpleXMLAttributes attr = xmls->attributes();
                
                if( attr.hasAttribute( "value" ) )
                    cppflags.push_back( PlatformDefines::getThePlatformDefines()->replace( attr.value( "value" ) ) );
            }
            else if( xmls->path() == "/platform/cflags" )
            {
                SimpleXMLAttributes attr = xmls->attributes();
                
                if( attr.hasAttribute( "value" ) )
                    cflags.push_back( PlatformDefines::getThePlatformDefines()->replace( attr.value( "value" ) ) );
            }
            else if( xmls->path() == "/platform/soext" )
            {
                SimpleXMLAttributes attr = xmls->attributes();
                
                if( attr.hasAttribute( "value" ) )
                    soext = attr.value( "value" );
            }
            else if( xmls->path() == "/platform/staticext" )
            {
                SimpleXMLAttributes attr = xmls->attributes();
                
                if( attr.hasAttribute( "value" ) )
                    staticext = attr.value( "value" );
            }
            else if( xmls->path() == "/platform/lflags" )
            {
                SimpleXMLAttributes attr = xmls->attributes();
                
                if( attr.hasAttribute( "value" ) )
                    lflags.push_back( PlatformDefines::getThePlatformDefines()->replace( attr.value( "value" ) ) );
            }
            else if( xmls->path() == "/platform/eflags" )
            {
                SimpleXMLAttributes attr = xmls->attributes();
                
                if( attr.hasAttribute( "value" ) )
                    eflags.push_back( PlatformDefines::getThePlatformDefines()->replace( attr.value( "value" ) ) );
            }
            else if( xmls->path() == "/platform/lib" )
            {
                SimpleXMLAttributes attr = xmls->attributes();
                
                if( attr.hasAttribute( "value" ) )
                    libs.push_back( attr.value( "value" ) );
            }
            else if( xmls->path() == "/platform/libdir" )
            {
                SimpleXMLAttributes attr = xmls->attributes();
                
                if( attr.hasAttribute( "value" ) )
                    libDirs.push_back( PlatformDefines::getThePlatformDefines()->replace( attr.value( "value" ) ) );
            }
            else if( xmls->path() == "/platform/define" )
            {
                SimpleXMLAttributes attr = xmls->attributes();

                if( attr.hasAttribute( "name" ) )
                {
                    string name = attr.value( "name" );

                    if( name.length() > 0 )
                    {
                        string value;
                        if( attr.hasAttribute( "value" ) )
                            value = PlatformDefines::getThePlatformDefines()->replace( attr.value( "value" ) );
                        
                        PlatformDefines::getThePlatformDefines()->set( name, value);
                    }
                }
            }
            else if( xmls->path() == "/platform/extension" )
            {
                xmls->setPathStart( 1 );
                ExtensionManager::getTheExtensionManager()->parseExtension( xmls );
                xmls->setPathStart( 0 );
            }
        }
        else if( xmls->isEndElement() )
        {
            if( xmls->path() == "/platform" && xmls->name() == "compile_mode" )
            {
                addCompileMode( currCm );
            }
            else if( xmls->path() == "/platform" && xmls->name() == "compile_trait" )
            {
                addCompileTrait( currCt );
            }
            else if( xmls->path() == "" && xmls->name() == "platform" )
                break;
        }
    }

    bool ok = !xmls->hasError();
    
    delete xmls;
    fclose( fpXml );

    return ok;
}


void PlatformSpec::addCompileMode( CompileMode cm )
{
    compileModes.push_back( cm );
}

void PlatformSpec::addCompileTrait( CompileTrait ct )
{
    compileTraits.push_back( ct );
}

void PlatformSpec::addTool( ToolSpec t )
{
    tools.push_back( t );
}


bool PlatformSpec::hasCompileMode( const std::string &mode )
{
    size_t i;
    
    for( i = 0; i < compileModes.size(); i++)
        if( mode == compileModes[i].getMode() )
            return true;
    
    return false;
}


CompileMode PlatformSpec::getCompileMode( const string &mode )
{
    size_t i;
    
    for( i = 0; i < compileModes.size(); i++)
        if( mode == compileModes[i].getMode() )
            return compileModes[i];
    
    return CompileMode( "INVALID" );
}


bool PlatformSpec::hasCompileTrait( const string &type )
{
    size_t i;
    
    for( i = 0; i < compileTraits.size(); i++)
        if( type == compileTraits[i].getType() )
            return true;
    
    return false;
}


CompileTrait PlatformSpec::getCompileTrait( const string &type )
{
    size_t i;
    
    for( i = 0; i < compileTraits.size(); i++)
        if( type == compileTraits[i].getType() )
            return compileTraits[i];
    
    return CompileTrait( "INVALID" );
}


ToolSpec PlatformSpec::getTool( const string &name )
{
    size_t i;

    for( i = 0; i < tools.size(); i++)
        if( name == tools[i].getName() )
            return tools[i];

    return ToolSpec( "INVALID" );
}


string PlatformSpec::getCompilerVersion() const
{
    return compilerVersion;
}


string PlatformSpec::getCppCompiler() const
{
    return compiler;
}


string PlatformSpec::getCCompiler() const
{
    return compilerc;
}


string PlatformSpec::getIncCommandArgs() const
{
    return join( " -I", incdirs, true);
}


string PlatformSpec::getCppFlagArgs() const
{
    return join( " ", cppflags, false);
}


string PlatformSpec::getCFlagArgs() const
{
    return join( " ", cflags, false);
}


string PlatformSpec::getLinkerFlagArgs() const
{
    return join( " ", lflags, false);
}


string PlatformSpec::getLinkExecutableFlagArgs() const
{
    return join( " ", eflags, false);
}


string PlatformSpec::getLibArgs() const
{
    return join( " -l", libs, true);
}


string PlatformSpec::getLibDirArgs() const
{
    return join( " -L", libDirs, true);
}


void PlatformSpec::syncTools( Executor &executor, bool printTimes)
{
    if( tools.size() == 0 )
        return;
    
    size_t i;
    SyncEngine se;
    
    for( i = 0; i < tools.size(); i++)
    {
        const ToolSpec &ts = tools[i];

        if( ts.getSyncUse().length() > 0 && ts.getSyncSource().length() > 0 && ts.getSyncTarget().length() > 0 )
        {
            string script_fn = buildDir + "/ferret_sync_" + ts.getSyncUse() + ".sh";
            vector<string> args;
            args.push_back( ts.getSyncSource() );
            args.push_back( ts.getSyncTarget() );
            
            se.addSyncCommand( ExecutorCommand( script_fn, args) );
        }
    }

    if( se.hasCommands() )
    {
        if( verbosity > 0 )
            cout << "platform tools sync start\n";
        
        se.doWork( executor, printTimes);
        OutputCollector::getTheOutputCollector()->clear();
        
        if( verbosity > 0 )
            cout << "platform tools sync end\n";
    }
}
