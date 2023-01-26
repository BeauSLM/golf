#include <stdio.h>
#include <string>

enum Token {
    // TODO: fill
    TOKEN_EOF = 0,

    // keywords
    TOKEN_BREAK,
    TOKEN_ELSE,
    TOKEN_FOR,
    TOKEN_FUNC,
    TOKEN_IF,
    TOKEN_RETURN,
    TOKEN_VAR,

    // operators and punctuation
    TOKEN_PLUS,
    TOKEN_MINUS,
    TOKEN_STAR,
    TOKEN_SLASH,
    TOKEN_PERCENT,
    TOKEN_LOGIC_AND,
    TOKEN_LOGIC_OR,
    TOKEN_EQ,     // ==
    TOKEN_LT,
    TOKEN_GT,
    TOKEN_ASSIGN, // =
    TOKEN_BANG,
    TOKEN_NEQ,
    TOKEN_LEQ,    // <=
    TOKEN_GEQ,    // >=
    TOKEN_LPAREN,
    TOKEN_RPAREN,
    TOKEN_LBRACE,
    TOKEN_RBRACE,
    TOKEN_COMMA,
    TOKEN_SEMICOLON,

    // identifiers
    TOKEN_ID,

    // TODO: literals
    TOKEN_INT,
    TOKEN_STRING,

    // for my own debugging purposes
    TOKEN_UNSET,
};

struct Tokinfo {
    Token token = TOKEN_UNSET;

    // TODO: other attribute information (col number etc)?
    // attributes
    int linenum = 0;
    std::string lexeme;
};

Tokinfo lex(FILE *fp);
