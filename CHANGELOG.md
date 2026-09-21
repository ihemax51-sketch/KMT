# KMTGuard Update History

## Update v7.0.0

Release date: 2026-09-21

- Consolidated KMTGuard into one complete edition containing the Filter, Admin Desktop, Client DLL, media, GameServer add-on, ShardManager add-on, and all existing feature packages.
- Removed edition-specific activation, subscription, remote validation, machine/IP binding, player-cap enforcement, and separate package generation requirements.
- Added one full-build command plus a component build command, both publishing to the single `D:\KMTGuard-build` delivery tree with SHA-256 verification.
- Requirement: replace and restart the Filter, replace the Client DLL, GameServer add-on, and ShardManager add-on, apply the supplied media, and restart the affected game services. No SQL update is required for this release.

## Update v1.0.0

Release date: 2026-09-14

### Native and runtime stability repair - 2026-09-21 (developer build)

- Prevented ShardManager and GameServer database workers from releasing shared runtime state before their threads have stopped, including safe cancellation and timeout handling during SQL delays.
- Added deterministic native database timeouts, safer malformed command/string handling, and stricter event database identifier validation without changing valid packets or opcodes.
- Prevented overlapping Filter database-worker generations during restart and disabled an archived GameServer worker that could access live player objects from the wrong thread.
- Hardened Client DLL startup and DirectX9 device access so unsupported hook ownership or unavailable device state fails safely instead of allowing partial initialization.
- Requirement: replace and restart the Filter, replace the GameServer and ShardManager add-ons during full maintenance, replace the Client DLL, and reopen the client. No SQL update or media update is required.

### Full platform stability and security audit - 2026-09-21 (developer documentation)

- Completed a platform-wide vSRO 188 source audit covering the Filter, GameServer and ShardManager add-ons, Client DLL, packet paths, database operations, events, memory ownership, and worker lifecycles.
- Documented a prioritized, compatibility-focused stability roadmap without changing live server, client, packet, SQL, or event behavior.
- Added a production repair roadmap for every audit finding, including component-specific implementation steps, downtime impact, test procedures, deployment ordering, and regression controls.
- Requirement: no server restart, Filter restart, SQL update, Client DLL replacement, GameServer replacement, ShardManager replacement, media update, or additional customer action is required.

### Client animation crash correction - 2026-09-20 (developer build)

- Stabilized custom player, killer, and fellow-pet animations so animation requests are queued and resolved against the current live object before playback instead of calling native animation functions directly during packet processing.
- Prevented stale or not-yet-ready character animation objects from crashing the game client when animation packets arrive during spawn, despawn, or visibility transitions.
- Requirement: replace the Developer Client DLL and reopen the game client. No Filter restart, SQL update, GameServer replacement, ShardManager replacement, or media update is required.

### Stability hardening for disconnect, crash, and freeze scenarios - 2026-09-20 (developer build)

- Improved Filter resilience so unexpected packet-handler errors block the problematic packet instead of disconnecting the player session.
- Added stricter validation for character action, logout, mastery, teleport-position, and Unique kill logging packets to prevent malformed or incomplete packets from causing disconnects or unstable feature behavior.
- Hardened client automation and custom window handling so missing or not-yet-created interface objects are ignored safely instead of freezing or crashing the game client.
- Requirement: replace and restart the Filter, replace the Developer Client DLL, and reopen the game client. No SQL update, GameServer replacement, ShardManager replacement, or media update is required.

### Client unique target stability correction - 2026-09-20 (developer build)

- Stabilized the client-side Unique Target effect during Unique combat so the game client ignores momentarily unavailable visual objects instead of freezing while processing combat updates.
- Requirement: replace the Developer Client DLL and reopen the game client. No Filter restart, SQL update, GameServer replacement, ShardManager replacement, or media update is required.

- Prepared a clean official release build by removing temporary investigation diagnostics from the Client DLL and Filter outputs.
- Restored normal production startup behavior without developer console windows or temporary diagnostic files.
- Corrected Reverse and travel-scroll region checks so travel item restrictions are enforced against the actual destination instead of stale saved destinations or the player's current area.
- Saved Reverse locations now persist reliably across restarts and update an existing saved slot instead of reporting success after a failed database write.
- Auto Skill and Auto Mastery now continue operating after the Skills window closes and recover from delayed skill/mastery responses instead of stopping permanently.
- The Client DLL now uses independent version 1 branding without changing the Filter or server component versions.
- Requirement: replace the Client DLL and Filter binaries during the normal maintenance window. No SQL or media update is required.

### mBot packet compatibility correction - 2026-09-15 (existing version)

- Corrected intermittent mBot disconnects during character selection by preserving the native character-list response before optional client extension data.
- Chat restriction and prohibited-language responses now use the native game notice protocol, preventing mBot from closing when a sent message is rejected.
- External bot sessions now receive compatible native notices instead of client-extension notice packets.
- Requirement: replace and restart the Filter. No SQL update, Client DLL replacement, GameServer replacement, ShardManager replacement, or media update is required.

### Client texture quality adjustment - 2026-09-19 (developer build)

- Added a lightweight Client DLL texture, clear-color tuning, and safe anti-aliasing selection to improve in-game texture sampling and give characters and the game image a clearer, sharper, less washed-out look without changing media files or gameplay systems.
- Requirement: replace the Developer Client DLL and reopen the game client. No Filter restart, SQL update, GameServer replacement, ShardManager replacement, or media update is required.

### Client background sight setting - 2026-09-19 (developer build)

- Added live Settings window buttons for Normal, High, and Best Background Sight Range, giving players a client-side way to choose the draw-distance level without using Save or Cancel buttons.
- Requirement: replace the Developer Client DLL and client media resources, then reopen the game client. No Filter restart, SQL update, GameServer replacement, or ShardManager replacement is required.

### GameServer command coordinate correction - 2026-09-19 (developer build)

- Corrected GameServer command handling so spawn and movement commands accept valid signed region IDs while preserving the native region value used by the game.
- Requirement: replace the GameServer add-on and ShardManager add-on, then restart ShardManager and every GameServer. No Filter restart, Client DLL replacement, media update, or SQL update is required.

### Character selection correction - 2026-09-12 (existing version)

- Removed custom double-click character entry while preserving native mouse handling, normal character selection, the original Start button, click state, and selection animations.
- Added lightweight Filter diagnostics for the character-list and character-entry packet sequence without changing packet processing.
- Preserved the native enter-game acknowledgement order by delivering character bootstrap data only after the acknowledgement has reached the client.
- Requirement: replace and restart the Filter, replace the Client DLL, and reopen the game after building these source corrections. No SQL or media update is required.

## Update v6.3.10

Release date: 2026-09-10

- Added a 60-second Auto Equip cooldown per character to prevent repeated requests.
- Players now receive a clear in-game notice with the remaining cooldown time.
- Requirement: replace and restart the Filter. No SQL update, Client DLL replacement, GameServer replacement, ShardManager replacement, or media update is required.

### Client correction - 2026-09-11

- Unique History now starts with Show DPS disabled; players can enable it manually.
- Shortened the Magic POP animation while preserving automatic refill and stop-on-win behavior.
- Corrected Magic POP Auto Refill after a consumed card and Auto Play incorrectly stopping after a losing coupon. Auto Play stops when a winning coupon is received.
- Requirement: replace the Client DLL and reopen the game client. No Filter restart, SQL update, GameServer replacement, ShardManager replacement, or media update is required.

### mBot compatibility correction - 2026-09-11

- Stabilized mBot client startup and fast-login sequencing while preserving the existing client verification and bot-admission policies.
- Login and character selection now wait briefly for an already-issued client verification request instead of failing because the response and automated login cross in transit.
- Prevented an intermittent client crash during native visual-object cleanup when an external launcher leaves the object's already-cleared child list unavailable during its final redundant cleanup pass.
- Requirement: replace and restart the Filter, replace the Client DLL, and reopen the game client. No SQL update, GameServer replacement, ShardManager replacement, or media update is required.

## Update v6.3.9

Release date: 2026-09-09

- Added a GameServer Trade System switch that disables only the original trade-goods gold payout while preserving the normal NPC sale and every other gold source.
- Requirement: apply the v6.3.9 SQL update, replace Admin Desktop and the GameServer add-on, then restart every GameServer. No Filter, ShardManager, Client DLL, or media update is required.
- Replaced the Dynamic Ranking drop-down selector with database-driven Old School tabs matching the Item Chest style. Only active ranking categories are shown, and the first available category opens automatically.
- Ranking tabs support all configured categories in a compact two-row layout, with full category names available on hover when a label is shortened.
- Ranking tabs now work with the original interface media, hide the retired search controls, and become clickable again as soon as ranking results arrive.
- Ranking tabs now use the compact, fixed-width, left-aligned classic layout shown in the Dynamic Ranking reference, vertically centered above the table instead of stretching across the window.
- Requirement: keep or restore the original working client media, replace the Client DLL, and reopen the game client. No new media import, Filter restart, or SQL update is required.
- Corrected the guild Union limit so the configured GameServer value is enforced by ShardManager when guilds join a Union.
- Corrected guild-point overflow protection so its configured value is validated and applied reliably during ShardManager startup.
- Requirement: replace and restart the ShardManager add-on. No Filter, GameServer, Client DLL, media, or SQL update is required.
- Completed the Secondary Slot shortcut correction so Space with 1 through 0 is recognized reliably by the active game window and activates the matching slot on the selected page.
- Requirement: replace the Client DLL and reopen the game client. No Filter restart, SQL update, GameServer replacement, or media update is required.

## Update v6.3.8

Release date: 2026-09-07

- Fixed the Secondary Slot keyboard shortcuts so holding Space and pressing 1 through 0 activates the matching slot on the selected page.
- Requirement: replace the Client DLL and reopen the game client. No Filter restart, SQL update, GameServer replacement, or media update is required.

## Update v6.3.7

Release date: 2026-09-06

- Fixed Fellow Pet movement speed so it is applied immediately when the pet's movement properties are initialized, without requiring a monster kill. This correction requires replacing the GameServer add-on and restarting every GameServer; no SQL, Filter, Client DLL, or media update is required.
- Added gold, purple, cyan, and orange server notice styles that appear in both the center notification area and the chat box.
- Updated colored notice banner edges to follow the selected color while preserving transparency, and removed temporary notice diagnostics. This correction requires replacing only the Client DLL and reopening the game client; in-game visual verification is pending.
- Existing notice procedures remain compatible and can use the new styles through notice types 11, 12, 13, and 14.
- Requirement: replace the Filter and Client DLL, restart the Filter, and reopen the game client. No SQL or media update is required.

## Update v6.3.6

Release date: 2026-09-05

- Corrected European total mastery so it follows the configured European value instead of being limited to twice the character level.
- Corrected the GameServer's European mastery value handling and made the Client verify the supported layout before applying separate Chinese and European totals.
- Preserved the normal rule that an individual mastery branch cannot exceed the character level.
- Requirement: replace the Client DLL and GameServer add-on, restart every GameServer, restart the Filter, and reopen the game client. No SQL or media update is required.

## Update v6.3.5

Release date: 2026-09-04

- Corrected mastery limits above 255 so configured Chinese and European totals are fully applied in the game.
- Ensured the European client receives its configured mastery cap through both the standard and extended settings paths.
- Ensured the configured mastery totals remain applied when the gameplay skill interface is created or refreshed by the client loader.
- Requirement: replace the Client DLL, replace the Filter and GameServer add-on, then restart the Filter, every GameServer, and the game client. No SQL update is required.

## Update v6.3.4

Release date: 2026-09-02

- Refreshed the optional CASY menu with the supplied compact dark-and-gold theme, KMT header mark, and lightweight hover feedback.
- Requirement: replace the Developer Client DLL and CASY client media. No Filter, SQL, or server restart is required.

## Update v6.3.3

Release date: 2026-09-02

- Fixed the Developer Client DLL build so it starts with the intended development configuration.
- Requirement: replace the Developer Client DLL. No media, Filter, SQL, or server restart is required.

## Update v6.3.2

Release date: 2026-09-02

- Refreshed the optional CASY menu with a compact muted-wood layout, Egyptian KMT header engraving, and integrated visual labels for its menu actions.
- Requirement: replace the Client DLL and CASY client media. No Filter restart or SQL update is required.

## Update v6.3.1

Release date: 2026-09-01

- Redesigned the Item Chest with the classic Silkroad frame, dark inset panel, compact controls, and expanded table layout.
- Requirement: replace the three Item Chest client media files. No Client DLL, Filter, SQL, or server restart is required.

## Update v6.3.0

Release date: 2026-08-31

- Added a dedicated two-slot Event Register guide icon that opens event registration directly from the in-game icon grid.
- The existing Event registration setting now controls whether this guide icon is shown.
- Requirement: replace the Client DLL and client media. Reconnect after changing the Event registration setting. No Filter executable or SQL update is required.

## Update v6.2.14

Release date: 2026-08-31

- Fixed native Trace being rejected when its selected player's destination was temporarily unavailable to the Filter. Known destinations are still checked before movement, and the actual destination remains protected after arrival.
- Requirement: replace and restart the Filter. No SQL update, Client DLL replacement, GameServer replacement, or media update is required.

## Update v6.2.13

Release date: 2026-08-31

- Redesigned the CASY menu with six branded primary controls and dedicated side controls for Icon Manager, Changelog, and Settings.
- Refined the CASY menu to a more compact in-game size while keeping the branded controls readable.
- Corrected mastery totals so Chinese and European characters use the configured GameServer limits instead of being held to the original European cap.
- Requirement: replace the Filter, Client DLL, and GameServer add-on. After setting the desired Chinese and European mastery limits, restart the Filter and every GameServer. Existing CASY media remains valid. No SQL or ShardManager replacement is required.

## Update v6.2.7

Release date: 2026-08-31

- Added an optional CASY in-game menu, controlled from Admin Desktop with the Menu CASY setting.
- Requirement: apply the v6.2.7 SQL update, replace and restart the Filter, and replace the Client DLL, Admin Desktop, and media. No GameServer or ShardManager replacement is required.

## Update v6.2.6

Release date: 2026-08-30

- Fixed the NPC sold-item recovery list so items sold to a shop remain available for immediate repurchase while the pickup visual effect is enabled.
- Requirement: replace the Client DLL. No Filter restart, SQL update, GameServer replacement, or media update is required.

## Update v6.2.5

Release date: 2026-08-30

- Fixed Reverse travel for new characters so using a Reverse Scroll no longer requires a previous Return Scroll first.
- Saved Reverse destinations are checked when the scroll is used, while new characters with no saved destination can use the native game flow and remain subject to destination protection after arrival. The GameServer still checks the actual target against item-region restrictions before movement.
- Requirement: replace the Filter and GameServer add-on, then restart the Filter and every GameServer. No SQL update, Client DLL replacement, or media update is required.

## Update v6.2.4

Release date: 2026-08-29

- Active Fellow Buffs are now removed automatically when the summoned Fellow Pet leaves the world.
- Requirement: replace the Filter and restart it. No SQL update, Client DLL replacement, or media update is required.

## Update v6.2.3

Release date: 2026-08-29

- Fellow Buff actions now require the character's Fellow Pet to be actively summoned.
- Requirement: replace the Filter and restart it. No SQL update, Client DLL replacement, or media update is required.

## Update v6.2.2

Release date: 2026-08-29

- Removed the retired live-character runtime event bridge and its optional database callback.
- Requirement: apply the v6.2.2 SQL update, replace the Filter, then restart the Filter. No Client DLL or media update is required.

## Update v6.2.1

Release date: 2026-08-29

- Fixed ActionWnd custom commands so button clicks are accepted by the Filter and reach the configured command procedure.
- Requirement: replace the Filter, apply the v6.2.1 SQL update, then restart the Filter. No Client DLL or media update is required.

## Update v6.2.0

Release date: 2026-08-29

- Added customer-controlled GameServer options to show the normal Unique kill notice when a Game Master lands the killing blow and to force Game Master characters visible when they enter the world.
- Added both options to the Admin Desktop GameServer Settings page. Both remain disabled by default until enabled by the customer.
- Requirement: apply the v6.2.0 SQL update, replace the GameServer add-on and Admin Desktop, then restart every GameServer. No Filter, ShardManager, Client DLL, or media update is required.

## Update v6.1.0

Release date: 2026-08-29

- Added a unified ActionWnd command flow for custom client buttons, supporting all custom ActionWnd IDs from 9000 onward.
- Added the first custom ActionWnd command for returning the character to town.
- Requirement: replace the Client DLL, restart the Filter, and apply the v6.1.0 SQL update. No GameServer or media update is required.

## CASY Web Studio 0.6.0

Release date: 2026-08-29

- Replaced the multi-studio customer interface with one focused Player Control screen plus a fast Item Finder.
- Added simple CharID or character-name targeting for silk, level, gold, STR/INT, skill points, teleports, notices, disconnect, unique spawning, and Item Chest rewards.
- Added automatic self-teleport refresh after direct character level, STR, INT, and skill-point updates.
- Connected gold, silk, movement, notices, disconnect, spawning, and Item Chest delivery to the verified KMTGuard procedures already installed on the server.
- Added instant item and monster prefix lookup, DDJ item thumbnails when available, IDs, and CodeName copy/use controls.
- Added focused desktop, tablet, and mobile layouts and removed unrelated studios, tables, and navigation from the customer workflow.
- Requirement: refresh CASY with `Ctrl+F5`. No Filter restart, Client DLL replacement, media update, or KMTGuard SQL update is required.

