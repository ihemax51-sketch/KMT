# PROJECT_REFERENCE — KMTGuard (مرجع المشروع الشامل)

> This file is maintained from the actual source tree at `D:\KMT-V2.0`.
> It summarizes verified code, paths, classes, functions, hooks, packets, and databases.
> Anything not fully verified is listed under **Unknown / Needs Verification**.
> The current source remains authoritative.

---

## 1. نظرة عامة على المشروع ووظيفة كل مكوّن

KMTGuard هو نظام حماية وإدارة كامل لخوادم Silkroad Online (v188 / sro_client). يتكون من عدة مكوّنات منفصلة تتكامل معًا:

| المكوّن | المسار | الوظيفة |
|---|---|---|
| Filter (الفلتر) | `filter/KMTGuardnew/KMTGuard` | خدمة Console .NET 8 بروكسي بين اللاعبين وسيرفرات اللعبة (Agent/Gateway/Download). يطبّق الحماية (HWID/IP/Flood)، الأوامر، الأحداث، الإشعارات، الرتب، الحماية من البوت، وغيرها. |
| Client DLL | `JTClientLibrary/source/DevKit_DLL` | DLL قديم (VC80/x86) يُحقن في `sro_client.exe` لتركيب الـHooks على واجهة اللعبة والـHooks الداخلية. اسم الخرج: `KMTGuardKit.dll`. |
| WebViewerBridge | `WebViewerBridge` | DLL x86 حديث مسؤول عن WebView2 لفتح الروابط داخل اللعبة. |
| Gameserver Addon | `gameserver` | مصادر Serverside لسيرفر اللعبة (SR_GameServer/ShardManager) بنظام CMake، وتتضمن مجلد `KMTGuardCustom` لتخصيصات KMTGuard داخل الـGameServer. |
| ShardManager Addon | `ShardManager/vSRO-ShardManager` | DLL يُحقن في ShardManager لتنفيذ أوامر الـGameServer Queue وحفظ الـUniques وتوجيه الرسائل بين الـGameServers. |
| AdminDesktop | `KMTGuard.AdminDesktop` | تطبيق WPF تحكم تشغيلي للفلتر (DB Admin، اللاعبون، الإعدادات، الأوامر). |
| SilkroadLauncher | `SilkroadLauncher` | لانشر WPF (Lyon) لدخول اللاعبين (يتصل بالـGateway والمصادقة والتنزيل من DownloadServer). |
| Database | `database` | Migrations SQL + ملفات الخرائط (Procedure/Table maps) + الحزم. |

**النسخة الحالية:** تُقرأ من `VERSION.txt` و`Directory.Build.props`.

---

## 2. خريطة المجلدات والملفات المهمة

```
D:\KMT-V2.0
├── filter/KMTGuardnew/            ← حل الفلتر (.NET 8)
│   ├── KMTGuard/                  ← المشروع الرئيسي (EXE أحادي الملف)
│   ├── KMTGuard.AgentHost/        ← نقطة بداية دور Agent
│   ├── KMTGuard.DownloadHost/     ← نقطة بداية دور Download
│   ├── KMTGuard.GatewayHost/      ← نقطة بداية دور Gateway
│   ├── KMTGuard.PacketPipelineTests/ ← اختبارات خط الـPackets
│   ├── KMTGuard.RuntimeContract/  ← عقد Named Pipes بين الأدوار
│   └── SilkroadSecurityAPI/       ← مكتبة فك/تشفير Silkroad (C#)
├── JTClientLibrary/               ← سورس Client DLL + وسائط
│   ├── source/DevKit_DLL/src/     ← كود الـHooks والتهيئة
│   ├── source/libs/ClientLib/src/ ← كلاسات CIF المخصّصة
│   ├── clientlibrary/             ← وسائط resinfo/ddj
│   ├── media/ و client-resources/ و localization/
│   └── BinOut/Release/KMTGuardKit.dll ← خرج DLL
├── gameserver/                    ← سورس الـGameServer (CMake)
├── ShardManager/                  ← إضافة الـShardManager (VC++)
├── KMTGuard.AdminDesktop/         ← WPF تشغيل/إدارة الفلتر
├── WebViewerBridge/               ← جسر WebView2
├── SilkroadLauncher/              ← اللانشر
├── database/
│   ├── migrations/                ← نسخ SQL الترقيات (20260622 … 20260803)
│   ├── backups/                   ← نسخ استرجاع مقصودة
│   ├── tests/ و validation/       ← اختبارات وتحقق SQL
│   └── kmtguard_procedure_name_map.csv + simple maps
├── docs/                          ← مراجع هندسية و UI
├── scripts/                       ← سكربتات Build/Publish/Deployment
├── build-scripts/                 ← البناء الموحد أو بناء مكوّن منفرد
├── deployment/                    ← قوالب النشر
└── lang/textuisystem.txt          ← مفاتيح نصوص الكلاينت
```

**المواقع الرئيسية للأدوار:**

| الدور | نقطة البداية | الخرج المتوقع |
|---|---|---|
| All (سابقًا الكل في واحد) | `KMTGuard/Startup.cs → Program.Main` | `bin\Release\net8.0\win-x64\KMTGuard.exe` |
| Agent | `KMTGuard.AgentHost/AgentHostProgram.cs` | `KMTGuard.Agent.exe` |
| Gateway | `KMTGuard.GatewayHost/GatewayHostProgram.cs` | `KMTGuard.Gateway.exe` |
| Download | `KMTGuard.DownloadHost/DownloadHostProgram.cs` | `KMTGuard.Download.exe` |
| Client DLL | `JTClientLibrary/source/DevKit_DLL/src/DllMain.cpp` | `BinOut/Release/KMTGuardKit.dll` |
| ShardManager Addon | `ShardManager/vSRO-ShardManager/DllMain.cpp` | `Outpus/KMTGuard_ShardManager.dll` |
| WebViewerBridge | `WebViewerBridge/WebViewerBridge.cpp` | `BinOut/Win32/Release/WebViewerBridge.dll` |

**الوجهة النهائية الثابتة (حسب AGENTS.md):** `D:\KMTGuard-build\{Filter,DLL,Media}` مع `KMTGuard.exe` و`KMTGuardKit.dll`.

---

## 3. التطبيقات والـDLLs الرئيسية ونقاط البداية

| التطبيق/الـDLL | نوع الخرج | نقطة البداية | ملاحظات |
|---|---|---|---|
| `KMTGuard.exe` | .NET 8 Console (تركيبة أحادية) | `filter/KMTGuardnew/KMTGuard/Startup.cs` `private static void Main()` | يشغّل كل الأدوار معًا (`FilterRole.All`) ثم حلقة أوامر تحكم. |
| `KMTGuard.Agent.exe` | .NET 8 Console | `AgentHostProgram.Main` → `Program.RunWorkerAsync(FilterRole.Agent)` | يتطلب `CHANGELOG.md` بجانبه. |
| `KMTGuard.Gateway.exe` | .NET 8 Console | `GatewayHostProgram` | يُحمّل دور Gateway فقط. |
| `KMTGuard.Download.exe` | .NET 8 Console | `DownloadHostProgram` | يُحمّل دور Download فقط. |
| `KMTGuardKit.dll` | VC++ x86 DLL | `DllMain` → `KMTGuardInitializationThread` → `InitializeKMTGuardClient` | يُحقن في `sro_client.exe` المطابق للبصمة (Base=0x00400000، SizeOfImage=0x00D70000، TimeDateStamp=0x4E311CB6). |
| `KMTGuard_ShardManager.dll` | VC++ DLL | `DllMain` / `InitializeShardManagerAddon` | نقطة الـHook: `replaceAddr(0x00783238, &CMainProcess::_OnProcessMessage)` ثم `CMainProcess::Initialize()` مع Detours. |
| `WebViewerBridge.dll` | VC++ x86 DLL | `DllMain` + `WVB_*` exports | يعتمد على WebView2 Runtime. |
| `KMTGuard.AdminDesktop` | WPF | `App.xaml` | يتصل بالفلتر عبر Named Pipes ورصد الحالة. |
| `SilkroadLauncher` | WPF (.NET) | `App.xaml` → `MainWindow` | عميل دخول اللاعبين. |

---

## 4. ترتيب تشغيل وتهيئة المشروع

### 4.1 تسلسل بدء الفلتر — `Program.StartCoreAsync` (`filter/KMTGuardnew/KMTGuard/Startup.cs`)
1. **Settings**: `new SettingsManager()` يقرأ/ينشئ `Settings.json` (في `AppContext.BaseDirectory` أو عبر env `KMTGUARD_SETTINGS_PATH`).
2. بناء `Connectionstring` من `Address/ProxyDb/Username/Password/MaximumPool/MinimumPool/ConnectionLifetime` مع قيم Pool حسب الدور.
3. **Logging**: `Serilog` → Console + ملف `logs/kmtguard-<role>-events-.log` (يومي، 7 ملفات).
4. `AcquireInstanceMutexes(role)` — كائنات `Global\KMTGuard_Filter_<Role>` لمنع التكرار.
5. `PlayerLanguage.Initialize(settings.Settings.Language)` — تحميل ملفات `Languages/*.json`.
6. التحقق من أن إعداد `ServerIP` موجود لاستخدامه في ربط خدمات اللعبة.
7. `ServerManager.InitialServers(role)` – تهيئة الخدمات والخوادم.
8. `RuntimeControlServer(role, RequestWorkerShutdownAsync)` – Named Pipe للتحكم.

### 4.2 تسلسل `ServerManager.InitialServers` (`ServerManagers/ServerManager.cs`)
1. `_serverSettings.InitServerSettings()` (إلا لدور Download).
2. `RefManager.Initialize()` (Agent/All) أو `InitializeGateway()` (Gateway).
3. إعداد مجموعات الجلسات: `AgentSessions/DownloadSessions/GatewaySessions = new ConcurrentSessionSet()`.
4. للدور Agent/All:
   - `JobControlService.InitializeAsync`
   - `OfflineStallService.InitializeAsync`
   - `TelegramNotificationService.InitializeAsync`
   - `DiscordNotificationService.InitializeAsync`
   - `DatabaseCommands.InitializeTimer()` + `InitializePlannedTimer()`
   - `AutoEventService.InitializeAsync`
   - `Scheduler.InitializeSchedulerTimer()`
   - `TeleportFreezeService.Start()`
