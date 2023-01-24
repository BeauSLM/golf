#include <stdio.h>

#include "error.h"

int main(int argc, char *argv[]) {
    FILE *fp;

    // NOTE: I don't use my error routine here because there's no line number to point to
    if (argc != 2 || !( fp = fopen(argv[1], "r") )) {
        fprintf(stderr, "Usage: %s <filename>\n\n", argv[0]);
        return 1;
    }

    return 0;
}
