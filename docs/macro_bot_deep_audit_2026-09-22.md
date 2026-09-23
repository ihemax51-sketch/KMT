# KMTGuard vSRO 188 — Macro Bot deep audit

**Audit baseline:** current `JTClientLibrary/` at commit `9a1eaab`
**Audit performed:** 2026-09-23
**Scope:** source audit only; no runtime source, packet, opcode, media, or behavior was changed.

## 1. Executive summary

This is an independent audit of the current Macro Bot implementation, including its callers and native dependencies outside `src/Macro`. The review followed registration from `DllMain`, native `CGInterface::OnTimerIMPL`, character-spawn packet `0x3305`, UI state, inventory/entity helpers, outbound messages, settings files, and transition resets.

The Macro runs on the native client/UI timer thread; no Macro worker thread was found. Recent defensive work provides a meaningful baseline: world-readiness gates, broad timer cancellation during transitions, live inventory-slot re-resolution for Auto Scroll, target re-resolution by unique ID, one-pick-per-tick throttling, action/equipment throttles, and skill-learning request serialization. Those are valid vSRO patterns.

The baseline nevertheless **requires repair**. One reachable settings-parser stack overwrite is Critical. Two High paths can break spawn processing or dereference an incomplete custom UI. Six Medium defects cover unbounded persisted timer values, cross-character state retention, repeated dead-state commands, non-atomic/colliding settings, and crowded-scene cost. Four Low issues are hardening or dormant-code risks.

| Severity | Active | Dormant | Unknown | Total |
|---|---:|---:|---:|---:|
| Critical | 1 | 0 | 0 | **1** |
| High | 2 | 0 | 0 | **2** |
| Medium | 6 | 0 | 0 | **6** |
| Low | 3 | 1 | 0 | **4** |

No finding is based merely on fixed addresses, native `thiscall`, Silkroad-owned pointers, or native UI timers.

## 2. Files inspected

Repository-wide discovery used behavioral names and references, not only paths. **53 source/header units (35,300 physical lines)** were inspected deeply or as traced dependencies; the entire `JTClientLibrary` tree was searched for Macro/automation symbols, timer IDs, `SendMsg`, native player/interface/entity/inventory access, persistence calls, and registrations.

Primary units: all 32 `.cpp/.h` files under `source/libs/ClientLib/src/Macro/`; `AutoPotion.cpp/.h`; `SkillAutomationController.cpp/.h`; `GInterface.cpp/.h`; `NetProcessIn.cpp/.h`; `PSCharacterSelect.cpp`; `PSTitle.cpp`; `DllMain.cpp`; `Util.cpp`; `ClientNet` message buffers; player, object, item, inventory, equipment, pet, party, skill-data, global-data, and UI resource wrappers reached by those units. Media resinfo for the Macro windows was cross-checked by ID/name searches.

## 3. Macro architecture map

```text
DllMain class registration + client resinfo
        ↓
CIFMacro / CIFMacroMenu tabs and slot controls
        ↓
feature flags, maps, selected IDs, sliders, per-character files
        ↓
CGInterface::OnTimerIMPL (native client/UI thread)
        ↓
world-ready gate → feature decision logic
        ↓
current g_pMyPlayerObj / CGInterface / inventory / equipment /
entity manager / global item-skill data (resolved each tick where practical)
        ↓
CIFSlotWithHelp::UseItem or SendMsg(CMsgStreamBuffer)
```

Registration does not start automation. UI commands set `Macro_AutoPotion`, `Macro_AutoHunt`, `Macro_AutoSkill`, `Macro_PetFilter`, or `Macro_AutoScroll`; native timers then perform work. `0x3305` performs first-spawn settings hydration. Character-select/title paths reset/suspend automation.

## 4. Complete feature inventory

