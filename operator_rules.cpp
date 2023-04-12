#include "codegen.hpp"
#include "ast.hpp"
#include "operator_rules.hpp"

#include <string>

inline std::string logic_not
( std::string instr, std::string destreg, std::string reg1, std::string reg2 )
{
    // silence unused warning
    instr.clear(); reg2.clear();
    return default_instruction_gen( "xori", destreg, reg1, "1" );
}

inline std::string u_minus
( std::string instr, std::string destreg, std::string reg1, std::string reg2 )
{
    // silence unused warning
    instr.clear(); reg2.clear();
    return default_instruction_gen( "negu", destreg, reg1, "" );
}

inline void divmodprologue( std::string reg1, std::string reg2 )
{
    emitinstruction( "move $a0, " + reg1 );
    emitinstruction( "move $a1, " + reg2 );
    emitinstruction( "jal divmodcheck" );
    emitinstruction( "move " + reg2 + ", $v0" );
    emitinstruction( "div " + reg1 + ", " + reg2 );
}

inline std::string div
( std::string instr, std::string destreg, std::string reg1, std::string reg2 )
{
    // silence unused warning
    instr.clear();

    divmodprologue( reg1, reg2 );
    return "mflo " + destreg;
}

inline std::string mod
( std::string instr, std::string destreg, std::string reg1, std::string reg2 )
{
    // silence unused warning
    instr.clear();

    divmodprologue( reg1, reg2 );
    return "mfhi " + destreg;
}

// table that encodes the legal operand types of each operator,
// as well as the type of the expression yielded by the operator
OpRule operator_rules[ NUM_OPS ] = {
    // arithmetic ops
    { AST_PLUS,      "int",    "int",    "int",  "addu",                 }, // done
    { AST_MINUS,     "int",    "int",    "int",  "subu",                 }, // done
    { AST_MUL,       "int",    "int",    "int",  "mul",                  },
    { AST_DIV,       "int",    "int",    "int",  "div", &div             }, // TODO: call div by zero check
    { AST_MOD,       "int",    "int",    "int",  "div", &mod             }, // TODO: call div by zero check

    // ordering ops
    { AST_LT,        "int",    "int",    "bool", "slt",                  }, // done
    { AST_LT,        "string", "string", "bool", "slt",                  }, // TODO: call strcmp
    { AST_GT,        "int",    "int",    "bool", "sgt",                  }, // done
    { AST_GT,        "string", "string", "bool", "sgt",                  }, // TODO: call strcmp
    { AST_LEQ,       "int",    "int",    "bool", "sle",                  }, // done
    { AST_LEQ,       "string", "string", "bool", "sle",                  }, // TODO: call strcmp
    { AST_GEQ,       "int",    "int",    "bool", "sge",                  }, // done
    { AST_GEQ,       "string", "string", "bool", "sge",                  }, // TODO: call strcmp

    // equality ops
    { AST_EQ,        "int",    "int",    "bool", "seq",                  }, // done
    { AST_EQ,        "bool",   "bool",   "bool", "seq",                  }, // done
    { AST_EQ,        "string", "string", "bool", "seq",                  }, // TODO: strcmp
    { AST_NEQ,       "int",    "int",    "bool", "sne",                  }, // done
    { AST_NEQ,       "bool",   "bool",   "bool", "sne",                  }, // done
    { AST_NEQ,       "string", "string", "bool", "sne",                  }, // TODO: strcmp

    // binary ops
    // NOTE: I won't use this for codegen cause its necessary to prune the traversal
    // of the expressions
    { AST_LOGIC_AND, "bool",   "bool",   "bool", "",     nullptr         },
    { AST_LOGIC_OR,  "bool",   "bool",   "bool", "",     nullptr         },

    // assignment
    // NOTE: I won't use this for codegen cause its necessary to prune the traversal
    // of the identifier being assigned to so I have to put it pre
    { AST_ASSIGN,    "int",    "int",    "void", "",     nullptr         },
    { AST_ASSIGN,    "bool",   "bool",   "void", "",     nullptr         },
    { AST_ASSIGN,    "string", "string", "void", "",     nullptr         },

    // unary operators
    { AST_UMINUS,    "int",    "",       "int",  "negu", &u_minus        },
    { AST_LOGIC_NOT, "bool",   "",       "bool", "xori", &logic_not      },
};
