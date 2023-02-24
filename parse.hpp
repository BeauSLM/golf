#pragma once

#include <stdio.h>
#include <vector>
#include <optional>

#include "lex.hpp"

enum ASTNodeID {
    // TODO: populate
    AST_PROGRAM,
    AST_SIGNATURE,
    AST_FORMAL,
    AST_FORMALS,
    AST_BLOCK,
    AST_ACTUALS,
    AST_FUNCCALL,

    // keywords
    AST_BREAK,
    AST_ELSE,
    AST_FOR,
    AST_FUNC,
    AST_IF,
    AST_IFELSE,
    AST_RETURN,
    AST_VAR,
    AST_GLOBVAR,
    AST_EXPR,
    AST_EXPRSTMT,
    AST_EMPTYSTMT,

    // operators and punctuation
    AST_PLUS,
    AST_MINUS,
    AST_UMINUS,
    AST_STAR,
    AST_SLASH,
    AST_PERCENT,
    AST_LOGIC_AND,
    AST_LOGIC_OR,
    AST_EQ,     // ==
    AST_LT,
    AST_GT,
    AST_ASSIGN, // =
    AST_LOGIC_NOT,
    AST_NEQ,
    AST_LEQ,    // <=
    AST_GEQ,    // >=

    // identifiers
    AST_ID,
    AST_NEWID,
    AST_TYPEID,

    AST_INT,
    AST_STRING,

    AST_UNSET,
};

struct ASTNode {
    // REVIEW: has linenum and lexeme like Token, should I factor them out?
    ASTNodeID type = AST_UNSET;
    int linenum = -1;
    std::string lexeme;
    std::vector<ASTNode> children;

    ASTNode( ASTNodeID type = AST_UNSET, int linenum = -1, std::string lexeme = "" ): type(type), linenum(linenum), lexeme(lexeme) {}

    inline void add_child( ASTNode kid )                  { children.push_back(kid); }
    inline void set_children( std::vector<ASTNode> kids ) { children = kids; }
};

ASTNode parse();

std::string ASTNode_to_string( ASTNode n );
