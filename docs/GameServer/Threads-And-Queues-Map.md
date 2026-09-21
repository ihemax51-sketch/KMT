# KMTGuard — Threads And Queues Map (خريطة الـ Threads والـ Queues)

> **Status tags:** ✅ Confirmed | 🔍 Inference | ⏱️ Needs Profiling | ❓ Unknown
> **النطاق:** GameServer + ShardManager + Filter (الثلاثة)

---

## 1. ✅ GameServer Threads

### 1.1 ConsumeThreadWorker ✅
- **المكان:** `CustomTimedJobManager.cpp` — `CreateConsumeThread()`
- **الخيط:** واحد — `DWORD WINAPI ConsumeThreadWorker(LPVOID)`
- **الدورة:** `while(true) { ConsumeTimedItemPlusRecords(); Sleep(1000); }`
  - داخل `ConsumeTimedItemPlusRecords`: `while(TimedItemList.size()>0)` مع `Sleep(2000)` بداخل while + `for` loop
- **يقرأ SQL:** نعم — `CSqlCon::TryExecNonQuery` (UPDATE `_Items` + DELETE `Item_Locked`)
- **يستخدم `m_connectionstr` المشترك:** نعم — نفس اتصال ODBC مع خيط Network
- **قفل:** `CAutoCriticalSection` محلي حول `SQLExecDirectA` فقط
- **❌ `ConsumeTimedItemPlusRecordsDevill` معلقة** — السطر 368: `// CCustomTimedJobManager::ConsumeTimedItemPlusRecordsDevill();`
- **Race:** `TimedItemList` و `LockedItemList` تُقرآن من خيطين (Network + ConsumeWorker) بلا مزامنة

### 1.2 خيط Network (ServerFramework) ✅
- **المكان:** إطار العمل `CGame::ProcessMessage` + `CGObjPC::ReaderPacket`
- **الوظيفة:** معالجة كل رسائل ShardManager (0x8888, 0x506x) وحزم الكلاينت المخصصة
- **SQL Synchronous:** `HandleNewAlchemyRequest` تنفذ `UpdateItemPlusInDatabase` + `GetItemBindingOpt` على هذا الخيط — **يوقف معالجة كل الحزم الأخرى**

---

## 2. ✅ ShardManager Threads

### 2.1 DatabaseFetchThread ✅
- **المكان:** `ShardManager/vSRO-ShardManager/AsyncGSCommands.cpp` — `InitDatabaseFetch()`
- **الخيط:** واحد — `CreateThread(0, 0, DatabaseFetchThread, 0, 0, 0)`
- **الدورة:** `while(m_IsRunningDatabaseFetch) { READ Command_GameServerQueue; Sleep(1000); }`
- **Connection:** 3 اتصالات: `m_dbLink`, `m_dbLinkHelper`, `m_dbUniqueLog`
- **❌ DELETE بعد كل رسالة:** `qUpdateResult << "DELETE FROM Command_GameServerQueue where ID = " << cID;` — **لا ACK من GS**
- **Retry?** ❌ لا — بمجرد حذف الصف لا يمكن إعادة المحاولة
- **Lock:** `m_IsRunningDatabaseFetch` كـ flag فقط

### 2.2 MainProcess::MyHandleMsg ✅
- **المكان:** `MainProcess.cpp` — Detour على `HANDLE_MSG_FUNC_OFFSET (0x006990D0)`
- **الخيط:** خيط رسائل ShardManager الأصلي
- **Synchronous SQL:** `EXEC Hook_UniqueSpawn/Kill` على نفس خيط الرسائل — **يوقف ShardManager عند تأخير SQL**

### 2.3 InitializeShardManager (بدء التشغيل) ✅
- **المكان:** `DllMain.cpp` — `CreateThread(NULL, 0, InitializeShardManager, NULL, 0, NULL)` في `DLL_PROCESS_ATTACH`

---

## 3. ✅ Filter Threads/Timers

### 3.1 Dotnet Thread Pool (Async Tasks) ✅

