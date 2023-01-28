// TODO: wrap printf somehow
#include <stdio.h>
#include <stdlib.h>
#include <string>

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
        default   : return std::string( 1, c );

    }
}

void error( const char *msg, int linenum ) {
    fprintf( stderr, "error: %s at or near line %d\n", msg, linenum );
    exit( EXIT_FAILURE );
}

void bad_char_error( const char *msg, char badchar, int linenum ) {
    const std::string badchar_str = ascii_char_to_string( badchar );
    fprintf( stderr, "error: %s '%s' at or near line %d\n", msg, badchar_str.data(), linenum );
    exit( EXIT_FAILURE );
}

// track number of warnings given
static int num_warnings = 0;
const static int max_warnings = 10;

void handle_warning_cascade( int linenum ) {
    if ( num_warnings >= max_warnings ) {
        error( "too many warnings", linenum );
        exit( EXIT_FAILURE );
    }
}

void warning( const char *msg, int linenum ) {
    fprintf( stderr, "warning: %s at or near line %d\n", msg, linenum );

    num_warnings++;
    handle_warning_cascade( linenum );
}

void bad_char_warning( const char *msg, char badchar, int linenum ) {
    const std::string badchar_str = ascii_char_to_string( badchar );
    fprintf( stderr, "warning: %s '%s' at or near line %d\n", msg, badchar_str.data(), linenum );

    num_warnings++;
    handle_warning_cascade( linenum );
}
