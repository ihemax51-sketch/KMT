# KMTGuard VIP System

The VIP system ranks characters by total account Silk spent anywhere.
Charging Silk does not increase the rank until the Silk balance is reduced.

## Desktop control

Open **Content & Economy > VIP System** in the KMTGuard Control Center.

Each row controls:

- `Required Silk`: the minimum lifetime account Silk spend for the icon.
- `Optional Buff Skill`: an active `_RefSkill.Basic_Code` value.
- An empty buff cell means the tier gives only its name icon.

The VIP System page also has an enable/disable button. The selected state is
stored in `dbo.System_Settings` as `VipSystemEnabled` and takes effect after
the Filter is restarted. Disabling the system preserves the existing tiers and
ranking history.

`Save & Apply` validates all icon and skill references, saves the six tiers in
one transaction, recalculates existing player ranks, and queues live rank/icon
updates. Buff changes apply when the character next logs in or changes tier.

Thresholds must be unique and between `0` and `2,000,000,000`.

## Runtime behavior

- `dbo.Hook_ItemMallBuy` applies a resolved account Silk-spend event to its
  active character (the legacy procedure name is retained for compatibility).
- `dbo.Rank_Silk` remains the authoritative per-character history.
- `dbo.Vip_CurrentRankings` exposes the ordered player list with character
  name, total Silk spent, tier, icon, and last update time.
- `dbo.Vip_RankHistory` records each applied spend and the previous/new rank.
- `dbo.Vip_Tiers` stores the configurable thresholds, icons, and buffs.
- `dbo.Vip_OnCharacterLogin` refreshes the configured buff safely.
- The filter reloads the authoritative rank row for every command `27`.

VIP progress is independent of Item Mall mode. An update trigger on
`SRO_VT_ACCOUNT.dbo.SK_Silk` records every decrease in `silk_own` or
`silk_gift`, whether it came from the original Item Mall, the old/new custom
mall, a Silk NPC, Silk stall, or another feature. Increases, charging, rewards,
and refunds are not counted.

Because Silk is account-level while icons are character-level, the Agent
resolves each durable spend event to the active character for that JID. If the
character disconnects before processing, the event remains pending until that
account enters the game again. Processing and completion share one transaction,
so a restart cannot count the same decrease twice.

After ten consecutive processing errors an event enters a 15-minute retry
backoff. It remains eligible for processing, so fixing a missing database
dependency recovers the spend automatically without recreating the event.

Items with IDs `45837` through `45844` retain the legacy exclusion and do not
increase VIP Silk history.

## Installation

For an existing installation, apply the database version folders in order
through the current package version. The v2.7.5 update repairs the
post-player-style VIP procedure, adds durable rank history and current
rankings, and requeues earlier failed spend events. Deploy the matching Filter
and Control Center after the SQL update. Existing tier settings and rank totals
are preserved.