5. `g_DelayedJobMgr.Run()` لـ Agent/Gateway/All.
6. قراءة `System_ProxyServices` من قاعدة `ProxyDb` وبناء `Servers` حسب الدور: `AddServer(service)` → `AgentServer/GatewayServer/DownloadServer`.
7. `Role == All` → خيط `KMTGuard-ConnectionTracker` لتحديث عنوان النافذة.
8. `StartAsync(true)` – ينتظر اكتمال ربط الـAutoStart services (مهلة 15 ثانية).
9. للدور Agent/All: `EXEC [dbo].[System_Start]` ثم `TelegramNotificationService.QueueServerOnlineAsync()`.

### 4.3 تسلسل بدء Client DLL (`DllMain.cpp`)
1. `DLL_PROCESS_ATTACH → DisableThreadLibraryCalls`.
2. `IsSupportedClientHost()` – يتحقق: الاسم `sro_client.exe`، Base `0x00400000`، `SizeOfImage=0x00D70000`، `TimeDateStamp=0x4E311CB6`.
3. `InstallClientMultiInstanceMutexHook()` – يكتب IAT hook على `CreateMutexA` لجعل `Silkroad Client` Mutex محليًا (Multi-client).
4. إنشاء خيط `KMTGuardInitializationThread` → `InitializeKMTGuardClient`:
   - `Setup()` في `Util.cpp` – تثبيت كل الـHooks (vftable + replaceAddr + replaceOffset).
   - `OnWndProc(DesktopCharacterHud::GameWndProcHook)`.
   - `m_Settings->getInstance()`, `m_CustomDataManager->getInstance()`, `m_Player->getInstance()`.
   - `RegisterObject(...)` لكل كلاس CIF جديد ثم `OverrideObject<...>(0x00…address)` للكائنات الموجودة.
   - `OnPreInitGameAssets(InstallRuntimeClasses);`
5. `WriteClientStartupDiagnostic` يكتب تشخيص البداية في ملف مبكر (إصلاح v2.7.8+).

### 4.4 تسلسل إيقاف الفلتر
- `Program.Main` finally → `StopEmbeddedAsync` → `CleanupRuntime`:
  - `RuntimeControlServer.DisposeAsync`
  - `ServerManager.Dispose` (إيقاف الخدمات + الـTimers + الجلسات)
  - تفريغ الـMutexes وإغلاق `Serilog`.
- `ServerManager.Dispose` يوقف: `OfflineStallService`, `DatabaseCommands.StopTimers`, `Scheduler.Stop`, `AutoEventService.Stop`, `Telegram/Discord`, `RefManager.StopTimers`, `ClientlessManager.Stop`, `TeleportFreezeService.Stop`, `DatabaseJobQueue.StopAsync`, `g_DelayedJobMgr.Stop`.

### 4.5 ترتيب معماري لتدفق اللاعب
```
اللاعب → SilkroadLauncher → GatewayServer (0xA102/0xA100 redirect)
       → AgentServer (لعبة) / DownloadServer (باتش)
كل Server: AsyncServer تفتح TcpListener وتقبل، تنشئ Session:
  Session: عميل (TCP) ↔ سيرفر (TCP) مع فك/تشفير Silkroad Security لكل اتجاه.
  PacketHandler: قوائم بيضاء/سوداء + Handlers (Client/Server/Module).
```

---

## 5. بنية Client DLL والفلتر وServer Addons

### 5.1 الفلتر (.NET 8) — `filter/KMTGuardnew`
```
KMTGuard (EXE)
├── Startup.cs               → Program static (Main, StartCoreAsync)
├── AsyncServer/             → AsyncServer (TcpListener عام) + IAsyncServer + TokenProvider
├── Server/
│   ├── AgentServer/         → AgentServer + PacketHandlers (Guild/Stall/COS/Chat/…)
│   ├── GatewayServer/       → GatewayServer + Handlers (A102, A100, 2322, DLL settings)
│   └── DownloadServer/      → DownloadServer (ترحيل باتش)
├── Session/                 → Session (عميل↔سيرفر), SessionData, ISession
├── PacketHandler/           → PacketHandler (Pipeline مع Priority), IPacketHandler
├── ServerManagers/          → ServerManager, RefManager, RankManager, ActionManager,
│                               JobControlService, BotProtectionService, OfflineStallService,
│                               PvpChallengeService, RegionControlService, TeleportFreezeService,
│                               TradeSellCaptchaService, TriggerService, SilkInfo,
│                               SilkStallService
├── Database/                → DatabaseCommands, Scheduler, sqlQueryHelper, Models
├── Features/                → Attendance, AutoEvents, Discord, ItemChest, Skills,
│                               Telegram, UniqueHistory, WebViewer
├── ChatFiltering/           → ChatWordFilter, BlockedWordRule
├── Clientless/              → ClientlessManager/Session/Account
├── CommandManager/          → CommandHandler
├── ConsoleUi/               → FilterConsole, KmtServiceLogFormatter
├── Localization/            → PlayerLanguage
├── Helpers/                 → DatabaseJobQueue, DelayedJobManager, HwidSecurity, EmailService, State
├── Runtime/                 → RuntimeControlServer
└── SettingManager/          → SettingsManager, Settings
```

- **SilkroadSecurityAPI** (`filter/KMTGuardnew/SilkroadSecurityAPI`): `Packet`, `PacketReader/Writer`, `Security`, `Blowfish`, `TransferBuffer`, `Utility`. هو طبقة فك/تشفير Silkroad الأصلية.

### 5.2 Client DLL — `JTClientLibrary`
```
JTClientLibrary
├── source/DevKit_DLL/src/   ← كود الـDLL (Hooks + Util + DllMain + imgui_windows)
├── source/libs/ClientLib/src/ ← ClientLib (كلاسات CIF، GInterface, CGInterface, …)
├── source/libs/ClientNet/    ← NetProcess (RegisterPacketHandlers)
├── source/libs/… (DiscordRichPresence, JMX_Library, MathHelpers, NavMesh, SimpleViewer, TypeId)
├── support/                  ← hook.h، MemberFunctionHook.h
├── third-party/              ← cxxtest, dxsdk, ghidra, imgui
├── media/ + clientlibrary/resinfo/ ← الملفات البصرية
```

- `Util.cpp` → `Setup()` يركّب كل الـHooks داخل `sro_client.exe`.
- `DllMain.cpp` → تسجيل كلاس CIF جديد + Override لكلاسات موجودة.
- أبرز الكائنات المسجّلة: `CIFMenu, CIFMenuGuide, CIFGrantName, CIFTitleManager(Slot), CIFIconManager(Slot), CIFDynamicRanking(Slot), CIFUniqueHistory(Slot), CIFEventRegister(Slot), CIFEventSchedule(Slot), CIFAchievements(Slot), CIFChangelog, CIFCustomMessageBox, CIFExtQuickSlot*, CIFChest*, CIFSocial, CIFDps, CIFPartyMemberViewer, CIFSavedLocation, CIFMovePartyMember, CIFKillerAnimationWnd, CIFDropLogWnd, CIFOfflineStall*, CIFVItemMall*, CIFVAvatarMall*, CIFWeb, CIFDailyLogin, CIFSoxEffect, CIFMacro*, CIFPopupList*, CIFKillCounter, CIFTeamCounter, CIFJobCounter, CIFAlchemyMacro*, CIFNewMsgBox, CIFItemLocker/Unlocker*, CIFFortressWar, CIFCustomEmojiList*, CIFItemTranslation*, CIFSecondaryPassword, CIFAttendance, CIFTargetPlayerEquip, CIFSettings, CIFMSFPS, CIFCounterWnd`.
- Overrides: `CIFTargetWindowPlayer(0x00eea5dc)`, `CIFMainPopup(0x00eea6dc)`, `CIFPlayerInfo(0x00eea7e8)`, `CIFTargetWindowJobPlayer(0x00eea5bc)`, `CIFWholeChat(0x00eec7a8)`, `CIFExtQuickSlot(0x00ee9a28)`, `CIFExtQuickSlotOption(0x00ee9a48)`, `CIFChatOptionBoard(0x00eec128)`, `CIFChatViewer(0x00EEC168)`.

### 5.3 ShardManager Addon — `ShardManager/vSRO-ShardManager`
- `DllMain.cpp`: يستدعي `CSettings::LoadIniSettings()`، ثم `replaceAddr(0x00783238, &CMainProcess::_OnProcessMessage)` و`CMainProcess::Initialize()`.
- `Network/MainProcess.cpp`:
  - `_OnProcessMessage` يعترض: `0x5060→0x5061` (قفل أيتم)، `0x5062→0x5063` (فك قفل أيتم)، `0x5065→0x5066` (قفل مؤقت)، `0x5067→0x5068`، `0x5068→0x5069` (شات)، ثم يُمرر الباقي للأصلي `0x0040dda0`.
  - `MyHandleMsg` (Detour على `HANDLE_MSG_FUNC_OFFSET=0x006990D0`) يعترض `0x7808` المحتوي `0x300C` ويعالج:
    - `0xC05` → `EXEC [KMTGuard].[dbo].[Hook_UniqueSpawn]`
    - `0xC06` → `EXEC [KMTGuard].[dbo].[Hook_UniqueKill]`
  - العناوين: `SET_MSG_HANDLER_FUNC_OFFSET=0x006990C0`, `HANDLE_MSG_FUNC_OFFSET=0x006990D0`, `GET_INSTANCE_FUNC_OFFSET=0x00401D30`.
- `AsyncGSCommands.h/cpp`: `m_dbLink.sqlConn / sqlCmd` (SQLCommand/SQLConnection في `Database/`).
- `TrainingCampHonorRankService.h/cpp`: تحديث Honor Rank (متصل بـ `Command_GameServerQueue Action_ID=200`).

---

## 6. أهم الكلاسات والدوال والـThreads والخدمات

