/*
    Inventory design selector for KMTGuard.

    Run on the KMTGuard/proxy database.
    True  = use the new 3D inventory design.
    False = use the original inventory design.

    The value is read when KMTGuard starts and is intentionally not live-reloaded.
    Restart KMTGuard, then reconnect the client after changing it.
*/

SET NOCOUNT ON;

IF OBJECT_ID(N'dbo.__Settings', N'U') IS NULL
BEGIN
    RAISERROR('__Settings table was not found.', 16, 1);
    RETURN;
END;

MERGE dbo.__Settings AS target
USING (VALUES (N'NewInventoryDesign', N'True')) AS source(SettingName, Value)
    ON target.SettingName = source.SettingName
WHEN NOT MATCHED THEN
    INSERT (SettingName, Value)
    VALUES (source.SettingName, source.Value);

PRINT 'Inventory design setting migration completed.';
