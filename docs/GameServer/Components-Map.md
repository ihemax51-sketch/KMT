# KMTGuard — GameServer Components Map (خريطة المكونات)

> **Status tags:** ✅ Confirmed from code | 🔍 Inference | ⏱️ Needs runtime profiling | ❓ Unknown / missing source

---

## جدول المكونات الكامل

| # | المكوّن | المسار | الوظيفة | Entry Point | أهم Classes | Threads/Workers | الخدمات | قواعد البيانات | المسؤول | ماذا يحدث عند توقفه |
|---|---|---|---|---|---|---|---|---|---|---|
| 1 | **CGame::ProcessMessage** | `App/src/Game.cpp` | ✅ معالجة 28+ نوع أمر من ShardManager (0x8888, 0x506x) | `CGame::ProcessMessage(CMsg* pMsg)` | `CGame`, `CGObjPC`, `CGObjMob` | لا (يُستدعى من خيط Network الخاص بـ ServerFramework) | ShardManager (رسائل عبر SM) | KMTGuard, SRO_VT_SHARD (غير مباشر عبر أوامر) | Network/Gameplay | **Critical** — أي أمر Queue لا يُنفَّذ؛ ShardManager يحذف الصف بعد الإرسال → فقدان أوامر نهائي |
| 2 | **CGObjPC::ReaderPacket** | `KMTGuardCustom/CGObjPCCustom.cpp` (6365 سطر) | ✅ معالجة 21 حزمة مخصصة من الفلتر مع Session Key validation | `CGObjPC::ReaderPacket(CMsg*)` | `CGObjPC`, `CSqlCon`, `CRegionAttackRestrictionsMgr` | لا (خيط Network) | Filter (0x35xx) | SRO_VT_SHARD._Items, _BindingOptionWithItem, _RefObjCommon | Gameplay/Network | حزم الفلتر المخصصة تُتجاهل بلا استجابة |
| 3 | **CSqlCon** | `SqlConnection/sqlCon.cpp` (1982 سطر) | ✅ كل SQL في الـ GS: 14 Loader عند الإقلاع + `GetItemBindingOpt` + `TryExecNonQuery` + `GameServerInitialized` + `Patches` | `CSqlCon::Initialize()` | `CSqlCon`, `CDbConnection`, `CAutoCriticalSection` | لا Worker منفصل — تحميلات عند الإقلاع فقط | SQL Server (ODBC) | KMTGuard: System_Settings, Item_Locked, System_GameServerSettings, Item_TimedPlus, Item_TimedDevilPlus, Party_Members, AutoCapeRegions, _ServerFortressDpsInfo, _ServerAutoCapebyWorldID, Security_HideNameRegions, ___CustomNpcInteraction, Security_AttackRules | Database | الـ GS لا يقرأ تنظيمه؛ LockedItemList فارغة → لا حماية قفل |
| 4 | **CDbConnection** | `SqlConnection/DbConnection.cpp` (124 سطر) | ✅ غلاف ODBC: `AllocStmt`, `FreeStmt`, `Connect`, `Disconnect` | `CSqlCon::Initialize()` | `CDbConnection` | — | SQL Server | — (غلاف) | Database | — |
| 5 | **CAutoCriticalSection** | `SqlConnection/AutoCriticalSection.cpp/h` ❓ | ✅ قفل حول `SQLExecDirectA` في `TryExecNonQuery` فقط | داخل `TryExecNonQuery` | `CAutoCriticalSection` | — | — | — | Thread Safety | 🔍 بدون القفل: Race Condition على `m_hConn` |
| 6 | **CCustomTimedJobManager** | `Objects/CustomTimedJobManager.cpp` (372 سطر) | ✅ Thread خلفي: يعيد Plus الأيتمات الموقوتة + ينظف جداول DB | `CreateConsumeThread()` → `ConsumeThreadWorker` | `CCustomTimedJobManager` | **ConsumeThreadWorker** (خيط واحد) | SRO_VT_SHARD._Items + KMTGuard.Item_TimedPlus/Devil/Item_Locked | KMTGuard, SRO_VT_SHARD | Gameplay/Database | المواد الموقوتة تبقى +Plus مرتفع للأبد (❌ Devill لا تُستهلك أبداً) |
| 7 | **CRegionAttackRestrictionsMgr** | `RegionRestrictionMgr.cpp` (194 سطر) | ✅ HideName hooks (3 hooks) + TowerDefense + FreeForAll + `CanMobInteractWithPlayer` | `CRegionRestrictionsMgr::Initialize()` | `CRegionRestrictionsMgr` | — | يقرأ `Security_AttackRules` (عبر `CSqlCon`) + `Security_HideNameRegions` (عبر `CRegionRestrictionDBSet`) | KMTGuard | Gameplay | ❓ Uniques STR/INT قد تُهاجم من لاعبين غير مؤهلين |
| 8 | **CRegionRestrictionDBSet** | `RegionRestrictionDBSet.cpp` (76 سطر) | ✅ قراءة `Security_HideNameRegions` من DB | `GetHideNameRegionRecords` | `CRegionRestrictionDBSet` | — | SQL | KMTGuard.Security_HideNameRegions | Database | HideName لا تعمل |
| 9 | **CDamageMeter** | `KMTGuardCustom/DamageMeter.cpp` (123 سطر) | ✅ Detour على `CGObjNPC::HandleAggroMap` — ينظف aggro list من اللاعبين الممنوعين + يرسل 0x5010 DPS للفلتر | `CDamageMeter::Initialize()` | `CDamageMeter`, `CGObjPC`, `IGObj` | — | Filter (0x5010 DPS packet) | لا | Gameplay/Network | عدّاد DPS لا يعمل |
| 10 | **DurabilityControl** | `KMTGuardCustom/DurabilityControl.cpp` (102 سطر) | ✅ Hook عند 0x00496E34 — يمنع نقصان المتانة مع السماح بالزيادة (repair/stones) | `DurabilityControl::Initialize()` | `DurabilityControl` | — | — | تقرأ `System_Settings` عند الإقلاع (DisableDurability) | Gameplay | توقف بدون تأثير (الحالة عند الإقلاع) |
| 11 | **GObjEvents** | `KMTGuardCustom/GObjEvents.cpp` (60 سطر) | ✅ placeHook على 0x00485AA3 → `CheckRegionNeedChange` → إرسال 0x3571 للفلتر | `CGObjEvents::Initialize()` | `CGObjEvents`, `CGObj` | — | Filter (0x3571 region change) | لا | Network/Gameplay | ❓ الفلتر لا يعرف تغيير المنطقة → قيود HWID/Region لا تعمل |
| 12 | **HonorRankRuntimeRefresh** | `KMTGuardCustom/HonorRankRuntimeRefresh.cpp` (116 سطر) | ✅ Hook على TrainingCampQueryCompletion → بث 0x34FE لكل اللاعبين بعد تحديث HonorRank | `HonorRankRuntimeRefresh::Initialize()` | `HonorRankRuntimeRefresh`, `CGObjPC` | — | ShardManager (تحديث بعد Action_ID 200) | SRO_VT_SHARD (SP `_TRAINING_CAMP_UPDATEHONORRANK` عبر ShardManager) | Gameplay | ❓ الرتب لا تتحدث للاعبين في الزمن الحقيقي |
| 13 | **PartyMonsterControl** | `KMTGuardCustom/PartyMonsterControl.cpp` (129 سطر) | ✅ تعديل immediate bytes في الذاكرة (0x00558F20, 0x005608E2) لتغيير minimum members و spawn rate | `PartyMonsterControl::Initialize()` | `PartyMonsterControl` | — | — | تقرأ `System_Settings` عند الإقلاع (EnablePartyMonsterSpawn...) | Gameplay | ❓ إعدادات تبقى حتى إعادة تشغيل GS فقط |
| 14 | **CNetHelper** | `NetHelper.cpp` (163 سطر) | ✅ غلاف: AllocMsg (word, encrypted), AllocMsgForBroadcast, FreeMsg, SendMsgToSM, CopyMsg**, SendMsgToParty/Guild/TrainingCamp/PC | `CNetHelper::Initialize()` | `CNetHelper` | — | ShardManager (SendMsgToSM) + النظام الأصلي | لا | Network | لا يمكن إرسال رسائل للـ SM أو تخصيص رسائل |
| 15 | **NewSettings** | `SettingMgr/NewSettings.cpp/h` ❓ | ✅ قراءة إعدادات DatabaseConnectionString + DisableDurability + PartyMonster... | `CSqlCon::Initialize()` | `CNewSettings` | — | ملف ini (`❓ unknown which file`) | KMTGuard.System_Settings | Configuration | — |
| 16 | **CustomNPCEvent** | `DevNew/CustomNPCEvent.cpp/h` ❓ | 🔍 تفاعلات NPC مخصصة مع ___CustomNpcInteraction | ❓ | ❓ | — | ❓ | KMTGuard.___CustomNpcInteraction | Gameplay | ❓ |
| 17 | **CmdSrcNet.h** | `CmdSrcNet.h` ❓ | 🔍 sClientContext (JID/AgentSessionId/ClientSessionId) + CCmdSrcNet (NewMsg/FreeMsg) | ❓ | `sClientContext`, `CCmdSrcNet` | ❓ | ❓ | ❓ | Network | ❓ |
| 18 | **GObjMob** | `Objects/GObjMob.cpp/h` ❓ | ✅ `CreateMob()` تستخدم بكثرة من 0x8888 و 0x3538 | `CGObjMob::CreateMob(refObjId, worldId, regionId, x, y, z, radius)` | `CGObjMob` | — | — | لا | Gameplay | أمر Spawn من ShardManager لا يُنفَّذ |
| 19 | **InstanceItem** | `Objects/InstanceItem.cpp/h` ❓ | ✅ `SetPlus`, `RefreshItemStats` — أساس كل Alchemy/Lock/Timed | داخل `CGObjPC::HandleNewAlchemyRequest`, `ConsumeTimedItemPlusRecords`... | `CInstanceItem` | — | — | لا | Gameplay | — |