| Feature | Entry/update | State/UI owner | Native dependencies | Stop/reset |
|---|---|---|---|---|
| Auto Potion | `StartAutomation`, check/use pairs | `CIFMacroMenuAutoPotion` | player HP/MP/status, inventory, item reuse manager, pets | `StopAutomation`; world-transition suspension |
| Auto Skill/Buff | `StartAutoSkill`, 250 ms | `CIFMacroMenuAutoSkill` | skill slots/data/cooldowns, equipment, selected entity, party | target reset; timer suspension |
| Auto Hunt | `StartAutoHunt`, 500 ms | `CIFMacroMenuAutoHunt` | player/region, inventory/equipment, entity manager, navmesh | flag-off kills hunt/town/invite timers; transition suspension |
| Auto attack/target | called by Auto Skill | selected unique ID plus transient `SelectObj` | current entity manager, native action path | `ResetSelectedTarget` |
| Pick Filter | `PickWithPet`, 500 ms | `CIFMacroMenuPickFilter` | ground entities, item data, inventory, grab pet | flag-off kills timer; transition suspension |
| Auto Scroll | `AutoScrolling`, 500 ms | `CIFMacroMenuAutoScrollSlot` | current inventory slot/item and player buffs | flag-off kills timer; transition suspension |
| Pet support | potion/pill/HGP/summon/res timers | Auto Potion | `CCOSDataMgr`, current pet object, item data | Auto Potion stop/reset |
| Skill/mastery automation | `SkillAutomationController::Tick`, 130 ms | controller/global settings | player skill/mastery maps and custom status | pending timeout, suspend, character reset |
| Party invite/reform/buff | hunt/skill callbacks | Hunt/Skill tabs | party object/entity list | timers and target state reset |

## 5. Timer/execution model

All listed callbacks are dispatched by `CGInterface::OnTimerIMPL` on the native game/UI thread. No Windows timer, D3D callback, or Macro-created background thread was found. Native event delivery is serialized; overlap is logical (multiple IDs due in one UI pass), not concurrent reentrancy.

| Timer family | IDs / nominal period | Work and stop condition |
|---|---|---|
| Potion bootstrap | `START_AUTO_POTION`, 500 ms | waits for world readiness, then starts feature timers; disabled stops all |
| Character HP/MP/vigor | 8 check/use IDs, 500 ms | threshold/cooldown checks and live item use; dead/not-ready/disabled stops |
| Pills | poison and purification check IDs, 400 ms; use IDs 500 ms | abnormal-state check/use; disabled or invalid state stops |
| Speed | `SPEED_TIMER`, 15 s | reapplies speed item when configured |
| Pet HP/HGP/pill | 6 IDs, 0.4–5 s | current COS and inventory checks |
| Pet summon/resurrection | 12 s / 7.5 s | current pet state and item use |
| Auto Skill | `START_AUTO_SKILL`, 250 ms | buff/target/hunt decision; readiness resets target |
| Auto Hunt | `START_AUTO_HUNT`, 500 ms | death, supplies, equipment, town/party behavior |
| Town / invite | configured hours / 5 s | return town and a single invite scan |
| Pick | `START_PICK_PET_TIMER`, 500 ms | entity scan; at most one request/tick |
| Auto Scroll | `START_AUTO_SCROLL_TIMER`, 500 ms | up to 8 live-slot checks; one use/tick |
| Skill learning | `SKILL_AUTOMATION_TIMER`, 130 ms | one outstanding mastery/skill request, 3 s timeout |

**Timers found:** 27 Macro-specific timer IDs (excluding unrelated animation queue).

## 6. Player lifecycle matrix

| Phase | Observed behavior | Verdict |
|---|---|---|
| Title / character select | world gate is false; title/selection code suspends timers and resets skill automation | safe baseline, subject to incomplete UI finding MB-H-02 |
| Character enter / spawn | `On3305` loads files, then sets `FirstSpawn`; native handler should run last | unsafe early-return path MB-H-01 |
| Normal gameplay | native timer thread; current objects generally re-resolved | acceptable with findings below |
| Teleport | suspension kills Macro timer range and clears target/start position | good defensive design |
| Death | attack/pick paths wait; hunt may repeatedly request return/res item | MB-M-03 |
| Resurrection | readiness and current status resume timers | acceptable |
| Disconnect/reconnect | world gate prevents sends and target is reset | acceptable; file state issue MB-M-02 remains |
| Character change | controller reset and first-spawn reload occur | maps are not uniformly cleared: MB-M-02 |
| Exit | process-lifetime DLL/native timers end with process; hot unload unsupported | accepted Client DLL contract |

## 7. Native pointer ownership map

