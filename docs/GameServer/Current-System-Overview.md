# KMTGuard — GameServer Overview (الخريطة الشاملة)

> **Status tags:** ✅ Confirmed from code | 🔍 Inference | ⏱️ Needs runtime profiling | ❓ Unknown / missing source
> **آخر تحديث:** 2026-08-06
> **الملفات المقروءة بالكامل:** Game.cpp, CGObjPCCustom.cpp (6365 سطر), sqlCon.cpp (1982 سطر), DbConnection.cpp, CustomTimedJobManager.cpp, DamageMeter.cpp, DurabilityControl.cpp, HonorRankRuntimeRefresh.cpp, PartyMonsterControl.cpp, RegionRestrictionMgr.cpp, RegionRestrictionDBSet.cpp, NetHelper.cpp, GObjEvents.cpp.

---

## 1. ✅ Confirmed — موقع الـ GameServer

```
gameserver/source/SilkroadOnline/SR_VIETNAM_SERVICE/v188/Server/SR_GameServer/GameServer/
├── App/src/Game.cpp                     ← CGame::ProcessMessage (0x8888, 0x506x)
├── GameServer/src/KMTGuardCustom/       ← CGObjPCCustom.cpp (6365 سطر), DamageMeter, DurabilityControl,
│                                            GObjEvents, HonorRankRuntimeRefresh, PartyMonsterControl
├── GameServer/src/SqlConnection/        ← sqlCon.cpp, DbConnection.cpp, AutoCriticalSection.cpp
├── GameServer/src/Objects/              ← CustomTimedJobManager, GObjPC, GObjMob, GItem, InstanceItem...
├── GameServer/src/RegionRestrictionMgr.cpp  ← HideName + GroupSpawn hooks
├── GameServer/src/RegionRestrictionDBSet.cpp ← SQL: Security_HideNameRegions
├── GameServer/src/NetHelper.cpp         ← AllocMsg, SendMsgToSM, CopyMsg
├── GameServer/src/GSLog/                ← Logger, LogCustoms, MsgCustom
├── GameServer/src/SettingMgr/           ← NewSettings (.ini reader)
├── GameServer/src/DevNew/               ← CustomNPCEvent.cpp
├── GameServer/src/CmdSrcNet.h           ← ❓ لم يُقرأ (sClientContext, CCmdSrcNet, SecurityGroups)
├── GameServer/src/Storage/              ← ❓ لم يُقرأ
├── GameServer/src/StorageOP/            ← ❓ لم يُقرأ (G*InventoryOP.h)
└── GameServer/src/World/                ← ❓ لم يُقرأ (GameWorldMgr)
```

---

## 2. ✅ Confirmed — الرسم النصي لتدفق البيانات

