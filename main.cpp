#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <sysexits.h>

#include "error.hpp"
#include "lex.hpp"
// #include "parse.hpp"

int main( int argc, char *argv[] ) {
    if ( argc != 2 ) {
        fprintf( stderr, "Usage: %s <filename>\n\n", argv[0] );
        return EX_USAGE;
    }


    // open file argument and give the FILE * to the lexer
    {
        // NOTE: if I need to close this instead of letting OS take care of it, I'll have to move this declaration
        // to a scope where I can close it after all lexing is done. For now, OS can clean up at the end.
        FILE *fp;

        if ( !( fp = fopen( argv[1], "r" ) ) ) {
            fprintf( stderr, "Could not open file '%s': %s\n", argv[1], strerror( errno ) );
            return EX_NOINPUT;
        }

        give_file_ptr_to_lexer( fp );
    }

    // print each token except EOF
    for ( Tokinfo t = lex(); t.token != TOKEN_EOF; t = lex() )
        printf( "%s\t[%s] @ line %d\n", token_to_string( t.token ), t.lexeme.data(), t.linenum );

    // auto root = parse( fp );
    // TODO: print the tree

    return EX_OK;
}
