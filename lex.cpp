#include "lex.hpp"
#include "error.hpp"

bool is_space(int c) { return c == '\n' || c == '\t' || c == ' ' || c == '\r'; }

bool is_letter(int c) { return isalpha(c) || c == '_'; }

Tokinfo lex(FILE *fp) {
    // init result and put its members in local scope
    Tokinfo result;
    auto & linenum = result.linenum;
    auto & token   = result.token;
    auto & lexeme  = result.lexeme;


    int ch;
    // spin until we find something interesting
    while ( ( ch = getc(fp) ) != EOF && is_space(ch) )
        if (ch == '\n') linenum++;

    lexeme = ch; // first (and possibly only) character of the lexeme

    switch (ch) {
        // NULL character isn't allowed in input
        case NULL:
            error("NULL character found - not permitted", linenum);
        case EOF:
            return Tokinfo { .token = TOKEN_EOF, .linenum = linenum };
        default:
            break;
    }

    return result;
}
