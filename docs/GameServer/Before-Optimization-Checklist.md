# KMTGuard — Before Optimization Checklist (قائمة ما قبل التحسين)

> **Status tags:** ✅ Confirmed | 🔍 Inference | ⏱️ Needs Profiling | ❓ Unknown
> **النطاق:** GameServer + ShardManager + Filter + Database + Dashboard (خماسي المكونات)

---

## 1. تعديلات لا تغيّر Behavior (أولوية قصوى — منخفضة الخطورة) ✅

| # | التعديل | المكان | التأثير | الحالة |
|---|---|---|---|---|
| 1 | إصلاح `sprintf` بدون buffer | `CustomTimedJobManager.cpp:87` | سطر واحد — يصحح Crash محتمل | ✅ مؤكد |
| 2 | إضافة قفل حول `TimedItemList` و`LockedItemList` | `Game.cpp` (0x5066/0x5068/0x5061/0x5063) + `CustomTimedJobManager.cpp` | لا يغيّر التدفق — يحمي من Race | ✅ مؤكد |
| 3 | إضافة `DELETE FROM Item_TimedPlus` بعد نجاح UPDATE | `CustomTimedJobManager.cpp::ConsumeTimedItemPlusRecords` | يكمل النية الأصلية للكود | ✅ مؤكد |
| 4 | إضافة Cache لـ `_BindingOptionWithItem` في `GetItemBindingOpt` | `sqlCon.cpp` | يوفر SELECT لكل Alchemy operation | ✅ مؤكد |
| 5 | تحسين `ConsumeThreadWorker` بـ `if TimedItemList.empty() Sleep(5000)` | `CustomTimedJobManager.cpp` | توفير CPU عندما لا توجد عناصر | ✅ مؤكد |

---

## 2. تعديلات منخفضة الخطورة ✅

| # | التعديل | المكان | الملاحظات |
|---|---|---|---|
| 6 | إعادة تفعيل `ConsumeTimedItemPlusRecordsDevill()` | `CustomTimedJobManager.cpp:368` | ❓ يجب التحقق من الدالة كاملة أولاً |
| 7 | إصلاح `NetHelper::CopyMsg` لنسخ payload فعلياً | `NetHelper.cpp:135-147` | الكود المعلق موجود — يحتاج تفعيل |
| 8 | إضافة `DELETE FROM Item_TimedDevilPlus` مع Transaction | `CustomTimedJobManager.cpp` | للحفاظ على Data Consistency |

---

## 3. تعديلات تحتاج Testing ✅

| # | التعديل | المكان | لماذا تحتاج Testing |
|---|---|---|---|
| 9 | تحويل `HandleNewAlchemyRequest` من Synchronous إلى خلفية | `CGObjPCCustom.cpp` | تغيير توقيت النجاح/الفشل للاعب |
| 10 | إضافة Transaction حول UPDATE `_Items` + DELETE `Item_Locked`/`Item_TimedDevilPlus` | `CustomTimedJobManager.cpp` | اختبار قطع الاتصال أثناء التنفيذ |
| 11 | إعادة تمكين معالجة `0x5069` (Chat Item Link) | `Game.cpp` | كود كبير معلّق — يحتاج إعادة بناء |
| 12 | إضافة ACK/Retry لـ `Command_GameServerQueue` | `AsyncGSCommands.cpp` (ShardManager) | **يتطلب بروتوكول جديد بين GS و SM** |

---

## 4. تعديلات تمس Database consistency (عليها قيود) ✅

| # | التعديل | المكان | القيود |
|---|---|---|---|
| 13 | Parameterize كل SQL (`sprintf` → `SQLBindParameter`) | `CGObjPCCustom.cpp`, `CustomTimedJobManager.cpp`, `sqlCon.cpp` | تغيير كبير على ODBC |
| 14 | مزامنة كتابة `Item_Locked` بين الفلتر وShardManager | خارج نطاق GS (فلتر/ShardManager) | يتطلب تنسيق بين مكوّنين |
| 15 | إضافة `UPDATE Status` بدلاً من `DELETE` في ShardManager لـ `Command_GameServerQueue` | `AsyncGSCommands.cpp` | تغيير تصميم — يحتاج ACK من GS |

---

## 5. تعديلات عالية الخطورة — لا تنفَّذ قبل فهم كامل ⚠️

| # | التعديل | المكان | لماذا عالي الخطورة |
|---|---|---|---|
| 16 | تغيير تدفق `Command_GameServerQueue` أو إضافة Retry | `AsyncGSCommands.cpp` | **يفقد الأوامر عند فشل GS حاليًا** |
| 17 | تغيير `GSKEY` أو نظام مفاتيح الجلسة | `CGObjPCCustom.cpp` | **يكسر كل الحزم المخصصة** |
| 18 | إعادة هندسة الاتصال DB في الـ GS (Connection Pool) | `sqlCon.cpp` + `DbConnection.cpp` | يتصل بالـ GS كله — **أي خطأ يسقط الـ GS** |
| 19 | إزالة `sprintf` واعتماد Parameterized Queries بالكامل | جميع ملفات GS | **تغيير واسع النطاق — يحتاج إعادة اختبار كل SQL** |

