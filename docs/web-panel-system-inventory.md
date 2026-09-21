# KMTGuard Web Panel System Inventory

Generated from the current source tree in `<repo root>`.

Scope used for this inventory:
- Filter source: `filter/KMTGuardnew/KMTGuard`
- Game server/custom shard references: `gameserver`, `JTShard`
- SQL migrations: `database/migrations`
- Build and handoff docs/scripts at repository root

Explicitly excluded from web-panel design per owner instruction:
- Client DLL UI implementation details under `JTClientLibrary`. The web panel may expose settings that the DLL consumes, but it must not depend on editing client UI internals.

This document is the gate before building the web panel. The panel must be system-first, not table-first. No generic SQL table editor, raw SQL editor, phpMyAdmin clone, or admin-facing column-name CRUD should be built.

## Discovered Runtime Shape

KMTGuard is a proxy/filter with separate Gateway, Agent, and Download server flows.

Main entry:
- `filter/KMTGuardnew/KMTGuard/Startup.cs`
- Creates `Settings.json` if missing.
- Builds SQL connection to the proxy database from `Settings.json`.
- Loads settings from `dbo.__Settings`.
- Runs environment/IP/process security checks.
- Starts server listeners through `ServerManager.InitialServers()`.
- Starts command console loop.

Core services:
- `ServerManagers/ServerManager.cs`
- `ServerManagers/RefManager.cs`
- `Database/DatabaseCommands.cs`
- `Database/Scheduler.cs`
- `PacketHandler/PacketHandler.cs`

Primary databases discovered/used:
- Proxy/filter DB: `KMTGuard` or configured `ProxyDb`.
- Shard DB: setting `ShardDB`, currently expected as `SRO_VT_SHARD`.
- Account DB: setting `AccountDB`, currently expected as `SRO_VT_ACCOUNT`.
- Log DB: panel config currently points to `SRO_VT_SHARDLOG`, but filter code mostly logs to files and selected proxy tables.

Important DB connection rules:
- Filter reads database names from `Settings.json` and `dbo.__Settings`.
- Many systems are cached by `RefManager` at startup.
- Some systems can be updated live through `_AsyncFilterCommands`.
- Some settings are loaded once and need reload/restart unless an explicit runtime command exists.

## Runtime Actions Map

### Command Queue

Source:
- `filter/KMTGuardnew/KMTGuard/Database/DatabaseCommands.cs`

Tables:
- `dbo._AsyncFilterCommands`
- `dbo._AsyncFilterCommandsPlanned`

Behavior:
- Polls pending commands every few seconds.
- Marks commands complete by setting `Status = 0`.
- Planned commands only allow SQL that starts with `EXEC ` and rejects semicolon/comment patterns.

Safe admin use:
- The web panel should not expose raw command rows.
- Build named actions such as disconnect player, broadcast notice, reload ranks, add item to chest, activate title/icon, timer broadcasts.
- Every panel action that writes `_AsyncFilterCommands` must log an audit row and display whether the action is immediate or queued.

Known runtime command families:
- Disconnect one character or all sessions.
- Send notice to all or selected character.
- Grant/activate/remove title colors, title names, and left/right icons.
- Add item chest reward and notify player.
- Toggle attack restrictions by region/world.
- Broadcast timer packets by world/region.
- Reload ranks/achievements/achievement conditions through command `40`.
- Update unique log runtime state.
- Planned stored procedure execution through command `100`, restricted to `EXEC`.

Risks:
- Incorrect command data can affect all online users.
- Reward commands can duplicate rewards if not idempotent.
- Planned SQL must never be user-editable raw SQL in the panel.

Panel module:
- Runtime Actions.
- Queued Jobs.
- Planned Jobs, but only as safe stored-procedure actions chosen from allowed templates.

### Scheduler

Source:
- `filter/KMTGuardnew/KMTGuard/Database/Scheduler.cs`
- `filter/KMTGuardnew/KMTGuard/ServerManagers/RefManager.cs`

Table:
- `dbo._Scheduler`

Fields used by code:
- `Idx`, `Name`, `Query`, `ScheduledDate`, `Time`, `RepeatType`, `RepeatDayOfWeek`, `IsEnabled`, `LastRunDateTime`

Behavior:
- Loads enabled jobs.
- Executes safe SQL only when query starts with `EXEC ` and rejects semicolon/comment patterns.
- Has a reload path inside scheduler code, but web panel must verify whether the running instance exposes it through command/API before promising live reload.

