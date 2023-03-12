#pragma once

#include <vector>

#include <string>
#include <unordered_map>

struct STabRecord
{
    std::string signature,
                returnsignature;

    bool        isconst = false,
                istype  = false;
};

// for my sanity
typedef std::unordered_map<std::string, STabRecord*> STab;

void
openscope();

void
closescope();

void define
( const std::string &name, const int linenum );

STabRecord * lookup
( const std::string &name, const int linenum );
