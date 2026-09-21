# Discord Notifications

Discord delivery is configured from **Operations → Discord Notifications** in
the KMTGuard Admin Desktop.

## Customer setup

1. Create an application and bot in the Discord Developer Portal.
2. Add the bot to the target Discord server.
3. Grant the bot **View Channel** and **Send Messages** in every destination.
4. Open Discord Developer Mode so channel IDs can be copied.
5. Paste the Bot Token into KMTGuard, save it, and use **Test Bot**.
6. Choose **Add Channel**, enter a friendly channel name and its numeric
   Discord Channel ID, then choose **Send Test**.
7. Enable Discord delivery and save the settings.

The Bot Token is protected for the local Windows machine before it is stored.
It is not placed in the notification queue, logs, or SQL calls. Notification
delivery only uses outbound HTTPS; the customer does not need inbound ports or
Discord gateway intents.

## Queueing from SQL

The supported SQL contract routes by the friendly name configured in the
desktop application:

```sql
EXEC dbo.Discord_Notification
    @ChannelName = N'Unique Notifications',
    @Message = N'Tiger Girl has appeared in Jangan.',
    @EventKey = N'unique-spawn:1954:20260729-180000';
```

`@EventKey` is optional. Supply a stable key when the calling procedure may
run more than once for the same event. Repeated calls with the same key return
the original queue ID instead of creating duplicate Discord messages.

Direct inserts are also supported:

```sql
INSERT dbo.DiscordNotificationQueue (ChannelName, MessageText)
VALUES (N'Global Announcements', N'The server will restart in 10 minutes.');
```

The stored procedure is preferred because it validates that the named
destination exists and is enabled before queueing.

Discord message content is limited to 2,000 characters. Automated mentions
are disabled so inserted text cannot unexpectedly ping roles or everyone.

## Delivery behavior

The Agent service claims queued rows atomically and sends them through the
configured bot. Successful rows are marked `Sent`. Temporary network,
rate-limit, and server failures use automatic retry with a bounded backoff.
Invalid credentials, missing channels, and denied channel permissions are
recorded as clear permanent failures in the desktop delivery activity.

Queue status values are:

```text
0  Pending
1  Processing
2  Sent
3  Failed
```

The **Retry Failed** action returns failed rows to the pending queue after the
customer fixes the Bot Token, channel ID, or channel permissions.
