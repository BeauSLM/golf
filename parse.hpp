#pragma once

#include <stdio.h>
#include <vector>

struct ASTNode {
    // TODO: members
    // - nodetype
    // - attribute (id, nums, etc)
    int linenum;
    std::vector<ASTNode> children;
};

ASTNode parse( FILE *fp );