## CASY Web Studio 0.5.0

Release date: 2026-08-28

- Rebuilt the CASY web interface around a comfortable task-first workspace with readable typography, restrained light surfaces, grouped navigation, and consistent controls across desktop, tablet, and mobile.
- Replaced the long sidebar and decorative top-bar controls with collapsible tool groups, real tool search, clear connection status, and a focused owner menu.
- Rebuilt Live Control as one guided three-step form covering silk, self teleport, position teleport, town return, targeted notice, and disconnect through the verified KMTGuard procedures.
- Added automatic CharID or player-name targeting without exposing procedure variants, queues, or runtime implementation details to the operator.
- Simplified the account, character, inventory, skill, silk, monster, drop, fortress, and economy studios so one selected action is visible at a time with its inputs, preview, confirmation, and result.
- Refreshed login, profile, connection, dashboard, item, NPC, area, audit, and maintenance presentation and removed raw-table shortcuts from the daily Area workflow.
- Requirement: refresh CASY with `Ctrl+F5` to load the latest web assets. No Filter, Client DLL, media, or KMTGuard SQL package update is required.

## CASY Web Studio 0.4.0

Release date: 2026-08-28

- Replaced the generic database-table screens with focused one-click task studios; raw records remain available only in the Advanced workspace.
- Added 30 guarded automation tools derived from the supplied 95-task SQL library, including account, character, inventory, silk, monster, spawn, drop, fortress, stack, Gacha, job EXP, and SOX operations.
- Added validation, impact preview, owner confirmation, backup proof for critical actions, and audit records around every published write tool.
- Fixed SQL Server pagination parameter handling that could incorrectly make connected studios appear empty.
- Removed unfinished data-only sections from normal navigation until their full task workflow is available.
- Requirement: refresh CASY and load the latest frontend assets. No Filter, Client DLL, media, or KMTGuard SQL package update is required.

## CASY Web Studio 0.3.1

Release date: 2026-08-28

- Refined the CASY panel chrome to match the approved reference direction with pale navigation, compact utilities, connection status, and warm action controls.
- Replaced legacy text glyphs with accessible SVG controls across the main web studios.
- Added local `Ctrl/Cmd + K` page search for faster navigation between studios.
- Corrected ID allocation for non-identity linked item rows and full-world dependencies.
- Included reviewed `Teleport_*` procedures in Live Control discovery without changing the KMTGuard runtime contract.
- Requirement: refresh the CASY web application and rebuild frontend assets. No Filter, Client DLL, or SQL update is required.

## CASY Web Studio 0.3.0

Release date: 2026-08-28

- Added the modern CASY administration workspace with grouped navigation, responsive layout, SVG identity assets, secure owner-only access, and HTTPS/security-header defaults.
- Added guarded Area Clone Quick/Full workflows, DDJ library reconciliation, composite-key/paginated Advanced Table Studio, filtered audit export, and Maintenance Vault dry-run confirmation.
- Focused Live Control on approved KMTGuard player procedures only; no Filter, Client DLL, SQL package, or runtime action contract was changed in this web release.
- Requirement: refresh the CASY web application and apply its Laravel migrations. Use a least-privilege SQL login for daily operations.

## Update v6.0.0

Release date: 2026-08-27

- Corrected Auto Mastery and Auto Skill so they continue operating after the Skills window is closed and stop safely when the character changes or the client cannot confirm an update.
- Fixed a client crash that could occur while opening the Skills window after the automation update.
- Restored Auto Mastery and Auto Skill activation so both features continue retrying safely while game skill data is still loading.
- Improved Auto Mastery and Auto Skill response timing while keeping their safe retry and server-protection behavior unchanged.
- Requirement for this Client correction: replace the Client DLL and client media/resources, then restart the game client. No SQL, Filter, or GameServer update is required.

- Rebuilt protected-region admission so character, Job, race, level, party, and travel-method requirements are checked before Teleport, Reverse, Trace, and managed server travel begins.
- Replaced the region-specific bot timer with an optional inactivity return. Each region can disable the feature or return characters to town after its configured number of seconds without movement.
- Hardened custom Reverse travel so the GameServer requires an authenticated admission approval before consuming the scroll or beginning world movement.
- Corrected the protected-region SQL update so newly added admission columns are available before existing rows are normalized on SQL Server.
- Corrected destination verification for normal and custom NPC teleport gates so valid travel is no longer denied before region admission checks run.
- Corrected signed Region IDs so protected regions with negative IDs are recognized consistently across Teleport, Reverse, Trace, managed travel, and arrival verification.
- Corrected Last Man Standing combat validation so players inside the configured event arena can target and attack each other correctly.
- Requirement for this LMS correction: replace the GameServer add-on and restart the GameServer. No SQL, Filter, Client DLL, or media update is required.
- Corrected Defend The Tower generation so both configured towers are queued through the existing `NPC_SpawnAtPosition` database procedure using the event's configured World ID, Region ID, and coordinates.
- Requirement for this Defend The Tower correction: replace the Filter and restart it. No SQL, GameServer, ShardManager, Client DLL, or media update is required.
- Restored GameServer monster generation for database spawn commands, including spawn by position, spawn by code name, and spawn near a player, so event towers and event monsters are no longer discarded before native creation.
- Requirement for this monster generation correction: replace the GameServer add-on and restart the GameServer. No SQL, Filter, ShardManager, Client DLL, or media update is required.
- Removed the recurring GameServer command bridge health line from the ShardManager console while retaining startup, recovery, and genuine failure diagnostics.
- Requirement for this console cleanup: replace the ShardManager add-on and restart the ShardManager. No SQL, Filter, GameServer, Client DLL, or media update is required.
- Closed the short Reverse guide-packet path that could begin travel before protected-region requirements were checked; malformed or unsupported travel selectors are now rejected before reaching the GameServer.
- Added a first-stage Reverse scroll activation check so saved recall and death destinations are evaluated before the scroll can enter native GameServer travel state, including paths that omit the later destination request.
- Corrected inactivity and denied-arrival recovery so characters are returned to their actual town recall point instead of being reloaded at the same coordinates.
- Simplified Region Control into clear Build, Job, race, party, travel, and inactivity choices; removed the retired bot policy and conflicting duplicate settings while preserving existing rules during the SQL conversion.
- Rebuilt the Admin Desktop Region Control page around the simplified table, including signed Region ID support and readable rule summaries.
- Added a GameServer-authoritative item/region travel guard: the server now records the real inventory Item ID at use time and rejects matching world movement before it starts. The same rule also prevents that item from being used while the character is already inside the configured region.
- Simplified item/region rules to World ID, Region ID, Item ID, Enabled, and Note. Rules are loaded once at GameServer startup, so changing them requires a GameServer restart and adds no recurring database load.
- Requirement for a fresh v6.0.0 installation: apply the included SQL update, replace the Filter, Admin Desktop, and GameServer add-on, then restart the Filter and GameServer.
- Requirement for this corrective refresh: apply the included v6.0.0 SQL update first, replace the Filter and GameServer add-on, then restart both services. No Client DLL or media update is required.

Release date: 2026-08-17

- Fixed an issue where players could bypass region restrictions by using a standard Reverse Return Scroll from a safe area to teleport into a restricted area. The Filter now intercepts and blocks the actual teleport request if the destination region is restricted.

- Prevented characters from registering for or teleporting into events (Survival Solo, Survival Party, Madness, Defend Tower, Last Man Standing) while wearing a Job suit.
- Players wearing a Job suit will now receive a warning message and must remove their suit to participate.

- Added a new Hook_SpawnComplete stored procedure that runs after a character has fully entered the game world, providing CharID, CharName, RegionID, WorldID, JobType, InParty, and FirstSpawn parameters for custom post-spawn logic such as welcome messages, VIP buffs, or tracking.
- FirstSpawn indicates whether the character has just logged in for the first time in this session; teleports and region changes do not re-trigger the hook.
- The hook runs in the background and does not block or delay the normal spawn process.
- Requirement: run the SQL update, replace the Filter files, and restart the Filter. No Client DLL, media, GameServer, or ShardManager update is required.

Release date: 2026-08-17

- Removed high-frequency Clientless hunting telemetry from database persistence and routine logs, preventing combat activity from saturating the background database queue or producing oversized log files.
- Clientless connection, online, disconnection, and genuine failure diagnostics remain available; hunting and connection behavior are unchanged.
- Prevented the new Job experience bar from closing the client when a character receives negative, incomplete, or otherwise invalid Job experience data.
- Monster Drops now load both grouped/random and direct monster item drops through the new database procedure used by the Filter. Apply the SQL update and replace the Filter files; no Client DLL or media update is required.
- Established KMTGuard 6.0.0 as the unified source baseline across the Filter, Client DLL, GameServer add-on, ShardManager add-on, launcher, licensing tools, and desktop applications.
- Preserved the complete existing source, release history, generated artifacts, media, database updates, and supporting tools in the baseline snapshot.
- Requirement: replace the Filter files and Client DLL, restart the Agent role, and fully restart the game client. No SQL, media, Admin Desktop, GameServer, or ShardManager update is required.

## Update v3.9.8

Release date: 2026-08-15

- Corrected false required-client-update disconnects when a player retried login after the Gateway had rejected a previous attempt.
- Failed Gateway login attempts now return verified players to the normal login state without repeating device verification or weakening client validation.
- Requirement: replace the Filter files and restart the Gateway role. No SQL update, Client DLL replacement, media update, Admin Desktop replacement, GameServer replacement, or ShardManager replacement is required.

## Update v3.9.7

Release date: 2026-08-15

- Fixed Reverse Scroll movement to a party member leaving the game interface in a broken loading state and disconnecting the player.
- Requirement: replace the GameServer add-on and restart the GameServer. No Filter, SQL, Client DLL, media, or ShardManager update is required.

## Update v3.9.6

Release date: 2026-08-14

- Eliminated the intermittent client startup crash seen with SBot, mBot, and other external loaders by preventing the login interface from starting before KMTGuard initialization and interface registration are complete.
- Added safe main-thread recovery for loader sequences that attach after the client's normal asset-initialization phase, so custom login interfaces cannot be created from an incomplete runtime state.
- Client startup now stops with a dedicated diagnostic if initialization genuinely fails instead of continuing into an unrelated client assertion.
- Requirement: replace the Client DLL, fully close the game and any bot or launcher processes, then start them again. No Filter, SQL, media, GameServer, or ShardManager update is required.

## Update v3.9.5

Release date: 2026-08-14

- Corrected the Clientless world-entry handoff so finishing the startup packet burst no longer cancels an active network read or closes an otherwise healthy character connection.
- Clientless characters now retain the same live socket while moving from character loading into online keepalive and combat-profile preparation.
- Restored the GameServer's complete native-compatible world-transfer handshake for authenticated Clientless hunting and town positioning, preventing characters from disappearing at their first automated relocation.
- GameServer startup now includes a reversible cleanup for item-option mappings that reference missing skills, preventing hidden blocking dialogs from leaving the shard unavailable after a restart while preserving every removed mapping in a backup table.
- Clientless Gateway rejections now retain the upstream reason code in diagnostics so account, capacity, and shard-availability failures can be distinguished without guesswork.
- Requirement: Developer/Test preview only. Replace the Developer Filter package and Developer/Test GameServer add-on, apply the v3.9.5 SQL update, then restart the Agent Filter and GameServer. No client DLL, media, or ShardManager replacement is required. CustomerProductionBase remains unchanged.

## Update v3.9.4

Release date: 2026-08-14

- Clientless characters now finish the complete initial world-spawn cycle before hunting, town parking, party automation, or custom movement begins, preventing newly connected characters from immediately disappearing and reconnecting.
- World entry now consumes the remaining startup packet burst before enabling automation instead of depending on database-loading speed or server timing.
- Socket receiving and keepalive now continue while combat profiles load, so large account batches do not disconnect when database preparation takes several seconds.
- Requirement: Developer/Test preview only. Replace the Developer Filter package and restart the Agent Filter. No SQL update, client DLL, media, GameServer, or ShardManager replacement is required. CustomerProductionBase remains unchanged.

## Update v3.9.3

Release date: 2026-08-14

- Restored the concise professional GameServer console by hiding routine per-setting old/new value lines while retaining genuine validation, patch, rollback, and startup failures.
- Kept the corrected single-owner Quest RaiseEvent diagnostic patch and all existing GameServer behavior unchanged.
- Requirement: Developer/Test preview only. Replace the Developer/Test GameServer add-on and restart the GameServer. No Filter, SQL, client DLL, media, or ShardManager update is required. CustomerProductionBase remains unchanged.

## Update v3.9.2

Release date: 2026-08-14

- Corrected the GameServer startup regression caused by applying the Quest RaiseEvent suppression from two initialization paths.
- The diagnostic is now owned by one verified startup patch that suppresses only its five-byte logging call, preserves the surrounding stack behavior, and restores the original call during rollback.
- Added a regression check that rejects duplicate ownership of the same GameServer instruction block.
- Requirement: Developer/Test preview only. Replace the Developer/Test GameServer add-on and restart the GameServer. No Filter, SQL, client DLL, media, or ShardManager update is required. CustomerProductionBase remains unchanged.

## Update v3.9.1

Release date: 2026-08-14

- Removed the repeatedly generated Quest RaiseEvent diagnostic from the GameServer hot path, reducing unnecessary logging overhead while preserving quest processing.
- Requirement: replace the GameServer add-on and restart the GameServer. No SQL update, Filter replacement, Client DLL replacement, media update, or ShardManager replacement is required.

## Update v3.9.0

Release date: 2026-08-14

- Developer/Test Create Accounts now controls Attack Pets and Grab Pets independently, allowing either pet, both pets, or no pets for each new batch.
- Attack-pet supplies are added only when an Attack Pet is selected, while Grab-only characters receive only their selected standard vSRO Grab Pet.
- Existing Clientless accounts preserve their previous combined pet preference when the independent settings are introduced, and Prepare Existing Combat respects the stored choice for each pet type.
- Requirement: Developer/Test preview only. Replace the Developer Filter package and apply the included v3.9.0 SQL update before testing new account creation. CustomerProductionBase remains on v3.8.0.

## Update v3.8.0

Release date: 2026-08-14

- Party Matching now offers two clear persistent modes: one individual Party Form per online Clientless character with no managed grouping, or real city-based parties of up to eight with one leader Party Form.
- Switching modes removes the previous managed Party Forms and party membership before applying the selected behavior, preventing characters from continuing to regroup after Individual Forms is selected.
- The Party Matching dashboard now explains both choices, applies the selected mode with one button, and enables the manual party rebuild action only for Groups of 8 mode.
- Disabling Party automation now removes managed Party Forms and leaves managed parties instead of leaving old party membership active.
- Requirement: replace the Filter package, restart the Agent Filter and Control Center, and apply the included v3.8.0 SQL update. No Client DLL replacement, media update, GameServer replacement, or ShardManager replacement is required.

## Update v3.7.0

Release date: 2026-08-14

- Bulk Clientless creation now completes mixed level ranges correctly when a low-level character has not naturally unlocked its first weapon skill yet; the character uses its normal attack until the mastery skill becomes available.
- Skill preparation still stops safely when an attack skill should be available but could not be installed, preserving the existing weapon, mastery, and combat validation.
- Create Accounts now provides independent Avatar Outfit and Attack & Grab Pets choices for each new batch, with both enabled by default to preserve existing behavior.
- Accounts created with avatars or pets disabled keep that choice when Prepare Existing Combat is used, while all existing accounts retain their current enabled behavior.
- Requirement: replace the Filter package and restart the Control Center. Apply the included v3.7.0 SQL update before creating new accounts. No Client DLL replacement, media update, GameServer replacement, or ShardManager replacement is required.

## Update v3.6.9

Release date: 2026-08-14

- The Control Center now uses the SQL password stored in Windows Credential Manager when the packaged settings file intentionally contains no password, preventing false SQL Offline status after an update.
- Developer/Test publishing now preserves the operator's existing local settings instead of replacing saved connection details.
- Requirement: replace the Filter package and restart the Control Center. No SQL update, Client DLL replacement, media update, GameServer replacement, or ShardManager replacement is required.

## Update v3.6.8

Release date: 2026-08-14

- The Control Center now opens responsively without loading complete service log files during Dashboard startup.
- Service log previews read only a bounded trailing window, so opening Runtime Actions or Service Logs remains fast even when an older log is very large.
- Filter service logs now roll automatically at 32 MB and retain a bounded history, preventing a single daily log from growing to gigabytes.
- Requirement: replace the Filter package and restart the Control Center, Agent, Download, and Gateway processes. No SQL update, Client DLL replacement, media update, GameServer replacement, or ShardManager replacement is required.

## Update v3.6.7

Release date: 2026-08-14

