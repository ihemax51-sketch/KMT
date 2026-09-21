/*
    KMTGuard player-style system rebuild.

    This is a one-way schema migration. It replaces the ambiguous Style_*
    object names with the public API below:

      Title_Add / Title_Remove / Title_Activate / Title_Deactivate
      Tag_Add / Tag_Remove / Tag_Activate / Tag_Deactivate
      TitleColor_Add / TitleColor_Remove / TitleColor_Activate / TitleColor_Deactivate
      NameColor_Add / NameColor_Remove / NameColor_Activate / NameColor_Deactivate
      LeftIcon_Add / LeftIcon_Remove / LeftIcon_Activate / LeftIcon_Deactivate
      RightIcon_Add / RightIcon_Remove / RightIcon_Activate / RightIcon_Deactivate
      GuildNickname_Set / GuildNickname_Clear

    Run this migration while the filter and GameServer are stopped.
*/

SET NOCOUNT ON;
SET XACT_ABORT ON;

BEGIN TRANSACTION;

UPDATE dbo.System_Settings
SET SettingName = N'HideTitleWhileTagActive'
WHERE SettingName = N'HideOldTitleWhileNewTitle'
  AND NOT EXISTS
  (
      SELECT 1
      FROM dbo.System_Settings
      WHERE SettingName = N'HideTitleWhileTagActive'
  );

DELETE dbo.System_Settings
WHERE SettingName = N'HideOldTitleWhileNewTitle';

IF COL_LENGTH(N'dbo.Achievement_List', N'RewardTitleID') IS NOT NULL
   AND COL_LENGTH(N'dbo.Achievement_List', N'RewardTagID') IS NULL
    EXEC sys.sp_rename N'dbo.Achievement_List.RewardTitleID', N'RewardTagID', N'COLUMN';

/* Rename the physical tables. No legacy views or synonyms are left behind. */
IF OBJECT_ID(N'dbo.Style_Titles', N'U') IS NOT NULL
    EXEC sys.sp_rename N'dbo.Style_Titles', N'Tags';
IF COL_LENGTH(N'dbo.Tags', N'TitleID') IS NOT NULL
    EXEC sys.sp_rename N'dbo.Tags.TitleID', N'TagID', N'COLUMN';
IF COL_LENGTH(N'dbo.Tags', N'TitleName') IS NOT NULL
    EXEC sys.sp_rename N'dbo.Tags.TitleName', N'TagName', N'COLUMN';

IF OBJECT_ID(N'dbo.Style_PlayerTitles', N'U') IS NOT NULL
    EXEC sys.sp_rename N'dbo.Style_PlayerTitles', N'PlayerTitles';

IF OBJECT_ID(N'dbo.Style_ActiveTitles', N'U') IS NOT NULL
    EXEC sys.sp_rename N'dbo.Style_ActiveTitles', N'ActiveTags';
IF COL_LENGTH(N'dbo.ActiveTags', N'RefTitleNameNewID') IS NOT NULL
    EXEC sys.sp_rename N'dbo.ActiveTags.RefTitleNameNewID', N'TagID', N'COLUMN';
IF OBJECT_ID(N'dbo.PK__ActiveTitleNameNew', N'PK') IS NOT NULL
   AND OBJECT_ID(N'dbo.PK_ActiveTags', N'PK') IS NULL
    EXEC sys.sp_rename N'dbo.PK__ActiveTitleNameNew', N'PK_ActiveTags', N'OBJECT';

IF OBJECT_ID(N'dbo.Style_PlayerTitleColors', N'U') IS NOT NULL
    EXEC sys.sp_rename N'dbo.Style_PlayerTitleColors', N'PlayerTitleColors';
IF OBJECT_ID(N'dbo.Style_ActiveTitleColors', N'U') IS NOT NULL
    EXEC sys.sp_rename N'dbo.Style_ActiveTitleColors', N'ActiveTitleColors';
IF OBJECT_ID(N'dbo.Style_ActiveNameColors', N'U') IS NOT NULL
    EXEC sys.sp_rename N'dbo.Style_ActiveNameColors', N'ActiveNameColors';

IF OBJECT_ID(N'dbo.Style_IconFiles', N'U') IS NOT NULL
    EXEC sys.sp_rename N'dbo.Style_IconFiles', N'Icons';
IF OBJECT_ID(N'dbo.Style_ActiveLeftIcons', N'U') IS NOT NULL
    EXEC sys.sp_rename N'dbo.Style_ActiveLeftIcons', N'ActiveLeftIcons';
IF OBJECT_ID(N'dbo.Style_ActiveRightIcons', N'U') IS NOT NULL
    EXEC sys.sp_rename N'dbo.Style_ActiveRightIcons', N'ActiveRightIcons';
IF OBJECT_ID(N'dbo.Style_GlobalColors', N'U') IS NOT NULL
    EXEC sys.sp_rename N'dbo.Style_GlobalColors', N'GlobalChatColors';
IF OBJECT_ID(N'dbo.Style_AutoCapeRegions', N'U') IS NOT NULL
    EXEC sys.sp_rename N'dbo.Style_AutoCapeRegions', N'AutoCapeRegions';

/* Tag ownership is independent from native/Hwan title ownership. */
IF OBJECT_ID(N'dbo.PlayerTags', N'U') IS NULL
BEGIN
    CREATE TABLE dbo.PlayerTags
    (
        ID INT IDENTITY(1,1) NOT NULL,
        CharID INT NOT NULL,
        TagID TINYINT NOT NULL,
        AddedAt DATETIME2(0) NOT NULL
            CONSTRAINT DF_PlayerTags_AddedAt DEFAULT (SYSUTCDATETIME()),
        CONSTRAINT PK_PlayerTags PRIMARY KEY CLUSTERED (ID),
        CONSTRAINT UQ_PlayerTags_Char_Tag UNIQUE (CharID, TagID)
    );
