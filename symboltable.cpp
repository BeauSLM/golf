// REVIEW: this all can probably go in the semantic checker file but I need
// a cpp file for every header atm cause of my makefile - I should fix that
#include "symboltable.hpp"
#include "error.hpp"

std::vector<STab> scopestack;

void
openscope()
{
    scopestack.emplace_back();
}

void
closescope()
{
    scopestack.pop_back();
}

STabRecord * define
( const std::string &name, const int linenum )
{
    STab &scope = scopestack.back();

    // check for redefinition
    if ( scope.find( name ) != scope.end() )
        error( linenum, "'%s' redefined", name.data() );

    return ( scope[ name ] = new STabRecord() );
}

STabRecord * lookup
( const std::string &name, const int linenum )
{
    // REVIEW: is this correct?
    // TODO: rename "it" - something that's more informative
    for ( auto it = scopestack.rbegin(); it != scopestack.rend(); it++ )
        if ( it->find(name) != it->end() )
            return ( *it )[ name ];

    error( linenum, "unknown name '%s'", name.data() );
}

