// g++ -std=c++11 -Wall -c SimpleXMLStream.cpp
#include <cstdlib>
#include <cstdio>
#include <cstring>
#include <cassert>

#include <string>
#include <iostream>
#include <sstream>

#include "simple_xml_stream.h"

using std::string;
using std::stringstream;

// #define DEBUG 1

bool SimpleXMLAttributes::hasAttribute( const std::string &name )
{
    std::vector<SimpleXMLAttribute>::iterator it;
    for( it=begin(); it!=end(); ++it)
    {
        if( (*it).name() == name )
            return true;
    }

    return false;
}


std::string SimpleXMLAttributes::value( const std::string &name )
{
    std::vector<SimpleXMLAttribute>::iterator it;
    for( it=begin(); it!=end(); ++it)
    {
        if( (*it).name() == name )
            return (*it).value();
    }

    return "";
}


/* intentionally not using ctype.h here -- locale sucks */
#define is_digit(c) ((c)>='0' && (c)<='9')
#define is_alpha(c) (((c)>='a' && (c)<='z') || ((c)>='A' && (c)<='Z'))
#define is_special(c) ((c)=='_' || (c)=='-' || (c)==':')
#define is_white(c) ((c)==' ' || (c)=='\t')

#define is_tagbeg(c)  (is_alpha(c) || is_special(c))
#define is_tagchar(c) (is_tagbeg(c) || is_digit(c))

#define is_ent(c)  ((c)=='<' || (c)=='>')
#define is_attribchar(c) (is_tagchar(c))


struct tag_t {
    int line;
    string tag_name;
    SimpleXMLAttributes attribs;
    string text;
}; 


SimpleXMLStream::SimpleXMLStream( FILE *in )
    : in_fp(in), ateof(false),line_num(0),
      curr(0),errors(0), trim(true),parsestack_p(0),path_start(0),
      current_token(INVALID)
{
    assert(in);
    for(int i=0; i<128; i++)
        parsestack[i] = 0;
    
    consume();
    line_num = 1;
}


SimpleXMLStream::~SimpleXMLStream()
{
    for(int i=0; i<128; i++)
        if( parsestack[i] )
            delete parsestack[i];
}


tag_t* SimpleXMLStream::tagConstr( const string &tag_name )
{
    tag_t *new_tag = new tag_t;
    assert( new_tag != 0 );

    new_tag->line = line_num;
    new_tag->tag_name = tag_name;
    
    return new_tag;
}


void  SimpleXMLStream::stackPush( tag_t *tag )
{
    assert( tag != 0 );

    tag->line = line_num;
#ifdef DEBUG
    printf( "pushing tag %s\n", tag->tag_name.c_str());
#endif
    
    if( parsestack[ parsestack_p ] != 0 )
    {
        delete parsestack[ parsestack_p ];
        parsestack[ parsestack_p ] = 0;
    }

    assert( parsestack_p<128 );
    parsestack[ parsestack_p++ ] = tag;
}

void SimpleXMLStream::stackPop()
{
    assert( parsestack_p>0 );
#ifdef DEBUG
    printf( "popping tag %s\n", parsestack[ parsestack_p-1 ]->tag_name.c_str());
#endif
    parsestack_p--;
}

tag_t* SimpleXMLStream::stackTop() const
{
    assert( parsestack_p>0 );
    return parsestack[ parsestack_p-1 ];
}


int SimpleXMLStream::stackSize() const
{
    return parsestack_p;
}


bool SimpleXMLStream::stackCompTop( const string &vs )
{
    tag_t *t = stackTop();
    return vs == t->tag_name;
}


int SimpleXMLStream::stackLineTop()
{
    tag_t *t = stackTop();
    return t->line;
}


void SimpleXMLStream::consume()
{
    int c = fgetc( in_fp );
    if( c>=0 )
    {
        input_buf[0] = (char)c;
        input_buf[1] = '\0';
        input_length = 1;
        
        if( input_buf[0]  == '\n' )
        {
            line_num++;
#ifdef DEBUG
            printf( "input line %d:\n", line_num);
#endif
            
            input_buf[0] = ' ';   /* return becomes white -- is this always correnct? */
        }       
    }
    else
    {
        ateof=true;
        curr=input_length = 0;
    }
}


#define  get_char()  ((curr < input_length)?input_buf[ curr ]:'$')

bool SimpleXMLStream::hasNextChar()
{
    if( curr == input_length )
        consume();
    return curr < input_length;
}

void SimpleXMLStream::skipWhite()
{
    while( hasNextChar() && is_white(get_char()) )
        consume();
}


void SimpleXMLStream::print_err( const string &s )
{
    std::cerr << "line " << line_num << " " << s;
}