| المكوّن | الملف | التكرار | النوع |
|---|---|---|---|
| `_commandTimer` | `DatabaseCommands.cs` | كل 2 ثانية | `System.Timers.Timer` → `ProcessCommands()` |
| `_plannedCommandTimer` | `DatabaseCommands.cs` | كل 3 ثوانٍ | `System.Timers.Timer` → `ProcessPlannedCommands()` |
| `timers` (Timer Updater) | `DatabaseCommands.cs` | كل ثانية | `System.Timers.Timer` → `UpdateTimers()` |
| `Scheduler` loop | `Scheduler.cs` | `Task.Run(SchedulerLoopAsync)` + `ScheduleRefreshSeconds=30` | Background Task loop |
| `RefManager._timer` | `RefManager.cs` | حسب `DynamicRankingRefreshMinutes` (افتراضي 10 دقائق) | `System.Threading.Timer` |
| `KMTGuard-ConnectionTracker` | `ServerManager.cs` | كل ثانية | `System.Threading.Thread` |
| `DelayedJobManager._worker` | `DelayedJobManager.cs` | مستمر مع `AutoResetEvent` (10ms polling) | `Task.Run(ThreadWorkerAsync)` |
| `TeleportFreezeService` | `TeleportFreezeService.cs` | `Task.Run(ProcessQueueAsync)` | Background Task مستمر |
| `DatabaseJobQueue` | `Helpers/DatabaseJobQueue.cs` | 2 Workers عبر `Channel<DatabaseJob>` (Bounded=512) | `Task.Run(ProcessQueueAsync)` لكل Worker |

### 3.2 DatabaseJobQueue ✅
- **المكان:** `Helpers/DatabaseJobQueue.cs`
- **النوع:** `Channel.CreateBounded<DatabaseJob>(Capacity=512, FullMode=Wait)`
- **العمال:** 2 (`WorkerCount = 2`)
- **الاستخدامات المؤكدة:**
  - `TryQueueBackground` (Fire-and-forget): `HandleHwidList`, item lock/unlock INSERT/DELETE, kill logger, `Hook_AlchemySuccess`
  - `RunAsync` (Wait): config save (`Player_SaveConfig`)
  - `RunIdempotentAsync` (Retry 3): session disconnect cleanup
- **Retry:** نعم — لـ `RunIdempotentAsync` (max 3 محاولات مع backoff 250ms/500ms/750ms + jitter)
- **Timeout handling:** يطبع Warning فقط ولا يوجد retry للـ `TryQueueBackground`

### 3.3 DelayedJobManager ✅
- **المكان:** `Helpers/DelayedJobManager.cs`
- **النوع:** `List<DelayedJobItem>` + `AutoResetEvent` (polling 10ms)
- **الاستخدامات المؤكدة:** Self Teleport delay, Alchemy Item Link delay, Event Suit snapshot, Auto Cape, GetUp command
- **لا حدود للحجم** — يمكن أن تكبر القائمة بلا حد إذا تأخر المعالجة

---

## 4. ✅ الخرائط في الذاكرة (بدون قفل في الـ GS)

| الخريطة | الملف | من يكتب | من يقرأ | قفل؟ |
|---|---|---|---|---|
| `g_FilterSessionKeys` | `CGObjPCCustom.cpp` | Network thread (0x35FE) | Network thread (كل 0x35xx) | ✅ `CRITICAL_SECTION` |
| `CSqlCon::LockedItemList` | `sqlCon.cpp` | Game.cpp (0x5061/0x5063) + `LoadLockedItems` | `CGObjPCCustom.cpp` (فحص) + `CustomTimedJobManager` (حذف) | ❌ لا يوجد |
| `CSqlCon::TimedItemList` | `sqlCon.cpp` | Game.cpp (0x5066) + `TimedPlusItems()` | `CGObjPCCustom.cpp` + `CustomTimedJobManager` (حذف) | ❌ لا يوجد |
| `CSqlCon::STimedDevillList` | `sqlCon.cpp` | `TimedDevillPlusItems()` | لا أحد (ConsumeDevill معلق) | ❌ لا يوجد |
| `CSqlCon::s_AttackRestrByMob` | `sqlCon.cpp` | `LoadAttackRestrictionsByMob` | `CGObjPCCustom` (0x7074) + `DamageMeter` | آمن (قراءة فقط بعد الإقلاع) |
| `CSqlCon::AutoCapeList/WorldId` | `sqlCon.cpp` | `ServerAutoCape*` | `CGObjPCCustom` | آمن |
| `SpawnedMobList` | `Game.cpp` | ❓ | ❓ | ❌ |
| `m_LockedItems` (ShardManager) | `AsyncGSCommands.cpp` | `LoadLockedItemList` | — | ❓ لم يُستخدم فعلياً |

