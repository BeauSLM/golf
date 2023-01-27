#include <stdio.h>
#include <stdlib.h>

#include "error.hpp"

void error(const char *msg, int linenum) {
    fprintf(stderr, "Error: %s at line number: %d\n", msg, linenum);
    exit(EXIT_FAILURE);
}

void bad_char_error(const char *msg, char badchar, int linenum) {
    fprintf(stderr, "Error: %s '%c' at line number: %d\n", msg, badchar, linenum);
    exit(EXIT_FAILURE);
}

// track number of warnings given
static int num_warnings = 0;
const static int max_warnings = 15;

void handle_warning_cascade() {
    if (num_warnings >= max_warnings) {
        fprintf(stderr, "Max warnings reached (that's %d warnings!). Exiting...\n", max_warnings);
        exit(EXIT_FAILURE);
    }
}

void warning(const char *msg, int linenum) {
    fprintf(stderr, "Warning: %s at line number: %d\n", msg, linenum);

    num_warnings++;
    handle_warning_cascade();
}

void bad_char_warning(const char *msg, char badchar, int linenum) {
    fprintf(stderr, "Warning: %s '%c' at line number: %d\n", msg, badchar, linenum);

    num_warnings++;
    handle_warning_cascade();
}