END;

/* Existing active tags are owned tags after migration. */
DECLARE @ShardDB SYSNAME = COALESCE
(
    NULLIF
    (
        (
            SELECT TOP (1) CONVERT(NVARCHAR(128), Value)
            FROM dbo.System_Settings WITH (NOLOCK)
            WHERE SettingName = N'ShardDB'
        ),
        N''
    ),
    N'SRO_VT_SHARD'
);

IF DB_ID(@ShardDB) IS NOT NULL
   AND OBJECT_ID(N'dbo.ActiveTags', N'U') IS NOT NULL
BEGIN
    DECLARE @MigrateActiveTags NVARCHAR(MAX) =
        N'INSERT dbo.PlayerTags (CharID, TagID)
          SELECT character.CharID, activeTag.TagID
          FROM dbo.ActiveTags AS activeTag
          INNER JOIN ' + QUOTENAME(@ShardDB) + N'.dbo._Char AS character
              ON character.CharName16 COLLATE DATABASE_DEFAULT
               = activeTag.CharName16 COLLATE DATABASE_DEFAULT
          WHERE NOT EXISTS
          (
              SELECT 1
              FROM dbo.PlayerTags AS ownedTag
              WHERE ownedTag.CharID = character.CharID
                AND ownedTag.TagID = activeTag.TagID
          );';
    EXEC sys.sp_executesql @MigrateActiveTags;
END;

/*
    Split owned icons by side. IDs stay stable because the client uses the
    ownership-row ID in its icon manager.
*/
IF OBJECT_ID(N'dbo.PlayerLeftIcons', N'U') IS NULL
BEGIN
    CREATE TABLE dbo.PlayerLeftIcons
    (
        ID INT IDENTITY(1,1) NOT NULL,
        CharID INT NOT NULL,
        IconID INT NOT NULL,
        CONSTRAINT PK_PlayerLeftIcons PRIMARY KEY CLUSTERED (ID),
        CONSTRAINT UQ_PlayerLeftIcons_Char_Icon UNIQUE (CharID, IconID)
    );
END;

IF OBJECT_ID(N'dbo.PlayerRightIcons', N'U') IS NULL
BEGIN
    CREATE TABLE dbo.PlayerRightIcons
    (
        ID INT IDENTITY(1,1) NOT NULL,
        CharID INT NOT NULL,
        IconID INT NOT NULL,
        CONSTRAINT PK_PlayerRightIcons PRIMARY KEY CLUSTERED (ID),
        CONSTRAINT UQ_PlayerRightIcons_Char_Icon UNIQUE (CharID, IconID)
    );
END;

IF OBJECT_ID(N'dbo.Style_PlayerIcons', N'U') IS NOT NULL
BEGIN
    SET IDENTITY_INSERT dbo.PlayerLeftIcons ON;
    INSERT dbo.PlayerLeftIcons (ID, CharID, IconID)
    SELECT source.ID, source.CharID, source.IconID
    FROM dbo.Style_PlayerIcons AS source
    WHERE source.Side = 0
      AND NOT EXISTS
      (
          SELECT 1 FROM dbo.PlayerLeftIcons AS target WHERE target.ID = source.ID
      );
    SET IDENTITY_INSERT dbo.PlayerLeftIcons OFF;

    SET IDENTITY_INSERT dbo.PlayerRightIcons ON;
    INSERT dbo.PlayerRightIcons (ID, CharID, IconID)
    SELECT source.ID, source.CharID, source.IconID
    FROM dbo.Style_PlayerIcons AS source
    WHERE source.Side = 1
      AND NOT EXISTS
      (
          SELECT 1 FROM dbo.PlayerRightIcons AS target WHERE target.ID = source.ID
      );
    SET IDENTITY_INSERT dbo.PlayerRightIcons OFF;

    DROP TABLE dbo.Style_PlayerIcons;
END;

/* Optional owned name-color collection, normalized to the existing color shape. */
IF OBJECT_ID(N'dbo.PlayerNameColors', N'U') IS NULL
BEGIN
    CREATE TABLE dbo.PlayerNameColors
    (
        ID INT IDENTITY(1,1) NOT NULL,
        CharID INT NOT NULL,
        ColorName VARCHAR(100) NOT NULL,
        ColorCode VARCHAR(100) NOT NULL,
        CONSTRAINT PK_PlayerNameColors PRIMARY KEY CLUSTERED (ID),
        CONSTRAINT UQ_PlayerNameColors_Char_Code UNIQUE (CharID, ColorCode)
    );
END;

GO

/* Remove the ambiguous procedure API before creating the final API. */
/*
    Some older installations expose the procedure API as synonyms rather
    than physical procedures. Remove those synonyms first so DROP PROCEDURE
    never targets an object of the wrong type.
*/
IF EXISTS (SELECT 1 FROM sys.synonyms WHERE schema_id = SCHEMA_ID(N'dbo') AND name = N'_AddColorToTitleManagerColor')
    DROP SYNONYM dbo._AddColorToTitleManagerColor;
IF EXISTS (SELECT 1 FROM sys.synonyms WHERE schema_id = SCHEMA_ID(N'dbo') AND name = N'_AddIconToIconManager')
    DROP SYNONYM dbo._AddIconToIconManager;
IF EXISTS (SELECT 1 FROM sys.synonyms WHERE schema_id = SCHEMA_ID(N'dbo') AND name = N'_AddTitleToTitleManager')
    DROP SYNONYM dbo._AddTitleToTitleManager;