### 6.1 الفلتر
| الكلاس/الدالة | الملف | الوظيفة |
|---|---|---|
| `Program.StartCoreAsync` | `Startup.cs` | نقطة التهيئة الكبرى |
| `Program.RunWorkerAsync(role)` | `Startup.cs` | تشغيل دور واحد مع إشارة إيقاف Ctrl+C |
| `ServerManager.InitialServers / AddServer / StartAsync` | `ServerManagers/ServerManager.cs` | إدارة دورة حياة الخوادم |
| `ServerManager.BroadcastPacket(بمختلف التصفيات)` | نفسه | بث للكل/بالاسم/بالعالم/بالمدينة |
| `ServerManager.IsOnlinePlayer / GetOnlinePlayerCount` | نفسه | تعريف اللاعب الأونلاين وعدّه |
| `AsyncServer.Start / HandleAcceptedClientAsync` | `AsyncServer/AsyncServer.cs` | حلقة الـAccept والحماية من الفيض |
| `Session.Start / DoReceiveFromClient / DoReceiveFromServer` | `Session/Session.cs` | حلقات الاستقبال الثنائية الاتجاه |
| `Session.Stop / TryDetachClientTransport` | نفسه | الإيقاف الآمن وخصم الـOfflineStall |
| `Session.TryPassClientPacketGuards` | نفسه | حارس الحزم (حجم/عدد/نوافذ زمنية للـCustom) |
| `PacketHandler.HandleClient / HandleServer / ExecutePipeline` | `PacketHandler/PacketHandler.cs` | خط المعالجة بالأولويات |
| `DatabaseCommands.ProcessCommands` | `Database/DatabaseCommands.cs` | ينفذ `Command_FilterQueue` كل 2 ثانية |
| `DatabaseCommands.ProcessPlannedCommands` | نفسه | ينفذ `Command_PlannedQueue` كل 3 ثوانٍ |
| `DatabaseCommands.SetRegionTimerAsync / UpdateTimers` | نفسه | عدّادات المناطق والعوالم |
| `RefManager.Initialize / LoadRanks* / StartLoadRanksTimer` | `ServerManagers/RefManager.cs` | مراجع RefObj + الرتب |
| `RankManager` | `ServerManagers/RankManager.cs` | رتب Silk |
| `ActionManager` | `ServerManagers/ActionManager.cs` | Tables الحالة (Teleport gates, Attack regions, Timers, KillCounters) |
| `FilterConsole` | `ConsoleUi/FilterConsole.cs` | واجهة الكونسول (Boot Sequence، /status، …) |
| `CommandHandler.ExecuteCommand` | `CommandManager/CommandHandler.cs` | أوامر الكونسول |

### 6.2 Threads / Tasks بارزة
| الاسم | المكان | الوظيفة |
|---|---|---|
| `KMTGuardInitializationThread` | `DllMain.cpp` | تهيئة الـClient DLL خارج DllMain |
| `KMTGuard-ConnectionTracker` | `ServerManager.cs` | تحديث عنوان النافذة كل ثانية |
| `_commandTimer` (2s) | `DatabaseCommands.InitializeTimer` | معالجة `Command_FilterQueue` |
| `_plannedCommandTimer` (3s) | `DatabaseCommands.InitializePlannedTimer` | معالجة `Command_PlannedQueue` |
| `timers` (1s) | `DatabaseCommands.EnsureTimerUpdaterStarted` | تحديث timers العالم/المنطقة |
| `Scheduler` Timer | `Database/Scheduler.cs` | تنفيذ `System_Schedule` |
| `RefManager._timer` | `RefManager.StartLoadRanksTimer` | إعادة تحميل الرتب (افتراضي 10 دقائق) |
| `RuntimeControlServer` listener | `Runtime/RuntimeControlServer.cs` | Named Pipe للتحكم عن بعد |
| `BotProtectionService` | `ServerManagers/BotProtectionService.cs` | حماية البوت (فحص عند Ping) |
| `DatabaseJobQueue` | `Helpers/DatabaseJobQueue.cs` | قائمة مهام DB خلفية محدودة السعة |

---

## 7. الـHooks وعناوينها وأماكن تركيبها

### 7.1 Client DLL — `JTClientLibrary/source/DevKit_DLL/src/Util.cpp → Setup()`

| العنوان | الدالة | النوع |
|---|---|---|
| `0x00E0963C` slot 17 | `CGFXVideo3D_Hook::CreateThingsHook` | vftableHook |
| `0x00E0963C` slot 26 | `CGFXVideo3D_Hook::EndSceneHook` | vftableHook |
| `0x00E0963C` slot 20 | `CGFXVideo3D_Hook::SetSizeHook` | vftableHook |
| `0x00db95a4` slot 10 | `CGInterface::OnCreateIMPL` | vftableHook |
| `0x00db95a4` slot 5 | `CGInterface::OnTimerIMPL` | vftableHook |
| `0x00d71100+1` | `CGInterface::OnCharIMPL` | replaceAddr |
| `0x00de2e7c/0x00de256c/0x00de2c24/0x00de26c4/0x00de211c` slot 15 | `Func_15_impl` (CICUser/CICharactor/CICPlayer/CICMonster/CICCos) | vftableHook |
| `0x009D0221` | `CICPlayer::UpdateNameColor` | replaceOffset |
| `0x00831337+4` | `WndProcHook` | replaceAddr |
| `0x0065c6f0` | `CAlramGuideMgrWnd::CreateGuideIcon` | placeHook |
| `0x008491d1` | `CGame_Hook::LoadGameOption` | replaceOffset |
| `0x00832a11` | `CGame_Hook::InitGameAssets_Impl` | replaceOffset |
| `0x0084c9bf` | `CNetProcessIn::RegisterPacketHandlers` | replaceOffset |
| `0x00898656` | `CNetProcessSecond::RegisterPacketHandlers` | replaceOffset |
| `0x008a4876` | `CNetProcessThird::RegisterPacketHandlers` | replaceOffset |
| `0x0060bbbf` | `CNIFUnderMenuBar::UseSlot` | replaceOffset |
| `0x0078c9ce+1` | `resinfo ginterface.txt` | بيانات |
| `0x0086b32a+1` | `resinfo pstitle.txt` | بيانات |
| `0x00dd92d4 / 0x00dd92e4` | `CPSTitle::OnServerPacketRecv / OnCreateIMPL` | replaceAddr |
| `0x00dd92bc` slot 12 | `CPSTitle::OnUpdateIMPL` | vftableHook |
| `0x00d758c3+1 / 0x00d758cd+1 / 0x0086ae77` | `CPSTitle::PressButtonServerList / PressConnectButton` | replaceAddr/Offset |
| `0x00d7110a+1` | `CGInterface::OnKeyDown` | replaceAddr |
| `0x00dd8134 / 0x00dd8160` | `CPSCharacterSelect::OnServerPacketRecv / FUN_0085ddb0` | replaceAddr |
| `0x00dd811c` slot 3/10 | `CPSCharacterSelect::MessageMap / OnCreateIMPL` | vftableHook |
| `0x0085eb82+1` | `resinfo pscharacterselect.txt` | بيانات |
| `0x006ac1fa+1 / 0x006b244b+1 / 0x007c0a2a+1 / 0x0063e3a1+1 / 0x0079f9e4+1 / 0x007a48db+1` | resinfo تخطيطات (equipment/inventory/itemmall/messagebox/cos/cosinfo) | بيانات |
| `0x008469c0` (داخل `CGame_Hook::LoadGameOption`) | استدعاء الأصل | call |
| `0x00BAE370` (داخل CreateThingsHook) | استدعاء الأصل | call |
| `0x008311C0` (داخل WndProcHook) | WndProc الأصل | call |
| `0x0086BC33 / 0x0086BC6F` | PatchWatermark (لون + نص النسخة) | CopyBytes |
| `0x004C1D23` (6 بايت) | RenderNop لخبر القتل مع GM | RenderNop |
| `0x007a9bd0` | `CIFChatViewer::ShowHideControls` | placeHook |
| `0x00558618` | `CIFConsole::SetVisibleMode` | replaceOffset |
| `0x00783238` **(ShardManager)** | `CMainProcess::_OnProcessMessage` | replaceAddr |
| ShardManager: `0x006990D0` | Detour `MyHandleMsg` (عبر Detours) | Detour |
| **IAT** | `CreateMutexA` في sro_client → `ClientCreateMutexForMultiClient` | IAT Hook |

**ملاحظة حول العناوين على مستوى CIF داخل `Util.cpp`** (كلاسات/وظائف عناوينها معروفة في الكود مثل `0x00B9C9C0` لتسجيل runtime classes).

### 7.2 Hook على مستوى اللعبة (Game hooks داخل ShardManager)
- `0x5060/0x5061` قفل أيتم → بث لجميع الـGS.
- `0x5062/0x5063` فك قفل أيتم → بث.
- `0x5065→0x5066` قفل أيتم مؤقت (CharID, ID64, OldOptLevel, endtime).
- `0x5067→0x5068` (ID64).
- `0x5068→0x5069` تحويل شات (SenderID, ChatType, ChatIndex, ReceiverName, Text, ItemSlot).
- `0x7808` يحتوي `0x300C + (0xC05|0xC06)` → `Hook_UniqueSpawn/Hook_UniqueKill`.

---

## 8. الـPackets والـOpcodes والـHandlers واتجاه كل Packet

### 8.1 نظام معالجة الـPackets في الفلتر
- `Session.DoReceiveFromClient` يستقبل الحزم من اللاعب → `PacketHandler.HandleClient(packet, session)`.
- `Session.DoReceiveFromServer` يستقبل من سيرفر اللعبة → `PacketHandler.HandleServer(packet, session)`.
- `HandleClient`: يعامل `0x9000` و`0x5000` و`0x2001` كحزم تحكم، ثم يطبّق **Blacklist** (فصل) و**Whitelist** (حجب) ثم يستدعي `ExecutePipeline`.
- النتائج: `Nothing` (تمرير عادي)، `Override` (إعادة كتابة الحزمة)، `Block` (إسقاط)، `Disconnect`.
- التسجيل: `RegisterClientHandler(opcode, handler)` و`RegisterModuleHandler(opcode, handler)` و`SetDefaultHandler/SetBlockHandler/SetDisconnectHandler`.

