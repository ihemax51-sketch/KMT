USE [SRO_VT_LOG]
GO

SET ANSI_NULLS ON
GO
SET QUOTED_IDENTIFIER ON
GO

/*
    Customer migration: MaxiGuard_User -> KMTGuard

    Only the old-filter calls that targeted MaxiGuard_User were converted.
    SRO_VT_SWITCHER and the local _Mars* procedures are intentionally kept.

    KMTGuard mappings used here:
      MaxiGuard _ChangeCharacterIcon       -> LeftIcon_Add + LeftIcon_Activate
      MaxiGuard _AddTitleToCharacter       -> Title_Add
      MaxiGuard _ChangeCharacterTitle      -> Title_Activate
      MaxiGuard _ChangeCharacterTitleColor -> TitleColor_Add + TitleColor_Activate
      MaxiGuard _BridgeCommands notice     -> Command_NoticeAll
      MaxiGuard _AddItemToChest            -> Item_AddChest (resolved by RefObjCommon.ID)
*/

ALTER PROCEDURE [dbo].[_AddLogItem]
    @CharID        INT,
    @ItemRefID     INT,
    @ItemSerial    BIGINT,
    @dwData        INT,
    @TargetStorage TINYINT,
    @Operation     TINYINT,
    @Slot_From     TINYINT,
    @Slot_To       TINYINT,
    @EventPos      VARCHAR(64),
    @strDesc       VARCHAR(128),
    @Gold          BIGINT