| Pointer/data | Owner | Storage pattern | Audit result |
|---|---|---|---|
| `g_pMyPlayerObj` | Silkroad world | global, checked at tick boundaries | not retained as Macro ownership |
| `g_pCGInterface` | Silkroad UI | global, timer dispatcher owner | world gate is good; nested chains remain in MB-H-02 |
| `SelectObj` | Silkroad entity manager | transient member pointer plus selected unique ID | `ResolveSelectedTarget` refreshes before use in principal attack path |
| inventory/equipment slots | native UI | normally fetched per tick | Auto Scroll additionally validates current slot/ref ID |
| skill/item metadata | global media manager | process-lifetime records | accepted native ownership |
| entity map values | native entity manager | scanned only on game/UI thread | no cross-thread iterator race found |
| COS/pet objects | world manager | re-resolved by unique ID | guarded in active pickup/pet paths |
| custom Macro windows | UI resource manager | looked up from external callbacks | incomplete child-resource handling: MB-H-02 |

## 8. Auto Potion audit

Threshold percentage uses a guarded max-value calculation. HP, MP, vigor, pill, speed, pet HP/HGP/pill, summon, and resurrection have distinct native timers. Current player/death checks and item reuse delay prevent the simplest tight loop. `StopAutomation` kills the whole timer family and clears running flags.

Risks are not packet-layout defects: action functions ultimately use native slot actions. The realistic failure is incomplete Macro resinfo: several paths dereference slot/control chains before their later null checks (MB-H-02). The repeated implementations also make lifecycle parity difficult (MB-L-02), but no unsupported direct opcode was attributed to potion consumption.

## 9. Auto Skill audit

The active logic throttles equipment, target, action, and zerk operations; skill entries are validated against current global data and native cooldown state. Targets are selected by unique ID (`0x7045`) and re-resolved. It does not intentionally keep a monster pointer across callbacks. Weapon/ammunition switching validates a compatible live inventory slot.

The hunt-to-skill bridge repeatedly obtains the Macro root and assumes all children exist (MB-H-02). Entity selection is O(E); skill-slot selection is bounded (24 slots). The separate skill/mastery learner scans the global skill map (MB-L-03).

## 10. Auto Hunt audit

The implemented state flow is:

```text
Disabled → Idle/WaitingForWorld → Searching → TargetSelected
→ EquipmentReady → Select/Attack → repeat
                     ↘ Dead/LowSupplies/Durability → Recover/ReturnTown
                     ↘ Party invite/reform/buff side paths
```

There is no independent movement worker; movement/attack is mediated by native action/skill paths and navmesh/region state. `StartRegion` and `StartPosition` reset when the world is not ready. Target status, ID, distance, hunt center, and intersection are rechecked. Repair hammer has a 5 s throttle.

The configured return interval is not range-checked (MB-M-01). The death branch can issue return/resurrection repeatedly at 500 ms (MB-M-03). Party/config maps survive character changes (MB-M-02).

## 11. Pick Filter audit

Active code iterates ground entities on the native UI thread, requires a `CIItem`, common data, positive unique ID, configured category, ownership/radius eligibility, and inventory capacity. It sends at most one request per 500 ms callback. A per-item 750 ms map blocks an immediate duplicate. Pet pickup resolves an active grab pet; character pickup excludes dead/stall/cast/riding states.

Worst case is O(E×I): each candidate can scan inventory for stackability/capacity, producing a credible crowded-event frame spike (MB-M-04). A large legacy implementation after an unconditional `return` is dormant (MB-L-04).

## 12. Auto Scroll audit

Eight configured slots are examined per 500 ms callback. Before use, code validates index bounds, re-fetches the live inventory slot, and compares `RefObjectId`; it therefore does not blindly use the drag/drop pointer after inventory movement. A 1.5 s per-slot throttle and one-use-per-tick break limit packet pressure. Resurrection and zerk have special conditions; ordinary scrolls check active/overlapping effects. This is a comparatively safe baseline, except for common custom-resource assumptions.

## 13. Packet opcode inventory

Direction is Client→Server in all rows. Layouts below are descriptive; the audit recommends no opcode or layout change.

