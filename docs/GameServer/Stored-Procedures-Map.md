# KMTGuard — GameServer Stored Procedures Map (خريطة الـ Stored Procedures)

> مراجعة تحليلية فقط. **لم يعدَّل أي كود ولم تنفَّذ أي SQL.**
> هذه الخريطة مبنية على استدعاءات الـ SP الموجودة في كود الـ GameServer `gameserver/` فقط.

---

## مقدمة

الـ GameServer نفسه يستدعي **عدداً قليلاً جداً** من الـ Stored Procedures مباشرة. معظم الـ SP تُستدعى من:
- الفلتر (`.NET`) — خارج نطاق هذه المراجعة.
- ShardManager (`ShardManager/vSRO-ShardManager`) — يُستدعى من داخل الـ GS عبر رسائل فقط.

**هذا القسم يغطي فقط ما يظهر في `gameserver/`:**

---

## 1. الـ SP المستدعاة مباشرة من كود الـ GameServer

### 1.1 `KMTGuard.dbo.Hook_GameServerStart`

| البند | القيمة |
|---|---|
| الاسم الكامل | `[KMTGuard].[dbo].[Hook_GameServerStart]` |
| قاعدة البيانات | `KMTGuard` |
| مكان الاستدعاء | `sqlCon.cpp` — `CSqlCon::GameServerInitialized()` |
| طريقة الاستدعاء | `EXEC [KMTGuard].[dbo].[Hook_GameServerStart] '%s'` (مع مسار العملية `path.c_str()`) |
| Parameters | `@path varchar` (مسار ملف الـ EXE — من `GetProcessInstanceId`/`path.c_str()`) |
| Return Value | لا يوجد (غير مسجل في الكود) |
| الجداول التي تقرأها | **محتوى الـ SP غير موجود في السورس** — `غير مؤكد` |
| الجداول التي تعدّلها | **محتوى الـ SP غير موجود في السورس** — `غير مؤكد` |
| Transaction | `غير مؤكد` |
| يستدعي SP أخرى | `غير مؤكد` |
| الأنظمة المعتمدة عليها | بدء العالم (World Startup) |
| يجب تنفيذها فوراً | نعم — عند إقلاع الـ GS |
| إذا تأخرت | إعلان بدء العالم يتأخر |
| إذا فشلت | `TryExecNonQuery` يرجع `false` ويُطبع خطأ فقط — **الـ GS يستمر** |
| يمكن إعادة تنفيذها بأمان | `غير مؤكد` — تحتاج فحص جسم SP |
| Idempotent؟ | `غير مؤكد` — إذا كانت تسجل حدث بدء فإعادة تنفيذها قد تكرر الحدث |
| Dashboard/GM Tool تستدعيها | `غير مؤكد` |
| مستوى الخطورة عند تعديلها | **عالي** — تُستدعى مرة واحدة لكل GS عند الإقلاع |

---

## 2. الـ SP التي يستدعيها الـ ShardManager (خارج `gameserver/` لكنها مرتبطة به)

> هذه ليست مستدعاة من كود الـ GS مباشرة، لكنها تخدم أنظمة الـ GS. مذكورة للاكتمال لأنها جزء من التدفق الإجمالي.

### 2.1 `KMTGuard.dbo.NPC_Sync`

| البند | القيمة |
|---|---|
| الاسم الكامل | `[KMTGuard].[dbo].[NPC_Sync]` |
| قاعدة البيانات | `KMTGuard` |
| مكان الاستدعاء | `sqlCon.cpp` — `CSqlCon::Patches()` (كود مكتشف في السورس بـ `sprintf("EXEC [KMTGuard].[dbo].[NPC_Sync] %d, %d, '%s'", ...)`) |
| Parameters | `@GameID int`, `@RefObjID int`, `@CodeName varchar` |
| تأثر | مزامنة قائمة NPC بين الـ GS والـ DB |

### 2.2 `KMTGuard.dbo.Hook_UniqueSpawn` / `Hook_UniqueKill`

