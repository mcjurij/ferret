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
        : n(name), v(value)
    {}
    
    std::string name() const
    { return n; }
    
    std::string value() const
    { return v; }
    
private:
    std::string n,v;
};


// container class for XML attributes
class SimpleXMLAttributes : public std::vector<SimpleXMLAttribute>
{
public:
    void append( const std::string &name, const std::string &value)
    { push_back( SimpleXMLAttribute( name, value) ); }

    void append( const SimpleXMLAttribute &attr )
    { push_back( attr ); }

    bool hasAttribute( const std::string &name );
    std::string value( const std::string &name );
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
    
    SimpleXMLStream::TokenType readNext();
    bool readNextStartElement();
    std::string readElementText();

    void skipCurrentElement();
    
    int lineNumber() const
    { return line_num; }

    bool isStartElement() const
    { return current_token == START_ELEMENT || current_token == SELF_CLOSE_ELEMENT; }
    
    bool isEndElement() const
    { return current_token == END_ELEMENT; }

    bool IsCharacters() const
    { return IsText(); }

    bool IsText() const
    { return current_token == TEXT; }

    bool hasError() const;

    bool atEnd() const
    { return ateof; }

    void doTrim(bool t)
    { trim = t; }
    
    std::string name() const;
    
    std::string text() const;
    
    SimpleXMLAttributes attributes() const;

    std::string path() const;

    void setPathStart( int start );
    
private:
    tag_t *tagConstr( const std::string &tag_name );
    void  stackPush( tag_t *tag );
    void  stackPop();
    tag_t* stackTop() const;
    int   stackSize() const;
    bool  stackCompTop( const std::string &vs );
    int   stackLineTop();
    void  consume();
    bool  hasNextChar();
    void  skipWhite();
    void  print_err( const std::string &s );
    void  parseTag();
    void  parseNext();
    void  readUntilMatch();
    std::string trimString( const std::string &s ) const;
    
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
    int path_start;
    
    TokenType current_token;
};

#endif