- Clientless characters assigned to stand in town are now distributed across multiple safe areas throughout their selected city instead of reusing old clustered home coordinates.
- Town parking is recalculated for the complete active city group, keeping characters visibly separated and consistently distributed when accounts are opened or reloaded.
- Characters returning after hunting or death now finish at their assigned separated town position rather than remaining stacked on the shared resurrection point.
- The distribution applies to Jangan, Donwhang, Hotan, SamarKand, Constantinople, and Alexandria North (SD), with spacing adjusted automatically for the number of active characters.
- Requirement: replace the Filter package and restart the Agent Filter. No SQL update, Client DLL replacement, media update, Admin Desktop replacement, GameServer replacement, or ShardManager replacement is required.

## Update v3.6.6

Release date: 2026-08-14

- Clientless pet variation is now restricted to a verified set of original vSRO attack and grab pets; custom, event, fellow, and incomplete media records are never selected.
- Clientless combat refuses to summon any pet outside the verified vSRO set, preventing nearby game clients from crashing because a custom pet has missing client resources.
- **Prepare Existing Combat** now replaces previously assigned non-standard pets with stable, varied vSRO pets and removes their obsolete companion records safely.
- Pet preparation now works across servers whose Filter and Shard databases use different collations.
- Requirement: stop or disconnect the managed accounts, replace the Filter package and Admin Desktop application, restart the Agent Filter, then run **Prepare Existing Combat** once. If an old grab pet contains stored items, empty it before preparation. No separate SQL script, Client DLL replacement, media update, GameServer replacement, or ShardManager replacement is required.

## Update v3.6.5

Release date: 2026-08-14

- Corrected Saved Accounts so Quick Login remains available after saving, removing, retrying, or using another saved account on the same login screen.
- Repeated clicks are now ignored safely while the next secure request is being prepared, preventing false request-expired messages without reducing device verification.
- Requirement: replace the Filter files and restart the Gateway role. No SQL update, Client DLL replacement, media update, Admin Desktop replacement, GameServer replacement, or ShardManager replacement is required.

## Update v3.6.4

Release date: 2026-08-14

- Clientless hunters now acquire the next available monster with a short, naturally varied reaction instead of waiting through a visible idle interval after each kill.
- New managed characters receive a varied attack pet and grab pet selected from the server's active vSRO catalog, together with attack-pet health, revival, and hunger supplies.
- Clientless combat automatically summons both pets, commands the attack pet against the current monster, restores its health and hunger, revives it when required, and safely summons either pet again after it disappears.
- **Prepare Existing Combat** now adds the same varied pet pair and supplies to existing managed characters while preserving pets they already own.
- Clientless coordinate transfers now use the direct-position route, removing the misleading GameServer `unknown teleport gate: 0` message generated by a position transfer that did not use a teleport gate.
- Pet provisioning is performed as one protected inventory allocation batch, keeping large account creation and preparation runs responsive.
- Requirement: replace the Filter package, Admin Desktop application, and GameServer add-on; restart the Agent Filter and GameServer, then run **Prepare Existing Combat** once for existing managed characters that need pets. No separate SQL script, Client DLL replacement, media update, or ShardManager replacement is required.

## Update v3.6.3

Release date: 2026-08-14

- Corrected every European Clientless armor family against the server's native vSRO records: Warriors now wear heavy armor, Rogues wear light armor, and Wizard, Warlock, Bard, and Cleric characters wear robes. Both normal and C_RARE sets are supported for male and female characters.
- New Clientless characters now receive one of six complete, gender-compatible avatar sets. **Prepare Existing Combat** gives the same varied avatars to managed characters whose avatar inventory is empty while preserving any avatar deliberately assigned earlier.
- Party Matching no longer creates one isolated form per character. Online characters are grouped into real GameServer parties of up to eight by city, and only each confirmed leader publishes a Party Form linked to the real party.
- Party grouping now runs automatically after the login batch settles, while the dashboard still provides a manual rebuild action when an operator wants to regroup immediately.
- Replaced the shared hunter-count control with a clear per-city runtime plan. Operators can choose the total accounts to keep online and the number of those accounts that should hunt; the difference remains online and parked in the city, while the remainder stays offline.
- Applying a city runtime plan uses stable account ordering so the same characters keep their online, hunter, and parked roles across reloads.
- The Hunting Areas coordinate editor now uses direct text fields with save-time validation, preventing the desktop application from freezing when moving between Region, X, Y, Z, and Radius fields.
- Requirement: replace the Filter package and Admin Desktop application, restart the Agent Filter service, and run **Prepare Existing Combat** once for existing managed characters that need corrected European equipment or avatars. No separate SQL script, Client DLL replacement, media update, GameServer replacement, or ShardManager replacement is required.

## Update v3.6.2

Release date: 2026-08-14

- Corrected the European Wizard and Rogue mastery assignment to match the server's native vSRO records: Wizard Staff now uses Wizard mastery, while Crossbow and Dagger use Rogue mastery.
- European account creation no longer rolls back a complete city batch with “No valid monster attack skill” when Wizard Staff, Crossbow, or Dagger is selected.
- Existing Wizard, Crossbow, and Dagger characters can now be repaired by **Prepare Existing Combat**, which replaces the incorrect mastery and skills with the correct weapon-compatible ranks.
- Verified every supported European weapon against the configured server reference database at level 110; each class resolves one or more valid learned damage-skill branches before creation is accepted.
- Requirement: replace the Filter package and Admin Desktop application, restart the Agent Filter service, and run **Prepare Existing Combat** once for European accounts created by an earlier version. No separate SQL script, Client DLL replacement, media update, GameServer replacement, or ShardManager replacement is required.

## Update v3.6.1

Release date: 2026-08-14

- Corrected the batch equipment-repair failure that stopped on the second character with a duplicate temporary slot key; **Prepare Existing Combat** now processes complete Chinese and European account batches.
- Reworked European damage-skill discovery to follow native vSRO damage metadata even when European skill records use different target flags, restoring Wizard Staff and the other European weapon rotations while still excluding imbues, resurrection, sacrifice, and base placeholders.
- Generated and prepared Clientless characters now receive a large level-scaled STR and INT survival reserve, giving both builds genuinely high GameServer-derived maximum HP and MP instead of only raising the current values.
- Every generated character now receives a standard vSRO speed-drug stack; Clientless combat activates it automatically and schedules renewal when its normal duration expires. **Prepare Existing Combat** adds the item to older managed characters when missing.
- New characters retain their original town coordinate for later standby returns, while older accounts use a stable five-area city-wide distribution instead of appearing in one cluster.
- Creating real parties of eight now clears stale individual listings and submits one Party Form per confirmed leader using the actual GameServer party ID; the dashboard action also enables Party Form automatically when required.
- Strengthened Clientless equipment repair and verification so previously incomplete European robe, light, or heavy armor profiles are rebuilt before their combat skills are prepared.
- Requirement: replace the Filter package and Admin Desktop application, restart the Agent Filter service, and run **Prepare Existing Combat** once for existing accounts. No separate SQL script, Client DLL replacement, media update, GameServer replacement, or ShardManager replacement is required.

## Update v3.6.0

Release date: 2026-08-14

- Corrected newly generated European equipment families so Warriors receive heavy armor, Rogues receive light armor, and Wizard, Warlock, Bard, and Cleric characters receive robes that match their vSRO class.
- New Clientless characters are now rejected safely unless every selected equipment slot resolves to a live item with a valid reference, durability, race, gender, level, and inventory link; incomplete characters are no longer reported as successfully equipped.
- **Prepare Existing Combat** now rebuilds and verifies the complete visible equipment set before repairing stats, masteries, skills, shields, arrows, and bolts.
- Corrected European attack discovery so valid entity-targeted Wizard Staff and other European damage skills are not discarded because their activity metadata overlaps the value used by Chinese imbues; base placeholders remain excluded.
- Character creation and preparation now stop with a clear error if the selected weapon has no valid learned monster-attack skill, preventing silent normal-attack-only profiles.
- Added **Create Parties of 8 Now** to the Clientless dashboard. It creates real confirmed vSRO parties by city and hunting area, assigns a leader, securely accepts only managed invitations, and caps each group at eight members.
- Returning Clientless characters now receive stable separate parking positions around their selected town instead of appearing on exactly the same coordinates.
- Requirement: replace the Filter package and Admin Desktop application, restart the Agent Filter service, and run **Prepare Existing Combat** once for previously created Clientless characters. No separate SQL script, Client DLL replacement, media update, GameServer replacement, or ShardManager replacement is required.

## Update v3.5.2

Release date: 2026-08-13

- Corrected false required-client-update disconnects that could occur when login started before device verification was delivered and completed.
- Repeated server-list and character-selection updates no longer replace an active device-verification request.
- Requirement: replace the Filter files and restart the Gateway and Agent roles. No SQL update, Client DLL replacement, media update, Admin Desktop replacement, GameServer replacement, or ShardManager replacement is required.

## Update v3.5.1

Release date: 2026-08-13

- Corrected false required-client-update disconnects on Windows Server, VPS, and player PCs when local device verification could not finish its first initialization.
- Multiple game clients opened under the same Windows account now initialize device verification consistently without interfering with each other.
- Requirement: replace the Client DLL and fully restart all game clients. No Filter restart, SQL update, media update, Admin Desktop replacement, GameServer replacement, or ShardManager replacement is required.

## Update v3.5.0

Release date: 2026-08-13

- Rebuilt Clientless world-loading and movement tracking around the complete vSRO character-data, teleport, position, and speed lifecycle, preventing repeated transfers, invisible characters, false arrivals, and synchronized reconnect loops.
- Corrected learned-skill selection for every supported Chinese and European weapon, including Wizard Staff and Warlock Rod, while separating real monster attacks, Chinese imbues, self buffs, and incompatible utility skills.
- Improved combat behavior with confirmed self buffs, independent emergency HP and MP recovery, ranged stopping distance, stable target movement, and normal-attack fallback only when no compatible learned attack is usable.
- Corrected generated and prepared characters so STR, INT, current HP, current MP, account ownership, mastery limits, and obsolete mastery skills remain consistent with the selected level and weapon profile.
- Added a customer-friendly hunter allocation per city: all enabled accounts may stay online while only the requested number hunts, and one button stops hunting and returns the selected cities to standby without disconnecting them.
- Replaced the Hunting Areas coordinate grid with a direct five-row editor so moving between Region, X, Y, Z, and Radius no longer freezes the Dashboard; incomplete values are validated only when saving.
- Added clearer city totals for assigned hunters, ready standby accounts, unavailable accounts, hunting roles, and explicit start, reload, standby, and disconnect actions.
- Requirement: replace the Filter package and Admin Desktop application, run **Prepare Existing Combat** once for previously created Clientless characters, then restart the Agent Filter service. The v3.3.0 database update must already be installed; no new SQL script, client DLL replacement, media update, GameServer replacement, or ShardManager replacement is required.

## Update v3.4.3

Release date: 2026-08-12

- Rebuilt Clientless skill discovery around the native vSRO damage and duration metadata so every weapon uses genuine learned damage skills instead of utility, condition-only, or base-placeholder actions.
- Clientless characters now prepare real compatible self buffs before combat, send self-target buffs with the correct character target, and choose one compatible Chinese imbue without repeatedly replacing it.
- Improved combat rotation to respect each skill's range and full action duration, immediately continue to another learned skill after a rejection, and reserve normal attacks for periods when no learned damage skill is ready.
- Removed hunting-area teleports from routine movement and target timeout recovery, preventing characters from repeatedly disappearing and reappearing while they hunt.
- Added explicit confirmed-death diagnostics so a real death and town recovery can be distinguished from movement recovery in Filter logs.
- Requirement: replace the Filter package and Admin Desktop application, then restart the Agent Filter service. No SQL update, client DLL replacement, media update, GameServer replacement, or ShardManager replacement is required.

## Update v3.4.2

Release date: 2026-08-12

- Corrected the Clientless world-entry and hunting-area teleport handshake for vSRO servers, preventing connected characters from becoming invisible after a position transfer.
- Clientless characters now confirm their initial world spawn only after the complete character payload is received, so the Dashboard no longer reports a premature in-game state.
- Added compatibility with both supported AgentServer game-reset opcode pairs while prioritizing the RSBot/vSRO protocol used by the configured server files.
- Requirement: replace the Filter package and Admin Desktop application, then restart the Agent Filter service. No SQL update, client DLL replacement, media update, GameServer replacement, or ShardManager replacement is required.

## Update v3.4.1

Release date: 2026-08-12

- Prevented Clientless characters from leaving combat when an incompatible support skill does not receive a GameServer confirmation; visible monsters and attack skills now always take priority over idle self-buff preparation.
- Limited automatic support casting to genuine self-target buffs and safely disables repeatedly incompatible buffs for the current session without interrupting hunting.
- Hunting-area, global hunting, and per-account hunting changes now apply live without disconnecting or reloading online Clientless accounts.
- Requirement: replace the Filter package and Admin Desktop application, then restart the Agent Filter service. No SQL update, client DLL replacement, media update, GameServer replacement, or ShardManager replacement is required.

## Update v3.4.0

Release date: 2026-08-12

- Rebuilt Clientless combat confirmation around the native vSRO action response so the Dashboard reports an attack skill only after GameServer confirms that exact skill for that character.
- Added automatic self-buff casting and active-buff tracking, including Chinese Fire and Lightning support skills, while keeping the total mastery level within the configured server limit.
- Corrected Full STR and Full INT allocation from the vSRO base value and added one-click repair for previously created Clientless characters.
- Corrected arrows and bolts so ranged characters equip their ammunition in the required secondary equipment slot, with automatic repair for existing accounts.
- Improved real combat behavior with weapon-matched skill rotation, casting-time-aware responses, MP use only when required, closer melee approach, and removal of conditional down-attack skills from normal rotation.
- Prevented routine Clientless reloads from racing AgentServer session cleanup, reducing authentication disconnect and reconnect loops during bulk operations.
- Expanded **Prepare Existing Combat** into a safe stop, repair, mastery and skill preparation, and restart workflow with automatic restart recovery if preparation fails.
- Requirement: replace the Filter package and Admin Desktop application, then use **Prepare Existing Combat** once for previously created accounts. No separate SQL script, client DLL replacement, media update, GameServer replacement, or ShardManager replacement is required.

## Update v3.3.4

Release date: 2026-08-12

- Corrected **Prepare Existing Skills** for installations where the Filter and Shard databases use different text collations.
- Requirement: replace the Filter package and Admin Desktop application, then use **Prepare Existing Skills** once for previously created Clientless accounts. No SQL script, client DLL replacement, media update, GameServer replacement, or ShardManager replacement is required.

## Update v3.3.3

Release date: 2026-08-12

- Added a one-click Dashboard action that opens the correct weapon mastery, installs the highest level-valid skill from every branch, and reloads all existing Clientless accounts automatically.
- Made weapon mastery with maximum skills the default for newly created Clientless hunting accounts while retaining the optional default-skills and mastery-only choices.
- Matched Clientless attack-skill discovery to the vSRO skill classification used by the built-in Macro Bot, preventing valid learned skills from being omitted.
- Corrected the encrypted vSRO inventory-use request used by Clientless HP and MP potions.
- Requirement: replace the Filter package and Admin Desktop application, then use **Prepare Existing Skills** once for previously created accounts. No SQL script, client DLL replacement, media update, GameServer replacement, or ShardManager replacement is required.

## Update v3.3.2

Release date: 2026-08-12

- Corrected Clientless hunting so learned attack skills are selected only when they match the weapon actually equipped by the character.
- Added native vSRO combat-response handling so accepted skills rotate with their real cooldown while rejected skills are skipped safely and the normal attack remains an automatic fallback.
- Added automatic MP preparation before the first learned skill and clearer live hunting details for approaching, selecting, skill execution, and GameServer rejection states.
- Requirement: replace the Filter package and restart the Agent Filter service. No SQL update, Admin Desktop replacement, client DLL replacement, media update, GameServer replacement, or ShardManager replacement is required.

## Update v3.3.1

Release date: 2026-08-12

- Corrected the Clientless hunting coordinate editor so moving from Region to X, Y, Z, or Radius no longer locks the Dashboard during numeric conversion.
- Made coordinate entry cell-based and validates complete rows only when saving, with support for standard, regional decimal, and Arabic digit input.
- Requirement: replace the Admin Desktop application. No additional SQL update is required after v3.3.0, and no Agent Filter restart, client DLL replacement, media update, GameServer replacement, or ShardManager replacement is required.

## Update v3.3.0

Release date: 2026-08-12

- Added a complete Clientless hunting workspace with five manually configurable Region, X, Y, Z, and radius areas for every supported city.
- Added even account distribution across enabled hunting areas, city-specific startup and reload control, per-account hunting pause and resume actions, and live area, target, activity, and recovery status.
- Added automatic normal-monster and unique targeting, optional unique priority, learned attack-skill rotation, basic-attack fallback, HP and MP potion use, death recovery, patrol movement, target timeout, and stuck-position recovery.
- Added Full STR, Full INT, or random stat builds that allocate every earned level point correctly when Clientless characters are created.
- Added automatic HP and MP supplies plus arrows or bolts for new ranged characters while preserving the existing race, gender, weapon, normal or rare equipment, no-equipment, mastery, and maximum valid skill choices.
- Requirement: replace the Filter package and Admin Desktop application, apply the included v3.3.0 SQL update, then restart the Agent Filter service. No client DLL replacement, media update, GameServer replacement, or ShardManager replacement is required.

