#ifndef FERRET_MINI_BAZEL_PARSER_H_
#define FERRET_MINI_BAZEL_PARSER_H_

#include <string>
#include <vector>
#include <cmath>
#include <map>


class BazelNode;
class FctPVariable;

class MiniBazelParserOperators;

class MiniBazelParser {
public:
    typedef enum { T_INVALID = 0, T_ISWHITE,
                   T_LPAREN, T_RPAREN,
                   T_LBRACK, T_RBRACK,
                   T_EQUAL,
                   T_COMMA,
                   T_MINUS, T_ADD, T_MUL, T_DIV, T_POWER,
                   T_FPNUMBER, T_INTNUMBER,
                   T_STRING,
                   T_IDENT,
                   T_ERROR, T_EOF
    }  token_type_t;
    
    // a token has a value and a type
    typedef struct {
        token_type_t type;
        char        *value;
    } token_t;
    
private:
    static const char *token_type_to_str( token_type_t tt );
    
    typedef std::map<std::string,FctPVariable *> Variables_t;
    typedef std::map<std::string,double> Constants_t;
    
public:
    MiniBazelParser( const std::string &fmb, const std::string &d);
    
    ~MiniBazelParser();
    
    FctPVariable *addVariable( const std::string &name );

    std::vector<std::string> getVariables() const;
    
    void addConstant( const std::string &name, double val);
    
private:
    void scanner_init( const char *fkt );
    void scanner_reset();
    char consume_char();
    char peek_char();
    
    bool scanDecimalLiteral( char *s, bool *is_int );
    bool scanExponentPart( char *s );
    
    token_t tokenize();
    
    const token_t& get_current_token()
    {
        return current_token;
    }
    
    void consume()
    {
        do {
            current_token = tokenize();
            if( current_token.type == T_EOF )
                break;
            
        } while( current_token.type == T_ISWHITE );

//      cout << "just consumed: " << token_type_to_str( current_token.type ) << ": "
//           << ((current_token.value  && *(current_token.value))?current_token.value:"") << "\n";
    }
    
    bool is_here( token_type_t tt )
    {
        return current_token.type == tt;
    }
    
    token_type_t peek()
    {
        return current_token.type;
    }
    
    std::string expect( token_type_t tt, bool advance = true);
    
    bool has_next_token()
    {
        return !is_here( T_EOF );
    }
    
    void eval_call( const std::string &name );
    void eval_assignment( const std::string &ident );
    void eval_variable( const std::string &name );
    
    void eval_simple_expr();
    void eval_unary_expr();
    void eval_primary_expr();
    void eval_exponent();
    void eval_multiplicative();
    void eval_additive();
    
    void eval_expr();
    
public:
    bool parse();
    BazelNode *getStartNode();
    
private:
    MiniBazelParserOperators *opera;
    
    char current_token_value[1024];
    char current_char;

    int current_pos;
    const char *scanner_fct;
    token_t current_token;
    
    bool err_state;
    bool at_eof,done;
    
    Variables_t variables;   //! maps variable name to binder object
    Constants_t constants;   //! maps constant name to double value
    
};


class MiniBazelParserOperators {

public:
    MiniBazelParserOperators( const std::string &d ) : startNode(0), currNode(0), dir(d) {}

    ~MiniBazelParserOperators()
    { }

    void setStartNode( BazelNode *n )
    { startNode = n; }
    BazelNode *getStartNode() const
    { return startNode; }
    
    BazelNode *mkNewNode();
    BazelNode *getCurrNode()
    { return currNode; }
    
    void setCurrentAssignTo( const std::string &name );
    void addAssign( const std::string &v );
    void finishAssign();

private:
    BazelNode *startNode, *currNode;
    std::string dir;
    std::string assignName;
    std::vector<std::string> assignValues;
    
public:
    void op( const MiniBazelParser::token_t& op_token );
    void unary_op( MiniBazelParser::token_type_t op_type );

    void variable_op( FctPVariable *v );
    void constant_op( double constant );
    
};



#endif