### 8.2 Opcodes مسجلة مباشرة في الكود (متحقق منها)
| الـOpcode | الاتجاه | المكان | الوظيفة |
|---|---|---|---|
| `0x2002` (Ping) | C→S | AgentServer, GatewayServer, DownloadServer | Ping/فحص بوت/منطقة |
| `0x7021` (حركة) | C→S | AgentServer | TeleportFreeze + RegionControl |
| `0x5038` | S→C | CustomGameServerPacketHandler | `TARGET_PLAYER_ITEM_INFO` |
| `0xB034` / `0x7034` | S→C / C→S | CustomGameServerPacketHandler | `SERVER_ITEM_MOVE` / `CLIENT_ITEM_MOVE` |
| `0x30BF` | S→C | CustomGameServerPacketHandler | `SERVER_ENTITY_STATE_UPDATE` |
| `0x5013` | S→C | CustomGameServerPacketHandler | `SERVER_KILL_LOGGER` |
| `0x5010` | S→C | CustomGameServerPacketHandler | `UNIQUE_DPS` |
| `0x5014` | S→C | CustomGameServerPacketHandler | `SERVER_MOB_KILL_LOGGER` |
| `0x385F` | S→C | CustomGameServerPacketHandler | `SERVER_FORTRESS_UPDATE` (تلغرام) |
| `0x5030` / `0x5031` | S→C | CustomGameServerPacketHandler | `SERVER_ITEM_LOCK_INFO_LOCKED/UNLOCKED` |
| `0x5035` | S→C | CustomGameServerPacketHandler | `GET_POS_INFO_FROM_GS_EVERY_TELEPORT` |
| `0x3571` | S→C | CustomGameServerPacketHandler | `SERVER_REGION_HAS_CHANGED` |
| `0x7025` | C→S | ChatPackets | `HandleChatReq` (شات) |
| `0x705C` | C→S | ChatPackets | `GLOBAL_ITEM_LINK_CLIENT` |
| `0x5033` | S→C | ChatPackets | `GLOBAL_ITEM_LINK` |
| `0xA102` | S→C | GatewayServer | `SERVER_GATEWAY_LOGIN_RESPONSE` → Redirect للـAgent |
| `0xA100` | S→C | GatewayServer | `SERVER_GATEWAY_PATCH_RESPONSE` → Redirect للـDownload |
| `0x2322` | S→C | GatewayServer | `SERVER_GATEWAY_LOGIN_IBUV_CHALLENGE` (إزالة Captcha) |
| `0x6323` | C→S | GatewayServer (ينشئها) | `removePacket` قيمة Captcha |
| `0x166A` | C→S | GatewayServer whitelist | تسجيل حساب من الـDLL |
| `0x1670/0x1672/0x1674` | C→S | GatewayServer whitelist | Secure Quick Login (إنشاء/تشغيل/إلغاء) |
| `0x35FE` | C→S (إرسال للـGS) | DatabaseCommands (CMD 41) | تسجيل مفتاح جلسة GameServer |
| `0x3539` | C→S | DatabaseCommands (CMD 41) | Self Teleport |
| `0x3501` | C→S | DatabaseCommands (CMD 7) | تعيين Hwan title |
| `0x7061` | C→S | DatabaseCommands (CMD 39) | طلب Get Up |
| `0x168A` | S→C | ServerManager.sendNotice/DatabaseCommands | Notice |
| `0x3026` | S→C | Session.SendNotice | Notice (ASCII) |
| `0x220A` | S→C | DB Commands | Timer للعالم/المنطقة |
| `0x207A/0x207C` | S→C | DB Commands | Kill Counter عام |
| `0x189A/0x189B` | S→C | DB Commands | Team Kill Counter |
| `0x189C/0x189D` | S→C | DB Commands | Job Kill Counter |
| `0x168B/0x168C/0x168D` | S→C | DB Commands | Title / Title Color / Icon |
| `0x170A/0x170B/0x173F/0x174B/0x174A/0x174E` | S→C | DB Commands | تحديث/إزالة الألوان والأيقونات |
| `0x202B/0x202C` | S→C | DB Commands | Tag title |
| `0x203B` | S→C | DB Commands | Item Chest row |
| `0x209C` | S→C | DB Commands | Silk Rank |
| `0x177E` | S→C | DB Commands | Achievement update |
| `0x193E` | S→C | DB Commands | Map ping |
| `0xA405/0xA406` | S→C | DB Commands | Name Color update/remove |
| `0x8888` | (رسمي) | ShardManager/GameServer | غلاف أوامر `Command_GameServerQueue` |
| `0x168A` (server notices) | S→C | — | (`NoticeType` + نص) |

### 8.3 مرجع الـOpcodes العام
`filter/KMTGuardnew/KMTGuard/docs/agent-opcodes-reference.md` يحتوي على جدول شامل (مصدر wiki Silva) لأوبكودات Agent: منها `0x6103/0xA103` (AGENT_AUTH)، `0x7001/0xB001` (Character Selection Join)، `0x7025/0xB025` (AGENT_CHAT)، `0x7010/0xB010` (Operator Command)، `0x7034/0xB034` (Inventory Operation)، `0x704C/0xB04C` (Item Use)، `0x7059/0x705A/0x705B` (Teleport)، وغيرها كثير. يُرجع إليه لفك أسماء الأوبكودات القياسية.

---

## 9. نوافذ الـUI وكلاسات CIF والـControl IDs والاختصارات

### 9.1 كلاسات CIF المخصصة (ClientLib)
| الواجهة | الملف | الوظيفة |
|---|---|---|
| `CIFMenu` | `Menu/IFMenu.h` | القائمة الرئيسية المخصصة |
| `CIFMenuGuide` | `IFMenuGuide.h` | Guide القائمة |
| `CIFGrantName` | `Menu/IFGrantName.h` | منح الأسماء |
| `CIFTitleManager/Slot` | `Menu/IFTitleManager*` | إدارة الـTitles |
| `CIFIconManager/Slot` | `Menu/IFIconManager*` | إدارة الأيقونات |
| `CIFDynamicRanking/Slot` | `Menu/IFDynamicRanking*` | الرانكات الديناميكية (9 شرائح) |
| `CIFUniqueHistory/Slot` | `Menu/IFUniqueHistory*` | سجل الـUniques |
| `CIFEventRegister/Slot` | `Menu/IFEventRegister*` | تسجيل الأحداث |
| `CIFEventSchedule/Slot` | `Menu/IFEventSchedule*` | جدول الأحداث |
| `CIFAchievements/Slot` | `Menu/IFAchievements*` | الإنجازات |
| `CIFChangelog` | `Menu/IFChangelog.h` | سجل التغييرات |
| `CIFChest/Guide/Slot` | `Guides/IFChest*` | Item Chest |
| `CIFCustomMessageBox` | `CustomInterface/IFCustomMessageBox.h` | صندوق رسائل مخصص |
| `CIFDps` | `CustomInterface/IFDps.h` | عدّاد الضرر |
| `CIFPartyMemberViewer` | `CustomInterface/IFPartyMemberViewer.h` | عرض أعضاء البارتي |
| `CIFSavedLocation` | `CustomInterface/IFSavedLocation.h` | المواقع المحفوظة |
| `CIFMovePartyMember/Slot` | `CustomInterface/IFMovePartyMember*` | نقل أعضاء البارتي |
| `CIFKillCounter` | `CustomInterface/IFKillCounter.h` | عداد القتل (عام) |
| `CIFTeamCounter` | `CustomInterface/IFTeamCounter.h` | عداد الفرق |
| `CIFJobCounter` | `CustomInterface/IFJobCounter.h` | عداد الجوب |
| `CIFKillerAnimationWnd` | `CustomInterface/IFKillerAnimationWnd.h` | أنيميشن القاتل |
| `CIFDropLogWnd` | `CustomInterface/IFDropLogWnd.h` | سجل الدروب (راجع `docs/droplogs_ui_reference.md`) |
| `CIFOfflineStall` | `CustomInterface/IFOfflineStall.h` | Offline Stall |
| `CIFPopupList / 2` | `CustomInterface/IFPopupList*.h` | قوائم منبثقة |
| `CIFSoxEffect` | `CustomInterface/IFSoxEffect.h` | تأثيرات Sox |
| `CIFCustomEmojiList(Slot)` | `CustomInterface/IFCustomEmojiList*` | الإيموجي |
| `CIFTradeCaptchaWnd` | `CustomInterface/IFTradeCaptchaWnd.h` | كابتشا بيع الـTrade |
| `CIFQuickLoginPanel` | `CustomInterface/IFQuickLoginPanel.h` | الدخول السريع |
| `CIFPvpChallengeWnd` | `CustomInterface/IFPvpChallengeWnd.h` | PVP Challenge |
| `CIFLuckySpinWnd` | `CustomInterface/IFLuckySpinWnd.h` | Lucky Spin |
| `CIFSpecialOffersWnd` | `CustomInterface/IFSpecialOffersWnd.h` | العروض الخاصة |
| `CIFLoginRegisterWnd` | `CustomInterface/IFLoginRegisterWnd.h` | تسجيل الدخول |
| `CIFWeb / CIFWebGuide` | `Web/IFWeb*` | المتصفح المدمج |
| `CIFDailyLogin/Guide` | `DailyLogin/IFDailyLogin*` | الحضور اليومي |
| `CIFAttendance` | `DailyLogin/IFAttendance.h` | Attendance |
| `CIFMacro*` (العديد) | `Macro/IFMacro*.h` | نظام الـMacro (AutoPotion, AutoSkill, AutoHunt) |
| `CIFAlchemyMacro*` | `MacroAlchemy/*` | Macro الألكيمي |
| `CIFItemTranslationWnd/Slot` | `ExtraUI/IFItemTranslation*` | ترجمة الآيتم |
| `CIFSettings` | `ExtraUI/IFSettings.h` | إعدادات مخصصة |
| `CIFMSFPS` | `ExtraUI/IFMSFPS.h` | عدّاد FPS |
| `CIFCounterWnd` | `ExtraUI/IFCounterWnd.h` | نافذة العدادات |
| `CIFTargetPlayerEquip` | `ExtraUI/IFTargetPlayerEquip.h` | معدات الهدف |
| `DesktopCharacterHud` | `ExtraUI/DesktopCharacterHud.h` | HUD سطح المكتب |
| `CIFSecondaryPassword` | `SecondPW/IFSecondaryPassword.h` | كلمة المرور الثانية |
| `CIFItemLocker/Unlocker` | `LockItems/*` | قفل الآيتمات |
| `CIFFortressWar` | `CustomInterface/IFFortressWar.h` | حرب القلعة |
| `CIFVItemMall*/CIFVAvatarMall*` | `NewItemMall/*` | المتجر الجديد |
| `IFExtQuickSlot*` | `SecondBar/*` + `IFExtQuickSlot.h` | الشريط السريع الثاني |

