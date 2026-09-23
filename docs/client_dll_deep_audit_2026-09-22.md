# KMTGuard vSRO 188 Client DLL deep source audit — 2026-09-22

## 1. Executive summary

This is a new audit of the current tree, not a restatement of the earlier platform audit. The review covered all **2,962 files** under `JTClientLibrary/`: all 421 `.cpp`/`.c` translation units, 827 `.h`/`.hpp` headers, build/configuration files, the 1,109 shipped resource/media files, the prebuilt desktop-HUD object boundary, and bundled third-party code. The detailed control-flow review concentrated on the 104,193 lines of first-party C/C++ that can enter `KMTGuardKit.dll`; vendor SDK/examples were dependency-reviewed rather than treated as KMT-owned runtime behavior.

The source declares **521 fixed-address hook or inline-patch sites** (including conditional and dormant sites). This count is source sites, not the number necessarily installed in one run: 387 are in `Setup`, 79 are feature-dependent title/bootstrap patches, 34 are static member-function hooks, and the remainder are bootstrap, quick-start, UI-layout, compatibility, and item-linking sites. The baseline has **0 Critical, 5 High, 8 Medium, and 4 Low findings**.

The most important conclusion is that the recent startup work prevents the client from *continuing* after a detected setup failure, but does not make hook publication transactional. Most patch primitives neither validate ownership/original bytes nor return failure, so their diagnostic wrappers can report success after a failed write. The D3D null guard fixes the immediate null-device dereference, but the render hook still changes device state every frame, does not preserve it, and has no complete lost/reset lifecycle. Both repairs are therefore **partially safe and require further work**.

No runtime source, packet, opcode, SQL, resource, or build output was changed by this audit.

### Finding totals

| Severity | Active | Dormant | Unknown | Total |
|---|---:|---:|---:|---:|
| Critical | 0 | 0 | 0 | **0** |
| High | 5 | 0 | 0 | **5** |
| Medium | 7 | 0 | 1 | **8** |
| Low | 3 | 0 | 1 | **4** |

## 2. Client DLL architecture map

| Layer | Primary source | Responsibility and lifetime |
|---|---|---|
| PE entry/bootstrap | `DevKit_DLL/src/DllMain.cpp` | Validates the x86 host, publishes the early asset-init gate, starts one initialization worker. Process-lifetime only; no detach path. |
| Compatibility gate | `ClientStartupCompatibility.cpp`, `hooks/CGame_Hook.cpp` | Atomic initialization state, 120-second game-thread wait, mBot child-list teardown guard. |
| Patch publication | `DevKit_DLL/src/Util.cpp`, `support/src/hook.cpp`, `MemberFunctionHook.h` | Installs fixed-address JMP/CALL displacement, direct pointer/vtable, and inline-byte patches. |
| Native ABI wrappers | `ClientLib/src`, `ClientNet/src`, `JMX_Library` | x86 `thiscall` wrappers and exact-layout types for the supported v188 image. |
| UI/runtime classes | `ClientLib/src/{Menu,CustomInterface,Macro,...}` | Registers custom GFX runtime classes, loads resinfo, owns child pointers and feature state. |
| Network | `NetProcessIn.cpp`, `NetProcessSecond.cpp`, `NetProcessThird.cpp`, `PSTitle.cpp` | Adds native and custom packet handlers and passes selected native packets through. |
| Render | `hooks/GFXVideo3d_Hook.cpp`, `ExtraUI/DesktopCharacterHudPortraitBridge.cpp` | Hooks the native graphics wrapper, optional callbacks, sampler/gamma tuning and portrait texture work. ImGui callbacks are compiled but not registered. |
| Automation | `Macro/*`, `SkillAutomationController.*`, `AutoPotion.*` | Runs on the native UI timer/update path; no independent automation worker was found. |
| Background integration | `DiscordRichPresence/.../DiscordManager.cpp` | One unmanaged Discord callback thread, process-lifetime global state. |
| Media | `clientlibrary`, `media`, `client-resources` | Resinfo/DDJ ownership for custom windows. Missing or divergent media is not uniformly tolerated. |

## 3. Initialization/lifecycle flow

Observed flow:

1. `DllMain(DLL_PROCESS_ATTACH)` disables thread notifications.
2. It checks filename, preferred base, PE32 magic, image size and timestamp.
3. For the supported host it validates the `CALL` at `0x00832A11`, verifies its current target, and changes its displacement to `InitGameAssets_Impl` while still under loader lock.
4. It creates `KMTGuardInitializationThread`, closes only the thread handle, and returns from loader lock.
5. The worker atomically claims initialization, repeats the image validation, constructs settings/custom-data/player singletons, registers runtime classes and overrides in vectors, and registers pre-init callbacks.
6. `SetupWithDiagnostics` calls `Setup`: image validation, static member hooks, teardown guard, then hundreds of sequential patch writes. It also starts Discord RPC, applies visual patches, and optionally applies QuickStart.
7. Success sets initialization state `2`. Failure sets `-1` and terminates the process rather than letting a partial hook set run.
8. The client startup thread, now redirected through `InitGameAssets_Impl`, waits up to 120 seconds. It then installs runtime classes on that game thread before calling the native asset initializer.
9. Native `CGInterface::OnCreateIMPL` creates resource-dependent windows; the three network-process registration replacements install packet handlers; the D3D wrapper vtable invokes create/end-scene/set-size hooks.