---

## 2. ✅ Confirmed — اعتماديات المكونات (من الكود المقرؤ)

```text
CGame::ProcessMessage (Game.cpp)
├── CGObjPC (GetCharObjById, TeleportToTown, UpdatePVPCapeType, Ressurect, UpdateSilk, UpdateGold, LiveSkill)
├── CGObjMob (CreateMob)
├── CGObjCOS_GoldPet (LiveSkill2)
├── CRegionAttackRestrictionsMgr (ConfigureTowerDefense / FreeForAll)
├── CSqlCon::LockedItemList (add/remove from 0x5061/0x5063)
├── CSqlCon::TimedItemList (add/remove from 0x5066/0x5068)
└── CNetHelper::SendMsgToSM (0x5067 timed plus remove broadcast)

CGObjPC::ReaderPacket (CGObjPCCustom.cpp)
├── CSqlCon::LockedItemList / TimedItemList / STimedDevillList (فحص قبل كل عملية)
├── CSqlCon::TryExecNonQuery (UpdateItemPlusInDatabase — كتابة _Items)
├── CSqlCon::GetItemBindingOpt (قراءة _BindingOptionWithItem — SELECT لكل Alchemy)
├── CRegionAttackRestrictionsMgr (CanMobInteractWithPlayer عند 0x7074)
├── CNetHelper::AllocMsg / BindStreamBufferWithMsg / FlushStreamBufferMsg (بناء رسائل)
├── CNetHelper::SendMsgToSM (0x5060 Lock, 0x5062 Unlock, 0x5068 Chat)
├── CGObjCOS_GoldPet (HandleCosSkill)
├── CGObjMob::CreateMob (0x3538 spawn unique)
└── g_pCGame->GetObjByGameID (الوصول لكل الكائنات)

DamageMeter (Detour على CGObjNPC_HandleAggroMap)
├── CRegionAttackRestrictionsMgr (CanMobInteractWithPlayer — prune aggro)
├── IGObj / CGObjPC (قراءة aggro_map وعناوين الذاكرة)
└── CGObjPC::AllocMsgForPeer / SendMsgToPeer (إرسال 0x5010 للفلتر)

HonorRankRuntimeRefresh (Hook على TrainingCampQueryCompletion)
├── g_pCGame->m_mapPcCharId (لكل اللاعبين الأونلاين)
├── CGObjPC::AllocMsgForPeer (0x34FE لكل لاعب)
└── (نظام CTrainingCampManager الأصلي — ❓ غير مقروء)
```

