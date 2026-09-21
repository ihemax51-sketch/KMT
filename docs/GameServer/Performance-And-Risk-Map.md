# KMTGuard — Performance And Risk Map (خريطة الأداء والمخاطر)

> **Status tags:** ✅ Confirmed from code | 🔍 Inference | ⏱️ Needs Profiling | ❓ Unknown

---

## 1. ❌ Bugs مؤكدة من الكود

### Bug 1: sprintf بدون buffer — Undefined Behavior
| البند | القيمة |
|---|---|
| **الملف** | `gameserver/.../Objects/CustomTimedJobManager.cpp` |
| **الدالة** | `ConsumeTimedItemPlusRecords()` |
| **الأسطر** | 87, 230, 304 (3 occurrences) |
| **المقتطف** | `sprintf("%s - What the fuck ?! Query failed, query = %s", __FUNCTION__, szQuery);` |
| **النوع** | Crash + Memory Corruption |
| **الخطورة** | **Critical** |
| **Evidence level** | ✅ Confirmed from code |
| **شروط الحدوث** | عندما يفشل `CSqlCon::TryExecNonQuery(szQuery)` (فشل UPDATE أو DELETE SQL) — خطأ شبكة، timeout، deadlock، إلخ |
| **التأثير** | يكتب format string + `szQuery` contents إلى عنوان مكدس عشوائي (لأن `sprintf` بدون buffer وسيط يتوقع وجود وسيط buffer أول). هذا Undefined Behavior — قد يسبب crash فوري أو تلف بيانات صامت. |

### Bug 2: ConsumeTimedItemPlusRecordsDevill غير مستدعاة
| البند | القيمة |
|---|---|
| **الملف** | `gameserver/.../Objects/CustomTimedJobManager.cpp` |
| **الدالة** | `ConsumeThreadWorker(LPVOID lpParam)` |
| **السطر** | 368 |
| **المقتطف** | `//    CCustomTimedJobManager::ConsumeTimedItemPlusRecordsDevill();` |
| **النوع** | Data Consistency (Feature disabled) |
| **الخطورة** | **Critical** |
| **Evidence level** | ✅ Confirmed from code |
| **شروط الحدوث** | موجود منذ build time — أي Timed Devil Plus item لن يُعاد OptLevel الأصلي له أبداً |
| **التأثير** | `STimedDevillList` تُحمَّل من DB لكن لا تُستهلك — لاعب يستخدم Devil Plus مؤقت سيبقى مرتفعاً للابد |

### Bug 3: ShardManager يحذف Command_GameServerQueue بدون ACK
| البند | القيمة |
|---|---|
| **الملف** | `ShardManager/vSRO-ShardManager/AsyncGSCommands.cpp` |
| **الدالة** | `DatabaseFetchThread()` |
| **الأسطر** | 93-636 (switch يصنع packet ويرسله) → 644-646 (DELETE) |
| **المقتطف** | `BroadcastMsgToGameServers(pMsg);` (سطور 113, 136, 154...)  ثم `qUpdateResult << "DELETE FROM ...Command_GameServerQueue where ID = " << cID;` (سطر 645) |
| **النوع** | Data Loss |
| **الخطورة** | **Critical** |
| **Evidence level** | ✅ Confirmed from code |
| **شروط الحدوث** | إذا فشل GameServer في استقبال أو معالجة 0x8888 (GS معطل، overloaded، packet loss) — الصف يُحذف ولا يوجد ACK/Retry |
| **التأثير** | فقدان أوامر نهائي (Silk, Gold, Items, Buffs...) دون تنفيذها ودون معرفة المرسل |
| **ملاحظة**: DELETE يحدث **بعد** BroadcastMsgToGameServers، لكن لا يوجد انتظار لنتيجة أو ACK |

