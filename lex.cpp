#include <assert.h>
#include "lex.hpp"
#include "error.hpp"
#include <queue>

bool is_space( int c ) { return c == '\n' || c == '\t' || c == ' ' || c == '\r'; }

bool is_letter( int c ) { return isalpha( c ) || c == '_'; }

std::queue<Tokinfo> tokens;

void unlex( Tokinfo t ) { tokens.push( t ); }

Tokinfo lex( FILE *fp ) {

    // static to preserve value across calls.
    // this allows me to inspect result's token before I set it in a given call
    // to check what the previous token was
    static Tokinfo result;

    // prioritize tokens in the queue
    if ( !tokens.empty() ) {
        result = tokens.front();
        tokens.pop();
        return result;
    }

    // CITATION-ish: the following code draws heavily from the FSM example given to us
    // by Dr. Aycock in class -
    // linked here -https://d2l.ucalgary.ca/d2l/le/content/500748/viewContent/5744437/View
    spin:
    int ch;
    // spin until we find something interesting
    while ( ( ch = getc( fp ) ) != EOF && is_space( ch ) )
        if ( ch == '\n' ) result.linenum++;

    result.lexeme = ch; // first (and possibly only) character of the lexeme

    // FSM based on our character that chooses what token to return
    // note that after the statement nothing happens except returning `result`
    // this means that calling `break` inside the statement is effectively a
    // return
    switch ( ch ) {
        // NULL character isn't allowed in input
        case NULL:
            warning( "skipping NULL character", result.linenum );
            goto spin;
        case EOF:
            // insert semicolon according to spec
            if (   result.token == TOKEN_ID
                || result.token == TOKEN_INT
                || result.token == TOKEN_STRING
                || result.token == TOKEN_BREAK
                || result.token == TOKEN_RETURN
                || result.token == TOKEN_RBRACE
                || result.token == TOKEN_RPAREN
            ) {
                result.token  = TOKEN_SEMICOLON;
                ungetc( ch, fp );
            }
            else result.token = TOKEN_EOF;

            result.lexeme = "";
            break;
        case '+':
            result.token = TOKEN_PLUS;
            break;
        case '-':
            result.token = TOKEN_MINUS;
            break;
        case '*':
            result.token = TOKEN_STAR;
            break;
        case '%':
            result.token = TOKEN_PERCENT;
            break;
        case '(':
            result.token = TOKEN_LPAREN;
            break;
        case ')':
            result.token = TOKEN_RPAREN;

            // peek next char to see if its a newline. if so, insert a semicolon
            if ( ( ch = getc( fp ) ) == '\n' )
                unlex( Tokinfo { TOKEN_SEMICOLON, result.linenum, "", } );

            // put back to mimick "peeking"
            ungetc( ch, fp );
            break;
        case '{':
            result.token = TOKEN_LBRACE;
            break;
        case '}':
            // if last token wasn't a semicolon, insert a semicolon into the token stream
            if ( result.token != TOKEN_SEMICOLON && result.token != TOKEN_UNSET ) {
                result.token  = TOKEN_SEMICOLON;
                result.lexeme = "";
                ungetc( ch, fp );
                break;
            }

            result.token = TOKEN_RBRACE;

            // peek next char to see if its a newline. if so, insert a semicolon
            if ( ( ch = getc( fp ) ) == '\n' )
                unlex( Tokinfo { TOKEN_SEMICOLON, result.linenum, "", } );

            // put back to mimick "peeking"
            ungetc( ch, fp );
            break;
        case ',':
            result.token = TOKEN_COMMA;
            break;
        case ';':
            result.token = TOKEN_SEMICOLON;
            break;
        case '&':
            // no unary & operator
            if ( ( ch = getc( fp ) ) != '&' ) error( "bitwise AND not supported in GoLF", result.linenum );

            result.token = TOKEN_LOGIC_AND;
            result.lexeme.push_back( ch );
            break;
        case '|':
            // no unary | operator
            if ( ( ch = getc( fp ) ) != '|' ) error( "bitwise OR not supported in GoLF", result.linenum );

            result.token = TOKEN_LOGIC_OR;
            result.lexeme.push_back( ch );
            break;
        case '=':
            // if '=' is next, then it's == comparison, else =
            if ( ( ch = getc( fp ) ) == '=' ) {
              result.token = TOKEN_EQ;
              result.lexeme.push_back( ch );
            } else {
              result.token = TOKEN_ASSIGN;
              ungetc( ch, fp );
            }
            break;
        case '<':
            // if '=' is next, then it's <= comparison, else =
            if ( ( ch = getc( fp ) ) == '=' ) {
              result.token = TOKEN_LEQ;
              result.lexeme.push_back( ch );
            } else {
              result.token = TOKEN_LT;
              ungetc( ch, fp );
            }
            break;
        case '>':
            // if '=' is next, then it's >= comparison, else =
            if ( ( ch = getc( fp ) ) == '=' ) {
              result.token = TOKEN_GEQ;
              result.lexeme.push_back( ch );
            } else {
              result.token = TOKEN_GT;
              ungetc( ch, fp );
            }
            break;
        case '!':
            // if '=' is next, then it's >= comparison, else =
            if ( ( ch = getc( fp ) ) == '=' ) {
              result.token = TOKEN_NEQ;
              result.lexeme.push_back( ch );
            } else {
              result.token = TOKEN_BANG;
              ungetc( ch, fp );
            }
            break;
        case '/':
            // single slash
            if ( ( ch = getc( fp ) ) != '/' ) {
                result.token = TOKEN_SLASH;
                ungetc( ch, fp );
                break;
            }

            // comment - spin until we hit newline or EOF then lex at that newline/EOF
            while ( ( ch = getc( fp ) ) != EOF && ch != '\n' )
                ;
            ungetc( ch, fp );
            goto spin;
        case '"': // beginning of string literal
            result.lexeme.clear(); // don't include the quote in the lexeme

            // spin until we hit a terminating quote
            while ( ( ch = getc( fp ) ) && ch != '"' ) {

                // error if newline or EOF encountered within string
                if ( ch == '\n' ) error( "string contains newline", result.linenum );
                if ( ch ==  EOF ) error( "string terminated by EOF", result.linenum );

                // on backslash: check for valid escape sequences
                if ( ch == '\\' ) {
                    result.lexeme += ch;
                    ch = getc( fp );

                    // error if newline or EOF encountered within string
                    if ( ch == '\n' ) error( "string contains newline", result.linenum );
                    if ( ch ==  EOF ) error( "string terminated by EOF", result.linenum );

                    if ( ch != 'b' && ch != 'f' &&ch != 'n' &&ch != 'r' &&ch != 't' &&ch != '\\' &&ch != '"' )
                        bad_char_error( "bad string escape", ch, result.linenum );
                }
                result.lexeme += ch;
            }

            // add semicolon if this thing is the last thing on the line or in file
            if ( ( ch = getc( fp ) ) == '\n' )
                unlex( Tokinfo{ TOKEN_SEMICOLON, result.linenum, "", } );

            result.token = TOKEN_STRING;
            ungetc( ch, fp );
            break;
        default:
            // spin on whitespace
            if ( is_space( ch ) ) goto spin;

            // number means integer literal
            if ( isdigit( ch ) ) {
                // spin until we hit a non-num
                while ( isdigit( ( ch = getc( fp ) ) ) )
                    result.lexeme += ch;

                // add semicolon if this thing is the last thing on the line or in file
                if ( ch == '\n' ) unlex( Tokinfo { TOKEN_SEMICOLON, result.linenum, "", } );

                result.token = TOKEN_INT;
                ungetc( ch, fp );
                break;
            }

            // letter means identifier or reserved word
            if ( is_letter( ch ) ) {
                // spin until we hit a non-letter
                while ( ( ch = getc( fp ) ) != EOF && ( is_letter( ch ) || isdigit( ch ) ) )
                    result.lexeme += ch;

                // check if token is a reserved work and act accordingly
                if      ( result.lexeme == "break"  ) result.token = TOKEN_BREAK;
                else if ( result.lexeme == "else"   ) result.token = TOKEN_ELSE;
                else if ( result.lexeme == "for"    ) result.token = TOKEN_FOR;
                else if ( result.lexeme == "func"   ) result.token = TOKEN_FUNC;
                else if ( result.lexeme == "if"     ) result.token = TOKEN_IF;
                else if ( result.lexeme == "return" ) result.token = TOKEN_RETURN;
                else if ( result.lexeme == "var"    ) result.token = TOKEN_VAR;
                //otherwise, result is a generic identifier
                else                                  result.token = TOKEN_ID;

                // if end of line and token is break, return, or an identifier, insert a semicolon
                if ( ( ch == '\n' ) && ( result.token == TOKEN_BREAK || result.token == TOKEN_RETURN || result.token == TOKEN_ID ) )
                    unlex( Tokinfo { TOKEN_SEMICOLON, result.linenum, "", } );

                ungetc( ch, fp );
                break;
            }

            // skip non-ascii with a warning
            if ( ch > 127 ) {
                warning( "skipping non-ASCII input character", result.linenum );
                goto spin;
            }
            // skip unknown character with warning
            bad_char_warning( "skipping unknown character", ch, result.linenum );
            goto spin;
            break;
    }

    return result;
}