There is no detach/unhook flow. The design is valid only for process-lifetime injection. `FreeLibrary`, loader-driven hot unload, or replacement of the DLL while the process lives is unsupported and unsafe (KMT-M-02).

Failure behavior is fail-closed at the *process* level, but not transactional: a patch helper can silently fail and allow setup to be marked ready (KMT-H-01), while a detected later failure terminates the entire client. The early gate serializes asset initialization, not every other startup thread or third-party hook installer.

## 4. Complete hook inventory

### Counting and common attributes

The complete source scan found **521 hook/patch sites**: 322 relative-offset replacements, 76 direct address replacements, 42 vtable replacements, 34 registered five-byte member JMPs, 24 NOP sites, 14 one-byte patches, 4 direct five-byte JMP sites, 3 byte-block writes, one explicit `JMPFunction`, and one JZ-to-JMP patch. In addition, the bootstrap and child-list guards are included in the 521 total as hand-written checked CALL-displacement hooks, and the item-linking detour is included as a conditional/dormant detour.

Unless a row says otherwise, all sites target the single fingerprint `(base 0x00400000, SizeOfImage 0x00D70000, TimeDateStamp 0x4E311CB6)`, are installed once, are **not reversible**, have no saved original bytes, and execute on either the initialization worker (`Setup`) or native packet/UI thread (feature-dependent `PSTitle` patches). Member targets use MSVC x86 member ABI (`thiscall`, with compiler thunks where applicable); naked targets state their own preservation contract.

### Inventory by installer/family

This family inventory covers every source site; contiguous sites with identical installer, primitive, ownership and timing are intentionally grouped rather than repeating 521 identical “expected bytes: not encoded” rows.

| Hook family / source sites | Count | Type / target | Expected original bytes | Convention | Timing / thread | Reversible / partial state |
|---|---:|---|---|---|---|---|
| Asset bootstrap gate, `CGame_Hook.cpp:52-82` | 1 | checked CALL displacement at `0x00832A11` | `E8` and target `0x00849110` | member `thiscall` | `DllMain`, loader thread | No; rejects foreign owner before write |
| mBot child teardown guard, `ClientStartupCompatibility.cpp:71-108` | 1 | checked CALL displacement at `0x00B92BA5` | `E8` and target `0x00571540` | `__fastcall` shim to native `thiscall` | Setup worker | No; rejects foreign owner |
| Static `HOOK_ORIGINAL_MEMBER` registry across `IFExtQuickSlotOption`, `GInterfaceSend`, `IFTargetWindow*`, `IFMainPopup`, `GInterfaceWndRelation`, `IFChatOptionBoard`, and `PSTitle` | 34 | five-byte JMP at named function entry | Not encoded | member `thiscall` | first Setup operation, worker | No; each site independent and silent on write failure |
| Core graphics wrapper, `Util.cpp:331-333` | 3 | vtable slots 17/26/20 at `0x00E0963C` | Not encoded | member `thiscall` | Setup worker | No; partial slots possible |
| Interface lifecycle/input, `Util.cpp:335-338,467` | 5 | CGInterface vtable/direct member entries | Not encoded | member `thiscall` | Setup worker | No |
| Native actor vtables/name/effect, `Util.cpp:340-345,967`; `PSTitle.cpp:1276` | 8 | vtable/CALL redirects | Not encoded | member `thiscall` | worker, plus packet-time feature patch | No; packet-time collision possible |
| Window procedure, `Util.cpp:348` | 1 | direct callback pointer at `0x0083133B` | Not encoded | Win32 `CALLBACK` | Setup worker | No |
| Guide/action/game lifecycle, `Util.cpp:350-360` | 7 | entry JMP and CALL redirects | Only bootstrap separately verifies one CALL | mixed naked/member | Setup worker | No; action hook restores three displaced instructions |
| World map, `Util.cpp:370-392` | 7 active | resinfo pointer, vtable slots, CALL redirects | Not encoded | member | Setup worker | No |
| Title screen/connect, `Util.cpp:417-425` | 7 | direct object entries/vtable/CALL | Not encoded | member | Setup worker | No |
| Character visuals/help bubbles, `Util.cpp:428-456` | 15 | CALL redirects | Not encoded | member | Setup worker | No |
| Skill board/equipment/inventory, `Util.cpp:461-479,1329`; `IFEquipment.cpp:100-109` | 15 | vtable, pointer tables, byte patches | Not encoded | member/data | Setup worker and UI creation | No |
| Character select, `Util.cpp:488-501`; `PSCharacterSelect.cpp:168` | 9 | vtable/direct/CALL and byte patch | Not encoded | member | Setup worker/UI create | No |
| Underbar/quick slots, `Util.cpp:505-554`; optional `DllMain.cpp:363` | 34 | CALL redirects/direct pointer | Not encoded | member | Setup worker | No |
| Message boxes/stall, `Util.cpp:561-700`; `IFStallSlot.cpp:11-19`; `PSTitle.cpp:1441-1455` | 100+ | CALL/direct/vtable/NOP/byte patches | Not encoded | member | worker and packet-time | No; duplicate sites exist |
| Native NPC talk fan-out, `Util.cpp:704-927` | 100+ | CALL displacement redirects plus one vtable/direct entry | Not encoded | member | Setup worker | No |
| Edit/minimap/inventory/chat/console, `Util.cpp:940-984` | 10 | direct/vtable/CALL/JMP/NOP | Not encoded | mixed | Setup worker | No |
| Watermark, `Util.cpp:1387-1419` | 2 | five-byte push-immediate blocks | Not encoded | inline data/code | Setup worker | No; static string must remain process-long |
| QuickStart, `QuickStart.cpp:44-53` | 5 | three pointer replacements, two function entries | Not encoded | member/data | end of Setup if INI enables it | No; dormant unless configured |
| Server-config feature patches, `PSTitle.cpp:1128-1276,1439-1548,1862` | 79 | CALL/direct/vtable/NOP/byte/JMP/Jcc patches | Not encoded | mixed | packet `0x1210`, game/network thread | No; live and non-transactional |
| Settings visual patch, `IFSettings.cpp:244` | 1 | ten NOPs | Not encoded | inline | UI action | No |
| Item linking, `GlobalItemLinking.cpp:140` | 1 | five-byte detour | Not encoded | naked shim | dormant (`Initialize` call commented) | No |
| Native notify CALL guard, `IFNotify.cpp:48-63` | 1 | checked opcode-only CALL displacement | only `E8`; original target not checked | free/static | feature initialization | No |

