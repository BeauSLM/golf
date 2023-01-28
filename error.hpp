#pragma once

// error and warning routines

// fail routine for fatal errors
void error( const char *msg, int linenum );

// warning message for recoverable errors
void warning( const char *msg, int linenum );

// warning message for giving information about bad characters
void bad_char_error( const char *msg, char badchar, int linenum );

// warning message for giving information about bad characters
void bad_char_warning( const char *msg, char badchar, int linenum );
