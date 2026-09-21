# Agent Opcodes Reference for KMTGuard AI Agents

Purpose: this file is the local working reference for AgentServer packet/opcode work in this repository. Any AI agent or developer touching `filter/KMTGuardnew/KMTGuard` should read this file before changing packet handlers, logs, custom client DLL packet sends, macro logic, stall logic, silk systems, inventory handling, or disconnect debugging.

Source: https://github.com/DummkopfOfHachtenduden/SilkroadDoc/wiki/Agent-packets

Source wiki commit used: `21e8ccdeaecb7ad7d641352ffc78da287377ec9c`

Generated: 2026-06-27 17:44:31

## How To Use This File

- `C -> S` means client to AgentServer. In KMTGuard logs this is usually `Agent packet C->S`.
- `S -> C` means AgentServer to client. In KMTGuard logs this is usually `Agent packet S->C`.
- Treat this as a packet-name map and investigation guide, not as proof that every payload layout matches this server build.
- Before adding a custom opcode, check this file first. Do not reuse a listed opcode unless the packet is intentionally compatible with the original game protocol.
- If a packet causes `receive from server disconnect`, search this file by opcode first, then inspect the linked packet detail in the wiki clone/source if needed.
- Unknown `sro_client.*` names are documented by address/name from the source wiki and should be treated as real reserved protocol traffic.
- Some opcodes are directional duplicates or server request/client response pairs. Always use direction plus gameplay context, not opcode number alone.
- For packet logs, prefer logging `opcode + direction + name + category`; this makes bugs like stall, macro, item use, and login redirect much faster to diagnose.
- For vSRO private-server changes, verify live behavior against KMTGuard logs before blocking, rewriting, or synthesizing a packet.

## Project Hot Zones

These are the areas this repo has recently touched and the packet groups that usually matter:

- Gateway/login handoff: `Authentication`, `Character selection`, `Game`.
- Macro/bot fixes: `Skill`, `Entity`, `Inventory`, `COS`.
- Silk Stall: `Stall`, `Silk`, `Inventory`.
- Lucky Spin and item rewards: `Silk`, `Inventory`, `Game`.
- Trade/job captcha: `Job`, `Inventory`, `Game`.
- Chat filter and global item link crashes: `Chat`, `Inventory`.
- Reverse/teleport scroll crashes: `Inventory`, `Teleport`.

## Focus Table

