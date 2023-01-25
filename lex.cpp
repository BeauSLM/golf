#include "lex.hpp"
#include "error.hpp"


struct Tokinfo lex(FILE *fp) {
    int ch, linenum = 0;

    while ( ( ch = getc(fp) ) ) {
        if (ch == EOF) {
            return (struct Tokinfo) { .token = TOKEN_EOF, .linenum = linenum };
        }
    }

    return (struct Tokinfo) {};
}
