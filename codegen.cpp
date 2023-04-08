#include "codegen.hpp"
#include "error.hpp"
#include <cassert>

static std::string output;

inline void emitinstruction
( std::string instruction )
{
    std::string result = '\t' + instruction + '\n';
    output += result;
}

inline std::string emitlabel
( std::string prefix, std::string label )
{
    std::string result = prefix + label + ":\n";
    output += result;
    return result;
}

void global_vars( ASTNode & node )
{
    if ( node.type == AST_GLOBVAR )
    {
        std::string type = node.symbolinfo->signature;

        emitinstruction( ".data" );
        std::string label = emitlabel( "G", node[ 0 ].lexeme );

        if ( type == "int" || type == "bool" )
            emitinstruction( ".word 0" );

        else
        {
            assert( type == "string" );

            // TODO: emit instruction ".word " + label of string constant
            emitinstruction( ".word TODO" );
        }

        emitinstruction( ".text" );
    }
}

void gen_code( ASTNode & root )
{
    // EMIT PROLOGUE
    // - set up stuff and go to main

    // pass 1 - code generation
    // prepost for everything else
    // funccall:
    //  - function prologue
    //  - jump and return address thing to function
    //  - function epilogue

    // - flush output

    // pass 2 - statically allocated things
    // preorder to emit global var declarations
    // - AST_GLOBVAR
    //    - a label plus either a word or a string constant
    // - AST_FUNC ( ????? )

    preorder( root, global_vars );

    printf( "%s", output.data() );
}
