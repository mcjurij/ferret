#include <cstdio>
#include <iostream>
#include <sstream>

#include "platform_spec.h"
#include "simple_xml_stream.h"
#include "glob_utility.h"
#include "platform_defines.h"
#include "extension.h"

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
    ToolSpec currTool;
    CompileMode currCm;
    CompileTrait currCt;
    
    while( !xmls->AtEnd() )
    {
        xmls->ReadNext();
        
        if( xmls->HasError() )
            break;
        
        if( xmls->IsStartElement() )
        {
            //cout << "  -- START Line: " << xmls->LineNumber() << ":";
            
            //cout << " START  Current path: >" << xmls->Path()  << "< ";
            if( xmls->Path() == "/platform/compiler_version" )
            {
                SimpleXMLAttributes attr = xmls->Attributes();
                    
                if( attr.HasAttribute( "value" ) )
                    compilerVersion = attr.Value( "value" );
            }
            else if( xmls->Path() == "/platform/compile_mode" )
            {
                SimpleXMLAttributes attr = xmls->Attributes();
                    
                if( attr.HasAttribute( "value" ) )
                    currCm = CompileMode( attr.Value( "value" ) );
            }
            else if( xmls->Path() == "/platform/compile_mode/cppflags" )
            {
                SimpleXMLAttributes attr = xmls->Attributes();
                
                if( attr.HasAttribute( "value" ) )
                    currCm.addCppFlag( PlatformDefines::getThePlatformDefines()->replace( attr.Value( "value" ) ) );
            }
            else if( xmls->Path() == "/platform/compile_mode/cflags" )
            {
                SimpleXMLAttributes attr = xmls->Attributes();
                
                if( attr.HasAttribute( "value" ) )
                    currCm.addCFlag( PlatformDefines::getThePlatformDefines()->replace(  attr.Value( "value" ) ) );
            }
            else if( xmls->Path() == "/platform/compile_mode/lflags" )
            {
                SimpleXMLAttributes attr = xmls->Attributes();
                
                if( attr.HasAttribute( "value" ) )
                    currCm.addLFlag( PlatformDefines::getThePlatformDefines()->replace( attr.Value( "value" ) ) );
            }
            else if( xmls->Path() == "/platform/compile_mode/eflags" )
            {
                SimpleXMLAttributes attr = xmls->Attributes();
                
                if( attr.HasAttribute( "value" ) )
                    currCm.addLEFlag( PlatformDefines::getThePlatformDefines()->replace( attr.Value( "value" ) ) );
            }
            else if( xmls->Path() == "/platform/compile_trait" )
            {
                SimpleXMLAttributes attr = xmls->Attributes();
                
                if( attr.HasAttribute( "type" ) )
                    currCt = CompileTrait( attr.Value( "type" ) );
            }
            else if( xmls->Path() == "/platform/compile_trait/cppflags" )
            {
                SimpleXMLAttributes attr = xmls->Attributes();
                
                if( attr.HasAttribute( "value" ) )
                    currCt.addCppFlag( PlatformDefines::getThePlatformDefines()->replace( attr.Value( "value" ) ) );
            }
            else if( xmls->Path() == "/platform/compile_trait/cflags" )
            {
                SimpleXMLAttributes attr = xmls->Attributes();
                
                if( attr.HasAttribute( "value" ) )
                    currCt.addCFlag( PlatformDefines::getThePlatformDefines()->replace(  attr.Value( "value" ) ) );
            }
            else if( xmls->Path() == "/platform/compile_trait/lflags" )
            {
                SimpleXMLAttributes attr = xmls->Attributes();
                
                if( attr.HasAttribute( "value" ) )
                    currCt.addLFlag( PlatformDefines::getThePlatformDefines()->replace( attr.Value( "value" ) ) );
            }
            else if( xmls->Path() == "/platform/compile_trait/eflags" )
            {
                SimpleXMLAttributes attr = xmls->Attributes();
                
                if( attr.HasAttribute( "value" ) )
                    currCt.addLEFlag( PlatformDefines::getThePlatformDefines()->replace( attr.Value( "value" ) ) );
            }
            else if( xmls->Path() == "/platform/tool" )
            {
                SimpleXMLAttributes attr = xmls->Attributes();
                    
                if( attr.HasAttribute( "name" ) )
                    currTool = ToolSpec( attr.Value( "name" ) );
            }
            else if( xmls->Path() == "/platform/tool/incdir" )
            {
                SimpleXMLAttributes attr = xmls->Attributes();
                
                if( attr.HasAttribute( "value" ) )
                    currTool.addIncDir( PlatformDefines::getThePlatformDefines()->replace( attr.Value( "value" ) ) );
            }
            else if( xmls->Path() == "/platform/tool/cppflags" )
            {
                SimpleXMLAttributes attr = xmls->Attributes();
                
                if( attr.HasAttribute( "value" ) )
                    currTool.addCppFlag( PlatformDefines::getThePlatformDefines()->replace( attr.Value( "value" ) ) );
            }
            else if( xmls->Path() == "/platform/tool/cflags" )
            {
                SimpleXMLAttributes attr = xmls->Attributes();
                
                if( attr.HasAttribute( "value" ) )
                    currTool.addCFlag( PlatformDefines::getThePlatformDefines()->replace( attr.Value( "value" ) ) );
            }
            else if( xmls->Path() == "/platform/tool/lflags" )
            {
                SimpleXMLAttributes attr = xmls->Attributes();
                
                if( attr.HasAttribute( "value" ) )
                    currTool.addLFlag( PlatformDefines::getThePlatformDefines()->replace( attr.Value( "value" ) ) );
            }
            else if( xmls->Path() == "/platform/tool/eflags" )
            {
                SimpleXMLAttributes attr = xmls->Attributes();
                
                if( attr.HasAttribute( "value" ) )
                    currTool.addLEFlag( PlatformDefines::getThePlatformDefines()->replace( attr.Value( "value" ) ) );
            }
            else if( xmls->Path() == "/platform/tool/lib" )
            {
                SimpleXMLAttributes attr = xmls->Attributes();
                
                if( attr.HasAttribute( "value" ) )
                    currTool.addLib( PlatformDefines::getThePlatformDefines()->replace( attr.Value( "value" ) ) );
            }
            else if( xmls->Path() == "/platform/tool/libdir" )
            {
                SimpleXMLAttributes attr = xmls->Attributes();
                
                if( attr.HasAttribute( "value" ) )
                    currTool.addLibDir( PlatformDefines::getThePlatformDefines()->replace( attr.Value( "value" ) ) );
            }
            else if( xmls->Path() == "/platform/compiler" )
            {
                SimpleXMLAttributes attr = xmls->Attributes();
                
                if( attr.HasAttribute( "value" ) )
                    compiler = PlatformDefines::getThePlatformDefines()->replace( attr.Value( "value" ) );  // c++ compiler
            }
            if( xmls->Path() == "/platform/compilerc" )
            {
                SimpleXMLAttributes attr = xmls->Attributes();
                
                if( attr.HasAttribute( "value" ) )
                    compilerc = PlatformDefines::getThePlatformDefines()->replace( attr.Value( "value" ) );  // c compiler
            }
            else if( xmls->Path() == "/platform/incdir" )
            {
                SimpleXMLAttributes attr = xmls->Attributes();
                
                if( attr.HasAttribute( "value" ) )
                    incdirs.push_back( PlatformDefines::getThePlatformDefines()->replace( attr.Value( "value" ) ) );
            }
            else if( xmls->Path() == "/platform/cppflags" )
            {
                SimpleXMLAttributes attr = xmls->Attributes();
                
                if( attr.HasAttribute( "value" ) )
                    cppflags.push_back( PlatformDefines::getThePlatformDefines()->replace( attr.Value( "value" ) ) );
            }
            else if( xmls->Path() == "/platform/cflags" )
            {
                SimpleXMLAttributes attr = xmls->Attributes();
                
                if( attr.HasAttribute( "value" ) )
                    cflags.push_back( PlatformDefines::getThePlatformDefines()->replace( attr.Value( "value" ) ) );
            }
            else if( xmls->Path() == "/platform/soext" )
            {
                SimpleXMLAttributes attr = xmls->Attributes();
                
                if( attr.HasAttribute( "value" ) )
                    soext = attr.Value( "value" );
            }
            else if( xmls->Path() == "/platform/staticext" )
            {
                SimpleXMLAttributes attr = xmls->Attributes();
                
                if( attr.HasAttribute( "value" ) )
                    staticext = attr.Value( "value" );
            }
            else if( xmls->Path() == "/platform/lflags" )
            {
                SimpleXMLAttributes attr = xmls->Attributes();
                
                if( attr.HasAttribute( "value" ) )
                    lflags.push_back( PlatformDefines::getThePlatformDefines()->replace( attr.Value( "value" ) ) );
            }
            else if( xmls->Path() == "/platform/eflags" )
            {
                SimpleXMLAttributes attr = xmls->Attributes();
                
                if( attr.HasAttribute( "value" ) )
                    eflags.push_back( PlatformDefines::getThePlatformDefines()->replace( attr.Value( "value" ) ) );
            }
            else if( xmls->Path() == "/platform/lib" )
            {
                SimpleXMLAttributes attr = xmls->Attributes();
                
                if( attr.HasAttribute( "value" ) )
                    libs.push_back( attr.Value( "value" ) );
            }
            else if( xmls->Path() == "/platform/libdir" )
            {
                SimpleXMLAttributes attr = xmls->Attributes();
                
                if( attr.HasAttribute( "value" ) )
                    libDirs.push_back( PlatformDefines::getThePlatformDefines()->replace( attr.Value( "value" ) ) );
            }
            else if( xmls->Path() == "/platform/define" )
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
            else if( xmls->Path() == "/platform/extension" )
            {
                xmls->setPathStart( 1 );
                ExtensionManager::getTheExtensionManager()->parseExtension( xmls );
                xmls->setPathStart( 0 );
            }
            
            //cout << "\n";
        }
        else if( xmls->IsEndElement() )
        {  //cout << "  -- END   tag: " << xmls->Name() << " ";
           // cout << "Line: " << xmls->LineNumber() << ":";
           // cout << "  Current path: >" << xmls->Path()  << "< ";
            
            if( xmls->Path() == "/platform" && xmls->Name() == "tool" )
            {
                addTool( currTool );
            }
            else if( xmls->Path() == "/platform" && xmls->Name() == "compile_mode" )
            {
                addCompileMode( currCm );
            }
            else if( xmls->Path() == "/platform" && xmls->Name() == "compile_trait" )
            {
                addCompileTrait( currCt );
            }
            else if( xmls->Path() == "" && xmls->Name() == "platform" )
                break;
        }
    }

    bool err = !xmls->HasError();
    
    delete xmls;
    fclose( fpXml );

    return err;
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
