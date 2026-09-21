# KMTGuard — Dependencies Map (خريطة الاعتماديات)

> **Status tags:** ✅ Confirmed | 🔍 Inference | ❓ Unknown
> **النطاق:** GameServer + ShardManager + Filter

---

## 1. ✅ كل System يعتمد على أي Files

```text
Filter Session Keys
├── CGObjPCCustom.cpp (SetFilterSessionKey, ValidateFilterSessionKey)
├── GSKEY (ثابت في الكود)
└── NOT DB

New Alchemy (0x3534)
├── CGObjPCCustom.cpp (HandleNewAlchemyRequest, UpdateItemPlusInDatabase, 120+ branch)
├── sqlCon.cpp (GetItemBindingOpt, TryExecNonQuery)
├── SRO_VT_SHARD.._Items (UPDATE OptLevel)
├── SRO_VT_SHARD.._BindingOptionWithItem (SELECT)
├── CGObjPCCustom.cpp (ItemIsWeapon/Armor/Shield/Accessory/Enhancer/ProofStone, GetDegreeLevel, Send3040)
└── Filter: AlchemyPackets.cs (0x7150/0xB150/0x3534/0x5017/0x5034)

Item Lock/Unlock
├── CGObjPCCustom.cpp (HandleItemLockRequest, HandleItemUnlockRequest)
├── sqlCon.cpp (LockedItemList, LoadLockedItems, GetItemBindingOpt)
├── Game.cpp (0x5061/0x5063)
├── MainProcess.cpp (ShardManager — _OnProcessMessage)
├── Filter: CustomGameServerPacketHandler.cs (DB writes)
└── KMTGuard.dbo.Item_Locked

Timed Plus
├── CustomTimedJobManager.cpp (ConsumeThreadWorker, ConsumeTimedItemPlusRecords, ConsumeTimedItemPlusRecordsDevill)
├── Game.cpp (0x5066/0x5068)
├── sqlCon.cpp (TimedPlusItems, TimedDevillPlusItems, TryExecNonQuery)
├── SRO_VT_SHARD.._Items (UPDATE)
├── KMTGuard: Item_TimedPlus, Item_TimedDevilPlus, Item_Locked (DELETE)
└── ShardManager: MainProcess.cpp (0x5065→0x5066, 0x5067→0x5068)

Command_GameServerQueue (0x8888)
├── Game.cpp (CGame::ProcessMessage case 0x8888 — 28+ type)
├── AsyncGSCommands.cpp (ShardManager DatabaseFetchThread)
├── MainProcess.cpp (استقبال 0x8888 من ShardManager)
├── CShardNetManager (BroadcastMsgToGameServers)
└── KMTGuard.dbo.Command_GameServerQueue

Honor Rank
├── HonorRankRuntimeRefresh.cpp (Initialize + BroadcastHonorRankSnapshot)
├── TrainingCampHonorRankService.cpp (ShardManager — Refresh)
├── SRO_VT_SHARD.._TRAINING_CAMP_UPDATEHONORRANK (SP)
└── Command_GameServerQueue Action_ID=200

DamageMeter
├── DamageMeter.cpp (Initialize + MyCGObjNPC_HandleAggroMap)
├── RegionRestrictionMgr.cpp (CanMobInteractWithPlayer)
└── Filter: CustomGameServerPacketHandler.cs (0x5010 UNIQUE_DPS)

DurabilityControl
├── DurabilityControl.cpp (Initialize + DisableDurabilityHook + InstallHook)
├── sqlCon.cpp (LoadDisableDurabilitySetting)
└── KMTGuard.dbo.System_Settings

PartyMonsterControl
├── PartyMonsterControl.cpp (Initialize + ApplySettings + WriteImmediateByte)
├── sqlCon.cpp (LoadPartyMonsterSettings)
└── KMTGuard.dbo.System_Settings

GObjEvents
├── GObjEvents.cpp (Initialize with placeHook 0x00485AA3)
└── Filter: CustomGameServerPacketHandler.cs (0x3571)

HideName Regions
├── RegionRestrictionMgr.cpp (Initialize — 3 hooks)
├── RegionRestrictionDBSet.cpp (GetHideNameRegionRecords)
└── KMTGuard.dbo.Security_HideNameRegions
```