IF EXISTS (SELECT 1 FROM sys.synonyms WHERE schema_id = SCHEMA_ID(N'dbo') AND name = N'_HandleIcons')
    DROP SYNONYM dbo._HandleIcons;
IF EXISTS (SELECT 1 FROM sys.synonyms WHERE schema_id = SCHEMA_ID(N'dbo') AND name = N'_HandleNewTitles')
    DROP SYNONYM dbo._HandleNewTitles;
IF EXISTS (SELECT 1 FROM sys.synonyms WHERE schema_id = SCHEMA_ID(N'dbo') AND name = N'_HandleTitleColors')
    DROP SYNONYM dbo._HandleTitleColors;
IF EXISTS (SELECT 1 FROM sys.synonyms WHERE schema_id = SCHEMA_ID(N'dbo') AND name = N'_RemoveIconLeftSide')
    DROP SYNONYM dbo._RemoveIconLeftSide;
IF EXISTS (SELECT 1 FROM sys.synonyms WHERE schema_id = SCHEMA_ID(N'dbo') AND name = N'_RemoveIconRightSide')
    DROP SYNONYM dbo._RemoveIconRightSide;
IF EXISTS (SELECT 1 FROM sys.synonyms WHERE schema_id = SCHEMA_ID(N'dbo') AND name = N'_RemoveNameColor')
    DROP SYNONYM dbo._RemoveNameColor;
IF EXISTS (SELECT 1 FROM sys.synonyms WHERE schema_id = SCHEMA_ID(N'dbo') AND name = N'_RemoveNewTitle')
    DROP SYNONYM dbo._RemoveNewTitle;
IF EXISTS (SELECT 1 FROM sys.synonyms WHERE schema_id = SCHEMA_ID(N'dbo') AND name = N'_RemoveTitleColor')
    DROP SYNONYM dbo._RemoveTitleColor;
IF EXISTS (SELECT 1 FROM sys.synonyms WHERE schema_id = SCHEMA_ID(N'dbo') AND name = N'_UpdateIconLeftSide')
    DROP SYNONYM dbo._UpdateIconLeftSide;
IF EXISTS (SELECT 1 FROM sys.synonyms WHERE schema_id = SCHEMA_ID(N'dbo') AND name = N'_UpdateIconRightSide')
    DROP SYNONYM dbo._UpdateIconRightSide;
IF EXISTS (SELECT 1 FROM sys.synonyms WHERE schema_id = SCHEMA_ID(N'dbo') AND name = N'_UpdateNameColor')
    DROP SYNONYM dbo._UpdateNameColor;
IF EXISTS (SELECT 1 FROM sys.synonyms WHERE schema_id = SCHEMA_ID(N'dbo') AND name = N'_UpdateNewTitle')
    DROP SYNONYM dbo._UpdateNewTitle;
IF EXISTS (SELECT 1 FROM sys.synonyms WHERE schema_id = SCHEMA_ID(N'dbo') AND name = N'_UpdateTitleColor')
    DROP SYNONYM dbo._UpdateTitleColor;

IF EXISTS (SELECT 1 FROM sys.synonyms WHERE schema_id = SCHEMA_ID(N'appearance') AND name = N'AddIcon')
    DROP SYNONYM appearance.AddIcon;
IF EXISTS (SELECT 1 FROM sys.synonyms WHERE schema_id = SCHEMA_ID(N'appearance') AND name = N'AddTitle')
    DROP SYNONYM appearance.AddTitle;
IF EXISTS (SELECT 1 FROM sys.synonyms WHERE schema_id = SCHEMA_ID(N'appearance') AND name = N'AddTitleColor')
    DROP SYNONYM appearance.AddTitleColor;
IF EXISTS (SELECT 1 FROM sys.synonyms WHERE schema_id = SCHEMA_ID(N'appearance') AND name = N'RemoveLeftIcon')
    DROP SYNONYM appearance.RemoveLeftIcon;
IF EXISTS (SELECT 1 FROM sys.synonyms WHERE schema_id = SCHEMA_ID(N'appearance') AND name = N'RemoveNameColor')
    DROP SYNONYM appearance.RemoveNameColor;
IF EXISTS (SELECT 1 FROM sys.synonyms WHERE schema_id = SCHEMA_ID(N'appearance') AND name = N'RemoveRightIcon')
    DROP SYNONYM appearance.RemoveRightIcon;
IF EXISTS (SELECT 1 FROM sys.synonyms WHERE schema_id = SCHEMA_ID(N'appearance') AND name = N'RemoveTitle')
    DROP SYNONYM appearance.RemoveTitle;
IF EXISTS (SELECT 1 FROM sys.synonyms WHERE schema_id = SCHEMA_ID(N'appearance') AND name = N'RemoveTitleColor')
    DROP SYNONYM appearance.RemoveTitleColor;
IF EXISTS (SELECT 1 FROM sys.synonyms WHERE schema_id = SCHEMA_ID(N'appearance') AND name = N'SelectIcon')
    DROP SYNONYM appearance.SelectIcon;
IF EXISTS (SELECT 1 FROM sys.synonyms WHERE schema_id = SCHEMA_ID(N'appearance') AND name = N'SelectTitle')
    DROP SYNONYM appearance.SelectTitle;
IF EXISTS (SELECT 1 FROM sys.synonyms WHERE schema_id = SCHEMA_ID(N'appearance') AND name = N'SelectTitleColor')
    DROP SYNONYM appearance.SelectTitleColor;
