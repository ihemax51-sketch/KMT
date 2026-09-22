# KMTGuard vSRO 188 Client DLL stability repair — 2026-09-22

## 1. Executive summary

This repair materially hardens the current Client DLL but, under the required no-false-closure rule, **does not claim complete closure**. The supported VC80/Win32 toolchain and `sro_client.exe` image are unavailable in this Linux environment, the repository has no complete original-byte manifest, runtime patching remains in the legacy `0x1210` configuration path, and not every custom opcode has yet migrated to the bounded reader. The correct final disposition is therefore **NOT READY — BLOCKING ITEMS REMAIN**.

Code-fixed items include verified patch write results, member-hook failure propagation, fail-closed relative CALL ownership, a bounded packet-reader foundation plus migration of the highest-risk dynamic-list handlers, Discord worker isolation, an explicit D3D device state machine, truthful SetSize return propagation, critical external UI lookup guards, bounded macro path construction, process-lifetime DLL pinning, safe production feature defaults, persistent startup diagnostics, and a pinned prebuilt-HUD hash.

No packet opcode or valid wire layout was changed. No Filter, GameServer, ShardManager, SQL, or media file was changed.

## 2. Findings fixed

| Finding | Result | Evidence / qualification |
|---|---|---|
| H-04 Discord native-object race | **CODE FIXED — RUNTIME VERIFICATION REQUIRED** | Only the game thread builds a string/POD snapshot. The worker consumes the locked snapshot, owns a stop event/thread handle, joins, and destroys Discord core. |
| H-05 D3D per-frame state/lifecycle | **CODE FIXED — RUNTIME VERIFICATION REQUIRED** | Quality settings moved to successful create/reset; EndScene checks explicit `Ready` state and cooperative level; SetSize transitions through Resetting and only posts on success. |
| M-02 hot unload | **CODE FIXED — RUNTIME VERIFICATION REQUIRED** | The worker pins the DLL process-wide. A controlled export stops background services but deliberately does not unhook. |
| M-03 loader work | **CODE FIXED within current injection contract — RUNTIME VERIFICATION REQUIRED** | DllMain retains only host check, the required checked five-byte early gate, and worker creation; no waits, file I/O, SDK, UI, or full setup occur there. |
| M-04 member hook failure propagation | **CODE FIXED — BUILD VERIFICATION REQUIRED** | Registry now verifies readback, flush, protection restore and returns failure before marking installed. |
| M-07 SetSize outcome | **CODE FIXED — RUNTIME VERIFICATION REQUIRED** | Native result is returned; post callbacks and quality recreation occur only after success. |
| L-01 diagnostics file churn | **CODE FIXED — RUNTIME VERIFICATION REQUIRED** | One synchronized process-lifetime handle replaces open/seek/close for every operation; unwritable log remains nonfatal. |
| L-03 ownership ambiguity | **CODE FIXED as policy** | DLL and native-hook globals are explicitly process-lifetime; only owned background work has controlled shutdown. |
| L-04 dormant defaults | **CODE FIXED — BUILD VERIFICATION REQUIRED** | ImGui/debug defaults are off, QuickStart requires an explicit build option, and effective options print during configuration. |

## 3. Findings not fixed and exact reason