| Category | C -> S | S -> C | Name | Obsolete | Source |
| --- | --- | --- | --- | --- | --- |
| Authentication | `0x6103` | `0xA103` | `AGENT_AUTH` |  | [wiki](https://github.com/DummkopfOfHachtenduden/SilkroadDoc/wiki/AGENT_AUTH) |
| Character selection | `0x7001` | `0xB001` | `AGENT_CHARACTER_SELECTION_JOIN` |  | [wiki](https://github.com/DummkopfOfHachtenduden/SilkroadDoc/wiki/AGENT_CHARACTER_SELECTION_JOIN) |
| Character selection | `0x7007` | `0xB007` | `AGENT_CHARACTER_SELECTION_ACTION` |  | [wiki](https://github.com/DummkopfOfHachtenduden/SilkroadDoc/wiki/AGENT_CHARACTER_SELECTION_ACTION) |
| Character selection | `0x7450` | `0xB450` | `AGENT_CHARACTER_SELECTION_RENAME` |  | [wiki](https://github.com/DummkopfOfHachtenduden/SilkroadDoc/wiki/AGENT_CHARACTER_SELECTION_RENAME) |
| Chat | `0x7025` | `0xB025` | `AGENT_CHAT` |  | [wiki](https://github.com/DummkopfOfHachtenduden/SilkroadDoc/wiki/AGENT_CHAT) |
| Chat | `None` | `0x3026` | `AGENT_CHAT_UPDATE` |  | [wiki](https://github.com/DummkopfOfHachtenduden/SilkroadDoc/wiki/AGENT_CHAT_UPDATE) |
| Chat | `None` | `0x302D` | `AGENT_CHAT_RESTRICT` |  | [wiki](https://github.com/DummkopfOfHachtenduden/SilkroadDoc/wiki/AGENT_CHAT_RESTRICT) |
| Config | `0x7158` | `None` | `AGENT_CONFIG_UPDATE` |  | [wiki](https://github.com/DummkopfOfHachtenduden/SilkroadDoc/wiki/AGENT_CONFIG_UPDATE) |
| COS (Call On Summons) | `0x70C5` | `0xB0C5` | `AGENT_COS_COMMAND` |  | [wiki](https://github.com/DummkopfOfHachtenduden/SilkroadDoc/wiki/AGENT_COS_COMMAND) |
| COS (Call On Summons) | `0x70C6` | `0xB0C6` | `AGENT_COS_TERMINATE` |  | [wiki](https://github.com/DummkopfOfHachtenduden/SilkroadDoc/wiki/AGENT_COS_TERMINATE) |
| COS (Call On Summons) | `0x70C7` | `0xB0C7` | `sro_client.008A7AC0` |  |  |
| COS (Call On Summons) | `None` | `0x30C8` | `AGENT_COS_INFO` |  | [wiki](https://github.com/DummkopfOfHachtenduden/SilkroadDoc/wiki/AGENT_COS_INFO) |
| COS (Call On Summons) | `None` | `0x30C9` | `AGENT_COS_UPDATE` |  | [wiki](https://github.com/DummkopfOfHachtenduden/SilkroadDoc/wiki/AGENT_COS_UPDATE) |
| COS (Call On Summons) | `None` | `0x30CA` | `AGENT_COS_UPDATE_STATE` |  | [wiki](https://github.com/DummkopfOfHachtenduden/SilkroadDoc/wiki/AGENT_COS_UPDATE_STATE) |
| COS (Call On Summons) | `0x70CB` | `0xB0CB` | `AGENT_COS_UPDATE_RIDESTATE` |  | [wiki](https://github.com/DummkopfOfHachtenduden/SilkroadDoc/wiki/AGENT_COS_UPDATE_RIDESTATE) |
| COS (Call On Summons) | `0x7116` | `0xB116` | `AGENT_COS_UNSUMMON` |  | [wiki](https://github.com/DummkopfOfHachtenduden/SilkroadDoc/wiki/AGENT_COS_UNSUMMON) |
| COS (Call On Summons) | `0x7117` | `0xB117` | `AGENT_COS_NAME` |  | [wiki](https://github.com/DummkopfOfHachtenduden/SilkroadDoc/wiki/AGENT_COS_NAME) |
| COS (Call On Summons) | `0x7420` | `0xB420` | `AGENT_COS_UPDATE_SETTINGS` |  | [wiki](https://github.com/DummkopfOfHachtenduden/SilkroadDoc/wiki/AGENT_COS_UPDATE_SETTINGS) |
| Entity | `0x7021` | `0xB021` | `AGENT_ENTITY_MOVEMENT` |  | [wiki](https://github.com/DummkopfOfHachtenduden/SilkroadDoc/wiki/AGENT_ENTITY_MOVEMENT) |
| Entity | `None` | `0xB070` | `AGENT_ENTITY_SKILL_CAST_BEGIN` |  | [wiki](https://github.com/DummkopfOfHachtenduden/SilkroadDoc/wiki/AGENT_ENTITY_SKILL_CAST_BEGIN) |
| Entity | `None` | `0xB071` | `AGENT_ENTITY_SKILL_CAST_END` |  | [wiki](https://github.com/DummkopfOfHachtenduden/SilkroadDoc/wiki/AGENT_ENTITY_SKILL_CAST_END) |
| Entity | `None` | `0xB0BD` | `AGENT_ENTITY_SKILL_BUFF_ADD` |  | [wiki](https://github.com/DummkopfOfHachtenduden/SilkroadDoc/wiki/AGENT_ENTITY_SKILL_BUFF_ADD) |
| Entity | `None` | `0xB072` | `AGENT_ENTITY_SKILL_BUFF_REMOVE` |  | [wiki](https://github.com/DummkopfOfHachtenduden/SilkroadDoc/wiki/AGENT_ENTITY_SKILL_BUFF_REMOVE) |
| Entity | `None` | `0x30BF` | `AGENT_ENTITY_STATE_UPDATE` |  | [wiki](https://github.com/DummkopfOfHachtenduden/SilkroadDoc/wiki/AGENT_ENTITY_STATE_UPDATE) |
| Exchange | `0x7081` | `0xB081` | `AGENT_EXCHANGE_START` |  | [wiki](https://github.com/DummkopfOfHachtenduden/SilkroadDoc/wiki/AGENT_EXCHANGE_START) |
| Exchange | `0x7082` | `0xB082` | `AGENT_EXCHANGE_CONFIRM` |  | [wiki](https://github.com/DummkopfOfHachtenduden/SilkroadDoc/wiki/AGENT_EXCHANGE_CONFIRM) |
| Exchange | `0x7083` | `0xB083` | `AGENT_EXCHANGE_APPROVE` |  | [wiki](https://github.com/DummkopfOfHachtenduden/SilkroadDoc/wiki/AGENT_EXCHANGE_APPROVE) |
| Exchange | `0x7084` | `0xB084` | `AGENT_EXCHANGE_CANCEL` |  | [wiki](https://github.com/DummkopfOfHachtenduden/SilkroadDoc/wiki/AGENT_EXCHANGE_CANCEL) |
| Exchange | `None` | `0x3085` | `AGENT_EXCHANGE_STARTED` |  | [wiki](https://github.com/DummkopfOfHachtenduden/SilkroadDoc/wiki/AGENT_EXCHANGE_STARTED) |
| Exchange | `None` | `0x3086` | `AGENT_EXCHANGE_CONFIRMED` |  | [wiki](https://github.com/DummkopfOfHachtenduden/SilkroadDoc/wiki/AGENT_EXCHANGE_CONFIRMED) |
| Exchange | `None` | `0x3087` | `AGENT_EXCHANGE_APPROVED` |  | [wiki](https://github.com/DummkopfOfHachtenduden/SilkroadDoc/wiki/AGENT_EXCHANGE_APPROVED) |
| Exchange | `None` | `0x3088` | `AGENT_EXCHANGE_CANCELED` |  | [wiki](https://github.com/DummkopfOfHachtenduden/SilkroadDoc/wiki/AGENT_EXCHANGE_CANCELED) |
| Exchange | `None` | `0x3089` | `AGENT_EXCHANGE_UPDATE` |  | [wiki](https://github.com/DummkopfOfHachtenduden/SilkroadDoc/wiki/AGENT_EXCHANGE_UPDATE) |
| Exchange | `None` | `0x308C` | `AGENT_EXCHANGE_UPDATE_ITEMS` |  | [wiki](https://github.com/DummkopfOfHachtenduden/SilkroadDoc/wiki/AGENT_EXCHANGE_UPDATE_ITEMS) |
| Game | `None` | `0x300C` | `AGENT_GAME_NOTIFY` |  | [wiki](https://github.com/DummkopfOfHachtenduden/SilkroadDoc/wiki/AGENT_GAME_NOTIFY) |
| Game | `0x3012` | `None` | `AGENT_GAME_READY` |  | [wiki](https://github.com/DummkopfOfHachtenduden/SilkroadDoc/wiki/AGENT_GAME_READY) |
| Game | `0x3014` | `None` | `AGENT_GAME_READY` |  | [wiki](https://github.com/DummkopfOfHachtenduden/SilkroadDoc/wiki/AGENT_GAME_READY) |
| Game | `0x3080` | `0x3080` | `AGENT_GAME_INVITE` |  | [wiki](https://github.com/DummkopfOfHachtenduden/SilkroadDoc/wiki/AGENT_GAME_INVITE) |
| Game | `None` | `0x35B5` | `AGENT_GAME_RESET` |  | [wiki](https://github.com/DummkopfOfHachtenduden/SilkroadDoc/wiki/AGENT_GAME_RESET) |
| Game | `0x35B6` | `-` | `AGENT_GAME_RESET_COMPLETE` |  | [wiki](https://github.com/DummkopfOfHachtenduden/SilkroadDoc/wiki/AGENT_GAME_RESET_COMPLETE) |
| Game | `None` | `0x34BE` | `AGENT_GAME_SERVERTIME` |  | [wiki](https://github.com/DummkopfOfHachtenduden/SilkroadDoc/wiki/AGENT_GAME_SERVERTIME) |
| Inventory | `None` | `0x3038` | `AGENT_INVENTORY_ENTITY_EQUIP` |  | [wiki](https://github.com/DummkopfOfHachtenduden/SilkroadDoc/wiki/AGENT_INVENTORY_ENTITY_EQUIP) |
| Inventory | `None` | `0x3039` | `AGENT_INVENTORY_ENTITY_UNEQUIP` |  | [wiki](https://github.com/DummkopfOfHachtenduden/SilkroadDoc/wiki/AGENT_INVENTORY_ENTITY_UNEQUIP) |
| Inventory | `None` | `0x3040` | `AGENT_INVENTORY_UPDATE_ITEM_STATS` |  | [wiki](https://github.com/DummkopfOfHachtenduden/SilkroadDoc/wiki/AGENT_INVENTORY_UPDATE_ITEM_STATS) |
| Inventory | `None` | `0x3041` | `AGENT_INVENTORY_ENTITY_EQUIP_TIMER_START` |  | [wiki](https://github.com/DummkopfOfHachtenduden/SilkroadDoc/wiki/AGENT_INVENTORY_ENTITY_EQUIP_TIMER_START) |
| Inventory | `None` | `0x3042` | `AGENT_INVENTORY_ENTITY_EQUIP_TIMER_STOP` |  | [wiki](https://github.com/DummkopfOfHachtenduden/SilkroadDoc/wiki/AGENT_INVENTORY_ENTITY_EQUIP_TIMER_STOP) |
| Inventory | `None` | `0x3047` | `AGENT_INVENTORY_STORAGE_INFO_BEGIN` |  | [wiki](https://github.com/DummkopfOfHachtenduden/SilkroadDoc/wiki/AGENT_INVENTORY_STORAGE_INFO_BEGIN) |
| Inventory | `None` | `0x3048` | `AGENT_INVENTORY_STORAGE_INFO_END` |  | [wiki](https://github.com/DummkopfOfHachtenduden/SilkroadDoc/wiki/AGENT_INVENTORY_STORAGE_INFO_END) |
| Inventory | `None` | `0x3049` | `AGENT_INVENTORY_STORAGE_INFO_DATA` |  | [wiki](https://github.com/DummkopfOfHachtenduden/SilkroadDoc/wiki/AGENT_INVENTORY_STORAGE_INFO_DATA) |
| Inventory | `None` | `0x3052` | `AGENT_INVENTORY_UPDATE_ITEM_DURABILITY` |  | [wiki](https://github.com/DummkopfOfHachtenduden/SilkroadDoc/wiki/AGENT_INVENTORY_UPDATE_ITEM_DURABILITY) |
| Inventory | `None` | `0x3092` | `AGENT_INVENTORY_UPDATE_SIZE` |  | [wiki](https://github.com/DummkopfOfHachtenduden/SilkroadDoc/wiki/AGENT_INVENTORY_UPDATE_SIZE) |
| Inventory | `None` | `0x3201` | `AGENT_INVENTORY_UPDATE_AMMO` |  | [wiki](https://github.com/DummkopfOfHachtenduden/SilkroadDoc/wiki/AGENT_INVENTORY_UPDATE_AMMO) |
| Inventory | `0x7034` | `0xB034` | `AGENT_INVENTORY_OPERATION` |  | [wiki](https://github.com/DummkopfOfHachtenduden/SilkroadDoc/wiki/AGENT_INVENTORY_OPERATION) |
| Inventory | `0x703C` | `0xB03C` | `AGENT_INVENTORY_STORAGE_OPEN` |  | [wiki](https://github.com/DummkopfOfHachtenduden/SilkroadDoc/wiki/AGENT_INVENTORY_STORAGE_OPEN) |
| Inventory | `0x703E` | `0xB03E` | `AGENT_INVENTORY_ITEM_REPAIR` |  | [wiki](https://github.com/DummkopfOfHachtenduden/SilkroadDoc/wiki/AGENT_INVENTORY_ITEM_REPAIR) |
| Inventory | `0x703F` | `0xB03F` | `sro_client.00880A70` |  |  |
| Inventory | `0x704C` | `0xB04C` | `AGENT_INVENTORY_ITEM_USE` |  | [wiki](https://github.com/DummkopfOfHachtenduden/SilkroadDoc/wiki/AGENT_INVENTORY_ITEM_USE) |
| Job | `None` | `0x30E0` | `AGENT_JOB_UPDATE_PRICE` |  | [wiki](https://github.com/DummkopfOfHachtenduden/SilkroadDoc/wiki/AGENT_JOB_UPDATE_PRICE) |
| Job | `0x70E1` | `0xB0E1` | `AGENT_JOB_JOIN` |  | [wiki](https://github.com/DummkopfOfHachtenduden/SilkroadDoc/wiki/AGENT_JOB_JOIN) |
| Job | `0x70E2` | `0xB0E2` | `AGENT_JOB_LEAVE` |  | [wiki](https://github.com/DummkopfOfHachtenduden/SilkroadDoc/wiki/AGENT_JOB_LEAVE) |
| Job | `0x70E3` | `0xB0E3` | `AGENT_JOB_ALIAS` |  | [wiki](https://github.com/DummkopfOfHachtenduden/SilkroadDoc/wiki/AGENT_JOB_ALIAS) |
| Job | `0x70E4` | `0xB0E4` | `AGENT_JOB_RANKING` |  | [wiki](https://github.com/DummkopfOfHachtenduden/SilkroadDoc/wiki/AGENT_JOB_RANKING) |
| Job | `0x70E5` | `0xB0E5` | `AGENT_JOB_OUTCOME` |  | [wiki](https://github.com/DummkopfOfHachtenduden/SilkroadDoc/wiki/AGENT_JOB_OUTCOME) |
| Job | `0x70E6` | `0xB0E6` | `AGENT_JOB_PREV_INFO` |  | [wiki](https://github.com/DummkopfOfHachtenduden/SilkroadDoc/wiki/AGENT_JOB_PREV_INFO) |
| Job | `None` | `0x30E6` | `AGENT_JOB_UPDATE_EXP` |  | [wiki](https://github.com/DummkopfOfHachtenduden/SilkroadDoc/wiki/AGENT_JOB_UPDATE_EXP) |
| Job | `None` | `0x30E7` | `AGENT_JOB_COS_DISTANCE` |  | [wiki](https://github.com/DummkopfOfHachtenduden/SilkroadDoc/wiki/AGENT_JOB_COS_DISTANCE) |
| Job | `None` | `0x30E8` | `AGENT_JOB_UPDATE_SCALE` |  | [wiki](https://github.com/DummkopfOfHachtenduden/SilkroadDoc/wiki/AGENT_JOB_UPDATE_SCALE) |
| Job | `0x74D4` | `0xB4D4` | `AGENT_JOB_EXPORT_DETAIL` |  | [wiki](https://github.com/DummkopfOfHachtenduden/SilkroadDoc/wiki/AGENT_JOB_EXPORT_DETAIL) |
| Job | `None` | `0x34D5` | `AGENT_JOB_UPDATE_SAFETRADE` |  | [wiki](https://github.com/DummkopfOfHachtenduden/SilkroadDoc/wiki/AGENT_JOB_UPDATE_SAFETRADE) |
| Silk | `0x7118` | `0xB118` | `AGENT_SILK_GACHA_PLAY` |  | [wiki](https://github.com/DummkopfOfHachtenduden/SilkroadDoc/wiki/AGENT_SILK_GACHA_PLAY) |
| Silk | `0x7119` | `0xB119` | `AGENT_SILK_GACHA_EXCHANGE` |  | [wiki](https://github.com/DummkopfOfHachtenduden/SilkroadDoc/wiki/AGENT_SILK_GACHA_EXCHANGE) |
| Silk | `0x711A` | `0xB11A` | `AGENT_SILK_HISTORY` |  | [wiki](https://github.com/DummkopfOfHachtenduden/SilkroadDoc/wiki/AGENT_SILK_HISTORY) |
| Silk | `None` | `0x3120` | `AGENT_SILK_GACHA_ANNOUNCE` |  | [wiki](https://github.com/DummkopfOfHachtenduden/SilkroadDoc/wiki/AGENT_SILK_GACHA_ANNOUNCE) |
| Silk | `0x7121` | `0xB121` | `sro_client.008A1360` |  |  |
| Silk | `None` | `0x3153` | `AGENT_SILK_UPDATE` |  | [wiki](https://github.com/DummkopfOfHachtenduden/SilkroadDoc/wiki/AGENT_SILK_UPDATE) |
| Silk | `None` | `0x3154` | `AGENT_SILK_NOTIFY` |  | [wiki](https://github.com/DummkopfOfHachtenduden/SilkroadDoc/wiki/AGENT_SILK_NOTIFY) |
| Skill | `0x70A1` | `0xB0A1` | `AGENT_SKILL_LEARN` |  | [wiki](https://github.com/DummkopfOfHachtenduden/SilkroadDoc/wiki/AGENT_SKILL_LEARN) |
| Skill | `0x70A2` | `0xB0A2` | `AGENT_SKILL_MASTERY_LEARN` |  | [wiki](https://github.com/DummkopfOfHachtenduden/SilkroadDoc/wiki/AGENT_SKILL_MASTERY_LEARN) |
| Skill | `0x7202` | `0xB202` | `AGENT_SKILL_WITHDRAW` |  | [wiki](https://github.com/DummkopfOfHachtenduden/SilkroadDoc/wiki/AGENT_SKILL_WITHDRAW) |
| Skill | `0x7203` | `0xB203` | `AGENT_SKILL_MASTERY_WITHDRAW` |  | [wiki](https://github.com/DummkopfOfHachtenduden/SilkroadDoc/wiki/AGENT_SKILL_MASTERY_WITHDRAW) |
| Skill | `None` | `0x3204` | `AGENT_SKILL_WITHDRAW_INFO_WND` |  | [wiki](https://github.com/DummkopfOfHachtenduden/SilkroadDoc/wiki/AGENT_SKILL_WITHDRAW_INFO_WND) |
| Stall | `0x70B1` | `0xB0B1` | `AGENT_STALL_CREATE` |  | [wiki](https://github.com/DummkopfOfHachtenduden/SilkroadDoc/wiki/AGENT_STALL_CREATE) |
| Stall | `0x70B2` | `0xB0B2` | `AGENT_STALL_DESTROY` |  | [wiki](https://github.com/DummkopfOfHachtenduden/SilkroadDoc/wiki/AGENT_STALL_DESTROY) |
| Stall | `0x70B3` | `0xB0B3` | `AGENT_STALL_TALK` |  | [wiki](https://github.com/DummkopfOfHachtenduden/SilkroadDoc/wiki/AGENT_STALL_TALK) |
| Stall | `0x70B4` | `0xB0B4` | `AGENT_STALL_BUY` |  | [wiki](https://github.com/DummkopfOfHachtenduden/SilkroadDoc/wiki/AGENT_STALL_BUY) |
| Stall | `0x70B5` | `0xB0B5` | `AGENT_STALL_LEAVE` |  | [wiki](https://github.com/DummkopfOfHachtenduden/SilkroadDoc/wiki/AGENT_STALL_LEAVE) |
| Stall | `None` | `0x30B7` | `AGENT_STALL_ENTITY_ACTION` |  | [wiki](https://github.com/DummkopfOfHachtenduden/SilkroadDoc/wiki/AGENT_STALL_ENTITY_ACTION) |
| Stall | `None` | `0x30B8` | `AGENT_STALL_ENTITY_CREATE` |  | [wiki](https://github.com/DummkopfOfHachtenduden/SilkroadDoc/wiki/AGENT_STALL_ENTITY_CREATE) |
| Stall | `None` | `0x30B9` | `AGENT_STALL_ENTITY_DESTROY` |  | [wiki](https://github.com/DummkopfOfHachtenduden/SilkroadDoc/wiki/AGENT_STALL_ENTITY_DESTROY) |
| Stall | `0x70BA` | `0xB0BA` | `AGENT_STALL_UPDATE` |  | [wiki](https://github.com/DummkopfOfHachtenduden/SilkroadDoc/wiki/AGENT_STALL_UPDATE) |
| Stall | `None` | `0x30BB` | `AGENT_STALL_ENTITY_NAME` |  | [wiki](https://github.com/DummkopfOfHachtenduden/SilkroadDoc/wiki/AGENT_STALL_ENTITY_NAME) |
| Teleport | `0x7059` | `0xB059` | `AGENT_TELEPORT_DESIGNATE` |  | [wiki](https://github.com/DummkopfOfHachtenduden/SilkroadDoc/wiki/AGENT_TELEPORT_DESIGNATE) |
| Teleport | `0x705A` | `0xB05A` | `AGENT_TELEPORT_USE` |  | [wiki](https://github.com/DummkopfOfHachtenduden/SilkroadDoc/wiki/AGENT_TELEPORT_USE) |
| Teleport | `0x705B` | `0xB05B` | `AGENT_TELEPORT_CANCEL` |  | [wiki](https://github.com/DummkopfOfHachtenduden/SilkroadDoc/wiki/AGENT_TELEPORT_CANCEL) |

## Complete Agent Packet Table

| Category | C -> S | S -> C | Name | Obsolete | Source |
| --- | --- | --- | --- | --- | --- |
| Academy | `0x7470` | `0xB470` | `AGENT_ACADEMY_CREATE` |  | [wiki](https://github.com/DummkopfOfHachtenduden/SilkroadDoc/wiki/AGENT_ACADEMY_CREATE) |
| Academy | `0x7471` | `0xB471` | `AGENT_ACADEMY_DISBAND` |  | [wiki](https://github.com/DummkopfOfHachtenduden/SilkroadDoc/wiki/AGENT_ACADEMY_DISBAND) |
| Academy | `0x7472` | `0xB472` | `sro_client.008997A0` |  |  |
| Academy | `0x7473` | `0xB473` | `AGENT_ACADEMY_KICK` |  | [wiki](https://github.com/DummkopfOfHachtenduden/SilkroadDoc/wiki/AGENT_ACADEMY_KICK) |
| Academy | `0x7474` | `0xB474` | `AGENT_ACADEMY_LEAVE` |  | [wiki](https://github.com/DummkopfOfHachtenduden/SilkroadDoc/wiki/AGENT_ACADEMY_LEAVE) |
| Academy | `0x7475` | `0xB475` | `AGENT_ACADEMY_GRADE` |  | [wiki](https://github.com/DummkopfOfHachtenduden/SilkroadDoc/wiki/AGENT_ACADEMY_GRADE) |
| Academy | `0x7476` | `0xB476` | `sro_client.008998F0` |  |  |
| Academy | `0x7477` | `0xB477` | `AGENT_ACADEMY_UPDATE_COMMENT` |  | [wiki](https://github.com/DummkopfOfHachtenduden/SilkroadDoc/wiki/AGENT_ACADEMY_UPDATE_COMMENT) |
| Academy | `0x7478` | `0xB478` | `AGENT_ACADEMY_HONOR_RANK` |  | [wiki](https://github.com/DummkopfOfHachtenduden/SilkroadDoc/wiki/AGENT_ACADEMY_HONOR_RANK) |
| Academy | `0x747A` | `0xB47A` | `AGENT_ACADEMY_MATCHING_REGISTER` |  | [wiki](https://github.com/DummkopfOfHachtenduden/SilkroadDoc/wiki/AGENT_ACADEMY_MATCHING_REGISTER) |
| Academy | `0x747B` | `0xB47B` | `AGENT_ACADEMY_MATCHING_CHANGE` |  | [wiki](https://github.com/DummkopfOfHachtenduden/SilkroadDoc/wiki/AGENT_ACADEMY_MATCHING_CHANGE) |
| Academy | `0x747C` | `0xB47C` | `AGENT_ACADEMY_MATCHING_DELETE` |  | [wiki](https://github.com/DummkopfOfHachtenduden/SilkroadDoc/wiki/AGENT_ACADEMY_MATCHING_DELETE) |
| Academy | `0x747D` | `0xB47D` | `AGENT_ACADEMY_MATCHING_LIST` |  | [wiki](https://github.com/DummkopfOfHachtenduden/SilkroadDoc/wiki/AGENT_ACADEMY_MATCHING_LIST) |
| Academy | `0x747E` | `0xB47E` | `AGENT_ACADEMY_MATCHING_JOIN` |  | [wiki](https://github.com/DummkopfOfHachtenduden/SilkroadDoc/wiki/AGENT_ACADEMY_MATCHING_JOIN) |
| Academy | `None` | `0x747E` | `AGENT_ACADEMY_MATCHING_REQUEST` |  | [wiki](https://github.com/DummkopfOfHachtenduden/SilkroadDoc/wiki/AGENT_ACADEMY_MATCHING_REQUEST) |
| Academy | `0x347F` | `None` | `AGENT_ACADEMY_MATCHING_RESPONSE` |  | [wiki](https://github.com/DummkopfOfHachtenduden/SilkroadDoc/wiki/AGENT_ACADEMY_MATCHING_RESPONSE) |
| Academy | `0x7483` | `0xB483` | `sro_client.0089B7B0` |  |  |
| Academy | `None` | `0x3C80` | `AGENT_ACADEMY_UPDATE` |  | [wiki](https://github.com/DummkopfOfHachtenduden/SilkroadDoc/wiki/AGENT_ACADEMY_UPDATE) |
| Academy | `None` | `0x3C81` | `AGENT_ACADEMY_INFO` |  | [wiki](https://github.com/DummkopfOfHachtenduden/SilkroadDoc/wiki/AGENT_ACADEMY_INFO) |
| Academy | `None` | `0x3C82` | `AGENT_ACADEMY_UPDATE_BUFF` |  | [wiki](https://github.com/DummkopfOfHachtenduden/SilkroadDoc/wiki/AGENT_ACADEMY_UPDATE_BUFF) |
| Academy | `None` | `0x3C86` | `sro_client.0089BF00` |  |  |
| Academy | `None` | `0x3C87` | `sro_client.0089BAE0` |  |  |
| Alchemy | `0x7150` | `0xB150` | `AGENT_ALCHEMY_REINFORCE` |  | [wiki](https://github.com/DummkopfOfHachtenduden/SilkroadDoc/wiki/AGENT_ALCHEMY_REINFORCE) |
| Alchemy | `0x7151` | `0xB151` | `AGENT_ALCHEMY_ENCHANT` |  | [wiki](https://github.com/DummkopfOfHachtenduden/SilkroadDoc/wiki/AGENT_ALCHEMY_ENCHANT) |
| Alchemy | `0x7155` | `0xB155` | `AGENT_ALCHEMY_MANUFACTURE` |  | [wiki](https://github.com/DummkopfOfHachtenduden/SilkroadDoc/wiki/AGENT_ALCHEMY_MANUFACTURE) |
| Alchemy | `None` | `0x3156` | `AGENT_ALCHEMY_CANCELED` |  | [wiki](https://github.com/DummkopfOfHachtenduden/SilkroadDoc/wiki/AGENT_ALCHEMY_CANCELED) |
| Alchemy | `0x7157` | `0xB157` | `AGENT_ALCHEMY_DISMANTLE` |  | [wiki](https://github.com/DummkopfOfHachtenduden/SilkroadDoc/wiki/AGENT_ALCHEMY_DISMANTLE) |
| Alchemy | `0x716A` | `0xB16A` | `AGENT_ALCHEMY_SOCKET` |  | [wiki](https://github.com/DummkopfOfHachtenduden/SilkroadDoc/wiki/AGENT_ALCHEMY_SOCKET) |
| Authentication | `0x6103` | `0xA103` | `AGENT_AUTH` |  | [wiki](https://github.com/DummkopfOfHachtenduden/SilkroadDoc/wiki/AGENT_AUTH) |
| Battle Arena | `None` | `0x34D2` | `AGENT_BARENA_OPERATION` |  | [wiki](https://github.com/DummkopfOfHachtenduden/SilkroadDoc/wiki/AGENT_BARENA_OPERATION) |
| Battle Arena | `0x74D3` | `None` | `AGENT_BARENA_REQUEST` |  | [wiki](https://github.com/DummkopfOfHachtenduden/SilkroadDoc/wiki/AGENT_BARENA_REQUEST) |
| CAS (Customer Advisory Service) | `0x6314` | `0xA314` | `AGENT_CAS_CLIENT` |  | [wiki](https://github.com/DummkopfOfHachtenduden/SilkroadDoc/wiki/AGENT_CAS_CLIENT) |
| CAS (Customer Advisory Service) | `None` | `0x6315` | `AGENT_CAS_SERVER_REQUEST` |  | [wiki](https://github.com/DummkopfOfHachtenduden/SilkroadDoc/wiki/AGENT_CAS_SERVER_REQUEST) |
| CAS (Customer Advisory Service) | `0x6316` | `None` | `AGENT_CAS_SERVER_RESPONSE` |  | [wiki](https://github.com/DummkopfOfHachtenduden/SilkroadDoc/wiki/AGENT_CAS_SERVER_RESPONSE) |
| Character | `0x70A7` | `None` | `AGENT_CHARACTER_BODYSTATE_REQUEST` |  | [wiki](https://github.com/DummkopfOfHachtenduden/SilkroadDoc/wiki/AGENT_CHARACTER_BODYSTATE_REQUEST) |
| Character | `None` | `0x304E` | `AGENT_CHARACTER_INFO_UPDATE` |  | [wiki](https://github.com/DummkopfOfHachtenduden/SilkroadDoc/wiki/AGENT_CHARACTER_INFO_UPDATE) |
| Character | `None` | `0x3056` | `AGENT_CHARACTER_EXP_UPDATE` |  | [wiki](https://github.com/DummkopfOfHachtenduden/SilkroadDoc/wiki/AGENT_CHARACTER_EXP_UPDATE) |
| Character selection | `0x7001` | `0xB001` | `AGENT_CHARACTER_SELECTION_JOIN` |  | [wiki](https://github.com/DummkopfOfHachtenduden/SilkroadDoc/wiki/AGENT_CHARACTER_SELECTION_JOIN) |
| Character selection | `0x7007` | `0xB007` | `AGENT_CHARACTER_SELECTION_ACTION` |  | [wiki](https://github.com/DummkopfOfHachtenduden/SilkroadDoc/wiki/AGENT_CHARACTER_SELECTION_ACTION) |
| Character selection | `0x7450` | `0xB450` | `AGENT_CHARACTER_SELECTION_RENAME` |  | [wiki](https://github.com/DummkopfOfHachtenduden/SilkroadDoc/wiki/AGENT_CHARACTER_SELECTION_RENAME) |
| Chat | `0x7025` | `0xB025` | `AGENT_CHAT` |  | [wiki](https://github.com/DummkopfOfHachtenduden/SilkroadDoc/wiki/AGENT_CHAT) |
| Chat | `None` | `0x3026` | `AGENT_CHAT_UPDATE` |  | [wiki](https://github.com/DummkopfOfHachtenduden/SilkroadDoc/wiki/AGENT_CHAT_UPDATE) |
| Chat | `None` | `0x302D` | `AGENT_CHAT_RESTRICT` |  | [wiki](https://github.com/DummkopfOfHachtenduden/SilkroadDoc/wiki/AGENT_CHAT_RESTRICT) |
| Community | `0x7302` | `0xB302` | `AGENT_COMMUNITY_FRIEND_ADD` |  | [wiki](https://github.com/DummkopfOfHachtenduden/SilkroadDoc/wiki/AGENT_COMMUNITY_FRIEND_ADD) |
| Community | `None` | `0x7302` | `AGENT_COMMUNITY_FRIEND_REQUEST` |  | [wiki](https://github.com/DummkopfOfHachtenduden/SilkroadDoc/wiki/AGENT_COMMUNITY_FRIEND_REQUEST) |
| Community | `0x3303` | `None` | `AGENT_COMMUNITY_FRIEND_RESPONSE` |  | [wiki](https://github.com/DummkopfOfHachtenduden/SilkroadDoc/wiki/AGENT_COMMUNITY_FRIEND_RESPONSE) |
| Community | `0x7304` | `0xB304` | `AGENT_COMMUNITY_FRIEND_DELETE` |  | [wiki](https://github.com/DummkopfOfHachtenduden/SilkroadDoc/wiki/AGENT_COMMUNITY_FRIEND_DELETE) |
| Community | `None` | `0x3305` | `AGENT_COMMUNITY_FRIEND_INFO` |  | [wiki](https://github.com/DummkopfOfHachtenduden/SilkroadDoc/wiki/AGENT_COMMUNITY_FRIEND_INFO) |
| Community | `0x7308` | `0xB308` | `AGENT_COMMUNITY_MEMO_OPEN` |  | [wiki](https://github.com/DummkopfOfHachtenduden/SilkroadDoc/wiki/AGENT_COMMUNITY_MEMO_OPEN) |
| Community | `0x7309` | `0xB309` | `AGENT_COMMUNITY_MEMO_SEND` |  | [wiki](https://github.com/DummkopfOfHachtenduden/SilkroadDoc/wiki/AGENT_COMMUNITY_MEMO_SEND) |
| Community | `0x730A` | `0xB30A` | `AGENT_COMMUNITY_MEMO_DELETE` |  | [wiki](https://github.com/DummkopfOfHachtenduden/SilkroadDoc/wiki/AGENT_COMMUNITY_MEMO_DELETE) |
| Community | `0x730B` | `0xB30B` | `AGENT_COMMUNITY_MEMO_LIST` |  | [wiki](https://github.com/DummkopfOfHachtenduden/SilkroadDoc/wiki/AGENT_COMMUNITY_MEMO_LIST) |
| Community | `0x730C` | `0xB30C` | `AGENT_COMMUNITY_MEMO_SEND_GROUP` |  | [wiki](https://github.com/DummkopfOfHachtenduden/SilkroadDoc/wiki/AGENT_COMMUNITY_MEMO_SEND_GROUP) |
| Community | `0x730D` | `0xB30D` | `AGENT_COMMUNITY_BLOCK` |  | [wiki](https://github.com/DummkopfOfHachtenduden/SilkroadDoc/wiki/AGENT_COMMUNITY_BLOCK) |
| Config | `0x7158` | `None` | `AGENT_CONFIG_UPDATE` |  | [wiki](https://github.com/DummkopfOfHachtenduden/SilkroadDoc/wiki/AGENT_CONFIG_UPDATE) |
| Consignment | `0x7506` | `0xB506` | `AGENT_CONSIGNMENT_DETAIL` |  | [wiki](https://github.com/DummkopfOfHachtenduden/SilkroadDoc/wiki/AGENT_CONSIGNMENT_DETAIL) |
| Consignment | `0x7507` | `0xB507` | `AGENT_CONSIGNMENT_CLOSE` |  | [wiki](https://github.com/DummkopfOfHachtenduden/SilkroadDoc/wiki/AGENT_CONSIGNMENT_CLOSE) |
| Consignment | `0x7508` | `0xB508` | `AGENT_CONSIGNMENT_REGISTER` |  | [wiki](https://github.com/DummkopfOfHachtenduden/SilkroadDoc/wiki/AGENT_CONSIGNMENT_REGISTER) |
| Consignment | `0x7509` | `0xB509` | `AGENT_CONSIGNMENT_UNREGISTER` |  | [wiki](https://github.com/DummkopfOfHachtenduden/SilkroadDoc/wiki/AGENT_CONSIGNMENT_UNREGISTER) |
| Consignment | `0x750A` | `0xB50A` | `AGENT_CONSIGNMENT_BUY` |  | [wiki](https://github.com/DummkopfOfHachtenduden/SilkroadDoc/wiki/AGENT_CONSIGNMENT_BUY) |
| Consignment | `0x750B` | `0xB50B` | `AGENT_CONSIGNMENT_SETTLE` |  | [wiki](https://github.com/DummkopfOfHachtenduden/SilkroadDoc/wiki/AGENT_CONSIGNMENT_SETTLE) |
| Consignment | `0x750C` | `0xB50C` | `AGENT_CONSIGNMENT_SEARCH` |  | [wiki](https://github.com/DummkopfOfHachtenduden/SilkroadDoc/wiki/AGENT_CONSIGNMENT_SEARCH) |
| Consignment | `None` | `0x350D` | `AGENT_CONSIGNMENT_UPDATE` |  | [wiki](https://github.com/DummkopfOfHachtenduden/SilkroadDoc/wiki/AGENT_CONSIGNMENT_UPDATE) |
| Consignment | `0x750E` | `0xB50E` | `AGENT_CONSIGNMENT_LIST` |  | [wiki](https://github.com/DummkopfOfHachtenduden/SilkroadDoc/wiki/AGENT_CONSIGNMENT_LIST) |
| Consignment | `None` | `0x3530` | `AGENT_CONSIGNMENT_BUFF_ADD` |  | [wiki](https://github.com/DummkopfOfHachtenduden/SilkroadDoc/wiki/AGENT_CONSIGNMENT_BUFF_ADD) |
| Consignment | `None` | `0x3531` | `AGENT_CONSIGNMENT_BUFF_REMOVE` |  | [wiki](https://github.com/DummkopfOfHachtenduden/SilkroadDoc/wiki/AGENT_CONSIGNMENT_BUFF_REMOVE) |
| Consignment | `None` | `0x3532` | `AGENT_CONSIGNMENT_BUFF_UPDATE` |  | [wiki](https://github.com/DummkopfOfHachtenduden/SilkroadDoc/wiki/AGENT_CONSIGNMENT_BUFF_UPDATE) |
| COS (Call On Summons) | `0x70C5` | `0xB0C5` | `AGENT_COS_COMMAND` |  | [wiki](https://github.com/DummkopfOfHachtenduden/SilkroadDoc/wiki/AGENT_COS_COMMAND) |
| COS (Call On Summons) | `0x70C6` | `0xB0C6` | `AGENT_COS_TERMINATE` |  | [wiki](https://github.com/DummkopfOfHachtenduden/SilkroadDoc/wiki/AGENT_COS_TERMINATE) |
| COS (Call On Summons) | `0x70C7` | `0xB0C7` | `sro_client.008A7AC0` |  |  |
| COS (Call On Summons) | `None` | `0x30C8` | `AGENT_COS_INFO` |  | [wiki](https://github.com/DummkopfOfHachtenduden/SilkroadDoc/wiki/AGENT_COS_INFO) |
| COS (Call On Summons) | `None` | `0x30C9` | `AGENT_COS_UPDATE` |  | [wiki](https://github.com/DummkopfOfHachtenduden/SilkroadDoc/wiki/AGENT_COS_UPDATE) |
| COS (Call On Summons) | `None` | `0x30CA` | `AGENT_COS_UPDATE_STATE` |  | [wiki](https://github.com/DummkopfOfHachtenduden/SilkroadDoc/wiki/AGENT_COS_UPDATE_STATE) |
| COS (Call On Summons) | `0x70CB` | `0xB0CB` | `AGENT_COS_UPDATE_RIDESTATE` |  | [wiki](https://github.com/DummkopfOfHachtenduden/SilkroadDoc/wiki/AGENT_COS_UPDATE_RIDESTATE) |
| COS (Call On Summons) | `0x7116` | `0xB116` | `AGENT_COS_UNSUMMON` |  | [wiki](https://github.com/DummkopfOfHachtenduden/SilkroadDoc/wiki/AGENT_COS_UNSUMMON) |
| COS (Call On Summons) | `0x7117` | `0xB117` | `AGENT_COS_NAME` |  | [wiki](https://github.com/DummkopfOfHachtenduden/SilkroadDoc/wiki/AGENT_COS_NAME) |
| COS (Call On Summons) | `0x7420` | `0xB420` | `AGENT_COS_UPDATE_SETTINGS` |  | [wiki](https://github.com/DummkopfOfHachtenduden/SilkroadDoc/wiki/AGENT_COS_UPDATE_SETTINGS) |
| Entity | `0x7021` | `0xB021` | `AGENT_ENTITY_MOVEMENT` |  | [wiki](https://github.com/DummkopfOfHachtenduden/SilkroadDoc/wiki/AGENT_ENTITY_MOVEMENT) |
| Entity | `None` | `0xB070` | `AGENT_ENTITY_SKILL_CAST_BEGIN` |  | [wiki](https://github.com/DummkopfOfHachtenduden/SilkroadDoc/wiki/AGENT_ENTITY_SKILL_CAST_BEGIN) |
| Entity | `None` | `0xB071` | `AGENT_ENTITY_SKILL_CAST_END` |  | [wiki](https://github.com/DummkopfOfHachtenduden/SilkroadDoc/wiki/AGENT_ENTITY_SKILL_CAST_END) |
| Entity | `None` | `0xB0BD` | `AGENT_ENTITY_SKILL_BUFF_ADD` |  | [wiki](https://github.com/DummkopfOfHachtenduden/SilkroadDoc/wiki/AGENT_ENTITY_SKILL_BUFF_ADD) |
| Entity | `None` | `0xB072` | `AGENT_ENTITY_SKILL_BUFF_REMOVE` |  | [wiki](https://github.com/DummkopfOfHachtenduden/SilkroadDoc/wiki/AGENT_ENTITY_SKILL_BUFF_REMOVE) |
| Entity | `None` | `0x30BF` | `AGENT_ENTITY_STATE_UPDATE` |  | [wiki](https://github.com/DummkopfOfHachtenduden/SilkroadDoc/wiki/AGENT_ENTITY_STATE_UPDATE) |
| Environment | `None` | `0x3020` | `AGENT_ENVIRONMENT_CELESTIAL_POSITION` |  | [wiki](https://github.com/DummkopfOfHachtenduden/SilkroadDoc/wiki/AGENT_ENVIRONMENT_CELESTIAL_POSITION) |
| Environment | `None` | `0x3027` | `AGENT_ENVIRONMENT_CELESTIAL_UPDATE` |  | [wiki](https://github.com/DummkopfOfHachtenduden/SilkroadDoc/wiki/AGENT_ENVIRONMENT_CELESTIAL_UPDATE) |
| Environment | `None` | `0x3809` | `AGENT_ENVIRONMENT_WEATHER_UPDATE` |  | [wiki](https://github.com/DummkopfOfHachtenduden/SilkroadDoc/wiki/AGENT_ENVIRONMENT_WEATHER_UPDATE) |
| Exchange | `0x7081` | `0xB081` | `AGENT_EXCHANGE_START` |  | [wiki](https://github.com/DummkopfOfHachtenduden/SilkroadDoc/wiki/AGENT_EXCHANGE_START) |
| Exchange | `0x7082` | `0xB082` | `AGENT_EXCHANGE_CONFIRM` |  | [wiki](https://github.com/DummkopfOfHachtenduden/SilkroadDoc/wiki/AGENT_EXCHANGE_CONFIRM) |
| Exchange | `0x7083` | `0xB083` | `AGENT_EXCHANGE_APPROVE` |  | [wiki](https://github.com/DummkopfOfHachtenduden/SilkroadDoc/wiki/AGENT_EXCHANGE_APPROVE) |
| Exchange | `0x7084` | `0xB084` | `AGENT_EXCHANGE_CANCEL` |  | [wiki](https://github.com/DummkopfOfHachtenduden/SilkroadDoc/wiki/AGENT_EXCHANGE_CANCEL) |
| Exchange | `None` | `0x3085` | `AGENT_EXCHANGE_STARTED` |  | [wiki](https://github.com/DummkopfOfHachtenduden/SilkroadDoc/wiki/AGENT_EXCHANGE_STARTED) |
| Exchange | `None` | `0x3086` | `AGENT_EXCHANGE_CONFIRMED` |  | [wiki](https://github.com/DummkopfOfHachtenduden/SilkroadDoc/wiki/AGENT_EXCHANGE_CONFIRMED) |
| Exchange | `None` | `0x3087` | `AGENT_EXCHANGE_APPROVED` |  | [wiki](https://github.com/DummkopfOfHachtenduden/SilkroadDoc/wiki/AGENT_EXCHANGE_APPROVED) |
| Exchange | `None` | `0x3088` | `AGENT_EXCHANGE_CANCELED` |  | [wiki](https://github.com/DummkopfOfHachtenduden/SilkroadDoc/wiki/AGENT_EXCHANGE_CANCELED) |
| Exchange | `None` | `0x3089` | `AGENT_EXCHANGE_UPDATE` |  | [wiki](https://github.com/DummkopfOfHachtenduden/SilkroadDoc/wiki/AGENT_EXCHANGE_UPDATE) |
| Exchange | `None` | `0x308C` | `AGENT_EXCHANGE_UPDATE_ITEMS` |  | [wiki](https://github.com/DummkopfOfHachtenduden/SilkroadDoc/wiki/AGENT_EXCHANGE_UPDATE_ITEMS) |
| FGW (Forgotten World) | `0x7519` | `0xB519` | `AGENT_FGW_RECALL_LIST` |  | [wiki](https://github.com/DummkopfOfHachtenduden/SilkroadDoc/wiki/AGENT_FGW_RECALL_LIST) |
| FGW (Forgotten World) | `0x751A` | `0xB51A` | `AGENT_FGW_RECALL_MEMBER` |  | [wiki](https://github.com/DummkopfOfHachtenduden/SilkroadDoc/wiki/AGENT_FGW_RECALL_MEMBER) |
| FGW (Forgotten World) | `None` | `0x741A` | `AGENT_FGW_RECALL_REQUEST` |  | [wiki](https://github.com/DummkopfOfHachtenduden/SilkroadDoc/wiki/AGENT_FGW_RECALL_REQUEST) |
| FGW (Forgotten World) | `0x751C` | `0xB51C` | `AGENT_FGW_RECALL_RESPONSE` |  | [wiki](https://github.com/DummkopfOfHachtenduden/SilkroadDoc/wiki/AGENT_FGW_RECALL_RESPONSE) |
| FGW (Forgotten World) | `0x751D` | `0xB51D` | `AGENT_FGW_EXIT` |  | [wiki](https://github.com/DummkopfOfHachtenduden/SilkroadDoc/wiki/AGENT_FGW_EXIT) |
| FGW (Forgotten World) | `None` | `0x351E` | `AGENT_FGW_UPDATE` |  | [wiki](https://github.com/DummkopfOfHachtenduden/SilkroadDoc/wiki/AGENT_FGW_UPDATE) |
| FlagWar (Capture the Flag) | `None` | `0x34B1` | `AGENT_FLAGWAR_UPDATE` |  | [wiki](https://github.com/DummkopfOfHachtenduden/SilkroadDoc/wiki/AGENT_FLAGWAR_UPDATE) |
| FlagWar (Capture the Flag) | `0x74B2` | `None` | `AGENT_FLAGWAR_REGISTER` |  | [wiki](https://github.com/DummkopfOfHachtenduden/SilkroadDoc/wiki/AGENT_FLAGWAR_REGISTER) |
| FRPVP (Free PVP) | `0x7516` | `0xB516` | `AGENT_FRPVP_UPDATE` |  | [wiki](https://github.com/DummkopfOfHachtenduden/SilkroadDoc/wiki/AGENT_FRPVP_UPDATE) |
| Game | `None` | `0x300C` | `AGENT_GAME_NOTIFY` |  | [wiki](https://github.com/DummkopfOfHachtenduden/SilkroadDoc/wiki/AGENT_GAME_NOTIFY) |
| Game | `0x3012` | `None` | `AGENT_GAME_READY` |  | [wiki](https://github.com/DummkopfOfHachtenduden/SilkroadDoc/wiki/AGENT_GAME_READY) |
| Game | `0x3014` | `None` | `AGENT_GAME_READY` |  | [wiki](https://github.com/DummkopfOfHachtenduden/SilkroadDoc/wiki/AGENT_GAME_READY) |
| Game | `0x3080` | `0x3080` | `AGENT_GAME_INVITE` |  | [wiki](https://github.com/DummkopfOfHachtenduden/SilkroadDoc/wiki/AGENT_GAME_INVITE) |
| Game | `None` | `0x35B5` | `AGENT_GAME_RESET` |  | [wiki](https://github.com/DummkopfOfHachtenduden/SilkroadDoc/wiki/AGENT_GAME_RESET) |
| Game | `0x35B6` | `-` | `AGENT_GAME_RESET_COMPLETE` |  | [wiki](https://github.com/DummkopfOfHachtenduden/SilkroadDoc/wiki/AGENT_GAME_RESET_COMPLETE) |
| Game | `None` | `0x34BE` | `AGENT_GAME_SERVERTIME` |  | [wiki](https://github.com/DummkopfOfHachtenduden/SilkroadDoc/wiki/AGENT_GAME_SERVERTIME) |
| Guide | `0x70EA` | `0xB0EA` | `AGENT_GUIDE` |  | [wiki](https://github.com/DummkopfOfHachtenduden/SilkroadDoc/wiki/AGENT_GUIDE) |
| Guild | `None` | `0x30EF` | `AGENT_GUILD_ENTITY_UPDATE_HOSTILITY` |  | [wiki](https://github.com/DummkopfOfHachtenduden/SilkroadDoc/wiki/AGENT_GUILD_ENTITY_UPDATE_HOSTILITY) |
| Guild | `0x70F0` | `0xB0F0` | `AGENT_GUILD_CREATE` |  | [wiki](https://github.com/DummkopfOfHachtenduden/SilkroadDoc/wiki/AGENT_GUILD_CREATE) |
| Guild | `0x70F1` | `0xB0F1` | `AGENT_GUILD_DISBAND` |  | [wiki](https://github.com/DummkopfOfHachtenduden/SilkroadDoc/wiki/AGENT_GUILD_DISBAND) |
| Guild | `0x70F2` | `0xB0F2` | `AGENT_GUILD_LEAVE` |  | [wiki](https://github.com/DummkopfOfHachtenduden/SilkroadDoc/wiki/AGENT_GUILD_LEAVE) |
| Guild | `0x70F3` | `0xB0F3` | `AGENT_GUILD_INVITE` |  | [wiki](https://github.com/DummkopfOfHachtenduden/SilkroadDoc/wiki/AGENT_GUILD_INVITE) |
| Guild | `0x70F4` | `0xB0F4` | `AGENT_GUILD_KICK` |  | [wiki](https://github.com/DummkopfOfHachtenduden/SilkroadDoc/wiki/AGENT_GUILD_KICK) |
| Guild | `None` | `0x38F5` | `AGENT_GUILD_UPDATE` |  | [wiki](https://github.com/DummkopfOfHachtenduden/SilkroadDoc/wiki/AGENT_GUILD_UPDATE) |
| Guild | `0x70F6` | `0xB0F6` | `AGENT_GUILD_DONATE_OBSOLETE` | yes | [wiki](https://github.com/DummkopfOfHachtenduden/SilkroadDoc/wiki/AGENT_GUILD_DONATE_OBSOLETE) |
| Guild | `None` | `0xB0F8` | `sro_client.00881890` |  |  |
| Guild | `0x70F9` | `0xB0F9` | `AGENT_GUILD_UPDATE_NOTICE` |  | [wiki](https://github.com/DummkopfOfHachtenduden/SilkroadDoc/wiki/AGENT_GUILD_UPDATE_NOTICE) |
| Guild | `0x70FA` | `0xB0FA` | `AGENT_GUILD_PROMOTE` |  | [wiki](https://github.com/DummkopfOfHachtenduden/SilkroadDoc/wiki/AGENT_GUILD_PROMOTE) |
| Guild | `0x70FB` | `0xB0FB` | `AGENT_GUILD_UNION_INVITE` |  | [wiki](https://github.com/DummkopfOfHachtenduden/SilkroadDoc/wiki/AGENT_GUILD_UNION_INVITE) |
| Guild | `0x70FC` | `0xB0FC` | `AGENT_GUILD_UNION_LEAVE` |  | [wiki](https://github.com/DummkopfOfHachtenduden/SilkroadDoc/wiki/AGENT_GUILD_UNION_LEAVE) |
| Guild | `0x70FD` | `0xB0FD` | `AGENT_GUILD_UNION_KICK` |  | [wiki](https://github.com/DummkopfOfHachtenduden/SilkroadDoc/wiki/AGENT_GUILD_UNION_KICK) |
| Guild | `0x70FF` | `0xB0FF` | `AGENT_GUILD_UPDATE_SIEGEAUTH` |  | [wiki](https://github.com/DummkopfOfHachtenduden/SilkroadDoc/wiki/AGENT_GUILD_UPDATE_SIEGEAUTH) |
| Guild | `None` | `0x30FF` | `AGENT_GUILD_ENTITY_UPDATE` |  | [wiki](https://github.com/DummkopfOfHachtenduden/SilkroadDoc/wiki/AGENT_GUILD_ENTITY_UPDATE) |
| Guild | `None` | `0x3100` | `AGENT_GUILD_ENTITY_REMOVE` |  | [wiki](https://github.com/DummkopfOfHachtenduden/SilkroadDoc/wiki/AGENT_GUILD_ENTITY_REMOVE) |
| Guild | `None` | `0x34B3` | `AGENT_GUILD_INFO_BEGIN` |  | [wiki](https://github.com/DummkopfOfHachtenduden/SilkroadDoc/wiki/AGENT_GUILD_INFO_BEGIN) |
| Guild | `None` | `0x3101` | `AGENT_GUILD_INFO_DATA` |  | [wiki](https://github.com/DummkopfOfHachtenduden/SilkroadDoc/wiki/AGENT_GUILD_INFO_DATA) |
| Guild | `None` | `0x34B4` | `AGENT_GUILD_INFO_END` |  | [wiki](https://github.com/DummkopfOfHachtenduden/SilkroadDoc/wiki/AGENT_GUILD_INFO_END) |
| Guild | `None` | `0x3102` | `AGENT_GUILD_UNION_INFO` |  | [wiki](https://github.com/DummkopfOfHachtenduden/SilkroadDoc/wiki/AGENT_GUILD_UNION_INFO) |
| Guild | `None` | `0x3103` | `AGENT_GUILD_ENTITY_UPDATE_SIEGEAUTH` |  | [wiki](https://github.com/DummkopfOfHachtenduden/SilkroadDoc/wiki/AGENT_GUILD_ENTITY_UPDATE_SIEGEAUTH) |
| Guild | `0x7103` | `0xB103` | `AGENT_GUILD_TRANSFER` |  | [wiki](https://github.com/DummkopfOfHachtenduden/SilkroadDoc/wiki/AGENT_GUILD_TRANSFER) |
| Guild | `0x7104` | `0xB104` | `AGENT_GUILD_UPDATE_PERMISSION` |  | [wiki](https://github.com/DummkopfOfHachtenduden/SilkroadDoc/wiki/AGENT_GUILD_UPDATE_PERMISSION) |
| Guild | `0x7105` | `0xB105` | `AGENT_GUILD_ELECTION_START` |  | [wiki](https://github.com/DummkopfOfHachtenduden/SilkroadDoc/wiki/AGENT_GUILD_ELECTION_START) |
| Guild | `0x7106` | `0xB106` | `AGENT_GUILD_ELECTION_PARTICIPATE` |  | [wiki](https://github.com/DummkopfOfHachtenduden/SilkroadDoc/wiki/AGENT_GUILD_ELECTION_PARTICIPATE) |
| Guild | `0x7107` | `0xB107` | `AGENT_GUILD_ELECTION_VOTE` |  | [wiki](https://github.com/DummkopfOfHachtenduden/SilkroadDoc/wiki/AGENT_GUILD_ELECTION_VOTE) |
| Guild | `None` | `0x3908` | `AGENT_GUILD_ELECTION_UPDATE` |  | [wiki](https://github.com/DummkopfOfHachtenduden/SilkroadDoc/wiki/AGENT_GUILD_ELECTION_UPDATE) |
| Guild | `None` | `0x3109` | `AGENT_GUILD_WAR_INFO` |  | [wiki](https://github.com/DummkopfOfHachtenduden/SilkroadDoc/wiki/AGENT_GUILD_WAR_INFO) |
| Guild | `0x7110` | `0xB110` | `AGENT_GUILD_WAR_START` |  | [wiki](https://github.com/DummkopfOfHachtenduden/SilkroadDoc/wiki/AGENT_GUILD_WAR_START) |
| Guild | `None` | `0x7110` | `AGENT_GUILD_WAR_REQUEST` |  | [wiki](https://github.com/DummkopfOfHachtenduden/SilkroadDoc/wiki/AGENT_GUILD_WAR_REQUEST) |
| Guild | `0x7112` | `0xB112` | `AGENT_GUILD_WAR_END` |  | [wiki](https://github.com/DummkopfOfHachtenduden/SilkroadDoc/wiki/AGENT_GUILD_WAR_END) |
| Guild | `0x7113` | `0xB113` | `sro_client.00881F80` |  |  |
| Guild | `0x7114` | `0xB114` | `AGENT_GUILD_WAR_REWARD` |  | [wiki](https://github.com/DummkopfOfHachtenduden/SilkroadDoc/wiki/AGENT_GUILD_WAR_REWARD) |
| Guild | `0x7250` | `0xB250` | `AGENT_GUILD_STORAGE_OPEN` |  | [wiki](https://github.com/DummkopfOfHachtenduden/SilkroadDoc/wiki/AGENT_GUILD_STORAGE_OPEN) |
| Guild | `0x7251` | `0xB251` | `AGENT_GUILD_STORAGE_CLOSE` |  | [wiki](https://github.com/DummkopfOfHachtenduden/SilkroadDoc/wiki/AGENT_GUILD_STORAGE_CLOSE) |
| Guild | `0x7252` | `0xB252` | `AGENT_GUILD_STORAGE_LIST` |  | [wiki](https://github.com/DummkopfOfHachtenduden/SilkroadDoc/wiki/AGENT_GUILD_STORAGE_LIST) |
| Guild | `None` | `0x3253` | `AGENT_GUILD_STORAGE_BEGIN` |  | [wiki](https://github.com/DummkopfOfHachtenduden/SilkroadDoc/wiki/AGENT_GUILD_STORAGE_BEGIN) |
| Guild | `None` | `0x3254` | `AGENT_GUILD_STORAGE_END` |  | [wiki](https://github.com/DummkopfOfHachtenduden/SilkroadDoc/wiki/AGENT_GUILD_STORAGE_END) |
| Guild | `None` | `0x3255` | `AGENT_GUILD_STORAGE_DATA` |  | [wiki](https://github.com/DummkopfOfHachtenduden/SilkroadDoc/wiki/AGENT_GUILD_STORAGE_DATA) |
| Guild | `0x7256` | `0xB256` | `AGENT_GUILD_UPDATE_NICKNAME` |  | [wiki](https://github.com/DummkopfOfHachtenduden/SilkroadDoc/wiki/AGENT_GUILD_UPDATE_NICKNAME) |
| Guild | `None` | `0x3256` | `AGENT_GUILD_ENTITY_UPDATE_NICKNAME` |  | [wiki](https://github.com/DummkopfOfHachtenduden/SilkroadDoc/wiki/AGENT_GUILD_ENTITY_UPDATE_NICKNAME) |
| Guild | `None` | `0x3257` | `AGENT_GUILD_ENTITY_UPDATE_CREST` |  | [wiki](https://github.com/DummkopfOfHachtenduden/SilkroadDoc/wiki/AGENT_GUILD_ENTITY_UPDATE_CREST) |
| Guild | `0x7258` | `0x7258` | `AGENT_GUILD_DONATE` |  | [wiki](https://github.com/DummkopfOfHachtenduden/SilkroadDoc/wiki/AGENT_GUILD_DONATE) |
| Guild | `0x7259` | `0x7259` | `AGENT_GUILD_MERCENARY_ATTR` |  | [wiki](https://github.com/DummkopfOfHachtenduden/SilkroadDoc/wiki/AGENT_GUILD_MERCENARY_ATTR) |
| Guild | `0x725A` | `0x725A` | `AGENT_GUILD_MERCENARY_TERMINATE` |  | [wiki](https://github.com/DummkopfOfHachtenduden/SilkroadDoc/wiki/AGENT_GUILD_MERCENARY_TERMINATE) |
| Guild | `0x7501` | `0xB501` | `AGENT_GUILD_GP_HISTORY` |  | [wiki](https://github.com/DummkopfOfHachtenduden/SilkroadDoc/wiki/AGENT_GUILD_GP_HISTORY) |
| Inventory | `None` | `0x3038` | `AGENT_INVENTORY_ENTITY_EQUIP` |  | [wiki](https://github.com/DummkopfOfHachtenduden/SilkroadDoc/wiki/AGENT_INVENTORY_ENTITY_EQUIP) |
| Inventory | `None` | `0x3039` | `AGENT_INVENTORY_ENTITY_UNEQUIP` |  | [wiki](https://github.com/DummkopfOfHachtenduden/SilkroadDoc/wiki/AGENT_INVENTORY_ENTITY_UNEQUIP) |
| Inventory | `None` | `0x3040` | `AGENT_INVENTORY_UPDATE_ITEM_STATS` |  | [wiki](https://github.com/DummkopfOfHachtenduden/SilkroadDoc/wiki/AGENT_INVENTORY_UPDATE_ITEM_STATS) |
| Inventory | `None` | `0x3041` | `AGENT_INVENTORY_ENTITY_EQUIP_TIMER_START` |  | [wiki](https://github.com/DummkopfOfHachtenduden/SilkroadDoc/wiki/AGENT_INVENTORY_ENTITY_EQUIP_TIMER_START) |
| Inventory | `None` | `0x3042` | `AGENT_INVENTORY_ENTITY_EQUIP_TIMER_STOP` |  | [wiki](https://github.com/DummkopfOfHachtenduden/SilkroadDoc/wiki/AGENT_INVENTORY_ENTITY_EQUIP_TIMER_STOP) |
| Inventory | `None` | `0x3047` | `AGENT_INVENTORY_STORAGE_INFO_BEGIN` |  | [wiki](https://github.com/DummkopfOfHachtenduden/SilkroadDoc/wiki/AGENT_INVENTORY_STORAGE_INFO_BEGIN) |
| Inventory | `None` | `0x3048` | `AGENT_INVENTORY_STORAGE_INFO_END` |  | [wiki](https://github.com/DummkopfOfHachtenduden/SilkroadDoc/wiki/AGENT_INVENTORY_STORAGE_INFO_END) |
| Inventory | `None` | `0x3049` | `AGENT_INVENTORY_STORAGE_INFO_DATA` |  | [wiki](https://github.com/DummkopfOfHachtenduden/SilkroadDoc/wiki/AGENT_INVENTORY_STORAGE_INFO_DATA) |
| Inventory | `None` | `0x3052` | `AGENT_INVENTORY_UPDATE_ITEM_DURABILITY` |  | [wiki](https://github.com/DummkopfOfHachtenduden/SilkroadDoc/wiki/AGENT_INVENTORY_UPDATE_ITEM_DURABILITY) |
| Inventory | `None` | `0x3092` | `AGENT_INVENTORY_UPDATE_SIZE` |  | [wiki](https://github.com/DummkopfOfHachtenduden/SilkroadDoc/wiki/AGENT_INVENTORY_UPDATE_SIZE) |
| Inventory | `None` | `0x3201` | `AGENT_INVENTORY_UPDATE_AMMO` |  | [wiki](https://github.com/DummkopfOfHachtenduden/SilkroadDoc/wiki/AGENT_INVENTORY_UPDATE_AMMO) |
| Inventory | `0x7034` | `0xB034` | `AGENT_INVENTORY_OPERATION` |  | [wiki](https://github.com/DummkopfOfHachtenduden/SilkroadDoc/wiki/AGENT_INVENTORY_OPERATION) |
| Inventory | `0x703C` | `0xB03C` | `AGENT_INVENTORY_STORAGE_OPEN` |  | [wiki](https://github.com/DummkopfOfHachtenduden/SilkroadDoc/wiki/AGENT_INVENTORY_STORAGE_OPEN) |
| Inventory | `0x703E` | `0xB03E` | `AGENT_INVENTORY_ITEM_REPAIR` |  | [wiki](https://github.com/DummkopfOfHachtenduden/SilkroadDoc/wiki/AGENT_INVENTORY_ITEM_REPAIR) |
| Inventory | `0x703F` | `0xB03F` | `sro_client.00880A70` |  |  |
| Inventory | `0x704C` | `0xB04C` | `AGENT_INVENTORY_ITEM_USE` |  | [wiki](https://github.com/DummkopfOfHachtenduden/SilkroadDoc/wiki/AGENT_INVENTORY_ITEM_USE) |
| Job | `None` | `0x30E0` | `AGENT_JOB_UPDATE_PRICE` |  | [wiki](https://github.com/DummkopfOfHachtenduden/SilkroadDoc/wiki/AGENT_JOB_UPDATE_PRICE) |
| Job | `0x70E1` | `0xB0E1` | `AGENT_JOB_JOIN` |  | [wiki](https://github.com/DummkopfOfHachtenduden/SilkroadDoc/wiki/AGENT_JOB_JOIN) |
| Job | `0x70E2` | `0xB0E2` | `AGENT_JOB_LEAVE` |  | [wiki](https://github.com/DummkopfOfHachtenduden/SilkroadDoc/wiki/AGENT_JOB_LEAVE) |
| Job | `0x70E3` | `0xB0E3` | `AGENT_JOB_ALIAS` |  | [wiki](https://github.com/DummkopfOfHachtenduden/SilkroadDoc/wiki/AGENT_JOB_ALIAS) |
| Job | `0x70E4` | `0xB0E4` | `AGENT_JOB_RANKING` |  | [wiki](https://github.com/DummkopfOfHachtenduden/SilkroadDoc/wiki/AGENT_JOB_RANKING) |
| Job | `0x70E5` | `0xB0E5` | `AGENT_JOB_OUTCOME` |  | [wiki](https://github.com/DummkopfOfHachtenduden/SilkroadDoc/wiki/AGENT_JOB_OUTCOME) |
| Job | `0x70E6` | `0xB0E6` | `AGENT_JOB_PREV_INFO` |  | [wiki](https://github.com/DummkopfOfHachtenduden/SilkroadDoc/wiki/AGENT_JOB_PREV_INFO) |
| Job | `None` | `0x30E6` | `AGENT_JOB_UPDATE_EXP` |  | [wiki](https://github.com/DummkopfOfHachtenduden/SilkroadDoc/wiki/AGENT_JOB_UPDATE_EXP) |
| Job | `None` | `0x30E7` | `AGENT_JOB_COS_DISTANCE` |  | [wiki](https://github.com/DummkopfOfHachtenduden/SilkroadDoc/wiki/AGENT_JOB_COS_DISTANCE) |
| Job | `None` | `0x30E8` | `AGENT_JOB_UPDATE_SCALE` |  | [wiki](https://github.com/DummkopfOfHachtenduden/SilkroadDoc/wiki/AGENT_JOB_UPDATE_SCALE) |
| Job | `0x74D4` | `0xB4D4` | `AGENT_JOB_EXPORT_DETAIL` |  | [wiki](https://github.com/DummkopfOfHachtenduden/SilkroadDoc/wiki/AGENT_JOB_EXPORT_DETAIL) |
| Job | `None` | `0x34D5` | `AGENT_JOB_UPDATE_SAFETRADE` |  | [wiki](https://github.com/DummkopfOfHachtenduden/SilkroadDoc/wiki/AGENT_JOB_UPDATE_SAFETRADE) |
| Logout | `0x7005` | `0xB005` | `AGENT_LOGOUT` |  | [wiki](https://github.com/DummkopfOfHachtenduden/SilkroadDoc/wiki/AGENT_LOGOUT) |
| Logout | `0x7006` | `0xB006` | `AGENT_LOGOUT_CANCEL` |  | [wiki](https://github.com/DummkopfOfHachtenduden/SilkroadDoc/wiki/AGENT_LOGOUT_CANCEL) |
| Logout | `None` | `0x300A` | `AGENT_LOGUT_SUCCESS` |  | [wiki](https://github.com/DummkopfOfHachtenduden/SilkroadDoc/wiki/AGENT_LOGUT_SUCCESS) |
| MagicOption | `0x34A9` | `0x34AA` | `AGENT_MAGICOPTION_GRANT` |  | [wiki](https://github.com/DummkopfOfHachtenduden/SilkroadDoc/wiki/AGENT_MAGICOPTION_GRANT) |
| Operator | `None` | `0x3405` | `AGENT_OPERATOR_PUNISHMENT` |  | [wiki](https://github.com/DummkopfOfHachtenduden/SilkroadDoc/wiki/AGENT_OPERATOR_PUNISHMENT) |
| Operator | `0x7010` | `0xB010` | `AGENT_OPERATOR_COMMAND` |  | [wiki](https://github.com/DummkopfOfHachtenduden/SilkroadDoc/wiki/AGENT_OPERATOR_COMMAND) |
| Party | `0x7060` | `0xB060` | `AGENT_PARTY_CREATE` |  | [wiki](https://github.com/DummkopfOfHachtenduden/SilkroadDoc/wiki/AGENT_PARTY_CREATE) |
| Party | `0x7061` | `0xB061` | `AGENT_PARTY_LEAVE` | yes | [wiki](https://github.com/DummkopfOfHachtenduden/SilkroadDoc/wiki/AGENT_PARTY_LEAVE) |
| Party | `0x7062` | `0xB062` | `AGENT_PARTY_INVITE` |  | [wiki](https://github.com/DummkopfOfHachtenduden/SilkroadDoc/wiki/AGENT_PARTY_INVITE) |
| Party | `0x7063` | `0xB063` | `AGENT_PARTY_KICK` | yes | [wiki](https://github.com/DummkopfOfHachtenduden/SilkroadDoc/wiki/AGENT_PARTY_KICK) |
| Party | `None` | `0x3864` | `AGENT_PARTY_UPDATE` |  | [wiki](https://github.com/DummkopfOfHachtenduden/SilkroadDoc/wiki/AGENT_PARTY_UPDATE) |
| Party | `None` | `0x3865` | `AGENT_PARTY_CREATED` |  | [wiki](https://github.com/DummkopfOfHachtenduden/SilkroadDoc/wiki/AGENT_PARTY_CREATED) |
| Party | `None` | `0x3065` | `AGENT_PARTY_CREATED_FROM_MATCHING` |  | [wiki](https://github.com/DummkopfOfHachtenduden/SilkroadDoc/wiki/AGENT_PARTY_CREATED_FROM_MATCHING) |
| Party | `None` | `0xB067` | `sro_client.OnJoinPartyAck` |  |  |
| Party | `None` | `0x3068` | `AGENT_PARTY_DISTRIBUTION` |  | [wiki](https://github.com/DummkopfOfHachtenduden/SilkroadDoc/wiki/AGENT_PARTY_DISTRIBUTION) |
| Party | `0x7069` | `0xB069` | `AGENT_PARTY_MATCHING_FORM` |  | [wiki](https://github.com/DummkopfOfHachtenduden/SilkroadDoc/wiki/AGENT_PARTY_MATCHING_FORM) |
| Party | `0x706A` | `0xB06A` | `AGENT_PARTY_MATCHING_CHANGE` |  | [wiki](https://github.com/DummkopfOfHachtenduden/SilkroadDoc/wiki/AGENT_PARTY_MATCHING_CHANGE) |
| Party | `0x706B` | `0xB06B` | `AGENT_PARTY_MATCHING_DELETE` |  | [wiki](https://github.com/DummkopfOfHachtenduden/SilkroadDoc/wiki/AGENT_PARTY_MATCHING_DELETE) |
| Party | `0x706C` | `0xB06C` | `AGENT_PARTY_MATCHING_LIST` |  | [wiki](https://github.com/DummkopfOfHachtenduden/SilkroadDoc/wiki/AGENT_PARTY_MATCHING_LIST) |
| Party | `0x706D` | `0xB06D` | `AGENT_PARTY_MATCHING_JOIN` |  | [wiki](https://github.com/DummkopfOfHachtenduden/SilkroadDoc/wiki/AGENT_PARTY_MATCHING_JOIN) |
| Party | `0x306E` | `0x706D` | `AGENT_PARTY_MATCHING_PLAYER_JOIN` |  | [wiki](https://github.com/DummkopfOfHachtenduden/SilkroadDoc/wiki/AGENT_PARTY_MATCHING_PLAYER_JOIN) |
| PK | `None` | `0x30CD` | `AGENT_PK_UPDATE_PENALTY` |  | [wiki](https://github.com/DummkopfOfHachtenduden/SilkroadDoc/wiki/AGENT_PK_UPDATE_PENALTY) |
| PK | `None` | `0x30CE` | `AGENT_PK_UPDATE_DAILY` |  | [wiki](https://github.com/DummkopfOfHachtenduden/SilkroadDoc/wiki/AGENT_PK_UPDATE_DAILY) |
| PK | `None` | `0x30D3` | `AGENT_PK_UPDATE_LEVEL` |  | [wiki](https://github.com/DummkopfOfHachtenduden/SilkroadDoc/wiki/AGENT_PK_UPDATE_LEVEL) |
| Quest | `0x30D4` | `0x30D4` | `AGENT_QUEST_TALK` |  | [wiki](https://github.com/DummkopfOfHachtenduden/SilkroadDoc/wiki/AGENT_QUEST_TALK) |
| Quest | `None` | `0x30D5` | `AGENT_QUEST_UPDATE` |  | [wiki](https://github.com/DummkopfOfHachtenduden/SilkroadDoc/wiki/AGENT_QUEST_UPDATE) |
| Quest | `None` | `0x30D6` | `AGENT_QUEST_MARK_ADD` |  | [wiki](https://github.com/DummkopfOfHachtenduden/SilkroadDoc/wiki/AGENT_QUEST_MARK_ADD) |
| Quest | `None` | `0x30D7` | `AGENT_QUEST_MARK_REMOVE` |  | [wiki](https://github.com/DummkopfOfHachtenduden/SilkroadDoc/wiki/AGENT_QUEST_MARK_REMOVE) |
| Quest | `0x70D8` | `0xB0D8` | `AGENT_QUEST_DINGDONG` |  | [wiki](https://github.com/DummkopfOfHachtenduden/SilkroadDoc/wiki/AGENT_QUEST_DINGDONG) |
| Quest | `0x70D9` | `0xB0D9` | `AGENT_QUEST_ABANDON` |  | [wiki](https://github.com/DummkopfOfHachtenduden/SilkroadDoc/wiki/AGENT_QUEST_ABANDON) |
| Quest | `None` | `0x30DA` | `AGENT_QUEST_GATHER` |  | [wiki](https://github.com/DummkopfOfHachtenduden/SilkroadDoc/wiki/AGENT_QUEST_GATHER) |
| Quest | `0x70DB` | `0xB0DB` | `AGENT_QUEST_GATHER_CANCEL` |  | [wiki](https://github.com/DummkopfOfHachtenduden/SilkroadDoc/wiki/AGENT_QUEST_GATHER_CANCEL) |
| Quest | `None` | `0x30DC` | `AGENT_QUEST_CAPTURE_RESULT` |  | [wiki](https://github.com/DummkopfOfHachtenduden/SilkroadDoc/wiki/AGENT_QUEST_CAPTURE_RESULT) |
| Quest | `None` | `0x30EC` | `AGENT_QUEST_NOTIFY` |  | [wiki](https://github.com/DummkopfOfHachtenduden/SilkroadDoc/wiki/AGENT_QUEST_NOTIFY) |
| Quest | `None` | `0x3514` | `AGENT_QUEST_REWARD_TALK` |  | [wiki](https://github.com/DummkopfOfHachtenduden/SilkroadDoc/wiki/AGENT_QUEST_REWARD_TALK) |
| Quest | `0x7515` | `0x3515` | `AGENT_QUEST_REWAD_SELECT` |  | [wiki](https://github.com/DummkopfOfHachtenduden/SilkroadDoc/wiki/AGENT_QUEST_REWAD_SELECT) |
| Quest | `None` | `0x3CA2` | `AGENT_QUEST_SCRIPT` |  | [wiki](https://github.com/DummkopfOfHachtenduden/SilkroadDoc/wiki/AGENT_QUEST_SCRIPT) |
| Siege | `0x705D` | `0xB05D` | `AGENT_SIEGE_RETURN` |  | [wiki](https://github.com/DummkopfOfHachtenduden/SilkroadDoc/wiki/AGENT_SIEGE_RETURN) |
| Siege | `0x705E` | `0xB05E` | `AGENT_SIEGE_ACTION` |  | [wiki](https://github.com/DummkopfOfHachtenduden/SilkroadDoc/wiki/AGENT_SIEGE_ACTION) |
| Siege | `None` | `0x385F` | `AGENT_SIEGE_UPDATE` |  | [wiki](https://github.com/DummkopfOfHachtenduden/SilkroadDoc/wiki/AGENT_SIEGE_UPDATE) |
| Silk | `0x7118` | `0xB118` | `AGENT_SILK_GACHA_PLAY` |  | [wiki](https://github.com/DummkopfOfHachtenduden/SilkroadDoc/wiki/AGENT_SILK_GACHA_PLAY) |
| Silk | `0x7119` | `0xB119` | `AGENT_SILK_GACHA_EXCHANGE` |  | [wiki](https://github.com/DummkopfOfHachtenduden/SilkroadDoc/wiki/AGENT_SILK_GACHA_EXCHANGE) |
| Silk | `0x711A` | `0xB11A` | `AGENT_SILK_HISTORY` |  | [wiki](https://github.com/DummkopfOfHachtenduden/SilkroadDoc/wiki/AGENT_SILK_HISTORY) |
| Silk | `None` | `0x3120` | `AGENT_SILK_GACHA_ANNOUNCE` |  | [wiki](https://github.com/DummkopfOfHachtenduden/SilkroadDoc/wiki/AGENT_SILK_GACHA_ANNOUNCE) |
| Silk | `0x7121` | `0xB121` | `sro_client.008A1360` |  |  |
| Silk | `None` | `0x3153` | `AGENT_SILK_UPDATE` |  | [wiki](https://github.com/DummkopfOfHachtenduden/SilkroadDoc/wiki/AGENT_SILK_UPDATE) |
| Silk | `None` | `0x3154` | `AGENT_SILK_NOTIFY` |  | [wiki](https://github.com/DummkopfOfHachtenduden/SilkroadDoc/wiki/AGENT_SILK_NOTIFY) |
| Skill | `0x70A1` | `0xB0A1` | `AGENT_SKILL_LEARN` |  | [wiki](https://github.com/DummkopfOfHachtenduden/SilkroadDoc/wiki/AGENT_SKILL_LEARN) |
| Skill | `0x70A2` | `0xB0A2` | `AGENT_SKILL_MASTERY_LEARN` |  | [wiki](https://github.com/DummkopfOfHachtenduden/SilkroadDoc/wiki/AGENT_SKILL_MASTERY_LEARN) |
| Skill | `0x7202` | `0xB202` | `AGENT_SKILL_WITHDRAW` |  | [wiki](https://github.com/DummkopfOfHachtenduden/SilkroadDoc/wiki/AGENT_SKILL_WITHDRAW) |
| Skill | `0x7203` | `0xB203` | `AGENT_SKILL_MASTERY_WITHDRAW` |  | [wiki](https://github.com/DummkopfOfHachtenduden/SilkroadDoc/wiki/AGENT_SKILL_MASTERY_WITHDRAW) |
| Skill | `None` | `0x3204` | `AGENT_SKILL_WITHDRAW_INFO_WND` |  | [wiki](https://github.com/DummkopfOfHachtenduden/SilkroadDoc/wiki/AGENT_SKILL_WITHDRAW_INFO_WND) |
| Stall | `0x70B1` | `0xB0B1` | `AGENT_STALL_CREATE` |  | [wiki](https://github.com/DummkopfOfHachtenduden/SilkroadDoc/wiki/AGENT_STALL_CREATE) |
| Stall | `0x70B2` | `0xB0B2` | `AGENT_STALL_DESTROY` |  | [wiki](https://github.com/DummkopfOfHachtenduden/SilkroadDoc/wiki/AGENT_STALL_DESTROY) |
| Stall | `0x70B3` | `0xB0B3` | `AGENT_STALL_TALK` |  | [wiki](https://github.com/DummkopfOfHachtenduden/SilkroadDoc/wiki/AGENT_STALL_TALK) |
| Stall | `0x70B4` | `0xB0B4` | `AGENT_STALL_BUY` |  | [wiki](https://github.com/DummkopfOfHachtenduden/SilkroadDoc/wiki/AGENT_STALL_BUY) |
| Stall | `0x70B5` | `0xB0B5` | `AGENT_STALL_LEAVE` |  | [wiki](https://github.com/DummkopfOfHachtenduden/SilkroadDoc/wiki/AGENT_STALL_LEAVE) |
| Stall | `None` | `0x30B7` | `AGENT_STALL_ENTITY_ACTION` |  | [wiki](https://github.com/DummkopfOfHachtenduden/SilkroadDoc/wiki/AGENT_STALL_ENTITY_ACTION) |
| Stall | `None` | `0x30B8` | `AGENT_STALL_ENTITY_CREATE` |  | [wiki](https://github.com/DummkopfOfHachtenduden/SilkroadDoc/wiki/AGENT_STALL_ENTITY_CREATE) |
| Stall | `None` | `0x30B9` | `AGENT_STALL_ENTITY_DESTROY` |  | [wiki](https://github.com/DummkopfOfHachtenduden/SilkroadDoc/wiki/AGENT_STALL_ENTITY_DESTROY) |
| Stall | `0x70BA` | `0xB0BA` | `AGENT_STALL_UPDATE` |  | [wiki](https://github.com/DummkopfOfHachtenduden/SilkroadDoc/wiki/AGENT_STALL_UPDATE) |
| Stall | `None` | `0x30BB` | `AGENT_STALL_ENTITY_NAME` |  | [wiki](https://github.com/DummkopfOfHachtenduden/SilkroadDoc/wiki/AGENT_STALL_ENTITY_NAME) |
| TAP (Temple Area Points) | `0x74DF` | `0xB4DF` | `AGENT_TAP_INFO` |  | [wiki](https://github.com/DummkopfOfHachtenduden/SilkroadDoc/wiki/AGENT_TAP_INFO) |
| TAP (Temple Area Points) | `0x74E0` | `0xB4E0` | `AGENT_TAP_UPDATE` |  | [wiki](https://github.com/DummkopfOfHachtenduden/SilkroadDoc/wiki/AGENT_TAP_UPDATE) |
| TAP (Temple Area Points) | `None` | `0x34E1` | `AGENT_TAP_ICON` |  | [wiki](https://github.com/DummkopfOfHachtenduden/SilkroadDoc/wiki/AGENT_TAP_ICON) |
| Teleport | `0x7059` | `0xB059` | `AGENT_TELEPORT_DESIGNATE` |  | [wiki](https://github.com/DummkopfOfHachtenduden/SilkroadDoc/wiki/AGENT_TELEPORT_DESIGNATE) |
| Teleport | `0x705A` | `0xB05A` | `AGENT_TELEPORT_USE` |  | [wiki](https://github.com/DummkopfOfHachtenduden/SilkroadDoc/wiki/AGENT_TELEPORT_USE) |
| Teleport | `0x705B` | `0xB05B` | `AGENT_TELEPORT_CANCEL` |  | [wiki](https://github.com/DummkopfOfHachtenduden/SilkroadDoc/wiki/AGENT_TELEPORT_CANCEL) |

## Numeric Lookup

Use this section when you only have a log line like `Agent packet C->S 0x70BA`.

| Opcode | Direction | Category | Name | Obsolete |
| --- | --- | --- | --- | --- |
| `0x300A` | `S->C` | Logout | `AGENT_LOGUT_SUCCESS` |  |
| `0x300C` | `S->C` | Game | `AGENT_GAME_NOTIFY` |  |
| `0x3012` | `C->S` | Game | `AGENT_GAME_READY` |  |
| `0x3014` | `C->S` | Game | `AGENT_GAME_READY` |  |
| `0x3020` | `S->C` | Environment | `AGENT_ENVIRONMENT_CELESTIAL_POSITION` |  |
| `0x3026` | `S->C` | Chat | `AGENT_CHAT_UPDATE` |  |
| `0x3027` | `S->C` | Environment | `AGENT_ENVIRONMENT_CELESTIAL_UPDATE` |  |
| `0x302D` | `S->C` | Chat | `AGENT_CHAT_RESTRICT` |  |
| `0x3038` | `S->C` | Inventory | `AGENT_INVENTORY_ENTITY_EQUIP` |  |
| `0x3039` | `S->C` | Inventory | `AGENT_INVENTORY_ENTITY_UNEQUIP` |  |
| `0x3040` | `S->C` | Inventory | `AGENT_INVENTORY_UPDATE_ITEM_STATS` |  |
| `0x3041` | `S->C` | Inventory | `AGENT_INVENTORY_ENTITY_EQUIP_TIMER_START` |  |
| `0x3042` | `S->C` | Inventory | `AGENT_INVENTORY_ENTITY_EQUIP_TIMER_STOP` |  |
| `0x3047` | `S->C` | Inventory | `AGENT_INVENTORY_STORAGE_INFO_BEGIN` |  |
| `0x3048` | `S->C` | Inventory | `AGENT_INVENTORY_STORAGE_INFO_END` |  |
| `0x3049` | `S->C` | Inventory | `AGENT_INVENTORY_STORAGE_INFO_DATA` |  |
| `0x304E` | `S->C` | Character | `AGENT_CHARACTER_INFO_UPDATE` |  |
| `0x3052` | `S->C` | Inventory | `AGENT_INVENTORY_UPDATE_ITEM_DURABILITY` |  |
| `0x3056` | `S->C` | Character | `AGENT_CHARACTER_EXP_UPDATE` |  |
| `0x3065` | `S->C` | Party | `AGENT_PARTY_CREATED_FROM_MATCHING` |  |
| `0x3068` | `S->C` | Party | `AGENT_PARTY_DISTRIBUTION` |  |
| `0x306E` | `C->S` | Party | `AGENT_PARTY_MATCHING_PLAYER_JOIN` |  |
| `0x3080` | `C->S` | Game | `AGENT_GAME_INVITE` |  |
| `0x3080` | `S->C` | Game | `AGENT_GAME_INVITE` |  |
| `0x3085` | `S->C` | Exchange | `AGENT_EXCHANGE_STARTED` |  |
| `0x3086` | `S->C` | Exchange | `AGENT_EXCHANGE_CONFIRMED` |  |
| `0x3087` | `S->C` | Exchange | `AGENT_EXCHANGE_APPROVED` |  |
| `0x3088` | `S->C` | Exchange | `AGENT_EXCHANGE_CANCELED` |  |
| `0x3089` | `S->C` | Exchange | `AGENT_EXCHANGE_UPDATE` |  |
| `0x308C` | `S->C` | Exchange | `AGENT_EXCHANGE_UPDATE_ITEMS` |  |
| `0x3092` | `S->C` | Inventory | `AGENT_INVENTORY_UPDATE_SIZE` |  |
| `0x30B7` | `S->C` | Stall | `AGENT_STALL_ENTITY_ACTION` |  |
| `0x30B8` | `S->C` | Stall | `AGENT_STALL_ENTITY_CREATE` |  |
| `0x30B9` | `S->C` | Stall | `AGENT_STALL_ENTITY_DESTROY` |  |
| `0x30BB` | `S->C` | Stall | `AGENT_STALL_ENTITY_NAME` |  |
| `0x30BF` | `S->C` | Entity | `AGENT_ENTITY_STATE_UPDATE` |  |
| `0x30C8` | `S->C` | COS (Call On Summons) | `AGENT_COS_INFO` |  |
| `0x30C9` | `S->C` | COS (Call On Summons) | `AGENT_COS_UPDATE` |  |
| `0x30CA` | `S->C` | COS (Call On Summons) | `AGENT_COS_UPDATE_STATE` |  |
| `0x30CD` | `S->C` | PK | `AGENT_PK_UPDATE_PENALTY` |  |
| `0x30CE` | `S->C` | PK | `AGENT_PK_UPDATE_DAILY` |  |
| `0x30D3` | `S->C` | PK | `AGENT_PK_UPDATE_LEVEL` |  |
| `0x30D4` | `C->S` | Quest | `AGENT_QUEST_TALK` |  |
| `0x30D4` | `S->C` | Quest | `AGENT_QUEST_TALK` |  |
| `0x30D5` | `S->C` | Quest | `AGENT_QUEST_UPDATE` |  |
| `0x30D6` | `S->C` | Quest | `AGENT_QUEST_MARK_ADD` |  |
| `0x30D7` | `S->C` | Quest | `AGENT_QUEST_MARK_REMOVE` |  |
| `0x30DA` | `S->C` | Quest | `AGENT_QUEST_GATHER` |  |
| `0x30DC` | `S->C` | Quest | `AGENT_QUEST_CAPTURE_RESULT` |  |
| `0x30E0` | `S->C` | Job | `AGENT_JOB_UPDATE_PRICE` |  |
| `0x30E6` | `S->C` | Job | `AGENT_JOB_UPDATE_EXP` |  |
| `0x30E7` | `S->C` | Job | `AGENT_JOB_COS_DISTANCE` |  |
| `0x30E8` | `S->C` | Job | `AGENT_JOB_UPDATE_SCALE` |  |
| `0x30EC` | `S->C` | Quest | `AGENT_QUEST_NOTIFY` |  |
| `0x30EF` | `S->C` | Guild | `AGENT_GUILD_ENTITY_UPDATE_HOSTILITY` |  |
| `0x30FF` | `S->C` | Guild | `AGENT_GUILD_ENTITY_UPDATE` |  |
| `0x3100` | `S->C` | Guild | `AGENT_GUILD_ENTITY_REMOVE` |  |
| `0x3101` | `S->C` | Guild | `AGENT_GUILD_INFO_DATA` |  |
| `0x3102` | `S->C` | Guild | `AGENT_GUILD_UNION_INFO` |  |
| `0x3103` | `S->C` | Guild | `AGENT_GUILD_ENTITY_UPDATE_SIEGEAUTH` |  |
| `0x3109` | `S->C` | Guild | `AGENT_GUILD_WAR_INFO` |  |
| `0x3120` | `S->C` | Silk | `AGENT_SILK_GACHA_ANNOUNCE` |  |
| `0x3153` | `S->C` | Silk | `AGENT_SILK_UPDATE` |  |
| `0x3154` | `S->C` | Silk | `AGENT_SILK_NOTIFY` |  |
| `0x3156` | `S->C` | Alchemy | `AGENT_ALCHEMY_CANCELED` |  |
| `0x3201` | `S->C` | Inventory | `AGENT_INVENTORY_UPDATE_AMMO` |  |
| `0x3204` | `S->C` | Skill | `AGENT_SKILL_WITHDRAW_INFO_WND` |  |
| `0x3253` | `S->C` | Guild | `AGENT_GUILD_STORAGE_BEGIN` |  |
| `0x3254` | `S->C` | Guild | `AGENT_GUILD_STORAGE_END` |  |
| `0x3255` | `S->C` | Guild | `AGENT_GUILD_STORAGE_DATA` |  |
| `0x3256` | `S->C` | Guild | `AGENT_GUILD_ENTITY_UPDATE_NICKNAME` |  |
| `0x3257` | `S->C` | Guild | `AGENT_GUILD_ENTITY_UPDATE_CREST` |  |
| `0x3303` | `C->S` | Community | `AGENT_COMMUNITY_FRIEND_RESPONSE` |  |
| `0x3305` | `S->C` | Community | `AGENT_COMMUNITY_FRIEND_INFO` |  |
| `0x3405` | `S->C` | Operator | `AGENT_OPERATOR_PUNISHMENT` |  |
| `0x347F` | `C->S` | Academy | `AGENT_ACADEMY_MATCHING_RESPONSE` |  |
| `0x34A9` | `C->S` | MagicOption | `AGENT_MAGICOPTION_GRANT` |  |
| `0x34AA` | `S->C` | MagicOption | `AGENT_MAGICOPTION_GRANT` |  |
| `0x34B1` | `S->C` | FlagWar (Capture the Flag) | `AGENT_FLAGWAR_UPDATE` |  |
| `0x34B3` | `S->C` | Guild | `AGENT_GUILD_INFO_BEGIN` |  |
| `0x34B4` | `S->C` | Guild | `AGENT_GUILD_INFO_END` |  |
| `0x34BE` | `S->C` | Game | `AGENT_GAME_SERVERTIME` |  |
| `0x34D2` | `S->C` | Battle Arena | `AGENT_BARENA_OPERATION` |  |
| `0x34D5` | `S->C` | Job | `AGENT_JOB_UPDATE_SAFETRADE` |  |
| `0x34E1` | `S->C` | TAP (Temple Area Points) | `AGENT_TAP_ICON` |  |
| `0x350D` | `S->C` | Consignment | `AGENT_CONSIGNMENT_UPDATE` |  |
| `0x3514` | `S->C` | Quest | `AGENT_QUEST_REWARD_TALK` |  |
| `0x3515` | `S->C` | Quest | `AGENT_QUEST_REWAD_SELECT` |  |
| `0x351E` | `S->C` | FGW (Forgotten World) | `AGENT_FGW_UPDATE` |  |
| `0x3530` | `S->C` | Consignment | `AGENT_CONSIGNMENT_BUFF_ADD` |  |
| `0x3531` | `S->C` | Consignment | `AGENT_CONSIGNMENT_BUFF_REMOVE` |  |
| `0x3532` | `S->C` | Consignment | `AGENT_CONSIGNMENT_BUFF_UPDATE` |  |
| `0x35B5` | `S->C` | Game | `AGENT_GAME_RESET` |  |
| `0x35B6` | `C->S` | Game | `AGENT_GAME_RESET_COMPLETE` |  |
| `0x3809` | `S->C` | Environment | `AGENT_ENVIRONMENT_WEATHER_UPDATE` |  |
| `0x385F` | `S->C` | Siege | `AGENT_SIEGE_UPDATE` |  |
| `0x3864` | `S->C` | Party | `AGENT_PARTY_UPDATE` |  |
| `0x3865` | `S->C` | Party | `AGENT_PARTY_CREATED` |  |
| `0x38F5` | `S->C` | Guild | `AGENT_GUILD_UPDATE` |  |
| `0x3908` | `S->C` | Guild | `AGENT_GUILD_ELECTION_UPDATE` |  |
| `0x3C80` | `S->C` | Academy | `AGENT_ACADEMY_UPDATE` |  |
| `0x3C81` | `S->C` | Academy | `AGENT_ACADEMY_INFO` |  |
| `0x3C82` | `S->C` | Academy | `AGENT_ACADEMY_UPDATE_BUFF` |  |
| `0x3C86` | `S->C` | Academy | `sro_client.0089BF00` |  |
| `0x3C87` | `S->C` | Academy | `sro_client.0089BAE0` |  |
| `0x3CA2` | `S->C` | Quest | `AGENT_QUEST_SCRIPT` |  |
| `0x6103` | `C->S` | Authentication | `AGENT_AUTH` |  |
| `0x6314` | `C->S` | CAS (Customer Advisory Service) | `AGENT_CAS_CLIENT` |  |
| `0x6315` | `S->C` | CAS (Customer Advisory Service) | `AGENT_CAS_SERVER_REQUEST` |  |
| `0x6316` | `C->S` | CAS (Customer Advisory Service) | `AGENT_CAS_SERVER_RESPONSE` |  |
| `0x7001` | `C->S` | Character selection | `AGENT_CHARACTER_SELECTION_JOIN` |  |
| `0x7005` | `C->S` | Logout | `AGENT_LOGOUT` |  |
| `0x7006` | `C->S` | Logout | `AGENT_LOGOUT_CANCEL` |  |
| `0x7007` | `C->S` | Character selection | `AGENT_CHARACTER_SELECTION_ACTION` |  |
| `0x7010` | `C->S` | Operator | `AGENT_OPERATOR_COMMAND` |  |
| `0x7021` | `C->S` | Entity | `AGENT_ENTITY_MOVEMENT` |  |
| `0x7025` | `C->S` | Chat | `AGENT_CHAT` |  |
| `0x7034` | `C->S` | Inventory | `AGENT_INVENTORY_OPERATION` |  |
| `0x703C` | `C->S` | Inventory | `AGENT_INVENTORY_STORAGE_OPEN` |  |
| `0x703E` | `C->S` | Inventory | `AGENT_INVENTORY_ITEM_REPAIR` |  |
| `0x703F` | `C->S` | Inventory | `sro_client.00880A70` |  |
| `0x704C` | `C->S` | Inventory | `AGENT_INVENTORY_ITEM_USE` |  |
| `0x7059` | `C->S` | Teleport | `AGENT_TELEPORT_DESIGNATE` |  |
| `0x705A` | `C->S` | Teleport | `AGENT_TELEPORT_USE` |  |
| `0x705B` | `C->S` | Teleport | `AGENT_TELEPORT_CANCEL` |  |
| `0x705D` | `C->S` | Siege | `AGENT_SIEGE_RETURN` |  |
| `0x705E` | `C->S` | Siege | `AGENT_SIEGE_ACTION` |  |
| `0x7060` | `C->S` | Party | `AGENT_PARTY_CREATE` |  |
| `0x7061` | `C->S` | Party | `AGENT_PARTY_LEAVE` | yes |
| `0x7062` | `C->S` | Party | `AGENT_PARTY_INVITE` |  |
| `0x7063` | `C->S` | Party | `AGENT_PARTY_KICK` | yes |
| `0x7069` | `C->S` | Party | `AGENT_PARTY_MATCHING_FORM` |  |
| `0x706A` | `C->S` | Party | `AGENT_PARTY_MATCHING_CHANGE` |  |
| `0x706B` | `C->S` | Party | `AGENT_PARTY_MATCHING_DELETE` |  |
| `0x706C` | `C->S` | Party | `AGENT_PARTY_MATCHING_LIST` |  |
| `0x706D` | `C->S` | Party | `AGENT_PARTY_MATCHING_JOIN` |  |
| `0x706D` | `S->C` | Party | `AGENT_PARTY_MATCHING_PLAYER_JOIN` |  |
| `0x7081` | `C->S` | Exchange | `AGENT_EXCHANGE_START` |  |
| `0x7082` | `C->S` | Exchange | `AGENT_EXCHANGE_CONFIRM` |  |
| `0x7083` | `C->S` | Exchange | `AGENT_EXCHANGE_APPROVE` |  |
| `0x7084` | `C->S` | Exchange | `AGENT_EXCHANGE_CANCEL` |  |
| `0x70A1` | `C->S` | Skill | `AGENT_SKILL_LEARN` |  |
| `0x70A2` | `C->S` | Skill | `AGENT_SKILL_MASTERY_LEARN` |  |
| `0x70A7` | `C->S` | Character | `AGENT_CHARACTER_BODYSTATE_REQUEST` |  |
| `0x70B1` | `C->S` | Stall | `AGENT_STALL_CREATE` |  |
| `0x70B2` | `C->S` | Stall | `AGENT_STALL_DESTROY` |  |
| `0x70B3` | `C->S` | Stall | `AGENT_STALL_TALK` |  |
| `0x70B4` | `C->S` | Stall | `AGENT_STALL_BUY` |  |
| `0x70B5` | `C->S` | Stall | `AGENT_STALL_LEAVE` |  |
| `0x70BA` | `C->S` | Stall | `AGENT_STALL_UPDATE` |  |
| `0x70C5` | `C->S` | COS (Call On Summons) | `AGENT_COS_COMMAND` |  |
| `0x70C6` | `C->S` | COS (Call On Summons) | `AGENT_COS_TERMINATE` |  |
| `0x70C7` | `C->S` | COS (Call On Summons) | `sro_client.008A7AC0` |  |
| `0x70CB` | `C->S` | COS (Call On Summons) | `AGENT_COS_UPDATE_RIDESTATE` |  |
| `0x70D8` | `C->S` | Quest | `AGENT_QUEST_DINGDONG` |  |
| `0x70D9` | `C->S` | Quest | `AGENT_QUEST_ABANDON` |  |
| `0x70DB` | `C->S` | Quest | `AGENT_QUEST_GATHER_CANCEL` |  |
| `0x70E1` | `C->S` | Job | `AGENT_JOB_JOIN` |  |
| `0x70E2` | `C->S` | Job | `AGENT_JOB_LEAVE` |  |
| `0x70E3` | `C->S` | Job | `AGENT_JOB_ALIAS` |  |
| `0x70E4` | `C->S` | Job | `AGENT_JOB_RANKING` |  |
| `0x70E5` | `C->S` | Job | `AGENT_JOB_OUTCOME` |  |
| `0x70E6` | `C->S` | Job | `AGENT_JOB_PREV_INFO` |  |
| `0x70EA` | `C->S` | Guide | `AGENT_GUIDE` |  |
| `0x70F0` | `C->S` | Guild | `AGENT_GUILD_CREATE` |  |
| `0x70F1` | `C->S` | Guild | `AGENT_GUILD_DISBAND` |  |
| `0x70F2` | `C->S` | Guild | `AGENT_GUILD_LEAVE` |  |
| `0x70F3` | `C->S` | Guild | `AGENT_GUILD_INVITE` |  |
| `0x70F4` | `C->S` | Guild | `AGENT_GUILD_KICK` |  |
| `0x70F6` | `C->S` | Guild | `AGENT_GUILD_DONATE_OBSOLETE` | yes |
| `0x70F9` | `C->S` | Guild | `AGENT_GUILD_UPDATE_NOTICE` |  |
| `0x70FA` | `C->S` | Guild | `AGENT_GUILD_PROMOTE` |  |
| `0x70FB` | `C->S` | Guild | `AGENT_GUILD_UNION_INVITE` |  |
| `0x70FC` | `C->S` | Guild | `AGENT_GUILD_UNION_LEAVE` |  |
| `0x70FD` | `C->S` | Guild | `AGENT_GUILD_UNION_KICK` |  |
| `0x70FF` | `C->S` | Guild | `AGENT_GUILD_UPDATE_SIEGEAUTH` |  |
| `0x7103` | `C->S` | Guild | `AGENT_GUILD_TRANSFER` |  |
| `0x7104` | `C->S` | Guild | `AGENT_GUILD_UPDATE_PERMISSION` |  |
| `0x7105` | `C->S` | Guild | `AGENT_GUILD_ELECTION_START` |  |
| `0x7106` | `C->S` | Guild | `AGENT_GUILD_ELECTION_PARTICIPATE` |  |
| `0x7107` | `C->S` | Guild | `AGENT_GUILD_ELECTION_VOTE` |  |
| `0x7110` | `C->S` | Guild | `AGENT_GUILD_WAR_START` |  |
| `0x7110` | `S->C` | Guild | `AGENT_GUILD_WAR_REQUEST` |  |
| `0x7112` | `C->S` | Guild | `AGENT_GUILD_WAR_END` |  |
| `0x7113` | `C->S` | Guild | `sro_client.00881F80` |  |
| `0x7114` | `C->S` | Guild | `AGENT_GUILD_WAR_REWARD` |  |
| `0x7116` | `C->S` | COS (Call On Summons) | `AGENT_COS_UNSUMMON` |  |
| `0x7117` | `C->S` | COS (Call On Summons) | `AGENT_COS_NAME` |  |
| `0x7118` | `C->S` | Silk | `AGENT_SILK_GACHA_PLAY` |  |
| `0x7119` | `C->S` | Silk | `AGENT_SILK_GACHA_EXCHANGE` |  |
| `0x711A` | `C->S` | Silk | `AGENT_SILK_HISTORY` |  |
| `0x7121` | `C->S` | Silk | `sro_client.008A1360` |  |
| `0x7150` | `C->S` | Alchemy | `AGENT_ALCHEMY_REINFORCE` |  |
| `0x7151` | `C->S` | Alchemy | `AGENT_ALCHEMY_ENCHANT` |  |
| `0x7155` | `C->S` | Alchemy | `AGENT_ALCHEMY_MANUFACTURE` |  |
| `0x7157` | `C->S` | Alchemy | `AGENT_ALCHEMY_DISMANTLE` |  |
| `0x7158` | `C->S` | Config | `AGENT_CONFIG_UPDATE` |  |
| `0x716A` | `C->S` | Alchemy | `AGENT_ALCHEMY_SOCKET` |  |
| `0x7202` | `C->S` | Skill | `AGENT_SKILL_WITHDRAW` |  |
| `0x7203` | `C->S` | Skill | `AGENT_SKILL_MASTERY_WITHDRAW` |  |
| `0x7250` | `C->S` | Guild | `AGENT_GUILD_STORAGE_OPEN` |  |
| `0x7251` | `C->S` | Guild | `AGENT_GUILD_STORAGE_CLOSE` |  |
| `0x7252` | `C->S` | Guild | `AGENT_GUILD_STORAGE_LIST` |  |
| `0x7256` | `C->S` | Guild | `AGENT_GUILD_UPDATE_NICKNAME` |  |
| `0x7258` | `C->S` | Guild | `AGENT_GUILD_DONATE` |  |
| `0x7258` | `S->C` | Guild | `AGENT_GUILD_DONATE` |  |
| `0x7259` | `C->S` | Guild | `AGENT_GUILD_MERCENARY_ATTR` |  |
| `0x7259` | `S->C` | Guild | `AGENT_GUILD_MERCENARY_ATTR` |  |
| `0x725A` | `C->S` | Guild | `AGENT_GUILD_MERCENARY_TERMINATE` |  |
| `0x725A` | `S->C` | Guild | `AGENT_GUILD_MERCENARY_TERMINATE` |  |
| `0x7302` | `C->S` | Community | `AGENT_COMMUNITY_FRIEND_ADD` |  |
| `0x7302` | `S->C` | Community | `AGENT_COMMUNITY_FRIEND_REQUEST` |  |
| `0x7304` | `C->S` | Community | `AGENT_COMMUNITY_FRIEND_DELETE` |  |
| `0x7308` | `C->S` | Community | `AGENT_COMMUNITY_MEMO_OPEN` |  |
| `0x7309` | `C->S` | Community | `AGENT_COMMUNITY_MEMO_SEND` |  |
| `0x730A` | `C->S` | Community | `AGENT_COMMUNITY_MEMO_DELETE` |  |
| `0x730B` | `C->S` | Community | `AGENT_COMMUNITY_MEMO_LIST` |  |
| `0x730C` | `C->S` | Community | `AGENT_COMMUNITY_MEMO_SEND_GROUP` |  |
| `0x730D` | `C->S` | Community | `AGENT_COMMUNITY_BLOCK` |  |
| `0x741A` | `S->C` | FGW (Forgotten World) | `AGENT_FGW_RECALL_REQUEST` |  |
| `0x7420` | `C->S` | COS (Call On Summons) | `AGENT_COS_UPDATE_SETTINGS` |  |
| `0x7450` | `C->S` | Character selection | `AGENT_CHARACTER_SELECTION_RENAME` |  |
| `0x7470` | `C->S` | Academy | `AGENT_ACADEMY_CREATE` |  |
| `0x7471` | `C->S` | Academy | `AGENT_ACADEMY_DISBAND` |  |
| `0x7472` | `C->S` | Academy | `sro_client.008997A0` |  |
| `0x7473` | `C->S` | Academy | `AGENT_ACADEMY_KICK` |  |
| `0x7474` | `C->S` | Academy | `AGENT_ACADEMY_LEAVE` |  |
| `0x7475` | `C->S` | Academy | `AGENT_ACADEMY_GRADE` |  |
| `0x7476` | `C->S` | Academy | `sro_client.008998F0` |  |
| `0x7477` | `C->S` | Academy | `AGENT_ACADEMY_UPDATE_COMMENT` |  |
| `0x7478` | `C->S` | Academy | `AGENT_ACADEMY_HONOR_RANK` |  |
| `0x747A` | `C->S` | Academy | `AGENT_ACADEMY_MATCHING_REGISTER` |  |
| `0x747B` | `C->S` | Academy | `AGENT_ACADEMY_MATCHING_CHANGE` |  |
| `0x747C` | `C->S` | Academy | `AGENT_ACADEMY_MATCHING_DELETE` |  |
| `0x747D` | `C->S` | Academy | `AGENT_ACADEMY_MATCHING_LIST` |  |
| `0x747E` | `C->S` | Academy | `AGENT_ACADEMY_MATCHING_JOIN` |  |
| `0x747E` | `S->C` | Academy | `AGENT_ACADEMY_MATCHING_REQUEST` |  |
| `0x7483` | `C->S` | Academy | `sro_client.0089B7B0` |  |
| `0x74B2` | `C->S` | FlagWar (Capture the Flag) | `AGENT_FLAGWAR_REGISTER` |  |
| `0x74D3` | `C->S` | Battle Arena | `AGENT_BARENA_REQUEST` |  |
| `0x74D4` | `C->S` | Job | `AGENT_JOB_EXPORT_DETAIL` |  |
| `0x74DF` | `C->S` | TAP (Temple Area Points) | `AGENT_TAP_INFO` |  |
| `0x74E0` | `C->S` | TAP (Temple Area Points) | `AGENT_TAP_UPDATE` |  |
| `0x7501` | `C->S` | Guild | `AGENT_GUILD_GP_HISTORY` |  |
| `0x7506` | `C->S` | Consignment | `AGENT_CONSIGNMENT_DETAIL` |  |
| `0x7507` | `C->S` | Consignment | `AGENT_CONSIGNMENT_CLOSE` |  |
| `0x7508` | `C->S` | Consignment | `AGENT_CONSIGNMENT_REGISTER` |  |
| `0x7509` | `C->S` | Consignment | `AGENT_CONSIGNMENT_UNREGISTER` |  |
| `0x750A` | `C->S` | Consignment | `AGENT_CONSIGNMENT_BUY` |  |
| `0x750B` | `C->S` | Consignment | `AGENT_CONSIGNMENT_SETTLE` |  |
| `0x750C` | `C->S` | Consignment | `AGENT_CONSIGNMENT_SEARCH` |  |
| `0x750E` | `C->S` | Consignment | `AGENT_CONSIGNMENT_LIST` |  |
| `0x7515` | `C->S` | Quest | `AGENT_QUEST_REWAD_SELECT` |  |
| `0x7516` | `C->S` | FRPVP (Free PVP) | `AGENT_FRPVP_UPDATE` |  |
| `0x7519` | `C->S` | FGW (Forgotten World) | `AGENT_FGW_RECALL_LIST` |  |
| `0x751A` | `C->S` | FGW (Forgotten World) | `AGENT_FGW_RECALL_MEMBER` |  |
| `0x751C` | `C->S` | FGW (Forgotten World) | `AGENT_FGW_RECALL_RESPONSE` |  |
| `0x751D` | `C->S` | FGW (Forgotten World) | `AGENT_FGW_EXIT` |  |
| `0xA103` | `S->C` | Authentication | `AGENT_AUTH` |  |
| `0xA314` | `S->C` | CAS (Customer Advisory Service) | `AGENT_CAS_CLIENT` |  |
| `0xB001` | `S->C` | Character selection | `AGENT_CHARACTER_SELECTION_JOIN` |  |
| `0xB005` | `S->C` | Logout | `AGENT_LOGOUT` |  |
| `0xB006` | `S->C` | Logout | `AGENT_LOGOUT_CANCEL` |  |
| `0xB007` | `S->C` | Character selection | `AGENT_CHARACTER_SELECTION_ACTION` |  |
| `0xB010` | `S->C` | Operator | `AGENT_OPERATOR_COMMAND` |  |
| `0xB021` | `S->C` | Entity | `AGENT_ENTITY_MOVEMENT` |  |
| `0xB025` | `S->C` | Chat | `AGENT_CHAT` |  |
| `0xB034` | `S->C` | Inventory | `AGENT_INVENTORY_OPERATION` |  |
| `0xB03C` | `S->C` | Inventory | `AGENT_INVENTORY_STORAGE_OPEN` |  |
| `0xB03E` | `S->C` | Inventory | `AGENT_INVENTORY_ITEM_REPAIR` |  |
| `0xB03F` | `S->C` | Inventory | `sro_client.00880A70` |  |
| `0xB04C` | `S->C` | Inventory | `AGENT_INVENTORY_ITEM_USE` |  |
| `0xB059` | `S->C` | Teleport | `AGENT_TELEPORT_DESIGNATE` |  |
| `0xB05A` | `S->C` | Teleport | `AGENT_TELEPORT_USE` |  |
| `0xB05B` | `S->C` | Teleport | `AGENT_TELEPORT_CANCEL` |  |
| `0xB05D` | `S->C` | Siege | `AGENT_SIEGE_RETURN` |  |
| `0xB05E` | `S->C` | Siege | `AGENT_SIEGE_ACTION` |  |
| `0xB060` | `S->C` | Party | `AGENT_PARTY_CREATE` |  |
| `0xB061` | `S->C` | Party | `AGENT_PARTY_LEAVE` | yes |
| `0xB062` | `S->C` | Party | `AGENT_PARTY_INVITE` |  |
| `0xB063` | `S->C` | Party | `AGENT_PARTY_KICK` | yes |
| `0xB067` | `S->C` | Party | `sro_client.OnJoinPartyAck` |  |
| `0xB069` | `S->C` | Party | `AGENT_PARTY_MATCHING_FORM` |  |
| `0xB06A` | `S->C` | Party | `AGENT_PARTY_MATCHING_CHANGE` |  |
| `0xB06B` | `S->C` | Party | `AGENT_PARTY_MATCHING_DELETE` |  |
| `0xB06C` | `S->C` | Party | `AGENT_PARTY_MATCHING_LIST` |  |
| `0xB06D` | `S->C` | Party | `AGENT_PARTY_MATCHING_JOIN` |  |
| `0xB070` | `S->C` | Entity | `AGENT_ENTITY_SKILL_CAST_BEGIN` |  |
| `0xB071` | `S->C` | Entity | `AGENT_ENTITY_SKILL_CAST_END` |  |
| `0xB072` | `S->C` | Entity | `AGENT_ENTITY_SKILL_BUFF_REMOVE` |  |
| `0xB081` | `S->C` | Exchange | `AGENT_EXCHANGE_START` |  |
| `0xB082` | `S->C` | Exchange | `AGENT_EXCHANGE_CONFIRM` |  |
| `0xB083` | `S->C` | Exchange | `AGENT_EXCHANGE_APPROVE` |  |
| `0xB084` | `S->C` | Exchange | `AGENT_EXCHANGE_CANCEL` |  |
| `0xB0A1` | `S->C` | Skill | `AGENT_SKILL_LEARN` |  |
| `0xB0A2` | `S->C` | Skill | `AGENT_SKILL_MASTERY_LEARN` |  |
| `0xB0B1` | `S->C` | Stall | `AGENT_STALL_CREATE` |  |
| `0xB0B2` | `S->C` | Stall | `AGENT_STALL_DESTROY` |  |
| `0xB0B3` | `S->C` | Stall | `AGENT_STALL_TALK` |  |
| `0xB0B4` | `S->C` | Stall | `AGENT_STALL_BUY` |  |
| `0xB0B5` | `S->C` | Stall | `AGENT_STALL_LEAVE` |  |
| `0xB0BA` | `S->C` | Stall | `AGENT_STALL_UPDATE` |  |
| `0xB0BD` | `S->C` | Entity | `AGENT_ENTITY_SKILL_BUFF_ADD` |  |
| `0xB0C5` | `S->C` | COS (Call On Summons) | `AGENT_COS_COMMAND` |  |
| `0xB0C6` | `S->C` | COS (Call On Summons) | `AGENT_COS_TERMINATE` |  |
| `0xB0C7` | `S->C` | COS (Call On Summons) | `sro_client.008A7AC0` |  |
| `0xB0CB` | `S->C` | COS (Call On Summons) | `AGENT_COS_UPDATE_RIDESTATE` |  |
| `0xB0D8` | `S->C` | Quest | `AGENT_QUEST_DINGDONG` |  |
| `0xB0D9` | `S->C` | Quest | `AGENT_QUEST_ABANDON` |  |
| `0xB0DB` | `S->C` | Quest | `AGENT_QUEST_GATHER_CANCEL` |  |
| `0xB0E1` | `S->C` | Job | `AGENT_JOB_JOIN` |  |
| `0xB0E2` | `S->C` | Job | `AGENT_JOB_LEAVE` |  |
| `0xB0E3` | `S->C` | Job | `AGENT_JOB_ALIAS` |  |
| `0xB0E4` | `S->C` | Job | `AGENT_JOB_RANKING` |  |
| `0xB0E5` | `S->C` | Job | `AGENT_JOB_OUTCOME` |  |
| `0xB0E6` | `S->C` | Job | `AGENT_JOB_PREV_INFO` |  |
| `0xB0EA` | `S->C` | Guide | `AGENT_GUIDE` |  |
| `0xB0F0` | `S->C` | Guild | `AGENT_GUILD_CREATE` |  |
| `0xB0F1` | `S->C` | Guild | `AGENT_GUILD_DISBAND` |  |
| `0xB0F2` | `S->C` | Guild | `AGENT_GUILD_LEAVE` |  |
| `0xB0F3` | `S->C` | Guild | `AGENT_GUILD_INVITE` |  |
| `0xB0F4` | `S->C` | Guild | `AGENT_GUILD_KICK` |  |
| `0xB0F6` | `S->C` | Guild | `AGENT_GUILD_DONATE_OBSOLETE` | yes |
| `0xB0F8` | `S->C` | Guild | `sro_client.00881890` |  |
| `0xB0F9` | `S->C` | Guild | `AGENT_GUILD_UPDATE_NOTICE` |  |
| `0xB0FA` | `S->C` | Guild | `AGENT_GUILD_PROMOTE` |  |
| `0xB0FB` | `S->C` | Guild | `AGENT_GUILD_UNION_INVITE` |  |
| `0xB0FC` | `S->C` | Guild | `AGENT_GUILD_UNION_LEAVE` |  |
| `0xB0FD` | `S->C` | Guild | `AGENT_GUILD_UNION_KICK` |  |
| `0xB0FF` | `S->C` | Guild | `AGENT_GUILD_UPDATE_SIEGEAUTH` |  |
| `0xB103` | `S->C` | Guild | `AGENT_GUILD_TRANSFER` |  |
| `0xB104` | `S->C` | Guild | `AGENT_GUILD_UPDATE_PERMISSION` |  |
| `0xB105` | `S->C` | Guild | `AGENT_GUILD_ELECTION_START` |  |
| `0xB106` | `S->C` | Guild | `AGENT_GUILD_ELECTION_PARTICIPATE` |  |
| `0xB107` | `S->C` | Guild | `AGENT_GUILD_ELECTION_VOTE` |  |
| `0xB110` | `S->C` | Guild | `AGENT_GUILD_WAR_START` |  |
| `0xB112` | `S->C` | Guild | `AGENT_GUILD_WAR_END` |  |
| `0xB113` | `S->C` | Guild | `sro_client.00881F80` |  |
| `0xB114` | `S->C` | Guild | `AGENT_GUILD_WAR_REWARD` |  |
| `0xB116` | `S->C` | COS (Call On Summons) | `AGENT_COS_UNSUMMON` |  |
| `0xB117` | `S->C` | COS (Call On Summons) | `AGENT_COS_NAME` |  |
| `0xB118` | `S->C` | Silk | `AGENT_SILK_GACHA_PLAY` |  |
| `0xB119` | `S->C` | Silk | `AGENT_SILK_GACHA_EXCHANGE` |  |
| `0xB11A` | `S->C` | Silk | `AGENT_SILK_HISTORY` |  |
| `0xB121` | `S->C` | Silk | `sro_client.008A1360` |  |
| `0xB150` | `S->C` | Alchemy | `AGENT_ALCHEMY_REINFORCE` |  |
| `0xB151` | `S->C` | Alchemy | `AGENT_ALCHEMY_ENCHANT` |  |
| `0xB155` | `S->C` | Alchemy | `AGENT_ALCHEMY_MANUFACTURE` |  |
| `0xB157` | `S->C` | Alchemy | `AGENT_ALCHEMY_DISMANTLE` |  |
| `0xB16A` | `S->C` | Alchemy | `AGENT_ALCHEMY_SOCKET` |  |
| `0xB202` | `S->C` | Skill | `AGENT_SKILL_WITHDRAW` |  |
| `0xB203` | `S->C` | Skill | `AGENT_SKILL_MASTERY_WITHDRAW` |  |
| `0xB250` | `S->C` | Guild | `AGENT_GUILD_STORAGE_OPEN` |  |
| `0xB251` | `S->C` | Guild | `AGENT_GUILD_STORAGE_CLOSE` |  |
| `0xB252` | `S->C` | Guild | `AGENT_GUILD_STORAGE_LIST` |  |
| `0xB256` | `S->C` | Guild | `AGENT_GUILD_UPDATE_NICKNAME` |  |
| `0xB302` | `S->C` | Community | `AGENT_COMMUNITY_FRIEND_ADD` |  |
| `0xB304` | `S->C` | Community | `AGENT_COMMUNITY_FRIEND_DELETE` |  |
| `0xB308` | `S->C` | Community | `AGENT_COMMUNITY_MEMO_OPEN` |  |
| `0xB309` | `S->C` | Community | `AGENT_COMMUNITY_MEMO_SEND` |  |
| `0xB30A` | `S->C` | Community | `AGENT_COMMUNITY_MEMO_DELETE` |  |
| `0xB30B` | `S->C` | Community | `AGENT_COMMUNITY_MEMO_LIST` |  |
| `0xB30C` | `S->C` | Community | `AGENT_COMMUNITY_MEMO_SEND_GROUP` |  |
| `0xB30D` | `S->C` | Community | `AGENT_COMMUNITY_BLOCK` |  |
| `0xB420` | `S->C` | COS (Call On Summons) | `AGENT_COS_UPDATE_SETTINGS` |  |
| `0xB450` | `S->C` | Character selection | `AGENT_CHARACTER_SELECTION_RENAME` |  |
| `0xB470` | `S->C` | Academy | `AGENT_ACADEMY_CREATE` |  |
| `0xB471` | `S->C` | Academy | `AGENT_ACADEMY_DISBAND` |  |
| `0xB472` | `S->C` | Academy | `sro_client.008997A0` |  |
| `0xB473` | `S->C` | Academy | `AGENT_ACADEMY_KICK` |  |
| `0xB474` | `S->C` | Academy | `AGENT_ACADEMY_LEAVE` |  |
| `0xB475` | `S->C` | Academy | `AGENT_ACADEMY_GRADE` |  |
| `0xB476` | `S->C` | Academy | `sro_client.008998F0` |  |
| `0xB477` | `S->C` | Academy | `AGENT_ACADEMY_UPDATE_COMMENT` |  |
| `0xB478` | `S->C` | Academy | `AGENT_ACADEMY_HONOR_RANK` |  |
| `0xB47A` | `S->C` | Academy | `AGENT_ACADEMY_MATCHING_REGISTER` |  |
| `0xB47B` | `S->C` | Academy | `AGENT_ACADEMY_MATCHING_CHANGE` |  |
| `0xB47C` | `S->C` | Academy | `AGENT_ACADEMY_MATCHING_DELETE` |  |
| `0xB47D` | `S->C` | Academy | `AGENT_ACADEMY_MATCHING_LIST` |  |
| `0xB47E` | `S->C` | Academy | `AGENT_ACADEMY_MATCHING_JOIN` |  |
| `0xB483` | `S->C` | Academy | `sro_client.0089B7B0` |  |
| `0xB4D4` | `S->C` | Job | `AGENT_JOB_EXPORT_DETAIL` |  |
| `0xB4DF` | `S->C` | TAP (Temple Area Points) | `AGENT_TAP_INFO` |  |
| `0xB4E0` | `S->C` | TAP (Temple Area Points) | `AGENT_TAP_UPDATE` |  |
| `0xB501` | `S->C` | Guild | `AGENT_GUILD_GP_HISTORY` |  |
| `0xB506` | `S->C` | Consignment | `AGENT_CONSIGNMENT_DETAIL` |  |
| `0xB507` | `S->C` | Consignment | `AGENT_CONSIGNMENT_CLOSE` |  |
| `0xB508` | `S->C` | Consignment | `AGENT_CONSIGNMENT_REGISTER` |  |
| `0xB509` | `S->C` | Consignment | `AGENT_CONSIGNMENT_UNREGISTER` |  |
| `0xB50A` | `S->C` | Consignment | `AGENT_CONSIGNMENT_BUY` |  |
| `0xB50B` | `S->C` | Consignment | `AGENT_CONSIGNMENT_SETTLE` |  |
| `0xB50C` | `S->C` | Consignment | `AGENT_CONSIGNMENT_SEARCH` |  |
| `0xB50E` | `S->C` | Consignment | `AGENT_CONSIGNMENT_LIST` |  |
| `0xB516` | `S->C` | FRPVP (Free PVP) | `AGENT_FRPVP_UPDATE` |  |
| `0xB519` | `S->C` | FGW (Forgotten World) | `AGENT_FGW_RECALL_LIST` |  |
| `0xB51A` | `S->C` | FGW (Forgotten World) | `AGENT_FGW_RECALL_MEMBER` |  |
| `0xB51C` | `S->C` | FGW (Forgotten World) | `AGENT_FGW_RECALL_RESPONSE` |  |
| `0xB51D` | `S->C` | FGW (Forgotten World) | `AGENT_FGW_EXIT` |  |
