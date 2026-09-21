SET NOCOUNT ON;
SET XACT_ABORT ON;

IF OBJECT_ID(N'dbo.ClientlessPartyFormSettings', N'U') IS NULL
    THROW 51110, 'dbo.ClientlessPartyFormSettings must exist before applying Clientless party modes.', 1;

BEGIN TRANSACTION;

IF COL_LENGTH(N'dbo.ClientlessPartyFormSettings', N'Mode') IS NULL
BEGIN
    EXEC(N'ALTER TABLE dbo.ClientlessPartyFormSettings ADD Mode VARCHAR(16) NOT NULL
        CONSTRAINT DF_ClientlessPartyForm_Mode DEFAULT (''GroupsOf8'') WITH VALUES;');
END;
ELSE
BEGIN
    EXEC(N'UPDATE dbo.ClientlessPartyFormSettings
        SET Mode = ''GroupsOf8''
        WHERE Mode IS NULL OR Mode NOT IN (''SoloForms'', ''GroupsOf8'');

        ALTER TABLE dbo.ClientlessPartyFormSettings ALTER COLUMN Mode VARCHAR(16) NOT NULL;');

    IF NOT EXISTS
    (
        SELECT 1
        FROM sys.default_constraints AS D
        INNER JOIN sys.columns AS C
            ON C.object_id = D.parent_object_id AND C.column_id = D.parent_column_id
        WHERE D.parent_object_id = OBJECT_ID(N'dbo.ClientlessPartyFormSettings')
          AND C.name = N'Mode'
    )
        EXEC(N'ALTER TABLE dbo.ClientlessPartyFormSettings ADD
            CONSTRAINT DF_ClientlessPartyForm_Mode DEFAULT (''GroupsOf8'') FOR Mode;');
END;

IF NOT EXISTS
(
    SELECT 1
    FROM sys.check_constraints
    WHERE parent_object_id = OBJECT_ID(N'dbo.ClientlessPartyFormSettings')
      AND name = N'CK_ClientlessPartyForm_Mode'
)
BEGIN
    EXEC(N'ALTER TABLE dbo.ClientlessPartyFormSettings WITH CHECK ADD
        CONSTRAINT CK_ClientlessPartyForm_Mode CHECK (Mode IN (''SoloForms'', ''GroupsOf8''));
        ALTER TABLE dbo.ClientlessPartyFormSettings CHECK CONSTRAINT CK_ClientlessPartyForm_Mode;');
END;

COMMIT TRANSACTION;