| Feature | Opcode | Payload / purpose | Preconditions / maximum source rate | Native/custom |
|---|---:|---|---|---|
| equip ammo/item | `0x7034` | mode, inventory slot+13, equipment slot, zero | compatible live slot; equip throttle | native |
| select target | `0x7045` | target unique ID | current entity, target throttle | native |
| action/attack/pick | `0x7074` | action bytes + object ID | live target/item; action or 500 ms pick throttle | native |
| berserk | `0x70A7` | enable byte | native mini-info says ready; zerk throttle | native |
| pet pickup | `0x70C5` | pet ID, action 8, item ID | current grab pet/current item; ≤2/s | native |
| resurrect/return | `0x3053` | mode byte | dead/hunt branch; currently up to 2/s | native |
| party invite | `0x7060` | target ID | eligible current character; one per invite callback | native |
| party reform/match | `0x7069` | match action/purpose/level/title | valid hunt UI settings | native |
| repair hammer | `0x704C` | inventory slot, item type | current live item; ≤0.2/s | native |
| save slot | `0x7158` | Macro slot serialization | UI edit | existing custom extension |
| Macro status | `0x187E` | enabled/status state | UI toggle | custom |
| potion settings | `0x188A`, `0x169A` | slot/active/value records | UI save/change | custom |
| skill settings | `0x188C` | configured skill state | UI save | custom |
| pet support | `0x189B` | pet/action/item fields | current COS/pet checks | custom |
| learn mastery | `0x70A2` | mastery ID + next level | one pending request; timeout ≤0.34/s | native |
| learn skill | `0x70A1` | skill ID | one pending request; timeout ≤0.34/s | native |

**Macro packet opcodes found:** 16 distinct outbound opcodes in active Macro/controller paths. `0x3305` is an inbound native spawn/state packet used to trigger settings hydration; party responses `0x306E/0x3080` are consumed by related state.

## 14. Packet-rate analysis

| Path | Structural maximum | Failure/lag behavior |
|---|---:|---|
| pick | 2 requests/s total | per-item throttle plus one-per-tick |
| repair | 0.2/s | explicit 5 s guard |
| target/action/equip/zerk | limited by action-specific tick guards and 250 ms timer | waits during native cast/equip |
| skill/mastery learner | 7.7/s only if each request is acknowledged immediately; ~0.33/s on no response | one pending request, 3 s timeout |
| Auto Scroll | ≤2 native uses/s total, each slot ≤0.67/s | current item is revalidated |
| death return/res scroll | **up to 2 attempts/s** | repeats until state changes; MB-M-03 |
| potion families | multiple 500 ms checks, native cooldown queried | simultaneous families possible, but native use path/cooldown mediates |

No unthrottled background send loop was found. Sends are suppressed when the common world gate detects disconnect/teleport/not-spawned state.

## 15. UI/resource audit

Macro classes and tabs are registered at startup and instantiated from custom resinfo. Own-`OnCreate` mandatory controls are a valid contract only after successful creation. External timer and spawn callbacks do not consistently establish that complete contract: they check the root or a few tab pointers, then directly chain through checkboxes, slots, sliders, party rows, cooldown manager, or popup children. This is MB-H-02. Hotkeys should no-op when the Macro root is absent; no Macro timer should start until a complete-window readiness predicate passes.

Duplicate resource-ID searches did not establish an active collision. UI work remains on the native UI thread.

## 16. Settings/persistence audit

Files are rooted below `Setting` and include `Client_Extra.txt`, `<character>_Macro.txt`, `<character>_MacroAutoBuffSettings.txt`, `<character>_PickupFilter.txt`, and related auto-skill/hunt files. Recent bounded formatting rejects overly long paths rather than overflowing fixed buffers.

Remaining defects: unsafe wide-token scanning (MB-C-01), early-return coupling to native spawn (MB-H-01), map state not cleared transactionally (MB-M-02), unbounded numeric values (MB-M-01), and direct/non-atomic character-name-only files (MB-M-05). Normal vSRO character-name validation limits traversal, so path traversal is not escalated without proof of a server permitting separator characters.

## 17. Performance/FPS analysis

| Hot path | Complexity | Realistic impact |
|---|---|---|
| Auto Potion | O(I) in several 0.4–5 s callbacks | modest normally; repeated inventory passes can add frame time |
| Auto Skill target search | O(E), some map copies | crowded events can spike but 250 ms cadence limits frequency |
| Pick Filter | O(E×I) worst case | Medium FPS risk in dense drop scenes (MB-M-04) |
| Auto Hunt supplies | O(I) twice per 500 ms plus equipment O(1) | avoidable repeated work, generally bounded |
| skill learner | O(S) per decision at 130 ms when enabled | Low/Medium transient cost (MB-L-03) |
| settings load/save | O(file), spawn/UI only | not a steady-frame cost |

