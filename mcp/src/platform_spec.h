#ifndef FERRET_PLATFORM_SPEC_H_
#define FERRET_PLATFORM_SPEC_H_

#include <string>
#include <vector>

class ToolSpec
{
public:
    ToolSpec()
        : name( "INVALID" )
    {
    }
    
    ToolSpec( const std::string &name )
        : name(name)
    {
    }

    std::string getName() const
    { return name; }
    
    void addIncDir( const std::string &dir )
    { incdirs.push_back( dir ); }    
    void addCppFlag( const std::string &flag );
    void addCFlag( const std::string &flag );
    void addLFlag( const std::string &flag )
    { lflags.push_back( flag ); }
    void addLEFlag( const std::string &flag )
    { eflags.push_back( flag ); }
    void addLib( const std::string &lib )
    { libs.push_back( lib ); }
    void addLibDir( const std::string &dir )
    { libDirs.push_back( dir ); }
    
    std::string getIncCommandArgs() const;
    std::string getCppFlagArgs() const;
    std::string getCFlagArgs() const;
    std::string getLinkerFlagArgs() const;
    std::string getLinkExecutableFlagArgs() const;
    std::string getLibArgs() const;
    std::string getLibDirArgs() const;
    
private:
    std::string name;
    
    std::vector<std::string> incdirs;
    std::vector<std::string> cppflags;         // c++ compiler flags when using this tool
    std::vector<std::string> cflags;           // c compiler flags when using this tool
    std::vector<std::string> lflags;           // link library flags when using this tool
    std::vector<std::string> eflags;           // link executable flags when using this tool
    std::vector<std::string> libs;
    std::vector<std::string> libDirs;
};


class CompileMode
{
public:
    CompileMode()
        : mode( "INVALID" )
    {
    }
    
    CompileMode( const std::string &mode )
        : mode(mode)
    {
    }

    std::string getMode() const
    { return mode; }
    
    void addCppFlag( const std::string &flag );
    void addCFlag( const std::string &flag );
    void addLFlag( const std::string &flag );
    void addLEFlag( const std::string &flag );
    
    std::string getCppFlagArgs() const;
    std::string getCFlagArgs() const;
    std::string getLinkerFlagArgs() const;
    std::string getLinkExecutableFlagArgs() const;
    
private:
    std::string mode;

    std::vector<std::string> cppflags;         // c++ compiler flags for compile mode
    std::vector<std::string> cflags;           // c compiler flags for compile mode
    std::vector<std::string> lflags;           // link library flags for compile mode
    std::vector<std::string> eflags;           // link executable flags for compile mode
};


class CompileTrait
{
public:
    CompileTrait()
        : type( "INVALID" )
    {
    }
    
    CompileTrait( const std::string &type )
        : type(type)
    {
    }

    std::string getType() const   // get the type ("library") this trait is for
    { return type; }
    
    void addCppFlag( const std::string &flag );
    void addCFlag( const std::string &flag );
    void addLFlag( const std::string &flag );
    void addLEFlag( const std::string &flag );
    
    std::string getCppFlagArgs() const;
    std::string getCFlagArgs() const;
    std::string getLinkerFlagArgs() const;
    std::string getLinkExecutableFlagArgs() const;
    
private:
    std::string type;

    std::vector<std::string> cppflags;         // c++ compiler flags for compile mode
    std::vector<std::string> cflags;           // c compiler flags for compile mode
    std::vector<std::string> lflags;           // link library flags for compile mode
    std::vector<std::string> eflags;           // link executable flags for compile mode
};


class PlatformSpec
{
    
public:
    static PlatformSpec *getThePlatformSpec();
    
    PlatformSpec()
    {}

    bool read( const std::string &fn );

    void addCompileMode( CompileMode cm );
    void addCompileTrait( CompileTrait ct );
    void addTool( ToolSpec t );
    bool hasCompileMode( const std::string &mode );
    CompileMode getCompileMode( const std::string &mode );
    bool hasCompileTrait( const std::string &type );
    CompileTrait getCompileTrait( const std::string &type );
    ToolSpec getTool( const std::string &name );

    std::string getCompilerVersion() const;
    std::string getCppCompiler() const;
    std::string getCCompiler() const;
    std::string getIncCommandArgs() const;
    std::string getCppFlagArgs() const;
    std::string getCFlagArgs() const;
    std::string getLinkerFlagArgs() const;
    std::string getLinkExecutableFlagArgs() const;
    std::string getLibArgs() const;
    std::string getLibDirArgs() const;
    
    std::string getSoFileName( const std::string &lib ) const
    { return "lib" + lib + soext; }
    
private:
    static PlatformSpec *thePlatformSpec;

    std::string compilerVersion;
    std::string compiler;                      // c++ compiler to use
    std::string compilerc;                     // c compiler to use
    std::vector<std::string> incdirs;
    std::vector<std::string> cppflags;         // c++ compiler flags
    std::vector<std::string> cflags;           // c compiler flags
    std::string soext;
    std::string staticext;
    std::vector<std::string> lflags;
    std::vector<std::string> eflags;
    std::vector<std::string> libs;
    std::vector<std::string> libDirs;
    
    std::vector<CompileMode>  compileModes;     // DEBUG, RELEASE...
    std::vector<CompileTrait> compileTraits;    // library
    std::vector<ToolSpec>     tools;
    
};

#endif