```text
Client (sro_client.exe)
  ↓ TCP مشفر Silkroad
Filter (KMTGuard Agent) — يعترض، يتحقق من Whitelist/Blacklist، يعدّل الحزم، يمرّر للـ GS
  ↓ TCP مشفر Silkroad
GameServer (SR_GameServer):
  ├── CGObjPC::ReaderPacket (CGObjPCCustom.cpp) ← حزم 0x35xx المخصصة مع Session Key
  │     ├── HandleNewAlchemyRequest (0x3534) — 15 مستوى مع/بدون Proof
  │     ├── HandleItemLockRequest/Unlock (0x3531/0x3532) — مع SearchItem 0xCEED/0xD6ED
  │     ├── HandleSilkPacket (0x3527) — UpdateSilk مباشرة
  │     ├── HandleCosSkill (0x3528) — CGObjCOS_GoldPet::LiveSkill2
  │     ├── HandleFellowSkill (0x3511) — BroadcastMsgToNearbyPlayers + LiveSkill
  │     ├── HandleCustomScrollUsage (0x3530) — SetLiveDeleteItem
  │     ├── HandleItemTranslateRequest (0x3505) — دفع ذهب أو بدون + SetLiveItem
  │     ├── HandleLiveItemChestPacket (0x3506) — AddItem + response 0xA405
  │     ├── HandleDisplayCharInfoRequest (0x3537) — 13 slot send 0x5038 لكل قطعة
  │     ├── HandleGlobalItemLink (0x705C) — 0x5033 with/without item data
  │     ├── HandleAlchemyLinkRequest (0x3533) — 0x5034 Alchemy link broadcast
  │     ├── HandleSelfTeleportRequest (0x3539) — MoveTo نفس المكان (dead/alive check)
  │     ├── HandleFilterTeleportRequest (0x3540) — MoveTo إلى موقع محدد
  │     ├── HandleGrantNameRequest (0x3500) — SetGrantName (Guild Level >= 4)
  │     ├── HandleTitleChangeRequest (0x3501) — UpdateHwan (Berserk/Hwan check)
  │     ├── HandleChangePvpCapeRequest (0x3502) — UpdatePVPCapeType
  │     └── HandleCustomReverseUseRequest (0x3504) — SetLiveDeleteItem + MoveTo special scrolls
  │
  ├── CGame::ProcessMessage (App/src/Game.cpp) ← رسائل من ShardManager
  │     ├── 0x8888 — 28+ أمر (Spawn, Kill, Buff, Teleport, Gold, Silk, Item...)
  │     ├── 0x5061/0x5063 — LockedItemList add/remove
  │     ├── 0x5066/0x5068 — TimedItemList add/remove
  │     └── 0x5069 — Chat redirect (❓ MU'ALLAQ كودها comment بالكامل)
  │
  ├── DamageMeter (Hook) — Detour على CGObjNPC_HandleAggroMap (0x004C44A0)
  │     └── cleanup aggro_map من اللاعبين غير المسموح لهم، ثم 0x5010 لكل PC أونلاين
  │
  ├── DurabilityControl (Hook) — تعطيل نقصان المتانة عند 0x00496E34
  │
  ├── GObjEvents (placeHook) — عند 0x00485AA3 → 0x3571 للفلتر
  │
  ├── HonorRankRuntimeRefresh (replaceOffset) — Hook على TrainingCampQueryCompletion
  │     └── BroadcastHonorRankSnapshot بث 0x34FE لجميع اللاعبين
  │
  ├── PartyMonsterControl — تعديل بايتات spawn rate في 0x00558F20 و 0x005608E2
  │
  └── CCustomTimedJobManager::ConsumeThreadWorker — خيط منفصل
        ├── ConsumeTimedItemPlusRecords (كل 3 ثواني)
        └── ConsumeTimedItemPlusRecordsDevill (❌ معلقة السطر 368)
```

---

## 3. ✅ Confirmed — اتصال DB في الـ GS

- **ODBC:** `SQLAllocHandle → SQLDriverConnectA` (اتصال واحد m_hConn على m_hEnv)
- **🔍 Inference:** `CAutoCriticalSection` يحمي فقط `TryExecNonQuery` (SQLExecDirectA) — لا يحمي `GetItemBindingOpt` التي تستخدم `AllocStmt` خاص بها على نفس m_hConn
- **✅ Confirmed:** `AllocStmt` تنشئ `hStmt` جديد دائماً → Race على hStmt غير مرجح، لكن Race على m_hConn محتمل عند تنفيذ SELECT و UPDATE في نفس الوقت من خيطين
- **✅ Confirmed:** كل SQL تُبنى بـ `sprintf` (لا Parameterization)
- **✅ Confirmed:** `GetItemBindingOpt` تُنفَّذ Synchronous داخل خيط Packet Handler لكل عملية Alchemy/Lock — SELECT على `_BindingOptionWithItem`
- **✅ Confirmed:** `UpdateItemPlusInDatabase` تُنفَّذ Synchronous لكل عملية Alchemy success/fail

---

## 3.5 حالة المكونات (Audit Status)