No SQL, external network call, filesystem write, or log write was found in the per-frame D3D path for Macro. Timers execute on the UI thread, so expensive scans manifest as FPS hitching rather than data races.

## 18. mBot compatibility analysis

| Area | Classification | Evidence |
|---|---|---|
| Hook ownership | **UNKNOWN** | Macro uses existing native timer/send facilities; this source cannot prove mBot hook addresses/ownership |
| Child-list/UI compatibility | **SAFE in source** | no mBot-specific child-list rewrite in Macro paths found |
| Potion/skill/target/pick | **POTENTIAL CONFLICT** | KMT can emit the same native gameplay actions while any external bot may also automate them; no mutual exclusion/owner arbitration exists |
| Packet layouts | **SAFE in source** | native vSRO field layouts are retained; no mBot chaining is invented |
| Proven crash/DC conflict | **UNKNOWN** | external mBot version/source and runtime trace are absent; no honest proof of collision is available |

Staging must test with the exact supported mBot build. Do not label coexistence proven from this repository alone.

## 19. Critical findings

### MB-C-01 — Unbounded `%ls` writes a stack buffer while loading party members

- **Severity / reachability:** CRITICAL / ACTIVE
- **Feature:** Auto Hunt party persistence
- **File / function / exact lines:** `source/libs/ClientLib/src/NetProcessIn.cpp`, `CNetProcessIn::On3305`, lines 6195–6201
- **Current behavior:** `swscanf(..., L"... %ls", ..., memberName)` has no field width for `wchar_t memberName[256]`; the input line can contain up to 511 characters.
- **Exact execution path:** server enables Macro → first character spawn receives `0x3305` → per-character Macro file opens → crafted/corrupt `Auto party member` line is parsed.
- **Real vSRO scenario:** a manually edited, migrated, partially corrupted, or shared settings file contains a party name longer than 255 wide characters.
- **Expected symptom:** stack overwrite, access violation, corrupted locals, or control-flow corruption during character spawn.
- **Minimal recommended fix:** use `%255ls` and validate member index/name length; preferably parse the delimiter into a bounded `std::n_wstring` and commit only after complete-file validation.
- **Regression risk:** low; valid vSRO names remain unchanged.
- **Required test:** 255/256/511-character names, missing delimiter, empty name, invalid index, and valid seven-member file under ASan-equivalent/Windows page heap.

## 20. High findings

### MB-H-01 — Optional Macro settings failures bypass the native `0x3305` handler

- **Severity / reachability:** HIGH / ACTIVE
- **Feature:** spawn/lifecycle/settings
- **File / function / exact lines:** `source/libs/ClientLib/src/NetProcessIn.cpp`, `On3305`, early returns at 6008–6013, 6052–6063, 6231–6233, 6257–6260; native dispatch only at 6404–6407.
- **Current behavior:** missing custom UI or rejected long settings path returns from the entire packet handler instead of skipping optional hydration; the required native handler is not called.
- **Exact execution path:** `0x3305` → `EnableMacro` and first spawn → UI/resource/path prerequisite absent → `return` → no native `0x0087FDD0` call.
- **Real vSRO scenario:** mismatched/missing Macro resinfo, delayed interface construction, or a long installation/character path.
- **Expected symptom:** incomplete character spawn/native state, missing UI, stuck load, or follow-on AVs from partially initialized state.
- **Minimal recommended fix:** isolate optional load into a helper returning status; always reset the stream and invoke the native handler exactly once. Failure should disable Macro for that character, not consume native processing.
- **Regression risk:** medium because native call order is ABI-sensitive; preserve current successful ordering and stream offset.
- **Required test:** every prerequisite missing individually, long path, unreadable directory, malformed files, plus exact valid spawn; assert native handler count is one.

### MB-H-02 — Timer/spawn code assumes a partially loaded Macro UI is complete