### Hook-safety verdict

* Instruction boundaries can only be confirmed for the two checked CALL sites and the naked action prologue whose displaced instructions are explicitly replayed. The remaining entry JMPs assume a five-byte boundary; CALL replacements assume the byte at the site is already `E8/E9`, but do not verify it.
* The shared helpers do flush instruction cache, except `MemberFunctionHookRegistry::Apply`, `MemoryHelper::Detour`, and several secondary patch helpers. Protection restoration is attempted, but most APIs discard its result.
* There is no trampoline containing overwritten bytes. `HOOK_ORIGINAL_MEMBER` is a replacement despite its name; callers reach native bodies through separate fixed-address wrappers.
* No FPU/SSE save is required by the inspected C++ member hooks under the MSVC x86 ABI. The action naked hook uses `pushad`/`pushfd`, restores them, replays `push ebp; mov ebp,esp; and esp,-8`, and jumps to `0x00793B06`.
* Third-party compatibility is explicitly checked at only two call sites. Every other site can overwrite another module's detour or be overwritten later.

## 5. Critical findings

No issue met the requested Critical threshold. The audit did not prove a common, direct arbitrary memory overwrite in the normal supported packet/media path.

## 6. High findings

### KMT-H-01 — Hook helpers report success after failed or foreign-owned writes

* **Severity / reachability:** HIGH / ACTIVE
* **File / function / lines:** `source/support/src/hook.cpp`, `placeHook`, `replaceOffset`, `replaceAddr`, lines 5-75; `DevKit_DLL/src/Util.cpp`, diagnostic wrappers and `Setup`, lines 173-239 and 319-333.
* **Behavior:** the primary patch functions return `void`, validate no original opcode/bytes, and only print on `VirtualProtect` failure. Diagnostic wrappers always return true unless an SEH exception occurs. Thus Setup can publish `ready` after an unapplied slot, a partially applied family, or overwriting a third-party detour.
* **Execution path:** initialization worker → `SetupWithDiagnostics` → sequential hook macro → void helper → wrapper logs “end” → initialization state becomes success.
* **Failure scenario:** mBot/overlay/another client DLL owns one site, or page-protection restore/write fails. A caller subsequently enters a mixture of native and KMT assumptions.
* **Why real for vSRO:** the DLL explicitly supports injection beside loaders/bots and installs hundreds of fixed sites after process start. Metadata fingerprinting does not establish current byte ownership.
* **Expected symptom:** startup AV, crash opening a particular window, packet-handler crash, or apparently random later failure rather than a clean startup rejection.
* **Minimal fix:** make all primitives return verified success; define expected bytes/opcode/current target for every site; accept native bytes or the same KMT target only; stage validation of all sites before the first write; terminate before publication on conflict.
* **Regression risk:** high because byte manifests must match the exact supported executable and intentional feature-dependent repatches.
* **Required test:** clean client plus mBot and a synthetic competing detour at the first/middle/last site; assert rejection before any mutation and verify every installed byte after setup.

### KMT-H-02 — Server packet `0x1210` performs unverified live code patching

* **Severity / reachability:** HIGH / ACTIVE
* **File / function / lines:** `source/libs/ClientLib/src/PSTitle.cpp`, `CPSTitle::OnServerPacketRecv`, lines 1113-1277 (and later feature patches through line 1548).
* **Behavior:** a server configuration packet triggers CALL, vtable, immediate, JMP and NOP writes on the packet-processing thread. No ownership check, global transaction, thread suspension, or idempotent current-target validation exists.
* **Execution path:** native title packet receive → custom `0x1210` branch → read feature flags → patch instructions while the client and other injected modules are running.
* **Failure scenario:** mBot or an overlay has already detoured a target, or another client thread executes a multi-byte site while it is being changed. The patch overwrites the other owner or exposes a torn instruction.
* **Why real for vSRO:** this is exactly the late-injection/multiple-detour configuration requested for compatibility review; unlike the bootstrap gate, these writes happen after startup concurrency exists.
* **Expected symptom:** intermittent startup/character-select crash, bot incompatibility, or crash only when a server-side feature flag is enabled.
* **Minimal fix:** preinstall stable dispatch hooks during gated setup and switch atomic data flags at packet time; where unavoidable, validate expected bytes/current target and serialize patch ownership.
* **Regression risk:** high; server-configured features and old UI modes depend on these sites.
* **Required test:** toggle each `0x1210` flag on clean client/mBot, replay the packet twice, inject after title creation, and verify byte ownership plus character entry.