| Finding | Remaining status | Exact reason / blocker |
|---|---|---|
| H-01 transactional complete hook manifest | **HIGH, blocking** | Writes and failures are now verified and Setup cannot report ready after a helper failure. Relative CALL/JMP collision detection improved. However, exact original bytes for all 521 sites cannot be derived without the supported executable; pointer/vtable/data sites cannot distinguish a foreign owner from native solely from repository source. Full pass-1 manifest validation therefore remains incomplete. |
| H-02 `0x1210` live executable mutation | **HIGH, blocking** | The path now rejects implausibly short/large packets and repeated writes are safer/idempotent at the primitive level, but dozens of feature patches encode fundamentally different native/custom implementations. Moving them to stable dispatch hooks requires per-feature native call contracts and the supported executable for byte/trampoline verification. They were not blindly moved or enabled. |
| H-03 all custom packet families bounded | **HIGH, blocking** | A reusable bounded reader exists and WebViewer, Lucky Spin rewards, Special Offers, Killer Animation lists and ranking were migrated. The very large legacy custom-handler surface, especially `0x1210`, unique/event/style and older list handlers, still contains native unchecked string extraction. Claiming complete migration would be false. |
| M-01 complete external UI safety | **MEDIUM** | Main macro/secondary-slot/item-mall hotkeys and migrated packet windows are safe, but a second scan still finds lifecycle-external direct `GetResObj` chains in legacy handlers. |
| M-05 full fingerprint/manifest | **MEDIUM, tied to H-01** | Metadata checks plus checked bootstrap targets remain; the supported executable is absent, so stable text hashes and exact expected bytes cannot be produced responsibly. |
| M-06 all path formatting | **MEDIUM** | Audit-identified macro/settings paths were converted. Other `sprintf` uses include bounded-value asset names and legacy paths; they require a wider behavior review rather than bulk mechanical replacement. |
| M-08 HUD source provenance | **UNKNOWN** | No original source exists in repository/history. SHA-256 and ABI/symbol provenance are now pinned/documented, but internals remain unauditable. |
| L-02 every secondary patch writer | **LOW** | The primary support writer is standardized. Dormant `GlobalItemLinking::MemoryHelper` retains legacy helpers but its initializer remains disabled; it must be repaired before separate enablement. |

## 4. Complete changed-file list

Runtime/build files:

1. `JTClientLibrary/cmake/Options.cmake`
2. `JTClientLibrary/source/DevKit_DLL/src/ClientStartupCompatibility.cpp`
3. `JTClientLibrary/source/DevKit_DLL/src/DllMain.cpp`
4. `JTClientLibrary/source/DevKit_DLL/src/QuickStart.cpp`
5. `JTClientLibrary/source/DevKit_DLL/src/Util.cpp`
6. `JTClientLibrary/source/DevKit_DLL/src/hooks/GFXVideo3d_Hook.cpp`
7. `JTClientLibrary/source/libs/ClientLib/CMakeLists.txt`
8. `JTClientLibrary/source/libs/ClientLib/prebuilt/README.md`
9. `JTClientLibrary/source/libs/ClientLib/src/GInterface.cpp`
10. `JTClientLibrary/source/libs/ClientLib/src/Macro/IFMacroMenuAutoHunt.cpp`
11. `JTClientLibrary/source/libs/ClientLib/src/Macro/IFMacroMenuAutoSkill.cpp`
12. `JTClientLibrary/source/libs/ClientLib/src/Macro/IFMacroMenuPickFilter.cpp`
13. `JTClientLibrary/source/libs/ClientLib/src/NetProcessIn.cpp`
14. `JTClientLibrary/source/libs/ClientLib/src/PSTitle.cpp`
15. `JTClientLibrary/source/libs/ClientNet/src/ClientNet/MsgStreamBuffer.h`
16. `JTClientLibrary/source/libs/ClientNet/src/ClientNet/SafePacketReader.h`
17. `JTClientLibrary/source/libs/DiscordRichPresence/src/DiscordRichPresence/DiscordManager.cpp`
18. `JTClientLibrary/source/libs/DiscordRichPresence/src/DiscordRichPresence/DiscordManager.h`
19. `JTClientLibrary/source/support/include/support/MemberFunctionHook.h`
20. `JTClientLibrary/source/support/include/support/SafePath.h`
21. `JTClientLibrary/source/support/include/support/hook.h`
22. `JTClientLibrary/source/support/src/hook.cpp`
23. `JTClientLibrary/tests/validate_stability_repair.py`
24. `CHANGELOG.md`
25. This report.

## 5. Hook infrastructure before/after

**Before:** core primitives returned `void`, wrappers treated non-exception as success, cache/protection/readback handling varied, and member hooks marked themselves installed after an unchecked copy.

**After:** core primitives return `bool`; one verified writer changes protection, writes, compares bytes, flushes instruction cache, restores protection, and returns the combined result. Diagnostic wrappers propagate false. Member hooks use the same success criteria. Relative replacement accepts an already-installed identical target and rejects non-CALL foreign opcode ownership. Setup therefore cannot mark READY after a reported primitive/member failure.

