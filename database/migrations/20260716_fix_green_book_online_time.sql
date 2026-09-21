USE [SRO_VT_ACCOUNT];
GO

SET NOCOUNT ON;
SET XACT_ABORT ON;

IF OBJECT_ID(N'dbo.TB_User', N'U') IS NULL
BEGIN
    RAISERROR(N'SRO_VT_ACCOUNT.dbo.TB_User was not found.', 16, 1);
    RETURN;
END

IF COL_LENGTH(N'dbo.TB_User', N'AccPlayTime') IS NULL
BEGIN
    RAISERROR(N'dbo.TB_User.AccPlayTime was not found.', 16, 1);
    RETURN;
END
GO

BEGIN TRY
    BEGIN TRANSACTION;

    IF COL_LENGTH(N'dbo.TB_User', N'OnlineTimee') IS NULL
    BEGIN
        ALTER TABLE dbo.TB_User
            ADD OnlineTimee INT NOT NULL
                CONSTRAINT DF_TB_User_OnlineTimee DEFAULT (0) WITH VALUES;
    END
    ELSE IF EXISTS
    (
        SELECT 1
        FROM sys.columns AS c
        INNER JOIN sys.types AS t ON t.user_type_id = c.user_type_id
        WHERE c.object_id = OBJECT_ID(N'dbo.TB_User')
          AND c.name = N'OnlineTimee'
          AND t.name <> N'int'
    )
    BEGIN
        RAISERROR(N'dbo.TB_User.OnlineTimee exists but is not an INT column.', 16, 1);
    END

    DECLARE @ResetRows INT;

    -- Compile this after ALTER TABLE so first-time installations can see OnlineTimee.
    EXEC sys.sp_executesql
        N'UPDATE dbo.TB_User
          SET AccPlayTime = 0,
              OnlineTimee = 0
          WHERE AccPlayTime IS NULL
             OR AccPlayTime <> 0
             OR OnlineTimee IS NULL
             OR OnlineTimee <> 0;

          SET @AffectedRows = @@ROWCOUNT;',
        N'@AffectedRows INT OUTPUT',
        @AffectedRows = @ResetRows OUTPUT;

    IF EXISTS
    (
        SELECT 1
        FROM sys.columns
        WHERE object_id = OBJECT_ID(N'dbo.TB_User')
          AND name = N'OnlineTimee'
          AND is_nullable = 1
    )
    BEGIN
        ALTER TABLE dbo.TB_User ALTER COLUMN OnlineTimee INT NOT NULL;
    END

    COMMIT TRANSACTION;

    SELECT
        N'GreenBookDatabaseFixApplied' AS [Status],
        @ResetRows AS ResetRows;
END TRY
BEGIN CATCH
    IF @@TRANCOUNT > 0
        ROLLBACK TRANSACTION;

    DECLARE @ErrorMessage NVARCHAR(4000) = ERROR_MESSAGE();
    RAISERROR(@ErrorMessage, 16, 1);
END CATCH;
GO

SELECT
    c.name AS ColumnName,
    TYPE_NAME(c.user_type_id) AS DataType,
    c.is_nullable AS IsNullable
FROM sys.columns AS c
WHERE c.object_id = OBJECT_ID(N'dbo.TB_User')
  AND c.name IN (N'AccPlayTime', N'OnlineTimee')
ORDER BY c.column_id;

SELECT
    COUNT_BIG(*) AS TotalAccounts,
    SUM(CASE WHEN AccPlayTime <> 0 THEN 1 ELSE 0 END) AS NonZeroAccPlayTime,
    SUM(CASE WHEN OnlineTimee <> 0 THEN 1 ELSE 0 END) AS NonZeroOnlineTimee
FROM dbo.TB_User;
GO