### KMT-H-03 — Custom packet readers trust counts, strings and UI readiness

* **Severity / reachability:** HIGH / ACTIVE
* **File / function / lines:** `source/libs/ClientLib/src/NetProcessIn.cpp`, examples `WebViewerConfig` lines 2141-2172, `LoadAttendanceRewardState` 2199-2229, `LoadLuckySpinRewards` 2718-2735, `LoadSpecialOffers` 2773-2803, `LoadKillerAnimations` 2829-2866, `LoadEventRegister` 3180-3200, and `LoadRank` 3427-3452; underlying reads at `ClientNet/src/ClientNet/MsgStreamBuffer.cpp:79-87`.
* **Behavior:** many handlers read the count before confirming even the count field exists, loop without checking remaining bytes per record, accept unbounded signed counts, and let native string readers trust wire lengths. Several then dereference `g_pCGInterface` or a resource object without a readiness/null guard.
* **Execution path:** Filter/server → registered custom opcode → `CNetProcessIn` handler on native network/game path → native `Read` or string extraction → UI update.
* **Failure scenario:** truncated response, count larger than supplied records, corrupt string prefix, duplicate packet before UI creation, or media preventing the target window from instantiating.
* **Why real for vSRO:** custom traffic is supplied by an external Filter and crosses reconnect/character-ready boundaries; packet loss is framed, but version skew, faulty server code, or deliberate malformed injection can still deliver a syntactically framed short payload.
* **Expected symptom:** AV in native buffer/string code, huge loop/freeze/allocation, or missing-UI null dereference.
* **Minimal fix:** a bounded reader facade with `remaining`, capped length/count, per-record minimum checks, exact/allowed trailing-byte policy, and deferred/no-op UI delivery until readiness.
* **Regression risk:** medium; preserve the existing valid layouts and allow explicitly versioned optional tails.
* **Required test:** fuzz every custom opcode with payload lengths 0..minimum+1, `-1/0/max/max+1` counts, oversized string prefixes, duplicate delivery, title-screen delivery and post-reconnect delivery.

### KMT-H-04 — Discord worker reads live native player objects off the game thread and survives shutdown

* **Severity / reachability:** HIGH / ACTIVE
* **File / function / lines:** `source/libs/DiscordRichPresence/src/DiscordRichPresence/DiscordManager.cpp`, `Start` lines 15-23, `UpdateState` lines 35-114, callback/thread lines 128-198.
* **Behavior:** the Discord callback thread invokes `UpdateState`, which dereferences `g_pMyPlayerObj`, its strings, guild and region, and `g_CTextStringManager` without synchronization. `Start` leaks the thread handle; no DLL detach calls `Stop`, joins the worker, or destroys Discord core. Plain `bool` fields are shared cross-thread.
* **Execution path:** Setup worker creates Discord thread → SDK callback `OnUserUpdated` → `m_dc->UpdateState()` → native Silkroad objects while the UI/game thread may replace them at teleport, character select, reconnect or shutdown.
* **Failure scenario:** callback overlaps player destruction/recreation, or process/DLL teardown begins while callback code is running.
* **Why real for vSRO:** native client object lifetimes are controlled by scene transitions and are not documented thread-safe. A null check does not pin the pointed object.
* **Expected symptom:** rare AV after character selection/reconnect/teleport or during shutdown; leaked kernel handle each start.
* **Minimal fix:** snapshot presence data on the game thread into a locked/POD buffer; Discord worker reads only the snapshot. Own the handle, signal stop, join, destroy SDK objects, and use interlocked state.
* **Regression risk:** low to medium; only presence timing changes.
* **Required test:** loop login/teleport/return-to-title/reconnect while forcing Discord callbacks; run Application Verifier/WinDbg and verify deterministic worker termination.

### KMT-H-05 — EndScene mutates global D3D9 state every frame without reset/state restoration

* **Severity / reachability:** HIGH / ACTIVE
* **File / function / lines:** `DevKit_DLL/src/hooks/GFXVideo3d_Hook.cpp`, `ApplyTextureQualityBoost` lines 13-46, `ApplyColorQualityBoost` 48-84, `EndSceneHook` 105-125, `SetSizeHook` 128-147.
* **Behavior:** every frame performs `GetDeviceCaps`, up to 36 state calls and a gamma-ramp update, does not capture/restore sampler/render/gamma state, ignores all HRESULTs, and calls `EndScene` directly. `SetSizeHook` has only pre/post callbacks around the native wrapper; no explicit D3D `OnLostDevice`/`OnResetDevice` resource lifecycle exists.
* **Execution path:** D3D wrapper vtable slot 26 → callbacks → state changes → device `EndScene`; slot 20 runs during resize/reset.
* **Failure scenario:** alt-tab/fullscreen switch/device loss while a stale non-null device remains, another overlay expects its state preserved, or the client depends on stage-specific filtering. Repeated gamma changes also affect desktop/fullscreen behavior.
* **Why real for vSRO:** the hook is active each rendered frame and competes with native rendering and overlays at the most sensitive D3D boundary.
* **Expected symptom:** black/incorrect render, post-alt-tab failure, overlay corruption, first/repeated-reset crash, and measurable FPS loss.
* **Minimal fix:** apply supported quality settings once after successful create/reset (or via a state block with restoration), check cooperative/reset results, explicitly release/recreate DLL-owned default-pool resources, and preserve/chaining ownership with other hooks.
* **Regression risk:** medium; visual output changes and old GPUs need coverage.
* **Required test:** 100 alt-tabs, minimize/restore, five resolution and fullscreen cycles, forced device loss, overlay/mBot coexistence, and before/after frame-time capture.

