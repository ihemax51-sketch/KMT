/*
    Guide icon visibility settings for KMTGuard.

    Run on the KMTGuard/proxy database.
    Set any value to N'False' to hide that icon from the in-game guide icon stack.
*/

SET NOCOUNT ON;

IF OBJECT_ID(N'dbo.__Settings', N'U') IS NOT NULL
BEGIN
    MERGE dbo.__Settings AS target
    USING
    (
        VALUES
            (N'ShowGuideMenu', N'True'),
            (N'ShowGuideLuckySpin', N'True'),
            (N'ShowGuideItemChest', N'True'),
            (N'ShowGuideMacro', N'True'),
            (N'ShowGuideDailyLogin', N'True'),
            (N'ShowGuideDiscord', N'True'),
            (N'ShowGuideWebsite', N'True'),
            (N'ShowGuideFacebook', N'True'),
            (N'ShowGuideAutoEquip', N'True'),
            (N'ShowGuideWebViewer', N'True'),
            (N'ShowGuideMapLocation', N'True'),
            (N'ShowGuideSpecialOffers', N'True')
    ) AS source(SettingName, Value)
        ON target.SettingName = source.SettingName
    WHEN NOT MATCHED THEN
        INSERT (SettingName, Value)
        VALUES (source.SettingName, source.Value);
END;

/*
    Examples:

    UPDATE dbo.__Settings SET Value = N'False' WHERE SettingName = N'ShowGuideLuckySpin';
    UPDATE dbo.__Settings SET Value = N'False' WHERE SettingName = N'ShowGuideSpecialOffers';
    UPDATE dbo.__Settings SET Value = N'False' WHERE SettingName = N'ShowGuideMacro';
    UPDATE dbo.__Settings SET Value = N'False' WHERE SettingName = N'ShowGuideWebViewer';

    Restart KMTGuard and relog after changing icon visibility.
*/

PRINT 'Guide icon visibility settings migration completed.';