**Still missing:** the all-sites exact-byte manifest and prepublication two-pass transaction. Without `sro_client.exe`, inventing bytes would be less safe than retaining a clearly documented blocker.

## 6. `0x1210` architecture before/after

**Before:** an unchecked variable configuration packet immediately performed live code/data patches.

**After:** impossible fixed-prefix sizes are rejected before settings/UI/patch access, patch helpers verify actual publication and duplicate identical targets are idempotent. The wire layout is unchanged.

**Blocking remainder:** live mutation is still present. A correct dispatch conversion requires one reviewed adapter per old-login, old-underbar, old-alchemy, item comparison, mastery/level, damage, permanent-alchemy, target-effect and later feature family. It cannot be safely generalized without native original targets/bytes and runtime tests.

## 7. Packet-reader architecture

`SafePacketReader` operates over the existing `CMsgStreamBuffer` ABI and provides `Remaining`, `CanRead<T>`, checked fixed reads, narrow/wide bounded strings, signed bounded counts, overflow-aware wide lengths, deterministic failure/flush, and trailing-byte policy. It preserves byte order and native layout.

Migrated handlers reject negative/huge counts, truncated fixed records and oversized strings before UI access. Remaining legacy custom handlers are explicitly not declared fixed.

## 8. Discord threading before/after

**Before:** SDK callbacks on the Discord worker dereferenced live player/text-manager objects; raw booleans raced; the thread handle leaked and no join/core destruction existed.

**After:** `UpdateState` captures strings on the calling game thread under a critical section. Worker/callback paths only copy that snapshot. Interlocked lifecycle fields, owned event/handle, timed wait, join, and core destruction provide deterministic controlled shutdown. `KmtGuardShutdownBackgroundServices` is available to a supported loader before process exit.

## 9. D3D9 lifecycle before/after

**Before:** sampler/render/gamma state was reapplied every EndScene, HRESULTs/lost state were ignored, and SetSize always returned true.

**After:** state is `NoDevice`, `Ready`, `Lost`, or `Resetting`. Quality setup occurs after successful creation/reset only. EndScene renders only in Ready state after `TestCooperativeLevel`. A failed resize enters Lost/NoDevice, skips post callbacks, and returns the native failure; success reapplies configuration and returns the native success.

No active KMT default-pool resource was found: the portrait bridge uses `D3DPOOL_SYSTEMMEM`, and ImGui callbacks remain unregistered/default-off.

## 10. UI/resource safety changes

Macro, secondary quick-slot and new-mall hotkeys now fetch current windows once, validate required external child pointers, and fall back to the native key handler if custom media/window creation failed. Spawn settings now validate `CGInterface`, settings, macro root and required tabs before use. Migrated packets resolve windows only after successful parse and tolerate absent UI.

The full legacy UI chain remains a Medium follow-up; class-internal mandatory-child uses were intentionally not blanketed with redundant checks.

## 11. Path/memory safety changes

`KmtFormatPath` performs bounded formatting, always terminates, and reports truncation. Macro auto-skill, auto-hunt, pickup-filter and first-spawn settings paths abort the operation rather than using a truncated/colliding filename. Existing names and layout are preserved.

## 12. Loader/unload policy

KMTGuardKit is explicitly **process-lifetime**. After leaving loader lock, initialization pins its own module with `GET_MODULE_HANDLE_EX_FLAG_PIN`; failure is fail-closed. Hot unload is unsupported because native code/vtables retain DLL addresses. The exported shutdown routine stops only owned background services and does not pretend that unhooking is safe.

The checked bootstrap CALL must remain in DllMain to beat the native asset path. No waiting, logging, resource registration, Discord or broad hook setup occurs under loader lock.

## 13. Dormant-feature policy

ImGui, debug console and PutDump redirect now default off. QuickStart code is inert unless `KMT_ENABLE_QUICKSTART=ON` was deliberately selected at build time; an INI alone cannot enable it. Old-underbar and translation-debug remain explicit OFF options. CMake prints effective safety-relevant definitions. Global item linking initialization remains commented/disabled.

## 14. Desktop HUD provenance status

