#ifndef FERRET_SCRIPT_TEMPLATE_H_
#define FERRET_SCRIPT_TEMPLATE_H_

#include <string>
#include <map>


class ScriptTemplate
{
public:
    ScriptTemplate( const std::string &fn )
        : fn(fn)
    {}

    bool readTemplate();

    void replace( const std::map<std::string,std::string> &replMap);

    std::string write( const std::string &scriptfn, int file_id);

private:
    bool hasNextChar() const
    { return pos < templContent.length(); }
    
    char consume();
    
    char getc() const
    { return templContent[pos]; }
    
private:
    std::string fn;
    std::string templContent;  // we slurp the file in
    std::string newContent;
    size_t pos;
};

#endif