---

## 6. Command_GameServerQueue — التدفق الدقيق (من الكود)
| المرحلة | السطر | الملف | الوصف |
|---|---|---|---|
| **1. SELECT** | 72 | `AsyncGSCommands.cpp` | `qSelectActions << "SELECT ID, Action_ID, Data1... FROM Command_GameServerQueue"` |
| **2. ExecuteQuery** | 76 | `AsyncGSCommands.cpp` | `m_dbLink.sqlCmd.ExecuteQuery(...)` |
| **3. FetchData (loop)** | 80 | `AsyncGSCommands.cpp` | `while (m_dbLink.sqlCmd.FetchData())` — صف بصف |
| **4. GetData** | 89-90 | `AsyncGSCommands.cpp` | `sqlCmd.GetData(1, ..., &cID, ...); GetData(2, ..., &cActionID, ...);` |
| **5. AllocMsgForGS** | 105 | `AsyncGSCommands.cpp` | `CShardNetManager::AllocMsgForGS()` — يخصص رسالة 0x8888 |
| **6. Write params** | 108-110 | `AsyncGSCommands.cpp` | `*pMsg << BYTE(1); *pMsg << CharID; pMsg->WriteStringA(GrantNameData1);` |
| **7. BroadcastMsgToGameServers** | 113 | `AsyncGSCommands.cpp` | `CShardNetManager::BroadcastMsgToGameServers(pMsg);` — إرسال فوري لكل GS |
| **8. DELETE (بعد الإرسال)** | 644-646 | `AsyncGSCommands.cpp` | `qUpdateResult << "DELETE FROM Command_GameServerQueue where ID = " << cID; m_dbLinkHelper.sqlCmd.ExecuteQuery(...);` |
| **9. Sleep** | 652 | `AsyncGSCommands.cpp` | `Sleep(1000);` — ينتظر ثانية ثم يعيد الدورة |

**نتيجة التدفق:**
- DELETE يحدث **بعد** `BroadcastMsgToGameServers` (وليس قبله)
- لا يوجد أي انتظار لنتيجة الإرسال (Fire-and-forget)
- لا يوجد ACK من GameServer إلى ShardManager
- لا يوجد Retry — إذا فشل GS في استقبال أو معالجة 0x8888، الصف يُحذف نهائياً
- `BroadcastMsgToGameServers` هو استدعاء أصلي لـ ShardManager الأصلي — `غير مؤكد` إذا كان ينتظر TCP ACK أو مجرد إرسال في الذاكرة

---

## 6. ملفات ومصادر غير مقروءة (قائمة كاملة ومحدثة)

### GameServer — غير مقروء
| # | الملف/المسار | الأولوية | ملاحظات |
|---|---|---|---|
| 1 | `Objects/InstanceItem.cpp` | **Critical** | أساس `SetPlus(byte)`, `RefreshItemStats()` — كل Alchemy/TimedPlus يعتمد عليه |
| 2 | `World/GameWorldMgr.cpp` | **High** | إدارة العوالم والطبقات |
| 3 | `CmdSrcNet.h` | **High** | `sClientContext` (JID, AgentSessionId, ClientSessionId, SecurityGroups), `CCmdSrcNet` |
| 4 | `Common/src/*` | **High** | كود مشترك غير محدد المحتوى |
| 5 | `ServerCommon/src/*` | **Medium** | كود خادم مشترك |
| 6 | `DevNew/CustomNPCEvent.cpp` (body) | **Medium** | تفاعلات NPC المخصصة |
| 7 | `Objects/GObjMob.cpp` (full body) | **Medium** | `CreateMob` فقط معروف؛ باقي الدوال غير مقروءة |
| 8 | `SettingMgr/NewSettings.cpp` (full) | **Medium** | لتحديد ملف ini الفعلي لسلسلة الاتصال |
| 9 | `Storage/*.cpp` + `StorageOP/*.h` | **Medium** | `GStorage`, `G*InventoryOP` (عمليات المخزون) |
| 10 | `GSLog/*` (Logger, LogCustoms, MsgCustom) | **Low** | نظام التسجيل الداخلي |
| 11 | `SqlConnection/AutoCriticalSection.cpp` (body) | **Low** | نمطه واضح من `RegionRestrictionDBSet.cpp` |

