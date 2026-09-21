SET NOCOUNT ON;
SET XACT_ABORT ON;

IF OBJECT_ID(N'dbo.ClientlessPartyFormSettings', N'U') IS NULL
BEGIN
    CREATE TABLE dbo.ClientlessPartyFormSettings
    (
        SettingID tinyint NOT NULL
            CONSTRAINT PK_ClientlessPartyFormSettings PRIMARY KEY,
        Enabled bit NOT NULL
            CONSTRAINT DF_ClientlessPartyForm_Enabled DEFAULT (0),
        Title varchar(64) NOT NULL
            CONSTRAINT DF_ClientlessPartyForm_Title DEFAULT ('{CharacterName}'),
        MinLevel tinyint NOT NULL
            CONSTRAINT DF_ClientlessPartyForm_MinLevel DEFAULT (1),
        MaxLevel tinyint NOT NULL
            CONSTRAINT DF_ClientlessPartyForm_MaxLevel DEFAULT (140),
        Purpose tinyint NOT NULL
            CONSTRAINT DF_ClientlessPartyForm_Purpose DEFAULT (0),
        SettingsFlag tinyint NOT NULL
            CONSTRAINT DF_ClientlessPartyForm_Settings DEFAULT (7),
        UpdatedAt datetime2 NOT NULL
            CONSTRAINT DF_ClientlessPartyForm_Updated DEFAULT (SYSDATETIME()),
        CONSTRAINT CK_ClientlessPartyForm_SingleRow CHECK (SettingID = 1),
        CONSTRAINT CK_ClientlessPartyForm_LevelRange CHECK (MinLevel <= MaxLevel)
    );
END;

IF NOT EXISTS
(
    SELECT 1
    FROM dbo.ClientlessPartyFormSettings
    WHERE SettingID = 1
)
BEGIN
    INSERT dbo.ClientlessPartyFormSettings (SettingID)
    VALUES (1);
END;
