/*
    Drop Logs guide icon visibility setting for KMTGuard.

    Run on the KMTGuard/proxy database.
    Set Value = N'False' to hide the Drop Logs guide icon after restarting KMTGuard.
*/

SET NOCOUNT ON;

IF OBJECT_ID(N'dbo.__Settings', N'U') IS NOT NULL
BEGIN
    MERGE dbo.__Settings AS target
    USING (VALUES (N'ShowGuideDropLogs', N'True')) AS source(SettingName, Value)
        ON target.SettingName = source.SettingName
    WHEN NOT MATCHED THEN
        INSERT (SettingName, Value)
        VALUES (source.SettingName, source.Value);
END;

PRINT 'Drop Logs icon visibility setting migration completed.';