IF EXISTS (SELECT 1 FROM sys.synonyms WHERE schema_id = SCHEMA_ID(N'appearance') AND name = N'SwitchCustomGlow')
    DROP SYNONYM appearance.SwitchCustomGlow;
IF EXISTS (SELECT 1 FROM sys.synonyms WHERE schema_id = SCHEMA_ID(N'appearance') AND name = N'SwitchCustomModel')
    DROP SYNONYM appearance.SwitchCustomModel;
IF EXISTS (SELECT 1 FROM sys.synonyms WHERE schema_id = SCHEMA_ID(N'appearance') AND name = N'UpdateLeftIcon')
    DROP SYNONYM appearance.UpdateLeftIcon;
IF EXISTS (SELECT 1 FROM sys.synonyms WHERE schema_id = SCHEMA_ID(N'appearance') AND name = N'UpdateNameColor')
    DROP SYNONYM appearance.UpdateNameColor;
IF EXISTS (SELECT 1 FROM sys.synonyms WHERE schema_id = SCHEMA_ID(N'appearance') AND name = N'UpdateRightIcon')
    DROP SYNONYM appearance.UpdateRightIcon;
IF EXISTS (SELECT 1 FROM sys.synonyms WHERE schema_id = SCHEMA_ID(N'appearance') AND name = N'UpdateTitle')
    DROP SYNONYM appearance.UpdateTitle;
IF EXISTS (SELECT 1 FROM sys.synonyms WHERE schema_id = SCHEMA_ID(N'appearance') AND name = N'UpdateTitleColor')
    DROP SYNONYM appearance.UpdateTitleColor;

DROP PROCEDURE IF EXISTS dbo.Style_AddTitle;
DROP PROCEDURE IF EXISTS dbo.Style_ChooseTitle;
DROP PROCEDURE IF EXISTS dbo.Style_UpdateTitle;
DROP PROCEDURE IF EXISTS dbo.Style_RemoveTitle;
DROP PROCEDURE IF EXISTS dbo.Style_AddTitleColor;
DROP PROCEDURE IF EXISTS dbo.Style_ChooseTitleColor;
DROP PROCEDURE IF EXISTS dbo.Style_UpdateTitleColor;
DROP PROCEDURE IF EXISTS dbo.Style_RemoveTitleColor;
DROP PROCEDURE IF EXISTS dbo.Style_UpdateNameColor;
DROP PROCEDURE IF EXISTS dbo.Style_RemoveNameColor;
DROP PROCEDURE IF EXISTS dbo.Style_AddIcon;
DROP PROCEDURE IF EXISTS dbo.Style_ChooseIcon;
DROP PROCEDURE IF EXISTS dbo.Style_UpdateLeftIcon;
DROP PROCEDURE IF EXISTS dbo.Style_RemoveLeftIcon;
DROP PROCEDURE IF EXISTS dbo.Style_UpdateRightIcon;
DROP PROCEDURE IF EXISTS dbo.Style_RemoveRightIcon;
DROP PROCEDURE IF EXISTS dbo.Style_GrantAndActivateHwanTitle;
DROP PROCEDURE IF EXISTS dbo.Command_ChangeName;

IF OBJECT_ID(N'dbo.Style_ChangeGlow', N'P') IS NOT NULL
    EXEC sys.sp_rename N'dbo.Style_ChangeGlow', N'ItemGlow_Change';
IF OBJECT_ID(N'dbo.Style_ChangeModel', N'P') IS NOT NULL
    EXEC sys.sp_rename N'dbo.Style_ChangeModel', N'ItemModel_Change';

/* Remove every pre-flatten and schema-organized API name as well. */
DROP PROCEDURE IF EXISTS dbo._AddColorToTitleManagerColor;
DROP PROCEDURE IF EXISTS dbo._AddIconToIconManager;
DROP PROCEDURE IF EXISTS dbo._AddTitleToTitleManager;
DROP PROCEDURE IF EXISTS dbo._HandleIcons;
DROP PROCEDURE IF EXISTS dbo._HandleNewTitles;
DROP PROCEDURE IF EXISTS dbo._HandleTitleColors;
DROP PROCEDURE IF EXISTS dbo._RemoveIconLeftSide;
DROP PROCEDURE IF EXISTS dbo._RemoveIconRightSide;
DROP PROCEDURE IF EXISTS dbo._RemoveNameColor;
DROP PROCEDURE IF EXISTS dbo._RemoveNewTitle;
DROP PROCEDURE IF EXISTS dbo._RemoveTitleColor;
DROP PROCEDURE IF EXISTS dbo._UpdateIconLeftSide;
DROP PROCEDURE IF EXISTS dbo._UpdateIconRightSide;
DROP PROCEDURE IF EXISTS dbo._UpdateNameColor;
DROP PROCEDURE IF EXISTS dbo._UpdateNewTitle;
DROP PROCEDURE IF EXISTS dbo._UpdateTitleColor;

DROP PROCEDURE IF EXISTS appearance.AddIcon;
DROP PROCEDURE IF EXISTS appearance.AddTitle;
DROP PROCEDURE IF EXISTS appearance.AddTitleColor;
DROP PROCEDURE IF EXISTS appearance.RemoveLeftIcon;
DROP PROCEDURE IF EXISTS appearance.RemoveNameColor;
DROP PROCEDURE IF EXISTS appearance.RemoveRightIcon;
DROP PROCEDURE IF EXISTS appearance.RemoveTitle;
DROP PROCEDURE IF EXISTS appearance.RemoveTitleColor;
DROP PROCEDURE IF EXISTS appearance.SelectIcon;
DROP PROCEDURE IF EXISTS appearance.SelectTitle;
DROP PROCEDURE IF EXISTS appearance.SelectTitleColor;
DROP PROCEDURE IF EXISTS appearance.SwitchCustomGlow;
DROP PROCEDURE IF EXISTS appearance.SwitchCustomModel;
DROP PROCEDURE IF EXISTS appearance.UpdateLeftIcon;
DROP PROCEDURE IF EXISTS appearance.UpdateNameColor;
DROP PROCEDURE IF EXISTS appearance.UpdateRightIcon;
DROP PROCEDURE IF EXISTS appearance.UpdateTitle;
DROP PROCEDURE IF EXISTS appearance.UpdateTitleColor;