Safe admin UI:
- Event/job name.
- Stored procedure selector from an allow-list.
- Date, time, repeat type, weekday.
- Enable/disable.
- Last run status.

Do not expose:
- Raw `Query` text editing for normal admins.

Reload:
- Needs scheduler reload or filter restart unless a safe runtime action is added.

## Packet Pipeline And Security

Source:
- `PacketHandler/PacketHandler.cs`
- `Database/sqlQueryHelper.cs`

Tables:
- `dbo.__Whitelist`
- `dbo.__Blacklist`

Behavior:
- Handshake opcodes `0x9000`, `0x5000`, `0x2001` are always allowed.
- Whitelist/blacklist is loaded by server type.
- Handler result can forward, block, disconnect, or override packet.

Panel module:
- Packet Security.
- View whitelist/blacklist as named packet rules, grouped by Gateway/Agent/Download.
- Enable/disable rules only through validated forms.

Admin fields:
- Opcode displayed as hex plus friendly name.
- Direction.
- Server type.
- Action.
- Reason/comment.

Risks:
- Blocking the wrong opcode can prevent login or gameplay.
- Adding unknown opcodes without validation can open bypasses.

Reload:
- Likely requires filter restart unless a reload method is exposed or added.

## Server Services

### Gateway Server

Source:
- `Server/GatewayServer/GatewayServer.cs`
- `Server/GatewayServer/PacketHandler/SERVER_DLL_SETTINGS_RESPONSE.cs`
- `Server/GatewayServer/PacketHandler/CLIENT_ACCOUNT_REGISTER_REQUEST.cs`

Important opcodes:
- C->S `0x2002`: handshake/service flow.
- S->C `0xA102`: login redirect.
- S->C `0xA100`: gateway response.
- S->C `0x2322`: server signal.
- C->S `0x165B`: HWID request.
- C->S `0x1211`: secondary password request.
- C->S `0x166A`: account register request.
- C->S `0x6102`: login request.
- C->S `0xA150`: DLL settings request.
- S->C `0xA101`: shard info replacement.

Tables/procs:
- `SRO_VT_ACCOUNT..TB_User`
- `SRO_VT_ACCOUNT..SK_Silk`
- `_AccountSecondaryPassword`
- `__Settings`

Panel module:
- Gateway/Login Settings.
- Account registration settings.
- Secondary password security.
- Online gateway diagnostics.

Safe actions:
- Enable/disable in-game registration.
- Configure captcha/login options.
- Search account by username.
- Reset secondary password with confirmation and audit log.

Risks:
- Account writes must use password hashing compatible with the gateway.
- Never expose raw password values.

### Agent Server

Source:
- `Server/AgentServer/AgentServer.cs`
- `Server/AgentServer/PacketHandler/**`

Important systems below are mostly Agent packet handlers.

Panel module:
- Agent Sessions.
- Player Search.
- Runtime Actions.
- Packet Security.

### Download Server

Source:
- `Server/DownloadServer/DownloadServer.cs`

Opcode:
- C->S `0x2002`

Panel module:
- Service Status only unless more download-specific logic is added.

## Settings System

Source:
- `Database/Models/__Settings.cs`
- `database/migrations/20260627_settings_cleanup_and_catalog.sql`

Table:
- `dbo.__Settings`

Settings categories discovered:
- `Core.Database`
- `Gateway.Login`
- `Client.MenuButtons`
- `Client.GuideIcons`
- `Client.UI`
- `Client.Links`
- `Client.ItemTranslation`
- `Client.Macro`
- `LuckySpin`
- `SpecialOffers`
- `Security.Limits`
- `Gameplay.Delays`
- `Trade.Captcha`

Views created by migration:
- `dbo.vw_Settings_Organized`
- `dbo.vw_Settings_InvalidValues`

Behavior:
- Filter loads settings into static `_serverSettings`.
- Invalid setting names are logged as fatal.
- Invalid bool/int values log errors/fatal depending type.

Panel module:
- Filter Settings, split into friendly pages by category.

Field widgets:
- Boolean settings as toggles.
- Delay/limit settings as numeric inputs with min/max.
- URLs as URL inputs.
- Database names hidden from normal admins, visible to technical admins only.

Reload:
- Needs a settings reload command or filter restart. Existing console command `/reload 1` should be inspected before wiring.

Risks:
- Unknown settings can break startup.
- Invalid numeric values can break runtime behavior.

## Reference Cache System

Source:
- `ServerManagers/RefManager.cs`