## Update v3.2.17

Release date: 2026-08-12

- Ensured the Hide and Seek event character becomes visible after reaching the selected hiding location when the required Game Master account enters the world invisibly.
- Added visibility-state confirmation before the search round is announced, preventing players from receiving an event that cannot be completed because its character is still hidden.
- Requirement: replace the Filter package and restart the Agent Filter service. No SQL update, client DLL replacement, media update, GameServer replacement, or ShardManager replacement is required.

## Update v3.2.16

Release date: 2026-08-12

- Restored reliable Hide and Seek movement by completing the managed event character's server teleport and loading sequence with the correct game protocol.
- Added compatibility protection so managed accounts can complete teleports without requiring a database whitelist update.
- Reorganized Clientless administration into a guided overview, account creation, account status, city runtime, and party workflow for operators without technical database knowledge.
- Added live Clientless totals and readiness summaries for every city, filters for account state and city, and the ability to start, reload, or stop one or several selected cities.
- Added Clientless creation choices for Chinese or European race, gender, exact weapon, rare or normal level-matched equipment, no equipment, and automatic weapon mastery with optional maximum valid skills.
- Ensured maximum-skill creation keeps only the highest level-valid rank from each skill branch and respects the server's character and mastery limits.
- Requirement: replace the Filter package and Admin Desktop application, then restart the Agent Filter service. No SQL update, client DLL replacement, media update, GameServer replacement, or ShardManager replacement is required.

## Update v3.2.15

Release date: 2026-08-12

- Corrected Hide and Seek positioning so its managed event character completes the server teleport and loading sequence before the search round begins.
- Made authenticated Filter teleports reliable after reconnects and improved failure diagnostics with the character's actual location state.
- Requirement: replace the Filter package and restart the Agent Filter service. No SQL update, client DLL replacement, media update, GameServer replacement, or ShardManager replacement is required.

## Update v3.2.14

Release date: 2026-08-12

- Added vertical scrolling to every event schedule editor so start times, day controls, and save actions remain accessible on smaller windows and displays.
- Requirement: replace the Admin Desktop application. No Filter restart, SQL update, client DLL replacement, media update, GameServer replacement, or ShardManager replacement is required.

## Update v3.2.13

Release date: 2026-08-11

- Made customer creation and package generation reuse the current validated licensed production release instead of rebuilding the complete solution every time.
- Added a bounded automatic-build timeout so a genuinely stalled release refresh stops with a clear error instead of leaving License Center busy indefinitely.
- Requirement: replace License Center only. No customer Filter restart, SQL update, client DLL replacement, media update, GameServer replacement, or ShardManager replacement is required.

## Update v3.2.12

Release date: 2026-08-11

- Rebuilt the Achievement Center as a cohesive Old School Silkroad window with classic framing, native inset surfaces, compact category controls, and a standard close button.
- Added wide achievement cards with category icons and live numeric progress bars so completion status is readable directly from the scrolling list.
- Preserved the existing Titles and Achievements tabs, filters, descriptions, title actions, and all gameplay behavior.
- Requirement: replace the client DLL and apply the included media update. No Filter restart, SQL update, GameServer replacement, or ShardManager replacement is required.

## Update v3.2.11

Release date: 2026-08-11

- Corrected ShardManager command-worker thread ownership so runtime health monitoring, orderly shutdown, and automatic recovery use valid operating-system thread handles.
- Removed the obsolete detached-thread compatibility layer that could report an active command worker as stopped and repeatedly emit invalid-handle monitor errors.
- Restored high-throughput bounded queue draining after confirming that the prior stopped-worker report was a monitoring error rather than an actual dispatch failure.
- Requirement: replace the ShardManager add-on and restart ShardManager. No SQL update, Filter replacement, GameServer replacement, client DLL replacement, or media update is required.

## Update v3.2.10

Release date: 2026-08-11

- Stabilized large command-queue draining by applying a safe bounded dispatch rate that does not overwhelm the ShardManager network bridge.
- Added automatic command-worker recovery when an unexpected native runtime failure stops the worker, so the remaining queue continues without restarting the whole server.
- Added native exception code, address, and restart diagnostics for unexpected command-worker termination.
- Removed a process-wide console interrupt handler from the command worker so unrelated console events cannot stop runtime command processing.
- Requirement: replace the ShardManager add-on and restart ShardManager. No SQL update, Filter replacement, GameServer replacement, client DLL replacement, or media update is required.

## Update v3.2.9

Release date: 2026-08-11

- Restored the live GameServer command bridge throughput so large pending queues drain at a bounded production rate instead of processing only one command per second.
- Made database polling, fetching, completion, and retry failures recover automatically without permanently stopping the worker or blocking all newer commands behind one failed row.
- Added a periodic ShardManager command-bridge health report with worker state, poll and progress age, last command and action, outcome, duration, totals, and database failure count.
- Preserved all existing GameServer action identifiers, payloads, validation, durable results, and command ordering.
- Requirement: replace the ShardManager add-on and restart ShardManager. No SQL update, Filter replacement, GameServer replacement, client DLL replacement, or media update is required.

## Update v3.2.8

Release date: 2026-08-11

- Removed the temporary disconnect investigation logging after confirming and correcting the authenticated-session timeout issue.
- Stopped generating the temporary per-session Filter reports, GameServer combat-disconnect reports, and continuous client network diagnostic files.
- Silenced the expected compatibility warning on Windows hosts that keep standard TCP keepalive active but do not expose its optional tuning controls.
- Retained the corrected authenticated-session monitoring and all normal operational, error, security, and licensing logs.
- Requirement: replace the Filter package, GameServer add-on, and client DLL; restart the Agent Filter and all GameServers. No SQL update, ShardManager replacement, or media update is required.

## Update v3.2.7

Release date: 2026-08-11

- Corrected authenticated session monitoring so active players are no longer disconnected when normal gameplay traffic replaces the separate idle heartbeat.
- Preserved the configured inactivity timeout for genuinely silent sessions and retained all existing packet, flood, bot, and region protections.
- Requirement: replace the Filter package and restart the Agent Filter service. No SQL update, client DLL replacement, GameServer replacement, ShardManager replacement, or media update is required.

## Update v3.2.6

Release date: 2026-08-11

- Added temporary disconnect forensics that preserve the last 50 session packets in memory and write them only when a player connection closes.
- Added correlation details for the character, session, both proxy sockets, disconnect initiator, call site, protection decision, combat activity, and the latest exception.
- Added matching GameServer and client-side forensic reports for combat sessions without changing packet handling, protection decisions, or gameplay behavior.
- Requirement: replace the Filter package, GameServer add-on, and client DLL; restart the Agent Filter and all GameServers. No SQL update, ShardManager replacement, or media update is required.

## Update v3.2.5

Release date: 2026-08-11

- Added privacy-safe client connection diagnostics that record network activity, disconnect reasons, recent packet identifiers, and stalled packet processing without recording packet contents or account data.
- Added separate per-client diagnostic files with bounded size so connection investigations remain usable when multiple clients run from the same folder.
- Requirement: replace the client DLL. No Filter restart, SQL update, GameServer replacement, ShardManager replacement, or media update is required.

## Update v3.2.4

Release date: 2026-08-11

- Made the target equipment viewer request fresh equipment every time it is opened, so changed equipment no longer remains cached.
- Cleared previously displayed slots before each accepted refresh, so removed equipment disappears and empty equipment sets display correctly.
- Corrected per-character privacy loading so one player's equipment visibility choice is never inherited from another player's settings.
- Requirement: replace the client DLL. To apply the privacy correction as well, replace the Filter package and restart the Agent Filter service. No media update, SQL update, GameServer replacement, or ShardManager replacement is required.

## Update v3.2.3

Release date: 2026-08-11

- Rebuilt the target equipment viewer as a compact Old School character showcase with balanced equipment columns, a dedicated accessory tray, and an integrated character-name plate.
- Preserved the full live character preview while removing oversized unused regions and the nested legacy-window appearance.
- Requirement: replace the client DLL and apply the included media update. No Filter restart, SQL update, GameServer replacement, or ShardManager replacement is required.

## Update v3.2.2

Release date: 2026-08-10

- Restored the target equipment viewer so inspecting another online character displays their equipped items correctly.
- Redesigned the target equipment viewer with the classic Silkroad Old School frame, native inset panels, and standard close control while preserving the live character preview.
- Kept the viewer visible within supported client resolutions and made both close controls return to a consistent state.
- Requirement: replace the Filter package, restart the Agent Filter service, replace the client DLL, and apply the included media update. No SQL update, GameServer replacement, or ShardManager replacement is required.

## Update v3.2.1

Release date: 2026-08-10

- Restyled the in-game website window with the classic Silkroad frame, native close control, and a properly bordered page area.
- Removed the transparent gap between the website header and the displayed page.
- Requirement: replace the client DLL. No Filter restart, SQL update, GameServer replacement, ShardManager replacement, or media update is required.

## Update v3.2.0

Release date: 2026-08-10

- Added permanent city groups for Clientless accounts, including automatic grouping of compatible existing accounts by their saved character location.
- Added city-specific Start, Reload, and Stop controls so one city can be managed without interrupting accounts running in other cities.
- Made disabling or deleting an account stop its active Clientless session immediately.
- Changed equipped account creation to add new accounts without clearing the existing Clientless list and separated creation from runtime startup.
- Corrected equipped account creation on restored databases whose invalid owner prevented required database operations from running.
- Made each equipped account creation transactional so a failed character cannot leave a partial Clientless account behind.
- Added city editing, city search, optional city import, clearer limits, and preservation of an existing password when an edited account is saved with the password field empty.
- Requirement: back up the KMTGuard database, apply the included SQL update, replace the Filter package, restart the Agent Filter service, and reopen the dashboard. No client DLL, GameServer, ShardManager, or media update is required.

## Update v3.1.3

Release date: 2026-08-10

- Corrected recurring queue failures caused by reused database sessions retaining an incompatible transaction isolation level.
- Restored reliable processing for Filter commands, planned commands, teleport freeze, Auto Events, Silk Stall recovery, VIP purchases, GameServer commands, and notification delivery across supported database isolation configurations.
- Added validation that prevents future queue updates from using incompatible locking settings.
- Requirement: back up the KMTGuard database, apply the included SQL update, replace the Filter package, and restart the Agent Filter service. No client DLL, GameServer, ShardManager, or media update is required.

## Update v3.1.2

Release date: 2026-08-10

- Corrected repeated SQL queue errors when customer databases use read-committed snapshot isolation.
- Restored reliable processing for Filter commands, planned commands, Auto Events, teleport freeze, Silk Stall recovery, VIP purchases, and notification queues.
- Kept concurrent queue claims protected from duplicate processing while supporting both standard and snapshot-enabled databases.
- Removed the repeated Hide and Seek system-character readiness entry from routine Filter logs while preserving failure warnings.
- Requirement: back up the KMTGuard database, apply the included SQL update, replace the Filter package, and restart the Agent Filter service. No client DLL, GameServer, ShardManager, or media update is required.

## Update v3.1.1

Release date: 2026-08-10

- Corrected the Auto Event selector to display event names instead of an internal data-row type.
- Corrected automatic schedule conflict checks so hourly and other repeating schedules can be saved successfully.
- Clarified that turning off an automatic schedule saves it in a paused state without allowing it to run.
- Requirement: replace the Admin Desktop application. The v3.1.0 SQL update remains required for recurring schedules. No new SQL, Filter runtime, client DLL, GameServer, ShardManager, or media update is required.

## Update v3.1.0

Release date: 2026-08-10

- Simplified Auto Event editing with a clear event selector, automatic editor loading, and a focused event table.
- Displayed the Alchemy target setting only while editing the Alchemy event and removed the customer-facing Monitor tab.
- Added recurring Auto Event schedules, including automatic hourly runs, with selected-day controls and clear repeat summaries.
- Prevented conflicting automatic schedules and enforced a 15-minute safety gap after an event finishes before another scheduled event can start.
- Requirement: back up the Events database, apply the included SQL update, replace the Filter and Admin Desktop, restart the Agent Filter service, and reopen the desktop application. No client DLL, GameServer, ShardManager, or media update is required.

## Update v3.0.7

Release date: 2026-08-10

- Moved Gameserver Patch into its own dashboard page directly below Filter Settings, so GameServer options are no longer mixed with Filter configuration sections.
- Kept all GameServer options grouped on the dedicated page with clear descriptions, supported value checks, and the required restart notice.
- Requirement: replace the Filter desktop package and restart the desktop application. The existing v3.0.6 SQL update and matching GameServer and ShardManager add-ons remain required for the underlying server fixes. No new SQL, client DLL, or media update is required.

## Update v3.0.6

Release date: 2026-08-10

- Added a clear Gameserver Patch section under Filter Settings with customer-friendly names, explanations, supported value limits, and a permanent GameServer restart notice.
- Consolidated customer-editable GameServer options into one database settings catalog, including durability, Green Book, and Party Monster controls.
- Corrected the PK drop-penalty level, Astral activation level, Astral recovery level, and negative guild-point protection so their saved values now control the intended server behavior.
- Removed duplicate and retired settings that could appear editable while having no effect.
- Requirement: stop the Filter, ShardManager, and all GameServers; back up the KMTGuard database; apply the included SQL update; replace the Admin Desktop, GameServer add-on, and ShardManager add-on; then restart all services. No client DLL or media update is required.

## Update v3.0.5

Release date: 2026-08-10

- Secured live Gold and item commands against invalid amounts, unsafe slots, mismatched items, overflow, insufficient balances, and partial item changes.
- Added durable GameServer command claiming and explicit delivery results so concurrent ShardManagers cannot run the same command and uncertain commands are not replayed.
- Corrected live teleport validation and destination selection, including world, layer, region, coordinate, and radius boundaries.
- Corrected Free For All combat so it is active only during the configured fight phase and only for eligible players inside the event area.
- Corrected Unique damage ranking to display the highest eight players consistently, including large damage values and tied scores.
- Added safe Fortress damage ranking for the highest eight guilds while preserving normal structure damage behavior.
- Improved GameServer hook startup, shutdown, error handling, and compatibility checks.
- Changed attack restriction configuration to load once when the GameServer starts instead of repeatedly querying the database during runtime refreshes.
- Requirement: stop the Filter, ShardManager, and all GameServers; back up the KMTGuard and shard databases; apply the included SQL update; replace the Filter, GameServer add-on, and ShardManager add-on; then restart all services. No client DLL or media update is required.

## Update v3.0.4

Release date: 2026-08-10

- Rebuilt PvP Challenge match recovery so accepted wagers are refunded safely after a Filter restart or a failure before the fight starts.
- Made wins, forfeits, draws, and refunds transaction-safe and kept live player Gold synchronized with completed settlements.
- Awarded the opponent when a player disconnects or leaves the assigned arena, while rejecting kills reported outside that arena.
- Prevented cape changes, teleports, Reverse/Return travel, exchanges, stalls, and early attacks from bypassing an active match or its opening freeze.
- Confirmed both players reached the assigned arena before starting the fight and refunded both wagers when arena entry fails.
- Corrected the challenge window to prevent duplicate submissions while a request is awaiting confirmation.
- Corrected the database update to use the current teleport procedure names and to stop with a clear failure if those procedures are not installed successfully.
- Added a self-contained repair update for installations where the earlier PvP Challenge SQL was only partially applied because it targeted legacy teleport object names.
- Requirement: stop the Agent Filter and all GameServers; back up the KMTGuard and shard databases; apply the included SQL update; replace the Filter, GameServer add-on, and client DLL; include the updated Filter language files; then start the GameServers and Agent Filter. No ShardManager replacement or media update is required.
- Rollback: restore the previous matching Filter, GameServer add-on, and client DLL together. The database naming correction may remain installed, but do not run an older Filter against unfinished PvP Challenge matches.

## Update v3.0.3

Release date: 2026-08-09

- Restored managed Clientless account login while preserving signed device verification for normal players and remote connections.
- Kept bot-login policy, account credential checks, and system-account restrictions enforced for Clientless sessions.
- Requirement: replace the Filter and restart the Filter services. No SQL update, client DLL replacement, GameServer replacement, ShardManager replacement, or media update is required.

## Update v3.0.2

Release date: 2026-08-09

- Corrected Auto Event rules so Lucky Party accepts only successful Party Matching registrations, Retype requires matching letter case, and Alchemy always uses the target plus selected in the dashboard.
- Confirmed that closing a stall after opening it during a Lucky Stall event does not remove the player's valid entry.
- Closed completed Auto Event rounds against late answers and preserved valid simultaneous answers instead of dropping them during winner selection.
- Processed successful Alchemy attempts immediately for event ranking instead of allowing unrelated database logging time to change the first winner.
- Updated the Lucky Party announcement to describe the required Party Matching action accurately.
- Requirement: replace the Filter, include the updated Filter language files, and restart the Agent Filter service. No SQL update, client DLL replacement, GameServer replacement, ShardManager replacement, or media update is required.

## Update v3.0.1

Release date: 2026-08-09

