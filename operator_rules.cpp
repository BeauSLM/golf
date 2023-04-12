#include "ast.hpp"
#include "operator_rules.hpp"

#include <string>

// table that encodes the legal operand types of each operator,
// as well as the type of the expression yielded by the operator
OpRule operator_rules[ NUM_OPS ] = {
    // arithmetic ops
    { AST_PLUS,      "int",    "int",    "int"  },
    { AST_MINUS,     "int",    "int",    "int"  },
    { AST_MUL,       "int",    "int",    "int"  },
    { AST_DIV,       "int",    "int",    "int"  },
    { AST_MOD,       "int",    "int",    "int"  },

    // ordering ops
    { AST_LT,        "int",    "int",    "bool" },
    { AST_LT,        "string", "string", "bool" },
    { AST_GT,        "int",    "int",    "bool" },
    { AST_GT,        "string", "string", "bool" },
    { AST_LEQ,       "int",    "int",    "bool" },
    { AST_LEQ,       "string", "string", "bool" },
    { AST_GEQ,       "int",    "int",    "bool" },
    { AST_GEQ,       "string", "string", "bool" },

    // equality ops
    { AST_EQ,        "int",    "int",    "bool" },
    { AST_EQ,        "bool",   "bool",   "bool" },
    { AST_EQ,        "string", "string", "bool" },
    { AST_NEQ,       "int",    "int",    "bool" },
    { AST_NEQ,       "bool",   "bool",   "bool" },
    { AST_NEQ,       "string", "string", "bool" },

    // binary ops
    { AST_LOGIC_AND, "bool",   "bool",   "bool" },
    { AST_LOGIC_OR,  "bool",   "bool",   "bool" },

    // assignment
    { AST_ASSIGN,    "int",    "int",    "void" },
    { AST_ASSIGN,    "bool",   "bool",   "void" },
    { AST_ASSIGN,    "string", "string", "void" },

    // unary operators
    { AST_UMINUS,    "int",    "",       "int"  },
    { AST_LOGIC_NOT, "bool",   "",       "bool" },
};