---

## 5. ✅ الخرائط/القواميس في الفلتر (Concurrent)

| الخريطة | الملف | النوع | قفل؟ |
|---|---|---|---|
| `AgentSessions` / `GatewaySessions` / `DownloadSessions` | `ServerManager.cs` | `ConcurrentSessionSet` (مخصص — `ConcurrentDictionary + ReaderWriterLockSlim`) | ✅ |
| `_selfTeleportCooldown` | `DatabaseCommands.cs` | `ConcurrentDictionary<int, long>` | ✅ (Atomic `TryAdd/TryUpdate`) |
| `_lastHwidListUpdateTicks` | `CustomGameServerPacketHandler.cs` | `ConcurrentDictionary<int, long>` | ✅ |
| `ActionManager.CreatedTimerList*` | `ActionManager.cs` | `ConcurrentDictionary` | ✅ |
| `RefManager.*` (Rank/Icon/Title...) | `RefManager.cs` | `Dictionary + SemaphoreSlim` | ✅ |
| `_chatFlagsCache` | `ChatPackets.cs` | `ConcurrentDictionary + TTL 5sec` | ✅ |

---

## 6. ⚠️ Race Conditions مؤكدة (من الكود)

| # | المشكلة | المكان | الخطورة |
|---|---|---|---|
| 1 | `TimedItemList` يُقرأ/يُكتب من خيطين بدون قفل | `Game.cpp` 0x5066 + `CustomTimedJobManager` | **High** |
| 2 | `LockedItemList` يُحذف من `CustomTimedJobManager` (ConsumeThread) ويُقرأ من Network thread | `CustomTimedJobManager` DELETE + `CGObjPCCustom` find | **High** |
| 3 | `m_hConn` ODBC مشترك بين `GetItemBindingOpt` (SELECT) و `TryExecNonQuery` (UPDATE) من خيطين | `sqlCon.cpp` + `DbConnection.cpp` | **Medium** (كل استدعاء له `hStmt` خاص، لكن `m_hConn` واحد) |
| 4 | ShardManager يحذف `Command_GameServerQueue` بدون ACK من GS | `AsyncGSCommands.cpp:646` | **Critical** (فقدان أوامر) |
| 5 | `sprintf` بدون buffer → كتابة عشوائية في المكدس | `CustomTimedJobManager.cpp:87` | **Critical** |

---

## 7. ⏱️ Needs Profiling

| المكوّن | القياس المطلوب |
|---|---|
| `ConsumeThreadWorker` حلقة | هل تسبب CPU spike مع 1000+ عنصر في `TimedItemList`؟ |
| `DatabaseJobQueue` حجم الـ Channel | هل يصل إلى `FullMode=Wait` في الذروة (512 عنصراً)؟ |
| `DelayedJobManager` حجم القائمة | هل تكبر بلا حد في حالات الحمل العالي؟ |
| ShardManager `DatabaseFetchThread` | كم صفاً في `Command_GameServerQueue` في المتوسط؟ هل Sleep(1000) كافٍ؟ |
| `PacketHandler.ExecutePipeline` | كم نسخة من الـ Packet تُنشأ لكل حزمة (GC impact)؟ |
| `ServerManager.AgentSessions.ToArray()` | كم مرة تُستدعى في الثانية (لكل Broadcast/Ping/HWID check)؟ |
