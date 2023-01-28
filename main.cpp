#include <stdio.h>
#include <string.h>
#include <errno.h>

#include "error.hpp"
#include "lex.hpp"

int main( int argc, char *argv[] ) {
    FILE *fp;

    // NOTE: I don't use my error routine here because the error message's format is much different
    if ( argc != 2 ) {
        fprintf( stderr, "Usage: %s <filename>\n\n", argv[0] );
        return EXIT_FAILURE;
    }


    // NOTE: I don't use my error routine here because the error message's format is much different
    // fail if file can't be opened
    if ( !( fp = fopen( argv[1], "r" ) ) ) {
        fprintf( stderr, "Could not open file '%s': %s\n", argv[1], strerror( errno ) );
        return EXIT_FAILURE;
    }

    // print each token except EOF
    for ( Tokinfo t = lex( fp ); t.token != TOKEN_EOF; t = lex( fp ) )
        printf( "%s\t[%s] @ line %d\n", token_to_string( t.token ), t.lexeme.data(), t.linenum );

    return EXIT_SUCCESS;
}