### Bug 4: Chat Item Link 0x5069 معطل بالكامل
| البند | القيمة |
|---|---|
| **الملف** | `gameserver/.../App/src/Game.cpp` |
| **الدالة** | `CGame::ProcessMessage()` |
| **الأسطر** | 715-963 |
| **المقتطف** | `case 0x5069: { ... }` — كل الكود بين الأقواس مُعلَّق بـ `/* ... */` |
| **النوع** | Feature incomplete |
| **الخطورة** | Medium |
| **Evidence level** | ✅ Confirmed from code |
| **التأثير** | شات خاص مع item link لا يصل للمستلم داخل الـ GS — ShardManager يبث 0x5069 لكن الـ GS لا يعالجها |

### Bug 5: CopyMsg لا تنسخ payload
| البند | القيمة |
|---|---|
| **الملف** | `gameserver/.../NetHelper.cpp` |
| **الدالة** | `CopyMsg(CMsg* pSrcMsg)` |
| **السطور** | 135-148 |
| **المقتطف** | `CMsg* pNewMsg = CNetHelper::AllocMsg(pSrcMsg->GetMsgId(), false); return pNewMsg;` (الكود المعلق بين 139-145 كان سينسخ البيانات لكنه comment) |
| **النوع** | Feature incomplete |
| **الخطورة** | Low |
| **Evidence level** | ✅ Confirmed from code |

---

## 2. ⚠️ Performance Issues (نمط الكود)

### Perf 1: SQL Synchronous داخل Packet Handler
| البند | القيمة |
|---|---|
| **الملف** | `gameserver/.../KMTGuardCustom/CGObjPCCustom.cpp` |
| **الدالة** | `HandleNewAlchemyRequest()` |
| **الأسطر** | 1606-6246 (كل فروع الألكيمي) |
| **التفاصيل** | كل `if-else` (120+ فرع) ينتهي بـ `this->UpdateItemPlusInDatabase(pTargetItem->ID64, pTargetItem->InstanceItem->OptLevel);` الذي ينفذ UPDATE عبر ODBC بشكل Synchronous |
| **النوع** | Latency + Blocking |
| **الخطورة** | **High** |
| **Evidence level** | ✅ Confirmed from code |
| **شروط الحدوث** | كل حزمة Alchemy من لاعب — يوقف معالجة كل الحزم الأخرى على نفس GS thread |
| **المطلوب للقياس** | ⏱️ Needs Runtime Profiling — كم ms تأخذ `TryExecNonQuery` في الذروة |

### Perf 2: GetItemBindingOpt SELECT متكرر
| البند | القيمة |
|---|---|
| **الملف** | `gameserver/.../SqlConnection/sqlCon.cpp` |
| **الدالة** | `CSqlCon::GetItemBindingOpt(INT64 ID64)` |
| **التفاصيل** | تُستدعى 2-3 مرات لكل عملية Alchemy (قبل وبعد كل فرع) — تُنفَّذ SELECT على `_BindingOptionWithItem` |
| **النوع** | Latency |
| **الخطورة** | Medium |
| **Evidence level** | ✅ Confirmed from code |
| **المطلوب للقياس** | ⏱️ Needs Runtime Profiling — تردد الاستدعاء لكل Alchemy |

### Perf 3: ConsumeThreadWorker حلقة لا نهائية
| البند | القيمة |
|---|---|
| **الملف** | `gameserver/.../Objects/CustomTimedJobManager.cpp` |
| **الدالة** | `ConsumeThreadWorker()` |
| **السطور** | 362-372 |
| **التفاصيل** | `while(true) { ConsumeTimedItemPlusRecords(); Sleep(1000); }` — الحلقة تستيقظ كل 1-3 ثوانٍ حتى لو TimedItemList فارغة |
| **النوع** | CPU waste |
| **الخطورة** | Low-Medium |
| **Evidence level** | ✅ Confirmed from code |

