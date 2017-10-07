/*
 *
 * Parse a mini (or rather tiny) subset of the bazel BUILD files
 *
 */

#include <cassert>
#include <iostream>
#include <string>
#include <vector>
#include <cmath>
#include <map>

#include "mini_bazel_parser.h"
#include "bazel_node.h"

using namespace std;

// intentionally not using cctype here
#define is_digit(c) ((c)>='0' && (c)<='9')
#define is_alpha(c) (((c)>='a' && (c)<='z') || ((c)>='A' && (c)<='Z'))

#define is_white(c) ((c)==' ' || (c)=='\t' || (c)=='\n')

#define is_entity_beg(c)  (is_alpha(c))
#define is_entity_char(c) (is_alpha(c) || (c)=='_' || is_digit(c))

#define is_fpnum_beg(c) (is_digit(c) || (c)=='.')


const char *MiniBazelParser::token_type_to_str( token_type_t tt )
{
    switch( tt )
    {
        case T_INVALID:
            return "INVALID";
        case T_ISWHITE:
            return "is white";

        case T_LPAREN:
            return " ( ";
        case T_RPAREN:
            return " ) ";
        case T_LBRACK:
            return " [ ";
        case T_RBRACK:
            return " ] ";
        case T_EQUAL:
            return " = ";
        case T_COMMA:
            return " , ";
    
        case T_MINUS:
            return " MINUS ";
        case T_ADD:
            return "+";
        case T_MUL:
            return "*";
        case T_DIV:
            return "/";
        case T_POWER:
            return "^";
        
        case T_FPNUMBER:
            return "FP NUMBER";
        case T_INTNUMBER:
            return "INTNUMBER";
        case T_STRING:
            return "STRING";
        case T_IDENT:
            return "IDENT";
        
        case T_ERROR:
            return "error";
        case T_EOF:
            return "EOF";
        default:
            return "? unknown token ?";
    }
    return "";
}


// parser helper class
class MiniBazelParserException {

    MiniBazelParserException();
public:
    MiniBazelParserException( const string & s ) : desc(s)
    {
    }

    string reason() const
    {
        return desc;
    }

private:
    string desc;
};


BazelNode *MiniBazelParserOperators::mkNewNode()
{
    BazelNode *node = new BazelNode( dir );
    if( startNode == 0 )
        startNode = node;
    else if( currNode )
        currNode->setSibling( node );
    currNode = node;
    
    return currNode;
}


void MiniBazelParserOperators::finishNode()
{
}


void MiniBazelParserOperators::setCurrentAssignTo( const string &name )
{
    assignName = name;
    assignValues.clear();
}


void MiniBazelParserOperators::addAssign( const string &v )
{
    assignValues.push_back( v );
}


void MiniBazelParserOperators::finishAssign()
{
    if( assignValues.size() > 0 )
    {
        if( assignName == "name" )
        {
            currNode->setName( assignValues[0] );
            currNode->setTarget( assignValues[0] );
        }
        else if( assignName == "srcs" )
            currNode->setSrcs( assignValues );
        else if( assignName == "hdrs" )
            currNode->setHdrs( assignValues );
        else if( assignName == "deps" )
            currNode->setDeps( assignValues );

        assignValues.clear();
    }
    assignName = "";
}


void MiniBazelParserOperators::op( const MiniBazelParser::token_t& op_token )
{
    switch( op_token.type )
    {
        case MiniBazelParser::T_ADD:

            break;
        case MiniBazelParser::T_MINUS:

            break;
        case MiniBazelParser::T_MUL:

            break;
        case MiniBazelParser::T_DIV:

            break;
        case MiniBazelParser::T_POWER:   // ^

            break;
        default:
            throw MiniBazelParserException( "unsupported operand" );
    }
}


void MiniBazelParserOperators::unary_op( MiniBazelParser::token_type_t op_type )
{
//    if( op_type == MiniBazelParser::T_MINUS )
//        tmp_inst_list.push_back( MiniBazelParserInstr::UNARY_MINUS );
}


void MiniBazelParserOperators::variable_op( FctPVariable *v )
{

}


void MiniBazelParserOperators::constant_op( double constant )
{

}



