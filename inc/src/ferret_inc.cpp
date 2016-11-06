// parse a source file to find all direct dependencies
// g++ -ansi -Wall -o ferret_dep ferret_dep.cpp

#include <cstdlib>
#include <cstdio>
#include <cstring>
#include <cassert>
#include <string>
#include <iostream>
#include <sstream>
#include <cstdio>
#include <vector>

using namespace std;


class ParseIncludes
{
public:	
	enum TokenType {
		INVALID = 0, ERROR,
        HASH, QUOTE, LESS_THAN, GREATER_THAN,
        KW_INCLUDE, FILE_NAME,
        NEWLINE,
        STRING,
        COMMENT,
        COMMENT_CPP
	};
	
	ParseIncludes( FILE *in );
	~ParseIncludes();

private:
    void scanFileName();

	ParseIncludes::TokenType ReadNext();

	int LineNumber() const
	{ return line_num; }
    
	bool AtEnd() const
	{ return ateof; }
    
    void scanIncludeKw();
    
    void parseIncludes();

public:
    bool parse();
    bool hasError() const;
    
private:
	void  consume();
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
    bool after_newline, preproc_mode;

    TokenType current_token;
    string current_value;
};


static int wraplen=0;
static void printWrapped( const string &fn )
{
    if( wraplen + fn.length() <= 80 )
    {
        cout << " " << fn;
        wraplen += fn.length()+1;
    }
    else
    {
        cout << " \\\n" << fn;
        wraplen = fn.length();
    }
}


/* intentionally not using ctype.h here -- locale sucks */
#define is_digit(c) ((c)>='0' && (c)<='9')
#define is_alpha(c) (((c)>='a' && (c)<='z') || ((c)>='A' && (c)<='Z'))
#define is_special(c) ((c)=='_' || (c)=='-' || (c)==':' || (c)=='/' || (c)=='.')
#define is_white(c) ((c)==' ' || (c)=='\t')

#define is_path_begin(c) (is_digit(c)||is_alpha(c)||is_special(c))
#define is_inc_char(c) ((c)>='c' && (c)<='n')

ParseIncludes::ParseIncludes( FILE *in )
	: in_fp(in), ateof(false),line_num(1),
	  curr(0),errors(0),
	  current_token(INVALID)
{
	assert(in);
    
	consume();
    
    after_newline = true;   // first line is like new line
    preproc_mode = false;   // not in "preprocessor" mode
}


ParseIncludes::~ParseIncludes()
{

}


void ParseIncludes::consume()
{
    int c = fgetc( in_fp );
	if( c>=0 )
	{
        input_buf[0] = (char)c;
        input_buf[1] = '\0';
        input_length = 1;

        if( input_buf[0] == '\n' )
            line_num++;
	}
	else
	{
		ateof=true;
		curr=input_length = 0;
	}
}


#define  get_char()  ((curr < input_length)?input_buf[ curr ]:'$')

bool ParseIncludes::HasNextChar()
{
	if( curr == input_length )
		consume();
	return curr < input_length;
}


void ParseIncludes::SkipWhite()
{
	while( HasNextChar() && is_white(get_char()) )
		consume();
}


void ParseIncludes::scanIncludeKw()
{
    if( get_char() == 'i' )
    {
        int i;
        const char *inckw = "include";
        int len = 7;  // = strlen( inckw );
        
        for( i = 0; i<len; i++)
            if( HasNextChar() && inckw[i] == get_char() )
                consume();
            else
                break;
        if( i==len )
            current_token = KW_INCLUDE;
    }
}


void ParseIncludes::scanFileName()
{
	char buf[1024];
    int i=0;

    while( HasNextChar() && get_char() != '>' && get_char() != '"' && get_char() != '\n' )
    {
        if( get_char() == '\\' )
        {
            consume();
            if( i<1022 )
            {
                buf[i++] = '\\';              // gcc uses backspaces to escape a space
                buf[i++] = get_char();
            }
            consume();
        }
        else
        {
            if( i<1023 )
                buf[i++] = get_char();
            consume();
        }
    }
    buf[i]=0;
    current_value = buf;
    current_token = FILE_NAME;

    printWrapped( current_value );
}