### Perf 4: AgentSessions.ToArray() في كل Broadcast
| البند | القيمة |
|---|---|
| **الملف** | `filter/KMTGuardnew/KMTGuard/ServerManagers/ServerManager.cs` |
| **الدوال** | `BroadcastPacket()`, `GetIPCount()`, `GetHWIDCount()`, `GetCharnamebyUniqueId()` |
| **التفاصيل** | كل استدعاء ينشئ `AgentSessions.ToArray()` — نسخة كاملة من كل الجلسات |
| **النوع** | GC Pressure + Memory |
| **الخطورة** | Medium |
| **Evidence level** | ✅ Confirmed from code |
| **المطلوب للقياس** | ⏱️ Needs Runtime Profiling |

### Perf 5: PacketHandler.ExecutePipeline نسخ زائد
| البند | القيمة |
|---|---|
| **الملف** | `filter/KMTGuardnew/KMTGuard/PacketHandler/PacketHandler.cs` |
| **الدالة** | `ExecutePipeline()` |
| **السطر** | 221 |
| **المقتطف** | `var readablePacket = new Packet(currentPacket);` |
| **التفاصيل** | لكل Handler في الـ Pipeline، ينشأ Packet جديد ينسخ كل البايتات |
| **النوع** | GC Pressure |
| **الخطورة** | Low-Medium |
| **Evidence level** | ✅ Confirmed from code |

---

## 3. 🔒 مخاطر الأمان (Security)

### Sec 1: SQL Injection — Vulnerable code pattern confirmed
| البند | القيمة |
|---|---|
| **الملفات** | `CGObjPCCustom.cpp`, `sqlCon.cpp`, `CustomTimedJobManager.cpp` |
| **النمط** | كل SQL تُبنى بـ `sprintf(szQuery, "UPDATE ... WHERE ID64 = %lld", ID64)` |
| **المستوى** | ✅ **Vulnerable code pattern confirmed** |
| **Player-controlled input confirmed?** | ❓ — القيم الممررة (`GrantName`, `SkillCodeName`, `strKillerName`) تأتي من ShardManager عبر 0x8888، وShardManager يقرأها من DB (`Command_GameServerQueue`). المسار: **DB → ShardManager → 0x8888 → GS → sprintf**. هل يمكن للاعب حقن SQL؟ **يحتاج تتبع المسار إلى من يكتب في Command_GameServerQueue (Dashboard/GM Tool/SP)** |
| **End-to-end exploit path?** | ❓ Unknown — يعتمد على: (1) من يكتب في Command_GameServerQueue، (2) هل هناك validation على المدخلات قبل الكتابة، (3) هل اللاعب يمكنه التأثير على هذه المدخلات |

### Sec 2: 0x3538 Spawn Unique — Player-controlled input NOT confirmed
| البند | القيمة |
|---|---|
| **الملف** | `CGObjPCCustom.cpp` — `ReaderPacket` |
| **الأسطر** | 303-331 |
| **النمط** | `byte uniquetype; *pMsg >> uniquetype; CGObjMob::CreateMob(46406+uniquetype, ...)` |
| **المستوى** | ✅ **Vulnerable code pattern confirmed** (GS side) |
| **Player-controlled input?** | ❓ — الحزمة تأتي عبر الفلتر الذي يتحقق من `ValidateFilterSessionKey`. **يحتاج تتبع: هل هناك مسار يسمح للاعب بإرسال 0x3538 مباشرة؟** (الفلتر يمرر الحزمة للـ GS فقط بعد Session Key check على 0x35xx) |
| **End-to-end exploit path?** | ❓ Unknown — يحتاج تتبع كامل من Client → Filter → GS |

### Sec 3: GSKEY ثابت — Pattern confirmed
| البند | القيمة |
|---|---|
| **الملف** | `CGObjPCCustom.cpp` السطر 107 |
| **المقتطف** | `#define GSKEY "KMTGUARD710632TEST"` |
| **المستوى** | ✅ Pattern confirmed — لكن sessionKey عشوائي 32 bytes فلا يمكن تزويره |
| **الخطورة** | Medium |

---

## 4. 📊 Data Consistency Risks

