#pragma once

#include <stdio.h>
#include <vector>

#include "lex.hpp"

enum ASTNodeID {
    // TODO: populate
    AST_PROGRAM,
    AST_FUNCTION,
    AST_NEWID,
    AST_SIG,
    AST_FORMALS,
    AST_TYPEID,
    AST_BLOCK,
};

struct ASTNode {
    // TODO: figure out what this should be
    // do i not store a tokinfo and just have a string of some kind in here???
    // REVIEW: maybe i just need a token and its children????
    ASTNodeID type;
    Tokinfo tok;
    std::vector<ASTNode> children;
};

ASTNode parse( FILE *fp );