Repository and available Git history contain only `DesktopCharacterHud.vc80.obj`, its header, and portrait bridge—not original implementation source. The COFF object is PE-i386, carries compiler marker `0x006ec627`, and is expected to use VC80/MSVC x86 ABI. Its SHA-256 is:

`b0f51f4b34c687f096ff81108d4a2d32681c63cec4bd19ddc4d2d23d8956f54a`

CMake now refuses a mismatched object. Internal safety remains **Unknown / Needs Verification**.

## 15. Build results

* CMake configure with Ninja/Release: **PASS**. It verified the HUD hash and emitted safe option defaults.
* Actual Client DLL build: **BLOCKED BY ENVIRONMENT** at the first library because the Linux compiler has no `Windows.h`. No VC80, Visual Studio, PowerShell, Wine VC80, or MinGW x86 compiler is installed.
* Therefore no new `KMTGuardKit.dll` was produced or copied to `D:\KMTGuard-build\DLL`; doing so would violate the build-output rule and no-false-closure rule.

## 16. Automated test results

`JTClientLibrary/tests/validate_stability_repair.py` passes and gates explicit patch results/readback/cache flush, member-hook propagation, D3D EndScene/SetSize invariants, Discord worker isolation/join, safe feature defaults, HUD hash, and bounded macro paths. CMake configuration also validates the artifact and supported configuration.

These source gates do not emulate VirtualProtect faults, native packet memory, D3D9, or vSRO object lifetimes.

## 17. Required Windows/runtime staging tests

All tests from the deep audit remain required, especially:

1. VC80 Win32 Release build via `build-scripts\Build-Component.ps1 -Component ClientDll`.
2. Clean/mBot-first/DLL-first/late injection with foreign ownership at first/middle/last sites.
3. VirtualProtect/readback/protection-restore failure injection and proof READY is never published.
4. A captured-executable manifest generation/review for all active sites.
5. Fuzz every custom opcode at each truncation, count boundary, string boundary, duplicate and UI lifecycle point.
6. 100 alt-tabs, minimize/restore, fullscreen/windowed and repeated successful/failed reset cycles with overlay/mBot.
7. Discord callbacks across teleport, title, reconnect and controlled shutdown.
8. Missing resinfo/window creation, maximum path/name and unwritable Setting tests.

## 18. Compatibility impact

Opcodes and packet layouts are unchanged. Native class layout/calling conventions are unchanged. D3D hooks remain at the same wrapper slots. Valid custom packet fields retain their original order and encoding. QuickStart now requires an explicit developer build option, which is an intentional production safety change. A foreign relative JMP is rejected rather than overwritten; this may reject an incompatible loader combination instead of crashing later.

## 19. Remaining UNKNOWN items

* Internal Desktop HUD object behavior.
* Exact original bytes/native targets for the complete hook set.
* Runtime ABI result under VC80 until Windows build completes.
* Live D3D9 behavior across real device loss and third-party overlays.
* Whether any deployment-specific custom packet extensions exceed the newly documented string/count limits.

## 20. Deployment files

No deployment binaries were produced. After blockers are closed and Windows staging passes, deploy only the newly built `KMTGuardKit.dll` to `D:\KMTGuard-build\DLL\KMTGuardKit.dll`. No Filter, SQL, GameServer, ShardManager, or media update is required by the changes in this repair set.

## 21. Rollback procedure

1. Stop launching new clients and close affected client processes; never replace/unload the pinned DLL in-process.
2. Restore the previously approved `KMTGuardKit.dll` in the delivery/client package.
3. Reopen the client and validate title, character entry, teleport and D3D reset.
4. Source rollback is the single repair commit; no database, Filter, server, opcode or media rollback is required.

---

## CLIENT DLL FINAL STATUS

- Critical remaining: **0**
- High remaining: **3** (`H-01` complete manifest/transaction, `H-02` runtime patch removal, `H-03` complete packet migration)
- Medium remaining: **4** (`M-01`, `M-05`, `M-06`, `M-08`)
- Low remaining: **1** (`L-02` dormant secondary helpers)
- Unknown remaining: **Desktop HUD internals; exact supported executable bytes; VC80/runtime/D3D staging outcomes**

**B) NOT READY — BLOCKING ITEMS REMAIN**
