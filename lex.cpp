#include "lex.hpp"
#include "error.hpp"

bool is_space(int c) { return c == '\n' || c == '\t' || c == ' ' || c == '\r'; }

Tokinfo lex(FILE *fp) {
    // TODO: other attribute information (col number etc)?
    int ch, linenum = 1;
    Tokinfo result;

    // spin until we find something interesting
    while ( ( ch = getc(fp) ) != EOF && is_space(ch) )
        if (ch == '\n') linenum++;

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
