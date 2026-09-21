SET NOCOUNT ON;
SET XACT_ABORT ON;

BEGIN TRY
    BEGIN TRANSACTION;

    IF OBJECT_ID(N'[dbo].[Security_ItemRegionRestrictions]', N'U') IS NULL
    BEGIN
        CREATE TABLE [dbo].[Security_ItemRegionRestrictions]
        (
            [ID] INT IDENTITY(1,1) NOT NULL CONSTRAINT [PK_Security_ItemRegionRestrictions] PRIMARY KEY,
            [WorldID] INT NOT NULL CONSTRAINT [DF_Security_ItemRegionRestrictions_WorldID] DEFAULT (0),
            [RegionID] INT NOT NULL,
            [ItemID] INT NOT NULL,
            [Enabled] BIT NOT NULL CONSTRAINT [DF_Security_ItemRegionRestrictions_Enabled] DEFAULT (1),
            [Note] NVARCHAR(250) NULL
        );
    END
    ELSE
    BEGIN
        IF COL_LENGTH(N'dbo.Security_ItemRegionRestrictions', N'Enabled') IS NULL
        BEGIN
            ALTER TABLE [dbo].[Security_ItemRegionRestrictions] ADD [Enabled] BIT NULL;

            IF COL_LENGTH(N'dbo.Security_ItemRegionRestrictions', N'AllowUse') IS NOT NULL
                EXEC(N'UPDATE [dbo].[Security_ItemRegionRestrictions]
                       SET [Enabled] = CASE WHEN [AllowUse] = 0 THEN 1 ELSE 0 END;');
            ELSE
                UPDATE [dbo].[Security_ItemRegionRestrictions] SET [Enabled] = 1;

            ALTER TABLE [dbo].[Security_ItemRegionRestrictions] ALTER COLUMN [Enabled] BIT NOT NULL;
        END;

        IF COL_LENGTH(N'dbo.Security_ItemRegionRestrictions', N'AllowUse') IS NOT NULL
        BEGIN
            DECLARE @AllowUseDefault SYSNAME;
            SELECT @AllowUseDefault = dc.[name]
            FROM sys.default_constraints AS dc
            INNER JOIN sys.columns AS c
                ON c.[object_id] = dc.[parent_object_id]
               AND c.[column_id] = dc.[parent_column_id]
            WHERE dc.[parent_object_id] = OBJECT_ID(N'[dbo].[Security_ItemRegionRestrictions]')
              AND c.[name] = N'AllowUse';

            IF @AllowUseDefault IS NOT NULL
                EXEC(N'ALTER TABLE [dbo].[Security_ItemRegionRestrictions] DROP CONSTRAINT '
                     + QUOTENAME(@AllowUseDefault) + N';');

            ALTER TABLE [dbo].[Security_ItemRegionRestrictions] DROP COLUMN [AllowUse];
        END;
    END;

    IF NOT EXISTS
    (
        SELECT 1
        FROM sys.default_constraints AS dc
        INNER JOIN sys.columns AS c
            ON c.[object_id] = dc.[parent_object_id]
           AND c.[column_id] = dc.[parent_column_id]
        WHERE dc.[parent_object_id] = OBJECT_ID(N'[dbo].[Security_ItemRegionRestrictions]')
          AND c.[name] = N'Enabled'
    )
        ALTER TABLE [dbo].[Security_ItemRegionRestrictions]
            ADD CONSTRAINT [DF_Security_ItemRegionRestrictions_Enabled] DEFAULT (1) FOR [Enabled];

    DELETE duplicateRule
    FROM
    (
        SELECT [ID], ROW_NUMBER() OVER
        (
            PARTITION BY [WorldID], [RegionID], [ItemID]
            ORDER BY [ID] DESC
        ) AS DuplicateNumber
        FROM [dbo].[Security_ItemRegionRestrictions]
    ) AS duplicateRule
    WHERE duplicateRule.DuplicateNumber > 1;

    IF EXISTS
    (
        SELECT 1 FROM sys.indexes
        WHERE [object_id] = OBJECT_ID(N'[dbo].[Security_ItemRegionRestrictions]')
          AND [name] = N'IX_Security_ItemRegionRestrictions'
    )
        DROP INDEX [IX_Security_ItemRegionRestrictions] ON [dbo].[Security_ItemRegionRestrictions];

    IF NOT EXISTS
    (
        SELECT 1 FROM sys.indexes
        WHERE [object_id] = OBJECT_ID(N'[dbo].[Security_ItemRegionRestrictions]')
          AND [name] = N'UX_Security_ItemRegionRestrictions_Target'
    )
        CREATE UNIQUE INDEX [UX_Security_ItemRegionRestrictions_Target]
            ON [dbo].[Security_ItemRegionRestrictions] ([WorldID], [RegionID], [ItemID]);

    IF OBJECT_ID(N'[dbo].[CK_Security_ItemRegionRestrictions_WorldID]', N'C') IS NULL
        ALTER TABLE [dbo].[Security_ItemRegionRestrictions]
            ADD CONSTRAINT [CK_Security_ItemRegionRestrictions_WorldID]
            CHECK ([WorldID] BETWEEN 0 AND 65535);

    IF OBJECT_ID(N'[dbo].[CK_Security_ItemRegionRestrictions_RegionID]', N'C') IS NULL
        ALTER TABLE [dbo].[Security_ItemRegionRestrictions]
            ADD CONSTRAINT [CK_Security_ItemRegionRestrictions_RegionID]
            CHECK ([RegionID] BETWEEN -32768 AND 32767);

    IF OBJECT_ID(N'[dbo].[CK_Security_ItemRegionRestrictions_ItemID]', N'C') IS NULL
        ALTER TABLE [dbo].[Security_ItemRegionRestrictions]
            ADD CONSTRAINT [CK_Security_ItemRegionRestrictions_ItemID]
            CHECK ([ItemID] >= 0);

    COMMIT TRANSACTION;
END TRY
BEGIN CATCH
    IF @@TRANCOUNT > 0 ROLLBACK TRANSACTION;
    THROW;
END CATCH;