## 7. Medium findings

### KMT-M-01 — Resource-dependent hotkeys and packets contain reachable null chains

* **Severity / reachability:** MEDIUM / ACTIVE
* **File / function / lines:** `source/libs/ClientLib/src/GInterface.cpp`, `OnKeyDown`, lines 1360-1454; `NetProcessIn.cpp`, `LoadSilkRank` 2260-2266, `LoadRank` 3427-3452, `AddIconToIconManager` 3517-3533.
* **Behavior/path:** a key press or custom packet directly chains through `GetResObj(...)->...`. A missing/mismatched resinfo or failed runtime-class creation makes the result null.
* **Failure scenario / symptom:** media version mismatch or failed custom UI creation, then T/F-key or relevant packet → immediate AV/missing UI.
* **Why real:** the custom media ships separately and the handlers can precede window readiness.
* **Minimal fix:** retrieve once, validate window and required children, no-op/log once when absent.
* **Regression risk/test:** low; test intentionally missing each resinfo and early packet/hotkey delivery.

### KMT-M-02 — Process detach/hot unload is unsupported but not actively blocked

* **Severity / reachability:** MEDIUM / ACTIVE
* **File / function / lines:** `DevKit_DLL/src/DllMain.cpp`, `DllMain`, lines 404-435; Discord lifetime above.
* **Behavior/path:** `DLL_PROCESS_DETACH` returns immediately. Hundreds of client addresses still point into the DLL and the Discord worker may still execute it.
* **Failure scenario / symptom:** a loader calls `FreeLibrary` or reloads the DLL → execution into unmapped code, AV, corrupted hook chain.
* **Minimal fix:** explicitly document/export “not unloadable” and make supported loaders pin the module, or implement quiesce/unhook/join with saved originals.
* **Regression risk/test:** high for a true unload implementation; test loader unload request and process shutdown.

### KMT-M-03 — Nontrivial work remains under loader lock

* **Severity / reachability:** MEDIUM / ACTIVE
* **File / function / lines:** `DllMain.cpp:404-434`; `CGame_Hook.cpp:52-82`.
* **Behavior/path:** DllMain parses the image, uses `VirtualProtect`/`memcpy`/`FlushInstructionCache`, changes code, and calls `CreateThread`. This is smaller than full setup, but still beyond minimal loader-lock work.
* **Failure scenario / symptom:** unusual injector/load ordering or security software causes loader-lock interaction; initialization fails before diagnostics are fully available.
* **Minimal fix:** let a loader-owned bootstrap call an exported initializer before client resume, or use a minimal pre-existing safe gate mechanism; never wait in DllMain.
* **Regression risk/test:** high because the gate must be installed before `0x00832A11`; test earliest and late supported injection paths.

### KMT-M-04 — Static member hook registry neither flushes cache nor propagates failure

* **Severity / reachability:** MEDIUM / ACTIVE
* **File / function / lines:** `source/support/include/support/MemberFunctionHook.h`, `Apply` lines 43-67.
* **Behavior/path:** 34 entry JMPs are copied and marked installed without `FlushInstructionCache`; failed protection restoration still marks success.
* **Failure scenario / symptom:** stale instruction cache or protected/foreign site yields delayed crash when a hooked UI method first executes.
* **Minimal fix:** return status, verify bytes, flush cache, restore protection, and mark installed only after readback.
* **Regression risk/test:** low; force helper failures and exercise every registered member hook.

### KMT-M-05 — Fingerprint validates metadata, not executable text

* **Severity / reachability:** MEDIUM / ACTIVE
* **File / function / lines:** `DllMain.cpp:114-150`; `Util.cpp:270-301`.
* **Behavior/path:** name/base/timestamp/image size can match a locally modified or packed executable whose patch sites differ.
* **Failure scenario / symptom:** modded v188 retains headers but changes one target → incorrect hook and startup or feature crash.
* **Minimal fix:** hash stable `.text` ranges or validate the complete expected-byte manifest before publication.
* **Regression risk/test:** medium; test official image and one-byte mutations at representative sites.

### KMT-M-06 — Fixed path formatting can overflow 256/512-byte stack buffers

