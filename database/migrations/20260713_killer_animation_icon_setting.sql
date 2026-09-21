/*
    Killer Animation guide icon visibility setting for KMTGuard.

    Run on the KMTGuard/proxy database.
    Set Value = N'False' to hide the icon, or N'True' to show it,
    then restart KMTGuard. This setting is intentionally not live-reloaded.
*/

SET NOCOUNT ON;

IF OBJECT_ID(N'dbo.__Settings', N'U') IS NULL
BEGIN
    RAISERROR('__Settings table was not found.', 16, 1);
    RETURN;
END;

MERGE dbo.__Settings AS target
USING (VALUES (N'ShowGuideKillerAnimation', N'True')) AS source(SettingName, Value)
    ON target.SettingName = source.SettingName
WHEN NOT MATCHED THEN
    INSERT (SettingName, Value)
    VALUES (source.SettingName, source.Value);

PRINT 'Killer Animation icon visibility setting migration completed.';
