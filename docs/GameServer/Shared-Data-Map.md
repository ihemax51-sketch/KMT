# KMTGuard — Shared Data Map (خريطة البيانات المشتركة)

> **Status tags:** ✅ Confirmed | 🔍 Inference | ⏱️ Needs Profiling | ❓ Unknown
> **النطاق:** GameServer + ShardManager + Filter + Dashboard

---

## 1. `KMTGuard.dbo.Item_Locked` — من يقرأ/يكتب

| الجهة | يقرأ | يكتب | كيف |
|---|---|---|---|
| **GameServer** | ✅ `LoadLockedItems` (إقلاع) + `LockedItemList` (ذاكرة) | ❌ لا — يستقبل تحديثات عبر 0x5061/0x5063 | `sqlCon.cpp` + `Game.cpp` |
| **ShardManager** | ✅ `LoadLockedItemList` (مرة واحدة فقط — `m_LockedItems`) | ❌ لا يكتب — يبث فقط | `AsyncGSCommands.cpp` |
| **Filter** | ❌ لا | ✅ `INSERT/DELETE` عبر `DatabaseJobQueue` (Async) | `CustomGameServerPacketHandler.cs` |
| **Dashboard** | ❓ Unknown | ❓ قد يعدّل مباشرة | — |

- **Source of Truth:** `Item_Locked` DB
- **خطورة:** بيانات قديمة بين GS (ذاكرة) و DB — يحتاج 0x5061/0x5063 للتحديث

---

## 2. `SRO_VT_SHARD.._Items` — OptLevel

| الجهة | يقرأ | يكتب |
|---|---|---|
| **GameServer** (KMTGuard) | `GetItemBindingOpt` (غير مباشر عبر `_BindingOptionWithItem`) | ✅ `UPDATE OptLevel` (Alchemy + TimedPlus) |
| **GameServer** (النظام الأصلي) | ✅ يقرأ/يكتب باستمرار | ✅ يكتب OptLevel (السوق/البيع/الترقية الأصلية) |
| **Filter** | `Item_GetInfo` SP | لا يكتب `OptLevel` مباشرة |

- **Race Condition مؤكد:** `HandleNewAlchemyRequest` و `ConsumeTimedItemPlusRecords` قد يكتبان OptLevel لنفس ID64 من خيطين مختلفين.

---

## 3. `KMTGuard.dbo.Item_TimedPlus` / `Item_TimedDevilPlus`

| الجهة | يقرأ | يكتب |
|---|---|---|
| **GameServer** | ✅ `TimedPlusItems()` / `TimedDevillPlusItems()` (إقلاع) | ✅ `DELETE` — لكن `Item_TimedPlus` لا يُحذف فعلياً، و `Item_TimedDevilPlus` يُحذف لكن الكود معلق |
| **ShardManager** | ❌ | ❌ |
| **Filter** | ❌ | ❓ |

---

## 4. `KMTGuard.dbo.System_Settings`

| الجهة | يقرأ | يكتب |
|---|---|---|
| **GameServer** | ✅ 3 Loaders (Durability, GreenBook, PartyMonster) — **مرة واحدة عند الإقلاع** | ❌ |
| **Filter** | ✅ `_serverSettings.InitServerSettings()` | ❌ (يقرأ عند الإقلاع ثم يعتمد على الكاش) |
| **AdminDesktop** | ✅ | ✅ يعدّل مباشرة |

- **خطورة:** أي تغيير في `System_Settings` عبر AdminDesktop لا يظهر في الـ GS حتى إعادة التشغيل.

---

## 5. `KMTGuard.dbo.Party_Members`

| الجهة | يقرأ | يكتب |
|---|---|---|
| **GameServer** | ✅ `CSqlCon::Patches()` — `SELECT TOP(1) PartyID` | ❌ |
| **Filter** | ❓ | ✅ `DELETE` عند disconnect في `CleanupDatabaseStateAsync` |

---

## 6. `KMTGuard.dbo.Auth_HWIDs` (HWID List)

| الجهة | يقرأ | يكتب |
|---|---|---|
| **GameServer** | ❌ | ❌ |
| **Filter** | ✅ `Auth_UpdateHWID` عبر `HandleHwidList` (TryQueueBackground) | ✅ `UPDATE Active=0` عند disconnect |
| **Dashboard** | ❓ | ❓ |

---

## 7. `Command_GameServerQueue`

| الجهة | يقرأ | يكتب |
|---|---|---|
| **ShardManager** | ✅ `DatabaseFetchThread` كل ثانية | ✅ **DELETE مباشر** بعد كل إرسال — **لا ACK** |
| **Dashboard/GM Tool** | ❓ | ✅ تضيف صفوفاً (INSERT) |
| **GameServer** | ❌ | ❌ |

- **خطورة Critical:** ShardManager يحذف الصف ثم يرسل 0x8888. إذا فشل GS، **يُفقد الأمر نهائياً**.

---

## 8. `Command_FilterQueue`

| الجهة | يقرأ | يكتب |
|---|---|---|
| **Filter** | ✅ `ProcessCommands` كل 2 ثانية — `SELECT ... WHERE Status=1` | ✅ `UPDATE Status=0` بعد التنفيذ |
| **Dashboard/SP** | ❓ | ✅ تضيف صفوفاً |
| **Stored Procedures** | `Item_AddChest` تضيف CommandID 17 | — |

---

## 9. `Command_PlannedQueue`

| الجهة | يقرأ | يكتب |
|---|---|---|
| **Filter** | ✅ `ProcessPlannedCommands` كل 3 ثوانٍ | ✅ `UPDATE Status=0` |
| **Dashboard** | ❓ | ✅ تضيف صفوفاً |

---

## 10. `SRO_VT_SHARD.._BindingOptionWithItem`

| الجهة | يقرأ | يكتب |
|---|---|---|
| **GameServer** | ✅ `GetItemBindingOpt` — **Synchronous SELECT لكل Alchemy** | ❌ |
| **Filter/ShardManager** | ❌ | ❌ |

- **فرصة Cache:** جدول شبه ثابت — قراءة عند الإقلاع توفر SELECT على كل حزمة.

---

## ملخص المخاطر

| الجدول | بيانات قديمة | Race | كتابة فوق أخرى | خطورة |
|---|---|---|---|---|
| `Item_Locked` | ✅ | ✅ (Filter Async vs GS Memory) | ✅ | **High** |
| `_Items` (OptLevel) | ✅ | ✅ (خيطان في GS) | ✅ | **Critical** |
| `System_Settings` | ✅ (إقلاع فقط) | لا | لا | Medium |
| `Command_GameServerQueue` | ❌ (يُحذف فوراً) | ❌ | ❌ | **Critical** (فقدان أوامر) |
| `Item_TimedDevilPlus` | ✅ (لا يُستهلك) | لا | لا | **Critical** (معلق) |