DROP SYNONYM IF EXISTS dbo._ActiveIconsLeftSide;
DROP SYNONYM IF EXISTS dbo._ActiveIconsRightSide;
DROP SYNONYM IF EXISTS dbo._ActiveNameColors;
DROP SYNONYM IF EXISTS dbo._ActiveTitleColors;
DROP SYNONYM IF EXISTS dbo._ActiveTitleNameNew;
DROP SYNONYM IF EXISTS dbo._CharacterIconManager;
DROP SYNONYM IF EXISTS dbo._CharacterTitleManager;
DROP SYNONYM IF EXISTS dbo._CharacterTitleManagerColor;
DROP SYNONYM IF EXISTS dbo._RefGlobalColor;
DROP SYNONYM IF EXISTS dbo._RefIconsMediaPath;
DROP SYNONYM IF EXISTS dbo._RefTitleNameNew;
DROP SYNONYM IF EXISTS dbo._ServerAutoCapebyRegionID;

DROP VIEW IF EXISTS dbo._ActiveIconsLeftSide;
DROP VIEW IF EXISTS dbo._ActiveIconsRightSide;
DROP VIEW IF EXISTS dbo._ActiveNameColors;
DROP VIEW IF EXISTS dbo._ActiveTitleColors;
DROP VIEW IF EXISTS dbo._ActiveTitleNameNew;
DROP VIEW IF EXISTS dbo._CharacterIconManager;
DROP VIEW IF EXISTS dbo._CharacterTitleManager;
DROP VIEW IF EXISTS dbo._CharacterTitleManagerColor;
DROP VIEW IF EXISTS dbo._RefGlobalColor;
DROP VIEW IF EXISTS dbo._RefIconsMediaPath;
DROP VIEW IF EXISTS dbo._RefTitleNameNew;
DROP VIEW IF EXISTS dbo._ServerAutoCapebyRegionID;
GO

CREATE OR ALTER PROCEDURE dbo.Title_Add
    @CharID INT,
    @TitleID TINYINT
AS
BEGIN
    SET NOCOUNT ON;
    SET XACT_ABORT ON;

    IF @CharID <= 0 OR @TitleID <= 0
        THROW 51000, 'CharID and TitleID must be greater than zero.', 1;

    IF NOT EXISTS
    (
        SELECT 1 FROM dbo.PlayerTitles
        WHERE CharID = @CharID AND TitleID = @TitleID
    )
    BEGIN
        INSERT dbo.PlayerTitles (CharID, TitleID) VALUES (@CharID, @TitleID);
        INSERT dbo.Command_FilterQueue (CommandID, Data1, Data2, Status)
        VALUES (4, @CharID, @TitleID, 1);
    END;
END;
GO

CREATE OR ALTER PROCEDURE dbo.Title_Remove
    @CharID INT,
    @TitleID TINYINT
AS
BEGIN
    SET NOCOUNT ON;
    DELETE dbo.PlayerTitles WHERE CharID = @CharID AND TitleID = @TitleID;
END;
GO

CREATE OR ALTER PROCEDURE dbo.Title_Activate
    @CharID INT,
    @TitleID TINYINT
AS
BEGIN
    SET NOCOUNT ON;

    IF NOT EXISTS
    (
        SELECT 1 FROM dbo.PlayerTitles
        WHERE CharID = @CharID AND TitleID = @TitleID
    )
        THROW 51001, 'The player does not own this title. Run Title_Add first.', 1;

    INSERT dbo.Command_FilterQueue (CommandID, Data1, Data2, Status)
    VALUES (7, @CharID, @TitleID, 1);
END;
GO

CREATE OR ALTER PROCEDURE dbo.Title_Deactivate
    @CharID INT
AS
BEGIN
    SET NOCOUNT ON;
    INSERT dbo.Command_FilterQueue (CommandID, Data1, Data2, Status)
    VALUES (7, @CharID, 0, 1);
END;
GO

CREATE OR ALTER PROCEDURE dbo.Tag_Add
    @CharID INT,
    @TagID TINYINT
AS
BEGIN
    SET NOCOUNT ON;

    IF NOT EXISTS (SELECT 1 FROM dbo.Tags WHERE TagID = @TagID)
        THROW 51002, 'TagID does not exist.', 1;

    IF NOT EXISTS
    (
        SELECT 1 FROM dbo.PlayerTags
        WHERE CharID = @CharID AND TagID = @TagID
    )
        INSERT dbo.PlayerTags (CharID, TagID) VALUES (@CharID, @TagID);
END;
GO

CREATE OR ALTER PROCEDURE dbo.Tag_Remove
    @CharID INT,
    @TagID TINYINT,
    @CharName16 VARCHAR(16) = NULL
