#pragma once

#include <vector>

#include <string>
#include <unordered_map>

struct STabRecord
{
    std::string signature,
                returnsignature,
                label;

    bool        isconst = false,
                istype  = false;

    int         stack_offset_bytes = 0, // for local variables
                stack_size_words   = 0; // for functions
};

// for my sanity
typedef std::unordered_map<std::string, STabRecord*> STab;

void
openscope();

void
closescope();

STabRecord * define
( const std::string &name, const int linenum );

STabRecord * lookup
( const std::string &name, const int linenum );