const char *token_to_string( TokenID token ) {
    switch ( token ) {
        case TOKEN_EOF:       return "EOF";
        case TOKEN_BREAK:     return "break";
        case TOKEN_ELSE:      return "else";
        case TOKEN_FOR:       return "for";
        case TOKEN_FUNC:      return "func";
        case TOKEN_IF:        return "if";
        case TOKEN_RETURN:    return "return";
        case TOKEN_VAR:       return "var";
        case TOKEN_PLUS:      return "+";
        case TOKEN_MINUS:     return "-";
        case TOKEN_STAR:      return "*";
        case TOKEN_SLASH:     return "/";
        case TOKEN_PERCENT:   return "%";
        case TOKEN_LOGIC_AND: return "&&";
        case TOKEN_LOGIC_OR:  return "||";
        case TOKEN_EQ:        return "==";
        case TOKEN_LT:        return "<";
        case TOKEN_GT:        return ">";
        case TOKEN_ASSIGN:    return "=";
        case TOKEN_BANG:      return "!";
        case TOKEN_NEQ:       return "!=";
        case TOKEN_LEQ:       return "<=";
        case TOKEN_GEQ:       return ">=";
        case TOKEN_LPAREN:    return "(";
        case TOKEN_RPAREN:    return ")";
        case TOKEN_LBRACE:    return "{";
        case TOKEN_RBRACE:    return "}";
        case TOKEN_COMMA:     return ",";
        case TOKEN_SEMICOLON: return ";";
        case TOKEN_ID:        return "id";
        case TOKEN_INT:       return "int";
        case TOKEN_STRING:    return "string";
        case TOKEN_UNSET:     return "DEBUG";
    }
}
