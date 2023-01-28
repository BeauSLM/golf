#include <assert.h>
#include "lex.hpp"
#include "error.hpp"
#include <queue>

bool is_space(int c) { return c == '\n' || c == '\t' || c == ' ' || c == '\r'; }

bool is_letter(int c) { return isalpha(c) || c == '_'; }

std::queue<Tokinfo> tokens;

void unlex(Tokinfo t) { tokens.push(t); }

Tokinfo lex(FILE *fp) {

    // static to preserve value across calls.
    // this allows me to inspect result's token before I set it in a given call
    // to check what the previous token was
    static Tokinfo result;

    // prioritize tokens in the queue
    if (!tokens.empty()) {
        result = tokens.front();
        tokens.pop();
        return result;
    }

    // put result's members in local scope
    // NOTE: these values are those of the last token returned, until we overwrite them
    auto & result_token   = result.token;
    auto & result_linenum = result.linenum;
    auto & result_lexeme  = result.lexeme;

    // CITATION-ish: the following code draws heavily from the FSM example given to us
    // by Dr. Aycock in class -
    // linked here -https://d2l.ucalgary.ca/d2l/le/content/500748/viewContent/5744437/View
    spin:
    int ch;
    // spin until we find something interesting
    while ( ( ch = getc(fp) ) != EOF && is_space(ch) )
        if (ch == '\n') result_linenum++;

    result.lexeme  = ch; // first (and possibly only) character of the lexeme

    switch (ch) {
        // NULL character isn't allowed in input
        case NULL:
            warning("skipping NULL character", result_linenum);
        case EOF:
            if (   result_token == TOKEN_ID
                || result_token == TOKEN_INT
                || result_token == TOKEN_STRING
                || result_token == TOKEN_BREAK
                || result_token == TOKEN_RETURN
                || result_token == TOKEN_RBRACE
                || result_token == TOKEN_RPAREN
            ) {
                result_token  = TOKEN_SEMICOLON;
                result_lexeme = "";
                ungetc(ch, fp);
                break;
            }
            result_token = TOKEN_EOF;
            break;
        case '+':
            result_token = TOKEN_PLUS;
            break;
        case '-':
            result_token = TOKEN_MINUS;
            break;
        case '*':
            result_token = TOKEN_STAR;
            break;
        case '%':
            result_token = TOKEN_PERCENT;
            break;
        case '(':
            result_token = TOKEN_LPAREN;
            break;
        case ')':
            result_token = TOKEN_RPAREN;

            // peek next char to see if its a newline. if so, insert a semicolon
            if ( ( ch = getc(fp) ) == '\n' )
                unlex(Tokinfo {TOKEN_SEMICOLON, result_linenum, "",});

            // put back to mimick "peeking"
            ungetc(ch, fp);
            break;
        case '{':
            result_token = TOKEN_LBRACE;
            break;
        case '}':
            // if last token wasn't a semicolon, insert a semicolon into the token stream
            if (result_token != TOKEN_SEMICOLON && result_token != TOKEN_UNSET) {
                result_token  = TOKEN_SEMICOLON;
                result_lexeme = "";
                ungetc(ch, fp);
                return result;
            }

            result_token = TOKEN_RBRACE;

            // peek next char to see if its a newline. if so, insert a semicolon
            if ( ( ch = getc(fp) ) == '\n' )
                unlex(Tokinfo {TOKEN_SEMICOLON, result_linenum, "",});

            // put back to mimick "peeking"
            ungetc(ch, fp);
            break;
        case ',':
            result_token = TOKEN_COMMA;
            break;
        case ';':
            result_token = TOKEN_SEMICOLON;
            break;
        case '&':
            // no unary & operator
            if ( (ch = getc(fp)) != '&' ) error("bitwise AND not supported in GoLF", result_linenum);
            result_token = TOKEN_LOGIC_AND;
            result_lexeme.push_back(ch);
            break;
        case '|':
            // no unary | operator
            if ( (ch = getc(fp)) != '|' ) error("bitwise OR not supported in GoLF", result_linenum);
            result_token = TOKEN_LOGIC_OR;
            result_lexeme.push_back(ch);
            break;
        case '=':
            // if '=' is next, then it's == comparison, else =
            if ((ch = getc(fp)) == '=') {
              result_token = TOKEN_EQ;
              result_lexeme.push_back(ch);
            } else {
              result_token = TOKEN_ASSIGN;
              ungetc(ch, fp);
            }
            break;
        case '<':
            // if '=' is next, then it's <= comparison, else =
            if ((ch = getc(fp)) == '=') {
              result_token = TOKEN_LEQ;
              result_lexeme.push_back(ch);
            } else {
              result_token = TOKEN_LT;
              ungetc(ch, fp);
            }
            break;
        case '>':
            // if '=' is next, then it's >= comparison, else =
            if ((ch = getc(fp)) == '=') {
              result_token = TOKEN_GEQ;
              result_lexeme.push_back(ch);
            } else {
              result_token = TOKEN_GT;
              ungetc(ch, fp);
            }
            break;
        case '!':
            // if '=' is next, then it's >= comparison, else =
            if ((ch = getc(fp)) == '=') {
              result_token = TOKEN_NEQ;
              result_lexeme.push_back(ch);
            } else {
              result_token = TOKEN_BANG;
              ungetc(ch, fp);
            }
            break;
        case '/':
            // REVIEW: maybe make this look better?
            // single slash
            if ( ( ch = getc(fp) ) != '/' ) {
                result_token = TOKEN_SLASH;
                ungetc(ch, fp);
                break;
            }
            // comment - spin until we hit newline or EOF then lex at that newline/EOF
            while ( ( ch = getc(fp) ) != EOF && ch != '\n' )
                ;
            ungetc(ch, fp);
            goto spin;
        case '"': // beginning of string literal
            // TODO: cleanup
            result_lexeme.clear(); // don't include the quote in the lexeme
            // spin until we hit a terminating quote
            while ( ( ch = getc(fp) ) && ch != '"' ) {

                // error if newline or EOF encountered within string
                if ( ch == '\n' ) error("string contains newline", result_linenum);
                if ( ch == EOF  ) error("string terminated by EOF", result_linenum);

                // on backslash: check for valid escape sequences
                if (ch == '\\') {
                    result_lexeme += ch;
                    ch = getc(fp);

                    // error if newline or EOF encountered within string
                    if ( ch == '\n' ) error("string contains newline", result_linenum);
                    if ( ch == EOF  ) error("string terminated by EOF", result_linenum);

                    if ( ch != 'b' && ch != 'f' &&ch != 'n' &&ch != 'r' &&ch != 't' &&ch != '\\' &&ch != '"' )
                        bad_char_error("bad string escape", ch, result_linenum);
                }
                result_lexeme += ch;
            }

            // add semicolon if this thing is the last thing on the line or in file
            if (( ch = getc(fp) ) == '\n' )
                unlex(Tokinfo{TOKEN_SEMICOLON, result_linenum, "",});

            result_token = TOKEN_STRING;
            ungetc(ch, fp);
            break;
        default:
            // spin on whitespace
            if (is_space(ch)) goto spin;

            // number means integer literal
            if (isdigit(ch)) {
                // spin until we hit a non-num
                while ( isdigit( ( ch = getc(fp) ) ) )
                    result_lexeme += ch;

                // add semicolon if this thing is the last thing on the line or in file
                if (ch == '\n' ) unlex(Tokinfo {TOKEN_SEMICOLON, result_linenum, "",});

                result_token = TOKEN_INT;
                ungetc(ch, fp);
                break;
            }

            // letter means identifier or reserved word
            if (is_letter(ch)) {
                // spin until we hit a non-letter
                while ( ( ch = getc(fp) ) != EOF && ( is_letter(ch) || isdigit(ch)) )
                    result_lexeme += ch;

                // check if token is a reserved work and act accordingly
                if      (result_lexeme == "break")  result_token = TOKEN_BREAK;
                else if (result_lexeme == "else")   result_token = TOKEN_ELSE;
                else if (result_lexeme == "for")    result_token = TOKEN_FOR;
                else if (result_lexeme == "func")   result_token = TOKEN_FUNC;
                else if (result_lexeme == "if")     result_token = TOKEN_IF;
                else if (result_lexeme == "return") result_token = TOKEN_RETURN;
                else if (result_lexeme == "var")    result_token = TOKEN_VAR;
                //otherwise, result is a generic identifier
                else                                result_token = TOKEN_ID;

                // if end of line and token is break, return, or an identifier, insert a semicolon
                if ( ( ch == '\n' ) && ( result_token == TOKEN_BREAK || result_token == TOKEN_RETURN || result_token == TOKEN_ID ) )
                    unlex(Tokinfo {TOKEN_SEMICOLON, result_linenum, "",});

                ungetc(ch, fp);
                break;
            }

            // skip non-ascii with a warning
            if (ch > 127) {
                warning("skipping non-ASCII input character", result_linenum);
                goto spin;
            }
            // skip unknown character with warning
            bad_char_warning("skipping unknown character", ch, result_linenum);
            goto spin;
            break;
    }

    return result;
}

const char *token_to_string(TokenID token) {
    switch (token) {
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
