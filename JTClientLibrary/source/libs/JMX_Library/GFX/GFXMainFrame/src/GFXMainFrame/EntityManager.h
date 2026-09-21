#pragma once

#include <BSLib/BSLib.h>

#include <map>
class CIObject;
// TODO: This is bad. Don't make global typedefs like that.

class CEntityManager : public CObjChild
{
public:
    /// \address 00ba3770
    void DestroyEntityIC(void* p);

    char _gap[4];

    // TODO: How sure are we that this is CIEntity and not CIObject? There's the proof for that.
    std::map<int, CIObject*> entities;
};
