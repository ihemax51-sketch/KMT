#include "LocationInfo.h"

void LocationInfo::normalize_x()
{
    if (pos.x >= NAVMESH_RESOLUTION)
    {
        pos.x -= NAVMESH_RESOLUTION;
        region.x += 1;
    }

    if (pos.x < 0)
    {
        pos.x += NAVMESH_RESOLUTION;
        region.x -= 1;
    }
}

void LocationInfo::normalize_z()
{
    if (pos.z >= NAVMESH_RESOLUTION)
    {
        pos.z -= NAVMESH_RESOLUTION;
        region.y += 1;
    }

    if (pos.z < 0)
    {
        pos.z += NAVMESH_RESOLUTION;
        region.y -= 1;
    }
}

bool LocationInfo::operator==(const LocationInfo& rhs) const
{
    return this->region.raw == rhs.region.raw && this->pos == rhs.pos;
}
