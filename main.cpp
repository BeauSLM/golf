#include <stdio.h>
#include <string.h>
#include <errno.h>

#include "error.hpp"
#include "lex.hpp"

int main( int argc, char *argv[] ) {
    FILE *fp;

    // NOTE: I don't use my error routine here because there's no line number to point to
    if ( argc != 2 ) {
        fprintf( stderr, "Usage: %s <filename>\n\n", argv[0] );
        return EXIT_FAILURE;
    }


    if ( !( fp = fopen( argv[1], "r" ) ) ) {
        fprintf( stderr, "Could not open file '%s': %s\n", argv[1], strerror( errno ) );
        return EXIT_FAILURE;
    }

    for ( Tokinfo t = lex( fp ); t.token != TOKEN_EOF; t = lex( fp ) ) {
        printf( "%s\t[%s] @ line %d\n", token_to_string( t.token ), t.lexeme.data(), t.linenum );
    }

    return EXIT_SUCCESS;
}
