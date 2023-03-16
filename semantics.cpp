#include "semantics.hpp"
#include "symboltable.hpp"
#include "error.hpp"

#include <limits.h>

static const struct {
    std::string name,
                signature,
                returnsignature;
    bool        isconst = false,
                istype  = false;
} universe[] = {
    { "$void",   "void",      "",     false, true,  },
    { "bool",    "bool",      "",     false, true,  },
    { "int",     "int",       "",     false, true,  },
    { "string",  "string",    "",     false, true,  },
    { "$true",   "bool",      "",     true,  false, },
    { "true",    "bool",      "",     true,  false, },
    { "false",   "bool",      "",     true,  false, },
    { "printb",  "f(bool)",   "void", false, false, },
    { "printc",  "f(int)",    "void", false, false, },
    { "printi",  "f(int)",    "void", false, false, },
    { "prints",  "f(string)", "void", false, false, },
    { "getchar", "f()",       "int",  false, false, },
    { "halt",    "f()",       "void", false, false, },
    { "len",     "f(string)", "int",  false, false, },
};

bool check_child_type( int child_ix, std::string type, ASTNode & node )
{
    return node.children[ child_ix ].expressiontype == type;
}

void check_operand_types( std::string type, ASTNode & node )
{
    if ( !check_child_type( 0, type, node ) || ( node.children.size() > 1 && !check_child_type( 1, type, node ) ) )
        error( node.linenum, "operand type mismatch for '%s'", ASTNode_to_string( node.type ).data() );
}

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

            // defining main:
            // error if it has arguments
            // error if it has a return type
            if ( node.type == AST_FUNC && node.children[ 0 ].lexeme == "main" )
            {
                auto &arguments = node.children[ 1 ].children[ 0 ].children;
                if ( arguments.size() > 0 )
                    error( node.linenum, "main() can't have arguments");

                auto &returntype = node.children[ 1 ].children[ 1 ].lexeme;
                if ( returntype != "$void" )
                    error( node.linenum, "main() can't have a return value" );

                main_is_defined = true;
            }

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
        // function parameters that we defer defining until we have opened the function's scope
        static std::vector<ASTNode *> funcparams;
        auto pass_2_pre = +[]( ASTNode &node )
        {
            switch ( node.type )
            {
                case AST_PROGRAM:
                {
                    node.symbolinfo = lookup( "main", -1 );
                }
                case AST_BLOCK:
                {
                    openscope();

                    for ( auto param : funcparams )
                    {
                        ASTNode & name = param->children[ 0 ];
                        ASTNode & type = param->children[ 1 ];

                        name.symbolinfo = define( name.lexeme, name.linenum );
                        type.symbolinfo = lookup( type.lexeme, type.linenum );

                        name.symbolinfo->signature = type.lexeme;
                    }

                    funcparams.clear();
                    break;
                }
                // REVIEW: this can be done pre or post because we can't define
                // custom types in this language
                case AST_TYPEID:
                {
                    node.symbolinfo = lookup( node.lexeme, node.linenum );

                    if ( !node.symbolinfo->istype ) error( node.linenum, "expected type, got '%s'", node.lexeme.data() );

                    break;
                }
                case AST_GLOBVAR:
                {
                    ASTNode &type    = node.children[ 1 ];
                    ASTNode &varname = node.children[ 0 ];

                    node.symbolinfo            = lookup( varname.lexeme, varname.linenum );
                    node.symbolinfo->signature = type.lexeme;

                    break;
                }
                case AST_FUNC:
                {
                    std::vector<ASTNode> & children = node.children;

                    ASTNode &funcname = children[ 0 ];
                    auto record       = lookup ( funcname.lexeme, funcname.linenum );

                    ASTNode &returntype = children[ 1 ].children[ 1 ];

                    record->returnsignature = returntype.lexeme;

                    record->signature = "f(";

                    // TODO: put all parameters in the symbol table

                    std::vector<ASTNode> & params = children[ 1 ].children[ 0 ].children;
                    for ( auto & param : params )
                    {
                        auto & typestring = param.children[ 1 ].lexeme;

                        record->signature += typestring;
                        record->signature += ",";

                        funcparams.push_back( &param );
                    }

                    // remove trailing "," that I added
                    if ( record->signature.size() > 2 ) record->signature.pop_back();

                    record->signature += ")";

                    break;
                }
                default:
                    break;
                    // error( node.linenum, "TODO" );
            }
        };

        auto pass_2_post = +[]( ASTNode &node )
        {
            switch ( node.type )
            {
                case AST_BLOCK:
                {
                    closescope();
                    break;
                }
                case AST_ID:
                {
                    node.symbolinfo = lookup( node.lexeme, node.linenum  );
                    break;
                }
                case AST_VAR:
                {
                    ASTNode &varname = node.children[ 0 ];
                    ASTNode &type    = node.children[ 1 ];

                    node.symbolinfo = define( varname.lexeme, varname.linenum );
                    node.symbolinfo->signature = type.lexeme;

                    break;
                }
                default:
                    break;
            }
        };

        prepost( root, pass_2_pre, pass_2_post );
    }

    // pass 3: propagate type information up the AST, starting at the leaves
    {
        auto pass_3 = +[]( ASTNode &node )
        {
            switch ( node.type )
            {
                case AST_INT:
                {
                    if ( node.lexeme.size() > 12 )
                        error( node.linenum, "integer literal out of range", node.lexeme.data() );
                    int64_t value = std::stoll( node.lexeme );
                    if ( value < INT_MIN )
                        error( node.linenum, "integer literal too small", node.lexeme.data() );

                    if ( value > INT_MAX )
                        error( node.linenum, "integer literal too large", node.lexeme.data() );

                    node.expressiontype = "int";
                }
                break;
                case AST_STRING:
                    node.expressiontype = "string";
                    break;
                case AST_PLUS:
                case AST_MINUS:
                case AST_MUL:
                case AST_DIV:
                case AST_MOD:
                    if ( node.children[ 0 ].expressiontype != "int" || node.children[ 1 ].expressiontype != "int" )
                        error( node.linenum, "type mismatch for '%s'", ASTNode_to_string( node.type ).data() );
                    node.expressiontype = "int";
                    break;
                case AST_UMINUS:
                    if ( node.children[ 0 ].expressiontype != "int" )
                        error( node.linenum, "type mismatch for '%s'", ASTNode_to_string( node.type ).data() );
                    node.expressiontype = "int";
                    break;
                case AST_ASSIGN:
                    node.expressiontype = "void";
                    break;
                case AST_LOGIC_AND:
                    node.expressiontype = "bool";
                    break;
                case AST_LOGIC_OR:
                    node.expressiontype = "bool";
                    break;
                case AST_EQ:
                    node.expressiontype = "bool";
                    break;
                case AST_LT:
                    node.expressiontype = "bool";
                    break;
                case AST_GT:
                    node.expressiontype = "bool";
                    break;
                case AST_LOGIC_NOT:
                    node.expressiontype = "bool";
                    break;
                case AST_NEQ:
                    node.expressiontype = "bool";
                    break;
                case AST_LEQ:
                    node.expressiontype = "bool";
                    break;
                case AST_GEQ:
                    node.expressiontype = "bool";
                    break;
                case AST_FUNCCALL:
                    node.expressiontype = node.symbolinfo->returnsignature;
                    node.children[ 0 ].expressiontype = node.symbolinfo->signature;
                    // number and type of args matches signature
                    break;
                case AST_ID:
                    node.expressiontype = node.symbolinfo->signature;
                    break;
                // TODO: for if, ifelse, and for, make sure the expression is a boolean
                case AST_IF:
                    break;
                case AST_IFELSE:
                    break;
                case AST_FOR:
                    break;
                default:
                    break;
            }
        };
        postorder( root, pass_3 );
    }
}

