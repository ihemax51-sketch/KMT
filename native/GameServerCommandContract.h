#pragma once

// Shared wire and validation contract used by both the ShardManager producer
// and the GameServer consumer. Keep this header compatible with VS2005.
namespace KmtGameServerCommand
{
    enum ActionId
    {
        ActionGrantName = 1,
        ActionSpawnAtPosition = 2,
        ActionSpawnNearPlayer = 3,
        ActionRemoveMonster = 4,
        ActionRemoveMonsterByWorld = 5,
        ActionAddSkill = 6,
        ActionRemoveSkill = 7,
        ActionAddSkillByCode = 8,
        ActionTownPlayer = 10,
        ActionTownWorld = 11,
        ActionPetSkill = 12,
        ActionCape = 13,
        ActionMovePlayer = 14,
        ActionGetUp = 15,
        ActionExperienceRate = 16,
        ActionChangeItem = 17,
        ActionConsumeItem = 18,
        ActionConsumeAndChangeItem = 19,
        ActionSilk = 20,
        ActionGold = 21,
        ActionTownWorldLayer = 22,
        ActionGetUpAtPosition = 23,
        ActionMovePlayerAtPosition = 24,
        ActionTowerCombat = 38,
        ActionFreeForAllCombat = 39,
        ActionSpawnAtPositionInPlayerWorld = 40,
        ActionRetiredItemChange = 131
    };

    const int MaximumBaseWorldId = 65535;
    const int MaximumRegionOrLayerId = 65535;
    const int MinimumSignedRegionId = -32768;
    const int MaximumCoordinate = 1000000;
    const int MaximumSpawnRadius = 1000000;
    const int MaximumInventorySlotWireValue = 255;

    inline bool IsValidWorldId(int value)
    {
        return value > 0 && value <= MaximumBaseWorldId;
    }

    inline bool IsValidRegionId(int value)
    {
        return (value >= MinimumSignedRegionId && value < 0) ||
               (value > 0 && value <= MaximumRegionOrLayerId);
    }

    inline unsigned short ToWireRegionId(int value)
    {
        return static_cast<unsigned short>(value);
    }

    inline int NormalizeRegionIdForCompare(int value)
    {
        return static_cast<int>(ToWireRegionId(value));
    }

    inline bool IsValidLayerId(int value)
    {
        return value >= 0 && value <= MaximumRegionOrLayerId;
    }

    inline bool IsValidCoordinate(int value)
    {
        return value >= -MaximumCoordinate && value <= MaximumCoordinate;
    }

    inline bool IsValidDestination(int worldId, int regionId, int x, int y, int z)
    {
        return IsValidWorldId(worldId) && IsValidRegionId(regionId) &&
               IsValidCoordinate(x) && IsValidCoordinate(y) && IsValidCoordinate(z);
    }

    inline bool IsValidSpawnRadius(int value)
    {
        return value >= 0 && value <= MaximumSpawnRadius;
    }

    inline bool IsValidInventorySlotWireValue(int value)
    {
        return value >= 0 && value <= MaximumInventorySlotWireValue;
    }

    inline bool IsValidToggle(int value)
    {
        return value == 0 || value == 1;
    }

    inline bool IsValidGoldRequest(__int64 amount, int mode)
    {
        return amount > 0 && IsValidToggle(mode);
    }
}
