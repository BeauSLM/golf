// TODO: remove use of std::string
#include <stdio.h>
#include <stdlib.h>
#include <sysexits.h>
#include <strings.h>

#include "error.hpp"

const std::string ascii_char_to_string( int c ) {
    switch ( c ) {
        // escaped characters
        case '\a' : return R"(\a)";
        case '\b' : return R"(\b)";
        case '\f' : return R"(\f)";
        case '\n' : return R"(\n)";
        case '\r' : return R"(\r)";
        case '\t' : return R"(\t)";
        case '\v' : return R"(\v)";
        case '\\' : return R"(\\)";
        case '\'' : return R"(\')";
        case '\"' : return R"(\")";
        case '\?' : return R"(\?)";
        // everything else
        // TODO: verify on all 7-bit ascii chars
        default   : return std::string( 1, c );
    }
}

// NOTE: this is the upper bound for an error messages length, after formatting (!!!)
#define ERROR_LEN 1024
void printf_wrapper( int linenum, const char *prefix, const char *msg, va_list args ) {
    char formatted[ERROR_LEN];
    bzero( formatted, ERROR_LEN );
    vsnprintf(formatted, ERROR_LEN, msg, args);
    fprintf( stderr, "%s: %s at or near line %d\n", prefix, formatted, linenum );
}

void error( int linenum, const char *msg, ... ) {
    va_list args;
    va_start(args, msg);
    printf_wrapper( linenum, "error", msg, args );
    va_end(args);

    exit( EX_DATAERR );
}

// track number of warnings given
static int num_warnings       =  0;
const static int max_warnings = 10;

void handle_warning_cascade( int linenum ) {
    if ( num_warnings >= max_warnings )
        error( linenum, "too many warnings" );
}

void warning( int linenum, const char *msg, ... ) {
    va_list args;
    va_start(args, msg);
    printf_wrapper( linenum, "warning", msg, args );
    va_end(args);

    num_warnings++;
    handle_warning_cascade( linenum );
}
