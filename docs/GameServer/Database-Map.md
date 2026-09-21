# KMTGuard — GameServer Database Map (خريطة قواعد البيانات)

> **Status tags:** ✅ Confirmed | 🔍 Inference | ⏱️ Needs Profiling | ❓ Unknown
> **مصدر المعلومات:** قراءة مباشرة من `sqlCon.cpp` (1982 سطر), `CGObjPCCustom.cpp`, `CustomTimedJobManager.cpp`, `RegionRestrictionDBSet.cpp`, `DbConnection.cpp`.

---

## 1. ✅ Confirmed — بنية الاتصال

| البند | القيمة | الحالة |
|---|---|---|
| نوع الاتصال | ODBC (`SQLDriverConnectA` + `SQLAllocHandle`) | ✅ |
| اتصال واحد `m_hConn` لكل GS | `CDbConnection::Connect()` | ✅ |
| قفل | `CAutoCriticalSection` يغلف فقط `TryExecNonQuery` (SQLExecDirectA) — لا يحمي `GetItemBindingOpt` | ✅ |
| Parameterization | ❌ — كل SQL تبني `sprintf` (لا `SQLBindParameter`) | ✅ |
| `AllocStmt` جديد لكل استدعاء | ✅ — `SQLAllocStmt(m_hConn, &hStmt)` | ✅ |
| `FreeStmt` يحرر `hStmt` | ✅ — `SQLFreeStmt(hStmt, SQL_CLOSE)` + `SQLFreeHandle` | ✅ |
| خطأ `sprintf` بدون buffer | ❌ `CustomTimedJobManager.cpp` السطر 87 | ✅ (Bug مؤكد) |

---

## 2. ✅ — جدول كل تعاملات SQL المؤكدة

### أ. قراءات عند الإقلاع (14 Loader)

| # | الدالة | الاستعلام | DB | الجدول | النوع | عند الفشل |
|---|---|---|---|---|---|---|
| 1 | `LoadDisableDurabilitySetting` | `SELECT TOP (1) Value FROM System_Settings WHERE SettingName = 'DisableDurability'` | KMTGuard | `System_Settings` | R | يعيد false → يبقي الافتراضي false |
| 2 | `LoadDisableGreenBookSetting` | `SELECT TOP (1) Value FROM System_Settings WHERE SettingName = 'DisableGreenBook'` | KMTGuard | `System_Settings` | R | يعيد false |
| 3 | `LoadPartyMonsterSettings` | `SELECT SettingName, Value FROM System_Settings WHERE SettingName IN ('EnablePartyMonsterSpawn', 'PartyMonsterMinimumMembers', 'PartyMonsterSpawnRate')` | KMTGuard | `System_Settings` | R | يعيد افتراضيات ثابتة |
| 4 | `LoadLockedItems` | `SELECT ItemID64 FROM Item_Locked` | KMTGuard | `Item_Locked` | R | يطبع خطأ |
| 5 | `LoadGameServerSettings` | `SELECT ID, SettingName, Value FROM System_GameServerSettings` | KMTGuard | `System_GameServerSettings` | R | يعيد false |
| 6 | `LoadFortressDPSInfo` | `SELECT StructObjID FROM _ServerFortressDpsInfo` | KMTGuard | `_ServerFortressDpsInfo` | R | يعيد false |
| 7 | `ServerAutoCapebyWorldID` | `SELECT ID, WorldID FROM _ServerAutoCapebyWorldID` | KMTGuard | `_ServerAutoCapebyWorldID` | R | يعيد false |
| 8 | `ServerAutoCapebyRegionID` | `SELECT ID, RegionID FROM AutoCapeRegions` | KMTGuard | `Style_AutoCapeRegions` | R | يعيد false |
| 9 | `LoadRefSkillByItemOptLevel` | `SELECT Link, RefSkillID FROM SRO_VT_SHARD.._RefSkillByItemOptLevel` | SRO_VT_SHARD | `_RefSkillByItemOptLevel` | R | يعيد false |
| 10 | `LoadRefAbilitybyItemOptLevel` | `SELECT ID, RefItemID, ItemOptLevel FROM SRO_VT_SHARD.._RefAbilityByItemOptLevel` | SRO_VT_SHARD | `_RefAbilityByItemOptLevel` | R | يعيد false |
| 11 | `TimedPlusItems` | `SELECT CharID,OrjPlus,ID64,EndTime FROM Item_TimedPlus` | KMTGuard | `Item_TimedPlus` | R | يعيد false |
| 12 | `TimedDevillPlusItems` | `SELECT CharID,OrjPlus,ID64,EndTime FROM Item_TimedDevilPlus` | KMTGuard | `Item_TimedDevilPlus` | R | يعيد false |
| 13 | `LoadAttackRestrictionsByMob` | `SELECT MobRefObjID, OnlyOffJob, OnlyOnJob, OnlyByThief, OnlyByTrader, OnlyStrPlayer, OnlyIntPlayer, ...` | KMTGuard | `Security_AttackRules` (❓ اسم مؤكد جزئياً) | R | يعيد false |
| 14 | `GetCustomNpcInteractionRecords` | `SELECT ID, CodeName128, InteractionID FROM ___CustomNpcInteraction` | KMTGuard | `___CustomNpcInteraction` | R | يعيد false |

