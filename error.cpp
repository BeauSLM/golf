#include <stdio.h>
#include <stdlib.h>

#include "error.hpp"

// TODO: routines that don't take line numbers?

void error(const char *msg, int linenum) {
    fprintf(stderr, "Error: %s\nLine number: %d\nExiting...\n", msg, linenum);
    exit(EXIT_FAILURE);
}

void warning(const char *msg, int linenum) {
    fprintf(stderr, "Warning: %s\nLine number: %d\n\n", msg, linenum);

    // fail on warning cascade
    static int num_warnings = 0;
    num_warnings++;
    const static int max_warnings = 15;
    if (num_warnings >= max_warnings) {
        fprintf(stderr, "Max warnings reached (that's %d warnings!). Exiting...\n", max_warnings);
        exit(EXIT_FAILURE);
    }
}