### 9.2 حقن الواجهات (Registration/Overrides)
- كل كلاس يُسجّل بـ `RegisterObject(&GFX_RUNTIME_CLASS(...))` في `DllMain.cpp`.
- الكائنات الموجودة تُستبدل بـ `OverrideObject<CIF…, 0x00…>()` (العناوين مذكورة في القسم 7/5.2).

### 9.3 الاختصارات
| الاختصار | الوظيفة | مكان المعالجة |
|---|---|---|
| `ESC` | إغلاق نافذة مخصصة (مع الحفاظ على سلوك Escape الأصلي لغير المخصص) | `WndProc_Hook.cpp` |
| `O` | فتح/إغلاق نافذة Pick Inventory | `WndProc_Hook.cpp` + `CGInterface::TogglePickInventoryWindow` |

### 9.4 ملفات الـResinfo
- التنسيق `clientlibrary\resinfo\ginterface.txt`, `pstitle.txt`, `pscharacterselect.txt`, `ifequipment.txt`, `ifinventory.txt`, `ifitemmall.txt`, `ifmessagebox.txt`, `ifcos.txt`, `ifcosinfo.txt`, `ifghachaselectwnd.txt` … إلخ. توجيه الأسماء يتم عبر `replaceAddr` في `Util.cpp`.
- `JTClientLibrary/clientlibrary/resinfo/` يحتوي ملفات التخطيطات (راجع `docs/client_ui_design_reference.md`).

---

## 10. قواعد البيانات والجداول والـStored Procedures والـCommand Queues

### 10.1 قواعد البيانات الرئيسية
| قاعدة البيانات | الدور |
|---|---|
| `KMTGuard` (اسم Proxy قديمًا) | قاعدة الفلتر/الإدارة (الجدول الرئيسي `System_ProxyServices`) |
| `SRO_VT_ACCOUNT` | حسابات اللاعبين (`TB_User`) |
| `SRO_VT_SHARD` | الشخصيات والآيتمات (`_Char`, `_RefObjCommon`, `_Guild`, ...) |
| `SRO_VT_SHARDLOG` | السجلات (مثل `_LogEventItem`) |
| `Events` | جداول أحداث KMTGuard الحديثة (AutoEvents, Survival, HideAndSeek …) |

(الأسماء تُقرأ من `System_Settings`: `AccountDB`, `ShardDB`, `LogDB`.)

### 10.2 أهم الجداول (تحققت من الكود/التوثيق)
- `System_ProxyServices` — تعريف خدمات الـProxy (ServerType + Bind/Remote).
- `System_Settings` — مركز الإعدادات (SettingName/Value/Category/DisplayOrder/Description).
- `System_Notices` — الإشعارات.
- `System_Schedule` — الجدولة (Name/Query/ScheduledDate/Time/RepeatType/…).
- `Command_FilterQueue` — أوامر الفلتر غير المجدولة (Status=1 قيد الانتظار).
- `Command_PlannedQueue` — أوامر الفلتر المجدولة (DateToExecute).
- `Command_GameServerQueue` — أوامر الـGameServer عبر الـShardManager (Action_ID، تغليف `0x8888`).
- `_HwidList` — سجل الـHWID للشخصيات (+ فهارس `IX_HwidList_CharID`, `IX_HwidList_Hwid_Active`, `IX_HwidList_Active`).
- `Auth_HWIDs` — HWID نشطة (`Active=0` عند الفصل/تنظيف).
- `_AsyncFilterCommands` / `_AsyncFilterCommandsPlanned` — نماذج أوامر الفلتر.
- `Security_RegionChat` — تعطيل الشات حسب المنطقة.
- `Security_BotProtectionLog` — سجلات قرارات البوت.
- `Vip_SilkSpendEvents` — أحداث إنفاق VIP Silk (خطأ/إعادة محاولة).
- `Rank_Silk`, `Rank_Categories`, `Rank_Data01..09`, `Rank_DataArchive` — الرتب.
- `Mall_Items/Avatars/Categories` — المتجر.
- `Offer_List`, `Offer_PurchaseLog` — العروض.
- `LuckySpin_Rewards`, `LuckySpin_Log` — Lucky Spin.
- `Attendance_Rewards/Players/RewardLog` — الحضور.
- `Log_PlayerKills` — سجل القتل.
- `Party_Members` — أعضاء البارتي (تنظيف عند الفصل).
- `System_Start` — SP يتم استدعاؤها عند بدء العالم.
- `Item_AddChest` / `Item_GetInfo` — الآيتمات/الصندوق.
- `_ItemChest`, `_Scheduler`, `_FilterRegionControl`, `_ControlTeleport`, `_SpecialOffer`, `_CharacterSettings`, `_KillerAnimation`, `_LuckySpinRewards`, `_RefAchievement*`, `_RefAttendanceReward`, `_RefEvent*`, `_RefFellowData`, `_RefHideSkillEffect`, `_RefHWANLevel`, `_RefMapSettings`, `_RefNewAvatarMall`, `_RefNewItemMall`, `_Rank_Custom1`, `_UniqueHistory`, `_Macro*`, `_FellowSkillData`, `_bypassHwidbyIP`, `_CharInstanceWorldData` (نماذج في `Database/Models/`).

### 10.3 جدول أوامر `Command_FilterQueue` (متحقق من `DatabaseCommands.cs`)
| CommandID | الوظيفة |
|---:|---|
| 1 | فصل بالاسم `Data1` |
| 2 | فصل الكل |
| 3 | Notice (sendall/sendchar + NoticeType + Message + CharName) |
| 4 | إضافة Hwan title |
| 5 | إضافة لون title |
| 6 | إضافة icon (يسار/يمين) |
| 7 | تغيير title live (غير Berserk) عبر `0x3501` |
| 8/9 | تحديث/إزالة لون title (`0x170A/0x170B`) |
| 10–13 | تحديث/إزالة الأيقونات (يسار/يمين) `0x173F/0x174B/0x174A/0x174E` |
| 15/16 | تفعيل/إزالة custom tag title (`0x202B/0x202C`) |
| 17 | إضافة صف Chest live (`0x203B`) |
| 18 | Live teleport gate state (جسم قديم معطل) |
| 19/20 | فتح/قفل هجوم Region/World |
| 21/22 | Timer لمرة واحدة (World/Region) `0x220A` |
| 23/24 | Timer متابع (World/Region) |
| 25/26 | Kill Counter عام (`0x207A/0x207C`) |
| 27 | تحديث Silk Rank live (`0x209C`) |
| 28 | Notice بالـCharID |
| 29 | تحديث Achievement (`0x177E`) |
| 30/31 | Team Kill Counter (`0x189A/0x189B`) |
| 32/33 | Job Kill Counter (`0x189C/0x189D`) |
| 34 | فصل بالـCharID |
| 35 | FTW kill counter old (معطل) |
| 36 | Map ping (`0x193E`) |
| 37/38 | لون الاسم update/remove (`0xA405/0xA406`) |
| 39 | Get Up عبر `0x7061` (DelayedJob) |
| 40 | إعادة تحميل الرتب والـAchievements |
| 41 | Self Teleport (`0x35FE` + `0x3539`) مع Cooldown |
| 42 | Online Item Chest broadcast (إدخال `Item_AddChest` لكل الأونلاين) |
| 1000 | إعادة تحميل NPC Unique IDs + RefObj |
| 9999 | تسجيل Unique Spawn (بث حالة) |
| 10000 | تسجيل Unique Kill + القاتل |

### 10.4 أوامر `Command_PlannedQueue`
| CommandID | الوظيفة |
|---:|---|
| 1 | فصل بالاسم في موعد |
| 2 | فصل الجميع |
| 3 | Notice في موعد |
| 100 | تنفيذ `EXEC` لـProcedure (تحقق `IsAllowedPlannedSql`) |

### 10.5 `Command_GameServerQueue` (يغلف `0x8888`)
| Action_ID | الوظيفة (بالترتيب من Data1) |
|---:|---|
| 1 | تغيير اسم/منحة |
| 2 | Spawn mob في مكان |
| 3 | Spawn mob قرب لاعب |
| 4/5 | قتل/إزالة mob (عام/بالعالم) |
| 6/7/8 | Buff (SkillID / إزالة / CodeName) |
| 10/11 | To Town (لاعب/عالم) |
| 12 | Fellow skill |
| 13 | تغيير Cape live |
| 14 | Teleport قديم |
| 15 | Get Up |
| 16 | EXP rate |
| 17/18/19 | تغيير/استهلاك/استهلاك+تغيير آيتم |
| 20 | أرصدة Silk live |
| 21 | Gold add/remove |
| 22 | World Layer state |
| 23 | Get Up في مكان |
| 24 | نقل لـSafe Zone |
| 38 | Defend The Tower rule |
| 39 | Free-for-all arena rule |
| 131 | Alchemy item live (legacy معلق) |
| 200 | تحديث Honor Rank (TrainingCampHonorRankService، خارج `0x8888`) |

---

## 11. خريطة الـFeatures والملفات والدوال المرتبطة

