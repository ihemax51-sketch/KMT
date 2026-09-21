SET NOCOUNT ON;
SET XACT_ABORT ON;

BEGIN TRY
    BEGIN TRANSACTION;

    IF NOT EXISTS
    (
        SELECT 1
        FROM dbo.System_Settings WITH (UPDLOCK, HOLDLOCK)
        WHERE SettingName = N'Menu-like-maxi'
    )
    BEGIN
        INSERT INTO dbo.System_Settings (SettingName, Value)
        VALUES (N'Menu-like-maxi', N'False');
    END;

    COMMIT TRANSACTION;
END TRY
BEGIN CATCH
    IF XACT_STATE() <> 0
        ROLLBACK TRANSACTION;
    THROW;
END CATCH;