### Filter — غير مقروء
| # | الملف | ملاحظات |
|---|---|---|
| 12 | `Scheduler.cs` | نظام الجدولة — تمت قراءة جزء منه فقط |
| 13 | `Startup.cs` | بدء التشغيل والأمان — الأجزاء الحرجة فقط |
| 14 | `RefManager.cs` (Complete) | 1178 سطر — تمت قراءة ~80% |
| 15 | `GatewayServer/GatewayServer.cs` | لم يُقرأ |
| 16 | `DownloadServer/DownloadServer.cs` | لم يُقرأ |
| 17 | Packet handlers المتبقية: `GuildPackets.cs`, `COSPackets.cs`, `JobPackets.cs`, `StallPackets.cs`, `Title_IconManagers.cs`, `NewReverse.cs`, `AcademyPackets.cs`, `CharacterSelection.cs`, `CharAction.cs`, `CharDataPackets.cs` | لم تُقرأ |

### ShardManager
| # | الملف | الحالة |
|---|---|---|
| — | جميع الملفات الأساسية مقروءة بالكامل ✅ | — |

### Database
| # | البند | الحالة |
|---|---|---|
| 18 | أجسام Stored Procedures (110 SP) | ❓ غير موجودة في السورس — تُدار في SQL Server فقط |
| 19 | Migrations (70+ ملف في `database/migrations/`) | ❓ تم قراءة 1 فقط (`20260727_item_chest_integrity`) |
| 20 | `database/tests/*.sql` (اختبارات) | ❓ لم تُقرأ |
| 21 | `database/backups/*.sql` (نسخ استرجاع) | ❓ لم تُقرأ |
| 22 | `database/package-versions.psd1` | ❓ لم يُقرأ |

### Dashboard / GM Tools
| # | المسار | الحالة |
|---|---|---|
| 23 | `KMTGuard.AdminDesktop/MainWindow.Dashboard.cs` | ❓ لم يُقرأ — يعدّل System_Settings وجداول أخرى |
| 24 | `KMTGuard.AdminDesktop/Services/*` | ❓ لم تُقرأ |

---

## 7. ملخص حالة كل ملف (مع تصنيفات دقيقة)

### تصنيفات الحالة:
- **Flow Fully Mapped** — كل تدفقات الكود في هذا الملف تم تتبعها وفهمها
- **Dependencies Fully Mapped** — كل اعتماديات هذا الملف على ملفات/جداول/SP أخرى معروفة
- **Needs Additional Dependencies** — الملف مقروء لكن اعتمادياته على ملفات أخرى غير مقروءة بعد (مذكورة)
- **Needs Runtime Profiling** — الملف مقروء لكن يحتاج قياسات أداء في بيئة حقيقية
- **Not Read** — لم يُقرأ بعد

### GameServer
| الملف | حالة التدفق | حالة الاعتماديات | يحتاج Profiling? | الاعتماديات غير المقروءة |
|---|---|---|---|---|
| `CGObjPCCustom.cpp` (6365 سطر) | ✅ Flow Fully Mapped | ⚠️ Needs Additional Dependencies | ⏱️ Alchemy latency | `InstanceItem.cpp` (SetPlus/RefreshItemStats), `GObjMob.cpp` (CreateMob), `GameWorldMgr.cpp` (MoveTo) |
| `CustomTimedJobManager.cpp` (372 سطر) | ✅ Flow Fully Mapped | ⚠️ Needs Additional Dependencies | ⏱️ Loop CPU | `InstanceItem.cpp` (SetPlus), `DbConnection.cpp` (AllocStmt thread safety) |
| `sqlCon.cpp` (1982 سطر) | ✅ Flow Fully Mapped | ⚠️ Needs Additional Dependencies | ⏱️ Connection contention | `DbConnection.cpp` (AllocStmt impl for race eval), `NewSettings.cpp` (ini file path) |
| `DbConnection.cpp` (124 سطر) | ✅ Flow Fully Mapped | ✅ Dependencies Fully Mapped | لا | — |
| `DamageMeter.cpp` (123 سطر) | ✅ Flow Fully Mapped | ⚠️ Needs Additional Dependencies | ⏱️ Aggro map size | `RegionRestrictionMgr.cpp` (CanMobInteractWithPlayer body), `GObjMob.cpp` |
| `DurabilityControl.cpp` (102 سطر) | ✅ Flow Fully Mapped | ✅ Dependencies Fully Mapped | لا | — |
| `HonorRankRuntimeRefresh.cpp` (116 سطر) | ✅ Flow Fully Mapped | ⚠️ Needs Additional Dependencies | ⏱️ Broadcast size | نظام CTrainingCampManager الأصلي (❓ غير مقروء) |
| `PartyMonsterControl.cpp` (129 سطر) | ✅ Flow Fully Mapped | ✅ Dependencies Fully Mapped | لا | — |
| `RegionRestrictionMgr.cpp` (194 سطر) | ✅ Flow Fully Mapped | ✅ Dependencies Fully Mapped | لا | — |
| `RegionRestrictionDBSet.cpp` (76 سطر) | ✅ Flow Fully Mapped | ✅ Dependencies Fully Mapped | لا | — |
| `NetHelper.cpp` (163 سطر) | ✅ Flow Fully Mapped | ✅ Dependencies Fully Mapped | لا | — |
| `GObjEvents.cpp` (60 سطر) | ✅ Flow Fully Mapped | ✅ Dependencies Fully Mapped | لا | — |
| `Game.cpp` (979 سطر) | ✅ Flow Fully Mapped | ⚠️ Needs Additional Dependencies | لا | `GObjPC.cpp` (GetCharObjById full), `GameWorldMgr.cpp` |
| `InstanceItem.cpp` | ❌ Not Read | ❌ Not Read | — | — |
| `GameWorldMgr.cpp` | ❌ Not Read | ❌ Not Read | — | — |

