/*
    Read-only validation for the final player-style schema.
    Run after 20260727_player_style_system_rebuild.sql.
*/
SET NOCOUNT ON;

DECLARE @ExpectedTables TABLE (Name SYSNAME PRIMARY KEY);
INSERT @ExpectedTables (Name)
VALUES
    (N'Tags'),
    (N'PlayerTags'),
    (N'ActiveTags'),
    (N'PlayerTitles'),
    (N'PlayerTitleColors'),
    (N'ActiveTitleColors'),
    (N'PlayerNameColors'),
    (N'ActiveNameColors'),
    (N'Icons'),
    (N'PlayerLeftIcons'),
    (N'ActiveLeftIcons'),
    (N'PlayerRightIcons'),
    (N'ActiveRightIcons'),
    (N'GlobalChatColors'),
    (N'AutoCapeRegions');

IF EXISTS
(
    SELECT 1
    FROM @ExpectedTables AS expected
    WHERE OBJECT_ID(N'dbo.' + QUOTENAME(expected.Name), N'U') IS NULL
)
    THROW 51200, 'One or more final player-style tables are missing.', 1;

DECLARE @ExpectedProcedures TABLE (Name SYSNAME PRIMARY KEY);
INSERT @ExpectedProcedures (Name)
VALUES
    (N'Title_Add'), (N'Title_Remove'),
    (N'Title_Activate'), (N'Title_Deactivate'),
    (N'Tag_Add'), (N'Tag_Remove'),
    (N'Tag_Activate'), (N'Tag_Deactivate'),
    (N'TitleColor_Add'), (N'TitleColor_Remove'),
    (N'TitleColor_Activate'), (N'TitleColor_Deactivate'),
    (N'NameColor_Add'), (N'NameColor_Remove'),
    (N'NameColor_Activate'), (N'NameColor_Deactivate'),
    (N'LeftIcon_Add'), (N'LeftIcon_Remove'),
    (N'LeftIcon_Activate'), (N'LeftIcon_Deactivate'),
    (N'RightIcon_Add'), (N'RightIcon_Remove'),
    (N'RightIcon_Activate'), (N'RightIcon_Deactivate'),
    (N'GuildNickname_Set'), (N'GuildNickname_Clear'),
    (N'ItemGlow_Change'), (N'ItemModel_Change');

IF EXISTS
(
    SELECT 1
    FROM @ExpectedProcedures AS expected
    WHERE OBJECT_ID(N'dbo.' + QUOTENAME(expected.Name), N'P') IS NULL
)
    THROW 51201, 'One or more final player-style procedures are missing.', 1;

IF EXISTS
(
    SELECT 1
    FROM sys.objects
    WHERE name LIKE N'Style[_]%'
)
    THROW 51202, 'A legacy Style_* database object still exists.', 1;

IF EXISTS
(
    SELECT 1
    FROM sys.objects
    WHERE name LIKE N'%TitleNameNew%'
)
OR EXISTS
(
    SELECT 1
    FROM sys.synonyms
    WHERE name LIKE N'%TitleNameNew%'
)
    THROW 51203, 'A legacy TitleNameNew database object still exists.', 1;

IF EXISTS
(
    SELECT CharID, TagID
    FROM dbo.PlayerTags
    GROUP BY CharID, TagID
    HAVING COUNT(*) > 1
)
    THROW 51204, 'Duplicate tag ownership rows were found.', 1;

IF EXISTS
(
    SELECT CharID, IconID
    FROM dbo.PlayerLeftIcons
    GROUP BY CharID, IconID
    HAVING COUNT(*) > 1
)
OR EXISTS
(
    SELECT CharID, IconID
    FROM dbo.PlayerRightIcons
    GROUP BY CharID, IconID
    HAVING COUNT(*) > 1
)
    THROW 51205, 'Duplicate icon ownership rows were found.', 1;

SELECT
    N'PASS' AS ValidationResult,
    (SELECT COUNT(*) FROM dbo.Tags) AS TagDefinitions,
    (SELECT COUNT(*) FROM dbo.PlayerTags) AS OwnedTags,
    (SELECT COUNT(*) FROM dbo.PlayerTitles) AS OwnedTitles,
    (SELECT COUNT(*) FROM dbo.Icons) AS IconDefinitions,
    (SELECT COUNT(*) FROM dbo.PlayerLeftIcons) AS OwnedLeftIcons,
    (SELECT COUNT(*) FROM dbo.PlayerRightIcons) AS OwnedRightIcons;

