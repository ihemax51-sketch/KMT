# KMTGuard vSRO 188 — Macro Bot stability repair

**Repair date:** 2026-09-23
**Baseline:** Macro audit at commit `cc6c002`
**Scope:** `JTClientLibrary` Macro automation only; opcodes, payload layouts, native ABI, and UI-thread affinity are unchanged.

## 1. Executive summary

This repair removes the reachable party-name stack overwrite, guarantees that optional Macro setup cannot suppress native `0x3305` processing, adds fail-closed UI readiness gates, replaces character map merge behavior with per-character transactional swaps, validates persisted hunt values, rate-limits death recovery, reduces pickup from O(E×I) to O(E+I), makes principal Macro saves atomic with sanitized character components, and deletes the unreachable legacy pickup implementation.

No worker thread was added. All automation remains on `CGInterface::OnTimerIMPL`. No opcode or packet field changed. A Macro readiness failure stops/does not start its timers while native Silkroad processing continues.

The code is suitable for a Windows build and runtime staging candidate, but is **not production-verified** in this Linux environment.

## 2. Changed files

- `GInterface.cpp`: per-feature UI readiness gate before timer dispatch.
- `NetProcessIn.cpp`: bounded parser, exactly-once native dispatch on setup failures, value validation, sanitized paths, and transactional character maps.
- Auto Potion/Skill/Hunt/Pick Filter `.cpp/.h`: readiness predicates, safe state, death backoff, pickup cache, and dormant-code deletion.
- `MacroSafety.h`: character-name sanitization, atomic text-file commit, and range helper.
- `validate_macro_stability_repair.py`: source regression gates.
- `CHANGELOG.md`: customer-facing outcome.

## 3. Finding disposition

| Finding | Status | Implementation |
|---|---|---|
| MB-C-01 | **CODE FIXED — RUNTIME VERIFICATION REQUIRED** | `%255ls` bounds the 256-wide buffer. |
| MB-H-01 | **CODE FIXED — RUNTIME VERIFICATION REQUIRED** | `DispatchNative3305` resets the read cursor and is called on every optional-setup exit and the normal path. |
| MB-H-02 | **CODE FIXED — RUNTIME VERIFICATION REQUIRED** | Timer dispatch and feature starts require current feature UI; failure disables Macro and cancels timers. |
| MB-M-01 | **CODE FIXED — RUNTIME VERIFICATION REQUIRED** | radius/enums/hours are validated and timer multiplication uses unsigned bounded hours. |
| MB-M-02 | **CODE FIXED — RUNTIME VERIFICATION REQUIRED** | prior character maps are cleared; parsed party/buff maps are swapped as complete models. |
| MB-M-03 | **CODE FIXED — RUNTIME VERIFICATION REQUIRED** | death recovery has a five-second pending backoff reset after leaving death. |
| MB-M-04 | **CODE FIXED — RUNTIME VERIFICATION REQUIRED** | stack capacity is indexed once per pickup tick: O(E+I). |
| MB-M-05 | **PARTIALLY FIXED** | writes are flush/commit/atomic-replace and names are sanitized; server/account namespace is unavailable in this layer, so identical names sharing one working directory remain an operational Unknown. |
| MB-M-06 | **PARTIALLY FIXED** | party/buff collections commit transactionally; legacy scalar/checkbox hydration is still line-oriented to preserve its format and needs Windows malformed-file testing. |
| MB-L-01 | **CODE FIXED — RUNTIME VERIFICATION REQUIRED** | invalid Windows filename characters are replaced and empty components rejected. |
| MB-L-02 | **PARTIALLY FIXED** | shared readiness boundary removes the proven unsafe entry; duplicated potion family logic remains a maintenance concern and was not rewritten due to regression risk. |
| MB-L-03 | **NOT FIXED** | global skill selection remains O(S); changing selection/cache invalidation without runtime skill-reset traces is unsafe. |
| MB-L-04 | **FIXED** | unreachable legacy pickup body was deleted; active bounded implementation is the sole path. |