- Customer packages now refresh the latest licensed production files automatically before creating a new customer or generating a new package for an existing customer.
- Package generation stops safely if the latest release cannot be prepared, preventing delivery of outdated or incomplete files.
- Requirement: replace the License Center application. No Filter, client DLL, GameServer, ShardManager, media, or SQL update is required for existing customers.

## Update v3.0.0

Release date: 2026-08-09

- Hardened packet processing against oversized, overlapping, incomplete, and malformed massive packets.
- Replaced device verification with a signed, one-time device proof and rejected outdated client components.
- Rebuilt Secondary Password authentication to preserve leading zeroes, enforce the correct login order, persist attempt blocking across reconnects, and fail safely during database outages.
- Removed free-form database command execution and added durable command claiming, retry, dead-letter, and duplicate-effect protection.
- Protected saved Quick Login accounts with authenticated encryption, short-lived session-bound requests, bounded rate limiting, and secure node credentials outside SQL.
- Added bounded connection deadlines, per-service session caps, TCP keepalive, resilient listener health handling, safer shutdown cleanup, and restartable database workers.
- Made shared runtime caches reload atomically and corrected traffic and shared-IP protection accounting.
- Updated the bundled licensing database dependency to remove a known high-severity security issue.
- Separated GameServer-owned settings from Filter settings and made invalid or unknown Filter configuration stop startup safely.
- Corrected startup compatibility for the internal packet security setting and databases that still contain the retired Offline Stall confirmation setting.
- Restored compatibility with existing NPC mapping data containing reused runtime identifiers so Filter startup no longer stops on those rows.
- Removed the separate Quick Login credential setup prerequisite; compatible installations now prepare and retain the required key automatically in Settings.json.
- Corrected the setup dashboard so valid VIP and internal security settings are no longer reported as invalid configuration.
- Restored startup compatibility with customer-specific and legacy settings while keeping strict validation for known Filter settings and their values.
- Removed database schema changes from normal Filter startup; the runtime now validates the installed database package without requiring migration-level permissions.
- Updated the SQL client driver and removed deprecated transitive identity packages.
- Requirement: stop login traffic; back up the database and current configuration; apply the included SQL update; enter the customer's selected SQL username and password in Settings.json; provision the Quick Login key; replace the Filter and client DLL; install the updated media/language files; then restart the Filter. All players must use the new client DLL.
- Existing saved Quick Login entries must be saved again after the update. No GameServer or ShardManager binary replacement is required.
- Rollback: restore the previous matching Filter and client DLL together. The additive SQL update may remain installed, but legacy Quick Login entries stay revoked.

## Update v2.13.11

Release date: 2026-08-09

- Corrected the Silk Stall database update so it runs successfully on supported SQL Server installations and remains safe to run more than once.
- Added a self-contained repair update for installations where the previous Silk Stall SQL attempt stopped with a syntax error.
- Requirement: apply the included SQL update. No Filter restart, GameServer restart, Filter replacement, GameServer replacement, ShardManager replacement, client DLL replacement, or media update is required.

## Update v2.13.10

Release date: 2026-08-09

- Added a clear KMTGuard readiness stamp to the native GameServer and ShardManager status windows after each service completes startup successfully.
- The readiness stamp shows the installed KMTGuard version and confirms that the corresponding server add-on is active, without adding recurring log noise.
- Requirement: replace the GameServer and ShardManager add-ons and restart both services. No SQL update, Filter replacement, client DLL replacement, or media update is required.

## Update v2.13.9

Release date: 2026-08-09

- Replaced the noisy ShardManager add-on output with a compact operational console that highlights startup progress, readiness, warnings, and failures while retaining a bounded diagnostic log.
- Hardened ShardManager startup so incompatible server files, unavailable database settings, failed worker creation, or incomplete runtime integration stop safely instead of leaving a partially active add-on.
- Rejected malformed internal routing messages safely and prevented failed or incomplete GameServer command broadcasts from being removed prematurely.
- Improved database connection cleanup, query timeouts, scheduled-command handling, unique-history processing, and runtime patch rollback.
- Requirement: replace the ShardManager add-on and restart ShardManager. No SQL update, Filter replacement, GameServer replacement, client DLL replacement, or media update is required.

## Update v2.13.8

Release date: 2026-08-09

- Hardened GameServer startup so missing protection data, incompatible server files, or failed runtime integration stops the process safely instead of running partially protected.
- Secured Filter-to-GameServer internal commands and rejected invalid, expired, duplicated, or client-originated registration attempts.
- Made item locking database-authoritative and transaction-safe so failed or repeated requests no longer consume a scroll or leave GameServers with conflicting lock state.
- Fixed fixed-value resurrection, attack restrictions, damage summaries, custom reverse movement, pet skills, monster spawning, mass removal, and kill logging for invalid or boundary inputs.
- Added bounded custom-packet writes, safer database cleanup, automatic security-data refresh, and operational health counters.
- Requirement: stop the Filter, every GameServer, and ShardManager; apply the included SQL update; replace all three server components; then start ShardManager, the GameServers, and finally the Filter. The Filter and GameServer from this update must be deployed together. No client DLL or media replacement is required.
- Rollback: stop the new server group and restore the previous Filter, GameServer, and ShardManager binaries together. The additive SQL update may remain installed.

## Update v2.13.7

Release date: 2026-08-09

- Fixed Offline Stall item tracking so rejected changes and completed purchases can no longer leave stale or missing shop slots.
- Enabled Silk Stall purchases to complete while the seller is using Offline Stall, with automatic closure after the final item is sold.
- Hardened authenticated login replacement, database timeouts, shutdown handling, connection limits, and licensed-player capacity while Offline Stalls remain connected.
- Added localized Offline Stall activation results and removed the retired login-confirmation database setting and packet authorization.
- Added a database integrity update that migrates legacy Offline Stall history to the active schema and validates the required settings and tables.
- Requirement: apply the included SQL update, replace the Filter, and restart every Filter service. No client DLL, GameServer add-on, or media update is required.

## Update v2.13.4

Release date: 2026-08-08

- Fixed Silk Stall purchases so the buyer is charged only Silk and neither the buyer nor seller loses or receives Gold.
- Added durable Silk reservations, automatic refunds for interrupted purchases, automatic completion of pending seller payments, duplicate-purchase protection, and safe balance-overflow checks.
- Fixed sold Silk Stall slots remaining available and aligned the transaction table used by new and existing installations.
- Requirement: apply the included SQL update, replace the Filter and GameServer add-on, then restart the Filter and every GameServer. No client DLL, ShardManager, or media update is required.

## Update v2.13.3

Release date: 2026-08-08

- Offline Stalls now close automatically when their owner starts an authenticated login, eliminating the confirmation prompt and continuing login automatically.
- Requirement: replace the Filter and restart it. No client DLL, GameServer, ShardManager, SQL, or media update is required.

## Update v2.13.2

Release date: 2026-08-08

- Hid repetitive custom-package price registration notices from the GameServer add-on console.
- Prevented normal GameServer system messages from being mislabeled as add-on initialization failures or changing the console status to Failed.
- Requirement: replace the GameServer add-on and restart every GameServer. No Filter, ShardManager, client DLL, SQL, or media update is required.

## Update v2.13.1

Release date: 2026-08-08

- Improved developer workspace cleanup so all generated client and GameServer build directories, build-check outputs, and temporary root object files are removed consistently.
- Requirement: no server restart, Filter restart, SQL update, client DLL replacement, media update, or customer action is required.

## Update v2.13.0

Release date: 2026-08-08

- Made GameServer startup changes transactional so a failed hook or runtime setting restores all earlier changes instead of leaving the process partially initialized.
- Added strict validation for database-backed GameServer limits, probabilities, reward identifiers, and party-monster settings before any runtime change is applied.
- Added automatic GameServer crash dumps and a companion crash summary to make fatal failures diagnosable instead of silent.
- Hardened GameServer database loading against invalid, truncated, empty, or partially read setting rows and added database latency and failure telemetry.
- Added safe rollback support for combat, logging, region, durability, party-monster, and Green Book runtime changes when initialization cannot complete.
- Requirement: replace the GameServer add-on and restart every GameServer. No Filter, ShardManager, client DLL, SQL, or media update is required.

## Update v2.12.1

Release date: 2026-08-08

- Added maintenance access for connections whose IP address is listed in the existing GM IP authorization table while the public server status remains Check for everyone else.
- Requirement: replace the Filter and restart the Gateway Filter service. No SQL, client DLL, GameServer add-on, or media update is required.

## Update v2.12.0

Release date: 2026-08-08

- Hardened GameServer packet handling against malformed, truncated, and invalid messages while preserving the original game protocol.
- Prevented stale reconnect sessions from invalidating a newer character session and improved cleanup safety during rapid reconnects.
- Added strict GameServer compatibility validation so an unsupported or modified server build stops add-on initialization safely instead of applying incompatible runtime changes.
- Reduced repeated combat work and temporary allocations in Live DPS and unique-monster restriction processing.
- Added operational telemetry for CPU, memory, handles, packet rate, packet latency, game-loop delay, queue-timer work, and active cache sizes with bounded log rotation.
- Replaced the noisy GameServer add-on startup output with a compact status console that keeps important readiness and failure messages visible.
- Fully removed the remaining executable helper from the retired custom New Alchemy path; the original game alchemy remains unchanged.
- Requirement: replace the GameServer add-on and restart every GameServer. No Filter, ShardManager, client DLL, SQL, or media update is required.

## Update v2.11.2

Release date: 2026-08-08

- Fixed the Offline Stall login confirmation so the Disconnect and Cancel buttons respond correctly on the login and character-selection screens.
- Requirement: replace the client DLL and fully restart the game client. No Filter restart, GameServer update, SQL update, or media update is required.

## Update v2.11.1

Release date: 2026-08-08

- Fixed an immediate GameServer startup crash introduced by the v2.11.0 telemetry update.
- Requirement: replace the GameServer add-on and restart every GameServer. No Filter, ShardManager, client DLL, SQL, or media update is required.

## Update v2.11.0

Release date: 2026-08-08

- Removed the custom Honor Rank runtime refresh system from the GameServer, ShardManager, client, command queue, and supporting database objects without changing the original game Honor Rank system.
- Hardened custom GameServer packet parsing against truncated data, invalid string lengths, invalid inventory slots, failed message allocation, and missing runtime objects.
- Fixed party-based monster restrictions to use the player's live party state and removed duplicate attack-packet inspection from the combat path.
- Fixed Filter session key cleanup when characters disconnect and removed the full online-player scan previously triggered during login.
- Batched Live DPS updates on the GameServer tick and bounded the pending unique-monster cache to reduce repeated work during heavy combat.
- Added GameServer telemetry for tick delay, packet rates, malformed packets, handler latency, and active cache sizes, plus a repeatable synthetic load test.
- Requirement: apply the v2.11.0 SQL update, replace the GameServer and ShardManager add-ons and client DLL, then restart every GameServer, the ShardManager, and all game clients. No Filter or media update is required.

## Update v2.10.7

Release date: 2026-08-08

- Prevented long-running GameServers from retaining stale unique-combat cache entries after monsters disappear without a normal kill.
- Removed temporary heap allocations and extra payload copies from custom item display and item-link message processing.
- Fixed a monster-death stability issue that could access a monster after the original GameServer death handler had released it.
- Reduced GameServer startup database round-trips by synchronizing NPC runtime identifiers in bounded batches.
- Serialized custom GameServer database operations safely and made missing startup settings deterministic instead of leaving undefined values.
- Requirement: replace the GameServer add-on and restart every GameServer. No Filter, client DLL, SQL, or media update is required.

## Update v2.10.6

Release date: 2026-08-08

- Reduced GameServer CPU spikes during high-rate unique combat by limiting repeated restriction cleanup and player lookups.
- Reduced GameServer memory overhead for locked-item and hidden-region indexes while keeping live updates synchronized safely.
- Fully excluded the retired custom New Alchemy and timed-plus runtime paths from GameServer processing; the original game alchemy remains unchanged.
- Fixed packet cloning so copied messages contain their full payload and never return an uninitialized message during fan-out.
- Improved native build consistency by removing duplicate source compilation that could select conflicting implementations.
- Requirement: replace the GameServer add-on and restart every GameServer. No Filter, client DLL, SQL, or media update is required.

## Update v2.10.5

Release date: 2026-08-08

- Added fast player-session lookups for character IDs, names, and runtime unique IDs across events, commands, and packet handling.
- Reduced temporary memory allocations by removing unnecessary full-session snapshots from broadcasts, connection limits, online counts, and event processing.
- Improved Live DPS and unique-kill processing so player names are resolved only for participants contained in each update.
- Prevented stopped sessions from affecting active IP and hardware limit counts while they are being removed.
- Added automatic stale-index validation and cleanup when characters change or sessions disconnect.
- Requirement: replace the Filter and restart it. No SQL, client DLL, GameServer add-on, or media update is required.

## Update v2.10.4

Release date: 2026-08-08

- Removed repeated clientless account database lookups from the player heartbeat path while preserving managed and system account policies.
- Reduced region and automatic PvP database pressure during mass movement, teleports, and simultaneous player heartbeats.
- Reduced idle automatic-event schedule polling while keeping queued administration commands responsive.
- Excluded disconnected, detached, and not-yet-ready sessions from client broadcasts to reduce unnecessary packet fan-out.
- Bounded temporary account, region, and event-team cache data during long Filter uptimes.
- Requirement: replace the Filter and restart it. No SQL, client DLL, GameServer add-on, or media update is required.

## Update v2.10.3

Release date: 2026-08-08

- Reduced idle Filter CPU usage by replacing frequent delayed-job and clientless socket polling with event-driven waits.
- Improved Offline Stall scalability by using one shared heartbeat worker and batching its database heartbeat updates.
- Removed duplicate unthrottled Live DPS updates and reduced player lookup work for each remaining snapshot.
- Reduced unnecessary database polling while the teleport command queue is idle or not installed.
- Prevented temporary teleport, region-notice, and self-teleport cooldown data from growing throughout long server uptimes.
- Requirement: replace the Filter and GameServer add-on, then restart the Filter and every GameServer. No client DLL, SQL, or media update is required.

## Update v2.10.2

Release date: 2026-08-08

- Stabilized all automatic events, including text, survival, competitive, and Hide and Seek events, from registration through rewards and cleanup.
- Prevented duplicate kill packets from granting repeated scores or per-kill rewards, and prevented late or failed arena registrations from remaining eligible.
- Daily event schedules now wait for an active event to finish instead of being lost for the day, while interrupted command-queue work is recovered automatically.
- Event stopping and Filter shutdown now complete player return, round finalization, and audit recording reliably without duplicate completion results.
- Hide and Seek now provisions its event character on first use, grants the full configured search time after positioning, and records its standard round and winner history.
- Reward delivery now rejects invalid amounts, isolates individual delivery failures, and continues processing the remaining configured rewards and players.
- Requirement: replace the complete Filter package, including its language files, then restart the Filter. No SQL, client DLL, GameServer add-on, or media update is required.

## Update v2.10.1

Release date: 2026-08-08

- Retired the custom New Alchemy system while preserving the original game alchemy system.
- Removed the custom interface from supported clients and blocked its requests and results at both the Filter and GameServer boundaries.
- Prevented the retired system from performing direct item database updates or triggering alchemy success integrations.
- Requirement: replace the Filter, client DLL, and GameServer add-on, then restart the Filter, every GameServer, and all game clients. No SQL or media update is required.

## Update v2.10.0

Release date: 2026-08-08

- Added Alexandria North (SD) as a separate city option when creating clientless accounts.
- Clientless characters are now distributed across multiple distant locations throughout every supported city, with substantially larger spacing between nearby characters.
- Requirement: replace the Admin Desktop, then recreate the desired clientless accounts to apply the new city and spawn distribution. No Filter, client DLL, GameServer, SQL, or media update is required.

## Update v2.9.4

Release date: 2026-08-08

- Fixed Macro Bot weapon and shield switching so shield-required skills complete before the configured attack weapon is restored.
- Bow and crossbow characters now restore their live ammunition after temporary shield use, allowing Auto Attack and Auto Skill to resume normally.
- Macro equipment follows items after inventory swaps and keeps valid targets across region-coordinate checks.
- Requirement: replace the client DLL and fully restart the game client. No Filter restart, GameServer update, SQL update, or media update is required.

## Update v2.9.3

Release date: 2026-08-05

- Synchronized the customer package, product binaries, and Update Center release version so newly generated customer packages start at the current published release.
- Requirement: rebuild and replace the customer package. Existing customer installations do not require a database update from this version-only synchronization.

## Update v2.7.20

Release date: 2026-08-05

- Added an enable/disable button to the Admin Desktop VIP System page.
- Disabling VIP System prevents new VIP spend processing and VIP rank handling after the Filter is restarted, while preserving the saved VIP tiers and ranking data.
- Requirement: apply the v2.7.20 SQL update, replace the Filter and Admin Desktop, then restart the Filter. No client DLL, GameServer add-on, or media update is required.

## Update v2.7.19

Release date: 2026-08-05

