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

        printf( "%s%s\n", indent.data(), ASTNode_to_string( rpre ).data() );
    };

    auto post = +[]( ASTNode & rpost ) {
        rpost.linenum += 0; // XXX: NO-OP to silence unused warning

        depth--;
    };

    prepost( root, pre, post );
}

std::string ASTNode_to_string( ASTNode &n ) {
    std::string numstring;
    if ( n.linenum > 0 ) numstring = " @ line " + std::to_string( n.linenum );

    std::string lexstring;
    if ( n.lexeme.size() > 0 ) lexstring = " [" + n.lexeme + "]";

    switch ( n.type ) {
        // "program structure" or someth idk
        case AST_PROGRAM:   return "program";
        case AST_SIGNATURE: return "sig";
        case AST_FORMAL:    return "formal";
        case AST_FORMALS:   return "formals";
        case AST_BLOCK:     return "block";
        case AST_ACTUALS:   return "actuals";
        case AST_FUNCCALL:  return "funccall";

        // statements
        case AST_EXPRSTMT:  return "exprstmt" + numstring;
        case AST_EMPTYSTMT: return "emptystmt";

        // keywords
        case AST_BREAK:     return "break"    + numstring;
        case AST_FOR:       return "for"      + numstring;
        case AST_FUNC:      return "func"     + numstring;
        case AST_IF:        return "if"       + numstring;
        case AST_IFELSE:    return "ifelse"   + numstring;
        case AST_RETURN:    return "return"   + numstring;
        case AST_VAR:       return "var"      + numstring;
        case AST_GLOBVAR:   return "globvar"  + numstring;

        case AST_PLUS:      return "+"        + numstring;
        case AST_MINUS:     return "-"        + numstring;
        case AST_UMINUS:    return "u-"       + numstring;
        case AST_MUL:       return "*"        + numstring;
        case AST_DIV:       return "/"        + numstring;
        case AST_MOD:       return "%"        + numstring;
        case AST_ASSIGN:    return "="        + numstring;
        case AST_LOGIC_AND: return "&&"       + numstring;
        case AST_LOGIC_OR:  return "||"       + numstring;
        case AST_LOGIC_NOT: return "!"        + numstring;
        case AST_EQ:        return "=="       + numstring;
        case AST_LT:        return "<"        + numstring;
        case AST_GT:        return ">"        + numstring;
        case AST_NEQ:       return "!="       + numstring;
        case AST_LEQ:       return "<="       + numstring;
        case AST_GEQ:       return ">="       + numstring;

        // identifiers
        case AST_ID:        return "id"       + lexstring + numstring;
        case AST_NEWID:     return "newid"    + lexstring + numstring;
        case AST_TYPEID:    return "typeid"   + lexstring + numstring;


        // literals
        case AST_INT:       return "int"      + lexstring + numstring;
        case AST_STRING:    return "string"   + lexstring + numstring;

        // sadness :(
        case AST_UNSET: error( n.linenum, "internal error" );
    }

    // this is unrachable, each case of the switch above returns or errors
    error( -1, "unreachable in node to_string" );
    return "";
}