Purpose:
- Loads most gameplay references into memory caches.

Loaded systems:
- Lucky Spin rewards.
- Special Offers.
- Scheduler.
- HWID bypass IPs.
- GM IPs.
- NPC unique IDs.
- Shard object/item/character references.
- Notices.
- HWAN titles.
- Rank categories and rank tables.
- Fellow pet references.
- New item mall/avatar mall.
- Global color.
- Event schedule/register.
- Attendance rewards.
- Mob kill logger refs.
- Fellow pet system.
- Map settings.
- Hide skill effects.
- Achievement refs and conditions.
- Title/icon media paths.
- Blocked skills.
- Active name/title colors and icons.

Panel rule:
- Any panel page editing a RefManager-loaded table must show a reload requirement.
- If a runtime command exists, expose a Reload button; otherwise say “requires filter restart”.

## Player And Session Systems

Source:
- `Session/Session.cs`
- `Session/SessionData.cs`
- `Session/ConcurrentSessionSet.cs`
- `ServerManagers/ServerManager.cs`
- `Server/AgentServer/PacketHandler/CharacterSelect/CharacterSelection.cs`
- `Server/AgentServer/PacketHandler/CharacterActions/CharDataPackets.cs`

Important opcodes:
- C->S `0x6103`: user info.
- C->S `0x7001`: character select.
- C->S `0x7007`: character selection action.
- S->C `0xB007`: character selection response.
- S->C `0xB001`: enter game response.
- S->C `0x3013`: character data.
- S->C `0x3054`: level info.

Tables:
- `SRO_VT_SHARD.._Char`
- `_PartyData`
- `_HwidList`
- `_CharacterSettings`
- `_CharInstanceWorldData`

Panel module:
- Players.
- Online Sessions.
- Character Details.
- Account Details.

Safe actions:
- Search character/account/guild.
- View online status, IP, HWID if permitted.
- Disconnect session through runtime command.
- View chest/rewards/history.

Do not expose:
- Direct edit of `CharID`, `JID`, inventory rows, or item ownership.

Risks:
- Direct player/item DB edits can corrupt character state.

## HWID And Account Security

Source:
- `Helpers/HwidSecurity.cs`
- `Database/sqlQueryHelper.cs`
- `Server/GatewayServer/PacketHandler/SERVER_DLL_SETTINGS_RESPONSE.cs`

Tables:
- `_HwidList`
- `_bypassHwidbyIP`
- `_GMIPList`
- `_AccountSecondaryPassword`
- `SRO_VT_ACCOUNT..TB_User`
- `SRO_VT_ACCOUNT..SK_Silk`

Settings:
- `HWID_LIMIT`
- `HWID_JOB_LIMIT`
- `IPLimit`
- `SecondaryPassword`

Panel module:
- Security Center.
- HWID/IP Rules.
- Secondary Password.
- Account Registration.

Safe actions:
- Search HWID/IP/account/character.
- Deactivate HWID entry.
- Add bypass IP with expiry/comment if schema supports it.
- Reset secondary password.

Risks:
- HWID/IP changes affect login limits and job limits.
- Must audit every security change.

## Chat Filtering And Chat Logs

Source:
- `ChatFiltering/ChatWordFilter.cs`
- `ChatFiltering/BlockedWordRule.cs`
- `Server/AgentServer/PacketHandler/Chat/ChatPackets.cs`
- `Server/AgentServer/PacketHandler/Chat/ChatLogger.cs`

Opcodes:
- C->S `0x7025`: chat request.
- C->S `0x705C`: global item link forward.
- S->C `0x5033`: global item link/global.

Tables:
- `dbo.BlockedWords`
- `dbo.ChatLog`
- `dbo.region_control`

Panel module:
- Chat Moderation.

Admin UI:
- Blocked word rules with match mode and active toggle.
- Region chat disable toggle using Region selector.
- Chat log search by sender, receiver, type, date.

Reload:
- `ChatWordFilter` load behavior must be checked before live updates. If cached, add reload or restart note.

Risks:
- Bad word matching can overblock normal chat.
- Chat logs may contain private messages; require permission.

## Trade Goods Captcha

Source:
- `ServerManagers/TradeSellCaptchaService.cs`
- `ServerManagers/TriggerService.cs`
- `Server/AgentServer/PacketHandler/CharacterActions/CharAction.cs`
- Migration `20260627_trade_sell_captcha.sql`

Stored procedures:
- `_OnTradeGoodsBuyingComplete_EDIT`
- `_OnTradeGoodsSellingComplete_EDIT`