void SimpleXMLStream::parseTag()
{
    tag_t *t=0;
    string vs;
    bool xmlhead = false;
    
    assert( get_char() == '<' );
    
    consume();  /* consume '<' */

    if( get_char() == '?' )  /* <?xml ...>  */
    {
        xmlhead = true;
        consume();
    }

    if( is_tagbeg( get_char() ) )
    {
        current_token = START_ELEMENT;
        vs += get_char();
        consume();
        while( hasNextChar() && is_tagchar( get_char() ) )
        {
            vs += get_char();
            consume();
        }
        
        if( vs.length() == 0 )
        {
            current_token = ERROR;
            errors++;
            print_err( "malformed tag\n" );
            
            return;
        }

#ifdef DEBUG
        printf( "found tag >%s<\n", vs.c_str());
#endif
        
        t = tagConstr( vs );
        
        skipWhite();
        
        /* ok now the attribute(s) */
        while( hasNextChar() && get_char()!='>' &&
               get_char()!='/' && get_char()!='?' && errors==0 )
        {
            string vs_an, vs_av;
            
            /* attribute name */
            while( hasNextChar() && is_attribchar( get_char() ) )
            {
                vs_an += get_char();
                consume();
            }
            
            if( vs_an.length() == 0 )
            {
                errors++;
                current_token = ERROR;
                print_err( "malformed attribute\n");
                break;
            }
            skipWhite();
            
            if( get_char() == '=' )
            {
                consume();
                skipWhite();
                if( get_char() == '"' || get_char() == '\'' )
                {
                    char until = get_char();
                    consume();
                    while( hasNextChar() && get_char()!=until )
                    {
                        vs_av += get_char();
                        consume();
                    }
                    
                    consume();  /* consume the " or ' */
#ifdef DEBUG
                    printf( "found attribute name  >%s<\n", vs_an.c_str());
                    printf( "found attribute value >%s<\n", vs_av.c_str());
#endif
                    
                    /* ok. attribute is in  */
                    t->attribs.append( vs_an, trimString(vs_av));
                }
                else
                {
                    current_token = ERROR;
                    errors++;
                    print_err( " expected \" (or \') for attribute value\n" );
                }
            }
            else
            {
                current_token = ERROR;
                errors++;
                print_err( " expected '='\n" );
            }

            skipWhite();
        }

        stackPush( t );
        
        if( get_char() == '/' )  /* self closing tag */
        {
            consume();
            current_token = SELF_CLOSE_ELEMENT;
        }
    }
    else
    {
        if( get_char() == '/' ) /* closing tag */
        {
            consume();
            
            skipWhite();
            while( hasNextChar() && is_tagchar( get_char() ) )
            {
                vs += get_char();
                consume();
            }
            
#ifdef DEBUG
            printf( "found closing tag >%s<\n", vs.c_str());
#endif
        
            if( stackSize() > 0 )
            {
                if( stackCompTop( vs ) )
                {
                    current_token = END_ELEMENT;
                    stackPop();
                }
                else
                {
                    fprintf( stderr,"line %d: closing tag does not match (open tag at line %d)\n", line_num, stackLineTop());
                    errors++;
                    current_token = ERROR;
                }
            }
            else
            {
                current_token = ERROR;
                errors++;
                print_err( "closing tag without any open tag\n" );
            }
            skipWhite();
            
            if( get_char() != '>' )
            {
                current_token = ERROR;
                errors++;
                print_err( "closing tag is crap\n" );
            }
        }
        else if( get_char() == '!' ) /* comment */
        {
            current_token = COMMENT;
            consume();
            consume();   /* consume '-' */
            consume();   /* consume '-' */

            while( hasNextChar()  )
            {
                if( get_char() == '-' )
                {
                    consume();
                    if( get_char() == '-' )
                    {
                        consume();
                        if( get_char() == '>' )
                            break; /* found end of comment */
                    }
                }
                
                consume(); /* consume & discard all comment chars */
            }
        }
        else
        {
            current_token = ERROR;
            errors++;
            fprintf( stderr, "'%c' is not an expected char at begin of tag\n", get_char());
        }
    }

    if( errors==0 )
    {
        if( xmlhead )
        {
            current_token = XML_HEADER;
            if( get_char() != '?' )
            {
                current_token = ERROR;
                errors++;
                print_err( "expected '?' before end of tag\n" );
            }
            else
            {
                consume();
                consume();
                stackPop();
            }
        }
        else
        {
            if( get_char() == '>' )                
                consume();   /* consume '>' */
            else
            {
                current_token = ERROR;
                errors++;
            }
        }
    }
    else
    {
        current_token = ERROR;
        
        if( t!= 0 )
            stackPop();
    }   
}


