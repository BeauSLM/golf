#pragma once

#include "codegen.hpp"
#include <cassert>

// XXX: why, just why
#define NUM_OPS 26

inline std::string default_instruction_gen
( std::string instr, std::string destreg, std::string reg1, std::string reg2 )
{
    std::string result = instr + " " + destreg + ", " + reg1;

    if ( reg2.size() > 0 )
    {
        result += ", " + reg2;
        if ( reg2[0] == '$' ) 
        {
            assert( reg2.size() > 0 );
            freereg( reg2 );
        }
    }

    assert( reg1.size() > 0 );
    freereg( reg1 );

    return result;
}

struct OpRule {
    ASTNodeID operatorid;
    std::string lhs, rhs, expressiontype, instr;

    std::string ( *geninstruction )( std::string, std::string, std::string, std::string )
        = &default_instruction_gen;
};
