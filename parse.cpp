#include "parse.hpp"
#include "lex.hpp"
#include "error.hpp"

Tokinfo expect(TokenID type) {
    // auto tok = lex();
    // if ( tok.type == type ) return;
    // error( tok.linenum, "expected token of type %s", to_string(type) );
}

ASTNode parse( FILE *fp ) {
    ASTNode root;

    auto tok = lex( fp );
    if ( tok.token != TOKEN_EOF ) {
        error( tok.linenum, "Expected EOF" );
    }
    return root;
}