void ParseIncludes::ParseNext()
{
    current_value = "";
    SkipWhite();
    
    if( ateof )
    {
        current_token = INVALID;
        return;
    }
    else if( get_char() == '#' )
    {
        current_token = HASH;
        consume();     /* consume '#' */
    }
    else if( get_char() == '<' )
    {
        current_token = LESS_THAN;
        consume();     /* consume '<' */
    }
    else if( get_char() == '>' )
    {
        current_token = GREATER_THAN;
        consume();     /* consume '>' */
    }
    else if( preproc_mode && get_char() == '"' )    // opening quote while in line starting with a '#'?
    {
        current_token = QUOTE;
        consume();
    }
    else if( !preproc_mode && get_char() == '"' )    // opening quote?
    {
        current_token = STRING;
        consume();     /* consume " */
        
        // read the entire string to prevent us from are getting confused by its contents
        while( HasNextChar() && get_char() != '"' )
        {
            if( get_char() == '\\' )
                consume();
            consume();
        }
        
        consume(); // consume closing quote
    }  
    else if( get_char() == '\n' )
    {
        current_token = NEWLINE;
        consume();
    }
    else if( get_char() == '/' )
    {
        consume();
        if( get_char() == '*' )           // c style comment
        {
            consume();
            current_token = COMMENT;
            bool done=false;

            do
            {
                while( HasNextChar() && get_char() != '*' )
                    consume();
                if( get_char() == '*' )
                {
                    consume();
                    if( get_char() == '/' )
                    {
                        consume();
                        done = true;
                    }
                }
            }
            while( !done );
        }
        else if( get_char() == '/' )     // c++ style comment
        {
            current_token = COMMENT_CPP;
            consume();
            while( HasNextChar() && get_char() != '\n' )
                consume();
            
            if( get_char() == '\n' )   // return belongs to comment
                consume();
        }
    }
    else
        consume();
}


ParseIncludes::TokenType ParseIncludes::ReadNext()
{
	ParseNext();

	return current_token;
}


void ParseIncludes::parseIncludes()
{
    if( after_newline && current_token == HASH )
    {
        after_newline = false;
        SkipWhite();
        scanIncludeKw();
        preproc_mode = true;
        
        if( current_token == KW_INCLUDE )
        {
            ParseNext();
            
            if( current_token == QUOTE )
            {
                scanFileName();
                ParseNext();
                
                if( current_token != QUOTE )
                {
                    cerr << "line " << line_num << " error: include directive malformed (expected closing \")\n";
                    errors++;
                }
            }
            else if( current_token == LESS_THAN )
            {
                scanFileName();
                ParseNext();
                
                if( current_token != GREATER_THAN )
                {
                    cerr << "line " << line_num << " error: include directive malformed (expected closing >)\n";
                    errors++;
                }
            }
            else
            {
                cerr << "line " << line_num << " error: include directive malformed (or you are using a macro - bad idea).\n";
                errors++;
            }
        }
        preproc_mode = false;
    }
    else if( current_token == NEWLINE || current_token == COMMENT_CPP )
    {
        ParseNext();
        after_newline = true;
        preproc_mode = false;
    }
    else
        ParseNext();  // stuff we are not interested in
}


bool ParseIncludes::parse()
{
    ParseNext();
    do {
        parseIncludes();
    } while( !AtEnd() && !hasError() );
	
	return hasError();
}


bool ParseIncludes::hasError() const
{
    return current_token == ERROR || errors > 0;
}


int main( int argc, char **argv)
{
    if( argc != 2 )
        return 1;
    FILE *fp = fopen( argv[1], "r");
    if( !fp )
        return 2;

    cout << argv[1] << ":";
    wraplen = strlen( argv[1] ) + 1;
    
    ParseIncludes p( fp );
    p.parse();
    cout << "\n";

    if( !p.hasError() )
        return 0;
    else
        return 1;
}
