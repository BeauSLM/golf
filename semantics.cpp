#include "semantics.hpp"
#include "symboltable.hpp"
#include "error.hpp"

#include "operator_rules.hpp"

#include <limits.h>
#include <string.h>

// symbols in the universe block
static const struct {
    std::string symbolname,
                signature,
                returnsignature;
    bool        isconst = false,
                istype  = false;
} universe[] = {
    // TODO: in second column: string -> str
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

// pass 1: populate file block of symbol table
static bool main_is_defined = false;
void pass_1
( ASTNode & node )
{
    if ( node.type != AST_GLOBVAR && node.type != AST_FUNC ) return;

    ASTNode &ident = node[ 0 ];

    // functions and variables are not constants or types
    STabRecord *record = define( ident.lexeme, ident.linenum );
    record->isconst    = record->istype = false;

    node.symbolinfo = record;

    // defining main:
    // error if it has arguments
    // error if it has a return type
    if ( node.type == AST_FUNC && node[ 0 ].lexeme == "main" )
    {
        auto &arguments = node[ 1 ][ 0 ].children;
        if ( arguments.size() > 0 )
            error( node.linenum, "main() can't have arguments");

        auto &returntype = node[ 1 ][ 1 ].lexeme;
        if ( returntype != "$void" )
            error( node.linenum, "main() can't have a return value" );

        main_is_defined = true;
    }

    // OPTIMIZE: global declarations can't have children that are global declarations (I think)
    // this means I can simply prune the traversal of the children of this node
}

// pass 2: fully populate global symbol table records and annotate all identifier
// nodes (and others) with their corresponding symbol table records
// i.e. populate all symbol table records and annotate the AST with them

// function parameters that we defer defining until we have opened the function's scope
static std::vector<ASTNode *> funcparams;
void pass_2_pre
( ASTNode &node )
{
    switch ( node.type )
    {
        case AST_PROGRAM:
        {
            node.symbolinfo = lookup( "main", -1 );
            break;
        }
        case AST_BLOCK:
        {
            openscope();

            // if we visited a function node, we populated the list `funcparams`
            // with the parameters of the function. defining them here ensures
            // that we've already opened the scope corresponding to the function
            // body. after defining them, the list is cleared
            for ( ASTNode * param : funcparams )
            {
                ASTNode & name = ( *param )[ 0 ];
                ASTNode & type = ( *param )[ 1 ];

                name.symbolinfo = define( name.lexeme, name.linenum );
                type.symbolinfo = lookup( type.lexeme, type.linenum );

                name.symbolinfo->signature = type.lexeme;

                // store the symbol record in the formal node for convenience
                param->symbolinfo = name.symbolinfo;
            }

            funcparams.clear();
            break;
        }
        // make sure our types are actually types
        case AST_TYPEID:
        {
            auto sym = lookup( node.lexeme, node.linenum );

            if ( !sym->istype ) error( node.linenum, "expected type, got '%s'", node.lexeme.data() );

            node.symbolinfo = sym;

            break;
        }
        case AST_GLOBVAR:
        {
            ASTNode &type    = node[ 1 ];
            ASTNode &varname = node[ 0 ];

            node.symbolinfo            = lookup( varname.lexeme, varname.linenum );
            node.symbolinfo->signature = type.lexeme;

            break;
        }
        // with functions we need define both the arguments and the return type
        case AST_FUNC:
        {
            // get symbolinfo from symbol table
            ASTNode &funcname = node[ 0 ];
            node.symbolinfo   = lookup ( funcname.lexeme, funcname.linenum );

            // give the symboltable record its return type
            ASTNode & returntype_ident       = node[ 1 ][ 1 ];
            STabRecord * returntype          = lookup( returntype_ident.lexeme, returntype_ident.linenum );
            node.symbolinfo->returnsignature = returntype->signature;

            // build the function's signature
            node.symbolinfo->signature    = "f(";
            std::vector<ASTNode> & params = node[ 1 ][ 0 ].children;
            for ( ASTNode & param : params )
            {
                std::string & typestring = param[ 1 ].lexeme;

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
    }
}

void pass_2_post
( ASTNode &node )
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
            ASTNode &varname = node[ 0 ];
            ASTNode &type    = node[ 1 ];

            node.symbolinfo            = define( varname.lexeme, varname.linenum );
            node.symbolinfo->signature = type.lexeme;

            break;
        }
        default:
            break;
    }
}

