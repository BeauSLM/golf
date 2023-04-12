#pragma once

// XXX: why, just why
#define NUM_OPS 26

struct OpRule {
    ASTNodeID operatorid;
    std::string lhs, rhs, expressiontype;
};