* **Severity / reachability:** MEDIUM / ACTIVE
* **File / function / lines:** `NetProcessIn.cpp:6211-6239`; `Macro/IFMacroMenuAutoSkill.cpp:986-990`; `Macro/IFMacroMenuAutoHunt.cpp:1427-1431`; `Macro/IFMacroMenuPickFilter.cpp:283-287`.
* **Behavior/path:** `sprintf` combines an up-to-MAX_PATH working directory, a character name and suffix into fixed arrays without a bound.
* **Failure scenario / symptom:** long installation path plus maximum legal character name exceeds a 256-byte target, corrupting the stack during spawn/settings load.
* **Minimal fix:** `_snprintf_s`/bounded path helper and reject truncation; use the same sanitized character filename routine everywhere.
* **Regression risk/test:** low; test 260-character install path and maximum character-name length.

### KMT-M-07 — D3D SetSize hook discards native outcome

* **Severity / reachability:** MEDIUM / ACTIVE
* **File / function / lines:** `GFXVideo3d_Hook.cpp:128-147`.
* **Behavior/path:** calls `CGFXVideo3d::SetSize(width,height)` but returns `true` unconditionally, so callers cannot see reset/resize failure.
* **Failure scenario / symptom:** unsupported resolution/device-loss reset fails yet the client proceeds as if successful, leading to black screen or a later stale-resource crash.
* **Minimal fix:** capture and return the native result and run post callbacks only on the contractually correct outcome.
* **Regression risk/test:** medium; verify native ABI/return contract, invalid resolutions, device loss and successful resize.

### KMT-M-08 — Prebuilt Desktop HUD object cannot be source-audited

* **Severity / reachability:** MEDIUM / UNKNOWN
* **File / function / lines:** `source/libs/ClientLib/CMakeLists.txt` external object declaration; `prebuilt/DesktopCharacterHud.vc80.obj`.
* **Behavior/path:** the object is linked into ClientLib and its `GameWndProcHook` is registered at initialization, but no corresponding source is present. ABI, pointer lifetime and hidden patch behavior cannot be verified.
* **Failure scenario / symptom:** an issue inside the object can affect every window message and cannot be reconciled with current headers.
* **Minimal fix:** restore/build its source or provide reproducible disassembly, symbols and hash provenance.
* **Regression risk/test:** unknown; compare a source-rebuilt object and exercise all HUD/window messages.

## 8. Low findings

### KMT-L-01 — Startup diagnostics perform hundreds of synchronous file opens

* **Severity / reachability:** LOW / ACTIVE
* **File / function / lines:** `ClientStartupCompatibility.cpp:149-203`; `Util.cpp:108-127,173-239`.
* **Behavior/path/scenario:** each hook emits begin/end by opening, seeking, writing and closing the same file. Slow/filtered disks can materially extend startup and approach the 120-second gate.
* **Symptom/fix/test:** delayed or apparently frozen launch; keep one buffered handle or emit only failures/summary; test on throttled disk and unwritable `Setting` directory.
* **Regression risk:** low.

### KMT-L-02 — Patch protection modes are inconsistent

* **Severity / reachability:** LOW / ACTIVE
* **File / function / lines:** `support/src/hook.cpp:47-70,103-130`; `GlobalItemLinking/MemoryHelper.cpp:8-47`.
* **Behavior:** some code pages are changed to `PAGE_READWRITE`, others `PAGE_EXECUTE_READWRITE`; several helpers ignore restoration failure or omit instruction-cache flush.
* **Scenario/symptom/fix/test:** hardening/DEP tooling may reject or retain an unintended page mode; standardize one verified RAII patch primitive and inspect page protections after setup.
* **Regression risk:** low.

### KMT-L-03 — Global process-lifetime allocations have ambiguous ownership

* **Severity / reachability:** LOW / ACTIVE
* **File / function / lines:** `DllMain.cpp:176-184`; `Util.cpp:1334-1337`.
* **Behavior:** settings/data/player singletons, `MemoryHelper`, item-linking and Discord manager are never destroyed.
* **Scenario/symptom/fix/test:** normal process exit reclaims them, so this is not a live leak symptom; it becomes relevant only to restart/unload. State ownership should be explicit and included in any future shutdown controller.
* **Regression risk:** medium if destructors touch already-destroyed Silkroad state; test only after introducing ordered shutdown.

### KMT-L-04 — Reachability of legacy helper patch code is not consistently documented

* **Severity / reachability:** LOW / UNKNOWN
* **File / function / lines:** `GlobalItemLinking/GlobalItemLinking.cpp:140`; `DllMain.cpp:182-184`.
* **Behavior:** `g_global` is allocated but `Initialize()` is commented, leaving its unsafe detour dormant in this baseline; other build flags (`CONFIG_IMGUI`, old underbar) similarly alter the hook set.
* **Scenario/symptom/fix/test:** accidental build-flag/config activation changes the audited surface; encode feature defaults in generated build metadata and CI-test each supported configuration.
* **Regression risk:** low.

## 9. Accepted vSRO-specific patterns / false positives

