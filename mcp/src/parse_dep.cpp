// parse a gcc generated dependency file
//
// g++ -ansi -Wall -c parse_dep.cpp

#include <cstdlib>
#include <cstdio>
#include <cstring>
#include <cassert>

#include <string>
#include <iostream>
#include <sstream>

#include "parse_dep.h"

using std::string;
using std::vector;
using std::cout;
using std::cerr;

void DepFileEntry::print() const
{
    cout << "   " << target << ":";
    size_t i;
    for( i = 0; i < depIncludes.size(); i++)
        cout << " " << depIncludes[i];
    cout << "\n";
}

// #define DEBUG 1

/* intentionally not using ctype.h here -- locale sucks */
#define is_digit(c) ((c)>='0' && (c)<='9')
#define is_alpha(c) (((c)>='a' && (c)<='z') || ((c)>='A' && (c)<='Z'))
#define is_special(c) ((c)=='_' || (c)=='-' || (c)==':' || (c)=='/' || (c)=='.' )
#define is_white(c) ((c)==' ' || (c)=='\t')

#define is_path_begin(c) (is_digit(c)||is_alpha(c)||is_special(c))


ParseDep::ParseDep( FILE *in )
	: in_fp(in), ateof(false),line_num(0),
	  curr(0),errors(0),
	  current_token(INVALID)
{
	assert(in);
    
	Consume();
	line_num = 1;

    saw_bs = false;
}


ParseDep::~ParseDep()
{

}


void ParseDep::Consume()
{
    int c = fgetc( in_fp );
	if( c>=0 )
	{
        input_buf[0] = (char)c;
        input_buf[1] = '\0';
        input_length = 1;
	}
	else
	{
		ateof=true;
		curr=input_length = 0;
	}
}


#define  get_char()  ((curr < input_length)?input_buf[ curr ]:'$')

bool ParseDep::HasNextChar()
{
	if( curr == input_length )
		Consume();
	return curr < input_length;
}


void ParseDep::SkipWhite()
{
	while( HasNextChar() && is_white(get_char()) )
		Consume();
}


void ParseDep::ParseFileName()
{
	char buf[1024];
    int i=0;
    assert( is_path_begin( get_char() ) );
    
    while( HasNextChar() && get_char() != ' ' && get_char() != ':' )
    {
        if( get_char() == '\\' )   // gcc escapes spaces in file names using a backslash
        {
            saw_bs = true;
            Consume();
        }

        if( get_char() == '\n' )
            break;
        else
            saw_bs = false;
        
        buf[i++] = get_char();
        Consume();
    }
    buf[i]=0;
    
    current_value = buf;
    current_token = FILE_NAME;
}


void ParseDep::ParseNext()
{
    current_value = "";
    SkipWhite();
    
    if( ateof )
    {
        current_token = INVALID;
        return;
    }
    else if( get_char() != ':' && is_path_begin(get_char()) )
        ParseFileName();
    else if( get_char() == ':' )
    {
        current_token = COLON;
        Consume();     /* consume ':' */
    }
    else if( get_char() == '\\' )
    {        
        current_token = LINE_CONTINUATION;
        Consume();     // consume backslash
        saw_bs = true;
    }
    else if( get_char() == '\n' )
    {
        line_num++;
        if( saw_bs )
        {
            saw_bs = false;
            current_token = LINE_CONTINUATION;
        }
        else
            current_token = NEWLINE;
        Consume();
    }
}


ParseDep::TokenType ParseDep::ReadNext()
{
	ParseNext();

	return current_token;
}


bool ParseDep::HasError() const
{
    return current_token == ERROR || errors > 0;
}


void ParseDep::ParseDependency()
{
    string t;
    vector<string> depIncs;
    
    if( current_token == FILE_NAME )
    {
        t = current_value;
        ParseNext();

        if( current_token == COLON )
        {
            ParseNext();
            
            while( current_token == FILE_NAME || current_token == LINE_CONTINUATION )
            {
                if( current_token == FILE_NAME )
                    depIncs.push_back( current_value );
                
                ParseNext();
            }
            
            entries.push_back( DepFileEntry( t, depIncs) );
        }
        else
        {
            cerr << "line " << line_num << " error: expected colon.\n";
            errors++;
        }
    }
    else if( current_token == NEWLINE )
    {
        ParseNext();
    }
    else if( !AtEnd() )
    {
        cerr << "line " << line_num << " error: expected file name.\n";
        errors++;
    }
}


bool ParseDep::parse()
{
    ParseNext();
    do {
        ParseDependency();
    } while( !AtEnd() && !HasError() );
	
	return HasError();
}
