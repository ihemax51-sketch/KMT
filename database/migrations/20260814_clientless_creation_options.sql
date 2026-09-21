SET NOCOUNT ON;
SET XACT_ABORT ON;

IF OBJECT_ID(N'dbo.Clientless_Accounts', N'U') IS NULL
    THROW 51100, 'dbo.Clientless_Accounts must exist before applying Clientless creation options.', 1;

BEGIN TRANSACTION;

IF COL_LENGTH(N'dbo.Clientless_Accounts', N'ProfileAvatarsEnabled') IS NULL
BEGIN
    EXEC(N'ALTER TABLE dbo.Clientless_Accounts ADD ProfileAvatarsEnabled BIT NOT NULL
        CONSTRAINT DF_ClientlessAccounts_ProfileAvatarsEnabled DEFAULT (1) WITH VALUES;');
END
ELSE
BEGIN
    EXEC(N'UPDATE dbo.Clientless_Accounts SET ProfileAvatarsEnabled = 1 WHERE ProfileAvatarsEnabled IS NULL;
        ALTER TABLE dbo.Clientless_Accounts ALTER COLUMN ProfileAvatarsEnabled BIT NOT NULL;');
    IF NOT EXISTS
    (
        SELECT 1
        FROM sys.default_constraints AS D
        INNER JOIN sys.columns AS C
            ON C.object_id = D.parent_object_id AND C.column_id = D.parent_column_id
        WHERE D.parent_object_id = OBJECT_ID(N'dbo.Clientless_Accounts')
          AND C.name = N'ProfileAvatarsEnabled'
    )
        EXEC(N'ALTER TABLE dbo.Clientless_Accounts ADD
            CONSTRAINT DF_ClientlessAccounts_ProfileAvatarsEnabled DEFAULT (1) FOR ProfileAvatarsEnabled;');
END;

IF COL_LENGTH(N'dbo.Clientless_Accounts', N'ProfilePetsEnabled') IS NULL
BEGIN
    EXEC(N'ALTER TABLE dbo.Clientless_Accounts ADD ProfilePetsEnabled BIT NOT NULL
        CONSTRAINT DF_ClientlessAccounts_ProfilePetsEnabled DEFAULT (1) WITH VALUES;');
END
ELSE
BEGIN
    EXEC(N'UPDATE dbo.Clientless_Accounts SET ProfilePetsEnabled = 1 WHERE ProfilePetsEnabled IS NULL;
        ALTER TABLE dbo.Clientless_Accounts ALTER COLUMN ProfilePetsEnabled BIT NOT NULL;');
    IF NOT EXISTS
    (
        SELECT 1
        FROM sys.default_constraints AS D
        INNER JOIN sys.columns AS C
            ON C.object_id = D.parent_object_id AND C.column_id = D.parent_column_id
        WHERE D.parent_object_id = OBJECT_ID(N'dbo.Clientless_Accounts')
          AND C.name = N'ProfilePetsEnabled'
    )
        EXEC(N'ALTER TABLE dbo.Clientless_Accounts ADD
            CONSTRAINT DF_ClientlessAccounts_ProfilePetsEnabled DEFAULT (1) FOR ProfilePetsEnabled;');
END;

COMMIT TRANSACTION;