* Fixed absolute addresses, x86 `thiscall` wrappers, exact object offsets, and native vtables are accepted for this one fingerprint; they are not findings by themselves.
* `CMsgStreamBuffer` delegates allocation/read/destruction to native functions by exact address. This ABI bridge is accepted; the finding is the absence of caller-level boundary policy for custom layouts.
* Native UI objects are generally owned by the Joymax resource manager. Not deleting pointers returned by `GetResObj` is correct.
* Process-lifetime globals are acceptable for a DLL that is pinned until process exit. They become defective only with the currently unspecified hot-unload path.
* `GetResObj` dereferences inside a successful window's own `OnCreate` are accepted when the mandatory resinfo contract guarantees the child. Findings are limited to packet/hotkey paths where missing media or readiness provides a realistic null path.
* ImGui source exists but callback registration is commented; it has no active reset/resource risk in this build.
* The many native packet forwarding wrappers in `NetProcessSecond/Third` preserve the original handler and are not custom parsers.
* Automation runs from the native `CGInterface::OnTimerIMPL`/window timer path, not arbitrary worker threads. This is the correct thread affinity model; frequency/per-frame cost still needs staging measurement.

## 10. Dormant dangerous code

| Code | Reachability | Risk if enabled |
|---|---|---|
| `GlobalItemLinking::Initialize` detour | DORMANT; call commented | Five-byte detour has no byte validation, cache flush or ownership chaining. |
| ImGui callbacks/windows | DORMANT; registrations commented | Would need complete lost/reset and WndProc chaining audit before enabling. |
| Large commented `OngoingNetMessage` call-site fan-out | DORMANT; only the single active redirect remains | Enabling the bulk block would massively increase collision and packet hot-path cost. |
| QuickStart hooks | DORMANT by default; INI-controlled | Replaces credential pointers and screen callbacks; must never ship enabled unintentionally. |
| Compile-time old-underbar/translation/debug patches | UNKNOWN per build flags | Changes active address set and byte ownership; build artifact should record defines. |

## 11. Previous-fix verification

### `DllMain.cpp`: partially safe

Confirmed improvements:

* Full setup moved off loader lock.
* The host is rejected by name, base and PE fingerprint before normal setup.
* Duplicate initialization is atomically rejected.
* The game asset call site verifies opcode and original target before the worker starts.
* Failed setup terminates rather than allowing a known partial installation to keep running.

Remaining gaps/regressions:

* The gate patch and `CreateThread` still occur under loader lock (KMT-M-03).
* The gate protects asset initialization, not all concurrently executable startup sites.
* Most patch helpers cannot communicate failure, so “setup succeeded” is not reliable (KMT-H-01).
* No detach/unload handling exists (KMT-M-02).
* The reference's claimed multi-instance `CreateMutexA` IAT hook is not present in the current source; current source is authoritative.

### `GFXVideo3d_Hook.cpp`: partially safe, further work required

Confirmed improvements:

* `CreateThingsHook` does not invoke callbacks when native creation fails or the device is null.
* `EndSceneHook` snapshots the member pointer into a local and rejects null before dereference.

Remaining gaps:

* A non-null COM pointer is not proof that a lost/reset/released device remains valid.
* State is changed every frame without preservation; HRESULTs are ignored.
* No explicit default-pool release/recreate sequence exists.
* SetSize ignores native outcome.

Overall verdict: **previous Client DLL fixes are partially safe and require further work**.

## 12. Crash-risk matrix

| Scenario | H-01 | H-02 | H-03 | H-04 | H-05 | M-01/M-07 |
|---|---|---|---|---|---|---|
| Startup | High | High | Medium | Low | Medium | Medium |
| Character select/entry | High | High | High | High | Low | High |
| Teleport | Medium | Low | Medium | High | Low | Medium |
| Reconnect | Medium | Low | High | High | Medium | High |
| Alt-tab/minimize | Low | Low | Low | Low | High | Medium |
| Resolution/fullscreen | Low | Low | Low | Low | High | High |
| Shutdown/hot unload | High | Low | Low | High | Medium | Medium |
| mBot/other hooks | High | High | Low | Low | High | Low |

## 13. D3D9/UI lifecycle matrix

| Transition | Device expectation | DLL-owned behavior | Verdict |
|---|---|---|---|
| First create | native call must succeed and device be non-null | callbacks only after both conditions | Improved/safe for null create |
| Normal frame | valid render-thread device | callbacks, repeated caps/state/gamma, direct EndScene | Active state/performance risk |
| Minimize/lost device | device may be non-null but lost | no cooperative-level/lost notification | Unsafe assumption |
| Reset/SetSize | native result determines success | pre callback, native call, post callback, unconditional true | Incomplete |
| Repeated reset | resources may need repeated release/recreate | no explicit state machine | Unknown/needs staging |
| Character select/teleport | UI/native objects can be recreated | packet and hotkey paths often look up current object, but some chain without checks | Mixed |
| Media missing | class/child creation can fail | many class-local children assumed; several external calls unchecked | Medium crash/missing-UI risk |
| Shutdown | device/UI torn down by native client | no detach or callback quiesce | process-exit only |

## 14. Packet boundary matrix