---

## 2. ✅ أي Procedure تخدم أي System

| الـ SP | تُستدعى من | تخدم |
|---|---|---|
| `Hook_GameServerStart` | `sqlCon.cpp::GameServerInitialized()` | World Startup |
| `NPC_Sync` | `sqlCon.cpp::Patches()` | مزامنة NPC |
| `Hook_UniqueSpawn` | `ShardManager/MainProcess.cpp` | Unique History — بث `0x9999` للفلتر |
| `Hook_UniqueKill` | `ShardManager/MainProcess.cpp` | Unique History — بث `0x10000` |
| `_TRAINING_CAMP_UPDATEHONORRANK` | `ShardManager/TrainingCampHonorRankService.cpp` | Honor Rank — يرسل `0x3C80` للـ GS |

---

## 3. ✅ أي Table يستخدمها أي Component

| الجدول | الـ Components |
|---|---|
| `System_Settings` | GS (`sqlCon.cpp` 3 loaders) + Filter (`_serverSettings.InitServerSettings`) + AdminDesktop (تعديل مباشر) |
| `System_GameServerSettings` | GS (`sqlCon.cpp::LoadGameServerSettings`) |
| `Item_Locked` | GS (`sqlCon.cpp::LoadLockedItems` + `LockedItemList` + `Game.cpp` 0x5061/0x5063) + ShardManager (`AsyncGSCommands::LoadLockedItemList`) + Filter (`CustomGameServerPacketHandler.cs` DB writes) |
| `Item_TimedPlus` / `Item_TimedDevilPlus` | GS (`sqlCon.cpp` loaders + `CustomTimedJobManager` writes) |
| `Party_Members` | GS (`sqlCon.cpp::Patches`) + Filter (disconnect cleanup) |
| `Style_AutoCapeRegions` | GS (`sqlCon.cpp::ServerAutoCapebyRegionID`) |
| `Security_HideNameRegions` | GS (`RegionRestrictionDBSet.cpp`) |
| `SRO_VT_SHARD.._Items` | GS (Alchemy + TimedPlus UPDATE) + النظام الأصلي |
| `SRO_VT_SHARD.._BindingOptionWithItem` | GS (`GetItemBindingOpt` — لكل Alchemy) |
| `SRO_VT_SHARD.._RefSkillByItemOptLevel` / `_RefAbilityByItemOptLevel` | GS (Loader عند الإقلاع + `ConsumeTimedItemPlusRecordsDevill`) |

---

## 4. ✅ أي Packet يشغّل أي Flow (ثلاثي المكونات)

| Opcode | المسار الكامل |
|---|---|
| `0x35FE` | Filter (ExploitFixPackets.cs بعد 0x3012) → GS (CGObjPCCustom.cpp SetFilterSessionKey) |
| `0x3534` | Filter (AlchemyPackets.cs) → GS (HandleNewAlchemyRequest) → UPDATE _Items → 0x5017 response → Filter (SERVER_NEW_ALCHEMY_RESULT) → `Hook_AlchemySuccess` (DatabaseJobQueue) |
| `0x3531/0x3532` | Filter → GS (HandleItemLock/Unlock) → 0x5060/0x5062 → ShardManager (MainProcess.cpp) → 0x5061/0x5063 → كل GS + Filter (INSERT/DELETE DB) |
| `0x8888` | Dashboard/GM → DB INSERT → ShardManager (DatabaseFetchThread) → DELETE row + 0x8888 → GS (CGame::ProcessMessage) |
| `0x7808→0x300C→0xC05/0xC06` | ShardManager (MyHandleMsg) → `EXEC Hook_UniqueSpawn/Kill` + Filter (0x9999/0x10000 via Command_FilterQueue) |
| `0x3C80` | ShardManager (TrainingCampHonorRankService) → GS (HonorRankRuntimeRefresh) → BroadcastHonorRankSnapshot (0x34FE لكل لاعب) |