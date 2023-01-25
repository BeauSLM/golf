#include <stdio.h>
#include <string>

enum Token {
    // TODO: fill
    TOKEN_EOF = 0,
    TOKEN_ID,

    // for my own debugging purposes
    TOKEN_UNSET,
};

struct Tokinfo {
    Token token = TOKEN_UNSET;

    // attributes
    int linenum = 0;
    std::string lexeme;
};

Tokinfo lex(FILE *fp);