| Opcode(s) / handler family | Direction | Current bounds policy | UI-ready policy | Verdict |
|---|---|---|---|---|
| Native handlers in Second/Third | server → client | native parser/forwarding | native | Accepted, layout owned by v188 |
| `0x1210` title configuration | server/Filter → client | long sequential reads, optional behavior not version-framed | title exists; applies live patches | High: malformed/version/collision risk |
| `0xA4F0` WebViewerConfig | Filter → client | count capped, but no per-string/remaining checks | interface checked after parse | Partial |
| `0x209A/208B/208C` attendance | Filter → client | some count caps; incomplete per-record checks | some window checks | Partial |
| `0x205E/208D/203B` chest | Filter → client | V2 has caps and minimum checks; V1 strings remain unchecked | window checked | V2 best current pattern; V1 fragile |
| `0x206F/2070` lucky spin | Filter → client | reward count unbounded; no remaining checks | interface/window assumed for lookup | High |
| `0x2072/2073` offers | Filter → client | count/string lengths unbounded | interface assumed | High |
| `0x2077/2078/2079` killer animations | Filter → client | count/string reads unbounded; animation ID validated | current object resolution improved | Partial/high parser risk |
| `0x2074/2075` drop logs | Filter → client | feature caps exist in portions; validate every record in staging | window lookup guarded in current response path | Medium |
| `0x177A/B/C/E/F` achievements/DPS | Filter → client | achievement counts capped; no per-record remaining; DPS byte count | several current-window checks | Partial |
| `0x171B/C/E,208A` unique/event | Filter → client | byte counts but strings/remaining unchecked | event windows often unchecked | High |
| `0x170E/F/171A` ranking | Filter → client | byte count; remaining/string unchecked | one handler dereferences missing ranking | High |
| `0x168B/C/D,170A/B,173F,174A/B/E` style | Filter → client | fixed/string reads unchecked | manager windows sometimes directly dereferenced | Medium/high on early delivery |
| counters/timer/map ping | Filter → client | small fixed layouts, still no common minimum guard | windows mostly checked in repaired paths | Medium |

No source-level maximum payload is imposed by the custom handlers. Native framing may cap packets, but that does not replace per-layout minimum/count/string validation.

## 15. Recommended fixing order

1. **Transactional hook manifest (H-01/M-04/M-05):** one verified primitive, validate all native bytes/owners first, then publish, with readback and cache flush.
2. **Remove packet-time code mutation (H-02):** install dispatch hooks during the gated phase and make `0x1210` change only validated data flags.
3. **Bounded custom packet reader (H-03):** migrate highest-risk count/string handlers first; use Chest V2's checks as a starting pattern, then add string-prefix bounds.
4. **Discord thread isolation and shutdown (H-04):** game-thread snapshot, owned handle, signal/join, SDK destruction.
5. **D3D lifecycle/state repair (H-05/M-07):** state preservation/one-time application, lost/reset state machine, correct return propagation and overlay chaining.
6. **UI readiness/resource guards (M-01):** cache no UI pointers across scene changes; resolve once per action and validate mandatory children.
7. **Bounded filesystem paths (M-06).**
8. **Declare/pin process-lifetime loading or design true unload (M-02/M-03).**
9. **Restore Desktop HUD source provenance (M-08).**
10. **Reduce diagnostics and unify page protection (L-01/L-02).**

## 16. Windows staging test plan

### Build and artifact controls

1. Build Win32 Release with the supported VC toolchain through `build-scripts\Build-Component.ps1 -Component ClientDll` only after repairs exist; this audit itself requires no build.
2. Record SHA-256 for `sro_client.exe`, DLL, prebuilt HUD object and media manifest; assert PE base/size/timestamp and future text hash.
3. Emit the effective compile definitions and compare the installed-hook count to the approved manifest.

### Startup/hook ownership

1. Launch clean, launcher-injected, mBot-first, DLL-first, and deliberately late-injected cases, 100 iterations each.
2. Synthetic competitor owns first, middle and last patch families; expected behavior is rejection before any write.
3. Remove write permission from a test page; expected behavior is verified failure, never ready state.
4. Verify every installed byte/current target and original page protection after initialization.

### Lifecycle/UI/media

1. Loop title → character select → game → teleport → disconnect → reconnect → title 100 times.
2. Delete/rename each custom resinfo and selected DDJ in a staging media copy; exercise every menu, hotkey and packet.
3. Deliver custom packets before `CGInterface` creation, during transition, twice, and after recreation.
4. Run with maximum path/character names and read-only/unwritable Setting directory.

### D3D9

1. 100 alt-tabs in windowed and fullscreen; 25 minimize/restore cycles.
2. Cycle all supported resolutions and fullscreen modes five times; inject forced `D3DERR_DEVICELOST`/reset failures.
3. Run with mBot plus one common overlay and record hook chain ownership.
4. Capture PresentMon/GPU timings before/after; require no material frame-time regression and no sampler/gamma leakage.

### Packets

1. For every custom opcode, generate zero-length, every truncated prefix, exact valid, valid+trailing, max and max+1 inputs.
2. Exercise negative/huge signed counts, `0xFF` byte counts, oversized string prefixes, invalid UTF data and duplicate snapshots.
3. Assert: no AV/hang, bounded allocations/loops, deterministic consume/reject, and no UI access before ready.
4. Replay known-good vSRO/Filter captures to prove byte-for-byte compatibility.

### Threads/shutdown

1. Force Discord callbacks continuously during all lifecycle loops.
2. Verify the worker never calls native Silkroad objects, closes its handle, exits on signal and does not execute after teardown.
3. Test normal process exit, crash exit and loader unload request; until a true unload design exists, the loader must refuse/pin hot unload.

### Exit criteria

No first-chance AV attributable to KMT, no hung initialization, no partial hook state, no packet-driven unbounded loop/allocation, no device/reset leak, no post-teardown DLL execution, and no statistically meaningful FPS regression versus the unmodified supported client.
