// REVIEW: this all can probably go in the semantic checker file but I need
// a cpp file for every header atm cause of my makefile - I should fix that
#include "symboltable.hpp"
#include "error.hpp"

std::vector<STab> scopestack;

void
openscope()
{
    scopestack.push_back( STab() );
}

void
closescope()
{
    scopestack.pop_back();
}

void define
( const std::string &name, const int linenum )
{
    STab &scope = scopestack.back();

    // check for redefinition
    if ( scopestack.back().find( name ) != scopestack.back().end() )
        error( linenum, "redefinition of %s", name.data() );

    scope[name] = new STabRecord();
}

STabRecord * lookup
( const std::string &name, const int linenum )
{
    // REVIEW: is this correct?
    // TODO: rename "it" - something that's more informative
    for ( auto it = scopestack.rbegin(); it != scopestack.rend(); it++ )
        if ( it->find(name) != it->end() )
            return ( *it )[ name ];

    error( linenum, "undefined reference to %s", name.data() );
}