- **Severity / reachability:** HIGH / ACTIVE
- **Feature:** all Macro tabs
- **File / function / exact lines:** `GInterface.cpp` 225–455; `NetProcessIn.cpp` 6052–6250; `Macro/IFMacroMenuAutoPotion.cpp` 997–1025; `Macro/IFMacroMenuAutoSkill.cpp` 1478–1528 and 1654–1669; `Macro/IFMacroMenuAutoHunt.cpp` 1026–1133.
- **Current behavior:** root/tab checks are followed by unchecked nested controls; Auto Potion tests a slot chain before its later null check; Auto Skill/Hunt chain through fresh `GetResObj` results and checkboxes.
- **Exact execution path:** custom window is registered but resinfo/child creation is incomplete → Macro enabled or timer retained → native UI timer/spawn hydration enters feature → missing child is dereferenced.
- **Real vSRO scenario:** customer media differs, one optional resource ID is missing, or interface recreation fails after character select/reconnect.
- **Expected symptom:** repeatable client AV on spawn, Macro enable, or first timer tick.
- **Minimal recommended fix:** establish one `IsMacroUiReady()` boundary that validates required tabs/children before publishing/enabling Macro; external callbacks fail idle and never chain through `GetResObj`. Keep mandatory own-`OnCreate` contracts unchanged after successful creation.
- **Regression risk:** medium; an over-strict predicate could disable a valid subset, so define per-feature readiness.
- **Required test:** remove each tab/required child in turn, fail window creation, invoke hotkey before/after create, then teleport/reconnect/recreate UI with timers active.

## 21. Medium findings

### MB-M-01 — Persisted numeric values can overflow native timer intervals
- **Severity / reachability:** MEDIUM / ACTIVE
- **Feature:** Auto Hunt settings
- **File / function / exact lines:** `NetProcessIn.cpp` 6089–6112; `Macro/IFMacroMenuAutoHunt.cpp` 1048–1053 and 1088–1092.
- **Current behavior:** arbitrary signed file values are accepted; `BACK_HOUR_SETTING * 3600000` uses integer arithmetic without range validation.
- **Execution/scenario/symptom:** malformed old/manual config → spawn load → hunt start → negative/overflowed interval; timer may fire immediately/repeatedly or never, causing unexpected return-town behavior or UI load.
- **Minimal fix / regression risk / test:** clamp and schema-validate hours, radius, enum values before commit; low risk. Test INT_MIN/INT_MAX, -1, 0, documented maxima, and overflow boundary.

### MB-M-02 — Character-scoped maps are merged rather than transactionally replaced
- **Severity / reachability:** MEDIUM / ACTIVE
- **Feature:** Auto Hunt party and Auto Skill buff persistence
- **File / function / exact lines:** `NetProcessIn.cpp` 6195–6250; `PSTitle.cpp` character reset path; Macro constructors initialize maps only once.
- **Current behavior:** `insert` loads party/buff entries without a guaranteed clear on each character transition; duplicate keys retain earlier values.
- **Execution/scenario/symptom:** character A → title/select → character B → first spawn load; A entries can remain or B duplicates can fail to replace, producing wrong buffs/invites.
- **Minimal fix / regression risk / test:** parse into temporary maps, validate, then swap after identifying current character; medium risk to legacy merge semantics. Test A→B→A, missing/partial file, duplicates, reconnect.

### MB-M-03 — Death recovery can attempt the same action every hunt tick
- **Severity / reachability:** MEDIUM / ACTIVE
- **Feature:** Auto Hunt recovery
- **File / function / exact lines:** `Macro/IFMacroMenuAutoHunt.cpp` 1093–1115; hunt timer at 1042–1046.
- **Current behavior:** while dead, resurrection-scroll `UseItem()` or `0x3053` is attempted every 500 ms with no local pending/ack timeout.
- **Execution/scenario/symptom:** death + lag/rejection/no state transition → 2 attempts/s until status changes; duplicate consumption attempts, server rejection, or DC risk.
- **Minimal fix / regression risk / test:** one pending death action with response/state-change or conservative timeout; medium risk to recovery responsiveness. Test lag, rejection, missing scroll, 30-second dead state, revive mid-timeout.

### MB-M-04 — Pick Filter worst case is O(E×I) on the UI thread
- **Severity / reachability:** MEDIUM / ACTIVE
- **Feature:** Pick Filter
- **File / function / exact lines:** `Macro/IFMacroMenuPickFilter.cpp` 464–534 and 620–640.
- **Current behavior:** each candidate ground item can trigger an inventory capacity/stack scan during a full entity scan every 500 ms.
- **Execution/scenario/symptom:** crowded event with many drops and a large/full inventory → UI-thread nested scans → periodic FPS hitch/freeze.
- **Minimal fix / regression risk / test:** compute inventory capacity/stack index once per tick and filter nearest candidates before expensive checks; medium risk to selection order. Benchmark E={100,1k,10k}, full inventory, pet/character modes.