AS
BEGIN
    SET NOCOUNT ON;
    SET XACT_ABORT ON;
    BEGIN TRANSACTION;

    DELETE dbo.PlayerTags WHERE CharID = @CharID AND TagID = @TagID;

    IF @CharName16 IS NOT NULL
       AND EXISTS
       (
           SELECT 1 FROM dbo.ActiveTags
           WHERE CharName16 = @CharName16 AND TagID = @TagID
       )
    BEGIN
        DELETE dbo.ActiveTags WHERE CharName16 = @CharName16;
        INSERT dbo.Command_FilterQueue (CommandID, Data1, Status)
        VALUES (16, @CharName16, 1);
    END;

    COMMIT TRANSACTION;
END;
GO

CREATE OR ALTER PROCEDURE dbo.Tag_Activate
    @CharID INT,
    @CharName16 VARCHAR(16),
    @TagID TINYINT,
    @QueueRuntime BIT = 1
AS
BEGIN
    SET NOCOUNT ON;
    SET XACT_ABORT ON;

    IF NOT EXISTS
    (
        SELECT 1 FROM dbo.PlayerTags
        WHERE CharID = @CharID AND TagID = @TagID
    )
        THROW 51003, 'The player does not own this tag. Run Tag_Add first.', 1;

    BEGIN TRANSACTION;

    UPDATE dbo.ActiveTags SET TagID = @TagID WHERE CharName16 = @CharName16;
    IF @@ROWCOUNT = 0
        INSERT dbo.ActiveTags (CharName16, TagID) VALUES (@CharName16, @TagID);

    IF @QueueRuntime = 1
        INSERT dbo.Command_FilterQueue (CommandID, Data1, Data2, Status)
        VALUES (15, @CharName16, @TagID, 1);

    COMMIT TRANSACTION;
END;
GO

CREATE OR ALTER PROCEDURE dbo.Tag_Deactivate
    @CharName16 VARCHAR(16),
    @QueueRuntime BIT = 1
AS
BEGIN
    SET NOCOUNT ON;
    DELETE dbo.ActiveTags WHERE CharName16 = @CharName16;
    IF @@ROWCOUNT > 0 AND @QueueRuntime = 1
        INSERT dbo.Command_FilterQueue (CommandID, Data1, Status)
        VALUES (16, @CharName16, 1);
END;
GO

CREATE OR ALTER PROCEDURE dbo.TitleColor_Add
    @CharID INT,
    @ColorCode VARCHAR(100),
    @ColorName VARCHAR(100)
AS
BEGIN
    SET NOCOUNT ON;

    IF NOT EXISTS
    (
        SELECT 1 FROM dbo.PlayerTitleColors
        WHERE CharID = @CharID AND ColorCode = @ColorCode
    )
    BEGIN
        INSERT dbo.PlayerTitleColors (CharID, ColorName, ColorCode)
        VALUES (@CharID, @ColorName, @ColorCode);

        DECLARE @ID INT = CONVERT(INT, SCOPE_IDENTITY());
        INSERT dbo.Command_FilterQueue
            (CommandID, Data1, Data2, Data3, Data4, Status)
        VALUES
            (5, @CharID, @ColorName, @ColorCode, @ID, 1);
    END;
END;
GO

CREATE OR ALTER PROCEDURE dbo.TitleColor_Remove
    @CharID INT,
    @ColorID INT
AS
BEGIN
    SET NOCOUNT ON;
    DELETE dbo.PlayerTitleColors WHERE ID = @ColorID AND CharID = @CharID;
END;
GO

CREATE OR ALTER PROCEDURE dbo.TitleColor_Activate
    @CharID INT,
    @CharName16 VARCHAR(16),
    @ColorID INT,
    @QueueRuntime BIT = 1
AS
BEGIN
    SET NOCOUNT ON;
    DECLARE @ColorCode VARCHAR(100);

    SELECT @ColorCode = ColorCode
    FROM dbo.PlayerTitleColors
    WHERE ID = @ColorID AND CharID = @CharID;

    IF @ColorCode IS NULL
        THROW 51004, 'The player does not own this title color.', 1;

    UPDATE dbo.ActiveTitleColors SET ColorCode = @ColorCode WHERE CharName16 = @CharName16;
    IF @@ROWCOUNT = 0
        INSERT dbo.ActiveTitleColors (CharName16, ColorCode) VALUES (@CharName16, @ColorCode);

    IF @QueueRuntime = 1
        INSERT dbo.Command_FilterQueue (CommandID, Data1, Data2, Status)
        VALUES (8, @CharName16, @ColorCode, 1);
END;
GO

CREATE OR ALTER PROCEDURE dbo.TitleColor_Deactivate
    @CharName16 VARCHAR(16),
    @QueueRuntime BIT = 1
AS
BEGIN
    SET NOCOUNT ON;
    DELETE dbo.ActiveTitleColors WHERE CharName16 = @CharName16;
    IF @@ROWCOUNT > 0 AND @QueueRuntime = 1
        INSERT dbo.Command_FilterQueue (CommandID, Data1, Status)
        VALUES (9, @CharName16, 1);
END;
GO

CREATE OR ALTER PROCEDURE dbo.NameColor_Add
    @CharID INT,
    @ColorCode VARCHAR(100),
    @ColorName VARCHAR(100)
AS
BEGIN
    SET NOCOUNT ON;
    IF NOT EXISTS
    (
        SELECT 1 FROM dbo.PlayerNameColors
        WHERE CharID = @CharID AND ColorCode = @ColorCode
    )
        INSERT dbo.PlayerNameColors (CharID, ColorName, ColorCode)
        VALUES (@CharID, @ColorName, @ColorCode);
END;
GO

CREATE OR ALTER PROCEDURE dbo.NameColor_Remove
    @CharID INT,
    @ColorID INT