| المكوّن | الملف | حالة التدفق | حالة الاعتماديات | ملاحظات |
|---|---|---|---|---|
| ProcessMessage | `Game.cpp` | ✅ Flow Fully Mapped | ⚠️ Needs Additional Dependencies | يعتمد على `GObjPC.cpp` (GetCharObjById body), `GameWorldMgr.cpp` (MoveTo) غير مقروءين |
| ReaderPacket + Handlers | `CGObjPCCustom.cpp` (6365 سطر) | ✅ Flow Fully Mapped | ⚠️ Needs Additional Dependencies | يعتمد على `InstanceItem.cpp` (SetPlus/RefreshItemStats), `GObjMob.cpp` (CreateMob), `GameWorldMgr.cpp` (MoveTo) غير مقروءين |
| SQL Layer | `sqlCon.cpp` (1982 سطر) | ✅ Flow Fully Mapped | ⚠️ Needs Additional Dependencies | `DbConnection.cpp` مقروء لكن `NewSettings.cpp` (مسار ini) + `AutoCriticalSection.cpp` (body) غير مقروءين |
| ODBC Wrapper | `DbConnection.cpp` | ✅ Flow Fully Mapped | ✅ Dependencies Fully Mapped | — |
| DamageMeter | `DamageMeter.cpp` | ✅ Flow Fully Mapped | ⚠️ Needs Additional Dependencies | `RegionRestrictionMgr.cpp` (CanMobInteractWithPlayer body), `GObjMob.cpp` |
| DurabilityControl | `DurabilityControl.cpp` | ✅ Flow Fully Mapped | ✅ Dependencies Fully Mapped | — |
| HonorRank | `HonorRankRuntimeRefresh.cpp` | ✅ Flow Fully Mapped | ⚠️ Needs Additional Dependencies | نظام CTrainingCampManager الأصلي غير مقروء |
| PartyMonster | `PartyMonsterControl.cpp` | ✅ Flow Fully Mapped | ✅ Dependencies Fully Mapped | — |
| RegionRestriction | `RegionRestrictionMgr.cpp` | ✅ Flow Fully Mapped | ✅ Dependencies Fully Mapped | — |
| RegionRestrictionDB | `RegionRestrictionDBSet.cpp` | ✅ Flow Fully Mapped | ✅ Dependencies Fully Mapped | — |
| NetHelper | `NetHelper.cpp` | ✅ Flow Fully Mapped | ✅ Dependencies Fully Mapped | — |
| GObjEvents | `GObjEvents.cpp` | ✅ Flow Fully Mapped | ✅ Dependencies Fully Mapped | — |
| TimedJobManager | `CustomTimedJobManager.cpp` | ✅ Flow Fully Mapped | ⚠️ Needs Additional Dependencies | `InstanceItem.cpp` (SetPlus), `DbConnection.cpp` (thread safety of AllocStmt) |
| InstanceItem | `Objects/InstanceItem.cpp` | ❌ Not Read | ❌ Not Read | **Critical** — أساس SetPlus/RefreshItemStats |
| GameWorldMgr | `World/GameWorldMgr.cpp` | ❌ Not Read | ❌ Not Read | **High** — MoveTo, إدارة العوالم |

---

## 4. 🔍 Inference/✅ Confirmed — تدفق Session Key

1. ✅ الفلتر: `session.GameServerPacketKey = Convert.ToHexString(RandomNumberGenerator.GetBytes(32))` (Session.cs سطر 83)
2. ✅ `0x3012` GameReady → الفلتر يرسل `0x35FE` ب `Program.GSPacketKey` + `session.GameServerPacketKey` بعد 250ms delay
3. ✅ الـ GS: `ReaderPacket` يستقبل `0x35FE` → `SetFilterSessionKey` إذا `bootstrapKey == GSKEY ("KMTGUARD710632TEST")`
4. ✅ كل حزمة 0x35xx تحمل Key → `ValidateFilterSessionKey` تتحقق في خريطة `g_FilterSessionKeys`
5. ✅ Prune عند 1024 مفتاح: يمسح الشخصيات غير المتصلة

---

## 5. ❓ Unknown / needs verification

| البند | السبب |
|---|---|
| CmdSrcNet.h (sClientContext, CCmdSrcNet, SecurityGroups) | لم يُقرأ — قد يحتوي معالجة حزم إضافية |
| Storage/ + StorageOP/ (جميع ملفات G*InventoryOP.h) | لم تُقرأ |
| GameWorldMgr.cpp (World/GameWorldMgr.cpp) | لم يُقرأ |
| CustomNPCEvent.cpp كاملاً | لم يُقرأ |
| GSLog/* | لم يُقرأ بالكامل |
| SR_ShardManager داخل gameserver | هل هو مطابق لـ ShardManager/vSRO-ShardManager؟ |
| AutoCriticalSection.cpp | لم يُقرأ (لكن نمطه واضح من RegionRestrictionDBSet) |
| InstanceItem.cpp (SetPlus, RefreshItemStats) | لم يُقرأ |