### MB-M-05 — Settings writes are non-atomic and namespace only by character text
- **Severity / reachability:** MEDIUM / ACTIVE
- **Feature:** persistence
- **File / function / exact lines:** Macro Auto Hunt/Skill/Pick save functions and `NetProcessIn.cpp` 6061–6069, 6231–6264.
- **Current behavior:** direct files can be observed partially written after crash; same character text in different client/server contexts maps to the same working-directory filename.
- **Execution/scenario/symptom:** interrupted save or two client instances sharing a directory → partial/cross-context settings and unexpected automation.
- **Minimal fix / regression risk / test:** stable sanitized account/server+character key, temp-file flush/close then atomic replace; medium migration risk. Test crash during write, two processes, same character name on two shards, unwritable directory.

### MB-M-06 — Spawn hydration mutates live UI incrementally before validation completes
- **Severity / reachability:** MEDIUM / ACTIVE
- **Feature:** all persisted Macro state
- **File / function / exact lines:** `NetProcessIn.cpp` 6002–6401.
- **Current behavior:** settings are applied line-by-line; a later malformed field/path/UI failure leaves a partial mixture of defaults, old state, and new state.
- **Execution/scenario/symptom:** truncated or version-mixed files during first spawn → some flags/slots updated before abort → stuck or inconsistent automation.
- **Minimal fix / regression risk / test:** parse/version/validate into POD models, then commit atomically after UI readiness; medium risk due to legacy permissive parsing. Fuzz every truncation point and verify all-or-default behavior.

## 22. Low findings

### MB-L-01 — Character-derived filenames rely on external name validation
- **Severity / reachability:** LOW / ACTIVE
- **Feature:** persistence
- **File / function / exact lines:** `NetProcessIn.cpp` 6061–6063, 6231–6260 and corresponding Macro save functions.
- **Current behavior/path/scenario:** bounded formatting prevents overflow, but no local canonicalization rejects separators/device names. Standard vSRO names are safe; a custom server with relaxed rules could redirect/collide paths.
- **Symptom / minimal fix / risk / test:** wrong file/read failure; encode character identity rather than embedding raw text. Migration risk medium. Test reserved characters and Windows device names.

### MB-L-02 — Auto Potion duplicates lifecycle-sensitive logic across many check/use pairs
- **Severity / reachability:** LOW / ACTIVE
- **Feature:** Auto Potion
- **File / function / exact lines:** `Macro/IFMacroMenuAutoPotion.cpp` 917–3528.
- **Current behavior/path/scenario:** near-duplicate branches have inconsistent ordering (including slot-chain checks) and make fixes easy to miss; reachable every timer tick, but duplication alone is not a crash claim.
- **Symptom / minimal fix / risk / test:** regression-prone behavior; centralize current-slot resolution and stop/schedule policy without changing timing. High regression risk; golden tests for every potion/pet family.

### MB-L-03 — Skill-learning decision repeatedly scans the global skill table
- **Severity / reachability:** LOW / ACTIVE
- **Feature:** skill/mastery automation
- **File / function / exact lines:** `SkillAutomationController.cpp` 95–183 and 428–510.
- **Current behavior/path/scenario:** O(S) selection can repeat at 130 ms while enabled. Large custom media skill tables can cause transient UI-thread cost.
- **Symptom / minimal fix / risk / test:** frame-time spikes while auto-learning; index candidates per mastery and invalidate on skill-state change. Medium risk. Benchmark supported and enlarged media.

### MB-L-04 — Thousands of lines of unsafe legacy pickup logic remain after an unconditional return
- **Severity / reachability:** LOW / DORMANT
- **Feature:** Pick Filter
- **File / function / exact lines:** `Macro/IFMacroMenuPickFilter.cpp` 642–2450 (approximately), after active return at 642.
- **Current behavior/path/scenario:** compiler-unreachable old code contains raw offsets, unchecked chains, and many duplicate sends. It cannot execute now, but moving/removing the return during maintenance would reactivate it.
- **Symptom / minimal fix / risk / test:** latent crash/packet-burst regression; delete it after source-history confirmation or mechanically exclude it. Low runtime risk; source gate should assert only the active bounded path exists.

