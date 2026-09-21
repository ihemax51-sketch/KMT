#pragma once

class CGObjPC;
class CMsg;

class CItemRegionTravelGuard
{
public:
    static bool Initialize();
    static void Shutdown();

    // Returns false when the item use must be stopped before the native handler.
    static bool InspectItemUse(CGObjPC* player, CMsg* message);

    static void ForgetPlayer(CGObjPC* player);
};