### ب. قراءات أثناء التشغيل

| # | الدالة | الاستعلام | DB | الجدول | النوع | Sync? | المستدعي |
|---|---|---|---|---|---|---|---|
| 15 | `GetItemBindingOpt` | `SELECT nOptValue FROM SRO_VT_SHARD.._BindingOptionWithItem with(nolock) where nItemDBID = %lld and bOptType = 2` | SRO_VT_SHARD | `_BindingOptionWithItem` | R | ✅ Sync (خيط Network) | كل HandleNewAlchemyRequest، HandleItemLock/Unlock، SERVER_REINFORCE_RESPONSE |
| 16 | `Patches` | `SELECT TOP(1) PartyID FROM Party_Members WITH (NOLOCK) WHERE CharID = %d` | KMTGuard | `Party_Members` | R | ✅ Sync | `CSqlCon::Patches()` |

### ج. كتابات أثناء التشغيل

| # | الدالة | الاستعلام | DB | الجدول | النوع | Sync/Async | TX? | عند الفشل |
|---|---|---|---|---|---|---|---|---|
| 17 | `UpdateItemPlusInDatabase` | `UPDATE SRO_VT_SHARD.._Items SET OptLevel = %d WHERE ID64 = %lld` | SRO_VT_SHARD | `_Items` | W | ✅ Sync (Network Thread) | ❌ | `printf` فقط — DB لا تتحدث |
| 18 | `ConsumeTimedItemPlusRecords` | `UPDATE SRO_VT_SHARD.._Items SET OptLevel = %d WHERE ID64 = %lld` | SRO_VT_SHARD | `_Items` | W | ⏱️ Async (ConsumeThread) | ❌ | `printf` فقط |
| 19 | `ConsumeTimedItemPlusRecords` | `DELETE FROM Item_Locked WHERE ItemID64 = %lld` | KMTGuard | `Item_Locked` | W | ⏱️ Async | ❌ | `printf` فقط |
| 20 | `ConsumeTimedItemPlusRecordsDevill` | `UPDATE SRO_VT_SHARD.._Items SET OptLevel = %d WHERE ID64 = %lld` | SRO_VT_SHARD | `_Items` | W | ⏱️ Async (❌ معلق) | ❌ | — (لا تُستدعى) |
| 21 | `ConsumeTimedItemPlusRecordsDevill` | `DELETE FROM Item_TimedDevilPlus where ID64 = %lld` | KMTGuard | `Item_TimedDevilPlus` | W | ⏱️ Async (❌ معلق) | ❌ | — |
| 22 | `GameServerInitialized` | `EXEC [KMTGuard].[dbo].[Hook_GameServerStart] '%s'` | KMTGuard | (SP) | W | ✅ Sync | ❌ | `printf` |
| 23 | `TryExecNonQuery` (مخصص) | تستخدم لكل SQL مباشر | — | — | W/R | ✅ Sync مع `CAutoCriticalSection` | ❌ | ترجع false |