AS
BEGIN
    SET NOCOUNT ON;
    DELETE dbo.PlayerNameColors WHERE ID = @ColorID AND CharID = @CharID;
END;
GO

CREATE OR ALTER PROCEDURE dbo.NameColor_Activate
    @CharID INT,
    @CharName16 VARCHAR(16),
    @ColorID INT,
    @QueueRuntime BIT = 1
AS
BEGIN
    SET NOCOUNT ON;
    DECLARE @ColorCode VARCHAR(100);

    SELECT @ColorCode = ColorCode
    FROM dbo.PlayerNameColors
    WHERE ID = @ColorID AND CharID = @CharID;

    IF @ColorCode IS NULL
        THROW 51005, 'The player does not own this name color.', 1;

    UPDATE dbo.ActiveNameColors SET ColorCode = @ColorCode WHERE CharName16 = @CharName16;
    IF @@ROWCOUNT = 0
        INSERT dbo.ActiveNameColors (CharName16, ColorCode) VALUES (@CharName16, @ColorCode);

    IF @QueueRuntime = 1
        INSERT dbo.Command_FilterQueue (CommandID, Data1, Data2, Status)
        VALUES (37, @CharName16, @ColorCode, 1);
END;
GO

CREATE OR ALTER PROCEDURE dbo.NameColor_Deactivate
    @CharName16 VARCHAR(16),
    @QueueRuntime BIT = 1
AS
BEGIN
    SET NOCOUNT ON;
    DELETE dbo.ActiveNameColors WHERE CharName16 = @CharName16;
    IF @@ROWCOUNT > 0 AND @QueueRuntime = 1
        INSERT dbo.Command_FilterQueue (CommandID, Data1, Status)
        VALUES (38, @CharName16, 1);
END;
GO

CREATE OR ALTER PROCEDURE dbo.LeftIcon_Add
    @CharID INT,
    @IconID INT
AS
BEGIN
    SET NOCOUNT ON;
    IF NOT EXISTS (SELECT 1 FROM dbo.Icons WHERE IconID = @IconID)
        THROW 51006, 'IconID does not exist.', 1;

    IF NOT EXISTS
    (
        SELECT 1 FROM dbo.PlayerLeftIcons
        WHERE CharID = @CharID AND IconID = @IconID
    )
    BEGIN
        INSERT dbo.PlayerLeftIcons (CharID, IconID) VALUES (@CharID, @IconID);
        DECLARE @ID INT = CONVERT(INT, SCOPE_IDENTITY());
        INSERT dbo.Command_FilterQueue
            (CommandID, Data1, Data2, Data3, Data4, Status)
        VALUES
            (6, @CharID, @IconID, 0, @ID, 1);
    END;
END;
GO

CREATE OR ALTER PROCEDURE dbo.LeftIcon_Remove
    @CharID INT,
    @IconID INT
AS
BEGIN
    SET NOCOUNT ON;
    DELETE dbo.PlayerLeftIcons WHERE CharID = @CharID AND IconID = @IconID;
END;
GO

CREATE OR ALTER PROCEDURE dbo.LeftIcon_Activate
    @CharID INT,
    @CharName16 VARCHAR(16),
    @IconID INT,
    @QueueRuntime BIT = 1
AS
BEGIN
    SET NOCOUNT ON;
    IF NOT EXISTS
    (
        SELECT 1 FROM dbo.PlayerLeftIcons
        WHERE CharID = @CharID AND IconID = @IconID
    )
        THROW 51007, 'The player does not own this left icon.', 1;

    UPDATE dbo.ActiveLeftIcons SET IconID = @IconID WHERE CharName16 = @CharName16;
    IF @@ROWCOUNT = 0
        INSERT dbo.ActiveLeftIcons (CharName16, IconID) VALUES (@CharName16, @IconID);

    IF @QueueRuntime = 1
        INSERT dbo.Command_FilterQueue (CommandID, Data1, Data2, Status)
        VALUES (10, @CharName16, @IconID, 1);
END;
GO

CREATE OR ALTER PROCEDURE dbo.LeftIcon_Deactivate
    @CharName16 VARCHAR(16),
    @QueueRuntime BIT = 1
AS
BEGIN
    SET NOCOUNT ON;
    DELETE dbo.ActiveLeftIcons WHERE CharName16 = @CharName16;
    IF @@ROWCOUNT > 0 AND @QueueRuntime = 1
        INSERT dbo.Command_FilterQueue (CommandID, Data1, Status)
        VALUES (12, @CharName16, 1);
END;
GO

CREATE OR ALTER PROCEDURE dbo.RightIcon_Add
    @CharID INT,
    @IconID INT
AS
BEGIN
    SET NOCOUNT ON;
    IF NOT EXISTS (SELECT 1 FROM dbo.Icons WHERE IconID = @IconID)
        THROW 51008, 'IconID does not exist.', 1;

    IF NOT EXISTS
    (
        SELECT 1 FROM dbo.PlayerRightIcons
        WHERE CharID = @CharID AND IconID = @IconID
    )
    BEGIN
        INSERT dbo.PlayerRightIcons (CharID, IconID) VALUES (@CharID, @IconID);
        DECLARE @ID INT = CONVERT(INT, SCOPE_IDENTITY());
        INSERT dbo.Command_FilterQueue
            (CommandID, Data1, Data2, Data3, Data4, Status)
        VALUES
            (6, @CharID, @IconID, 1, @ID, 1);
    END;
END;
GO

CREATE OR ALTER PROCEDURE dbo.RightIcon_Remove
    @CharID INT,
    @IconID INT
AS
BEGIN
    SET NOCOUNT ON;
    DELETE dbo.PlayerRightIcons WHERE CharID = @CharID AND IconID = @IconID;
