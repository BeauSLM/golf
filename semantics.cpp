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

static const struct {
    ASTNodeID operatorid;
    std::string lhs, rhs, expressiontype;
} type_legals[] = {
    // arithmatic ops
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

// pass 1: populate file block
static bool main_is_defined = false;
void pass_1
( ASTNode & node )
{
    if ( node.type != AST_GLOBVAR && node.type != AST_FUNC ) return;

    // identifier (name) of the symbol
    ASTNode &ident = node.children[ 0 ];

    // functions and variables are not constants or types
    // REVIEW: which linenum should the record get???
    STabRecord *record = define( ident.lexeme, ident.linenum );
    record->isconst    = record->istype = false;

    node.symbolinfo = record;

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

            for ( ASTNode * param : funcparams )
            {
                ASTNode & name = param->children[ 0 ];
                ASTNode & type = param->children[ 1 ];

                name.symbolinfo = define( name.lexeme, name.linenum );
                type.symbolinfo = lookup( type.lexeme, type.linenum );

                name.symbolinfo->signature = type.lexeme;

                // store the symbol record in the formal node for convenience
                param->symbolinfo = name.symbolinfo;
            }

            funcparams.clear();
            break;
        }
        // REVIEW: this can be done pre or post because we can't define
        // custom types in this language
        case AST_TYPEID:
        {
            auto sym = lookup( node.lexeme, node.linenum );

            if ( !sym->istype ) error( node.linenum, "expected type, got '%s'", node.lexeme.data() );

            node.symbolinfo = sym;

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

            // get symbolinfo from symbol table
            ASTNode &funcname = children[ 0 ];
            node.symbolinfo   = lookup ( funcname.lexeme, funcname.linenum );

            // give the symboltable record its return type
            auto returntype = lookup( children[ 1 ].children[ 1 ].lexeme, children[ 1 ].children[ 1 ].linenum );
            node.symbolinfo->returnsignature = returntype->signature;

            // build the function's signature
            node.symbolinfo->signature = "f(";
            std::vector<ASTNode> & params = children[ 1 ].children[ 0 ].children;
            for ( ASTNode & param : params )
            {
                std::string & typestring = param.children[ 1 ].lexeme;

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
            ASTNode &varname = node.children[ 0 ];
            ASTNode &type    = node.children[ 1 ];

            node.symbolinfo = define( varname.lexeme, varname.linenum );
            node.symbolinfo->signature = type.lexeme;

            break;
        }
        case AST_FUNCCALL:
        {
            auto linenum    = node.children[ 0 ].linenum;
            auto stabrecord = node.children[ 0 ].symbolinfo;

            // REVIEW: don't i need look at the normal signature??
            if ( !stabrecord || !strncmp( stabrecord->returnsignature.data(), "f(", 2 ) )
                error( linenum, "can't call something that isn't a function" );

            node.symbolinfo = node.children[ 0 ].symbolinfo;
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
        // check operand types
        {
            for ( auto typecheck : type_legals )
            {
                if ( typecheck.operatorid != node.type ) continue;

                auto & lhs = node.children[ 0 ].expressiontype;
                if ( lhs != typecheck.lhs ) continue;

                if ( typecheck.rhs.size() > 0 )
                {
                    auto & rhs = node.children[ 1 ].expressiontype;
                    if ( rhs != typecheck.rhs ) continue;
                }

                node.expressiontype = typecheck.expressiontype;
                goto match_found;
            }
            error( node.linenum, "operand type mismatch for '%s'", ASTNode_to_string( node.type ).data() );
match_found:
            break;
        }
        case AST_FUNCCALL:
        {
            node.expressiontype = node.symbolinfo->returnsignature;

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

            if ( callsig != node.symbolinfo->signature )
                error( linenum, "number/type of arguments doesn't match function declaration" );

            break;
        }
        case AST_IF:
        case AST_IFELSE:
        case AST_FOR:
        {
            if ( node.children[ 0 ].expressiontype != "bool" )
                error( node.linenum, "%s expression must be boolean type", ASTNode_to_string( node.type ).data() );

            break;
        }
        default:
            break;
    }
}

static ASTNode * current_function = nullptr;
static bool need_return = false;
static int for_level = 0;
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
        case AST_FUNC:
        {
            current_function = &node;
            need_return = current_function->symbolinfo->returnsignature != "void";

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
            if ( need_return )
                error( current_function->linenum, "no return statement in function" );

            current_function = nullptr;
            need_return = false;
            break;
        }
        case AST_RETURN:
        {
            if ( !need_return && node.children.size() > 0 )
                error( node.linenum, "this function can't return a value" );

            if ( need_return && node.children.size() == 0 )
                error( node.linenum, "this function must return a value" );

            std::string returnvaltype = node.children[ 0 ].expressiontype;
            std::string returntype    = current_function->symbolinfo->returnsignature;
            if ( need_return && node.children.size() > 0 && returnvaltype != returntype )
                error( node.linenum, "returned value has the wrong type" );

            need_return = false;

            break;
        }
        case AST_ASSIGN:
        {
            auto lhs = node.children[ 0 ].symbolinfo;
            auto rhs = node.children[ 1 ].symbolinfo;

            if ( !lhs || lhs->signature.size() == 0 || lhs->returnsignature.size() > 0 )
                error( node.linenum, "can only assign to a variable" );

            if ( lhs->isconst )
                error( node.linenum, "can't assign to a constant" );

            if ( lhs->istype )
                error( node.linenum, "can't use type '%s' here", node.children[ 0 ].lexeme.data() );
            if ( rhs && rhs->istype )
                error( node.linenum, "can't use type '%s' here", node.children[ 1 ].lexeme.data() );

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
                symbol.signature,
                symbol.returnsignature,
                symbol.isconst,
                symbol.istype
            };

            // NOTE: not using define() because I know the information of the predefined symbols
            scopestack.back()[ symbol.name ] = record;
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