// pass 3: propagate type information up the AST, starting at the leaves
void pass_3
( ASTNode &node )
{
    switch ( node.type )
    {
        // check if int literal fits in 32 bits
        case AST_INT:
        {
            // too many digits
            if ( node.lexeme.size() > 12 )
                error( node.linenum, "integer literal out of range", node.lexeme.data() );

            int64_t value = std::stoll( node.lexeme );
            if ( value < INT_MIN )
                error( node.linenum, "integer literal too small", node.lexeme.data() );

            if ( value > INT_MAX )
                error( node.linenum, "integer literal too large", node.lexeme.data() );

            node.expressiontype = "int";
        } break;
        case AST_STRING:
        {
            node.expressiontype = "string";
        } break;
        case AST_ID:
        {
            node.expressiontype = node.symbolinfo->signature;
            break;
        }
        case AST_PLUS:
        case AST_MINUS:
        case AST_MUL:
        case AST_DIV:
        case AST_MOD:
        case AST_UMINUS:
        case AST_ASSIGN:
        case AST_LOGIC_AND:
        case AST_LOGIC_OR:
        case AST_LOGIC_NOT:
        case AST_EQ:
        case AST_NEQ:
        case AST_LT:
        case AST_GT:
        case AST_LEQ:
        case AST_GEQ:
        // check operand types and give operator its expressiontype
        {
            extern OpRule operator_rules[ NUM_OPS ];

            for ( auto typecheck : operator_rules )
            {
                if ( typecheck.operatorid != node.type ) continue;

                auto & lhs = node[ 0 ].expressiontype;
                if ( lhs != typecheck.lhs ) continue;

                // if typecheck.rhs is empty, we're looking at a unary operator
                // i.e. one operand
                if ( typecheck.rhs.size() > 0 )
                {
                    auto & rhs = node[ 1 ].expressiontype;
                    if ( rhs != typecheck.rhs ) continue;
                }

                // passed the type check -> give node its expression type and skip the error
                node.expressiontype = typecheck.expressiontype;
                goto match_found;
            }
            error( node.linenum, "operand type mismatch for '%s'", ASTNode_to_string( node.type ).data() );
match_found:
            break;
        }
        // check that our function call has the correct number and type of arguments
        case AST_FUNCCALL:
        {
            // make sure the identifier we're calling is actually a function
            int linenum      = node[ 0 ].linenum;
            STabRecord * sym = node[ 0 ].symbolinfo;

            if ( !sym || strncmp( sym->signature.data(), "f(", 2 ) )
                error( linenum, "can't call something that isn't a function" );

            node.expressiontype = sym->returnsignature;

            std::string callsig              = "f(";
            std::vector<ASTNode> & arguments = node[ 1 ].children;
            for ( auto & param : arguments )
            {
                auto & typestring = param.expressiontype;

                callsig += typestring;
                callsig += ",";
            }
            // remove trailing "," that I added
            if ( callsig.size() > 2 ) callsig.pop_back();

            callsig += ")";

            if ( callsig != sym->signature )
                error( linenum, "number/type of arguments doesn't match function declaration" );

            break;
        }
        case AST_IF:
        case AST_IFELSE:
        case AST_FOR:
        // enforce that condition is boolean
        {
            if ( node[ 0 ].expressiontype != "bool" )
                error( node.linenum, "%s expression must be boolean type", ASTNode_to_string( node.type ).data() );

            break;
        }
        default:
            break;
    }
}

static ASTNode * current_function = nullptr;
static bool need_return = false, has_returned = false;
static int for_level    = 0,     block_level  = 0;
//pass 4: finish up
void pass_4_pre
( ASTNode &node )
{
    switch ( node.type )
    {
        case AST_FOR:
        {
            for_level++;
            break;
        }
        case AST_BLOCK:
        {
            block_level++;
            break;
        }
        case AST_FUNC:
        {
            current_function = &node;
            need_return      = current_function->symbolinfo->returnsignature != "void";
            has_returned     = !need_return;

            break;
        }
        default:
            break;
    }
}

void pass_4_post
( ASTNode &node )
{
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
            block_level--;

            if ( !has_returned && block_level == 0 )
                error( current_function->linenum, "no return statement in function" );

            if ( block_level == 0 )
            {
                current_function = nullptr;
                need_return = false;
            }

            break;
        }
        case AST_RETURN:
        {
            if ( !need_return && node.children.size() > 0 )
                error( node.linenum, "this function can't return a value" );

            if ( need_return && node.children.size() == 0 )
                error( node.linenum, "this function must return a value" );

            // an early return
            if ( !need_return && node.children.size() == 0 )
                return;

            std::string returnvaltype = node[ 0 ].expressiontype;
            std::string returntype    = current_function->symbolinfo->returnsignature;
            if ( need_return && node.children.size() > 0 && returnvaltype != returntype )
                error( node.linenum, "returned value has the wrong type" );

            has_returned = true;

            break;
        }
        case AST_ASSIGN:
        {
            auto lhs = node[ 0 ].symbolinfo;
            auto rhs = node[ 1 ].symbolinfo;

            if ( !lhs || lhs->signature.size() == 0 || lhs->returnsignature.size() > 0 )
                error( node.linenum, "can only assign to a variable" );

            if ( lhs->isconst )
                error( node.linenum, "can't assign to a constant" );

            if ( lhs->istype )
                error( node.linenum, "can't use type '%s' here", node[ 0 ].lexeme.data() );

            if ( rhs && rhs->istype )
                error( node.linenum, "can't use type '%s' here", node[ 1 ].lexeme.data() );

            break;
        }
        default:
            break;
    }
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
                .signature       = symbol.signature,
                .returnsignature = symbol.returnsignature,
                .isconst         = symbol.isconst,
                .istype          = symbol.istype
            };

            if ( symbol.returnsignature.size() > 0 )
                record->label = symbol.symbolname;

            // NOTE: not using define() because I know the information of the predefined symbols
            scopestack.back()[ symbol.symbolname ] = record;
        }
    }

    openscope(); // file block
    preorder( root, pass_1);

    if ( !main_is_defined )
        error( -1, "missing main() function" );

    prepost( root, pass_2_pre, pass_2_post );

    postorder( root, pass_3 );

    prepost( root, pass_4_pre, pass_4_post );
}