Settings:
- `EnableTradeSellCaptcha`
- `TradeSellCaptchaTimeoutSeconds`
- `TradeSellCaptchaMaxAttempts`

Behavior:
- Protects trade goods selling flow with a verification challenge.
- Uses existing secondary-password-like numeric UI on client side, but panel only manages server settings/logs.

Panel module:
- Trade Protection.

Safe UI:
- Enable/disable.
- Timeout and max attempts.
- Trade logs and failed verification logs if a log table exists or is added.

Risks:
- Incorrect trigger/procedure handling can block legitimate trade goods.
- Existing owner-created triggers must not be removed.

Reload:
- Settings reload/restart unless runtime reload exists.

## Silk Stall

Source:
- `Server/AgentServer/PacketHandler/Stall/StallPackets.cs`
- `ServerManagers/SilkStallService.cs`
- Migration `20260625_silk_stall_transactions.sql`

Opcodes:
- C->S `0x186D`: stall type selection.
- C->S `0x70BA`: stall update/action.
- C->S `0x70B1`: stall create.
- C->S `0x70B4`: stall buy.
- S->C `0xB0B1`: create response.
- S->C `0xB0B2`: destroy response.
- S->C `0xB0B3`: talk response.
- S->C `0xB0B4`: buy response.

Tables:
- `dbo.SilkStallTransactions`
- `SRO_VT_ACCOUNT..SK_Silk`

Stored procedure:
- `_OnStallCreate`

Behavior:
- Tracks silk stall transaction status.
- Uses account silk balance with locking/transactions.
- Handles buyer/seller settlement and rollback states.

Panel module:
- Silk Stall.

Admin UI:
- Transaction history.
- Pending/failed transactions.
- Search by seller/buyer/item.
- Reconciliation view.

Editable:
- No direct transaction edits for normal admins.
- Only safe support actions after validation, with audit and transaction.

Risks:
- Silk duplication.
- Item/silk mismatch.
- Race conditions on purchase settlement.

## Lucky Spin

Source:
- `Server/AgentServer/PacketHandler/CustomUIPackets.cs`
- `ServerManagers/RefManager.cs`
- Migration `20260626_lucky_spin.sql`

Settings:
- `EnableLuckySpin`
- `EnableLuckySpinSilk`
- `LuckySpinPrice`
- `ShowGuideLuckySpin`

Tables:
- `dbo._LuckySpinRewards`
- `dbo._LuckySpinLog`
- `dbo._ItemChest`

Related stored procedure:
- `_AddItemToChest` is the correct procedure name in this environment.

Observed opcodes:
- C->S custom UI requests are routed through `CustomUIPackets`.
- Item chest response uses module `0xA405`.

Behavior:
- Rewards loaded into `RefManager.m_LuckySpin`.
- Spin awards item through chest flow and logs result.
- Rewards only show correctly after active reward rows exist and cache reload/restart occurs.

Panel module:
- Lucky Spin.

Admin UI:
- Enable/disable.
- Price/currency.
- Reward builder with Item selector, amount, rate, active toggle.
- Reward preview using item name/icon where possible.
- Spin logs and player history.

Reload:
- Reward changes require `RefManager.LoadLuckySpin()` runtime command/API or restart.

Risks:
- Duplicate rewards if retry is not idempotent.
- Invalid item IDs can fail purchase/reward grant.
- Rate values must be validated.

## Special Offers Shop

Source:
- `Server/AgentServer/PacketHandler/CustomUIPackets.cs`
- `ServerManagers/RefManager.cs`
- Migration `20260627_special_offers.sql`

Settings:
- `EnableSpecialOffers`
- `ShowGuideSpecialOffers`

Tables:
- `dbo._SpecialOffers`
- `dbo._SpecialOfferLog`
- `dbo._ItemChest`
- `SRO_VT_ACCOUNT..SK_Silk`

Behavior:
- Offers loaded into `RefManager.m_SpecialOffers`.
- Purchase writes log and grants item through chest.
- Uses payment type and preview mode.

Panel module:
- Special Offers.

Admin UI:
- Create/edit offer by choosing item through Item selector.
- Preview item selector/mode.
- Main price, sale price, payment type, stock/service, sort order.
- Page/order preview.
- Purchase logs.

Do not expose:
- Raw `ItemID` as primary input.

Reload:
- Requires `RefManager.LoadSpecialOffers()` runtime action/API or restart.