## 4. Native spawn safety

`On3305` retains the existing successful order: optional settings hydration first, stream cursor reset, then the original native handler. Every new failure exit explicitly dispatches the native handler once. Optional media/path/config failure therefore disables/skips Macro setup rather than blocking character initialization. The native function address and calling convention are unchanged.

## 5. UI and lifecycle safety

Feature readiness is checked at the native timer boundary. Auto Potion validates its required slots, Auto Skill validates configured slot families, Auto Hunt validates controls used by its tick, and Pick Filter validates its filter controls. Missing media fails idle. Existing title/teleport/disconnect suspension remains the lifecycle owner; no background access was introduced.

## 6. Settings and parser safety

The party parser cannot exceed `memberName[256]`. Hunt settings accept only documented enum/hour values and a bounded radius. Party and buff maps are cleared per character and built into temporary maps before `swap`. Character filename components replace Windows-invalid characters and are capped at 64 wide characters.

Principal Hunt, Skill Buff, and Pick Filter saves write a process-specific temporary file, flush, `_commit`, close, and atomically replace the destination with write-through. Failed commits delete the temporary file and leave the last valid destination intact.

## 7. Automation rate and performance

Death recovery is limited from two attempts/second to one attempt per five seconds while the client remains dead. Pickup still sends at most one request per 500 ms, but inventory mergeability is computed once and reused for all ground candidates. Valid selection order and packet fields are unchanged.

## 8. Compatibility impact

- vSRO 188 opcodes and payloads: unchanged.
- Fixed native addresses/calling conventions: unchanged.
- Macro UI resources and visible behavior: unchanged when complete.
- Threading: native UI thread only; no new worker.
- mBot: no hooks, ownership, chaining, or coexistence policy changed.
- Existing valid filenames remain identical; only invalid Windows filename characters are normalized.

## 9. Automated checks

`validate_macro_stability_repair.py` checks parser width, native dispatch coverage, map transactions, value clamping, readiness gating, death backoff, O(E+I) pickup structure, dormant-code removal, atomic replacement, and sanitization. The earlier Client DLL stability source gate is also retained.

CMake Release configuration completed and verified the supported safe options and pinned Desktop HUD hash. A Linux compilation was attempted and stopped immediately because the Windows-only project includes `Windows.h`; therefore no Win32 DLL was produced and the code-fix statuses remain runtime-verification-required.

## 10. Required Windows staging

1. Build Win32 Release with the supported MSVC toolchain and deploy only to the prescribed staging output.
2. Capture `0x3305` on normal spawn and every missing UI/path/config failure; assert one native dispatch.
3. Load 255/256/511-character party lines with page heap/Application Verifier.
4. Exercise title/select/spawn, 100 teleports, death/revive with 30 seconds of lag, reconnect, and A→B→A character switching.
5. Remove each Macro resinfo child and confirm disabled/idle behavior without AV.
6. Benchmark pickup at 100/1,000/10,000 entities with full inventory.
7. Kill the process during each settings save and verify either old or complete new file, never a partial destination.
8. Test the exact supported mBot build with overlapping automation, without changing either ownership model.

## 11. Remaining Unknowns

- Exact Win32 compiler acceptance and x86 runtime behavior require the unavailable Windows toolchain.
- Account/server identity is not exposed at the persistence boundary; identical character names using one client directory can still share settings.
- Legacy scalar settings hydration is not fully model-transactional.
- Skill-table caching remains deliberately unchanged.
- Runtime behavior with the exact supported mBot binary remains Unknown until staging.

## MACRO BOT FINAL STATUS

- Critical remaining: 0 code paths identified; runtime verification required.
- High remaining: 0 code paths identified; runtime verification required.
- Medium remaining: 2 partial/Unknown items (settings namespace and scalar transactionality).
- Low remaining: 2 maintenance items (potion duplication and skill-scan cost).
- Unknown remaining: Windows build/runtime and exact mBot coexistence.

**READY FOR WINDOWS BUILD AND STAGING — NOT PRODUCTION VERIFIED**
