# KMTGuard Bot Protection

The bot policy is enforced by the Gateway and Agent filters. It uses the
existing KMTGuard DLL challenge plus the managed clientless identity assigned
to accounts in `dbo.Clientless_Accounts`.

## Global settings

The following rows live in `dbo.System_Settings` and are refreshed by the
filter within approximately two seconds:

| Setting | Default | Effect |
| --- | --- | --- |
| `AllowBotLogin` | `True` | When `False`, managed clientless accounts and Gateway clients without valid DLL proof are disconnected. Existing managed clientless sessions are disconnected on their next ping. |
| `AllowBotTrade` | `True` | When `False`, bot sessions cannot use player exchange, buy/sell trade goods, or summon a trade transport. |
| `BotProtectionLogEnabled` | `True` | Writes enforcement decisions to `dbo.Security_BotProtectionLog`. |

Examples:

```sql
UPDATE dbo.System_Settings
SET Value = N'False'
WHERE SettingName = N'AllowBotLogin';

UPDATE dbo.System_Settings
SET Value = N'False'
WHERE SettingName = N'AllowBotTrade';
```

## Region policy

Region rules remain in `dbo.Security_RegionFeatures`:

| Column | Effect |
| --- | --- |
| `Enable_NoBot` | Enables the bot-only policy for this Region/World rule. Normal verified players are unaffected. |
| `NoBot_TimeSeconds` | Allowed time from entry. `0` applies the action immediately. Movement does not reset it. |
| `NoBot_Action` | `0` returns the bot to town; `1` disconnects it. |
| `NoBot_WarningSeconds` | Sends one warning when this many seconds remain. `0` disables the warning. |
| `NoBot_LogOnly` | Records the decision without returning or disconnecting the bot. Use for rollout testing. |

Immediate disconnect example:

```sql
UPDATE dbo.Security_RegionFeatures
SET Enable_NoBot = 1,
    NoBot_TimeSeconds = 0,
    NoBot_Action = 1,
    NoBot_WarningSeconds = 0,
    NoBot_LogOnly = 0
WHERE RegionID = 25000;
```

Ten-minute limit followed by return to town:

```sql
UPDATE dbo.Security_RegionFeatures
SET Enable_NoBot = 1,
    NoBot_TimeSeconds = 600,
    NoBot_Action = 0,
    NoBot_WarningSeconds = 30,
    NoBot_LogOnly = 0
WHERE RegionID = 25000;
```

## Classification boundary

This layer reliably identifies KMTGuard-managed clientless sessions and blocks
clients that do not complete the existing DLL proof when bot login is disabled.
A bot controlling a real, verified game client is not automatically proven to
be a bot by this identity layer; behavioral scoring and periodic attestation
are separate defense layers and should be rolled out in log-only mode first.