| البند | القيمة |
|---|---|
| الاسم الكامل | `[KMTGuard].[dbo].[Hook_UniqueSpawn]` / `[KMTGuard].[dbo].[Hook_UniqueKill]` |
| مكان الاستدعاء | `ShardManager/vSRO-ShardManager/Network/MainProcess.cpp` — `MyHandleMsg` عند `0x7808` يحتوي `0x300C` مع `0xC05` (Spawn) أو `0xC06` (Kill) |
| Parameters | `@RefObjID int` (Spawn)؛ `@RefObjID int, @KillerName nvarchar` (Kill) |
| التنفيذ | **Synchronous على نفس خيط رسائل الـ ShardManager** — أي تأخير SQL يؤخر الـ ShardManager كله |
| الأنظمة | Unique History — بث للفلتر عبر `0x9999/0x10000` |
| إذا فشلت | يُطبع خطأ فقط بلا Retry |

### 2.3 `SRO_VT_SHARD.dbo._TRAINING_CAMP_UPDATEHONORRANK`

| البند | القيمة |
|---|---|
| الاسم الكامل | `[SRO_VT_SHARD].[dbo].[_TRAINING_CAMP_UPDATEHONORRANK]` |
| مكان الاستدعاء | `ShardManager/vSRO-ShardManager/TrainingCampHonorRankService.cpp` — `Refresh(requestId)` |
| Parameters | لا يوجد (تُستدعى بلا معاملات) |
| Return | `@Result int` (يُقرأ عبر `SELECT @Result`) — **القيمة <= 0 تعتبر فشلاً** |
| النتيجة | عند النجاح يبث `0x3C80` subtype `0x0B` لكل الـ GameServers → الـ GS يعيد بث الرتب للاعبين |
| Transaction | `غير مؤكد` |
| الأنظمة | Honor Rank Runtime Refresh |
| يجب تنفيذها فوراً | نعم — تُستدعى من `Command_GameServerQueue Action_ID=200` |
| إذا تأخرت | تحديث الرتب يتأخر |
| إذا فشلت | يُسجَّل في `HonorRankRefreshHistory` بهيئة `Failed` ويُعاد المحاولة في الوجهة التالية |
| Idempotent؟ | `غير مؤكد` |

---

## 3. الـ SP غير الموجودة في السورس

**محتوى جسم كل الـ SP التالية غير موجود داخل `gameserver/`:**

| الـ SP | المصدر الحالي |
|---|---|
| `Hook_GameServerStart` | `غير مؤكد` — تُدار في قاعدة بيانات `KMTGuard` |
| `NPC_Sync` | `غير مؤكد` — تُدار في قاعدة بيانات `KMTGuard` |
| `Hook_UniqueSpawn` | `غير مؤكد` — تُدار في قاعدة بيانات `KMTGuard` |
| `Hook_UniqueKill` | `غير مؤكد` — تُدار في قاعدة بيانات `KMTGuard` |
| `_TRAINING_CAMP_UPDATEHONORRANK` | `غير مؤكد` — تُدار في `SRO_VT_SHARD` |

**ملاحظة:** ملف `database/migrations/*.sql` قد يحتوي أجسام بعضها (مثل `Hook_GameServerStart` في `20260715_shardmanager_gameserver_integration.sql`)، لكنني لم أقرأ هذا الملف بعد — `غير مؤكد ويحتاج تتبع إضافي`.

---

## 4. ما الذي يجب مراجعته قبل تعديل أي SP

| SP | ملفات يجب قراءتها |
|---|---|
| `Hook_GameServerStart` | `sqlCon.cpp` (الاستدعاء) + `database/migrations/20260715_shardmanager_gameserver_integration.sql` |
| `NPC_Sync` | `sqlCon.cpp` (الاستدعاء) |
| `Hook_UniqueSpawn/Kill` | `ShardManager/vSRO-ShardManager/Network/MainProcess.cpp` |
| `_TRAINING_CAMP_UPDATEHONORRANK` | `ShardManager/vSRO-ShardManager/TrainingCampHonorRankService.cpp` |

---

## 5. ملاحظة حرجة

الـ GameServer لا يعتمد على الـ SP في مسارات الأيتم والـ Gold والـ Silk — هذه تتم مباشرة بـ SQL في `CGObjPCCustom.cpp` أو عبر `0x8888` من ShardManager (الذي يحذف الصف بـ DELETE بعد البث). أي تعديل مستقبلي يجب أن يأخذ بعين الاعتبار أن **مصدر الحقيقة للأوامر هو `Command_GameServerQueue` التي يحذفها ShardManager**.