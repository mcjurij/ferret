#include <cstdio>
#include <iostream>
#include <sstream>

#include "platform_spec.h"
#include "simple_xml_stream.h"
#include "glob_utility.h"

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
                    currCm.addCppFlag( attr.Value( "value" ) );
            }
            else if( xmls->Path() == "/platform/compile_mode/cflags" )
            {
                SimpleXMLAttributes attr = xmls->Attributes();
                
                if( attr.HasAttribute( "value" ) )
                    currCm.addCFlag(  attr.Value( "value" ) );
            }
            else if( xmls->Path() == "/platform/compile_mode/lflags" )
            {
                SimpleXMLAttributes attr = xmls->Attributes();
                
                if( attr.HasAttribute( "value" ) )
                    currCm.addLFlag( attr.Value( "value" ) );
            }
            else if( xmls->Path() == "/platform/compile_mode/eflags" )
            {
                SimpleXMLAttributes attr = xmls->Attributes();
                
                if( attr.HasAttribute( "value" ) )
                    currCm.addLEFlag(  attr.Value( "value" ) );
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
                    currTool.addIncDir( attr.Value( "value" ) );
            }
            else if( xmls->Path() == "/platform/tool/cppflags" )
            {
                SimpleXMLAttributes attr = xmls->Attributes();
                
                if( attr.HasAttribute( "value" ) )
                    currTool.addCppFlag( attr.Value( "value" ) );
            }
            else if( xmls->Path() == "/platform/tool/cflags" )
            {
                SimpleXMLAttributes attr = xmls->Attributes();
                
                if( attr.HasAttribute( "value" ) )
                    currTool.addCFlag( attr.Value( "value" ) );
            }
            else if( xmls->Path() == "/platform/tool/lflags" )
            {
                SimpleXMLAttributes attr = xmls->Attributes();
                
                if( attr.HasAttribute( "value" ) )
                    currTool.addLFlag( attr.Value( "value" ) );
            }
            else if( xmls->Path() == "/platform/tool/eflags" )
            {
                SimpleXMLAttributes attr = xmls->Attributes();
                
                if( attr.HasAttribute( "value" ) )
                    currTool.addLEFlag( attr.Value( "value" ) );
            }
            else if( xmls->Path() == "/platform/tool/lib" )
            {
                SimpleXMLAttributes attr = xmls->Attributes();
                
                if( attr.HasAttribute( "value" ) )
                    currTool.addLib( attr.Value( "value" ) );
            }
            else if( xmls->Path() == "/platform/tool/libdir" )
            {
                SimpleXMLAttributes attr = xmls->Attributes();
                
                if( attr.HasAttribute( "value" ) )
                    currTool.addLibDir( attr.Value( "value" ) );
            }
            else if( xmls->Path() == "/platform/compiler" )
            {
                SimpleXMLAttributes attr = xmls->Attributes();
                
                if( attr.HasAttribute( "value" ) )
                    compiler = attr.Value( "value" );               // c++ compile
            }
            if( xmls->Path() == "/platform/compilerc" )
            {
                SimpleXMLAttributes attr = xmls->Attributes();
                
                if( attr.HasAttribute( "value" ) )
                    compilerc = attr.Value( "value" );              // c compiler
            }
            else if( xmls->Path() == "/platform/incdir" )
            {
                SimpleXMLAttributes attr = xmls->Attributes();
                
                if( attr.HasAttribute( "value" ) )
                    incdirs.push_back( attr.Value( "value" ) );
            }
            else if( xmls->Path() == "/platform/cppflags" )
            {
                SimpleXMLAttributes attr = xmls->Attributes();
                
                if( attr.HasAttribute( "value" ) )
                    cppflags.push_back( attr.Value( "value" ) );
            }
            else if( xmls->Path() == "/platform/cflags" )
            {
                SimpleXMLAttributes attr = xmls->Attributes();
                
                if( attr.HasAttribute( "value" ) )
                    cflags.push_back( attr.Value( "value" ) );
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
                    lflags.push_back( attr.Value( "value" ) );
            }
            else if( xmls->Path() == "/platform/eflags" )
            {
                SimpleXMLAttributes attr = xmls->Attributes();
                
                if( attr.HasAttribute( "value" ) )
                        eflags.push_back( attr.Value( "value" ) );
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
                    libDirs.push_back( attr.Value( "value" ) );
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
            //cout << "\n";
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