END;
GO

CREATE OR ALTER PROCEDURE dbo.RightIcon_Activate
    @CharID INT,
    @CharName16 VARCHAR(16),
    @IconID INT,
    @QueueRuntime BIT = 1
AS
BEGIN
    SET NOCOUNT ON;
    IF NOT EXISTS
    (
        SELECT 1 FROM dbo.PlayerRightIcons
        WHERE CharID = @CharID AND IconID = @IconID
    )
        THROW 51009, 'The player does not own this right icon.', 1;

    UPDATE dbo.ActiveRightIcons SET IconID = @IconID WHERE CharName16 = @CharName16;
    IF @@ROWCOUNT = 0
        INSERT dbo.ActiveRightIcons (CharName16, IconID) VALUES (@CharName16, @IconID);

    IF @QueueRuntime = 1
        INSERT dbo.Command_FilterQueue (CommandID, Data1, Data2, Status)
        VALUES (11, @CharName16, @IconID, 1);
END;
GO

CREATE OR ALTER PROCEDURE dbo.RightIcon_Deactivate
    @CharName16 VARCHAR(16),
    @QueueRuntime BIT = 1
AS
BEGIN
    SET NOCOUNT ON;
    DELETE dbo.ActiveRightIcons WHERE CharName16 = @CharName16;
    IF @@ROWCOUNT > 0 AND @QueueRuntime = 1
        INSERT dbo.Command_FilterQueue (CommandID, Data1, Status)
        VALUES (13, @CharName16, 1);
END;
GO

CREATE OR ALTER PROCEDURE dbo.GuildNickname_Set
    @CharID INT,
    @Nickname VARCHAR(12)
AS
BEGIN
    SET NOCOUNT ON;
    IF @Nickname IS NULL
       OR LEN(@Nickname) NOT BETWEEN 2 AND 12
       OR @Nickname LIKE '%[^A-Za-z0-9_]%'
        THROW 51010, 'Guild nickname must contain 2-12 English letters, digits, or underscore.', 1;

    INSERT dbo.Command_GameServerQueue (Action_ID, Data1, Data2)
    VALUES (1, @CharID, @Nickname);
END;
GO

CREATE OR ALTER PROCEDURE dbo.GuildNickname_Clear
    @CharID INT
AS
BEGIN
    SET NOCOUNT ON;
    INSERT dbo.Command_GameServerQueue (Action_ID, Data1, Data2)
    VALUES (1, @CharID, '');
END;
GO

/*
    A successful migration must leave no runtime object from the old style API.
    Constraint names are metadata only and are intentionally not part of this
    check; tables, views, synonyms, and procedures are.
*/
IF EXISTS
(
    SELECT 1
    FROM sys.objects AS objectRow
    WHERE SCHEMA_NAME(objectRow.schema_id) IN (N'dbo', N'appearance')
      AND objectRow.name IN
      (
          N'Style_Titles',
          N'Style_PlayerTitles',
          N'Style_ActiveTitles',
          N'Style_PlayerTitleColors',
          N'Style_ActiveTitleColors',
          N'Style_ActiveNameColors',
          N'Style_IconFiles',
          N'Style_PlayerIcons',
          N'Style_ActiveLeftIcons',
          N'Style_ActiveRightIcons',
          N'Style_GlobalColors',
          N'Style_AutoCapeRegions',
          N'Style_AddTitle',
          N'Style_ChooseTitle',
          N'Style_UpdateTitle',
          N'Style_RemoveTitle',
          N'Style_AddTitleColor',
          N'Style_ChooseTitleColor',
          N'Style_UpdateTitleColor',
          N'Style_RemoveTitleColor',
          N'Style_UpdateNameColor',
          N'Style_RemoveNameColor',
          N'Style_AddIcon',
          N'Style_ChooseIcon',
          N'Style_UpdateLeftIcon',
          N'Style_RemoveLeftIcon',
          N'Style_UpdateRightIcon',
          N'Style_RemoveRightIcon',
          N'Style_GrantAndActivateHwanTitle',
          N'Style_ChangeGlow',
          N'Style_ChangeModel',
          N'Command_ChangeName',
          N'AddIcon',
          N'AddTitle',
          N'AddTitleColor',
          N'RemoveLeftIcon',
          N'RemoveNameColor',
          N'RemoveRightIcon',
          N'RemoveTitle',
          N'RemoveTitleColor',
          N'SelectIcon',
          N'SelectTitle',
          N'SelectTitleColor',
          N'SwitchCustomGlow',
          N'SwitchCustomModel',
          N'UpdateLeftIcon',
          N'UpdateNameColor',
          N'UpdateRightIcon',
          N'UpdateTitle',
          N'UpdateTitleColor'
      )
)
    THROW 51099, 'Player style migration left one or more legacy objects behind.', 1;

IF EXISTS
(
    SELECT 1
    FROM sys.synonyms
    WHERE SCHEMA_NAME(schema_id) = N'dbo'
      AND name IN
      (
          N'_ActiveIconsLeftSide',
          N'_ActiveIconsRightSide',
          N'_ActiveNameColors',
          N'_ActiveTitleColors',
          N'_ActiveTitleNameNew',
          N'_CharacterIconManager',
          N'_CharacterTitleManager',
          N'_CharacterTitleManagerColor',
          N'_RefGlobalColor',
          N'_RefIconsMediaPath',
          N'_RefTitleNameNew',
          N'_ServerAutoCapebyRegionID'
      )
)
    THROW 51100, 'Player style migration left one or more legacy synonyms behind.', 1;

COMMIT TRANSACTION;
GO