Risks:
- Wrong item/proc parameter types can disconnect player.
- Silk purchase must be transactional.
- Duplicate reward prevention required.

## Item Chest And Rewards

Source:
- `Server/AgentServer/PacketHandler/CustomUIPackets.cs`
- `Database/sqlQueryHelper.cs`
- `Database/DatabaseCommands.cs`

Tables:
- `_ItemChest`
- `_AttendanceRewardLog`
- `_LuckySpinLog`
- `_SpecialOfferLog`

Stored procedure:
- `_AddItemToChest`

Opcodes:
- C->S `0xB296`: item chest request.
- S->C `0xA405`: item chest response.
- S->C custom `0x203B`: runtime chest notification.

Panel module:
- Rewards / Item Chest.

Admin UI:
- Send reward to character/account using Item selector.
- Pending chest items.
- Reward history.
- Idempotency key per admin reward.

Risks:
- Item duplication.
- Invalid item amount.
- Sending to wrong character.

Required safety:
- Transaction.
- Audit log.
- Confirmation.
- Duplicate protection.

## New Item Mall And Avatar Mall

Source:
- `Server/AgentServer/PacketHandler/CustomUIPackets.cs`
- `ServerManagers/RefManager.cs`

Tables:
- `_RefNewItemMall`
- `_RefNewAvatarMall`

Opcode:
- C->S `0xB297`: new item mall buy request.

Behavior:
- Catalog loaded from RefManager.
- Purchases update silk and grant item.

Panel module:
- Custom Item Mall.

Admin UI:
- Product catalog with item search selector.
- Price/payment settings.
- Active/service toggles.
- Purchase logs if existing or create panel audit.

Reload:
- Requires RefManager reload or restart.

Risks:
- Same as rewards and silk.

## Titles, Icons, Name Colors, Grant Name

Source:
- `Server/AgentServer/PacketHandler/UI/Title_IconManagers.cs`
- `Server/AgentServer/PacketHandler/CustomUIPackets.cs`
- `Database/DatabaseCommands.cs`
- `ServerManagers/RefManager.cs`

Opcodes:
- C->S `0x169C`: icon manager.
- C->S `0x169B`: title manager.
- C->S `0x207B`: grant name request.
- S->C `0x168B`, `0x168C`, `0x168D`: runtime grants/updates.
- S->C `0x170A`, `0x170B`, `0x173F`, `0x174A`, `0x174B`, `0x174E`, `0x202B`, `0x202C`: active visual updates.

Tables:
- `_CharacterTitleManagerColor`
- `_CharacterIconManager`
- `_ActiveNameColors`
- `_ActiveTitleColors`
- `_ActiveIconsLeftSide`
- `_ActiveIconsRightSide`
- `_ActiveTitleNameNew`
- `_RefTitleNameNew`
- `_RefIconsMediaPath`

Stored procedures:
- `_HandleIcons`
- `_HandleTitleColors`

Panel module:
- Character Cosmetics.

Admin UI:
- Grant/remove icon/title/color to character.
- Use Character selector and Icon/Title selector.
- Preview active cosmetics.
- Runtime apply/revoke through `_AsyncFilterCommands` where supported.

Risks:
- Incorrect active rows can desync online display.
- Runtime updates should be queued with audit.

## Achievements

Source:
- `CustomUIPackets.cs`
- `RefManager.cs`

Tables:
- `_RefAchievement`
- `_RefAchievementCondition`
- `_Achievement`
- `_AchievementCondition`

Panel module:
- Achievements.

Admin UI:
- Achievement definitions.
- Conditions builder.
- Player progress details.

Reload:
- Ref changes need `RefManager.LoadRefAchievements()` and `LoadRefAchievementsCondition()` or restart.
- Runtime command `40` reloads ranks/achievements/conditions.

Risks:
- Incorrect condition definitions can grant rewards incorrectly.

## Attendance / Daily Login

Source:
- `CustomUIPackets.cs`
- `RefManager.cs`

Tables:
- `_RefAttendanceReward`
- `_Attendance`
- `_AttendanceRewardLog`

Panel module:
- Attendance.

Admin UI:
- Reward calendar.
- Player attendance history.
- Claim status.

Risks:
- Duplicate daily rewards.
- Date/time boundary mistakes.

## Event Register And Event Schedule

Source:
- `CustomUIPackets.cs`
- `RefManager.cs`

Opcode:
- C->S `0xB301`: event register request.

Tables:
- `_RefEventRegister`
- `_RefEventSchedule`

