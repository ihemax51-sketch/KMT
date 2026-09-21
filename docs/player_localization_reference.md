# KMTGuard Player Localization Reference

This is the mandatory engineering reference for player-facing wording in the
KMTGuard client and Filter. Read it before changing a client window, custom
packet response, Filter notice, event announcement, scoreboard title, security
message, or language file.

## Architecture

KMTGuard has two localization owners. They are intentionally separate:

1. Client-owned interface text uses Silkroad `TextUISystem` resources.
   This covers static window labels, buttons, tooltips, headings, and client
   messages created entirely inside `KMTGuardKit.dll`.
2. Filter-generated player text uses external JSON files beside the Filter.
   This covers packet responses, notices, warnings, event messages, purchase
   results, security responses, region restrictions, scoreboards, and other
   wording produced by the managed Filter.

The Filter cannot resolve a client `TextUISystem` key. Do not send an
unresolved `TextUISystem` key as a normal Filter notice and expect the client
to translate it. Resolve Filter text on the server with `PlayerLanguage`.

The client language and Filter language are selected independently so a
customer can package the correct client resource and choose the matching
Filter language.

## Canonical Files

Filter localization implementation:

```text
filter/KMTGuardnew/KMTGuard/Localization/PlayerLanguage.cs
```

Required fallback language:

```text
filter/KMTGuardnew/KMTGuard/Languages/English.json
```

Supplied Turkish language:

```text
filter/KMTGuardnew/KMTGuard/Languages/Turkish.json
```

Runtime instructions delivered with the Filter:

```text
filter/KMTGuardnew/KMTGuard/Languages/README.txt
```

Language setting:

```json
"Language": "English"
```

The setting accepts `English`, `Turkish`, common aliases such as `en`,
`en-US`, `tr`, and `tr-TR`, or the filename of a custom language without the
`.json` extension.

## Mandatory Filter Rules

- Never add hardcoded player-facing wording to Filter packet handlers,
  services, event systems, or response helpers.
- Add a stable semantic key to both `English.json` and `Turkish.json`.
- Treat `English.json` as the complete, required fallback contract.
- Resolve the key at the point where the player message is created:

```csharp
PlayerLanguage.Get("Feature.ActionSucceeded")
PlayerLanguage.Get("Feature.WaitSeconds", remainingSeconds)
```

- Keep internal values out of localization: packet opcodes, SQL identifiers,
  event codes, database states, item code names, protocol tokens, log
  categories, and command names must remain stable.
- Operator console text and diagnostic logs are not player-facing and do not
  need `PlayerLanguage` unless the same value is also sent to a player.
- Customer-authored database content such as custom event names, prompts,
  descriptions, or announcement bodies remains customer data. Localize the
  product-owned wrapper around it.
- Do not change packet structure, field order, encoding, or opcode merely to
  localize wording.

## Keys And Placeholders

Use dot-separated semantic names:

```text
QuickLogin.Accepted
ItemChest.ClaimStartFailed
Competitive.TeamScoreboardTitle
SystemNotices.MSG_REVERSE_DELAY
```

Never use the English sentence itself as a key.

Translations must preserve the same placeholder index set as English.
Reordering placeholders is allowed; removing or inventing indexes is not.

```json
"Reward.Delivered": "Delivered {0} x {1}."
```

If English contains `{0}` and `{1}`, every translation must contain both.
Numeric or format suffixes such as `{0:N0}` should remain compatible.

`PlayerLanguage` formats with invariant culture so protocol-facing numbers
remain deterministic. An incompatible translated placeholder set is rejected
for that key and the English text is used safely.

## System_Notices

Existing Filter calls use:

```csharp
RefManager.GetNoticeMessage("NOTICE_NAME")
```

`RefManager` checks the active language for:

```text
SystemNotices.NOTICE_NAME
```

All `System_Notices` names currently referenced by the Filter are supplied in
both standard language files. If a matching language key exists, it overrides
the database wording. If it does not exist, the existing database text remains
the compatibility fallback for customer-defined notices.

Call sites that apply `string.Format` to a notice must keep the same
placeholder indexes in both language files.

## Loading, Fallback, And Reloading

At startup the Filter:

1. Loads required `Languages/English.json`.
2. Loads the language selected by `Settings.json`.
3. Validates keys, string values, duplicates, and placeholders.
4. Publishes an immutable language snapshot for concurrent packet handlers.

Missing active-language keys fall back to English. A missing key in both
languages is logged once and returned visibly as `[Key.Name]` so packaging
mistakes are detectable.

The Admin Desktop Dashboard is the preferred selector for changing the player
notice language. Its Apply action validates and loads the selected JSON file in
every running Filter service, then persists the selection to `Settings.json`.
No Filter restart is required. If a service is stopped, the persisted selection
is loaded normally the next time it starts. A failed load must leave both the
active runtime language and saved setting unchanged.

Changing `Language` by editing `Settings.json` directly still requires a Filter
restart. Editing wording in the language file that is already active can be
applied from the Filter console with:

```text
/reload language
/reloadlang
```

## Adding Another Filter Language

1. Copy `English.json`.
2. Rename the copy to the language name, for example `German.json`.
3. Translate values only; never rename keys.
4. Preserve every placeholder.
5. Select `German` from the Dashboard after its file is deployed, or set
   `"Language": "German"` in `Settings.json`.
6. Use Dashboard Apply for a live switch; restart only after a manual JSON edit.
7. Run the localization smoke tests before delivery.

English must still be shipped because it is the runtime fallback.

## Client TextUISystem Rules

Static KMTGuard client UI wording must use the established `TextUISystem`
keys and the separate ready-to-import client language resources. A new client
control or client-only notification needs a stable text key in every supplied
client language file.

Do not move Filter-generated messages into `TextUISystem`. Do not move static
client labels into Filter JSON. Ownership is determined by which process
creates the final visible sentence.

## Required Validation

After changing Filter messages or language files:

```powershell
dotnet build filter\KMTGuardnew\KMTGuard\KMTGuard.csproj -c Release --no-restore
dotnet run --project filter\KMTGuardnew\KMTGuard.PacketPipelineTests\KMTGuard.PacketPipelineTests.csproj -c Release --no-restore
```

The smoke tests load English and Turkish, verify aliases, verify complete key
coverage, validate placeholders, and exercise `System_Notices` resolution.

Also audit for direct player strings before finishing:

```powershell
rg -n --glob '*.cs' --glob '!bin/**' --glob '!obj/**' 'WriteUnicode\s*\(\s*\x22|SendNotice\s*\(\s*\x22' filter\KMTGuardnew\KMTGuard
```

Review every result. Database content, character names, item code names, and
internal protocol data are not translations; a literal player sentence is.

## Packaging And Delivery

The unified component builder copies the complete source `Languages`
directory into the Filter package and fails when
`English.json`, `Turkish.json`, or `README.txt` is missing.

After localization source changes, run:

```powershell
build-scripts\Build-Component.ps1 -Component Filter
```

Verify:

```text
D:\KMTGuard-build\Filter\Languages
```

Both supplied JSON files must have identical key sets and compatible
placeholder indexes. Refresh the delivery hashes and confirm the restarted
Filter log reports the selected language and expected active/fallback key
counts.

Any customer-visible localization change also requires a clear entry in the
root `CHANGELOG.md`, including whether a Filter restart, client file, media
file, or SQL update is required.
