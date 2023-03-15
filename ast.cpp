#include "ast.hpp"
#include "error.hpp"

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
    precallback( root );

    // TODO: if we need to prune, bail here

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
    std::string result;

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

    // this is unrachable, each case of the switch above returns or errors
    error( -1, "unreachable in node to_string" );
    return "";
}

std::string ASTNode_printstring( ASTNode &n ) {
    std::string nodestring = ASTNode_to_string( n.type );

    std::string numstring;
    if ( n.linenum > 0 ) numstring = " @ line " + std::to_string( n.linenum );

    std::string lexstring;
    if ( n.lexeme.size() > 0 ) lexstring = " [" + n.lexeme + "]";

    switch ( n.type ) {
        // "program structure" or someth idk
        case AST_PROGRAM:   return nodestring;
        case AST_SIGNATURE: return nodestring;
        case AST_FORMAL:    return nodestring;
        case AST_FORMALS:   return nodestring;
        case AST_BLOCK:     return nodestring;
        case AST_ACTUALS:   return nodestring;
        case AST_FUNCCALL:  return nodestring;

        // statements
        case AST_EMPTYSTMT: return nodestring;
        case AST_EXPRSTMT:  return nodestring + numstring;

        // keywords
        case AST_BREAK:     return nodestring + numstring;
        case AST_FOR:       return nodestring + numstring;
        case AST_FUNC:      return nodestring + numstring;
        case AST_IF:        return nodestring + numstring;
        case AST_IFELSE:    return nodestring + numstring;
        case AST_RETURN:    return nodestring + numstring;
        case AST_VAR:       return nodestring + numstring;
        case AST_GLOBVAR:   return nodestring + numstring;

        case AST_PLUS:      return nodestring + numstring;
        case AST_MINUS:     return nodestring + numstring;
        case AST_UMINUS:    return nodestring + numstring;
        case AST_MUL:       return nodestring + numstring;
        case AST_DIV:       return nodestring + numstring;
        case AST_MOD:       return nodestring + numstring;
        case AST_ASSIGN:    return nodestring + numstring;
        case AST_LOGIC_AND: return nodestring + numstring;
        case AST_LOGIC_OR:  return nodestring + numstring;
        case AST_LOGIC_NOT: return nodestring + numstring;
        case AST_EQ:        return nodestring + numstring;
        case AST_LT:        return nodestring + numstring;
        case AST_GT:        return nodestring + numstring;
        case AST_NEQ:       return nodestring + numstring;
        case AST_LEQ:       return nodestring + numstring;
        case AST_GEQ:       return nodestring + numstring;

        // identifiers
        case AST_ID:        return nodestring + lexstring + numstring;
        case AST_NEWID:     return nodestring + lexstring + numstring;
        case AST_TYPEID:    return nodestring + lexstring + numstring;


        // literals
        case AST_INT:       return nodestring + lexstring + numstring;
        case AST_STRING:    return nodestring + lexstring + numstring;

        // sadness :(
        case AST_UNSET: error( n.linenum, "internal error" );
    }

    // this is unrachable, each case of the switch above returns or errors
    error( -1, "unreachable in node printstring" );
    return "";
}

