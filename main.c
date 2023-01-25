#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include "error.h"
#include "lex.h"

int main(int argc, char *argv[]) {
    FILE *fp;

    // NOTE: I don't use my error routine here because there's no line number to point to
    if ( argc != 2 ) {
        fprintf(stderr, "Usage: %s <filename>\n\n", argv[0]);
        return EXIT_FAILURE;
    }


    if ( !(fp = fopen(argv[1], "r")) ) {
        fprintf(stderr, "Could not open file '%s': %s\n", argv[1], strerror(errno));
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