// variable binder
class FctPVariable {
private:
    FctPVariable();
    
public:
    FctPVariable( const string &n ) : name(n), val_addr(0) {}

    string getName() const
    { return name; }
    
    void bind( double *a )
    { val_addr = a; }

    double value() const
    { return *val_addr; }
    
private:
    string name;
    double *val_addr;
};



// MiniBazelParser --------------------------------------------------------------
MiniBazelParser::MiniBazelParser( const string &fmb, const string &d)
{
    scanner_init( fmb );
    err_state = false;
    done = false;
    
    opera = new MiniBazelParserOperators( d );
}


MiniBazelParser::~MiniBazelParser()
{
    Variables_t::iterator itv;
    for( itv = variables.begin(); itv != variables.end(); ++itv)
        delete itv->second;

    delete opera;
}


FctPVariable *MiniBazelParser::addVariable( const string &name )
{
    Variables_t::iterator it = variables.find( name );
    
    FctPVariable *var = 0;
    if( it == variables.end() )
        var = new FctPVariable( name );
    else
        var = it->second;
    
    variables[ name ] = var;
    
    return var;
}


vector<string> MiniBazelParser::getVariables() const
{
    Variables_t::const_iterator itv;
    vector<string> names;
    
    for( itv = variables.begin(); itv != variables.end(); ++itv)
        names.push_back( itv->second->getName() );

    return names;
}


void MiniBazelParser::addConstant( const string &name, double val)
{
    constants[ name ] = val;
}

    
void MiniBazelParser::scanner_init( const string &fmb ) 
{
    current_char = 0;
    at_eof = true;
    current_pos = 0;
    current_line = 1;
    
    scanner_inp = fmb;
    if( (current_char = scanner_inp[0] ) != 0 )
    {
        at_eof = false;
    }
    err_state = false;
}


void MiniBazelParser::scanner_reset() 
{
    at_eof = false;
    current_pos = 0;
    current_line = 1;
    current_token.type = T_INVALID;

    current_char = scanner_inp[0];
}


char MiniBazelParser::consume_char()
{
    char cc = current_char;
    if( (current_char = scanner_inp[++current_pos]) != 0 )
    {
        if( cc == '\n' )
            current_line++;
        return cc;
    }
    else
    {
        at_eof = true;
        current_char = 0;
        current_token.type = T_EOF;
        return cc;
    }
}


char MiniBazelParser::peek_char()
{
    if( current_char != 0 )
    {
        return current_char;
    }
    else
    {
        at_eof = true;
        return (char)0;
    }
}


bool MiniBazelParser::scanDecimalLiteral( char *s, bool *is_int )
{
    char c = peek_char();
    int before_dot = 0;
    
    assert( is_digit(c) || (c == '.') );

    *is_int = false;
    
    if( is_digit(c) ) {
        before_dot++;
        *s = consume_char(); s++;
        while( is_digit(peek_char()) )
        {
            *s = consume_char(); s++;
        }   
    }
    if( peek_char()=='.' ) {
        *s = consume_char();
        s++;
        
        if( before_dot && (peek_char() == 'e' || peek_char() == 'E') ) {  // 3.E+10 case
            return scanExponentPart( s );
        }
        else
        {
            if( before_dot == 0 && !is_digit(peek_char()) ) // .E+10 not allowed
            {
                *s = 0;
                return false;
            }
            else
            {
                while( is_digit(peek_char()) )
                {
                    *s = consume_char(); s++;
                }
                *s = 0;
                if( peek_char() == 'e' || peek_char() == 'E' )
                    return scanExponentPart( s );
                else
                    return true;
            }
        }
    }
    else if(before_dot && (peek_char() == 'e' || peek_char() == 'E'))   // 3E+10 case
        return scanExponentPart( s );

    if( before_dot )
        *is_int = true;
    *s = 0;
    return true;
}


bool MiniBazelParser::scanExponentPart( char *s )  // exp. part is always optional
{
    char c = peek_char();
    
    assert( (c == 'e') || (c == 'E') );
    
    *s = consume_char(); s++;
    
    c = peek_char();
    if( (c == '+') || (c == '-') )      // exponent can have a sign
    {
        *s = consume_char(); s++;
    }

    c = peek_char();
    if( is_digit(c) ){                        // consume digits after e,e- or e+
        *s = consume_char(); s++;
        while( is_digit(peek_char()) )
        {
            *s = consume_char(); s++;
        }
    }
    else     // this is an error since exponent indicated but not followed by integer
        return false;
    *s = 0;
    return true;
}


