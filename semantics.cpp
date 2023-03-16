// TODO: think long and hard abt where my symbol table pointers are gonna go
#include "semantics.hpp"
#include "symboltable.hpp"
#include "error.hpp"

#include <limits.h>
#include <string.h>

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
                        name.expressiontype = type.lexeme;
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
                    node.symbolinfo   = lookup ( funcname.lexeme, funcname.linenum );

                    ASTNode &returntype = children[ 1 ].children[ 1 ];

                    node.symbolinfo->returnsignature = returntype.lexeme;

                    node.symbolinfo->signature = "f(";

                    // TODO: put all parameters in the symbol table

                    std::vector<ASTNode> & params = children[ 1 ].children[ 0 ].children;
                    for ( auto & param : params )
                    {
                        auto & typestring = param.children[ 1 ].lexeme;

                        node.symbolinfo->signature += typestring;
                        node.symbolinfo->signature += ",";

                        funcparams.push_back( &param );
                    }

                    // remove trailing "," that I added
                    if ( node.symbolinfo->signature.size() > 2 ) node.symbolinfo->signature.pop_back();

                    node.symbolinfo->signature += ")";

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
                case AST_FUNCCALL:
                {
                    auto linenum = node.children[ 0 ].linenum;
                    auto stabrecord = node.children[ 0 ].symbolinfo;

                    // REVIEW: don't i need look at the normal signature??
                    if ( !stabrecord || !strncmp( stabrecord->returnsignature.data(), "f(", 2 ) )
                        error( linenum, "can't call something that isn't a function" );

                    node.expressiontype = node.children[ 0 ].symbolinfo->returnsignature;
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
                {
                    node.expressiontype = "string";
                    break;
                }
                case AST_PLUS:
                case AST_MINUS:
                case AST_MUL:
                case AST_DIV:
                case AST_MOD:
                case AST_UMINUS:
                {
                    check_operand_types( "int", node );

                    node.expressiontype = "int";
                    break;
                }
                case AST_ID:
                {
                    if ( !strncmp( node.symbolinfo->signature.data(), "f(", 2 ) )
                        node.expressiontype = node.symbolinfo->returnsignature;
                    else
                        node.expressiontype = node.symbolinfo->signature;

                    break;
                }
                case AST_ASSIGN:
                {
                    auto lhs = node.children[ 0 ].symbolinfo;
                    auto rhs = node.children[ 1 ].symbolinfo;

                    if ( !lhs || lhs->signature.size() == 0 )
                        error( node.linenum, "can only assign to a variable" );

                    if ( lhs->isconst )
                        error( node.linenum, "can't assign to a constant" );

                    if ( lhs->istype )
                        error( node.linenum, "can't use type '%s' here", node.children[ 0 ].lexeme.data() );
                    if ( rhs && rhs->istype )
                        error( node.linenum, "can't use type '%s' here", node.children[ 1 ].lexeme.data() );

                    if ( lhs->returnsignature.size() > 0 || !strncmp( lhs->signature.data(), "f(", 2 ) )
                        error( node.linenum, "cannot assign to a function" );


                    // REVIEW: does every identifier have a valid type at this point?
                    std::string type = node.children[ 0 ].expressiontype;
                    check_operand_types( type, node );

                    node.expressiontype = "void";
                    break;
                }
                case AST_LOGIC_AND:
                case AST_LOGIC_OR:
                case AST_LOGIC_NOT:
                {
                    check_operand_types( "bool", node );

                    node.expressiontype = "bool";
                    break;
                }
                case AST_EQ:
                case AST_NEQ:
                {
                    std::string type = node.children[ 0 ].expressiontype;
                    check_operand_types( type, node );

                    node.expressiontype = "bool";
                    break;
                }
                case AST_LT:
                case AST_GT:
                case AST_LEQ:
                case AST_GEQ:
                {
                    std::string type = node.children[ 0 ].expressiontype;
                    // XXX: bleh
                    if ( type == "int" )
                        check_operand_types( type, node );
                    else
                        check_operand_types( "string", node );

                    node.expressiontype = "bool";
                    break;
                }
                case AST_FUNCCALL:
                {
                    auto linenum = node.children[ 0 ].linenum;

                    // TODO: stick in function with other signature builder
                    std::string callsig = "f(";
                    std::vector<ASTNode> & arguments = node.children[ 1 ].children;
                    for ( auto & param : arguments )
                    {
                        auto & typestring = param.expressiontype;

                        callsig += typestring;
                        callsig += ",";
                    }
                    // remove trailing "," that I added
                    if ( callsig.size() > 2 ) callsig.pop_back();

                    callsig += ")";

                    if ( callsig != node.children[ 0 ].symbolinfo->signature )
                        error( linenum, "number/type of arguments doesn't match function declaration" );

                    break;
                }
                case AST_IF:
                case AST_IFELSE:
                case AST_FOR:
                {
                    if ( !check_child_type( 0, "bool", node ) )
                        error( node.linenum, "%s expression must be boolean type", ASTNode_to_string( node.type ).data() );
                    break;
                }
                default:
                    break;
            }
        };
        postorder( root, pass_3 );
    }

    // pass 4: finish up
    {
        static int for_level = 0;
        static std::string returntype;
        // TODO: typecheck assignments in pass 3 but check that the operands are legal in here
        auto pass_4_pre = +[]( ASTNode &node ) {
            switch ( node.type )
            {
                case AST_FOR:
                {
                    for_level++;
                    break;
                }
                case AST_FUNC:
                {
                    returntype = node.symbolinfo->returnsignature;
                    break;
                }
                default:
                    break;
            }
        };

        auto pass_4_post = +[]( ASTNode &node ) {
            switch ( node.type )
            {
                case AST_BREAK:
                {
                    if ( for_level == 0 ) error( node.linenum, "break must be inside 'for'" );

                    break;
                }
                case AST_FOR:
                {
                    for_level--;
                    break;
                }
                case AST_BLOCK:
                {
                }
                default:
                    break;
            }
        };

        prepost( root, pass_4_pre, pass_4_post );
    }
}

