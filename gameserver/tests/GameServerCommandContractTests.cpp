#include <GameServerCommandContract.h>
#include <KMTGuardCustom/LiveDpsMath.h>

int main()
{
    using namespace KmtGameServerCommand;

    if (!IsValidWorldId(1) || !IsValidWorldId(65535) ||
        IsValidWorldId(0) || IsValidWorldId(65536)) return 1;
    if (!IsValidRegionId(1) || !IsValidRegionId(65535) ||
        !IsValidRegionId(-1) || !IsValidRegionId(-32768) ||
        IsValidRegionId(0) || IsValidRegionId(65536) ||
        IsValidRegionId(-32769)) return 2;
    if (ToWireRegionId(-31024) != 34512 ||
        NormalizeRegionIdForCompare(-31024) != 34512 ||
        NormalizeRegionIdForCompare(31024) != 31024) return 10;
    if (!IsValidLayerId(0) || !IsValidLayerId(65535) ||
        IsValidLayerId(-1) || IsValidLayerId(65536)) return 3;
    if (!IsValidCoordinate(-1000000) || !IsValidCoordinate(1000000) ||
        IsValidCoordinate(-1000001) || IsValidCoordinate(1000001)) return 4;
    if (!IsValidSpawnRadius(0) || !IsValidSpawnRadius(1000000) ||
        IsValidSpawnRadius(-1) || IsValidSpawnRadius(1000001)) return 5;
    if (!IsValidInventorySlotWireValue(0) || !IsValidInventorySlotWireValue(255) ||
        IsValidInventorySlotWireValue(-1) || IsValidInventorySlotWireValue(256)) return 6;
    if (!IsValidDestination(1, 1, -1000000, 0, 1000000) ||
        !IsValidDestination(1, -31024, -1000000, 0, 1000000) ||
        IsValidDestination(0, 1, 0, 0, 0) ||
        IsValidDestination(1, 0, 0, 0, 0) ||
        IsValidDestination(1, 1, 1000001, 0, 0)) return 7;
    if (!IsValidGoldRequest(1, 0) || !IsValidGoldRequest(1, 1) ||
        IsValidGoldRequest(0, 1) || IsValidGoldRequest(-1, 1) ||
        IsValidGoldRequest((-9223372036854775807LL - 1), 1) ||
        IsValidGoldRequest(1, 2)) return 8;
    if (ActionChangeItem != 17 || ActionConsumeItem != 18 ||
        ActionConsumeAndChangeItem != 19 || ActionGold != 21 ||
        ActionTownWorldLayer != 22 || ActionTowerCombat != 38 ||
        ActionFreeForAllCombat != 39 ||
        ActionSpawnAtPositionInPlayerWorld != 40 ||
        ActionRetiredItemChange != 131) return 9;

    if (LiveDpsMath::ToLegacyWire(0ULL) != 0U ||
        LiveDpsMath::ToLegacyWire(1ULL) != 1U ||
        LiveDpsMath::ToLegacyWire(4294967294ULL) != 4294967294U ||
        LiveDpsMath::ToLegacyWire(4294967295ULL) != 4294967295U ||
        LiveDpsMath::ToLegacyWire(4294967296ULL) != 4294967295U ||
        LiveDpsMath::ToLegacyWire(5000000000ULL) != 4294967295U ||
        LiveDpsMath::ToLegacyWire(10000000000ULL) != 4294967295U) return 11;
    if (LiveDpsMath::Accumulate(4294967294ULL, 4294967294U, 4294967295U) != 4294967295ULL ||
        LiveDpsMath::Accumulate(4294967295ULL, 4294967295U, 0U) != 4294967296ULL ||
        LiveDpsMath::Accumulate(5000000000ULL, 100U, 200U) != 5000000100ULL) return 12;

    return 0;
}
