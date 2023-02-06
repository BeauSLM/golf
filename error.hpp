#pragma once

#include <string>

// error and warning routines

// fail routine for fatal errors
void error( int linenum, const char *msg, ... );

// warning message for recoverable errors
void warning( int linenum, const char *msg, ... );

// warning message for giving information about bad characters
void bad_char_error( const char *msg, char badchar, int linenum );

// warning message for giving information about bad characters
void bad_char_warning( const char *msg, char badchar, int linenum );

// INCOMPLETE: convert ascii character to readable form
const std::string ascii_char_to_string( int c );