- Added a database-controlled option in the Admin Desktop to disable Green Book restrictions across all GameServers.
- The original Green Book behavior remains enabled by default until the new option is turned on.
- Requirement: apply the v2.7.19 SQL update, replace the GameServer add-on and Admin Desktop, then restart every GameServer. No Filter, client DLL, or media update is required.

## Update v2.7.18

Release date: 2026-08-05

- Updated the game window title branding to KMTGuard on the login screen and beside the active character name.
- Requirement: replace the client DLL and restart the game client. No launcher, Filter, GameServer, SQL, or media update is required.

## Update v2.7.17

Release date: 2026-08-04

- Fixed a client crash that could occur while the login interface was being created immediately after startup.
- Client feature components are now fully prepared before the game can activate their interface integrations.
- Kept the corrected launcher and external-loader startup behavior unchanged.
- Requirement: replace the client DLL and fully close all launchers, loaders, and game clients before starting again. No launcher, Filter, GameServer, SQL, or media update is required.

## Update v2.7.16

Release date: 2026-08-04

- Restored complete in-game client features after package validation while preserving the corrected launcher startup behavior.
- Kept external-loader startup independent from launcher synchronization and retained detailed startup diagnostics for troubleshooting.
- Requirement: replace the client DLL and fully close all launchers, loaders, and game clients before starting again. No launcher, Filter, GameServer, SQL, or media update is required.

## Update v2.7.15

Release date: 2026-08-04

- Corrected the client-thread startup dispatch used after external-loader launches so the complete client extension can continue initialization once the client's message queue is active.
- Kept launcher synchronization and external-loader startup behavior unchanged.
- Requirement: replace the client DLL and fully close all launchers, loaders, and game clients before starting again. No Filter, GameServer, SQL, or media update is required.

## Update v2.7.14

Release date: 2026-08-04

- Restored the complete client extension after the startup diagnostic, while keeping external loader startup independent from client synchronization objects.
- Client hook and interface initialization now runs once on the client thread after its message loop is available, reducing startup races during external-loader launches.
- Requirement: replace the client DLL and fully close all launchers, loaders, and game clients before starting again. No Filter, GameServer, SQL, or media update is required.

## Update v2.7.13

Release date: 2026-08-04

- Added a temporary client diagnostic mode that validates the client package while leaving all game and interface hooks disabled, allowing startup failures to be isolated safely.
- Removed client-extension changes to the game's instance synchronization while startup diagnosis is in progress.
- Expanded file-based startup diagnostics to identify the exact hook operation, address, and exception when hook initialization is re-enabled for staged testing.
- Corrected the supplied legacy launcher so an existing Ready synchronization object no longer displays a critical error or closes the launcher, while leaving the remaining launcher startup flow unchanged.
- Requirement: replace the client DLL and launcher, then fully close all launchers, loaders, and game clients before testing. This diagnostic build intentionally leaves KMTGuard client interface features disabled. No Filter, GameServer, SQL, or media update is required.

## Update v2.7.12

Release date: 2026-08-04

- Restored reliable game startup through the normal launcher and supported external loaders by separating client-extension initialization from the launcher's synchronization sequence.
- Multiple game clients can now run simultaneously without weakening the launcher's normal client-start handshake.
- Added an early file-based startup record to assist diagnosis when a client cannot finish loading.
- Requirement: replace the client DLL and fully close all launchers, loaders, and game clients before starting again. No launcher, Filter, GameServer, SQL, or media update is required.

## Update v2.7.11

Release date: 2026-08-04

- Restored normal launcher and edxSilkroadLoader startup by allowing the game client to complete its original launcher handshake before the KMTGuard client extension starts.
- Removed the client startup breakpoint that could prevent supported external loaders from creating the game process.
- KMTGuard client initialization now runs at a deterministic point before the game interface loads, preventing the earlier startup race without delaying the launcher handshake.
- Requirement: replace the client DLL and fully close all launchers, loaders, and game clients before starting again. No launcher, Filter, GameServer, SQL, or media update is required.

## Update v2.7.10

Release date: 2026-08-04

- Removed the KMTGuard client-side conflict that could show a duplicate-instance error or interfere with a supported external loader.
- Eliminated the early-startup race captured in client crash dumps by completing external-loader and KMTGuard initialization before the game begins loading its interface.
- An incomplete external-loader startup now stops safely instead of allowing partially initialized client components to continue.
- Restricted client initialization to `sro_client.exe` so loading the DLL from another executable cannot change that process.
- Requirement: replace the client DLL and fully close all launchers, loaders, and game clients before starting again. No Filter restart, SQL update, GameServer update, or media update is required.

## Update v2.7.9

Release date: 2026-08-04

- Removed the recurring Mutex Critical startup failure for direct launches, normal launchers, and third-party loaders.
- Corrected repeated edxSilkroadLoader launches and multiclient startup so each game process waits for its own loader initialization instead of reusing another client's ready state.
- Prevented client hooks from being installed in the middle of game initialization, which could crash the client immediately after its window opened.
- Requirement: replace the client DLL and fully restart the launcher and all game clients. No Filter restart, SQL update, GameServer update, or media update is required.

## Update v2.7.8

Release date: 2026-08-04

- Restored edxSilkroadLoader client startup by coordinating third-party loader initialization before the KMTGuard client extension starts.
- Added persistent client startup and crash diagnostics so early client failures leave actionable records instead of disappearing with a temporary console.
- Normal launcher startup remains unchanged when no supported external loader is present.
- Requirement: replace the client DLL and fully restart the game client. No Filter restart, SQL update, GameServer update, or media update is required.

## Update v2.7.7

Release date: 2026-08-03

- Restored SBot, Mbot, and loader-based client startup when Allow Bot Login is enabled, including successful character selection before the game client opens.
- Allowed bot sessions remain distinctly identified and continue to follow configured login limits, bot-restricted regions, and bot trade restrictions.
- Disabling Allow Bot Login continues to reject unverified bot sessions and disconnects already connected bot sessions safely.
- Requirement: replace and restart the Filter. No SQL update, GameServer update, client DLL, or media update is required.

## Update v2.7.6

Release date: 2026-08-03

- Player resurrection after a kill in Survival Party, Survival Solo, Madness, Defend the Tower, and Last Man Standing now waits 10 seconds so the defeated player's client can finish entering the death state before revival.
- Delayed resurrection is cancelled safely when the event round ends or the player is no longer participating, while end-of-event recovery remains immediate.
- Requirement: replace and restart the Filter. No SQL update, GameServer update, client DLL, or media update is required.

## Update v2.7.5

Release date: 2026-08-03

- Corrected the Pick Inventory O shortcut so it uses the stable client keyboard path and the same key press cannot be processed again after the custom window opens or closes.
- Repaired VIP Silk-spend processing after the player-style upgrade and automatically requeues previously failed VIP spend records.
- The VIP database update now restores and enables the global Silk-spend tracker when it is missing from a customer Account database.
- VIP processing now keeps a durable, duplicate-safe spend and rank-change history, enforces one current rank row per character, and exposes an ordered player ranking in the Control Center.
- Failed VIP events continue retrying at a safe interval after repeated errors, allowing processing to recover after a database dependency is corrected.
- Player Commands now scrolls vertically so all command groups and controls remain reachable on smaller dashboard window sizes.
- Requirement: apply the v2.7.5 SQL update first, replace and restart the Filter and Admin Desktop, and replace the client DLL before fully restarting the game client. No GameServer or media update is required.

## Update v2.7.4

Release date: 2026-08-03

- Restored normal Escape-key behavior for the game's original windows and exit menu when no KMTGuard custom window is being closed, while preventing the same key press from opening the exit menu after closing a KMTGuard window.
- Restored the login server-list status after the in-game Restart flow so the server no longer remains displayed in Check state until the client is fully closed and reopened.
- Prevented the launcher startup mutex warning that could appear before pressing Play when launching from a KMTGuard-enabled client folder.
- Stabilized the O shortcut so the ground-item pickup inventory opens and closes through a single keyboard path.
- Requirement: replace the client DLL and LyonSroLauncher executable, then fully restart the launcher and game client. No Filter restart, SQL update, GameServer update, or media update is required.

## Update v2.7.3

Release date: 2026-08-02

- Restored the Pick Inventory shortcut so pressing O opens and closes the ground-item pickup inventory window again.
- Corrected the local development client DLL package so it starts without production license binding.
- Requirement: replace the client DLL and fully restart the game client. No Filter restart, SQL update, GameServer update, or media update is required.

## Update v2.7.2

Release date: 2026-08-02

- Restored the original mouse keypad for Secondary Password entry so number, clear, delete, confirm, cancel, and change actions use the client’s normal button path.
- Requirement: replace the client DLL and client media, then fully restart the game client. No Filter restart, SQL update, or GameServer update is required.

## Update v2.7.1

Release date: 2026-08-01

- Split Bot Operations into separate icon-based pages for runtime control, account provisioning, account management, and Party Matching.
- Removed the combined Clientless tab layout and now shows shared account defaults only where they are required.
- Corrected Party Monster controls for the verified vSRO 188 GameServer variant while retaining safe rejection for incompatible builds.
- Requirement: replace and restart the Admin Desktop and GameServer add-on. Apply the v2.7.0 SQL update first if it has not already been installed. No Agent service restart, client DLL, or media update is required.

## Update v2.7.0

Release date: 2026-08-01

- Added dashboard controls to enable or disable Party Monster spawning, select the minimum required party-member count, and set the eligible spawn chance from 0% to 100%.
- Disabling the feature keeps its configured member count and spawn rate saved for later use, while normal monster spawning continues unchanged.
- Invalid setting values are rejected by the dashboard and corrected safely by the GameServer if the database is edited manually.
- Party Monster controls support the verified vSRO 188 GameServer instruction variants and reject incompatible builds without applying a partial change.
- Corrected the Clientless Control Center layout so shared account defaults and workflow tabs remain separated and usable at supported window sizes.
- Requirement: apply the v2.7.0 SQL update, replace the Admin Desktop and GameServer add-on files, then restart the Admin Desktop and every GameServer. No Filter, client DLL, or media update is required.

## Update v2.6.21

Release date: 2026-08-01

- Unique History now moves the field-map viewport through the client's native map-pan behavior after selecting and initializing the correct map page.
- Corrected the selected death position's map scale and removed the navigation path that could leave the view centered on the player.
- Requirement: replace the client DLL and fully restart the game client. No Filter restart, SQL update, GameServer update, or media update is required.

## Update v2.6.20

Release date: 2026-08-01

- Unique History field-map navigation now follows the verified client map-opening sequence, so selecting a death record initializes the correct field page before moving to its marker.
- Restored the normal world-map lifecycle and removed the delayed navigation used by the previous correction attempts.
- Requirement: replace the client DLL and fully restart the game client. No Filter restart, SQL update, GameServer update, or media update is required.

## Update v2.6.19

Release date: 2026-08-01

- Restored normal world-map behavior and corrected Unique History centering so a selected field-map death marker opens in the visible map area instead of the player's current location.
- Selecting another Unique History entry now navigates the map using the selected marker's correct map position.
- Requirement: replace the client DLL and fully restart the game client. No Filter restart, SQL update, GameServer update, or media update is required.

## Update v2.6.18

Release date: 2026-08-01

- Corrected Unique History world-map navigation so the map viewport itself moves to the selected death marker instead of remaining centered on the player.
- The selected marker is placed at the center of the visible map area without switching to an unrelated map page.
- Requirement: replace the client DLL and fully restart the game client. No Filter restart, SQL update, GameServer update, or media update is required.

## Update v2.6.17

Release date: 2026-08-01

- Unique History now opens normal world maps centered on the selected unique's recorded death marker instead of the player's current position.
- Selecting another unique moves both the marker and the map view to the newly selected location.
- Requirement: replace the client DLL and fully restart the game client. No Filter restart, SQL update, GameServer update, or media update is required.

## Update v2.6.16

Release date: 2026-08-01

- Added a customer-controlled `DisableDurability` setting that prevents weapon and equipment durability from decreasing when enabled.
- The setting is disabled by default, and incompatible GameServer versions are rejected safely instead of receiving an unsupported runtime patch.
- Requirement: apply the v2.6.16 SQL update, replace the Filter, Admin Desktop, and GameServer add-on files, then restart the Filter, dashboard, and every GameServer. No client DLL or media update is required.

## Update v2.6.15

Release date: 2026-07-31

- Quick Logon now keeps the Filter's enabled or disabled setting when the game uses its internal Restart flow.
- Rebuilding the login scene no longer forces a disabled Quick Logon window to appear; a fresh client start waits for the Filter setting before showing it.
- Requirement: replace the client DLL and fully restart the game client. No Filter restart, Admin Desktop, media, GameServer, or SQL update is required.

## Update v2.6.14

Release date: 2026-07-31

- Added a Player Notice Language selector to the Admin Desktop Dashboard for switching Filter-generated notices between English, Turkish, and deployed custom languages without editing JSON manually.
- Language changes now apply immediately to every running Filter service and are saved for future starts; invalid or missing language files are rejected without replacing the active language.
- Admin Desktop settings saves now preserve the selected Filter language.
- Requirement: replace the Filter and Admin Desktop files, then start both normally. No Filter restart is required for later Dashboard language changes, and no client DLL, media, GameServer, or SQL update is required.

## Update v2.6.13

Release date: 2026-07-31

- Unique History now opens its recorded marker through the normal world-map lifecycle instead of forcing the unrelated Entire Map page.
- Escape now closes one custom game window without opening the exit menu, while pressing Escape with no custom window open continues to show the original exit menu normally.
- Requirement: replace the client DLL and fully restart the game client. No Filter restart, SQL update, GameServer update, or media update is required.

## Update v2.6.12

Release date: 2026-07-31

- Unique History now waits for the requested map page to finish loading before focusing it, so selecting a dead unique opens the area that contains its already-recorded death marker.
- Closing a custom window with Escape no longer opens the normal Escape menu from the same key press.
- Requirement: replace the client DLL and fully restart the game client. No Filter restart, SQL update, GameServer update, or media update is required.

## Update v2.6.11

Release date: 2026-07-31

- Auto Equip now runs immediately when its guide icon is selected, without the intermediate confirmation window.
- Added a configurable maximum Auto Equip level; characters above the configured level no longer see the icon and cannot invoke the operation.
- Auto Equip database integrations now receive the authenticated character level together with the character ID, with duplicate in-progress requests rejected safely.
- Restored Escape-key closing for custom game windows through the main game input path, while preserving each window's normal close cleanup.
- Unique History selections now render a visible location marker using the stable map-location renderer on both world and instance maps.
- Requirement: apply the v2.6.11 SQL update, replace the Filter, client DLL, and Admin Desktop files, then restart the Filter, dashboard, and game client. No GameServer or media update is required.

## Update v2.6.10

Release date: 2026-07-31

- Corrected Unique History map selection so runtime instance layers no longer change the recorded world or send markers to unrelated maps.
- Existing Unique History rows are corrected during the database update and are also normalized safely when the Filter loads them.
- Requirement: apply the v2.6.10 SQL update, replace the Filter files, and restart the Filter. Keep the v2.6.9 or newer client DLL installed. No GameServer or media update is required.

## Update v2.6.9

Release date: 2026-07-31

- Custom game windows now close consistently with the Escape key while preserving each window's normal cleanup behavior.
- Restored both the close button and Escape-key closing behavior for the Alchemy Macro window, including safe cancellation of active automation.
- Requirement: replace the client DLL and fully restart the game client. No Filter restart, SQL update, GameServer update, or media update is required.

## Update v2.6.8

Release date: 2026-07-31

- Corrected Unique History markers inside dungeon and instance maps so they point to the recorded kill position.
- Requirement: replace the client DLL and fully restart the game client. No Filter restart, SQL update, GameServer update, or media update is required.

## Update v2.6.7

Release date: 2026-07-31

- Restored the O keyboard shortcut for opening and closing the automatic pickup inventory.
- Requirement: replace the client DLL and fully restart the game client. No Filter restart, SQL update, or media update is required.

## Update v2.6.6

Release date: 2026-07-31

- Rebuilt Unique History location tracking so kills retain precise coordinates and open the correct world or instance map instead of displaying a stale or unrelated location.
- Unique History records now survive Filter restarts and keep their status, killer, location, and damage ranking intact.
- Corrected damage rankings for more than eight attackers, preserved descending damage order, and fixed the eighth-place label.
- Live updates now preserve the current page, clear stale selections safely, and keep the map marker and damage panel synchronized with the selected record.
- Improved window behavior for empty rows, repeated open/close cycles, status colors, elapsed time, visible live-DPS control, and low-resolution panel placement.
- Requirement: apply the v2.6.6 SQL update, replace the Filter, GameServer add-on, client DLL, and supplied media file, then restart the Filter, GameServer, and game client.

## Update v2.6.5

Release date: 2026-07-31

- Restored the Runtime Actions workspace so service controls, module health, and runtime status appear below the command header without overlap or missing content.
- Restored the System Health, Database Updates, and Service Logs workspaces affected by the same layout issue.
- Prevented saved Auto Potion and Auto Hunt timers from carrying across the in-game Restart flow and disconnecting the same character during re-entry.
- Returning to character selection now clears previous-session automation state, reloads the selected character's settings, and waits for spawn, statistics, inventory, and equipment data before resuming automatically.
- Requirement: replace the Admin Desktop files and client DLL, restart the dashboard, and fully restart the game client. No Filter service restart, SQL update, or media update is required.

