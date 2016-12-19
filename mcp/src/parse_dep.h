// parse a ferret_inc (or gcc) generated dependency file

#ifndef FERRET_PARSE_DEP_H_
#define FERRET_PARSE_DEP_H_

#include <string>
#include <cstdio>
#include <vector>

struct DepFileEntry
{
    std::string target;     // we do not care (since we know the target), but save it for completeness.
    std::vector<std::string> depIncludes;

    DepFileEntry( const std::string &target, const std::vector<std::string>  &depIncludes)
        : target(target), depIncludes(depIncludes)
    {}

    void print() const;
};


class ParseDep
{
public: 
    enum TokenType {
        INVALID = 0,ERROR,
        FILE_NAME, COLON,
        COMMENT,
        LINE_CONTINUATION,
        NEWLINE
    };
    
    ParseDep( FILE *in );
    ~ParseDep();

private:
    void ParseFileName();

    ParseDep::TokenType ReadNext();

    int LineNumber() const
    { return line_num; }

    bool HasError() const;

    bool AtEnd() const
    { return ateof; }
    
    void ParseDependency();

public:
    bool parse();
    
    std::vector<DepFileEntry> getDepEntries() const
    { return entries; }
    
private:
    void  Consume();
    bool  HasNextChar();
    void  SkipWhite();
    void  ParseNext();
    
    FILE *in_fp;
    bool ateof;
    int line_num;
    char input_buf[2];
    int input_length;
    int curr;
    int errors;
    bool saw_bs;

    TokenType current_token;
    std::string current_value;

    std::vector<DepFileEntry> entries;
};

#endif
