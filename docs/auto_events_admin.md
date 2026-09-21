# Auto Events Admin Guide

Installed events:

- Retype
- Trivia
- FirstType
- Math
- LongestOnline
- LuckyStaller
- LuckyStall
- LuckyParty
- LuckyGlobal
- Alchemy

Battle Royale is intentionally not included.

The event system stores its own config, content, queue, and logs in the separate database:

```text
Events
```

The filter still uses the existing filter/account/shard databases for live sessions and reward delivery.

Install or upgrade with:

```sql
database/migrations/20260711_events_database.sql
```

## Console Commands

```text
/event start Retype
/event start Trivia
/event start FirstType
/event start Math
/event start LongestOnline
/event start LuckyStaller
/event start LuckyStall
/event start LuckyParty
/event start LuckyGlobal
/event start Alchemy
/event stop
/event reload
/event status
/reload 3
```

`/reload 3` reloads the client event schedule cache, scheduler jobs, and Auto Events config.

## Scheduler

Use the existing `_Scheduler` table. The scheduler only accepts a single `EXEC` command.

Example job query:

```sql
EXEC Events.dbo._AutoEventEnqueueStart N'Retype'
```

Supported event codes:

```text
Retype, Trivia, FirstType, Math, LongestOnline, LuckyStaller, LuckyStall, LuckyParty, LuckyGlobal, Alchemy
```

LuckyParty, LuckyGlobal, and LuckyStall only use participations created after the active round starts. Old parties, old globals, and old stalls are ignored.

## Event Config

Manage rounds and anti-cheat from:

```sql
SELECT * FROM Events.dbo._AutoEventConfig;
```

Important columns:

- `Enabled`: turn event on/off.
- `StartDelaySeconds`: warm-up time before the first round starts. Default is 60 seconds.
- `RoundCount`: default is 3.
- `RoundDurationSeconds`: answer window or selection delay.
- `InterRoundDelaySeconds`: pause between rounds.
- `MinLevel`: minimum character level.
- `HwidLimit`: max online characters allowed from the same HWID for this event. `0` means unlimited.
- `UniqueWinnerPerRun`: blocks the same CharID/HWID/IP from winning multiple rounds in the same event run.
- `RequireHwid`: requires verified DLL HWID.
- `AnswerCooldownMs`: throttles answer spam.
- `AlchemyTargetPlus`: required plus for Alchemy rounds.

Example:

```sql
UPDATE Events.dbo._AutoEventConfig
SET RoundCount = 3,
    StartDelaySeconds = 60,
    RoundDurationSeconds = 60,
    HwidLimit = 1,
    UniqueWinnerPerRun = 1,
    RequireHwid = 1
WHERE EventCode = N'Retype';

EXEC Events.dbo._AutoEventEnqueueReload;
```

## Question And Text Content

Manage chat event content from:

```sql
SELECT * FROM Events.dbo._AutoEventRoundContent;
```

Examples:

```sql
INSERT INTO Events.dbo._AutoEventRoundContent (EventCode, Prompt, Answer, Weight)
VALUES
    (N'Trivia', N'What is 12D sun called? Type SUN', N'SUN', 10),
    (N'Retype', N'Type this exactly: KMTGUARD-GUARD', N'KMTGUARD-GUARD', 10),
    (N'FirstType', N'First to type: PROFESSIONAL', N'PROFESSIONAL', 10);

EXEC Events.dbo._AutoEventEnqueueReload;
```

Math generates rounds automatically. LongestOnline, LuckyStaller, LuckyStall, LuckyParty, and LuckyGlobal do not need content.

Within one event run, content is not reused while unused active rows are available. Retype and FirstType generate a fresh token if their configured content runs out.

## Rewards

Manage normal event rewards from:

```sql
SELECT * FROM Events.dbo._AutoEventReward;
```

Reward types:

```text
SilkOwn, SilkGift, SilkPoint, Gold, ItemChest
```

Examples:

```sql
-- 20 silk own for every Retype round winner
INSERT INTO Events.dbo._AutoEventReward (EventCode, Placement, RewardType, Amount)
VALUES (N'Retype', 1, N'SilkOwn', 20);

-- Item chest reward
INSERT INTO Events.dbo._AutoEventReward
    (EventCode, Placement, RewardType, ItemCodeName128, ItemCount, Plus)
VALUES
    (N'Alchemy', 1, N'ItemChest', 'ITEM_MALL_GLOBAL_CHATTING', 5, 0);

EXEC Events.dbo._AutoEventEnqueueReload;
```

Disable a reward instead of deleting it:

```sql
UPDATE Events.dbo._AutoEventReward
SET IsActive = 0
WHERE RewardID = 1;
```

## Logs

Use these tables for auditing:

```sql
SELECT * FROM Events.dbo._AutoEventRun ORDER BY RunID DESC;
SELECT * FROM Events.dbo._AutoEventRound ORDER BY RoundID DESC;
SELECT * FROM Events.dbo._AutoEventWinnerLog ORDER BY LogID DESC;
SELECT * FROM Events.dbo._AutoEventCommandQueue ORDER BY CommandID DESC;
```

The winner log stores CharID, JID, HWID, IP, answer/evidence, and reward summary.