---

## 3. ⏱️ Needs Runtime Profiling — تقييمات الأداء

| المكوّن | القياس المطلوب | المشتبه فيه |
|---|---|---|
| `GetItemBindingOpt` | عدد مرات الاستدعاء لكل Alchemy operation | SELECT على كل عملية — قد يكون 2-3 مرات لكل Alchemy (فحص قبل وبعد) |
| `UpdateItemPlusInDatabase` | وقت تنفيذ UPDATE | Synchronous على خيط Network — يؤثر على Latency |
| `DamageMeter::MyCGObjNPC_HandleAggroMap` | حجم `aggro_map` في الذروة | قد يكون كبيراً — max 15 entry per packet |
| `HonorRankRuntimeRefresh::BroadcastHonorRankSnapshot` | عدد اللاعبين × حجم `CMsg* cachedRanking` | بث لجميع اللاعبين دفعة واحدة |
| `CustomTimedJobManager::ConsumeTimedItemPlusRecords` | حجم `TimedItemList` | حلقة while+for — O(n) لكل item مع Sleep(2000) بداخل while |

---

## 4. ❓ Unknown — مكونات غير مقروءة

| المكوّن | المسار | الأولوية للقراءة |
|---|---|---|
| GameWorldMgr | `World/GameWorldMgr.cpp` | عالية — أساس إدارة العوالم |
| InstanceItem (SetPlus, RefreshItemStats) | `Objects/InstanceItem.cpp` | عالية — أساس Alchemy/Lock |
| GObjMob (full body) | `Objects/GObjMob.cpp` | متوسطة — CreateMob فقط معروف |
| GStorage, G*InventoryOP | `Storage/`, `StorageOP/` | متوسطة |
| CustomNPCEvent (full) | `DevNew/CustomNPCEvent.cpp` | متوسطة |
| GSLog (Logger, LogCustoms, MsgCustom) | `GSLog/` | منخفضة |
| AutoCriticalSection (body) | `SqlConnection/AutoCriticalSection.cpp` | منخفضة — نمطه واضح |
| CmdSrcNet.h | `CmdSrcNet.h` | عالية — قد يحتوي معالجات حزم |
| NewSettings (body) | `SettingMgr/NewSettings.cpp` | متوسطة — لمعرفة ملف ini |
| Common/src كلها | `Common/src/` | عالية |
| ServerCommon/src كلها | `../../ServerCommon/src/` | متوسطة |