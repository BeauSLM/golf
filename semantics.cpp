#include "semantics.hpp"
#include "symboltable.hpp"
#include "error.hpp"

static const struct {
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

        for ( const auto &symbol : universe )
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
        static bool main_is_defined = false;
        auto define_globals = +[]( ASTNode & node )
        {
            if ( node.type != AST_GLOBVAR && node.type != AST_FUNC ) return;

            // identifier (name) of the symbol
            ASTNode &ident = node.children[ 0 ];

            // functions and variables are not constants or types
            // REVIEW: which linenum should the record get???
            STabRecord *record = define( ident.lexeme, ident.linenum );
            record->isconst    = record->istype = false;

            if ( node.type == AST_FUNC && node.children[ 0 ].lexeme == "main" )
                main_is_defined = true;

            // OPTIMIZE: global declarations can't have children that are global declarations (I think)
            // this means I can simply prune the traversal of the children of this node
        };

        preorder( root, define_globals );

        if ( !main_is_defined )
            error( -1, "missing main() function" );
    }

    // pass 2: fully populate global symbol table records and annotate all identifier
    // notes with their corresponding symbol table records
    {
        // TODO:
        auto pass_2_pre = +[]( ASTNode &node )
        {
            // TODO:
            switch ( node.type ) {
            switch ( node.type )
            {
                case AST_TYPEID:
                {
                    node.symbolinfo = lookup( node.lexeme, node.linenum );

                    if ( !node.symbolinfo->istype ) error( node.linenum, "expected type, got '%s'", node.lexeme.data() );

                    break;
                }
                case AST_GLOBVAR:
                {
                    // verify that the type is valid
                    ASTNode &type = node.children[ 1 ];

                    // get the name's symbol record and give it it's type
                    // NOTE: we defined all global symbols in pass 1
                    ASTNode &varname  = node.children[ 0 ];
                    auto record       = lookup( varname.lexeme, varname.linenum );
                    record->signature = type.lexeme;

                    node.symbolinfo   = record;

                    break;
                }
                default:
                    break;
                    // error( node.linenum, "TODO" );
            }
        };

        auto pass_2_post = +[]( ASTNode &node )
        {
            // TODO:
            // if ( node.type == AST_BLOCK ) closescope();
        };

        prepost( root, pass_2_pre, pass_2_post );
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

