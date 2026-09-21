# Telegram Notifications

Telegram delivery is configured from the KMTGuard Admin Desktop under
**Operations → Telegram Notifications**.

## First-time setup

1. Create a bot with `@BotFather` and copy its Bot Token.
2. Add the bot to the target channel as an administrator with permission to
   post messages.
3. Enter the channel ID (`-100...`) or a public `@channel` username.
4. Save the settings and use **Send Test** before enabling live alerts.
5. Run `database\v1.4.3\20260728_telegram_notifications.sql` on the KMTGuard database when installing a
   new database or upgrading an installation that has not opened this page.

The Agent stores pending alerts in a durable SQL queue. Network delivery is
performed by the background notification worker, with duplicate protection and
automatic retry. Event reminders use the configured local server schedule.

## Queueing an alert from a SQL procedure

Any game or event procedure can queue a notification with one simple call:

```sql
DECLARE @TelegramMessage NVARCHAR(4000);

SET @TelegramMessage =
    N'<b>🔥 UNIQUE SPAWN ALERT</b>' +
    NCHAR(10) + NCHAR(10) +
    N'<b>Tiger Girl</b> has appeared in Jangan.';

EXEC dbo.Telegram_Notification
    @Category = N'UniqueSpawn',
    @EventKey = N'unique-spawn:1954:20260728-180000',
    @Message = @TelegramMessage;
```

`@Message` accepts Telegram HTML and must contain the real English display
name (for example, `Tiger Girl`), never a server CodeName such as
`MOB_CH_TIGERWOMAN`. `@Category` is a short label used in the delivery log.
Pass a stable `@EventKey` when the same event may be retried; repeated calls
with that key return the existing queue ID and do not create a duplicate.
If no key is supplied, the procedure generates a unique one.

The procedure only writes to `dbo.TelegramNotificationQueue`. The Agent sends
the queued message in the background, so game procedures never perform an
HTTP request or wait on Telegram.
