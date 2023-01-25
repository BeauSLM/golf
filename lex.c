#include "lex.h"
#include "error.h"


struct Tokinfo lex(FILE *fp) {
    int ch, linenum = 0;

    while ( ( ch = getc(fp) ) ) {
        if (ch == EOF) {
            return (struct Tokinfo) { .token = TOKEN_EOF, .linenum = linenum };
        }
    }

    return (struct Tokinfo) {};
}
