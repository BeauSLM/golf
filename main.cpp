#include <stdio.h>
#include <string.h>
#include <errno.h>

#include "error.hpp"
#include "lex.hpp"
#include "parse.hpp"

void printast( const ASTNode & root, int depth ) {
    // HACK: this is cursed. like actually
    std::string indent;
    for ( int i = 0; i < depth; i++ ) indent += "    ";

    printf( "%s%s\n", indent.data(), ASTNode_to_string( root ).data() );

    for ( const auto & kid : root.children )
        printast( kid, depth + 1);
}

int main( int argc, char *argv[] ) {
    if ( argc != 2 ) {
        fprintf( stderr, "Usage: %s <filename>\n\n", argv[0] );
        return EXIT_FAILURE;
    }


    // open file argument and give the FILE * to the lexer
    {
        // NOTE: if I need to close this instead of letting OS take care of it, I'll have to move this declaration
        // to a scope where I can close it after all lexing is done. For now, OS can clean up at the end.
        extern FILE *fp; // in `lex.cpp` - only lexer cares abt it after this

        if ( !( fp = fopen( argv[1], "r" ) ) ) {
            fprintf( stderr, "Could not open file '%s': %s\n", argv[1], strerror( errno ) );
            return EXIT_FAILURE;
        }
    }

    // print each token except EOF
    // for ( Tokinfo t = lex(); t.token != TOKEN_EOF; t = lex() )
    //     printf( "%s\t[%s] @ line %d\n", token_to_string( t.token ), t.lexeme.data(), t.linenum );

    auto root = parse();
    printast( root, 0 );

    return EXIT_SUCCESS;
}