### DC 1: UPDATE _Items بدون Transaction
| البند | القيمة |
|---|---|
| **الملف** | `CGObjPCCustom.cpp` |
| **الدالة** | `HandleNewAlchemyRequest()` |
| **الأسطر** | 1693, 1739, 1784, 1829... (كل فرع ألكيمي) |
| **التفاصيل** | `this->UpdateItemPlusInDatabase(pTargetItem->ID64, pTargetItem->InstanceItem->OptLevel);` — ينفذ UPDATE بدون BEGIN/COMMIT/ROLLBACK |
| **الخطورة** | **Critical** — فشل UPDATE = DB تختلف عن ذاكرة GS |
| **Evidence level** | ✅ Confirmed from code |

### DC 2: UPDATE + DELETE بدون Transaction في Timed Plus
| البند | القيمة |
|---|---|
| **الملف** | `CustomTimedJobManager.cpp` |
| **الدالة** | `ConsumeTimedItemPlusRecords()` |
| **الأسطر** | 72-92 |
| **التفاصيل** | UPDATE `_Items` (سطر 72-75)، إذا نجح: DELETE `Item_Locked` (سطر 83-84). لا Transaction. |
| **الخطورة** | **High** — UPDATE ينجح وDELETE يفشل = صف يتيم |

### DC 3: LockedItemList غير متزامنة مع DB
| البند | القيمة |
|---|---|
| **المصدر** | Filter `CustomGameServerPacketHandler.cs` يكتب `Item_Locked` عبر `DatabaseJobQueue` (Async) |
| **GS** | يقرأ فقط عند الإقلاع + عبر 0x5061/0x5063 من ShardManager |
| **الخطورة** | **High** — قفل جديد قد لا يصل للـ GS إذا فشل ShardManager broadcast |

---

## 5. 🧠 Memory Risks

### Mem 1: خرائط std::map بلا حدود
| الخريطة | الملف | الحد الأقصى |
|---|---|---|
| `TimedItemList` | `sqlCon.cpp` | ❌ لا يوجد |
| `LockedItemList` | `sqlCon.cpp` | ❌ لا يوجد |
| `STimedDevillList` | `sqlCon.cpp` | ❌ لا يوجد (ولا تُستهلك) |
| `g_FilterSessionKeys` | `CGObjPCCustom.cpp` | ✅ Prune عند 1024 |

### Mem 2: new BYTE[] في Packet construction
| الملف | الدالة | السطور |
|---|---|---|
| `CGObjPCCustom.cpp` | `HandleGlobalItemLink` | 1091 |
| `CGObjPCCustom.cpp` | `HandleAlchemyLinkRequest` | 1510 |
| `CGObjPCCustom.cpp` | `HandleDisplayCharInfoRequest` | 1567 |
| `CGObjPCCustom.cpp` | 0x8888 Type 131 | 581 |

**Evidence level:** ✅ Confirmed from code
**Risk:** منخفض — `delete[]` موجود بعد الاستخدام مباشرة

---

## 6. أولويات الإصلاح (بدون تنفيذ)

| الرتبة | المشكلة | المستوى | النوع | الخطورة |
|---|---|---|---|---|
| 1 | `sprintf` بدون buffer (3 occurrences) | ✅ Confirmed | Crash Bug | **Critical** |
| 2 | ShardManager DELETE بدون ACK | ✅ Confirmed | Data Loss | **Critical** |
| 3 | `ConsumeTimedItemPlusRecordsDevill` معلقة | ✅ Confirmed | Data Bug | **Critical** |
| 4 | SQL Sync في خيط Network | ✅ Confirmed | Performance | High |
| 5 | UPDATE بدون Transaction | ✅ Confirmed | Data Consistency | High |
| 6 | `Item_TimedPlus` لا يُحذف من الجدول | ✅ Confirmed | Data Accumulation | High |
| 7 | Race: `TimedItemList` بلا قفل | ✅ Confirmed | Data Consistency | High |
| 8 | SQL Injection pattern (تقييم محدود) | ✅ Pattern only | Security | High (إذا تأكد المسار) |