MiniBazelParser::token_t MiniBazelParser::tokenize()
{
    char *s = current_token_value ;
    token_t t;
    int errors=0;
    token_type_t tt;
    char c = peek_char();
    
    t.value = current_token_value;
    *s = 0;
    
    if( !at_eof )
    {
        if( is_white(c) )
        {
            while( !at_eof && is_white(peek_char()) )
                consume_char();
            
            tt = T_ISWHITE;
        }
        else if( is_entity_beg(c) )
        {
            *s = consume_char(); s++;
            
            while( !at_eof && is_entity_char(peek_char()) )
            {
                *s = consume_char(); s++;
            }
            *s = 0;
            
            tt = T_IDENT;
        }
        else if( c=='"' )
        {
            consume_char();
            
            while( !at_eof && peek_char()!='"' )   // FIXME handle escaped "
            {
                *s = consume_char(); s++;
            }
            *s = 0;
            consume_char();
            
            tt = T_STRING;
        }
        else if( c=='=' )
        {
            consume_char();
            tt = T_EQUAL;
        }
        else if( c==',' )
        {
            consume_char();
            tt = T_COMMA;
        }
        else if( c=='(' )
        {
            consume_char();
            tt = T_LPAREN;
        }
        else if( c==')' )
        {
            consume_char();
            tt = T_RPAREN;
        }
        else if( c=='[' )
        {
            consume_char();
            tt = T_LBRACK;
        }
        else if( c==']' )
        {
            consume_char();
            tt = T_RBRACK;
        }
        else if( is_fpnum_beg(c) )
        {
            bool is_int;
            if( !scanDecimalLiteral( s, &is_int) )
                errors++;
            tt = (is_int)?T_INTNUMBER:T_FPNUMBER;
        }
        else if( c=='-' )
        {
            consume_char();
            tt = T_MINUS;
        }
        else if( c=='+' )
        {
            consume_char();
            tt = T_ADD;
        }
        else if( c=='*' )
        {
            consume_char();
            tt = T_MUL;
        }
        else if( c=='/' )
        {
            consume_char();
            tt = T_DIV;
        }
        else if( c=='^' )
        {
            consume_char();
            tt = T_POWER;
        }
        else
        {
            cerr << "unexpected char";
            if( !at_eof )
                cerr << " '" << c << "'";
            cerr << "\n";
            errors++;
        }
        if( !errors )
        {
            t.type  = tt;
        }
    }
    else
        t.type  = T_EOF;
    
    if( errors )
    {
        t.type  = T_ERROR;
        *t.value = 0;
        current_token_value[0] = '\0'; 
        cerr << "tokenizer has errors.\n";
    }
    current_token.type = t.type;
    current_token.value = current_token_value;
    
    return t;
}


string MiniBazelParser::expect( token_type_t tt, bool advance)
{
    string s = current_token.value;
    
    if( !is_here( tt ) )
    {
        if( tt == T_FPNUMBER && current_token.type == T_INTNUMBER )  // fp relax for ints
        {
            if( advance )
                consume();
        }
        else
            throw MiniBazelParserException(
                string("expected ") + token_type_to_str( tt ) + " but found " +
                token_type_to_str( current_token.type ) );
    }
    else if( advance )
        consume();
    
    return s;
}


void MiniBazelParser::eval_call( const string &name )
{
    assert( is_here( T_LPAREN ) );

    if( name == "cc_library" || name == "cc_binary" )
    {
        BazelNode *n = opera->mkNewNode();

        if( name == "cc_library" )
            n->setType( "library" );
        else if( name == "cc_binary" )
            n->setType( "executable" );
    }
    
    int count_args = 0;
    
    do {
        consume();     // consume first '(', ',' afterwards
        if( is_here( T_RPAREN ) )
            break;

        eval_expr();
        count_args++;
    }
    while( is_here( T_COMMA ) );
    
    expect( T_RPAREN );
    
    if( name == "cc_library" || name == "cc_binary" )
    {
        opera->finishNode();
    }
}