Panel module:
- Events.

Admin UI:
- Event list.
- Registration windows.
- Schedule editor with day/time pickers.
- Participant list if stored.

Reload:
- RefManager reload or restart.

## Ranking Systems

Source:
- `ServerManagers/RefManager.cs`
- `ServerManagers/RankManager.cs`

Tables:
- `_RefRankCategories`
- `_Rank_Custom1` through `_Rank_Custom9`
- `_SilkRank`

Opcode:
- C->S `0x180A`: rank request.

Panel module:
- Rankings.

Admin UI:
- View rank categories.
- View generated rank data.
- Trigger safe rank reload.

Do not expose:
- Manual edits to generated rank rows unless a real admin workflow exists.

Reload:
- Ranks reload timer runs every 30 minutes in `RefManager.StartLoadRanksTimer()`.
- Runtime command `40` reloads rank/achievement caches.

## Region, Map, Skill, And Teleport Controls

Source:
- `ServerManagers/ActionManager.cs`
- `ServerManagers/RefManager.cs`
- `Features/Skills/SkillRuleManager.cs`
- `Features/Skills/CooldownService.cs`
- `Server/AgentServer/PacketHandler/CharacterActions/CharAction.cs`
- `Server/AgentServer/PacketHandler/UI/NewReverse.cs`

Opcodes:
- C->S `0x705A`: teleport use.
- C->S `0x7074`: character action.
- C->S `0x3053`: get up request.
- C->S `0x70A7`: berserk.
- C->S `0x201F`, `0x181C`, `0x200C`, `0x200D`: new reverse features.

Tables:
- `_RefMapSettings`
- `_RefHideSkillEffect`
- `_blocked_skill`
- `SkillControl`
- `teleport_control`
- `_ControlTeleport`
- `_NewReverseSavedLocations`

Stored procedures:
- `_OnTeleportControl_EDIT`
- `_OnCharacterGetUp_EDIT`

Panel module:
- Regions & Rules.

Admin UI:
- Region selector.
- Blocked skill selector.
- Teleport rule editor.
- Map settings.
- Character saved locations view.

Risks:
- Incorrect region or skill IDs can break gameplay.
- Teleport rules can strand players.

Reload:
- RefManager reload or restart unless runtime commands exist.

## Macro Settings

Source:
- `CustomUIPackets.cs`

Opcode:
- C->S `0x187E`: update macro setting.

Tables:
- `_MacroSetting`
- `_MacroAutoPotion`

Settings:
- `Macro`
- `ShowGuideMacro`

Panel module:
- Macro Support.

Admin UI:
- Enable/disable macro feature.
- View per-character macro config only for support/debug.

Do not expose:
- Raw macro slot data as normal admin forms unless decoded into friendly fields.

Risks:
- Incorrect macro config can create client-side errors or unwanted automated actions.

## Fellow / Pet Systems

Source:
- `CustomUIPackets.cs`
- `Server/AgentServer/PacketHandler/COS/COSPackets.cs`
- `RefManager.cs`

Opcodes:
- C->S `0x189A`: save fellow skill.
- C->S `0x189B`: use fellow skill.
- S->C custom `0x189C`, `0x189D`: related responses.
- S->C `0x30C8`, `0x30C9`, `0xB0CB`: COS updates.
- C->S `0x70C6`, `0x70CB`, `0x7116`: COS actions.

Tables:
- `_FellowSkillData`
- `_RefFellowPetSystem`
- `_RefFellowPetRefObjID`

Stored procedures:
- `_GetFellowPetID64`
- `_HandleFellowData`

Panel module:
- Pets & Fellows.

Admin UI:
- View pet/fellow configs and skills.
- Support diagnostics by character.

Risks:
- Pet runtime data is sensitive. Avoid direct edits unless workflow is fully understood.

## Party Systems

Source:
- `Server/AgentServer/PacketHandler/Party/PartyData.cs`
- `CustomUIPackets.cs`

Opcodes:
- S->C `0xB067`, `0xB060`, `0xB069`, `0x3065`, `0x3864`.
- C->S `0x7069`, `0x7060`, `0x706D`, `0x7062`, `0x7061`, `0x7063`.
- C->S `0x185A`, `0x185C`: party member viewer.
- C->S `0xB299`: party ping.

Tables:
- `_PartyData`

Panel module:
- Party Diagnostics.

Admin UI:
- View active party cache if stored.
- Search player party state.

Risks:
- Usually view-only. Runtime party manipulation can disrupt gameplay.