### د. قراءة في ShardManager (خارج GS لكنها مرتبطة)

| # | المكان | الاستعلام | DB | الجدول |
|---|---|---|---|---|
| 24 | `RegionRestrictionDBSet::GetHideNameRegionRecords` | `SELECT ID, RegionID FROM Security_HideNameRegions` | KMTGuard | `Security_HideNameRegions` |

---

## 3. تصنيف العمليات حسب الحساسية

### A. Critical Immediate Write
- ✅ `UPDATE _Items SET OptLevel` في `UpdateItemPlusInDatabase` — اللاعب ينتظر
- ✅ `EXEC Hook_GameServerStart` — إعلان بدء العالم

### B. Ordered Deferred Write
- ✅ `DELETE FROM Item_Locked` بعد UPDATE الناجح — يجب الحفاظ على الترتيب
- ✅ `DELETE FROM Item_TimedDevilPlus` بعد UPDATE — نفس الشيء (❌ لكنه معلق)
- ✅ `DELETE FROM Item_TimedPlus` — ❌ مفقود في الكود (لا يُحذف الجدول)

### C. Async Safe (Logs/Analytics)
- ❌ لا توجد — كل الـ Logs عبر الفلتر/ShardManager

### D. Read Cache Candidate
- ✅ `SELECT FROM _RefSkillByItemOptLevel + _RefAbilityByItemOptLevel` — تُقرأ عند الإقلاع فقط (Cache بالفعل)

### D. Read Cache Candidate (Strong — يحتاج تحقق إضافي قبل التنفيذ)
- 🔍 `SELECT FROM _BindingOptionWithItem` (GetItemBindingOpt) — **Strong Cache Candidate**
  - يُستدعى 2-3 مرات لكل Alchemy operation
  - **غير مصنف Safe بعد** لأن: (1) لم يُتحقق من Dashboard/GM Tools التي تعدّل `_BindingOptionWithItem`، (2) لم تُقرأ أجسام SP التي قد تكتب في هذا الجدول (`Hook_AlchemySuccess`, `Item_AddChest`...)، (3) لم يُتحقق من Web Panel/API خارجي قد يعدّل الجدول
  - يحتاج مراجعة كل writers قبل تنفيذ الـ Cache

### E. Unknown
- ❓ `SELECT MobRefObjID, ...` الاستعلام الكامل في `LoadAttackRestrictionsByMob` — لم يُقرأ الجدول بالكامل

---

## 4. ملاحظات حرجة (مؤكدة من الكود)

1. ❌ **`sprintf` بدون buffer** — `CustomTimedJobManager.cpp:87` — Crash محتمل
2. ❌ **لا Transaction بين UPDATE و DELETE** — كل عملية مستقلة
3. ❌ **`GetItemBindingOpt` لا تستخدم قفل `TryExecNonQuery`** — تخلق `hStmt` خاص بها على `m_hConn` المشترك ← Race Condition محتمل مع `ConsumeThreadWorker` الذي يستخدم `TryExecNonQuery`
4. ✅ **`Item_TimedPlus` لا يُحذف من الجدول أبداً** في `ConsumeTimedItemPlusRecords` — فقط `Item_Locked` يُحذف
5. ✅ **`Item_TimedDevilPlus` يُحذف** لكن الكود معلق بالكامل
6. ✅ **الفلتر يكتب `Item_Locked` عبر `DatabaseJobQueue`** — Async → الـ GS قد لا يرى التحديث حتى رسالة `0x5061`
7. ✅ **كل SQL تُبنى بـ `sprintf`** — SQL Injection محتمل من `GrantName`, `SkillCodeName`, `strKillerName` (تمر عبر ShardManager)