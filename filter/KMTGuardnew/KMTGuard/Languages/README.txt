KMTGuard player-message language files

Set "Language" in Settings.json:

  "Language": "English"
  "Language": "Turkish"

Use the Player Notice Language selector on the Admin Desktop Dashboard to save
and apply a language immediately to all running Filter services. A service that
is stopped loads the saved choice on its next start. Restart the Filter only
when Language is changed by editing Settings.json directly. The reload commands
below are for reloading edits to the language file that is already active.

The Filter loads Languages\English.json as the required fallback and then loads
the selected language file. You can add another language by copying
English.json, renaming the copy, translating its values, and setting Language
to that file name without the .json extension.

Keep every key unchanged. Keep placeholders such as {0}, {1}, and {0:N0}
present in the translated value. A translation with incompatible placeholders
is rejected for that key and the English value is used safely.

After editing the active file, run one of these commands in the Filter console:

  /reload language
  /reloadlang

System_Notices compatibility:

Every System_Notices name currently used by the Filter is already included in
English.json and Turkish.json. The database text remains the compatibility
fallback for any custom notice name. To override another database notice
through the language file, add a key in this form to both language files:

  "SystemNotices.NOTICE_NAME": "Translated text"

Do not rename internal database states, event codes, or SQL values. This folder
controls only wording sent to players.