| الميزة | الملفات (Filter) | الدوال/النقاط |
|---|---|---|
| Offline Stall | `ServerManagers/OfflineStallService.cs`, `Server/OfflineStallGatewayBridge.cs`, `Session.TryDetachClientTransport`, `CustomInterface/IFOfflineStall.h` | إعدادات `EnableOfflineStall`, `OfflineStallMaxHours`؛ الجلسة تبقى بعد فصل العميل، وإعادة الدخول الموثقة تنهيها تلقائيًا. |
| PVP Challenge | `ServerManagers/PvpChallengeService.cs`, `CustomInterface/IFPvpChallengeWnd.h` | إعداد `EnablePvpChallenge` |
| Quick Login | `Server/QuickLoginAgentAuthBridge.cs`, `CustomInterface/IFQuickLoginPanel.h` | أوبكودات Gateway `0x167x` + `0x1211` |
| Special Offers | `Database/Models/_SpecialOffer.cs`, `RefManager.m_SpecialOffers`, `CustomInterface/IFSpecialOffersWnd.h` | إعداد `EnableSpecialOffers` |
| Lucky Spin | `Database/Models/_LuckySpinRewards.cs`, `RefManager.m_LuckySpin`, `CustomInterface/IFLuckySpinWnd.h` | `EnableLuckySpin`, `EnableLuckySpinSilk`, `LuckySpinPrice` |
| Macro (AutoPotion/Skill/Hunt/Alchemy) | `Features/Skills/*` + `Macro/*`, `MacroAlchemy/*` | إعداد `Macro` |
| Attendance | `Features/Attendance/AttendanceService.cs`, `DailyLogin/IFAttendance.h` | `Attendance_Rewards` |
| AutoEvents | `Features/AutoEvents/AutoEventService.cs`, `CompetitiveEventService`, `HideAndSeekEventService`, `SurvivalParty/SoloEventService`, `PartyDataLocator` | `/event start/stop/reload/status`, `Events` DB |
| Discord | `Features/Discord/DiscordNotificationService.cs` | إشعارات Discord |
| Telegram | `Features/Telegram/TelegramNotificationService.cs` | إشعارات Telegram/Fortress |
| Item Chest | `Features/ItemChest/ItemChestService.cs`, `Database/Models/_ItemChest.cs`, `Guides/IFChest*` | `Item_AddChest` |
| Unique History | `Features/UniqueHistory/UniqueHistoryService.cs`, `Database/Models/_UniqueHistory.cs`, `Menu/IFUniqueHistory*` | `Hook_UniqueSpawn/Kill` |
| Bot Protection | `ServerManagers/BotProtectionService.cs`, `ChatFiltering/` | `AllowBotLogin`, `AllowBotTrade`, `BotProtectionLogEnabled`, `Security_BotProtectionLog` |
| Region Control | `ServerManagers/RegionControlService.cs`, `Database/Models/_FilterRegionControl.cs` | chat/منع حركة/هجوم حسب Region |
| Teleport Freeze | `ServerManagers/TeleportFreezeService.cs` | تجميد عند التليسبورت |
| Trade Sell Captcha | `ServerManagers/TradeSellCaptchaService.cs`, `Session/PendingTradeSellCaptcha.cs`, `CustomInterface/IFTradeCaptchaWnd.h` | `EnableTradeSellCaptcha`, `TradeSellCaptchaMaxAttempts`, `TradeSellCaptchaTimeoutSeconds` |
| Job Control | `ServerManagers/JobControlService.cs`, `Database/migrations/20260727_job_control_procedures.sql` | `HWID_JOB_LIMIT` |
| Silk Stall | `ServerManagers/SilkStallService.cs`, `Database/migrations/20260625_silk_stall_transactions.sql` | ستال مقابل Silk |
| Clientless | `Clientless/ClientlessManager.cs`, `ClientlessSession.cs`, `ClientlessAccount.cs` | أوامر `/clientless` + Runtime Control |
| WebViewer | `Features/WebViewer/WebViewerManager.cs`, `WebViewerBridge/` | أزرار DB-backed؛ `webviewer.json` |
| Title/Icon/NameColor/Tags | `DatabaseCommands` CMDs 4–16/37/38 + `Menu/IFTitleManager*`, `IFIconManager*` | نظام Style (`Style_*` procedures) |
| Dynamic Ranking | `Menu/IFDynamicRanking*`, `RefManager.Rank_*`, `RankManager` | `Rank_SetCategory`, `Rank_UpsertEntry`, refresh 10 دقائق |
| Achievements | `Menu/IFAchievements*`, `RefManager`, CMD 29 | `Achievement_*` |
| Changelog | `Menu/IFChangelog.h` | `ShowChangelogFirstSpawn` |
| Secondary Password | `SecondPW/IFSecondaryPassword.h`, `Session.SecondaryCodeEntered` | إعداد `SecondaryPassword` |
| Event Suit / Region Security | `Database/migrations/20260723_security_region_event_suit.sql` | `QueueEventSuitSnapshot` في `CustomGameServerPacketHandler` |
| VIP | `Database/migrations/20260726_vip_tier_configuration.sql`, `20260803_vip_runtime_recovery_and_history.sql`, `DatabaseCommands.ProcessVipSilkSpendEventsAsync` | `Hook_ItemMallBuy` + `Vip_SilkSpendEvents` |
| Drop Logs | `CustomInterface/IFDropLogWnd.h` (+ `IFDropLogsWnd`) | راجع `docs/droplogs_ui_reference.md` |
| Killer Animation | `CustomInterface/IFKillerAnimationWnd.h`, `Database/Models/_KillerAnimation.cs` | `ShowGuideKillerAnimation` |
| Disable Durability | `Database/migrations/20260801_disable_durability.sql` | (SQL) |
| Party Monster Settings | `Database/migrations/20260801_party_monster_settings.sql` | (SQL) |
| Auto Equip Level | `Database/migrations/20260731_auto_equip_level_contract.sql` | `Hook_AutoEquip` |

---

## 12. ملفات الإعدادات ومكان قراءة كل إعداد

| الملف | المكان/القارئ | الإعدادات |
|---|---|---|
| `Settings.json` (يُنشأ تلقائيًا عند غيابه) | `SettingManager/SettingsManager.cs` عبر `SettingsPath` (env `KMTGUARD_SETTINGS_PATH` أو `AppContext.BaseDirectory`) | `Address, Port, Username, Password, ProxyDb, MaximumPool, MinimumPool, ConnectionLifetime, ServerIP, Language` |
| `filter/KMTGuardnew/KMTGuard/config/Settings.example.json` | قالب مثال فقط | نفس المفاتيح |
| `webviewer.json` | `Features/WebViewer/WebViewerManager.cs` (لا يُنسخ داخل single-file؛ يبقى بجانب EXE) | إعدادات أزرار الـWebViewer المحلية |
| `Languages/English.json`, `Turkish.json` | `Localization/PlayerLanguage.cs` (مفاتيح لغة اللاعب) | النصوص القابلة للتحرير |
| `System_Settings` (DB) | `ServerManagers` (Services) عبر `_serverSettings` | كل مفاتيح التشغيل: HWID/IP limits، Stalls، Captcha، UI، روابط، أسماء قواعد البيانات… (`docs/kmtguard_customer_database_reference.md`) |
| `CHANGELOG.md` بجانب الـAgent EXE | `KMTGuard.AgentHost/AgentHostProgram.cs` | مطلوب وجوده عند بدء دور Agent (فشل بدونه) |
| `admins.json` | `KMTGuard.AdminDesktop/Services/AuthService.cs` — `%ProgramData%\KMTGuardAdmin\admins.json` | حسابات مسؤولي Desktop |
| `LauncherSettings` | `SilkroadLauncher/SilkroadLauncher/LauncherSettings.cs` | إعدادات اللانشر |
| `Settings.ini` (ShardManager) | `ShardManager/vSRO-ShardManager/SettingManagers/Settings.h` `CSettings::LoadIniSettings()` | إعدادات SQL للـShardManager |

---

## 13. الـLogs وطرق تشخيص الأخطاء والـCrashes

- **Serilog (الفلتر):**
  - Console (مع `KmtServiceLogFormatter`) عندما توجد نافذة Console.
  - ملفات: `logs/kmtguard-<role>-events-.log` — يومي مع `retainedFileCountLimit: 7`.
  - `Program.LoggingLevelSwitch` للتحكم الحي بالمستوى.
- **أهم رسائل التشخيص:**
  - `Database background job timed out` / `ProcessPlannedCommands` / `OnTimerTick` / `HandleHwidList` — ظواهر SQL timeout شائعة (لا تعني فشل الفلتر بالضرورة).
  - `receive from server disconnect` + opcode → راجع `docs/agent-opcodes-reference.md`.
  - `Packet handler failed for {ClientIp} opcode 0x…` → `PacketHandler.InvokeHandlerSafeAsync`.
- **تشخيص كلاينت:**
  - `ClientStartupDiagnostics` (`WriteClientStartupDiagnostic`) — سجل ملف مبكر لبداية الكلاينت (v2.7.8+).
  - `ClientCrashDiagnostics.h/cpp` — أدوات التقاط الانهيارات.
  - `OutputDebugStringA` للمطور.
- **ShardManager:** طباعة `printf` لكشف الرسائل، مع hexdump عند استثناء `MyHandleMsg`.
- **Decrypting packet flow:** `SilkroadSecurityAPI` يتعامل مع `Security.Recv/TransferIncoming/TransferOutgoing`؛ يمكن تتبع الحزم بـ `TraceEarlyPacket` (أول 30 حزمة لـAgent فقط عند Verbose).
- **قنوات Runtime:** `RuntimeControlServer` (Named Pipe `KMTGuard.Runtime.<Role>.v1`) يتيح `runtime.ping`, `runtime.shutdown`, `runtime.language.set`, `agent.onlineplayers`, `agent.clientless.*`, `agent.quicklogin.*`, `agent.offlinestall.*`.

---

## 14. ملفات الـSolution والـProjects والمتطلبات الخارجية ومسارات الـOutput