## Update v2.6.4

Release date: 2026-07-31

- Restored the complete Control Center sidebar after a layout layer obscured its navigation rail, workspace menu, search, and administrator status card.
- Sidebar navigation remains visible across Dashboard, setup, operations, event, content, and system workspaces after a full rebuild.
- Fixed a game client crash that could occur immediately after selecting a character when automatic potion settings were already active.
- Automatic potion checks now wait safely for character and pet health data to finish loading, then resume without requiring the feature to be disabled or reconfigured.
- Requirement: replace the Admin Desktop files and client DLL, restart the dashboard, and fully restart the game client. No Filter service restart, SQL update, or media update is required.

## Update v2.6.3

Release date: 2026-07-31

- Fixed Unique History refreshes so reopening the window always shows the current server state, including a correctly cleared empty list.
- Alive uniques now appear first, followed by the most recent history entries, and live status changes are reflected without leaving duplicate or stale rows.
- Corrected DPS detail delivery for history entries and improved stability when paging or selecting entries with incomplete data.
- Requirement: replace the Filter and client DLL, restart the KMTGuard Agent, and fully restart the game client. No SQL or media update is required.

## Update v2.6.2

Release date: 2026-07-30

- Reorganized the Clientless workspace into dedicated sections for runtime control, account creation, account management, account search, and Party Form settings.
- Party Form configuration is now presented in its own panel with a clear status area and guidance for the title, level range, purpose, and sharing options.
- Requirement: replace the Admin Desktop files and restart the dashboard. No Filter restart, SQL update, or client-file update is required.

## Update v2.6.1

Release date: 2026-07-30

- Improved automatic clientless Party Form registration by respecting the configured server level cap and retrying temporary AgentServer rejections after login.
- Party Form activity now reports the effective title, level range, settings, retry attempt, and the exact server error code when registration is refused.
- Requirement: replace the Filter files and restart the KMTGuard Agent. No manual SQL update or client-file update is required.

## Update v2.6.0

Release date: 2026-07-30

- Added dashboard controls that let administrators enable or disable automatic Party Matching forms for regular clientless characters.
- Party title, level range, purpose, and experience/item sharing can be configured from the Clientless Accounts workspace.
- Enabling the feature applies it immediately to online clientless characters and future logins; disabling it removes active managed forms and prevents new ones.
- Requirement: replace the Filter and Admin Desktop files, then restart the KMTGuard Agent and Admin Desktop. No manual SQL update or client-file update is required.

## Update v2.5.11

Release date: 2026-07-30

- Corrected the remaining account-screen caption issue so Register and Quick Login actions stay readable after returning from character selection.
- Quick Login data, saved accounts, account fields, and Secondary Password behavior remain unchanged.
- Requirement: replace the client DLL and fully restart the game client. No Filter restart, SQL update, or media update is required.

## Update v2.5.10

Release date: 2026-07-30

- Fixed button captions on the account screen after changing accounts, including Register and all Quick Login actions.
- Register now opens the account-registration window from the active login screen after changing accounts.
- Account fields, saved-account entries, and Secondary Password behavior remain unchanged.
- Requirement: replace the client DLL and fully restart the game client. No Filter restart, SQL update, or media update is required.

## Update v2.5.9

Release date: 2026-07-30

- Corrected the final login-screen refresh so Register and all visible Quick Login action labels remain readable after changing accounts.
- The saved-account panel, account fields, and Secondary Password behavior remain unchanged.
- Requirement: replace the client DLL and fully restart the game client. No Filter restart, SQL update, or media update is required.

## Update v2.5.8

Release date: 2026-07-30

- Fixed the Register action and Saved Accounts / Quick Login labels disappearing after returning to the login screen to change accounts.
- The login screen now restores its account actions and saved-account labels without requiring the game client to be closed and reopened.
- Secondary Password behavior is unchanged.
- Requirement: replace the client DLL and fully restart the game client. No Filter restart, SQL update, or media update is required.

## Update v2.5.7

Release date: 2026-07-30

- Corrected the underlying login-screen recovery so the account fields and actions return to their normal ready state after an incorrect account password or another rejected login.
- Players can dismiss the error, correct their details, and use Connect again without restarting the game.
- Secondary Password behavior is unchanged.
- Requirement: replace the client DLL and fully restart the game client. No Filter restart, SQL update, or media update is required.

## Update v2.5.6

Release date: 2026-07-30

- Corrected the login screen so account controls remain available after the game rejects an incorrect account password.
- Players can dismiss the login error, correct their account password, and connect again without restarting the game.
- Secondary Password behavior is unchanged.
- Requirement: replace the client DLL. No Filter restart, SQL update, or media update is required.

## Update v2.5.5

Release date: 2026-07-30

- Fixed the Secondary Password window becoming unresponsive after its title frame was clicked.
- The title, close control, password fields, and actions remain unchanged; the window now stays safely centered instead of entering an invalid drag state.
- Requirement: replace the client DLL. No Filter restart, SQL update, or media update is required.

## Update v2.5.4

Release date: 2026-07-30

- Fixed the login screen remaining disabled after an incorrect account password or another rejected account login.
- Players can correct their account details and try Connect again without closing and reopening the game.
- Requirement: replace the client DLL. No Filter restart, SQL update, or media update is required.

## Update v2.5.3

Release date: 2026-07-30

- Fixed login-screen account registration saving a password that could differ from the value typed into the masked password fields.
- Password and confirmation remain hidden as stars, and keyboard navigation with Tab or Shift+Tab remains available.
- Accounts created while affected are not changed automatically and should have their passwords reset or be recreated.
- Requirement: replace the client DLL. No Filter restart, SQL update, or media update is required.

## Update v2.5.2

Release date: 2026-07-30

- Fixed the procedure scheduler page failing to load when determining whether a scheduled job is currently running.
- Added compatibility handling so missing optional running-state data no longer prevents schedules from being displayed.
- Requirement: replace the Admin Desktop files. No Filter restart, SQL update, client DLL, or media update is required.

## Update v2.5.1

Release date: 2026-07-30

- Removed the unintended bright white borders from the Old School account registration, PvP Challenge, and Lucky Spin surfaces.
- Replaced the incompatible programmatic border treatment with native dark Silkroad field surfaces and framed work areas while preserving every existing layout and action.
- Requirement: replace the client DLL. No Filter restart, SQL update, or media update is required.

## Update v2.5.0

Release date: 2026-07-30

- Update releases can now contain only the selected components: Filter, Client DLL, GameServer DLL, ShardManager DLL, database files, or media files.
- DLL-only releases no longer include unchanged Filter files or other unselected DLLs.
- After successful completion, temporary Filter and updater payloads are removed automatically while selected manual files remain available in their clearly named folders.
- Only the latest Filter rollback copy is retained, and the updater replacement helper no longer remains beside the Filter files.
- All KMTGuard Filter applications, the Client DLL, GameServer add-on, ShardManager add-on, Update Center, and owner tools now identify themselves consistently as version 2.5.0.
- The in-game client watermark now shows KMTGuard v2.5.0 in the selected client language.
- Customer production packages are now rebuilt cleanly after internal test builds so test-only activation behavior cannot carry into customer deliveries.
- Developer publishing can continue safely while the already-installed License Center is open when its executable is byte-for-byte identical to the new build.
- Requirement: replace the Filter files, Client DLL, GameServer add-on, ShardManager add-on, and owner License Center, then restart the Filter and server modules. Import the updated client text resource. No SQL update or other media update is required.

## Update v2.4.1

Release date: 2026-07-30

- Changed the procedure scheduler editor so administrators can enter the target database and stored procedure names directly instead of choosing from the Filter database procedure list.
- Existing schedules remain compatible and can still be edited or run normally.
- Requirement: replace the Admin Desktop files. No Filter restart, SQL update, client DLL, or media update is required.

## Update v2.4.0

Release date: 2026-07-30

- Redesigned Lucky Spin in the Old School Silkroad style with a classic framed wheel, clearer reward and result areas, compact controls, and consistent native colors.
- Redesigned both PvP Challenge windows in the Old School style with recessed entry fields, clearer challenge details, compact accept and decline actions, and precisely aligned close controls.
- Corrected the close-control alignment in the Like Maxi in-game menu without changing the menu layout or its actions.
- Existing Lucky Spin rewards, PvP Challenge requests, results, timers, and server behavior are unchanged.
- Requirement: replace the client DLL. No Filter restart, SQL update, or media update is required.

## Update v2.3.7

Release date: 2026-07-30

- Fixed the Update Publisher action so packaging starts reliably and publishing errors are always shown clearly.
- The publisher now proposes the next available update version automatically after loading or publishing a release.
- The Update Center now remembers the successfully installed release so the same update is not offered again after restarting.
- Requirement: replace the owner License Center. Customers receive the Update Center correction with their next Filter update; no SQL, DLL replacement, or media update is required for this correction.

## Update v2.3.6

Release date: 2026-07-30

- Redesigned the login-screen account registration window in the Old School Silkroad style with a classic frame, recessed fields, compact actions, consistent spacing, and a precisely aligned close control.
- Added clearly presented account ID and password requirements while preserving the existing registration validation and request behavior.
- Password and confirmation values are masked while typing, and Tab or Shift+Tab moves naturally between all registration fields.
- Requirement: replace the client DLL. No Filter restart, SQL update, or media update is required.

## Update v2.3.5

Release date: 2026-07-30

- Established a reusable Old School visual standard for future classic Silkroad client windows while preserving the existing Item Mall design standard separately.
- Future client windows can now be requested explicitly in either Old School or Item Mall style with consistent frames, fields, buttons, spacing, and close controls.
- Requirement: no client files, Filter restart, SQL update, or media update is required.

## Update v2.3.4

Release date: 2026-07-30

- Raised the complete Secondary Password window slightly for a better login-screen position without changing its layout, controls, or behavior.
- Requirement: replace the client DLL. No Filter restart, SQL update, or media update is required when v2.3.3 is already installed.

## Update v2.3.3

Release date: 2026-07-29

- Fixed the Secondary Password action buttons appearing outside the window after it was centered.
- Replaced and precisely aligned the close control in the classic title frame.
- Requirement: replace the client DLL. No Filter restart, SQL update, or media update is required when v2.3.2 is already installed.

## Update v2.3.2

Release date: 2026-07-29

- Restored the Secondary Password window to the classic Silkroad interface style with matching fields, frame, close control, and action buttons.
- Fixed complete keyboard entry in every passcode field, including consecutive digits, Backspace, Delete, Tab navigation, and confirmation-field focus.
- Fixed the confirmation action appearing as a plain white control while the passcode was being entered.
- Requirement: replace the client DLL and update the supplied Secondary Password client resource. No Filter restart or SQL update is required.

## Update v2.3.1

Release date: 2026-07-29

- Fixed a client startup crash caused by an incompatible Secondary Password resource header.
- Requirement: update the Secondary Password client resource file. No DLL replacement, Filter restart, SQL update, or additional media is required when v2.2.0 is already installed.

## Update v2.3.0

Release date: 2026-07-29

- Added centrally managed English and Turkish language files for all player-facing messages generated by the Filter.
- Filter messages now use stable language keys, including account access, item purchases, security checks, stalls, event registration and results, region restrictions, scoreboards, challenges, rewards, and warnings.
- Existing system notices used by the Filter are included in both supplied languages, while customer-defined database notices remain compatible.
- Missing or invalid translations safely fall back to English, and unsafe placeholder changes are rejected to prevent broken player messages.
- The active language is selected from `Settings.json`; supported language aliases and custom language files can be reloaded from the Filter console after wording changes.
- Requirement: replace the Filter files, keep both supplied language files together, select the required language, and restart the Filter. No SQL update, client DLL, or client media update is required.

## Update v2.2.0

Release date: 2026-07-29

- Rebuilt the Secondary Password window with a professional in-game security layout, clear passcode fields, masked input, focused-field highlighting, and consistent actions.
- Passcodes are now entered directly from the keyboard with numeric-only input, Backspace and Delete support, Tab navigation, and Enter-to-continue or submit.
- Creating and changing a passcode now provides dedicated new and confirmation fields without duplicated digits or blocked confirmation input.
- Requirement: replace the client DLL, update the supplied client media, and import the updated client text resource. No Filter restart or SQL update is required.

## Update v2.1.1

Release date: 2026-07-29

- Fixed the Secondary Password keypad so masked digits appear immediately when a number is selected, without first clicking the password field.
- Requirement: replace the client DLL. No Filter restart, SQL update, or media update is required.

## Update v2.1.0

Release date: 2026-07-29

- All KMTGuard client windows, controls, notifications, and formatted messages now read their wording from client language resources.
- Added a complete Turkish client translation while keeping every label independently editable for any other language.
- Added separate ready-to-import Turkish-only and English-only client text files for simple language switching.
- Customer language wording can now be changed through the supplied text resource without rebuilding the client DLL.
- Requirement: replace the client DLL and import the supplied client text resource. No Filter restart or SQL update is required.

## Update v2.0.0

Release date: 2026-07-29

- Added a secure licensed Update Center that automatically checks the customer release channel from the Desktop Dashboard.
- Filter updates now download through the existing licensed connection, verify every delivered file, preserve customer settings and license data, create a rollback copy, and restart the administration application after installation.
- New Client, GameServer, and ShardManager DLL files are prepared specifically for the licensed customer and placed in a clearly labeled `New DLL` folder for manual installation.
- Added saved component-folder locations and one-click verification so customers can confirm whether each new DLL has been installed correctly.
- Added owner release publishing to the License Center with release notes, required-update status, optional database and media delivery, and a published-version history.
- Existing subscriptions and activation keys remain valid across updates; customers do not need to reactivate for each release.
- Requirement: existing customers must install the v2 Filter package once to receive the Update Center. Future Filter updates are delivered through the Update Center. GameServer, ShardManager, and Client DLL updates still require manual copying while the related applications are stopped.

## Update v1.9.1

Release date: 2026-07-29

- Newly generated Developer/Test and licensed customer packages now organize database updates into clear Filter-version folders.
- Database verification scripts, product guides, and network tools are separated from runtime files so the Filter folder stays clean and easier to install.
- Package generation now rejects missing, duplicated, unassigned, or incorrectly versioned database updates before a customer package can be created.
- The Database Updates workspace reads the new version folders in release order while keeping optional validation scripts separate.
- Requirement: generate a new package to receive the organized layout. No Filter restart, SQL execution, client DLL, or media update is required for existing installations.

## Update v1.9.0

Release date: 2026-07-29

- Added reliable Discord bot notifications with one protected Bot Token and any number of customer-defined destination channels.
- Added a clear Discord workspace where administrators can add, name, enable, test, edit, and remove channel destinations using simple cards instead of database grids.
- Added manual Discord notifications, recent delivery activity, understandable permission and configuration errors, and one-click retry for failed messages.
- Server systems can now route queued Discord messages by the friendly destination name selected by the customer.
- Temporary Discord, network, and rate-limit failures retry automatically, while duplicate event protection prevents repeated alerts.
- Automated messages cannot unexpectedly mention roles or everyone.
- Requirement: replace the Filter and Admin Desktop files, apply the included Discord database update, and restart the Agent Filter service and Admin Desktop application. No client DLL or media update is required.

## Update v1.8.0

Release date: 2026-07-29

- Added a professional five-step Guided Setup that takes administrators from license confirmation to a healthy running filter.
- New installations now verify SQL access, save the server configuration, confirm the licensed server identity, inspect required database components, and start all services in the correct order.
- The administration workspace stays focused on setup and recovery tools until onboarding is complete, then automatically unlocks the full operations, events, content, and system workspaces.
- Added clear customer-friendly results for connection failures, missing runtime files, invalid server addresses, missing required database components, and service startup failures.
- Added direct access to packaged database updates whenever a required component needs attention; optional feature updates remain reviewable without blocking startup.
- Added a password-free support report that can be copied from Guided Setup or the setup-required dashboard.
- Replaced the unconfigured dashboard with a clear setup call to action while retaining permanent access to Guided Setup for future health checks.
- Existing installations are guided through the new readiness check once after updating.
- Requirement: replace the Filter/Admin Desktop files and restart the Admin Desktop application. No SQL or client media update is required.

## Update v1.7.0

Release date: 2026-07-29

- Rebuilt the procedure scheduler around clear customer choices: run once, every few minutes or hours, every day, or on selected weekdays.
- Added an interval schedule that can start immediately, making recurring refresh procedures such as “every 10 minutes” straightforward to configure.
- Added a searchable stored-procedure picker, optional parameter entry, schedule summaries, next-run previews, and clear enabled, paused, running, succeeded, and failed states.
- Existing schedules can now be selected, edited, enabled or paused, run immediately for testing, or deleted without editing database rows manually.
- Added a durable execution history with trigger type, result, duration, timestamp, and failure details for scheduled and manual runs.
- Weekly schedules now support multiple selected weekdays while remaining compatible with existing single-day schedules.
- The scheduler continues to prevent duplicate execution across filter instances and now records scheduled execution outcomes for easier support.
- Requirement: replace the Filter/Admin Desktop files, apply the included professional scheduler SQL update, and restart the filter services.