void SimpleXMLStream::parseNext()
{
    if( current_token == SELF_CLOSE_ELEMENT && parsestack_p > 0 )
    {
        stackPop();                                  // remove the previously pushed element
        current_token = END_ELEMENT;
        return;
    }
    
    switch( get_char() )
    {
        case '<':
            parseTag();
            break;
            
        case '>':
            current_token = ERROR;
            print_err( "stray '>'\n" );
            break;
            
        default:
            current_token = TEXT;
            if( stackSize() > 0 )
            {           
                while( hasNextChar() && !is_ent(get_char()) )
                {
                    stackTop()->text += get_char();
                    consume();
                }
#ifdef DEBUG
                std::cout << "found text >" << stackTop()->text << "<\n";
#endif
            }
            else
                consume();
    }
}


SimpleXMLStream::TokenType SimpleXMLStream::readNext()
{
    parseNext();

    return current_token;
}


void SimpleXMLStream::readUntilMatch()
{
    int save_level = parsestack_p;
    while( !atEnd() && !hasError() && parsestack_p >= save_level)
        parseNext();
}


string SimpleXMLStream::trimString( const string &s ) const
{
    if( trim && s.length()>0 )
    {
        size_t j=s.find_last_not_of( " \t\r" );
        if( j == string::npos )
            return "";
        size_t i=s.find_first_not_of( " \t\r" );
        return s.substr( i, j-i+1);
    }
    else
        return s;
}

    
bool SimpleXMLStream::readNextStartElement()
{
    if( current_token == START_ELEMENT || current_token == SELF_CLOSE_ELEMENT )
        parseNext();
    
    int save_level = parsestack_p;
    while( !atEnd() && !hasError() && current_token != START_ELEMENT && current_token != SELF_CLOSE_ELEMENT &&
           parsestack_p >= save_level )
        parseNext();

    return !hasError() && (current_token == START_ELEMENT || current_token == SELF_CLOSE_ELEMENT );
}


string SimpleXMLStream::readElementText()
{
    if( current_token != START_ELEMENT )
        return "";
    
    parseNext();
    if( current_token != TEXT || stackSize() == 0 )
    {
        if( stackSize() == 0 )
            std::cerr << "Warning: No text at this position (no tag?).\n";
        return "";
    }
    readUntilMatch();
    assert( parsestack[ parsestack_p ] );
    
    return trimString( parsestack[ parsestack_p ]->text );
}


void SimpleXMLStream::skipCurrentElement()
{
    if( current_token == START_ELEMENT )
    {
        parseNext();
        
        readUntilMatch();
    }
    else if( current_token == SELF_CLOSE_ELEMENT )
        parseNext();
}


bool SimpleXMLStream::hasError() const
{
    if( !ateof )
        return current_token == ERROR || errors > 0;
    else
        return parsestack_p>0  // XML file closed properly?
            || current_token == ERROR || errors > 0;
}


bool SimpleXMLStream::Parse()        // only for TESTING
{   
    while( hasNextChar() && !hasError() )
        parseNext();
    
    return hasError();
}


string SimpleXMLStream::name() const
{
    if( current_token == START_ELEMENT || current_token == SELF_CLOSE_ELEMENT )
        return stackTop()->tag_name;
    else if( current_token == END_ELEMENT && parsestack[ parsestack_p ] )
        return parsestack[ parsestack_p ]->tag_name;
    else
        return "";
}


string SimpleXMLStream::text() const
{
    if( stackSize() == 0 )
    {
        std::cerr << "Fatal: No text at this position (no tag).\n";
        return "";
    }
    
    if( current_token == END_ELEMENT && parsestack[ parsestack_p ] )
        return trimString( parsestack[ parsestack_p ]->text );
    else
        return trimString( stackTop()->text );
}


SimpleXMLAttributes SimpleXMLStream::attributes() const
{
    if( stackSize() == 0 )
    {
        std::cerr << "Fatal: No attributes at this position (no tag).\n";
        return SimpleXMLAttributes();
    }
    
    if( current_token == END_ELEMENT && parsestack[ parsestack_p ] )
        return parsestack[ parsestack_p ]->attribs;
    else
        return stackTop()->attribs;
}


std::string SimpleXMLStream::path() const
{
    stringstream ss;
    int i;

    for( i = path_start; i < parsestack_p; i++)
        ss << "/" << parsestack[ i ]->tag_name;
    
    return ss.str();
}


void SimpleXMLStream::setPathStart( int start )
{
    path_start = start;
    
    if( path_start > parsestack_p )
        path_start = parsestack_p;
}