### 14.1 الحلول والمشاريع
| الحل/المشروع | المسار | التقنية |
|---|---|---|
| `KMTGuard.sln` | `filter/KMTGuardnew/KMTGuard.sln` | .NET 8 |
| `KMTGuard.csproj` | `filter/KMTGuardnew/KMTGuard/KMTGuard.csproj` | Exe، net8.0، win-x64، `PublishSingleFile=true`, trimmed=false |
| `SilkroadSecurityAPI.csproj` | `filter/KMTGuardnew/SilkroadSecurityAPI/SilkroadSecurityAPI.csproj` | مكتبة |
| `KMTGuard.RuntimeContract.csproj` | `filter/KMTGuardnew/KMTGuard.RuntimeContract/KMTGuard.RuntimeContract.csproj` | مكتبة العقد |
| `KMTGuard.AgentHost/GatewayHost/DownloadHost.csproj` | `filter/KMTGuardnew/…` | أدوار منفصلة |
| `KMTGuard.PacketPipelineTests.csproj` | `filter/KMTGuardnew/KMTGuard.PacketPipelineTests/…` | اختبارات |
| `KMTGuard.AdminDesktop.csproj` | `KMTGuard.AdminDesktop/KMTGuard.AdminDesktop.csproj` | WPF |
| `JTClientLibrary/CMakeLists.txt` | `JTClientLibrary/CMakeLists.txt` | CMake (ClientLib) |
| `source/DevKit_DLL/CMakeLists.txt` | `JTClientLibrary/source/DevKit_DLL/CMakeLists.txt` | CMake (KMTGuardKit) |
| `gameserver/CMakeLists.txt` + presets | `gameserver/` | CMake (Server) |
| `KMTGuard-SM.sln` | `ShardManager/KMTGuard-SM.sln` | VC++ (vSRO-ShardManager.vcxproj) |
| `WebViewerBridge.sln` | `WebViewerBridge/WebViewerBridge.sln` | VC++ (vcxproj) |
| `SilkroadLauncher.sln` | `SilkroadLauncher/SilkroadLauncher.sln` | WPF .NET (x86) |
| IntegrationTest projects | `KMTGuard.AdminDesktop.RuntimeTests` | اختبارات تشغيلية |

### 14.2 المتطلبات الخارجية (حزم NuGet في KMTGuard.csproj)
- `Dapper 2.1.35`
- `Microsoft.Data.SqlClient 5.2.2`
- `Newtonsoft.Json 13.0.3`
- `Serilog 4.3.0` + `Serilog.Sinks.Console 6.0.0` + `Serilog.Sinks.File 7.0.0`
- `System.Management 9.0.10`

### 14.3 مسارات الـOutput
- الفلتر Build: `<repo>\filter\KMTGuardnew\KMTGuard\bin\Release\net8.0\win-x64` (حسب `FILTER_ENGINEERING_HANDOFF.md`).
- الـClient DLL: `JTClientLibrary/BinOut/Release/KMTGuardKit.dll` + `JTClientLibrary/bin` المتولّدة.
- الـShardManager: `ShardManager/vSRO-ShardManager/Outpus/KMTGuard_ShardManager.dll`.
- الـWebViewerBridge: `BinOut/Win32/Release/WebViewerBridge.dll`.
- النشر النهائي (ثابت): `D:\KMTGuard-build\Filter`، `D:\KMTGuard-build\DLL`، `D:\KMTGuard-build\Media`، و`D:\KMTGuard-build\ServerAddons`.
- سكربتات البناء: `build-scripts/Build-All.ps1` للبناء الكامل و`build-scripts/Build-Component.ps1` لبناء مكوّن منفرد.

---

## 15. أهم مسارات الكود مثل Startup وLogin وCharacterGameReady وShutdown

| المسار | الوصف | الملفات |
|---|---|---|
| **Startup** | `Program.Main → StartCoreAsync → ServerManager.InitialServers → StartAsync` + `EXEC System_Start` | `Startup.cs`, `ServerManager.cs` |
| **Login (Gateway)** | `GatewayServer` + `SESSION_GATEWAY_LOGIN_RESPONSE (0xA102)` → QuickLoginAgentAuthBridge → redirect للـAgent | `GatewayServer.cs`, `QuickLoginAgentAuthBridge.cs` |
| **Login (Agent Auth)** | `0x6103/A103 AGENT_AUTH` يُمرر عبر الـPipeline (Default) | `PacketHandler.cs` + opcodes reference |
| **Character Selection** | `CPSCharacterSelect` Hooks في الكلاينت؛ `0x7001/0xB001`, `0x7007/0xB007`, `0x7450/0xB450` | `Util.cpp`, `session.SessionData` |
| **CharacterGameReady** | `Session.CharacterGameReady` يُضبط عند `0x3012/0x3014` (Game Ready) — بوابة لمعظم الـCustom packets والقيود | `Session.cs` (`IsReadyForInGameCustomPacket`), handlers |
| **Teleport / Region change** | `0x5035 GET_POS_INFO_FROM_GS_EVERY_TELEPORT`, `0x3571 SERVER_REGION_HAS_CHANGED` → تحديث `SessionData.WorldID/Region/Layer` + `RegionControlService` + Event Suit + HWID queue | `CustomGameServerPacketHandler.cs` |
| **Kill/Unique** | GameServer → `Hook_UniqueKill/Spawn` عبر ShardManager → unique history/بث | `MainProcess.cpp`, `UniqueHistoryService` |
| **Shutdown** | `StopEmbeddedAsync → CleanupRuntime` ؛ `ServerManager.Dispose` يوقف الخدمات بـترتيب؛ `Session.Stop` ينظف `Party_Members` و`Auth_HWIDs` | `Startup.cs`, `ServerManager.cs`, `Session.cs` |
| **Inline Item Mall (VIP)** | `ProcessVipSilkSpendEventsAsync` يستدعي `Hook_ItemMallBuy` عند إنفاق Silk | `DatabaseCommands.cs` |
| **HWID update** | `_HandleHwidList` → `DatabaseJobQueue.TryQueueBackground` + cooldown 15s | `CustomGameServerPacketHandler.cs`, `DatabaseJobQueue.cs` |

---

## 16. المشاكل المعروفة والمناطق الحساسة ومخاطر التعديل

1. **SQL Timeout** (`Execution Timeout Expired`) في `ProcessCommands/ProcessPlannedCommands/OnTimerTick/HandleHwidList`: معروفة (راجع `FILTER_ENGINEERING_HANDOFF.md`). لا تعني بالضرورة فشل الفلتر؛ غالبًا أداء SQL.
2. **`_HandleHwidList`** يعتمد على SP غير موجودة في السورس (يأخذها من DB). عند التعديل: تحقق من نوع عمود `_HwidList.Hwid`؛ إذا `text/ntext/…` فاعتبر تشغيل migration الاختياري `20260624_hwidlist_hwid_normalize_optional.sql` (خارج أوقات الذروة).
3. **مفاتيح `Command_GameServerQueue`** تنفَّذ داخل الـShardManager؛ لا تخترع `Action_ID` جديدًا خارج الجدول المعتمد.
4. **حماية جلسات اللعب** (HWID/IP/Flood/Bot) جزء من سلوك الفلتر التشغيلي؛ أي تعديل فيها يحتاج اختبار اتصال فعلي.
5. **نوافذ الكلاينت وUI**: لا تخلط زي «Item Mall» (المرجع الحديث) مع «Old School» في النافذة نفسها (AGENTS.md). راجع `docs/client_ui_design_reference.md` و`old_school_reference.md`.
6. **Drop Logs**: UI polish فقط؛ لا تغيّر packets/opcodes/filter/SQL/SP، والتخطيط resinfo-driven عبر `ifdroplogswnd.txt`.
7. **`Session` هشّة** تجاه الحزم غير المتوقعة؛ حارس الحزم (`TryPassClientPacketGuards`) يقطع الجلسة عند التجاوز. أي opcode مضاف للقائمة `IsCustomClientOpcode` يحتاج مراجعة (قبل 0x3012 أو لا).
8. **WebView2 غير موجود** → يجب ألا يفشل الكلاينت (فشل صامت).
9. **`KMTGuardKit.dll`** يخصّ v188 `sro_client.exe` ببصمة محددة؛ أي تغيير في Base/Size/TimeDateStamp يكسر الـHooks.
11. **BASE لتسجيل الـRuntime classes** `0x00B9C9C0` حساس؛ أي خطأ في ترتيب/حجم الكلاس يسقط الكلاينت.
12. **مزامنة الـQueues**: `Command_FilterQueue` و`Command_PlannedQueue` تعتمدان على `Status=1`؛ لا تدخل تعديلات يدوية بدون فهم.
13. **Per-command Cooldown** (SelfTeleport, HWID=15s) — عدم احترامها يسبب تجاهل الأوامر.
14. **أوامر `CommandID 14,35,131`** غير مفعّلة أو معلّقة في الكود — لا تعتمد عليها.
15. **خروج دور Agent بسلام** يتطلب `CHANGELOG.md` موجودًا بجانب الـEXE (وإلا لن يبدأ).

---

## 17. قواعد آمنة يجب اتباعها عند تعديل المشروع مستقبلًا

1. اقرأ **AGENTS.md** ومستندات `docs/` ذات الصلة قبل تغيير نصوص/واجهات/حزم.
2. كل تغيير للمستخدم النهائي يجب أن يُسجَّل في `CHANGELOG.md` (جديد أولًا، مع تاريخ، بدون تفاصيل داخلية).
3. لا تغيّر أسماء/معاملات الـStored Procedures أو `Hook_*`؛ استخدم `ALTER PROCEDURE` بنفس التوقيع وأضف المنطق بعد `SET NOCOUNT ON`.
4. لا تصنع `CommandID`/`Action_ID` جديدًا في SQL إلا بعد تخطيط تنفيذ متوافق في الفلتر/الShardManager.
5. غيّر إعدادًا واحدًا في `System_Settings` ثم أعد تشغيل الخدمات واختبر بحساب عادي (وليس GM فقط)؛ لا تجمع تغييرات متعارضة (New/Old login, New/Old Alchemy…).
6. لا تشغّل `20260624_hwidlist_hwid_normalize_optional.sql` أثناء الذروة (ALTER COLUMN lock).
7. عند تعديل Client DLL: تأكد من الحفاظ على بصمة `sro_client.exe` المدعومة، وراجع العلاقة مع اللانشر/الـLoaders (كانت هناك مشاكل بدء متكررة v2.7.x).
8. عند تعديل الفلتر: أبقِ الأعمال DB الثقيلة خارج مسارات الـPackets الحساسة (`DatabaseJobQueue`).
9. عند إضافة Hook/Opcode في `Session`: ضع في الحسبان قوائم Whitelist/Blacklist و`IsCustomClientOpcode` و`RequiresReadyCharacterForCustomOpcode`.
10. لا تُبنِ داخل مجلد السورس إلا للأغراض المؤقتة؛ الوجهة النهائية `D:\KMTGuard-build`.
11. استخدم أدوارًا منفصلة (Agent/Gateway/Download) في الإنتاج عند الحاجة، واحترم `AcquireInstanceMutexes`.
12. لا تعدّل `RuntimeContracts` إلا مع تحديث `RuntimeProtocol.Version` وكل العملاء.
13. عند أي تغيير في SQL/Build: استخدم `scripts/Publish-KmtGuardDatabase.ps1` مع `build-scripts/Build-All.ps1` وتحقق من `database/package-versions.psd1` (كل ملف SQL مُسنَد لنسخة واحدة في `CHANGELOG.md`).

