#include "Node.h"


CNode::CNode(const LocationInfo &location)
        : parent(0), location(location), local_cost(0)
{
}

bool almostEquals(float a, float b)
{
    return (a - b) * (a - b) < 0.01;
}

void CNode::calculate_cost(const CNode &destination)
{
    heuristic_cost = distance_to(destination);
    local_cost = parent->local_cost + distance_to(*parent);

    if (parent && parent->parent)
    {
        float dxp1 = location.pos.x - parent->location.pos.x;
        float dzp1 = location.pos.z - parent->location.pos.z;

        float dxp2 = parent->location.pos.x - parent->parent->location.pos.x;
        float dzp2 = parent->location.pos.z - parent->parent->location.pos.z;

        if (!almostEquals(dxp1, dxp2) || !almostEquals(dzp1, dzp2))
        {
            local_cost += 5; // penalty cost
        }
    }

    global_cost = heuristic_cost + local_cost;
}

float CNode::distance_to(const CNode &node)
{
    int region_distance_x = this->location.region.x - node.location.region.x;
    int region_distance_y = this->location.region.y - node.location.region.y;

    // Switch to ingame units, region-y is mesh-z :S
    float distance_x = region_distance_x * NAVMESH_RESOLUTION;
    float distance_z = region_distance_y * NAVMESH_RESOLUTION;

    float node_distance_x = this->location.pos.x - node.location.pos.x;
    float node_distance_z = this->location.pos.z - node.location.pos.z;

    float total_distance_x = distance_x + node_distance_x;
    float total_distance_z = distance_z + node_distance_z;
#if EXO_BETTER_DISTANCE
    return max(total_distance_x, total_distance_z) + (sqrt(2.0) -1) * min(total_distance_x, total_distance_z);
#else
    return sqrt(total_distance_x * total_distance_x + total_distance_z * total_distance_z);
#endif
}
