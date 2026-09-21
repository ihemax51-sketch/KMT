/*
    Trade goods sell captcha for KMTGuard.

    Run on the KMTGuard/proxy database, then restart KMTGuard.
*/

SET NOCOUNT ON;

IF OBJECT_ID(N'dbo.__Settings', N'U') IS NOT NULL
BEGIN
    MERGE dbo.__Settings AS target
    USING
    (
        VALUES
            (N'EnableTradeSellCaptcha', N'True'),
            (N'TradeSellCaptchaTimeoutSeconds', N'60'),
            (N'TradeSellCaptchaMaxAttempts', N'3')
    ) AS source(SettingName, Value)
        ON target.SettingName = source.SettingName
    WHEN NOT MATCHED THEN
        INSERT (SettingName, Value)
        VALUES (source.SettingName, source.Value);
END;

PRINT 'Trade sell captcha migration completed.';