## Update v1.6.0
**Release Date:** 2026-07-29

### New Features
- Rebuilt the Admin Desktop home screen as a live operations desk with service health, online capacity, database readiness, active event schedules, recent administrator activity, and a prioritized action list.
- Added direct customer access to system health, database update status, chat history, and service logs.
- Added a complete warm light visual identity across the administration workspace, settings editor, service configuration, tables, controls, and runtime console.

### Improvements
- Filter settings and proxy services now use a cleaner worksheet layout that makes large configurations easier to scan and edit.
- Database readiness is now summarized on the home screen with direct access to detailed checks.
- The packaged administration build now includes the database updates required by its Database Updates workspace.
- Runtime controls now respect the selected settings file and avoid targeting another installation that happens to use the same process names.
- Telegram settings no longer overwrite stored notification choices that are not shown in the current screen.
- Settings validation now recognizes every supported setting type and reports unknown custom settings clearly.

### Update Requirements
- Replace the KMTGuard Filter and Admin Desktop files, then restart the Admin Desktop application.
- Apply the included settings-validation database update to remove false configuration warnings.
- No client DLL or media update is required.

## Lyon SRO Launcher v6.0.0
**Release Date:** 2026-07-29

### Improvements
- Introduced a new Royal Citadel visual identity designed specifically around the Lyon SRO lion logo.
- Added a cleaner cinematic layout with champagne-gold surfaces, turquoise energy accents, and a focused game-entry control.
- Preserved the launcher safeguard that keeps all game language data untouched.

### Update Requirements
- Replace only the launcher executable with the v6 edition.
- No client media import is required for the embedded launcher artwork.

## Lyon SRO Launcher v5.0.0
**Release Date:** 2026-07-28

### Improvements
- Added a premium Silk Road chronicle launcher with handcrafted classic fantasy styling and the original Lyon SRO logo.
- Added layered brass, parchment, caravan-map, and animated astrolabe details.
- Launcher settings no longer read, select, rewrite, or save game language data.

### Update Requirements
- Replace only the launcher executable with the v5 edition.
- Restore a clean Media.pk2 first if an older launcher previously changed its language data.

## Update v1.5.0
**Release Date:** 2026-07-28

### New Features
- License Center can now issue a replacement license file for a customer's existing personalized package without replacing their Filter, client DLL, GameServer add-on, or ShardManager add-on files.
- The replacement workflow verifies an existing customer DLL before issuing a compatible activation file.

### Improvements
- License Center now explains that the Filter must be started once as Administrator so GameServer and ShardManager can use the activated shared license.
- Release publishing is more reliable on development machines that use the build tools bundled with Visual Studio.

### Update Requirements
- Replace and restart the License Server, then replace the License Center application.
- No customer database, Filter, client DLL, media, GameServer add-on, or ShardManager add-on update is required.

## Update v1.4.6
**Release Date:** 2026-07-28

### New Features
- Added dedicated Login Account and Logout Account controls for the Hide and Seek event character in the desktop application.

### Bug Fixes
- The Hide and Seek event account no longer logs in automatically through event maintenance, schedules, event start, or normal clientless startup.
- Newly created city clientless groups now start automatically after replacing the previous group.
- Clientless bulk creation now warns when bot login protection would prevent entry and lets the administrator enable it before continuing.

### Update Requirements
- Replace the KMTGuard Filter and Admin Desktop files, then restart the desktop application and Agent Filter service.
- No database, client DLL, or media update is required.

## Update v1.4.5
**Release Date:** 2026-07-28

### Improvements
- The Telegram desktop page now shows only active notification features.
- Removed the retired Unique spawn, Unique kill, killer-name, and Unique display-name controls from the desktop interface.
- Added a clear note that Unique and custom alerts are now queued from SQL through `dbo.Telegram_Notification`.

### Update Requirements
- Replace the KMTGuard Admin Desktop file and restart the desktop application.
- No Agent Filter, client DLL, media, or database update is required.

## Update v1.4.4
**Release Date:** 2026-07-28

### Documentation Fixes
- Corrected the SQL integration example so multi-line Telegram messages can be passed to `dbo.Telegram_Notification` without a SQL syntax error.

### Update Requirements
- No service restart or binary update is required.

## Update v1.4.3
**Release Date:** 2026-07-28

### Improvements
- Telegram alerts can now be queued from any game or event procedure with the simple `dbo.Telegram_Notification` procedure.
- Unique packet observers were removed from the notification path; procedures can publish the exact English display name directly, without exposing internal CodeNames.

### Update Requirements
- Apply the Telegram notification database migration, replace the KMTGuard Filter and Admin Desktop files, then restart the Agent Filter service and desktop application.
- No client DLL or media update is required.

## Update v1.4.2
**Release Date:** 2026-07-28

### Bug Fixes
- Unique spawn notifications now read the model identifier carried directly by the server's Unique notification, so Object IDs are no longer mistaken for monster definitions.

### Update Requirements
- Replace the KMTGuard Filter files and restart the Agent Filter service.
- No database, client DLL, or media update is required.

## Update v1.4.1
**Release Date:** 2026-07-28

### Bug Fixes
- Unique spawn notifications now support servers that deliver entity appearances through grouped spawn packets.

### Update Requirements
- Replace the KMTGuard Filter files and restart the Agent Filter service.
- No database, client DLL, or media update is required.

## Update v1.4.0
**Release Date:** 2026-07-28

### New Features
- Clientless account creation now supports an exact account count for each available city in one operation.
- Automatically created characters now use varied, natural gaming names instead of generated prefixes and random letter strings.

### Improvements
- Clientless characters are distributed across the full selected city area with stronger spacing between nearby characters.

### Update Requirements
- Replace the KMTGuard Filter and Admin Desktop files, then restart the desktop application and Agent Filter service.
- No database, client DLL, or media update is required.

## Update v1.3.2
**Release Date:** 2026-07-28

### Bug Fixes
- Unique spawn notifications now resolve the temporary world object identifier to the correct monster definition before building the message.
- Standard and server-added Uniques are now validated from the server's rarity classification, while public messages continue to use safe English display names.

### Update Requirements
- Replace the KMTGuard Filter files and restart the Agent Filter service.
- No database, client DLL, or media update is required.

## Update v1.3.1
**Release Date:** 2026-07-28

### Bug Fixes
- Fixed an Admin Desktop startup failure introduced by the Telegram Notifications interface.

### Update Requirements
- Replace the KMTGuard Filter files. No database, client DLL, or media update is required.

## Update v1.3.0
**Release Date:** 2026-07-28

### New Features
- Added a complete Telegram Notifications center to the desktop application with protected bot credentials, channel configuration, connection testing, manual announcements, delivery history, and failed-message retry.
- Added professional English alerts for Unique spawns and kills, event reminders, event starts and finishes, server availability, and Fortress War activity.
- Added configurable event reminder times and individual switches for every notification category.
- Added a dedicated Unique display-name registry so public alerts use real English monster names and never expose internal CodeNames or localization keys.

### Reliability and Security
- Telegram messages now use a durable delivery queue with automatic retry, duplicate-event protection, and no blocking network work on game packet processing.
- Bot credentials are encrypted for the Windows server before database storage.
- Unresolved Unique names are safely withheld and shown in the desktop application for administrator correction.

### Update Requirements
- Run the supplied Telegram notifications database update on the KMTGuard database.
- Replace the KMTGuard Filter and Admin Desktop files, then restart the Agent Filter service.
- No game client DLL or media replacement is required.

## Update v1.2.9
**Release Date:** 2026-07-28

### Improvements
- Replaced the game startup loading artwork with ten distinct, cinematic Silk Road scenes.
- Centered the original Lyon SRO logo consistently across every loading screen.
- Added calmer compositions with softer focus around the logo for clearer visibility.

### Update Requirements
- Replace the loading-screen media files and fully restart the game client.
- No database update is required.

## Update v1.2.8
**Release Date:** 2026-07-28

### Bug Fixes
- Auto Alchemy now stops safely when its selected item is removed, the window is closed, or the game scene changes.
- Auto Alchemy now waits for the server result before continuing and stops safely if the server does not respond.
- Improved Auto Alchemy reliability for blue, attribute, protection-stone, and absorption upgrades.
- Region alchemy restrictions now apply consistently to all Auto Alchemy operations.
- Regions that disable advanced elixirs no longer block normal Auto Alchemy attempts.

### Update Requirements
- Replace the KMTGuard Filter files and client DLL, restart all KMTGuard Filter services, and fully restart the game client.
- No database update is required.

## Update v1.2.7
**Release Date:** 2026-07-28

### Bug Fixes
- In-game KMTGuard windows can now be closed consistently with the Escape key as well as their close button.

### Update Requirements
- Replace the client DLL and fully restart the game client.
- No Filter restart, media import, or database update is required.

## Lyon SRO Launcher v4.0.0
**Release Date:** 2026-07-28

### Improvements
- Replaced the launcher presentation with a modern cinematic gate interface and smooth entrance animations.
- Added a dedicated command deck for realm status, notices, and update progress.
- Kept the English-only experience and the authorized Lyon SRO connection protection.

### Update Requirements
- Use the new launcher executable from the v4 delivery folder.
- No media import is required for the launcher artwork; existing launcher versions remain available separately.

## Update v1.2.6
**Release Date:** 2026-07-28

### Bug Fixes
- VIP rank changes now grant and activate the correct right-side rank icon through the current player-style system.
- VIP icons are now removed safely when a character no longer qualifies for a configured rank.

### Update Requirements
- Run the supplied database update on the KMTGuard database after installing the current player-style database update.
- No Filter restart or client-file replacement is required.

## Update v1.2.5
**Release Date:** 2026-07-27

### Bug Fixes
- Restart now restores the complete verified server-list data through the game client's native handler.
- Server selection and connection eligibility are restored together instead of changing only the displayed status text.

### Update Requirements
- Replace the client DLL and fully restart every game client.
- No Filter restart or database update is required.

## Update v1.2.4
**Release Date:** 2026-07-27

### Bug Fixes
- The login screen now restores the complete verified server-list state when Restart rebuilds the screen without creating a new Gateway connection.
- Server selection and connection eligibility now remain functional after returning to the login screen.
- Login-screen custom button labels are restored during screen reconstruction.

### Update Requirements
- Replace the KMTGuard Filter files and client DLL, restart all KMTGuard Filter services, and fully restart the game client.
- No database update is required.

## Update v1.2.3
**Release Date:** 2026-07-27

### Bug Fixes
- Fixed the server list remaining in Check state after returning to the login screen or opening multiple clients.
- Login-screen server status and location indicators now refresh consistently after Restart.
- Register and Quick Login button labels now remain visible when the login screen is rebuilt.

### Update Requirements
- Replace the KMTGuard Filter files and client DLL, restart all KMTGuard Filter services, and restart the game client.
- No database update is required.

## Update v1.2.2
**Release Date:** 2026-07-27

### Bug Fixes
- Universal Pills and all configured macro item, skill, weapon, buff, and scroll slots now return correctly after reconnecting.
- Auto Potion now checks existing abnormal states immediately and stops all related background activity when disabled.
- Character and pet potion handling is more reliable across simultaneous health, mana, vigor, pill, speed, summon, and resurrection conditions.
- Auto Hunt now restores its saved state, evaluates HP and MP potion totals correctly, and safely stops when a requested town return cannot be completed.
- Patrol movement, target range calculations, party invitations, party buffs, and party resurrection now select valid nearby targets consistently.
- Auto Skill now handles invalid or outdated skill entries safely and supports weapon-independent skills without requiring a configured weapon slot.
- Auto Scroll now validates the live inventory item, effect, and slot before use and prevents repeated invalid use attempts.
- Pet Filter now respects cross-region distance, filters items consistently for character and pet pickup, limits pickup requests, and releases old pickup history.
- Prevented several macro-related crashes during window cleanup, scene changes, missing item data, oversized skill lists, and invalid saved slots.

### Improvements
- Macro setting files are created reliably when the local settings folder is missing.
- Saved Auto Hunt choices and party lists are reflected correctly in the configuration window.

### Update Requirements
- Replace the client DLL and restart the game client.
- No Filter restart or database update is required.

## Update v1.2.1
**Release Date:** 2026-07-27

### Bug Fixes
- Fixed the server list remaining in Check state after returning to the login screen.
- Fixed simultaneous client launches intermittently leaving all but one client unable to select the server.
- Server-list responses are now retained safely while each client connection completes its security negotiation.

### Update Requirements
- Replace the KMTGuard Filter files and restart all KMTGuard Filter services.
- No database or client-file update is required.

## Update v1.2.0
**Release Date:** 2026-07-27

### New Features
- Added centrally managed server slots so one customer license can authorize multiple Game Servers on separate public IP addresses.
- Added a shared online-player limit across all active licensed servers.
- Added independent license enforcement for the Filter, GameServer add-on, ShardManager add-on, and client DLL package.

### Improvements
- The License Center now accepts one IPv4 address per server slot and supports common one-, two-, three-, four-, five-, and eight-server plans.
- Selecting the number of Game Servers now creates a separate, clearly labeled IP field for every server.
- Customer packages now carry a signed customer binding in the client and server add-ons.
- Short-lived leases are protected against being copied to another Windows server while remaining automatically renewable from a licensed IP.

### Security
- Unlisted server IPs, excess active server slots, copied leases, modified package bindings, and combined player totals above the licensed limit are rejected.
- Suspended licenses and server-IP changes clear active server heartbeats immediately.

### Update Requirements
- Restart the License Server after installing this update.
- Generate and deliver a new customer package, then replace the Filter, GameServer add-on, ShardManager add-on, and client DLL files.
- Start the Filter once on each licensed server before starting GameServer or ShardManager.
- For multi-server customers, set each server's Filter `ServerIP` value to that server's own licensed public IP.

## Update v1.1.0
**Release Date:** 2026-07-27

### Improvements
- Rebuilt the Attendance event as a reliable repeating 35-day cycle.
- Attendance now uses the database server date consistently across all Agent instances.
- Reward definitions refresh when the Attendance window is opened, including an intentionally empty reward list.
- Attendance progress and available rewards now refresh after every record or claim action.

### Bug Fixes
- Prevented an Attendance reward from being delivered more than once during simultaneous requests or interrupted database operations.
- Prevented duplicate player progress and reward-state records.
- Removed obsolete reward states automatically when their reward definition is removed.
- Prevented invalid progress or reward counts from exceeding the client calendar and reward-slot limits.
- Players now receive a clear response when attendance was already recorded or when a reward is unavailable.

### Security
- Added strict validation for Attendance reward items, quantities, days, and protocol limits.
- Malformed and excessively repeated Attendance requests are rejected safely.

### Update Requirements
- Stop all KMTGuard Agent instances before applying the included Attendance database update.
- Replace both the KMTGuard filter files and the client DLL, then restart the Agent service.
- Run the included Attendance validation after the database update.
- This update does not add Attendance rewards; reward configuration remains under the server administrator's control.

## Update v1.0.0
**Release Date:** 2026-07-27

### New Features
- تمت إضافة سجل تحديثات موحد يوضح للعميل المزايا والإصلاحات والتحسينات في كل إصدار.

### Improvements
- أصبح سجل التحديثات جزءًا أساسيًا من إصدار KMTGuard Agent لضمان توفر تفاصيل التحديث مع كل نسخة.

### Update Requirements
- يجب إعادة تشغيل خدمة KMTGuard Agent بعد تثبيت الإصدار الجديد.
- لا يتطلب هذا التحديث تعديل قاعدة البيانات أو ملفات العميل.
- يجب إبقاء ملف سجل التحديثات ضمن ملفات KMTGuard Agent عند نقل الخدمة أو تثبيتها.
## v2.6.25 — 2026-08-01

### Added

- Added a dedicated **Bot Operations** workspace with a clearer **Clientless Manager** entry.

### Improved

- Reorganized Clientless administration into icon-based Runtime, Provisioning, Accounts, and Party Matching tabs.
- Added a shared Account Defaults panel so connection and reconnect settings stay visible across every bot workflow.
- Clientless account, runtime, and Party Form information now refreshes automatically when the workspace opens.

## v2.6.24 — 2026-08-01

### Added

- Added a dedicated **Notifications** workspace for Discord and Telegram administration.
- Added safe deletion for selected Lucky Spin rewards while preserving historical spin records.

### Improved

- Consolidated Unique Rules, Region Control, Scheduler, and Web Buttons under **System Game**.
- Added vertical scrolling to PvP Challenge and Special Offers editors so every setting remains accessible at smaller window sizes.

### Requirements

- Restart KMTGuard after changing Lucky Spin rewards to refresh the live reward cache.

## v2.6.23 — 2026-08-01

### Fixed

- Moved **Unique Rules** into its own **System Game** navigation workspace instead of grouping it with data and system tools.

## v2.6.22 — 2026-08-01

### Added

- Added a dedicated **Unique Rules** dashboard module for managing unique attack restrictions.
- Administrators can add, edit, reload, and delete rules by Unique ID, configure attacker/job restrictions, and set party/guild requirements.
- Changes are audited and require a GameServer restart to refresh its cached rules.