---

## 18. Quick Reference

### 18.1 أبرز الملفات
| الملف | أهميته |
|---|---|
| `filter/KMTGuardnew/KMTGuard/Startup.cs` | التهيئة والتشغيل |
| `filter/KMTGuardnew/KMTGuard/ServerManagers/ServerManager.cs` | إدارة الخوادم والبث |
| `filter/KMTGuardnew/KMTGuard/Database/DatabaseCommands.cs` | أوامر DB + الـQueues |
| `filter/KMTGuardnew/KMTGuard/Session/Session.cs` | جلسة العميل↔السيرفر |
| `filter/KMTGuardnew/KMTGuard/PacketHandler/PacketHandler.cs` | خط الحزم |
| `filter/KMTGuardnew/KMTGuard/Server/AgentServer/AgentServer.cs` | Handlers Agent + Whitelist |
| `filter/KMTGuardnew/KMTGuard/Server/GatewayServer/GatewayServer.cs` | Login/Patch redirect |
| `JTClientLibrary/source/DevKit_DLL/src/DllMain.cpp` | حقن الـDLL وتسجيل الواجهات |
| `JTClientLibrary/source/DevKit_DLL/src/Util.cpp` | كل الـHooks |
| `ShardManager/vSRO-ShardManager/Network/MainProcess.cpp` | Hook شارد + Uniques |
| `docs/kmtguard_customer_database_reference.md` | مرجع العملاء الكامل لـSQL |

### 18.2 أهم الكلاسات
- Filter: `Program`, `ServerManager`, `AsyncServer`, `AgentServer`, `GatewayServer`, `DownloadServer`, `Session`, `PacketHandler`, `DatabaseCommands`, `RefManager`, `RankManager`, `ActionManager`, `FilterConsole`, `CommandHandler`, `RuntimeControlServer`.
- Client: `CIFMenu`, `CIFTitleManager`, `CIFIconManager`, `CIFDynamicRanking`, `CIFUniqueHistory`, `CIFAchievements`, `CIFChest`, `CIFWeb`, `CIFMacro*`, `CIFSecondaryPassword`, `CIFOfflineStall`, `CIFKillCounter/TeamCounter/JobCounter`, `CIFDropLogWnd`, `CIFKillerAnimationWnd`.
- ShardManager: `CMainProcess`, `CShardNetManager`, `CMsgStreamBuffer`, `AsyncGSCommands`, `TrainingCampHonorRankService`, `CSettings`.

### 18.3 أهم الدوال
- `Program.StartCoreAsync`, `ServerManager.InitialServers`, `ServerManager.StartAsync`.
- `Session.DoReceiveFromClient/Server`, `Session.TryPassClientPacketGuards`.
- `PacketHandler.ExecutePipeline`, `PacketHandler.InvokeHandlerSafeAsync`.
- `DatabaseCommands.ProcessCommands/ProcessPlannedCommands/MarkCommandCompleteAsync`.
- `Util.cpp Setup()`, `DllMain InitializeKMTGuardClient`.
- `CMainProcess::_OnProcessMessage`, `CMainProcess::MyHandleMsg`.

### 18.4 أهم الـHooks (خلاصة)
- `0x00E0963C` slots 17/26/20 (Create/EndScene/SetSize) — Direct3D.
- `0x00db95a4` slots 10/5 (CGInterface OnCreate/OnTimer).
- `0x00831337+4` (WndProc) — اختصارات ESC/O.
- `0x0084c9bf/0x00898656/0x008a4876` — RegisterPacketHandlers للـNetProcess.
- `0x00783238` (ShardManager) — `_OnProcessMessage`.
- `0x006990D0` (ShardManager) — `MyHandleMsg` Detour.
- IAT `CreateMutexA` (sro_client) — Multi-client.

### 18.5 أهم الـPackets (خلاصة)
- `0x2002` Ping؛ `0x7021` Movement؛ `0x7025` Chat؛ `0x3012/0x3014` Game Ready.
- `0x5038/0xB034/0x7034/0x30BF/0x5013/0x5010/0x5014/0x385F/0x5030/0x5031/0x5035/0x3571` — Custom GameServer.
- `0xA102/0xA100/0x2322/0x6323` — Gateway.
- `0x168A/0x3026` Notice؛ `0x220A` Timer؛ `0x207A/C`, `0x189A/B`, `0x189C/D` Counters.
- `0x168B/C/D`, `0x170A/B`, `0x173F`, `0x174A/B/E`, `0x202B/C`, `0xA405/6` — Style/UI.
- `0x209C` SilkRank؛ `0x177E` Achievement؛ `0x193E` MapPing؛ `0x203B` Chest؛ `0x8888` GSQueue.
- `0x35FE/0x3539` Self-Teleport؛ `0x3501` Hwan Title؛ `0x7061` GetUp.

---

## 19. Unknown / Needs Verification

| البند | السبب |
|---|---|
| نصوص جسم الـStored Procedures (مثل `_HandleHwidList`, `_FilterStartup`, `Hook_*`, `Command_*`) | ليست في شجرة السورس؛ تُدار في قاعدة البيانات (`docs/kmtguard_customer_database_reference.md` يصف الاستخدام لكن ليس الجسم الفعلي). |
| العدد الدقيق للجداول (مذكور `106 جدول` و`109 Procedure` في مرجع العملاء المؤرخ 2026-07-26) | لم يُتحقق من قاعدة فعلية؛ قد يتغير بعد migrations لاحقة. |
| `Command_GameServerQueue` تغليف `0x8888` داخل الـGameServer | يوجد دليل في مرجع العملاء؛ كود الـGameServer يحتوي `KMTGuardCustom/` (GObjEvents, DamageMeter, DurabilityControl, HonorRankRuntimeRefresh, PartyMonsterControl, CGObjPCCustom) لكن تعامل `0x8888` التفصيلي لم يُفحص سطرًا بسطر. |
| عناوين ودوال الـGameServer الإضافية داخل `SR_GameServer` | تم فحص أسماء الملفات (CmdSrcNet.h يحوي `sClientContext` (JID/AgentSessionId/ClientSessionId/SecurityGroups) و`CCmdSrcNet` بـ `NewMsg/FreeMsg`؛ `KMTGuardCustom/` يحتوي تخصيصات) لكن التنفيذ الكامل للدوال لم يُقرأ بالكامل. |
| — (تم التحقق) `Scheduler.cs` | يقرأ `System_Schedule` (RepeatType: None/Interval/Daily/Weekly؛ `ExecutionTimeoutSeconds` افتراضي 7200؛ `CatchUpWindowSeconds` افتراضي 300؛ `MaxConcurrentJobs=4`؛ `LeaseSeconds=120`؛ `HeartbeatSeconds=30`؛ `ScheduleRefreshSeconds=30`؛ يسجل في `System_ScheduleHistory` ويكتب `RunningToken/RunningBy/LeaseUntilUtc/LastStatus/LastError/LastDurationMs`). |
| — (تم التحقق) `SilkroadLauncher/Network` | `GatewayModule.cs`: أوبكودات Gateway (0x6100 Patch Request، 0x6101 Shard List، 0x6102 Login، 0x6104 Web Notice، 0x6106 Shard Ping، 0x6323 Captcha؛ وA100/A101/A102/A104/2322/A323). `DownloadModule.cs`: تنزيل (0x6004 File Request، 0x6005 Ready، 0x1001 Chunk، 0xA004 Completed) + فك zlib + استيراد media.pk2 + تحديث SV.T. |
| ملفات `imgui_windows/*` | موجودة لكن غير مستخدمة افتراضيًا (CONFIG_IMGUI معطلة في `DllMain.cpp`). |
| بقية `Util.cpp` المتعلقة بـ`OngoingNetMessage` (الكثير منها معطّل في comments) | بعض المسارات معلّقة؛ لم يُتحقق أي منها مفعّل فعليًا سوى `0x009ebde2`. |
| `Database/Models` التفصيلية لكل عمود | الأسماء مؤكدة؛ مخطط كل جدول لم يُفحص من DB فعلية. |
| عنوان `0x777…` في `Util.cpp` وعدد كبير من الـreplaceOffset (مثل `OngoingNetMessage`, `CIFMessageBox.OnClickConfirm`) | كثيرة وتحتاج جدولًا موسعًا من المطور قبل أي تعديل. |
| — (تم التحقق) `webviewer.json` | محتواه: `buttons[]` كل زر يحمل `name/icon/url/frameWidth/frameHeight` (مثال Website ← `clientlibrary\guides\kmt_web_viewer_1.ddj`، frame 1000x650). |

---

## خاتمة التحقق
- أسماء الملفات والمسارات المذكورة أعلاه مطابقة لما رُصد في القوائم الفعلية (`list_files`) والقراءات المباشرة.
- المعلومات المُقرأة مباشرة من الكود مميزة كـ«متحقق منها»؛ المعلومات المعتمدة على التوثيق الداخلي فقط مذكورة مصدرها.
- **لم يتم تعديل أي Source File أثناء إعداد هذا المرجع، ولم يُشغَّل أي Build/Compile/Test أو المشروع.**
