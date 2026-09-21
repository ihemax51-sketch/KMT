SET NOCOUNT ON;

IF OBJECT_ID(N'dbo.ClientlessPartyFormSettings', N'U') IS NULL
    THROW 51111, 'Validation failed: dbo.ClientlessPartyFormSettings is missing.', 1;

IF COL_LENGTH(N'dbo.ClientlessPartyFormSettings', N'Mode') IS NULL
    THROW 51112, 'Validation failed: Party Form Mode is missing.', 1;

IF EXISTS
(
    SELECT 1
    FROM dbo.ClientlessPartyFormSettings
    WHERE Mode IS NULL OR Mode NOT IN ('SoloForms', 'GroupsOf8')
)
    THROW 51113, 'Validation failed: Party Form Mode contains an unsupported value.', 1;

IF NOT EXISTS
(
    SELECT 1
    FROM sys.default_constraints AS D
    INNER JOIN sys.columns AS C
        ON C.object_id = D.parent_object_id AND C.column_id = D.parent_column_id
    WHERE D.parent_object_id = OBJECT_ID(N'dbo.ClientlessPartyFormSettings')
      AND C.name = N'Mode'
)
    THROW 51114, 'Validation failed: Party Form Mode default is missing.', 1;

IF NOT EXISTS
(
    SELECT 1
    FROM sys.check_constraints
    WHERE parent_object_id = OBJECT_ID(N'dbo.ClientlessPartyFormSettings')
      AND name = N'CK_ClientlessPartyForm_Mode'
      AND is_disabled = 0
      AND is_not_trusted = 0
)
    THROW 51115, 'Validation failed: Party Form Mode constraint is missing or untrusted.', 1;

SELECT SettingID, Enabled, Mode, Title, MinLevel, MaxLevel, Purpose, SettingsFlag, UpdatedAt
FROM dbo.ClientlessPartyFormSettings
WHERE SettingID = 1;
