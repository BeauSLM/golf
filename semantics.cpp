#include "semantics.hpp"
#include "symboltable.hpp"
#include "error.hpp"

#include <assert.h>

const struct {
    std::string name,
                signature,
                returnsignature;
    bool        isconst = false,
                istype  = false;
} universe[] = {
    { "$void",   "void",    "",     false, true,  },
    { "bool",    "bool",    "",     false, true,  },
    { "int",     "int",     "",     false, true,  },
    { "string",  "str",     "",     false, true,  },
    { "$true",   "bool",    "",     true,  false, },
    { "true",    "bool",    "",     true,  false, },
    { "false",   "bool",    "",     true,  false, },
    { "printb",  "f(bool)", "void", false, false, },
    { "printc",  "f(int)",  "void", false, false, },
    { "printi",  "f(int)",  "void", false, false, },
    { "prints",  "f(str)",  "void", false, false, },
    { "getchar", "f()",     "int",  false, false, },
    { "halt",    "f()",     "void", false, false, },
    { "len",     "f(str)",  "int",  false, false, },
};

// NOTE: only called once
void checksemantics
( ASTNode & root )
{
    // populate universe block with predefined symbols
    openscope(); // universe block
    {
        extern std::vector<STab> scopestack;
        scopestack.clear();

        for ( auto &symbol : universe )
        {
            auto record = new STabRecord
            {
                symbol.signature,
                symbol.returnsignature,
                symbol.isconst,
                symbol.istype
            };

            // NOTE: not using define() because I know the information of the predefined symbols
            scopestack.back()[ symbol.name ] = record;
        }
    }

    // pass 1: populate file block
    openscope(); // file block
    {
        auto define_globals = +[]( ASTNode & node )
        {
            if ( node.type == AST_GLOBVAR || node.type == AST_FUNC )
            {
                // identifier (name) of the symbol
                auto ident = node.children[ 0 ];

                // make sure the node actually has the children it should
                // TODO: remove
                {
                    assert( node.children.size() > 0 );
                    assert( ident.type = AST_NEWID );
                }

                // REVIEW: which linenum???
                define( ident.lexeme, ident.linenum );
            }
        };

        preorder( root, define_globals );
    }

    // pass 2: fully populate global symbol table records and annotate all identifier
    // notes with their corresponding symbol table records
    {
        // TODO:
        auto foo = +[]( ASTNode &node )
        {
            // TODO:
            switch ( node.type ) {
                case AST_FUNC:

                default:
                    error( node.linenum, "TODO" );
            }
        };

        auto bar = +[]( ASTNode &node )
        {
            // TODO:
            if ( node.type == AST_BLOCK ) closescope();
        };

        prepost( root, foo, bar );
    }

#if 0
    // pass 3: propagate type information up the AST, starting at the leaves
    {
        auto foo = +[]( ASTNode &node )
        {
            // TODO:
            if ( node.type == AST_BLOCK ) openscope();
        };
        postorder( root, foo );
    }
#endif
}

