#include "ast.hpp"
#include "error.hpp"

#include <strings.h>

void preorder
( ASTNode & root, void ( *callback )( ASTNode & ) )
{
    prepost( root, callback, +[]( ASTNode & ) {} );
}

void postorder
( ASTNode & root, void ( *callback )( ASTNode & ) )
{
    for ( auto & child : root.children ) postorder( child, callback );

    callback( root );
}

// TODO: prune the traversal if we need to
void prepost
( ASTNode & root, void ( *precallback )( ASTNode & ), void ( *postcallback )( ASTNode & ) )
{
    try { precallback( root ); }
    catch ( PruneTraversalException & e ) { return; }

    for ( auto & child : root.children ) prepost( child, precallback, postcallback );

    postcallback( root );
}

void printast( ASTNode & root ) {
    // track how far in the AST we are
    static int depth = -1;

    auto pre = +[]( ASTNode & rpre ) {
        depth++;

        std::string indent;
        for ( int i = 0; i < depth; i++ ) indent += "    ";

        printf( "%s%s\n", indent.data(), ASTNode_printstring( rpre ).data() );
    };

    auto post = +[]( ASTNode & rpost ) {
        rpost.linenum += 0; // XXX: NO-OP to silence unused warning

        depth--;
    };

    prepost( root, pre, post );
}

std::string ASTNode_to_string( ASTNodeID n ) {
    switch ( n ) {
        // "program structure" or someth idk
        case AST_PROGRAM:   return "program";
        case AST_SIGNATURE: return "sig";
        case AST_FORMAL:    return "formal";
        case AST_FORMALS:   return "formals";
        case AST_BLOCK:     return "block";
        case AST_ACTUALS:   return "actuals";
        case AST_FUNCCALL:  return "funccall";

        // statements
        case AST_EMPTYSTMT: return "emptystmt";
        case AST_EXPRSTMT:  return "exprstmt";

        // keywords
        case AST_BREAK:     return "break";
        case AST_FOR:       return "for";
        case AST_FUNC:      return "func";
        case AST_IF:        return "if";
        case AST_IFELSE:    return "ifelse";
        case AST_RETURN:    return "return";
        case AST_VAR:       return "var";
        case AST_GLOBVAR:   return "globvar";

        case AST_PLUS:      return "+";
        case AST_MINUS:     return "-";
        case AST_UMINUS:    return "u-";
        case AST_MUL:       return "*";
        case AST_DIV:       return "/";
        case AST_MOD:       return "%";
        case AST_ASSIGN:    return "=";
        case AST_LOGIC_AND: return "&&";
        case AST_LOGIC_OR:  return "||";
        case AST_LOGIC_NOT: return "!";
        case AST_EQ:        return "==";
        case AST_LT:        return "<";
        case AST_GT:        return ">";
        case AST_NEQ:       return "!=";
        case AST_LEQ:       return "<=";
        case AST_GEQ:       return ">=";

        // identifiers
        case AST_ID:        return "id";
        case AST_NEWID:     return "newid";
        case AST_TYPEID:    return "typeid";


        // literals
        case AST_INT:       return "int";
        case AST_STRING:    return "string";

        // sadness :(
        case AST_UNSET: error( -1, "internal error" );
    }

    // this is unreachable, each case of the switch above returns or errors
    error( -1, "unreachable in node to_string" );
}

std::string ASTNode_printstring( ASTNode &n ) {
    std::string result = ASTNode_to_string( n.type );

    std::string numstring;
    if ( n.linenum > 0 ) numstring = " @ line " + std::to_string( n.linenum );

    std::string lexstring;
    if ( n.lexeme.size() > 0 ) lexstring = " [" + n.lexeme + "]";

#if MILESTONE == 3
    std::string sigstring;
    if ( n.expressiontype.size() > 0 ) sigstring = " sig=" + n.expressiontype;
    // else if ( n.symbolinfo ) sigstring = " sig=" + n.symbolinfo->signature;

    std::string stabstring;
    if ( n.symbolinfo ) {
        char hexstring[ 32 ];
        bzero( hexstring, 32 );
        sprintf( hexstring, "%p", n.symbolinfo );
        stabstring = " sym=" + std::string( hexstring );
    }

    numstring = sigstring + stabstring + numstring;
#endif

    switch ( n.type ) {
        // "program structure" or someth idk
        case AST_SIGNATURE:
        case AST_FORMAL:
        case AST_FORMALS:
        case AST_BLOCK:
        case AST_ACTUALS:

#if MILESTONE == 2
        case AST_PROGRAM:
        case AST_FUNCCALL:
#endif

        // statements
        case AST_EMPTYSTMT:    break;
        case AST_EXPRSTMT:

#if MILESTONE > 2
        case AST_PROGRAM:
        case AST_FUNCCALL:
#endif

        // keywords
        case AST_BREAK:
        case AST_FOR:
        case AST_FUNC:
        case AST_IF:
        case AST_IFELSE:
        case AST_RETURN:
        case AST_VAR:
        case AST_GLOBVAR:

        case AST_PLUS:
        case AST_MINUS:
        case AST_UMINUS:
        case AST_MUL:
        case AST_DIV:
        case AST_MOD:
        case AST_ASSIGN:
        case AST_LOGIC_AND:
        case AST_LOGIC_OR:
        case AST_LOGIC_NOT:
        case AST_EQ:
        case AST_LT:
        case AST_GT:
        case AST_NEQ:
        case AST_LEQ:
        case AST_GEQ:
            result += numstring;
            break;

        // identifiers
        case AST_ID:
        case AST_NEWID:
        case AST_TYPEID:

        // literals
        case AST_INT:
        case AST_STRING:
            result += lexstring + numstring;
            break;

        // sadness :(
        case AST_UNSET: error( n.linenum, "internal error" );
    }

    return result;
}