### ShardManager
| الملف | حالة التدفق | حالة الاعتماديات |
|---|---|---|
| `MainProcess.cpp` (290 سطر) | ✅ Flow Fully Mapped | ✅ Dependencies Fully Mapped |
| `AsyncGSCommands.cpp` (721 سطر) | ✅ Flow Fully Mapped | ✅ Dependencies Fully Mapped |
| `TrainingCampHonorRankService.cpp` (145 سطر) | ✅ Flow Fully Mapped | ✅ Dependencies Fully Mapped |
| `DllMain.cpp` (38 سطر) | ✅ Flow Fully Mapped | ✅ Dependencies Fully Mapped |
| `Settings.cpp` (199 سطر) | ✅ Flow Fully Mapped | ✅ Dependencies Fully Mapped |

### Filter (الملفات الحرجة)
| الملف | حالة التدفق | حالة الاعتماديات | يحتاج Profiling? |
|---|---|---|---|
| `Session.cs` (820 سطر) | ✅ Flow Fully Mapped | ✅ Dependencies Fully Mapped | ⏱️ GC (GetBytes, ToArray) |
| `PacketHandler.cs` (306 سطر) | ✅ Flow Fully Mapped | ✅ Dependencies Fully Mapped | ⏱️ Packet copy per handler |
| `DatabaseCommands.cs` (2117 سطر) | ✅ Flow Fully Mapped | ⚠️ Needs Additional Dependencies | لا (SP bodies missing) |
| `DatabaseJobQueue.cs` (235 سطر) | ✅ Flow Fully Mapped | ✅ Dependencies Fully Mapped | ⏱️ Queue capacity |
| `CustomGameServerPacketHandler.cs` (1288 سطر) | ✅ Flow Fully Mapped | ✅ Dependencies Fully Mapped | لا |
| `AlchemyPackets.cs` (577 سطر) | ✅ Flow Fully Mapped | ✅ Dependencies Fully Mapped | لا |

---

## 8. _BindingOptionWithItem Cache Assessment

| البند | التصنيف |
|---|---|
| **الملف** | `sqlCon.cpp` — `CSqlCon::GetItemBindingOpt(INT64 ID64)` |
| **الاستعلام** | `SELECT nOptValue FROM SRO_VT_SHARD.._BindingOptionWithItem WITH (NOLOCK) WHERE nItemDBID = %lld AND bOptType = 2` |
| **التكرار** | 2-3 مرات لكل عملية Alchemy (قبل وبعد كل فرع) |
| **استقرار البيانات** | 🔍 **Strong Cache Candidate** (يحتاج تحقق إضافي) |
| **لماذا ليس Safe Cache بعد:** | 1. غير معروف إذا كان Dashboard/GM Tool يعدّل `_BindingOptionWithItem` مباشرة |
| | 2. غير معروف إذا كانت SP Bodies تعدّل هذا الجدول (مثل `Hook_AlchemySuccess`, `Item_AddChest`) |
| | 3. غير معروف إذا كان هناك Web Panel أو API خارجي يعدّل الجدول |
| **التوصية الحالية** | **Strong Cache Candidate** — يحتاج مراجعة كل من يكتب في `_BindingOptionWithItem` قبل تنفيذ الـ Cache |

---

## خاتمة
- **الملفات المقروءة بالكامل مع Flow Fully Mapped:** 22 ملف
- **الملفات التي تحتاج اعتماديات إضافية:** 6 ملفات (CGObjPCCustom، CustomTimedJobManager، sqlCon، DamageMeter، HonorRankRuntimeRefresh، Game.cpp)
- **الملفات غير المقروءة:** 25+ ملف/مصدر (GameServer: 11، Filter: 10، Database: 5، Dashboard: كامل)
- **الـ SP bodies:** 110 إجراء — غير موجودة في السورس

**تحذير:** أي تعديل على الأنظمة Critical يجب أن يكمل قراءة الاعتماديات غير المقروءة أولاً.