## Alchemy

Source:
- `Server/AgentServer/PacketHandler/Alchemy/AlchemyPackets.cs`

Opcodes:
- C->S `0x7150`: reinforce.
- C->S `0xB300`: new alchemy.
- S->C `0xB150`: reinforce response.
- S->C `0x5017`: new alchemy result.
- S->C `0x5034`: alchemy link.

Stored procedure:
- `_OnAlchemySuccess_EDIT` on success path.

Settings:
- `OldAlchemy`
- `NewAlchemy`
- `PermanentAlchemy`
- `MaxPlus`
- `MaxPlusDevil`
- `AlchemyItemLinkMinLevel`

Panel module:
- Alchemy Rules.

Admin UI:
- Limits and UI toggles.
- Success log if table/proc writes data.

Risks:
- Plus limits affect economy.

## Exploit Fix And Anti-Cheat Systems

Source:
- `Server/AgentServer/PacketHandler/ExploitFixPackets.cs`
- `Startup.cs`
- `PacketHandler/PacketHandler.cs`

Opcodes:
- C->S `0x3012`: game ready.
- C->S `0x34A9`: magic option grant exploit.
- C->S `0x7005`: logout crash exploit.
- C->S `0x70A2`: skill mastery exploit.
- C->S `0x3510`: game server crash exploit.
- C->S `0x7450`: shard exploit.
- C->S `0x7158`: config update.
- S->C `0xA103`: agent auth.
- S->C `0x3020`: celestial/character unique id.

Panel module:
- Security Events.

Admin UI:
- Security settings.
- Recent violations from logs if parsed.
- Packet rule status.

Risks:
- Must not let admins disable critical exploit fixes casually.
- Any disable action requires high permission and confirmation.

## WebViewer Buttons

Source:
- `Features/WebViewer/WebViewerManager.cs`
- Migration `20260624_webviewer_buttons.sql`

Table:
- `dbo._WebViewerButtons`

Settings:
- `ShowGuideWebViewer`

Behavior:
- Loads enabled buttons ordered by display order.
- Empty table should hide dynamic web viewer icons.

Panel module:
- Web Buttons.

Admin UI:
- Add/edit/remove web buttons with icon path, URL, frame size, enabled toggle.
- Preview URL and icon path.

Reload:
- WebViewer manager load timing must be confirmed. If it loads on `A400`/ready event, reconnect may apply changes; otherwise add reload action.

Risks:
- External URLs can be phishing/malware. Validate scheme and require permission.

## Notices

Source:
- `RefManager.LoadNoticesIntoCache()`

Table:
- `__Notices`

Panel module:
- Notices.

Admin UI:
- Edit configured notice strings.
- Send immediate notice through runtime command.

Reload:
- Cached by RefManager. Runtime send notice is immediate through command queue, but text cache edits require reload/restart.

## Special GameServer Custom Systems

Source:
- `gameserver/source/.../KMTGuardCustom`
- `gameserver/source/.../SqlConnection/sqlCon.cpp`
- `JTShard/vSRO-GameServer`

Tables referenced:
- `KMTGuard..___SR_GSSettings`
- `KMTGuard..__AttackRestrictions`
- `_LockedItemList`
- `_ServerFortressDpsInfo`
- `_ServerAutoCapebyWorldID`
- `_ServerAutoCapebyRegionID` or older DB name references.
- `_RefSkillByItemOptLevel`
- `_RefAbilityByItemOptLevel`
- `_TimedItemPlus`
- `_TimedDevillPlus`
- `___CustomNpcInteraction`
- `_RefHideNameRegionsGS`

Panel module:
- GameServer Rules.

Admin UI:
- Only after verifying current DB names and live usage.
- Many references are legacy/hardcoded and must be normalized before exposing.

Risks:
- GameServer C++ uses raw formatted SQL in several places. Any panel operations touching these tables must validate hard.

## Database Migrations Inventory

Migrations found:
- `20260622_secondary_password_security.sql`
- `20260622_self_teleport.sql`
- `20260623_async_filter_commands_indexes.sql`
- `20260624_chat_support_tables.sql`
- `20260624_hwidlist_hwid_normalize_optional.sql`
- `20260624_hwidlist_indexes.sql`
- `20260624_webviewer_buttons.sql`
- `20260625_silk_stall_transactions.sql`
- `20260626_lucky_spin.sql`
- `20260627_guide_icon_visibility_settings.sql`
- `20260627_settings_cleanup_and_catalog.sql`
- `20260627_special_offers.sql`
- `20260627_trade_sell_captcha.sql`

