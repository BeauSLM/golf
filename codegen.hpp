#pragma once

#include "ast.hpp"

void gen_code( ASTNode & root );

std::string allocreg();

void freereg
( std::string reg );

void emitlabel
( std::string code );

void emitinstruction
( std::string instr );
