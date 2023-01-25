#include "lex.hpp"
#include "error.hpp"


Tokinfo lex(FILE *fp) {
    // TODO: other attribute information (col number etc)?
    int ch, linenum = 1;
    Tokinfo result;

    // spin until we find something interesting
    while ( ( ch = getc(fp) ) != EOF && isspace(ch) )
        if (ch == '\n') linenum++;

    switch (ch) {
        case EOF:
            return Tokinfo { .token = TOKEN_EOF, .linenum = linenum };
        default:
            break;
    }

    return result;
}