AS
BEGIN
    SET NOCOUNT ON;

    ---------------------------------------------------------------------------
    -- Preserve the original SRO item logging behavior.
    ---------------------------------------------------------------------------
    DECLARE @LenPos INT = LEN(@EventPos);
    DECLARE @LenDesc INT = LEN(@strDesc);

    IF (@LenPos > 0 AND @LenDesc > 0)
    BEGIN
        INSERT INTO [dbo].[_LogEventItem]
            (EventTime, CharID, ItemRefID, dwData, TargetStorage, Operation,
             Slot_From, Slot_To, EventPos, strDesc, Serial64, Gold)
        VALUES
            (GETDATE(), @CharID, @ItemRefID, @dwData, @TargetStorage, @Operation,
             @Slot_From, @Slot_To, @EventPos, @strDesc, @ItemSerial, @Gold);
    END
    ELSE IF (@LenPos > 0 AND @LenDesc = 0)
    BEGIN
        INSERT INTO [dbo].[_LogEventItem]
            (EventTime, CharID, ItemRefID, dwData, TargetStorage, Operation,
             Slot_From, Slot_To, EventPos, Serial64, Gold)
        VALUES
            (GETDATE(), @CharID, @ItemRefID, @dwData, @TargetStorage, @Operation,
             @Slot_From, @Slot_To, @EventPos, @ItemSerial, @Gold);
    END
    ELSE IF (@LenPos = 0 AND @LenDesc > 0)
    BEGIN
        INSERT INTO [dbo].[_LogEventItem]
            (EventTime, CharID, ItemRefID, dwData, TargetStorage, Operation,
             Slot_From, Slot_To, strDesc, Serial64, Gold)
        VALUES
            (GETDATE(), @CharID, @ItemRefID, @dwData, @TargetStorage, @Operation,
             @Slot_From, @Slot_To, @strDesc, @ItemSerial, @Gold);
    END
    ELSE IF (@LenPos = 0 AND @LenDesc = 0)
    BEGIN
        INSERT INTO [dbo].[_LogEventItem]
            (EventTime, CharID, ItemRefID, dwData, TargetStorage, Operation,
             Slot_From, Slot_To, Serial64, Gold)
        VALUES
            (GETDATE(), @CharID, @ItemRefID, @dwData, @TargetStorage, @Operation,
             @Slot_From, @Slot_To, @ItemSerial, @Gold);
    END;

    IF (@Operation = 35)
    BEGIN
        INSERT INTO [dbo].[_LogCashItem]
            (RefItemID, CharID, Cnt, EventTime, Serial64)
        VALUES
            (@ItemRefID, @CharID, @dwData, GETDATE(), @ItemSerial);
    END;

    DECLARE @CharName16 VARCHAR(16);

    SELECT @CharName16 = C.CharName16
    FROM [SRO_VT_SHARD].[dbo].[_Char] AS C WITH (NOLOCK)
    WHERE C.CharID = @CharID;

    ---------------------------------------------------------------------------
    -- Character icon scrolls (left-side icon).
    -- Old MaxiGuard icon resources 1..23 map to KMTGuard rudiment icons 7..29.
    ---------------------------------------------------------------------------
    IF (@Operation = 16 AND @ItemRefID BETWEEN 43982 AND 44004
        AND @CharName16 IS NOT NULL)
    BEGIN
        DECLARE @KmtIconID INT = @ItemRefID - 43975;

        EXEC [KMTGuard].[dbo].[LeftIcon_Add]
            @CharID = @CharID,
            @IconID = @KmtIconID;

        EXEC [KMTGuard].[dbo].[LeftIcon_Activate]
            @CharID = @CharID,
            @CharName16 = @CharName16,
            @IconID = @KmtIconID;
    END;

    IF (@Operation = 16 AND @ItemRefID = 43920 AND @CharName16 IS NOT NULL)
    BEGIN
        EXEC [KMTGuard].[dbo].[LeftIcon_Deactivate]
            @CharName16 = @CharName16;
    END;

    ---------------------------------------------------------------------------
    -- Existing customer Model/Glow switcher. This is not a MaxiGuard_User call.
    ---------------------------------------------------------------------------
    IF (@ItemRefID BETWEEN 42373 AND 42380)
       OR (@ItemRefID BETWEEN 43824 AND 43830)
    BEGIN
        EXEC [SRO_VT_SWITCHER].[dbo].[_AddLogItem]
            @CharID,
            @ItemRefID,
            @Operation,
            @Slot_From,
            @Slot_To;
    END;

    ---------------------------------------------------------------------------
    -- Existing customer scroll procedures. These are kept unchanged.
    ---------------------------------------------------------------------------
    IF (@ItemRefID BETWEEN 41798 AND 41806)
    BEGIN
        EXEC [SRO_VT_LOG].[dbo].[_MarsResetScroll]
            @CharID, @ItemRefID, @Slot_To, @Operation;
    END;

    IF (@ItemRefID BETWEEN 42164 AND 42176)
    BEGIN
        EXEC [SRO_VT_LOG].[dbo].[_MarsMasteryScroll]
            @CharID, @ItemRefID, @Slot_To, @Operation;
    END;

    ---------------------------------------------------------------------------
    -- Title scrolls: grant ownership and immediately activate the title.
    ---------------------------------------------------------------------------
    IF (@ItemRefID BETWEEN 42177 AND 42196
        AND @Operation = 41
        AND @CharName16 IS NOT NULL)
    BEGIN
        DECLARE @TitleID TINYINT = CONVERT(TINYINT, @ItemRefID - 42176);

        EXEC [KMTGuard].[dbo].[Title_Add]
            @CharID = @CharID,
            @TitleID = @TitleID;

        EXEC [KMTGuard].[dbo].[Title_Activate]
            @CharID = @CharID,
            @TitleID = @TitleID;
    END;

    IF (@ItemRefID BETWEEN 43939 AND 43948)
    BEGIN
        EXEC [SRO_VT_LOG].[dbo].[_MarsSilkScroll]
            @CharID, @ItemRefID, @Operation;
    END;

    IF (@Operation = 41 AND @ItemRefID = 42223)
    BEGIN
        EXEC [SRO_VT_LOG].[dbo].[_MarsStartPackScroll]
            @CharID, @ItemRefID, @Operation;
    END;

    IF (@Operation = 41 AND @ItemRefID = 42224)
    BEGIN
        EXEC [SRO_VT_LOG].[dbo].[_MarsStartPackScroll_2]
            @CharID, @ItemRefID, @Operation;
    END;

    IF (@ItemRefID BETWEEN 42219 AND 42222)
    BEGIN
        EXEC [SRO_VT_LOG].[dbo].[_MarsSpecialPackScroll]
            @CharID, @ItemRefID, @Operation;
    END;

    ---------------------------------------------------------------------------
    -- Title color scrolls.
    ---------------------------------------------------------------------------
    IF (@Operation = 41 AND @Slot_To = 255 AND @CharName16 IS NOT NULL)
    BEGIN
        DECLARE @TitleColor VARCHAR(100) =
            CASE @ItemRefID
                WHEN 42197 THEN 'fbff00'
                WHEN 42198 THEN '00bfff'
                WHEN 42199 THEN '44ff00'
                WHEN 42200 THEN 'ff0000'
                WHEN 42201 THEN 'ff7300'
                ELSE NULL
            END;

        IF (@TitleColor IS NOT NULL)
        BEGIN
            EXEC [KMTGuard].[dbo].[TitleColor_Add]
                @CharID = @CharID,
                @ColorCode = @TitleColor,
                @ColorName = @TitleColor;

            DECLARE @TitleColorID INT =
            (
                SELECT TOP (1) ID
                FROM [KMTGuard].[dbo].[PlayerTitleColors]
                WHERE CharID = @CharID AND ColorCode = @TitleColor
                ORDER BY ID
            );

            EXEC [KMTGuard].[dbo].[TitleColor_Activate]
                @CharID = @CharID,
                @CharName16 = @CharName16,
                @ColorID = @TitleColorID;
        END;
    END;

    ---------------------------------------------------------------------------
    -- Custom title server notice (old MaxiGuard BridgeCommands replacement).
    ---------------------------------------------------------------------------
    IF (@ItemRefID = 42196 AND @Operation = 41 AND @CharName16 IS NOT NULL)
    BEGIN
        DECLARE @TitleNotice VARCHAR(MAX) =
            @CharName16 + ' used the special [Custom] title.';

        EXEC [KMTGuard].[dbo].[Command_NoticeAll]
            @NoticeType = 7,
            @Notice = @TitleNotice;
    END;

    ---------------------------------------------------------------------------
    -- Quest-box rewards (old MaxiGuard _AddItemToChest replacement).
    -- Duplicate reward rows are deliberately retained to preserve old behavior.
    ---------------------------------------------------------------------------
    IF (@Operation = 41 AND @Slot_To = 255
        AND @ItemRefID IN (42229, 42230, 42231, 42232, 42233))
    BEGIN
        DECLARE @Rewards TABLE
        (
            RowID        INT IDENTITY(1, 1) PRIMARY KEY,
            ItemCodeName VARCHAR(128) NOT NULL,
            Quantity     INT NOT NULL,
            RewardSource VARCHAR(100) NOT NULL,
            Plus         INT NOT NULL
        );

        IF (@ItemRefID = 42230) -- Medusa Quest Box
        BEGIN
            INSERT INTO @Rewards (ItemCodeName, Quantity, RewardSource, Plus)
            VALUES
                ('ITEM_ETC_SD_TOKEN_04', 1, 'Medusa Box', 0),
                ('ITEM_ETC_SD_TOKEN_03', 2, 'Medusa Box', 0),
                ('ITEM_ETC_ARCHEMY_MAGICSTONE_ATHANASIA_11', 1, 'Medusa Box', 0),
                ('ITEM_ETC_ARENA_COIN', 20, 'Medusa Box', 0);
        END;

        IF (@ItemRefID = 42232) -- Sereness Quest Box
        BEGIN
            INSERT INTO @Rewards (ItemCodeName, Quantity, RewardSource, Plus)
            VALUES
                ('ITEM_ETC_ARCHEMY_MAGICSTONE_ATHANASIA_11', 1, 'Sereness Box', 0),
                ('ITEM_ETC_ARCHEMY_MAGICSTONE_ASTRAL_11', 1, 'Sereness Box', 0);
        END;

        IF (@ItemRefID = 42233) -- Jupiter Quest Box
        BEGIN
            INSERT INTO @Rewards (ItemCodeName, Quantity, RewardSource, Plus)
            VALUES
                ('ITEM_MARS_LUCKY2', 1, 'Jupiter Box', 0),
                ('ITEM_MALL_GACHA_CARD', 1, 'Jupiter Box', 0),
                ('ITEM_MALL_GACHA_CARD', 1, 'Jupiter Box', 0),
                ('ITEM_MALL_GACHA_CARD', 1, 'Jupiter Box', 0),
                ('ITEM_MALL_GACHA_CARD', 1, 'Jupiter Box', 0),
                ('ITEM_MALL_GACHA_CARD', 1, 'Jupiter Box', 0),
                ('ITEM_ETC_ARCHEMY_MAGICSTONE_ATHANASIA_11', 3, 'Jupiter Box', 0);
        END;

        IF (@ItemRefID = 42231) -- Roc Quest Box
        BEGIN
            INSERT INTO @Rewards (ItemCodeName, Quantity, RewardSource, Plus)
            VALUES
                ('ITEM_ETC_SD_TOKEN_04', 2, 'Roc Box', 0),
                ('ITEM_ETC_SD_TOKEN_03', 4, 'Roc Box', 0),
                ('ITEM_ETC_ARCHEMY_MAGICSTONE_ATHANASIA_11', 2, 'Roc Box', 0),
                ('ITEM_ETC_ARENA_COIN', 30, 'Roc Box', 0);
        END;

        IF (@ItemRefID = 42229) -- Quest Box
        BEGIN
            INSERT INTO @Rewards (ItemCodeName, Quantity, RewardSource, Plus)
            VALUES
                ('ITEM_ETC_ARENA_COIN', 10, 'Quest Box', 0),
                ('ITEM_ETC_ARCHEMY_MAGICSTONE_ATHANASIA_11', 5, 'Quest Box', 0),
                ('ITEM_ETC_ALL_SPOTION_01', 100, 'Quest Box', 0),
                ('ITEM_MARS_LUCKY1', 1, 'Quest Box', 0),
                ('ITEM_MALL_GLOBAL_CHATTING', 2, 'Quest Box', 0),
                ('ITEM_MARS_1GUN_BLESS', 1, 'Quest Box', 0),
                ('ITEM_MALL_GACHA_CARD', 1, 'Quest Box', 0);
        END;

        DECLARE @RewardRowID INT = 1;
        DECLARE @RewardRowCount INT = (SELECT COUNT(*) FROM @Rewards);
        DECLARE @RewardCodeName VARCHAR(128);
        DECLARE @RewardQuantity INT;
        DECLARE @RewardSource VARCHAR(100);
        DECLARE @RewardPlus INT;
        DECLARE @RewardRefObjID INT;

        WHILE (@RewardRowID <= @RewardRowCount)
        BEGIN
            SELECT
                @RewardCodeName = R.ItemCodeName,
                @RewardQuantity = R.Quantity,
                @RewardSource = R.RewardSource,
                @RewardPlus = R.Plus
            FROM @Rewards AS R
            WHERE R.RowID = @RewardRowID;

            SET @RewardRefObjID = NULL;

            SELECT TOP (1) @RewardRefObjID = ROC.ID
            FROM [SRO_VT_SHARD].[dbo].[_RefObjCommon] AS ROC WITH (NOLOCK)
            WHERE ROC.CodeName128 = @RewardCodeName
              AND ROC.TypeID1 = 3
            ORDER BY ROC.ID;

            IF (@RewardRefObjID IS NOT NULL)
            BEGIN
                EXEC [KMTGuard].[dbo].[Item_AddChest]
                    @CharID = @CharID,
                    @ItemRefObjID = @RewardRefObjID,
                    @Quantity = @RewardQuantity,
                    @From = @RewardSource,
                    @Plus = @RewardPlus;
            END;

            SET @RewardRowID += 1;
        END;
    END;
END
GO