void MiniBazelParser::eval_assignment( const string &ident )
{
    assert( is_here( T_EQUAL ) );
    consume();

    opera->setCurrentAssignTo( ident );
    
    if( is_here( T_LBRACK ) )
    {
        do {
            consume();     // consume '[' first, ',' afterwards
            if( is_here( T_RBRACK ) )
                break;
            
            eval_expr();
        }
        while( is_here( T_COMMA ) );
        
        expect( T_RBRACK );
    }
    else
    {
        eval_expr();
    }

    opera->finishAssign();
}


void MiniBazelParser::eval_variable( const string &name )
{
    // let's first see if it is a known constant
    Constants_t::const_iterator it = constants.find( name );

    if( it != constants.end() )
        opera->constant_op( it->second );
    else
        opera->variable_op( addVariable( name ) );
}


void MiniBazelParser::eval_simple_expr()
{
    switch( peek() )
    {
        case T_FPNUMBER:
        case T_INTNUMBER:
            cout << current_token.value << "\n";
            opera->constant_op( double(atof(current_token.value)) );
            consume();
            break;

        case T_STRING:
            cout << current_token.value << "\n";
            opera->addAssign( current_token.value );
            consume();
            break;
            
        case T_IDENT:
            {
                cout << current_token.value << endl;
            
                string ident = current_token.value;
                consume();
                if( is_here( T_LPAREN ) ) // function ?
                {
                    eval_call(ident);     // then ident is the name of the function
                }
                else if( is_here( T_EQUAL ) ) // assignment?
                {
                    eval_assignment( ident );
                }
                else       // variable
                {
                    eval_variable(ident);
                }
            }
            break;
            
        default:
            throw MiniBazelParserException( string("unexpected value (found ") +
                                            (*current_token.value?current_token.value:
                                             token_type_to_str( current_token.type )) + ")" );
            break;
    }
}


void MiniBazelParser::eval_unary_expr()
{
    if( is_here( T_MINUS)  )
    {
        consume();
        // cout << "found unary minus!" << endl;
        eval_primary_expr();
        cout << "* -1" << endl;
        opera->unary_op( T_MINUS );
    }
    else
        eval_simple_expr();
}


void MiniBazelParser::eval_primary_expr()
{
    if( is_here( T_LPAREN ) )
    {
        consume();
        eval_expr();

        expect( T_RPAREN );
    }
    else
    {
        eval_unary_expr();
    }
}


void MiniBazelParser::eval_exponent()
{
    eval_primary_expr();
    
    if( is_here( T_POWER ) ) {
        token_t t = current_token;
        consume();
        
        eval_exponent();  // evaluate from right
        cout << "^" << endl;
        opera->op( t );
    }    
}


void MiniBazelParser::eval_multiplicative()
{
    eval_exponent();
    
    while( is_here( T_MUL ) || is_here( T_DIV ) )
    {
        string op = is_here( T_MUL )?"*":"/";
        token_t t = current_token;
        
        consume();
        
        eval_exponent();
        cout << " " << op << endl;
        opera->op( t );
    }
}


void MiniBazelParser::eval_additive()
{
    eval_multiplicative();
    
    while( is_here( T_ADD ) || is_here( T_MINUS ) )
    {
        string op = is_here( T_ADD )?"+":"-";
        token_t t = current_token;
        
        consume();
        
        eval_multiplicative();
        cout << " " << op << endl;
        opera->op( t );
    }
}


void MiniBazelParser::eval_expr()
{
    eval_additive();
}


bool MiniBazelParser::parse()
{
    try {
        consume();
        
        do{
            eval_expr();
        }
        while( !is_here( T_EOF ) );
    }
    catch( MiniBazelParserException & e ) {
        cerr << "line " << current_line << ": " << e.reason() << endl;
        err_state = true;
    }

    if( !is_here( T_EOF ) )
    {
        // leftovers?
        if( !err_state && has_next_token() )
        {
            cerr << "syntax error near '"
                 << get_current_token().value << "' at end of input" << endl;
            consume();
            err_state = true;
        }
    }
    
    scanner_reset();   // reset scanner
    return !err_state;
}


BazelNode *MiniBazelParser::getStartNode()
{
    return opera->getStartNode();
}
