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

bool SimpleXMLAttributes::HasAttribute( const std::string &name )
{
	std::vector<SimpleXMLAttribute>::iterator it;
	for( it=begin(); it!=end(); ++it)
	{
		if( (*it).Name() == name )
			return true;
	}

	return false;
}


std::string SimpleXMLAttributes::Value( const std::string &name )
{
	std::vector<SimpleXMLAttribute>::iterator it;
	for( it=begin(); it!=end(); ++it)
	{
		if( (*it).Name() == name )
			return (*it).Value();
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
	
	Consume();
	line_num = 1;
}


SimpleXMLStream::~SimpleXMLStream()
{
	for(int i=0; i<128; i++)
		if( parsestack[i] )
			delete parsestack[i];
}


tag_t* SimpleXMLStream::TagConstr( const string &tag_name )
{
	tag_t *new_tag = new tag_t;
	assert( new_tag != 0 );

	new_tag->line = line_num;
	new_tag->tag_name = tag_name;
	
	return new_tag;
}


void  SimpleXMLStream::StackPush( tag_t *tag )
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

void SimpleXMLStream::StackPop()
{
	assert( parsestack_p>0 );
#ifdef DEBUG
	printf( "popping tag %s\n", parsestack[ parsestack_p-1 ]->tag_name.c_str());
#endif
	parsestack_p--;
}

tag_t* SimpleXMLStream::StackTop() const
{
	assert( parsestack_p>0 );
	return parsestack[ parsestack_p-1 ];
}


int SimpleXMLStream::StackSize() const
{
	return parsestack_p;
}


bool SimpleXMLStream::StackCompTop( const string &vs )
{
	tag_t *t = StackTop();
	return vs == t->tag_name;
}


int SimpleXMLStream::StackLineTop()
{
	tag_t *t = StackTop();
	return t->line;
}


void SimpleXMLStream::Consume()
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

bool SimpleXMLStream::HasNextChar()
{
	if( curr == input_length )
		Consume();
	return curr < input_length;
}

void SimpleXMLStream::SkipWhite()
{
	while( HasNextChar() && is_white(get_char()) )
		Consume();
}


#define print_err(x) \
  { fprintf( stderr, "line %d: ", line_num); \
  fprintf( stderr,(x)); }

void SimpleXMLStream::ParseTag()
{
	tag_t *t=0;
	string vs;
	bool xmlhead = false;
	
	assert( get_char() == '<' );
	
	Consume();  /* consume '<' */

	if( get_char() == '?' )  /* <?xml ...>  */
	{
		xmlhead = true;
		Consume();
	}

	if( is_tagbeg( get_char() ) )
	{
		current_token = START_ELEMENT;
		vs += get_char();
		Consume();
		while( HasNextChar() && is_tagchar( get_char() ) )
		{
			vs += get_char();
			Consume();
		}
        
		if( vs.length() == 0 )
		{
			current_token = ERROR;
			errors++;
			print_err( "malformed tag\n");
			
			return;
		}

#ifdef DEBUG
		printf( "found tag >%s<\n", vs.c_str());
#endif
		
		t = TagConstr( vs );
		
		SkipWhite();
		
		/* ok now the attribute(s) */
		while( HasNextChar() && get_char()!='>' &&
			   get_char()!='/' && get_char()!='?' && errors==0 )
		{
			string vs_an, vs_av;
			
			/* attribute name */
			while( HasNextChar() && is_attribchar( get_char() ) )
			{
				vs_an += get_char();
				Consume();
			}
            
			if( vs_an.length() == 0 )
			{
				errors++;
				current_token = ERROR;
				print_err( "malformed attribute\n");
				break;
			}
			SkipWhite();
			
			if( get_char() == '=' )
			{
				Consume();
				SkipWhite();
				if( get_char() == '"' || get_char() == '\'' )
				{
					char until = get_char();
					Consume();
					while( HasNextChar() && get_char()!=until )
					{
						vs_av += get_char();
						Consume();
					}
                    
					Consume();  /* consume the " or ' */
#ifdef DEBUG
					printf( "found attribute name  >%s<\n", vs_an.c_str());
					printf( "found attribute value >%s<\n", vs_av.c_str());
#endif
					
					/* ok. attribute is in  */
					t->attribs.Append( vs_an, Trim(vs_av));
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

			SkipWhite();
		}

		StackPush( t );
		
		if( get_char() == '/' )  /* self closing tag */
		{
			Consume();
			current_token = SELF_CLOSE_ELEMENT;
		}
	}
	else
	{
		if( get_char() == '/' ) /* closing tag */
		{
			Consume();
			
			SkipWhite();
			while( HasNextChar() && is_tagchar( get_char() ) )
			{
				vs += get_char();
				Consume();
			}
			
#ifdef DEBUG
			printf( "found closing tag >%s<\n", vs.c_str());
#endif
		
			if( StackSize() > 0 )
			{
				if( StackCompTop( vs ) )
				{
					current_token = END_ELEMENT;
					StackPop();
				}
				else
				{
					fprintf( stderr,"line %d: closing tag does not match (open tag at line %d)\n", line_num, StackLineTop());
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
			SkipWhite();

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
			Consume();
			Consume();   /* consume '-' */
			Consume();   /* consume '-' */

			while( HasNextChar()  )
			{
				if( get_char() == '-' )
				{
					Consume();
					if( get_char() == '-' )
					{
						Consume();
						if( get_char() == '>' )
							break; /* found end of comment */
					}
				}
				
				Consume(); /* consume & discard all comment chars */
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
				Consume();
				StackPop();
			}
		}
		else
		{
			if( get_char() == '>' )                
                Consume();   /* consume '>' */
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
			StackPop();
	}	
}


void SimpleXMLStream::ParseNext()
{
    if( current_token == SELF_CLOSE_ELEMENT && parsestack_p > 0 )
    {
        StackPop();                                  // remove the previously pushed element
        current_token = END_ELEMENT;
        return;
    }
    
	switch( get_char() )
	{
		case '<':
			ParseTag();
			break;
			
		default:
			current_token = TEXT;
			if( StackSize() > 0 )
			{			
				while( HasNextChar() && !is_ent(get_char()) )
				{
					StackTop()->text += get_char();
					Consume();
				}
#ifdef DEBUG
				std::cout << "found text >" << StackTop()->text << "<\n";
#endif
			}
			else
				Consume();
	}
}


SimpleXMLStream::TokenType SimpleXMLStream::ReadNext()
{
	ParseNext();

	return current_token;
}


void SimpleXMLStream::ReadUntilMatch()
{
	int save_level = parsestack_p;
	while( !AtEnd() && !HasError() && parsestack_p >= save_level)
		ParseNext();
}


string SimpleXMLStream::Trim( const string &s ) const
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

	
bool SimpleXMLStream::ReadNextStartElement()
{
	if( current_token == START_ELEMENT || current_token == SELF_CLOSE_ELEMENT )
		ParseNext();
	
	int save_level = parsestack_p;
	while( !AtEnd() && !HasError() && current_token != START_ELEMENT && current_token != SELF_CLOSE_ELEMENT &&
           parsestack_p >= save_level )
		ParseNext();

	return !HasError() && (current_token == START_ELEMENT || current_token == SELF_CLOSE_ELEMENT );
}


string SimpleXMLStream::ReadElementText()
{
	if( current_token != START_ELEMENT )
		return "";
	
	ParseNext();
	if( current_token != TEXT || StackSize() == 0 )
	{
		if( StackSize() == 0 )
			std::cerr << "Warning: No text at this position (no tag?).\n";
		return "";
	}
	ReadUntilMatch();
	assert( parsestack[ parsestack_p ] );
	
	return Trim( parsestack[ parsestack_p ]->text );
}


void SimpleXMLStream::SkipCurrentElement()
{
	if( current_token == START_ELEMENT )
	{
		ParseNext();
		
		ReadUntilMatch();
	}
    else if( current_token == SELF_CLOSE_ELEMENT )
        ParseNext();
}


bool SimpleXMLStream::HasError() const
{
	if( !ateof )
		return current_token == ERROR || errors > 0;
	else
		return parsestack_p>0  // XML file closed properly?
			|| current_token == ERROR || errors > 0;
}


bool SimpleXMLStream::Parse()        // only for TESTING
{	
	while( HasNextChar() && !HasError() )
		ParseNext();
	
	return HasError();
}


string SimpleXMLStream::Name() const
{
	if( current_token == START_ELEMENT || current_token == SELF_CLOSE_ELEMENT )
		return StackTop()->tag_name;
	else if( current_token == END_ELEMENT && parsestack[ parsestack_p ] )
		return parsestack[ parsestack_p ]->tag_name;
	else
		return "";
}


string SimpleXMLStream::Text() const
{
	if( StackSize() == 0 )
	{
		std::cerr << "Fatal: No text at this position (no tag).\n";
		return "";
	}
	
	if( current_token == END_ELEMENT && parsestack[ parsestack_p ] )
		return Trim( parsestack[ parsestack_p ]->text );
	else
		return Trim( StackTop()->text );
}


SimpleXMLAttributes SimpleXMLStream::Attributes() const
{
	if( StackSize() == 0 )
	{
		std::cerr << "Fatal: No attributes at this position (no tag).\n";
		return SimpleXMLAttributes();
	}
	
	if( current_token == END_ELEMENT && parsestack[ parsestack_p ] )
		return parsestack[ parsestack_p ]->attribs;
	else
		return StackTop()->attribs;
}


std::string SimpleXMLStream::Path() const
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
