
#ifndef FERRET_SIMPLEXMLSTREAM_H
#define FERRET_SIMPLEXMLSTREAM_H

#include <string>
#include <cstdio>
#include <vector>


// holds XML attributes (name + value)
class SimpleXMLAttribute
{
public:
	SimpleXMLAttribute( const std::string &name, const std::string &value)
		: name(name), value(value)
	{}

	std::string Name() const
	{ return name; }

	std::string Value() const
	{ return value; }

private:
	std::string name,value;
};


// container class for XML attributes
class SimpleXMLAttributes : public std::vector<SimpleXMLAttribute>
{
public:
	void Append( const std::string &name, const std::string &value)
	{ push_back( SimpleXMLAttribute( name, value) ); }

	void Append( const SimpleXMLAttribute &attr )
	{ push_back( attr ); }

	bool HasAttribute( const std::string &name );
	std::string Value( const std::string &name );
};


struct tag_t;

/*
 * Interface (very roughly) modelled after Qt's http://doc.qt.nokia.com/latest/qxmlstreamreader.html
 *
 * Differences: FILE instead of QFile as input, fewer TokenTypes, no DTD or other
 *              complicated and fancy XML stuff, no character encodings
 */
class SimpleXMLStream
{

public:
	
	enum TokenType {
		INVALID = 0,ERROR,
		START_ELEMENT,
		END_ELEMENT,
        SELF_CLOSE_ELEMENT,
		TEXT = 6,
		COMMENT,
        XML_HEADER = 40
	};
	
	SimpleXMLStream( FILE *in );
	~SimpleXMLStream();
	
	bool Parse();       // TESTING
	
	SimpleXMLStream::TokenType ReadNext();
	bool ReadNextStartElement();
	std::string ReadElementText();

	void SkipCurrentElement();
	
	int LineNumber() const
	{ return line_num; }

	bool IsStartElement() const
	{ return current_token == START_ELEMENT || current_token == SELF_CLOSE_ELEMENT; }
	
	bool IsEndElement() const
	{ return current_token == END_ELEMENT; }

	bool IsCharacters() const
	{ return IsText(); }

	bool IsText() const
	{ return current_token == TEXT; }

	bool HasError() const;

	bool AtEnd() const
	{ return ateof; }

	void DoTrim(bool t)
	{ trim = t; }
	
	std::string Name() const;
	
	std::string Text() const;
	
	SimpleXMLAttributes Attributes() const;

    std::string Path() const;
    
private:
	tag_t *TagConstr( const std::string &tag_name );
	void  StackPush( tag_t *tag );
	void  StackPop();
	tag_t* StackTop() const;
	int   StackSize() const;
	bool  StackCompTop( const std::string &vs );
	int   StackLineTop();
	void  Consume();
	bool  HasNextChar();
	void  SkipWhite();
	void  ParseTag();
	void  ParseNext();
	void  ReadUntilMatch();
	std::string Trim( const std::string &s ) const;
	
	FILE *in_fp;
	bool ateof;
	int line_num;
	char input_buf[3];
	int input_length;
	int curr;
	int errors;
	bool trim;
	tag_t *parsestack[ 128 ];
	int    parsestack_p;

	TokenType current_token;
};

#endif
