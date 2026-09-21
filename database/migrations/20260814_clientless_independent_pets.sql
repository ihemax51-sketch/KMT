SET NOCOUNT ON;
SET XACT_ABORT ON;

IF OBJECT_ID(N'dbo.Clientless_Accounts', N'U') IS NULL
    THROW 51120, 'dbo.Clientless_Accounts must exist before applying independent Clientless pet options.', 1;

IF COL_LENGTH(N'dbo.Clientless_Accounts', N'ProfilePetsEnabled') IS NULL
    THROW 51121, 'Apply the v3.7.0 Clientless creation-options update first.', 1;

BEGIN TRANSACTION;

IF COL_LENGTH(N'dbo.Clientless_Accounts', N'ProfileAttackPetEnabled') IS NULL
BEGIN
    EXEC(N'ALTER TABLE dbo.Clientless_Accounts ADD ProfileAttackPetEnabled BIT NULL;');
    EXEC(N'UPDATE dbo.Clientless_Accounts
        SET ProfileAttackPetEnabled = ISNULL(ProfilePetsEnabled, 1);');
    EXEC(N'ALTER TABLE dbo.Clientless_Accounts ALTER COLUMN ProfileAttackPetEnabled BIT NOT NULL;');
    EXEC(N'ALTER TABLE dbo.Clientless_Accounts ADD
        CONSTRAINT DF_ClientlessAccounts_ProfileAttackPetEnabled DEFAULT (1) FOR ProfileAttackPetEnabled;');
END
ELSE
BEGIN
    EXEC(N'UPDATE dbo.Clientless_Accounts
        SET ProfileAttackPetEnabled = ISNULL(ProfilePetsEnabled, 1)
        WHERE ProfileAttackPetEnabled IS NULL;
        ALTER TABLE dbo.Clientless_Accounts ALTER COLUMN ProfileAttackPetEnabled BIT NOT NULL;');

    IF NOT EXISTS
    (
        SELECT 1
        FROM sys.default_constraints AS D
        INNER JOIN sys.columns AS C
            ON C.object_id = D.parent_object_id AND C.column_id = D.parent_column_id
        WHERE D.parent_object_id = OBJECT_ID(N'dbo.Clientless_Accounts')
          AND C.name = N'ProfileAttackPetEnabled'
    )
        EXEC(N'ALTER TABLE dbo.Clientless_Accounts ADD
            CONSTRAINT DF_ClientlessAccounts_ProfileAttackPetEnabled DEFAULT (1) FOR ProfileAttackPetEnabled;');
END;

IF COL_LENGTH(N'dbo.Clientless_Accounts', N'ProfileGrabPetEnabled') IS NULL
BEGIN
    EXEC(N'ALTER TABLE dbo.Clientless_Accounts ADD ProfileGrabPetEnabled BIT NULL;');
    EXEC(N'UPDATE dbo.Clientless_Accounts
        SET ProfileGrabPetEnabled = ISNULL(ProfilePetsEnabled, 1);');
    EXEC(N'ALTER TABLE dbo.Clientless_Accounts ALTER COLUMN ProfileGrabPetEnabled BIT NOT NULL;');
    EXEC(N'ALTER TABLE dbo.Clientless_Accounts ADD
        CONSTRAINT DF_ClientlessAccounts_ProfileGrabPetEnabled DEFAULT (1) FOR ProfileGrabPetEnabled;');
END
ELSE
BEGIN
    EXEC(N'UPDATE dbo.Clientless_Accounts
        SET ProfileGrabPetEnabled = ISNULL(ProfilePetsEnabled, 1)
        WHERE ProfileGrabPetEnabled IS NULL;
        ALTER TABLE dbo.Clientless_Accounts ALTER COLUMN ProfileGrabPetEnabled BIT NOT NULL;');

    IF NOT EXISTS
    (
        SELECT 1
        FROM sys.default_constraints AS D
        INNER JOIN sys.columns AS C
            ON C.object_id = D.parent_object_id AND C.column_id = D.parent_column_id
        WHERE D.parent_object_id = OBJECT_ID(N'dbo.Clientless_Accounts')
          AND C.name = N'ProfileGrabPetEnabled'
    )
        EXEC(N'ALTER TABLE dbo.Clientless_Accounts ADD
            CONSTRAINT DF_ClientlessAccounts_ProfileGrabPetEnabled DEFAULT (1) FOR ProfileGrabPetEnabled;');
END;

COMMIT TRANSACTION;