Panel must track which migrations are applied. A diagnostics page should check object existence and invalid values without offering generic edits.

## Proposed Web Panel Modules

### Foundation
- Authentication.
- Permissions.
- Audit log.
- SQL connection health.
- Filter runtime health.
- Lookup services.

### Dashboard
- Gateway/Agent/Download status if runtime API exposes it.
- Online sessions.
- DB status.
- Last errors from `logs/kmtguard-.log`.
- Pending async commands.
- Enabled security systems.
- Current event/schedule status.

### Players
- Character search.
- Account search.
- Online sessions.
- Disconnect action.
- Rewards/chest history.
- Security profile.

### Rewards And Economy
- Send reward.
- Item chest.
- Lucky Spin.
- Special Offers.
- Item Mall.
- Attendance.

### Game Management
- Events.
- Rankings.
- Achievements.
- Titles/icons/colors.
- Regions and rules.
- Teleports/new reverse.

### Security
- Packet rules.
- HWID/IP limits.
- Blocked words/chat moderation.
- Trade captcha.
- Exploit fixes status.
- Login/register/secondary password.

### Runtime Operations
- Safe command queue actions.
- Scheduler.
- Notices.
- Cache reload actions where supported.

### Configuration
- Friendly settings pages grouped by `Category`.
- No raw `__Settings` table view.

### Logs
- Chat logs.
- Lucky Spin logs.
- Special Offer logs.
- Silk Stall transaction logs.
- Admin audit logs.
- Filter error logs.

## Lookup Services Required

The panel must provide selectors instead of ID inputs:
- Character selector from `SRO_VT_SHARD.._Char`.
- Account selector from `SRO_VT_ACCOUNT..TB_User`.
- Item selector from `SRO_VT_SHARD.._RefObjCommon` joined with item refs where possible.
- Mob selector from `_RefObjCommon` / `_RefObjChar`.
- Region selector from known region IDs and map settings.
- Skill selector from shard skill tables when available.
- Icon selector from `_RefIconsMediaPath`.
- Title selector from `_RefTitleNameNew`.
- Event selector from `_RefEventRegister` / `_RefEventSchedule`.
- Reward builder wrapping `_AddItemToChest`.

## Admin API Requirement

Current runtime communication is database-command-queue based. For safer full panel support, add an internal Admin API inside the filter only for operations that cannot be represented safely by existing `_AsyncFilterCommands`.

API requirements:
- Bind to localhost/private IP only.
- API key or signed token.
- Permission-aware action names.
- Rate limited.
- No raw SQL.
- Audit every operation.
- Return reload/restart requirements.

Initial API candidates:
- Health/status.
- Online sessions.
- Reload settings.
- Reload RefManager subsets: Lucky Spin, Special Offers, ranks, achievements, web buttons, settings.
- Tail recent logs.

## Security Rules For Panel Implementation

Every sensitive action requires:
- Authenticated admin.
- Permission check.
- CSRF protection.
- Validation.
- Confirmation dialog.
- DB transaction when writing multiple rows.
- Audit log with admin, IP, action, target, old value, new value, result, error.

High-risk operations:
- Sending rewards.
- Silk changes.
- Account/secondary password reset.
- Runtime disconnect all.
- Packet whitelist/blacklist changes.
- Scheduler/planned procedure changes.
- Security feature disabling.

## Immediate Build Plan

1. Keep existing Laravel project only as framework shell, not as generic SQL panel.
2. Remove/hide generic table CRUD routes from navigation.
3. Add panel-owned tables:
   - admins/users if Laravel auth not enough.
   - roles/permissions.
   - audit logs.
   - idempotency keys for reward actions.
4. Build shared services:
   - `JtguardDb`
   - `ShardDb`
   - `AccountDb`
   - `LookupService`
   - `RuntimeCommandService`
   - `AuditService`
   - `SettingsService`
5. Build first complete modules:
   - Dashboard/health.
   - Settings grouped by friendly categories.
   - Players and online sessions.
   - Rewards/Item Chest.
   - Lucky Spin.
   - Special Offers.
6. Add runtime Admin API only after proving which actions cannot be safely done through existing queue.

## Non-Goals

The panel must not provide:
- Raw table browser.
- Raw SQL editor.
- Generic row insert/update/delete pages.
- Admin-facing internal IDs as primary input.
- Client DLL source editing.
- Opcodes editing without named safe packet-rule workflow.