## 23. Accepted vSRO-specific patterns / existing defensive code

- Native `CGInterface` timers are the correct thread-affinity mechanism; no Macro worker touches Silkroad objects.
- Fixed opcodes, native object offsets/wrappers, `thiscall`, and Silkroad-owned metadata are accepted for the supported vSRO 188 fingerprint.
- World readiness validates spawn, player status/HP/MP, interface, popup, inventory, and equipment before dispatch.
- Transition suspension kills Macro timers, clears flags/selected target/pick throttles, and resets start region.
- Auto Scroll re-resolves live slots and compares item identity before use.
- Pick sends one request per tick and has a per-item duplicate guard.
- Auto Skill re-resolves selected objects, checks native cooldowns, and throttles target/equip/action/zerk paths.
- Skill learning permits one pending request and backs off for three seconds without acknowledgement.
- Bounded `KmtFormatPath` is an improvement over `sprintf`; rejected paths must simply not bypass native packet handling.

## 24. Dormant dangerous code

The post-`return` Pick Filter implementation is the primary dormant hazard (MB-L-04). Searches also found old/commented automation branches and raw-offset alternatives; none was promoted to an active crash finding. Build/source gates should prevent removal of the active return or compilation of old branches without a new audit. Dormant GlobalItemLinking/ImGui/QuickStart belong to the broader Client audit and are not Macro features.

## 25. Recommended fixing order

1. **MB-C-01:** bound/replace the party-member parser.
2. **MB-H-01:** guarantee exactly-once native `0x3305` dispatch independent of optional Macro hydration.
3. **MB-H-02:** introduce per-feature UI readiness and publish/start only complete windows.
4. **MB-M-06/M-02:** transactional character-scoped settings models and explicit reset/swap.
5. **MB-M-01/M-03:** validate timer settings and add death-action pending/backoff.
6. **MB-M-04:** cache inventory eligibility per pickup tick.
7. **MB-M-05/L-01:** atomic, namespaced, sanitized persistence with migration.
8. Remove dormant pickup code, consolidate potion resolution, then optimize the skill index.

Do not alter valid opcodes or native layouts. Repair should fail Macro to Disabled/Idle while always preserving native client processing.

## 26. Windows staging test plan

1. Build Win32 Release with the supported MSVC/toolset and exact vSRO 188 executable/media fingerprint.
2. Parser tests: valid files, every truncation, 255/256/511-wide names, duplicate keys, malformed integers, INT bounds, old/no files, unwritable `Setting`.
3. Resource tests: remove root, each tab, each required child; fail/recreate interface; hotkey before/after creation.
4. Lifecycle: title→select→spawn; 100 teleports; death/revive variants; disconnect/reconnect; A→B→A characters; exit while every feature is enabled.
5. Packet capture: verify the 16 layouts byte-for-byte; assert no send pre-spawn/during teleport; inject lag/rejection and measure maxima in §14.
6. Inventory: move/delete/replenish potion/scroll between check and action; storage transitions; full inventory; pet summon/despawn.
7. Combat: weapon/ammo switching, deleted/unlearned skill, mastery update, target despawn/death, out-of-region target, zerk unavailable.
8. Performance: frame-time traces for 100/1,000/10,000 entities and drops, maximum inventory/skill table, Macro off/on.
9. Compatibility: exact mBot build with each overlapping feature separately and together; capture hook ownership, sends, DCs, and child-list behavior. Report UNKNOWN until completed.
10. Run Application Verifier/page heap and collect minidumps for any AV. A build alone is not runtime proof.

---

## MACRO BOT AUDIT STATUS

**Files inspected:** 53 source/header units (35,300 physical lines), plus repository-wide symbol/resource discovery
**Features found:** 9 feature families
**Timers found:** 27 Macro-specific IDs
**Macro packet opcodes found:** 16 active outbound opcodes

**Critical:** 1
**High:** 2
**Medium:** 6
**Low:** 4

**Top crash risks:** unbounded party-name scan; incomplete Macro UI child chains; optional load bypassing native spawn.
**Top DC risks:** repeated death recovery during lag; simultaneous third-party automation; invalid persisted timer/state values.
**Top FPS risks:** O(E×I) pickup scan; repeated entity selection scans; repeated inventory and global-skill scans on the UI thread.

**Overall: REQUIRES REPAIR**
