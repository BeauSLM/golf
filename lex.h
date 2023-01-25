#include <stdio.h>

enum Token {
    // TODO: fill
    TOKEN_EOF = 0,
    TOKEN_ID,

    // for my own debugging purposes
    TOKEN_UNSET,
};

struct Tokinfo {
    enum Token token;

    // attributes
    int linenum      = 0;
    char *lexeme     = NULL;
};

struct Tokinfo lex(FILE *fp);
