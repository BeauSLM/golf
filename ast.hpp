#pragma once

#include <stdio.h>
#include <vector>

#include "lex.hpp"
#include "symboltable.hpp"

enum ASTNodeID {
    // "program structure" or someth idk
    AST_PROGRAM,
    AST_SIGNATURE,
    AST_FORMAL,
    AST_FORMALS,
    AST_BLOCK,
    AST_ACTUALS,
    AST_FUNCCALL,

    // statements
    AST_EXPRSTMT,
    AST_EMPTYSTMT,

    // keywords
    AST_BREAK,
    AST_FOR,
    AST_FUNC,
    AST_IF,
    AST_IFELSE,
    AST_RETURN,
    AST_VAR,
    AST_GLOBVAR,

    // operators
    AST_PLUS,
    AST_MINUS,
    AST_UMINUS,
    AST_MUL,
    AST_DIV,
    AST_MOD,
    AST_ASSIGN, // =
    AST_LOGIC_AND,
    AST_LOGIC_OR,
    AST_EQ,     // ==
    AST_LT,
    AST_GT,
    AST_LOGIC_NOT,
    AST_NEQ,
    AST_LEQ,    // <=
    AST_GEQ,    // >=

    // identifiers
    AST_ID,
    AST_NEWID,
    AST_TYPEID,

    // literals
    AST_INT,
    AST_STRING,

    AST_UNSET,
};

struct ASTNode {
    ASTNodeID type = AST_UNSET;
    int linenum    = -1;

    std::string lexeme;
    std::vector<ASTNode> children;

    STabRecord * symbolinfo = nullptr;
    std::string expressiontype;

    ASTNode( ASTNodeID type = AST_UNSET, int linenum = -1, std::string lexeme = "" )
        : type( type ), linenum( linenum ), lexeme( lexeme )
        {}

    inline void add_child   ( ASTNode kid               ) { children.push_back(kid); }
    inline void set_children( std::vector<ASTNode> kids ) { children = kids;         }

    ASTNode & operator[]( size_t i ) { return children[ i ]; }
};

// traversal routines
void preorder
( ASTNode & root, void ( *callback    )( ASTNode & ) );

void postorder
( ASTNode & root, void ( *callback    )( ASTNode & ) );

void prepost
( ASTNode & root, void ( *precallback )( ASTNode & ), void ( *postcallback )( ASTNode & ) );

// printing utilities
std::string ASTNode_to_string( ASTNodeID n );
std::string ASTNode_printstring( ASTNode &n );
void printast( ASTNode & root );
