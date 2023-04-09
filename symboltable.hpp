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

    int         frame_offset_bytes = 1; // NOTE: < 0 cause stack grows down
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
