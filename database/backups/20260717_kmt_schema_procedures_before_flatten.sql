-- ========================================
-- KMT.Achievement_AddPlayer
-- ========================================

CREATE   PROCEDURE [KMT].[Achievement_AddPlayer]
    @CharID INT
AS
BEGIN
    SET NOCOUNT ON;
    SET XACT_ABORT ON;

    -- ØªØ­Ù‚Ù‚ Ù…Ù† Ø§Ù„Ù€ CharID
    IF (@CharID IS NULL OR @CharID <= 0)
        RETURN -100;

    ---------------------------------------------------------------------
    -- Ø¬Ù…Ø¹ ÙƒÙ„ Ø§Ù„Ù€ RefAchievement + RefConditions Ø§Ù„Ù…ÙØ¹Ù‘Ù„Ø© (Service <> 0)
    ---------------------------------------------------------------------
    DECLARE @AchievementData TABLE
    (
        RefAchievementID INT,
        RefConditionID   INT
    );

    INSERT @AchievementData (RefAchievementID, RefConditionID)
    SELECT RAC.RefAchievementID, RAC.ID
    FROM [KMT].[Achievement_Conditions] AS RAC
    JOIN [KMT].[Achievement_List] AS RA
      ON RA.ID = RAC.RefAchievementID
     AND RA.Service <> 0;

    IF @@ERROR <> 0 
        RETURN -1;

    IF NOT EXISTS (SELECT 1 FROM @AchievementData)
        RETURN 2;

    ---------------------------------------------------------------------
    -- TRY/CATCH Ù…Ø¹ ØªØ±Ø§Ù†Ø²Ø§ÙƒØ´Ù†
    ---------------------------------------------------------------------
    BEGIN TRY
        BEGIN TRAN;

        -- Ø®Ø±ÙŠØ·Ø© Ø¨ÙŠÙ† RefAchievementID ÙˆØ§Ù„Ù€ ID Ø§Ù„ÙØ¹Ù„ÙŠ ÙÙŠ Ø¬Ø¯ÙˆÙ„ [KMT].[Achievement_Players]
        DECLARE @Map TABLE
        (
            RefAchievementID INT PRIMARY KEY,
            AchievementID    INT
        );

        -----------------------------------------------------------------
        -- Ø§Ù„Ù…ÙˆØ¬ÙˆØ¯ÙŠÙ† Ø£ØµÙ„Ù‹Ø§ Ù„Ù„Ø´Ø®ØµÙŠØ©
        -----------------------------------------------------------------
        INSERT INTO @Map (RefAchievementID, AchievementID)
        SELECT A.RefAchievementID, A.ID
        FROM [KMT].[Achievement_Players] AS A WITH (UPDLOCK, HOLDLOCK)
        JOIN @AchievementData AS AD
          ON AD.RefAchievementID = A.RefAchievementID
        WHERE A.CharID = @CharID;

        -----------------------------------------------------------------
        -- Ø¥Ø¯Ø®Ø§Ù„ Ø§Ù„Ù†Ø§Ù‚Øµ ÙÙŠ [KMT].[Achievement_Players] + ØªØ¬Ù…ÙŠØ¹ IDs Ø§Ù„Ù…ÙØ¶Ø§ÙØ©
        -----------------------------------------------------------------
        INSERT [KMT].[Achievement_Players] (CharID, RefAchievementID, State)
        OUTPUT inserted.RefAchievementID, inserted.ID 
            INTO @Map (RefAchievementID, AchievementID)
        SELECT @CharID, AD.RefAchievementID, 0
        FROM @AchievementData AS AD
        WHERE NOT EXISTS (
            SELECT 1
            FROM [KMT].[Achievement_Players] AS A
            WHERE A.CharID = @CharID
              AND A.RefAchievementID = AD.RefAchievementID
        )
        GROUP BY AD.RefAchievementID;

        -----------------------------------------------------------------
        -- Ø¥Ø¯Ø®Ø§Ù„ Ø´Ø±ÙˆØ· Ø§Ù„Ø¥Ù†Ø¬Ø§Ø² Ù…Ø¹ Ø§Ù„Ù€ AchievementID Ø§Ù„ØµØ­ÙŠØ­
        -----------------------------------------------------------------
        INSERT [KMT].[Achievement_PlayerConditions]
              (CharID, AchievementID, RefAchievementConditionID, ProgressCount)
        SELECT @CharID, M.AchievementID, AD.RefConditionID, 0
        FROM @AchievementData AS AD
        JOIN @Map AS M
          ON M.RefAchievementID = AD.RefAchievementID
        WHERE NOT EXISTS (
            SELECT 1
            FROM [KMT].[Achievement_PlayerConditions] AS AC
            WHERE AC.CharID = @CharID
              AND AC.RefAchievementConditionID = AD.RefConditionID
        );

        COMMIT TRAN;
        SET NOCOUNT OFF;
        RETURN 1;
    END TRY
    BEGIN CATCH
        IF XACT_STATE() <> 0
            ROLLBACK TRAN;

        SET NOCOUNT OFF;
        RETURN -9;
    END CATCH
END

GO

-- ========================================
-- KMT.Achievement_UniqueKill
-- ========================================

CREATE   PROCEDURE [KMT].[Achievement_UniqueKill]
    @RefObjID    INT,
    @KillerrName VARCHAR(16)
AS
BEGIN
    SET NOCOUNT ON;

    ----------------------------------------------------
    -- 1) ØªÙ†Ø¸ÙŠÙ Ø§Ù„Ø§Ø³Ù… + ØªØ­Ù‚Ù‚ Ù…Ø¨Ø¯Ø¦ÙŠ Ù…Ù† Ø§Ù„Ø¨Ø±Ø§Ù…ÙŠØªØ±Ø²
    ----------------------------------------------------
    SET @KillerrName = LTRIM(RTRIM(@KillerrName));

    IF (@RefObjID IS NULL OR @KillerrName IS NULL OR @KillerrName = '')
        RETURN;

    ----------------------------------------------------
    -- 2) Ø¬Ù„Ø¨ CharID Ù…Ù† Ø§Ù„Ø´Ø§Ø±Ø¯ Ø¨Ø§Ù„Ø§Ø³Ù… (Ø´Ø®ØµÙŠØ© Ø§Ù„Ù‚Ø§ØªÙ„)
    --    TOP(1) Ù…Ø¹ ORDER BY Ù„ØªØ¬Ù†Ø¨ Ø§Ø®ØªÙŠØ§Ø± Ø¹Ø´ÙˆØ§Ø¦ÙŠ Ù„Ùˆ ÙÙŠÙ‡
    --    Ø£ÙƒØ«Ø± Ù…Ù† Ø´Ø®ØµÙŠØ© Ø¨Ù†ÙØ³ Ø§Ù„Ø§Ø³Ù… Ù„Ø£ÙŠ Ø³Ø¨Ø¨
    ----------------------------------------------------
    DECLARE @CharID INT;

    SELECT TOP (1) 
           @CharID = C.CharID
    FROM   SRO_VT_SHARD.._Char AS C WITH (NOLOCK)
    WHERE  C.CharName16 COLLATE DATABASE_DEFAULT = @KillerrName COLLATE DATABASE_DEFAULT
    ORDER BY C.CharID DESC;   -- Ø¢Ø®Ø± CharID (ØªÙ‚Ø¯ÙŠØ±ÙŠØ§Ù‹ Ø£Ø­Ø¯Ø« ÙˆØ§Ø­Ø¯Ø©)

    IF (@CharID IS NULL)
        RETURN;  -- Ø§Ø³Ù… Ø§Ù„Ø´Ø®ØµÙŠØ© Ù…Ø´ Ù…ÙˆØ¬ÙˆØ¯

    ----------------------------------------------------
    -- 3) Ù†Ù‚Ø·Ø© Ø«Ø§Ø¨ØªØ© Ù„ÙƒÙ„ Ù‚ØªÙ„Ø© (ØªÙ‚Ø¯Ø± ØªØ¹Ø¯Ù„Ù‡Ø§ Ù„Ø§Ø­Ù‚Ø§Ù‹ Ù„Ùˆ Ø­Ø¨ÙŠØª)
    ----------------------------------------------------
    DECLARE @PointGive INT = 1;

    ----------------------------------------------------
    -- 4) ØªØ¬Ù‡ÙŠØ² Ø¬Ø¯ÙˆÙ„ Ù…Ø¤Ù‚Øª ÙŠØ±Ø¨Ø· RefObjID Ø¨Ø§Ù„Ù€ ConditionID
    --    Ø¨Ø­ÙŠØ«:
    --    - Ù†Ù‚Ø±Ø£ IDs Ù…Ø±Ø© ÙˆØ§Ø­Ø¯Ø© Ù…Ù† _RefObjCommon
    --    - Ù†Ø¯Ø¹Ù… Ø£ÙƒØªØ± Ù…Ù† CodeName Ù„Ù†ÙØ³ Ø§Ù„Ù€ Unique
    --    - Ù†Ø·Ù„Ù‘Ø¹ ConditionID Ø¹Ù„Ù‰ Ø­Ø³Ø¨ @RefObjID
    ----------------------------------------------------
    DECLARE @Uniques TABLE
    (
        RefObjID     INT PRIMARY KEY,
        ConditionID  INT
    );

    -- Tiger Girl
    INSERT INTO @Uniques (RefObjID, ConditionID)
    SELECT ID, 1
    FROM   SRO_VT_SHARD.._RefObjCommon WITH (NOLOCK)
    WHERE  CodeName128 COLLATE DATABASE_DEFAULT = 'MOB_CH_TIGERWOMAN';

    -- Cerberus
    INSERT INTO @Uniques (RefObjID, ConditionID)
    SELECT ID, 2
    FROM   SRO_VT_SHARD.._RefObjCommon WITH (NOLOCK)
    WHERE  CodeName128 COLLATE DATABASE_DEFAULT = 'MOB_EU_CERBERUS';

    -- Captain Ivy (Ø£ÙƒØªØ± Ù…Ù† CodeName Ù„Ù†ÙØ³ Ø§Ù„Ù€ Unique)
    INSERT INTO @Uniques (RefObjID, ConditionID)
    SELECT ID, 3
    FROM   SRO_VT_SHARD.._RefObjCommon WITH (NOLOCK)
    WHERE  CodeName128 COLLATE DATABASE_DEFAULT IN ('MOB_AM_IVY','MOB_CH_IVY');

    -- Uruchi
    INSERT INTO @Uniques (RefObjID, ConditionID)
    SELECT ID, 4
    FROM   SRO_VT_SHARD.._RefObjCommon WITH (NOLOCK)
    WHERE  CodeName128 COLLATE DATABASE_DEFAULT = 'MOB_OA_URUCHI';

    -- Isyutaru
    INSERT INTO @Uniques (RefObjID, ConditionID)
    SELECT ID, 5
    FROM   SRO_VT_SHARD.._RefObjCommon WITH (NOLOCK)
    WHERE  CodeName128 COLLATE DATABASE_DEFAULT IN ('MOB_OA_ISYUTARU','MOB_KK_ISYUTARU');

    -- Lord Yarkan
    INSERT INTO @Uniques (RefObjID, ConditionID)
    SELECT ID, 6
    FROM   SRO_VT_SHARD.._RefObjCommon WITH (NOLOCK)
    WHERE  CodeName128 COLLATE DATABASE_DEFAULT IN ('MOB_KK_LORD_YARKAN','MOB_TK_BONELORD');

    -- Demon Shaitan
    INSERT INTO @Uniques (RefObjID, ConditionID)
    SELECT ID, 7
    FROM   SRO_VT_SHARD.._RefObjCommon WITH (NOLOCK)
    WHERE  CodeName128 COLLATE DATABASE_DEFAULT IN ('MOB_TQ_SHITAN','MOB_RM_TAHOMET');

    ----------------------------------------------------
    -- 5) ØªØ­Ø¯ÙŠØ¯ Ø§Ù„Ù€ ConditionID Ø¨Ù†Ø§Ø¡Ù‹ Ø¹Ù„Ù‰ @RefObjID
    ----------------------------------------------------
    DECLARE @ConditionID INT;

    SELECT @ConditionID = U.ConditionID
    FROM   @Uniques AS U
    WHERE  U.RefObjID = @RefObjID;

    -- Ù„Ùˆ Ø§Ù„Ù€ RefObjID Ù…Ø´ Ù…Ù† Ø¶Ù…Ù† Ø§Ù„Ù€ uniques Ø§Ù„Ù„ÙŠ Ø­Ø§Ø·Ø·Ù‡Ø§ØŒ Ù…ÙÙŠØ´ Ø¥Ù†Ø¬Ø§Ø² ÙŠØªØ­Ø¯Ø«
    IF (@ConditionID IS NULL)
        RETURN;

    ----------------------------------------------------
    -- 6) ØªØ­Ø¯ÙŠØ« Ø§Ù„Ø¥Ù†Ø¬Ø§Ø² Ù„Ù„Ø´Ø®ØµÙŠØ© Ø§Ù„Ù„ÙŠ Ù‚ØªÙ„Øª ÙÙ‚Ø·
    --    NOTE: ØªØ£ÙƒØ¯ Ø¥Ù† dbo._UpdateAchievement Ù…Ø¨Ù†ÙŠ Ø¹Ù„Ù‰ CharID
    --          Ù…Ø´ Ø¹Ù„Ù‰ JID Ù„Ùˆ Ø¹Ø§ÙŠØ² Ø§Ù„Ø¥Ù†Ø¬Ø§Ø²Ø§Øª ØªÙƒÙˆÙ† per-char
    ----------------------------------------------------
    EXEC [KMT].[Achievement_Update] 
        @CharID,          -- CharID Ø§Ù„Ø®Ø§Øµ Ø¨Ø§Ù„Ø´Ø®ØµÙŠØ© Ø§Ù„Ù‚Ø§ØªÙ„Ø©
        @ConditionID,     -- AchievementCondition (Ù†ÙØ³Ù‡ min/max)
        @ConditionID,     -- AchievementCondition
        @PointGive;       -- Ø§Ù„Ù†Ù‚Ø§Ø· Ø§Ù„Ù…Ø¹Ø·Ø§Ø©

    ----------------------------------------------------
    -- 7) Ø¥Ø¯Ø®Ø§Ù„ Ø£Ù…Ø± ÙÙŠ Ø¬Ø¯ÙˆÙ„ Ø§Ù„Ù€ AsyncFilterCommands
    --    Ù„Ùˆ Ø¹Ù†Ø¯Ùƒ ØªØµÙ…ÙŠÙ… Ù…Ø¹ÙŠÙ‘Ù† Ù„Ø¨ÙŠØ§Ù†Ø§Øª Data1/Data2 ØºÙŠØ± Ø«Ø§Ø¨ØªØ©
    --    ØªÙ‚Ø¯Ø± ØªØ³ØªØ®Ø¯Ù… @CharID Ø£Ùˆ @ConditionID Ø¨Ø¯Ù„ Ø§Ù„Ù€ 40 Ø§Ù„Ø«Ø§Ø¨ØªØ©
    ----------------------------------------------------
    INSERT INTO [KMT].[Command_FilterQueue] (CommandID, Data1, Data2, Status)
    VALUES (40, 40, 40, 1);
    -- Ù…Ø«Ø§Ù„ Ø¨Ø¯ÙŠÙ„ Ù„Ùˆ Ø­Ø¨ÙŠØª ØªØ³ØªØ®Ø¯Ù… Ù…Ø¹Ù„ÙˆÙ…Ø§Øª Ø­Ù‚ÙŠÙ‚ÙŠØ©:
    -- VALUES (40, @CharID, @ConditionID, 1);

END

GO

-- ========================================
-- KMT.Achievement_UniqueKillByID
-- ========================================

CREATE   PROCEDURE [KMT].[Achievement_UniqueKillByID]
    @RefObjID        INT,
    @KillerCharName  VARCHAR(16)
AS
BEGIN
    SET NOCOUNT ON;

    -- Ø³ÙŠØ§Ù‚ Ø£Ø³Ø§Ø³ÙŠ
    DECLARE @CharID INT = (
        SELECT CharID 
        FROM SRO_VT_SHARD.._Char 
        WHERE CharName16 = @KillerCharName
    );
    IF @CharID IS NULL RETURN;

    ---------------------------------------------------------
    -- âœ… T i g e r  G i r l  (Ø¥Ù†Ø¬Ø§Ø² ÙÙ‚Ø·)
    ---------------------------------------------------------
    DECLARE @UniqueTigerGirl INT = (
        SELECT ID 
        FROM SRO_VT_SHARD.._RefObjCommon 
        WHERE CodeName128 LIKE 'MOB_CH_TIGERWOMAN'
    );
    DECLARE @TigerAchievementCondition INT = 1;  
    DECLARE @AchievementKillPoint_Tiger INT = 1;          

    IF (@RefObjID = @UniqueTigerGirl)
    BEGIN
        EXEC [KMT].[Achievement_Update] 
            @CharID,
            @TigerAchievementCondition,
            @TigerAchievementCondition,
            @AchievementKillPoint_Tiger;

        INSERT INTO [KMT].[Command_FilterQueue](CommandID, Data1, Data2, Status)
        VALUES (40, 40, 40, 1);
    END

    ---------------------------------------------------------
    -- âœ… U r u c h i  (Ø¥Ù†Ø¬Ø§Ø² ÙÙ‚Ø·)
    ---------------------------------------------------------
    DECLARE @UniqueUruchi INT = (
        SELECT ID 
        FROM SRO_VT_SHARD.._RefObjCommon 
        WHERE CodeName128 LIKE 'MOB_OA_URUCHI'
    );
    DECLARE @UruchiAchievementCondition INT = 4;
    DECLARE @AchievementKillPoint_Uruchi INT = 1;

    IF (@RefObjID = @UniqueUruchi)
    BEGIN
        EXEC [KMT].[Achievement_Update] 
            @CharID,
            @UruchiAchievementCondition,
            @UruchiAchievementCondition,
            @AchievementKillPoint_Uruchi;

        INSERT INTO [KMT].[Command_FilterQueue](CommandID, Data1, Data2, Status)
        VALUES (40, 40, 40, 1);
    END

    ---------------------------------------------------------
    -- âœ… I s y u t a r u  (Ø¥Ù†Ø¬Ø§Ø² ÙÙ‚Ø·)
    ---------------------------------------------------------
    DECLARE @UniqueIsyutaru INT = (
        SELECT ID 
        FROM SRO_VT_SHARD.._RefObjCommon 
        WHERE CodeName128 LIKE 'MOB_KK_ISYUTARU'
    );
    DECLARE @IsyutaruAchievementCondition INT = 5;
    DECLARE @AchievementKillPoint_Isyutaru INT = 5;  -- Ø²ÙŠ Ù…Ø§ ÙƒÙ†Øª ÙƒØ§ØªØ¨

    IF (@RefObjID = @UniqueIsyutaru)
    BEGIN
        EXEC [KMT].[Achievement_Update] 
            @CharID,
            @IsyutaruAchievementCondition,
            @IsyutaruAchievementCondition,
            @AchievementKillPoint_Isyutaru;

        INSERT INTO [KMT].[Command_FilterQueue](CommandID, Data1, Data2, Status)
        VALUES (40, 40, 40, 1);
    END
END

GO

-- ========================================
-- KMT.Achievement_Update
-- ========================================

CREATE   PROCEDURE [KMT].[Achievement_Update]
    @CharID                     INT,
    @RefAchievementID           INT,
    @RefAchievementConditionID  INT,
    @Progress                   INT
AS
BEGIN
    SET NOCOUNT ON;

    DECLARE @RefAchievementRewardType INT;
    DECLARE @CompleteCount           INT;
    DECLARE @CurrentCompleteCount    INT;
    DECLARE @AchievementState        INT;
    DECLARE @NewProgress             INT;

    ---------------------------------------------------------------------
    -- Ù„Ùˆ Ù…ÙÙŠØ´ Achievement Ù„Ù„ÙƒØ§Ø±ÙƒØªØ± Ø¯Ù‡ â€“ Ù†Ø·Ù„Ø¹ Ù…Ø¨Ø§Ø´Ø±Ø©
    ---------------------------------------------------------------------
    IF NOT EXISTS (
        SELECT 1 
        FROM [KMT].[Achievement_Players] 
        WHERE CharID = @CharID 
          AND RefAchievementID = @RefAchievementID
    )
        RETURN;

    ---------------------------------------------------------------------
    -- Ù„Ùˆ Ù…ÙÙŠØ´ Condition Ù„Ù„ÙƒØ§Ø±ÙƒØªØ± Ø¯Ù‡ â€“ Ù†Ø·Ù„Ø¹ Ù…Ø¨Ø§Ø´Ø±Ø©
    ---------------------------------------------------------------------
    IF NOT EXISTS (
        SELECT 1 
        FROM [KMT].[Achievement_PlayerConditions] 
        WHERE CharID = @CharID 
          AND RefAchievementConditionID = @RefAchievementConditionID
    )
        RETURN;

    ---------------------------------------------------------------------
    -- Ù‚Ø±Ø§Ø¡Ø§Øª Ø£Ø³Ø§Ø³ÙŠØ© Ù…Ù† Ø¬Ø¯Ø§ÙˆÙ„ Ø§Ù„Ù€ Ref ÙˆØ§Ù„Ø¬Ø¯Ø§ÙˆÙ„ Ø§Ù„Ø­Ù‚ÙŠÙ‚ÙŠØ©
    ---------------------------------------------------------------------
    SELECT 
        @RefAchievementRewardType = RewardType
    FROM [KMT].[Achievement_List]
    WHERE ID = @RefAchievementID;

    SELECT 
        @CompleteCount = CompleteCount
    FROM [KMT].[Achievement_Conditions]
    WHERE ID = @RefAchievementConditionID;

    SELECT 
        @CurrentCompleteCount = ProgressCount
    FROM [KMT].[Achievement_PlayerConditions]
    WHERE CharID = @CharID
      AND RefAchievementConditionID = @RefAchievementConditionID;

    SELECT 
        @AchievementState = State
    FROM [KMT].[Achievement_Players]
    WHERE CharID = @CharID
      AND RefAchievementID = @RefAchievementID;

    ---------------------------------------------------------------------
    -- Ø­Ù…Ø§ÙŠØ© Ø¨Ø³ÙŠØ·Ø© Ù„Ùˆ Ø§Ù„Ù‚ÙŠÙ… NULL Ø£Ùˆ Ø§Ù„Ù€ Progress <= 0
    ---------------------------------------------------------------------
    SET @CurrentCompleteCount = ISNULL(@CurrentCompleteCount, 0);
    SET @CompleteCount        = ISNULL(@CompleteCount, 0);
    SET @Progress             = ISNULL(@Progress, 0);

    IF (@Progress <= 0 OR @CompleteCount <= 0)
        RETURN;

    ---------------------------------------------------------------------
    -- Ø­Ø³Ø§Ø¨ Ø§Ù„ØªÙ‚Ø¯Ù… Ø§Ù„Ø¬Ø¯ÙŠØ¯ Ù…Ø¹ Ø§Ù„ÙƒØ§Ø¨ Ø¹Ù†Ø¯ CompleteCount
    ---------------------------------------------------------------------
    SET @NewProgress = @CurrentCompleteCount + @Progress;

    IF (@NewProgress > @CompleteCount)
        SET @NewProgress = @CompleteCount;

    ---------------------------------------------------------------------
    -- ØªØ­Ø¯ÙŠØ« Ø§Ù„Ù€ Condition
    ---------------------------------------------------------------------
    UPDATE [KMT].[Achievement_PlayerConditions]
    SET ProgressCount = @NewProgress
    WHERE CharID = @CharID
      AND RefAchievementConditionID = @RefAchievementConditionID;

    ---------------------------------------------------------------------
    -- Ù‡Ù„ ÙƒÙ„ Ø§Ù„Ù€ Conditions Ø§Ù„Ø®Ø§ØµØ© Ø¨Ø§Ù„Ù€ Achievement Ø§ÙƒØªÙ…Ù„ØªØŸ
    ---------------------------------------------------------------------
    IF NOT EXISTS (
        SELECT 1
        FROM [KMT].[Achievement_PlayerConditions] AC
        JOIN [KMT].[Achievement_Conditions] RAC 
             ON AC.RefAchievementConditionID = RAC.ID
        WHERE AC.CharID = @CharID
          AND RAC.RefAchievementID = @RefAchievementID
          AND AC.ProgressCount < RAC.CompleteCount
    )
    BEGIN
        -- ÙƒÙ„ Ø§Ù„Ø´Ø±ÙˆØ· Ø®Ù„ØµØª
        IF (@AchievementState = 0)
        BEGIN
            IF (@RefAchievementRewardType = 0)
            BEGIN
                -- Ø­Ø§Ù„Ø© 1: Achievement Ø¨Ø¯ÙˆÙ† Reward Ø£Ùˆ Ù†ÙˆØ¹ 0
                UPDATE [KMT].[Achievement_Players]
                SET State = 1
                WHERE CharID = @CharID
                  AND RefAchievementID = @RefAchievementID;

                EXEC [KMT].[Command_NoticeByID] @CharID, 8, 'Achievement Completed.';

                INSERT INTO [KMT].[Command_FilterQueue]
                    (CommandID, Data1, Data2, Data3, Data4, Data5, Status)
                VALUES
                    (29, @CharID, @RefAchievementID, @RefAchievementConditionID, @NewProgress, 1, 1);
            END
            ELSE
            BEGIN
                -- Ø­Ø§Ù„Ø© 2: Achievement Ù„Ù‡ Ù†ÙˆØ¹ Reward Ù…Ø®ØªÙ„Ù
                UPDATE [KMT].[Achievement_Players]
                SET State = 2
                WHERE CharID = @CharID
                  AND RefAchievementID = @RefAchievementID;

                EXEC [KMT].[Command_NoticeByID] @CharID, 8, 'Achievement Completed.';

                INSERT INTO [KMT].[Command_FilterQueue]
                    (CommandID, Data1, Data2, Data3, Data4, Data5, Status)
                VALUES
                    (29, @CharID, @RefAchievementID, @RefAchievementConditionID, @NewProgress, 2, 1);
            END
        END
        -- Ù„Ùˆ AchievementState Ù…Ø´ 0 (ÙŠØ¹Ù†ÙŠ Ù…ØªÙƒÙ…Ù„ Ù‚Ø¨Ù„ ÙƒØ¯Ù‡)ØŒ Ù…Ø´ Ù‡Ù†ØºÙŠØ± Ø­Ø§Ø¬Ø© ØªØ§Ù†ÙŠ
    END
    ELSE
    BEGIN
        -----------------------------------------------------------------
        -- Ù„Ø³Ù‡ ÙÙŠ Conditions Ù†Ø§Ù‚ØµØ© â€“ Ù†Ø®Ù„ÙŠ Ø§Ù„Ù€ Achievement State = 0
        -----------------------------------------------------------------
        UPDATE [KMT].[Achievement_Players]
        SET State = 0
        WHERE CharID = @CharID
          AND RefAchievementID = @RefAchievementID;

        INSERT INTO [KMT].[Command_FilterQueue]
            (CommandID, Data1, Data2, Data3, Data4, Data5, Status)
        VALUES
            (29, @CharID, @RefAchievementID, @RefAchievementConditionID, @NewProgress, 0, 1);
    END

    SET NOCOUNT OFF;
END

GO

-- ========================================
-- KMT.Auth_Login
-- ========================================
-- =============================================
-- Author:		<Author,,Name>
-- Create date: <Create Date,,>
-- Description:	<Description,,>
-- =============================================
CREATE   PROCEDURE [KMT].[Auth_Login]
    -- Add the parameters for the stored procedure here
    @UserName VARCHAR(32),
    @Password varchar(32)
AS
BEGIN
    -- SET NOCOUNT ON added to prevent extra result sets from
    -- interfering with SELECT statements.
    SET NOCOUNT ON;
    
    IF EXISTS (SELECT 1 FROM SRO_VT_ACCOUNT..TB_User (nolock) WHERE StrUserID = @UserName)
    BEGIN
        DECLARE @StoredPasswordHash VARCHAR(128) = (SELECT password FROM SRO_VT_ACCOUNT..TB_User WITH (nolock) WHERE StrUserID = @UserName);
        DECLARE @InputPasswordHash VARCHAR(128) = CONVERT(VARCHAR(128), HASHBYTES('MD5', @Password), 2);
        
        IF @StoredPasswordHash = @InputPasswordHash
        BEGIN
            SELECT 1; -- Return 1 for success
        END
        ELSE
        BEGIN
            SELECT 0; -- Return 0 for failure
        END
    END
    ELSE
    BEGIN
        SELECT 0; -- Return 0 if user does not exist
    END
END

GO

-- ========================================
-- KMT.Auth_UpdateHWID
-- ========================================
CREATE   PROCEDURE [KMT].[Auth_UpdateHWID]
    @Active tinyint,
    @CharID int,
    @CharName16 varchar(16),
    @IP varchar(100),
    @Hwid nvarchar(max),
    @JobStatus tinyint,
    @LatestRegionId int,
    @LatestWorldId int,
    @CurLevel int
AS
BEGIN
    -- Ekstra sonuÃ§ kÃ¼melerinin SELECT ifadelerini etkilemesini Ã¶nlemek iÃ§in NOCOUNT ON kullanÄ±ldÄ±.
    SET NOCOUNT ON;

    -- Benzersiz web token oluÅŸtur
    DECLARE @WebToken varchar(64) = LOWER(CONVERT(varchar(64), NEWID()));

    -- WebToken Ã§akÄ±ÅŸmasÄ±nÄ± Ã¶nlemek iÃ§in kontrol ekleyelim
    WHILE EXISTS (SELECT 1 FROM [KMT].[Auth_HWIDs] with (nolock) WHERE webtoken = @WebToken)
    BEGIN
        SET @WebToken = LOWER(CONVERT(varchar(64), NEWID()));
    END

    IF EXISTS (SELECT 1 FROM [KMT].[Auth_HWIDs] WHERE CharID = @CharID)
    BEGIN
        UPDATE [KMT].[Auth_HWIDs] 
        SET Active = @Active, 
            CharName16 = @CharName16, 
            [IP] = @IP, 
            Hwid = @Hwid, 
            JobStatus = @JobStatus, 
            LatestRegionId = @LatestRegionId, 
            LatestWorldId = @LatestWorldId, 
            CurLevel = @CurLevel,
            WebToken = @WebToken -- GÃ¼ncelleme sÄ±rasÄ±nda yeni benzersiz WebToken oluÅŸtur
        WHERE CharID = @CharID;
    END
    ELSE
    BEGIN
        INSERT INTO [KMT].[Auth_HWIDs] (Active, CharID, CharName16, [IP], Hwid, JobStatus, LatestRegionId, LatestWorldId, CurLevel, WebToken) 
        VALUES (@Active, @CharID, @CharName16, @IP, @Hwid, @JobStatus, @LatestRegionId, @LatestWorldId, @CurLevel, @WebToken);
    END
END
GO

-- ========================================
-- KMT.Command_ChangeName
-- ========================================
CREATE   PROCEDURE [KMT].[Command_ChangeName]
    @CharID INT,
    @GrantName VARCHAR(16)
AS
BEGIN
    INSERT INTO [KMT].[Command_GameServerQueue](Action_ID, Data1, Data2) VALUES(1, @CharID, @GrantName)
END

GO

-- ========================================
-- KMT.Command_DisconnectAll
-- ========================================
CREATE   PROCEDURE [KMT].[Command_DisconnectAll]

AS		

INSERT INTO [KMT].[Command_FilterQueue](CommandID, Status) VALUES(2, 1)


GO

-- ========================================
-- KMT.Command_DisconnectByID
-- ========================================
CREATE   PROCEDURE [KMT].[Command_DisconnectByID]
	@CharID int
AS		

INSERT INTO [KMT].[Command_FilterQueue](CommandID, Data1, Status) VALUES(34, @CharID, 1)


GO

-- ========================================
-- KMT.Command_DisconnectByName
-- ========================================
CREATE   PROCEDURE [KMT].[Command_DisconnectByName]
	@CharName16 varchar(16)
AS		

INSERT INTO [KMT].[Command_FilterQueue](CommandID, Data1, Status) VALUES(1, @CharName16, 1)


GO

-- ========================================
-- KMT.Command_GetUp
-- ========================================
CREATE   PROCEDURE [KMT].[Command_GetUp]
		@CharID int
AS		

INSERT INTO [KMT].[Command_GameServerQueue](Action_ID, Data1) VALUES(15, @CharID)

GO

-- ========================================
-- KMT.Command_GetUpAtPosition
-- ========================================

CREATE   PROCEDURE [KMT].[Command_GetUpAtPosition]
	@CHARID int,
	@GameWorldID int,
	@RegionId int,
	@PosX int,
	@PosY int,
	@PosZ int
AS		

INSERT INTO [KMT].[Command_GameServerQueue](Action_ID, Data1, Data2, Data3, Data4, Data5, Data6) VALUES(23, @CHARID, @GameWorldID, @RegionId, @PosX, @PosY, @PosZ)



GO

-- ========================================
-- KMT.Command_MapPingByID
-- ========================================
CREATE   PROCEDURE [KMT].[Command_MapPingByID]
	@CharID int,
	@RegionID int,
	@PosX int,
	@PosZ int,
	@PosY int,
	@Seconds int
AS		

INSERT INTO [KMT].[Command_FilterQueue](CommandID, Data1, Data2, Data3,Data4,Data5, Data6, Status) VALUES(36, @CharID, @RegionID, @PosX, @PosY, @PosZ, @Seconds, 1)


GO

-- ========================================
-- KMT.Command_NoticeAll
-- ========================================
CREATE   PROCEDURE [KMT].[Command_NoticeAll]
	@NoticeType int,
	@Notice varchar(max)
AS		

INSERT INTO [KMT].[Command_FilterQueue](CommandID, Data1, Data2, Data3,Status) VALUES(3, 'sendall', @NoticeType, @Notice, 1)


GO

-- ========================================
-- KMT.Command_NoticeByID
-- ========================================
CREATE   PROCEDURE [KMT].[Command_NoticeByID]
	@CharID int,
	@NoticeType int,
	@Notice varchar(max)
AS		

INSERT INTO [KMT].[Command_FilterQueue](CommandID, Data1, Data2, Data3, Status) VALUES(28, @CharID, @Notice, @NoticeType, 1)


GO

-- ========================================
-- KMT.Command_NoticeByName
-- ========================================
CREATE   PROCEDURE [KMT].[Command_NoticeByName]
	@CharName16 varchar(16),
	@NoticeType int,
	@Notice varchar(max)
AS		

INSERT INTO [KMT].[Command_FilterQueue](CommandID, Data1, Data2, Data3, Data4, Status) VALUES(3, 'sendchar', @NoticeType, @Notice, @CharName16, 1)


GO

-- ========================================
-- KMT.Event_AddJobKill
-- ========================================

CREATE   PROCEDURE [KMT].[Event_AddJobKill]
@WorldID int,
@CharName16 varchar(12),
@Kill int,
@Job int
AS

INSERT INTO [KMT].[Command_FilterQueue](CommandID, Data1, Data2, Data3, Data4, Status) VALUES(33, @WorldID, @CharName16, @Kill, @Job, 1)


	--- 2 = thief 1-3 hunter trader
GO

-- ========================================
-- KMT.Event_AddJobKillCounter
-- ========================================

CREATE   PROCEDURE [KMT].[Event_AddJobKillCounter]
@WorldID int
AS

INSERT INTO [KMT].[Command_FilterQueue](CommandID, Data1, Status) VALUES(32, @WorldID,  1)


	/* Bu prosedÃ¼r ile kill counter oluÅŸturduÄŸunuz bÃ¶lgeler iÃ§in _IncreaseKillCounter komutunu kullanabilirsiniz. Bu prosedÃ¼rÃ¼ aynÄ± bÃ¶lge iÃ§in ikinci kez Ã§aÄŸÄ±rÄ±rsanÄ±z kill counter sÄ±fÄ±rlanÄ±r. */
GO

-- ========================================
-- KMT.Event_AddKill
-- ========================================

CREATE   PROCEDURE [KMT].[Event_AddKill]
@WorldID int,
@CharName16 varchar(12),
@Kill int
AS

INSERT INTO [KMT].[Command_FilterQueue](CommandID, Data1, Data2, Data3, Status) VALUES(26, @WorldID, @CharName16, @Kill, 1)


	/* Bu prosedÃ¼r ile _MakeNewKillCounterbyRegionID prosedÃ¼rÃ¼ ile oluÅŸturduÄŸunuz countera ekleme yapabilirsiniz. */
GO

-- ========================================
-- KMT.Event_AddKillCounter
-- ========================================

CREATE   PROCEDURE [KMT].[Event_AddKillCounter]
@Title varchar(50),
@WorldID int
AS

INSERT INTO [KMT].[Command_FilterQueue](CommandID, Data1, Data2, Status) VALUES(25, @Title, @WorldID,  1)


	/* Bu prosedÃ¼r ile kill counter oluÅŸturduÄŸunuz bÃ¶lgeler iÃ§in _IncreaseKillCounter komutunu kullanabilirsiniz. Bu prosedÃ¼rÃ¼ aynÄ± bÃ¶lge iÃ§in ikinci kez Ã§aÄŸÄ±rÄ±rsanÄ±z kill counter sÄ±fÄ±rlanÄ±r. */
GO

-- ========================================
-- KMT.Event_AddRegionTimer
-- ========================================

CREATE   PROCEDURE [KMT].[Event_AddRegionTimer]
@Seconds int,
@RegionID int
AS

	INSERT INTO [KMT].[Command_FilterQueue](CommandID, Data1, Data2, Status) VALUES(24, @Seconds, @RegionID,  1)


	/* Bu prosedÃ¼r ile timer oluÅŸturduÄŸunuz takdirde sÃ¼re bitene kadar aynÄ± region'a teleport olduÄŸunda timer kaldÄ±ÄŸÄ± yerden devam edecektir. */
GO

-- ========================================
-- KMT.Event_AddTeamKill
-- ========================================

CREATE   PROCEDURE [KMT].[Event_AddTeamKill]
@WorldID int,
@CharName16 varchar(12),
@Kill int,
@Team int
AS

INSERT INTO [KMT].[Command_FilterQueue](CommandID, Data1, Data2, Data3, Data4, Status) VALUES(31, @WorldID, @CharName16, @Kill, @Team, 1)


	/* Bu prosedÃ¼r ile _MakeNewTeamKillCounterbyWorldID prosedÃ¼rÃ¼ ile oluÅŸturduÄŸunuz countera ekleme yapabilirsiniz. */
GO

-- ========================================
-- KMT.Event_AddTeamKillCounter
-- ========================================

CREATE   PROCEDURE [KMT].[Event_AddTeamKillCounter]
@WorldID int
AS

INSERT INTO [KMT].[Command_FilterQueue](CommandID, Data1, Status) VALUES(30, @WorldID,  1)


	/* Bu prosedÃ¼r ile kill counter oluÅŸturduÄŸunuz bÃ¶lgeler iÃ§in _IncreaseKillCounter komutunu kullanabilirsiniz. Bu prosedÃ¼rÃ¼ aynÄ± bÃ¶lge iÃ§in ikinci kez Ã§aÄŸÄ±rÄ±rsanÄ±z kill counter sÄ±fÄ±rlanÄ±r. */
GO

-- ========================================
-- KMT.Event_AddWorldTimer
-- ========================================

CREATE   PROCEDURE [KMT].[Event_AddWorldTimer]
@Seconds int,
@WorldID int
AS

	INSERT INTO [KMT].[Command_FilterQueue](CommandID, Data1, Data2, Status) VALUES(23, @Seconds, @WorldID,  1)


	/* Bu prosedÃ¼r ile timer oluÅŸturduÄŸunuz takdirde sÃ¼re bitene kadar aynÄ± world id'e teleport olduÄŸunda timer kaldÄ±ÄŸÄ± yerden devam edecektir. */
GO

-- ========================================
-- KMT.Event_Attendance
-- ========================================
CREATE   PROCEDURE [KMT].[Event_Attendance]
    @CharID int,
    @Date date,
    @ReturnValue INT OUTPUT
AS
BEGIN
    SET NOCOUNT ON;
    -- [KMT].[Attendance_Players] tablosunda kayÄ±t yoksa ekle
    IF NOT EXISTS (SELECT 1 FROM [KMT].[Attendance_Players] WHERE CharID = @CharID)
    BEGIN
        INSERT INTO [KMT].[Attendance_Players] (CharID, DayCount, LastAttendedDate) 
        VALUES (@CharID, 1, @Date);
        
        -- Ä°lk defa eklenen kayÄ±tta CanTake deÄŸeri 1 olacak ÅŸekilde ekle
        INSERT INTO [KMT].[Attendance_RewardLog] (RefRewardID, CharID, DayCount, CanTake, AlreadyTaken)
        SELECT r.ID, @CharID, r.DayCount, 
               CASE WHEN r.DayCount = 1 THEN 1 ELSE 0 END AS CanTake,
               0 AS AlreadyTaken
        FROM [KMT].[Attendance_Rewards] r
        WHERE NOT EXISTS (
            SELECT 1 
            FROM [KMT].[Attendance_RewardLog] l 
            WHERE l.CharID = @CharID AND l.DayCount = r.DayCount AND l.RefRewardID = r.ID
        );
        SET @ReturnValue = 1;
        RETURN 1;
    END
    ELSE
    BEGIN
        DECLARE @CurrentDayCount int;
        DECLARE @LastDate date;

        SELECT @CurrentDayCount = DayCount, @LastDate = LastAttendedDate
        FROM [KMT].[Attendance_Players]
        WHERE CharID = @CharID;

        IF @LastDate < @Date
        BEGIN
            UPDATE [KMT].[Attendance_Players] 
            SET DayCount = @CurrentDayCount + 1, LastAttendedDate = @Date 
            WHERE CharID = @CharID;
            
            -- Mevcut kayÄ±tlarÄ± gÃ¼ncelle
            UPDATE [KMT].[Attendance_RewardLog]
            SET CanTake = 1
            WHERE CharID = @CharID
              AND EXISTS (
                  SELECT 1
                  FROM [KMT].[Attendance_Players] a
                  WHERE a.CharID = @CharID AND a.DayCount >= [KMT].[Attendance_RewardLog].DayCount AND [KMT].[Attendance_RewardLog].AlreadyTaken != 1
              )
              AND EXISTS (
                  SELECT 1
                  FROM [KMT].[Attendance_Rewards] r
                  WHERE r.DayCount = [KMT].[Attendance_RewardLog].DayCount AND r.ID = [KMT].[Attendance_RewardLog].RefRewardID
              );
            
            -- Eksik kayÄ±tlarÄ± ekle
            INSERT INTO [KMT].[Attendance_RewardLog] (RefRewardID, CharID, DayCount, CanTake, AlreadyTaken)
            SELECT r.ID, @CharID, r.DayCount,
                   CASE WHEN EXISTS (
                             SELECT 1
                             FROM [KMT].[Attendance_Players] a
                             WHERE a.CharID = @CharID AND a.DayCount >= r.DayCount
                         )
                         THEN 1
                         ELSE 0
                   END AS CanTake,
                   0 AS AlreadyTaken
            FROM [KMT].[Attendance_Rewards] r
            WHERE NOT EXISTS (
                SELECT 1 
                FROM [KMT].[Attendance_RewardLog] l 
                WHERE l.CharID = @CharID AND l.DayCount = r.DayCount AND l.RefRewardID = r.ID
            );
            SET @ReturnValue = 1;
            RETURN 1;
        END
        ELSE
        BEGIN
            SET @ReturnValue = 0;
            RETURN 0;
        END
    END
END

GO

-- ========================================
-- KMT.Event_MobKilled
-- ========================================

CREATE   PROCEDURE [KMT].[Event_MobKilled] 
    @RefObjID        INT,
    @KilledWorldID   INT,
    @KilledRegionID  SMALLINT,
    @KilledPosX      FLOAT,
    @KilledPosY      FLOAT,
    @KilledPosZ      FLOAT,
    @CharID          INT,
    @Charname        VARCHAR(32)
AS
BEGIN
    SET NOCOUNT ON;
    SET XACT_ABORT ON;

    -- Ø§Ø­Ø³Ø¨ ÙÙ‚Ø· Ù„Ùˆ Ø§Ù„Ù…ÙˆØ¨ Ø§Ù„Ù…Ø·Ù„ÙˆØ¨
    IF (@RefObjID = 175001116)
    BEGIN
        -- (Ø§Ø®ØªÙŠØ§Ø±ÙŠ) ØªØ£ÙƒØ¯ Ø¥Ù† Ø­Ø¯Ø« BeastFury (ID=9) Ø´ØºÙ‘Ø§Ù„ Ø¯Ù„ÙˆÙ‚ØªÙŠ
        IF EXISTS (
            SELECT 1
            FROM Events.dbo.Event_Control WITH (NOLOCK)
            WHERE EventID = 9 AND IsOpen = 1
        )
        BEGIN
            BEGIN TRY
                BEGIN TRAN;

                -- Ø­Ø§ÙˆÙ„ ØªØ²ÙˆØ¯ Ù„Ùˆ Ø§Ù„Ù„Ø§Ø¹Ø¨ Ù…ÙˆØ¬ÙˆØ¯
                UPDATE Events.dbo.Player_BeastFury WITH (ROWLOCK)
                SET KillCount      = KillCount + 1,
                    LastUpdatedUtc = SYSUTCDATETIME()
                WHERE CharID = @CharID;

                -- Ù„Ùˆ Ù…ÙÙŠØ´ ØµÙØŒ Ø§Ø¹Ù…Ù„Ù‡ Insert ÙƒØ¨Ø¯Ø§ÙŠØ© Ø¨Ù€ 1
                IF (@@ROWCOUNT = 0)
                BEGIN
                    INSERT INTO Events.dbo.Player_BeastFury (CharID, KillCount, LastUpdatedUtc)
                    VALUES (@CharID, 1, SYSUTCDATETIME());
                END

                COMMIT;
            END TRY
            BEGIN CATCH
                IF (XACT_STATE() <> 0) ROLLBACK;
                -- ØªÙ‚Ø¯Ø± ØªØ¶ÙŠÙ Ù„ÙˆØ¬ Ù‡Ù†Ø§ Ù„Ùˆ Ø­Ø§Ø¨Ø¨
                -- RAISERROR('HandleMobKilled failed: %s', 16, 1, ERROR_MESSAGE());
            END CATCH
        END
    END
END

GO

-- ========================================
-- KMT.Event_RegisterGoldLottery
-- ========================================

/* ÙŠØ³Ø¬Ù‘Ù„ Ø§Ù„Ù„Ø§Ø¹Ø¨ ÙÙŠ Lottery Gold (EventID=6) ÙˆÙŠØ®ØµÙ… Ø§Ù„ØªØ°ÙƒØ±Ø©.
   ÙŠÙØ³ØªØ¯Ø¹Ù‰ ÙÙ‚Ø· Ù…Ù† _OnEventRegister_EDIT.
*/
CREATE PROCEDURE [KMT].[Event_RegisterGoldLottery]
    @CharID     INT,
    @CharName16 VARCHAR(16)
AS
BEGIN
    SET NOCOUNT ON;

    DECLARE @CharName NVARCHAR(64) = CONVERT(NVARCHAR(64), @CharName16);
    DECLARE @NoticeType INT = 1;

    DECLARE @ClosedMsg NVARCHAR(MAX) = N'Lottery Gold: Registration is closed right now.';
    DECLARE @DupMsg    NVARCHAR(MAX) = N'Lottery Gold: You are already registered for this round.';
    DECLARE @NoGoldMsg NVARCHAR(MAX) = N'Lottery Gold: Not enough gold. You need 10,000,000 gold.';
    DECLARE @OkMsg     NVARCHAR(MAX) = N'Lottery Gold: 10,000,000 gold deducted. You are registered. Good luck!';
    DECLARE @FailMsg   NVARCHAR(MAX) = N'Lottery Gold: registration failed. Try again later.';

    /* Ù‚ÙÙ„ Ù„ÙƒÙ„ Ù„Ø§Ø¹Ø¨ Ù„Ù…Ù†Ø¹ Ø§Ù„Ø³Ø¨Ø§Ù‚/Ø§Ù„Ø¯Ø¨Ù„ ÙƒÙ„ÙŠÙƒ */
    DECLARE @LockResource NVARCHAR(100) = N'LotteryGold_Reg_' + CONVERT(NVARCHAR(20), @CharID);
    DECLARE @lockRes INT;
    EXEC @lockRes = sys.sp_getapplock
        @Resource=@LockResource, @LockMode=N'Exclusive', @LockOwner=N'Session', @LockTimeout=5000;
    IF (@lockRes < 0) RETURN;

    BEGIN TRY
        /* 0) Ù‚Ø±Ø§Ø¡Ø© ÙƒÙ†ØªØ±ÙˆÙ„ Ø§Ù„Ø­Ø¯Ø« Ù…Ù† Events */
        DECLARE @isOpen BIT, @s DATETIME2(3), @e DATETIME2(3), @cost BIGINT, @now DATETIME2(3)=SYSUTCDATETIME();
        SELECT @isOpen=IsOpen, @s=StartUtc, @e=EndUtc, @cost=TicketCost
        FROM [Events].[dbo].[_LotteryGold_Control] WITH (NOLOCK)
        WHERE EventID = 6;

        IF (@cost IS NULL) SET @cost = 10000000;

        /* Ø§Ù„Ø³Ù…Ø§Ø­ Ù„Ùˆ IsOpen=1 Ø£Ùˆ (@now Ø¨ÙŠÙ† Start..End) */
        IF ( ISNULL(@isOpen,0)=0 AND NOT (@s IS NOT NULL AND @e IS NOT NULL AND @now>=@s AND @now<@e) )
        BEGIN
            INSERT INTO [KMT].[Command_FilterQueue] (CommandID, Data1, Data2, Data3, Status)
            VALUES (28, CONVERT(NVARCHAR(50), @CharID), @ClosedMsg, N'1', 1);
            GOTO _EXIT;
        END

        /* 1) Ù„Ùˆ Ù…Ø³Ø¬Ù‘Ù„ Ø¨Ø§Ù„ÙØ¹Ù„ ÙÙŠ Ø¬Ø¯ÙˆÙ„ Events â†’ Ø±Ø³Ø§Ù„Ø© Ø¯Ø¨Ù„ */
        IF EXISTS (SELECT 1 FROM [Events].[dbo].[_LotteryGold_RegPlayers] WITH (NOLOCK) WHERE CharID=@CharID)
        BEGIN
            INSERT INTO [KMT].[Command_FilterQueue] (CommandID, Data1, Data2, Data3, Status)
            VALUES (28, CONVERT(NVARCHAR(50), @CharID), @DupMsg, N'1', 1);
            GOTO _EXIT;
        END

        /* 2) ØªØ£ÙƒÙŠØ¯ Ø§Ù„Ø±ØµÙŠØ¯ Ù…Ù† Ø§Ù„Ø´Ø§Ø±Ø¯ */
        DECLARE @CurrentGold BIGINT;
        SELECT @CurrentGold = C.RemainGold  -- ØºÙŠÙ‘Ø± Ø§Ù„Ø§Ø³Ù… Ù„Ùˆ Ø¹Ù…ÙˆØ¯Ùƒ Ù…Ø®ØªÙ„Ù
        FROM [SRO_VT_SHARD].[dbo].[_Char] AS C WITH (NOLOCK)
        WHERE C.CharID = @CharID;

        IF (@CurrentGold IS NULL OR @CurrentGold < @cost)
        BEGIN
            INSERT INTO [KMT].[Command_FilterQueue] (CommandID, Data1, Data2, Data3, Status)
            VALUES (28, CONVERT(NVARCHAR(50), @CharID), @NoGoldMsg, N'1', 1);
            GOTO _EXIT;
        END

        /* 3) Ø¥Ø¯Ø±Ø§Ø¬ Ø§Ù„ØªØ³Ø¬ÙŠÙ„ Ø£ÙˆÙ„Ù‹Ø§ ÙÙŠ Events (Ù†ØªØ¹Ø§Ù…Ù„ Ù…Ø¹ PK/Unique Ø¨Ø´ÙƒÙ„ ØµØ±ÙŠØ­) */
        BEGIN TRY
            INSERT INTO [Events].[dbo].[_LotteryGold_RegPlayers] (CharID, CharName, GoldDeducted)
            VALUES (@CharID, @CharName, @cost);
        END TRY
        BEGIN CATCH
            IF ERROR_NUMBER() IN (2627, 2601)
            BEGIN
                INSERT INTO [KMT].[Command_FilterQueue] (CommandID, Data1, Data2, Data3, Status)
                VALUES (28, CONVERT(NVARCHAR(50), @CharID), @DupMsg, N'1', 1);
                GOTO _EXIT;
            END
            ELSE
            BEGIN
                DECLARE @Err1 NVARCHAR(MAX) = CONCAT(N'LotteryGold_Register_KMTGuard INSERT ERROR: ', ERROR_MESSAGE(),
                                      N' [', ERROR_NUMBER(), '/', ERROR_SEVERITY(), '/', ERROR_STATE(),
                                      N' @ line ', ERROR_LINE(), N']');
                PRINT @Err1;

                INSERT INTO [KMT].[Command_FilterQueue] (CommandID, Data1, Data2, Data3, Status)
                VALUES (28, CONVERT(NVARCHAR(50), @CharID), @FailMsg, N'1', 1);
                GOTO _EXIT;
            END
        END CATCH

        /* 4) Ø®ØµÙ… Ø§Ù„Ø¬ÙˆÙ„Ø¯ Ø¹Ø¨Ø± __LiveGold */
        BEGIN TRY
            EXEC [KMT].[Live_Gold] @CharID=@CharID, @Gold=@cost, @AddOrRemove=0;  -- 0=Remove
        END TRY
        BEGIN CATCH
            /* ÙØ´Ù„ Ø§Ù„Ø®ØµÙ… â†’ Ù†Ø´ÙŠÙ„ Ø§Ù„ØªØ³Ø¬ÙŠÙ„ Ø¹Ù„Ø´Ø§Ù† Ø§Ù„Ø§ØªØ³Ø§Ù‚ */
            DELETE FROM [Events].[dbo].[_LotteryGold_RegPlayers] WHERE CharID=@CharID;

            DECLARE @Err2 NVARCHAR(MAX) = CONCAT(N'LotteryGold_Register_KMTGuard DEDUCT ERROR: ', ERROR_MESSAGE(),
                                  N' [', ERROR_NUMBER(), '/', ERROR_SEVERITY(), '/', ERROR_STATE(),
                                  N' @ line ', ERROR_LINE(), N']');
            PRINT @Err2;

            INSERT INTO [KMT].[Command_FilterQueue] (CommandID, Data1, Data2, Data3, Status)
            VALUES (28, CONVERT(NVARCHAR(50), @CharID), @FailMsg, N'1', 1);
            GOTO _EXIT;
        END CATCH

        /* 5) Ø±Ø³Ø§Ù„Ø© Ù†Ø¬Ø§Ø­ */
        INSERT INTO [KMT].[Command_FilterQueue] (CommandID, Data1, Data2, Data3, Status)
        VALUES (28, CONVERT(NVARCHAR(50), @CharID), @OkMsg, N'1', 1);

    END TRY
    BEGIN CATCH
        DECLARE @Err NVARCHAR(MAX) = CONCAT(N'LotteryGold_Register_KMTGuard ERROR: ', ERROR_MESSAGE(),
                            N' [', ERROR_NUMBER(), '/', ERROR_SEVERITY(), '/', ERROR_STATE(),
                            N' @ line ', ERROR_LINE(), N']');
        PRINT @Err;

        INSERT INTO [KMT].[Command_FilterQueue] (CommandID, Data1, Data2, Data3, Status)
        VALUES (28, CONVERT(NVARCHAR(50), @CharID), @FailMsg, N'1', 1);
    END CATCH

_EXIT:
    EXEC sys.sp_releaseapplock @Resource=@LockResource, @LockOwner=N'Session';
END

GO

-- ========================================
-- KMT.Event_Reload
-- ========================================

CREATE   PROCEDURE [KMT].[Event_Reload]
    @RequestedBy nvarchar(64) = N'Scheduler'
AS
BEGIN
    SET NOCOUNT ON;

    INSERT INTO [KMT].[AutoEvent_CommandQueue] (CommandType, EventCode, RequestedBy)
    VALUES (N'RELOAD', N'', @RequestedBy);
END

GO

-- ========================================
-- KMT.Event_Start
-- ========================================

CREATE   PROCEDURE [KMT].[Event_Start]
    @EventCode nvarchar(32),
    @RequestedBy nvarchar(64) = N'Scheduler'
AS
BEGIN
    SET NOCOUNT ON;

    INSERT INTO [KMT].[AutoEvent_CommandQueue] (CommandType, EventCode, RequestedBy)
    VALUES (N'START', @EventCode, @RequestedBy);
END

GO

-- ========================================
-- KMT.Event_Stop
-- ========================================

CREATE   PROCEDURE [KMT].[Event_Stop]
    @RequestedBy nvarchar(64) = N'Scheduler'
AS
BEGIN
    SET NOCOUNT ON;

    INSERT INTO [KMT].[AutoEvent_CommandQueue] (CommandType, EventCode, RequestedBy)
    VALUES (N'STOP', N'', @RequestedBy);
END

GO

-- ========================================
-- KMT.Hook_AlchemySuccess
-- ========================================
CREATE   PROCEDURE [KMT].[Hook_AlchemySuccess]
	@CharID int,
	@CharName varchar(25),
	@ItemID int,
	@Plus tinyint,
	@AdvLevel tinyint,
	@Slot tinyint
AS
-- Alchemy Plus Notice Link
	--INSERT INTO [KMT].[Command_GameServerQueue](Action_ID, Data1, Data2, Data3, Data4, Data5) VALUES(131, @CharID, @ItemID, @Plus + @AdvLevel, @Slot, @AdvLevel)

	/*
		Bu prosedÃ¼rÃ¼ dÃ¼zenleyerek bir karakter Alchemy ile baÅŸarÄ±lÄ± bir ÅŸekilde artÄ± bastÄ±ÄŸÄ±nda iÅŸlem yapabilirsiniz.

		@CharID = EÅŸya geliÅŸtiren karakterin IDsi.
		@CharName = EÅŸya geliÅŸtiren karakterin ismi.
		@ItemID = GeliÅŸtirilen eÅŸyanÄ±n IDsi.
		@Plus = GeliÅŸtirilen eÅŸyanÄ±n geldiÄŸi + seviyesi. Adv. Elixir dahil edilmeden bildirilir.
		@AdvLevel = Var ise, eÅŸyadaki Advanced Elixir seviyesi.
		@Slot = GeliÅŸtirilen eÅŸyanÄ±n bulunduÄŸu slot.
		Bu veritabanÄ±ndaki diÄŸer prosedÃ¼rlerin aksine bu prosedÃ¼r her restartta orjinal haline dÃ¶ndÃ¼rÃ¼lmeyecektir. Dikkatli dÃ¼zenleyiniz!
	*/

GO

-- ========================================
-- KMT.Hook_AutoEquip
-- ========================================

CREATE   PROCEDURE [KMT].[Hook_AutoEquip]
    @CharID INT
AS
BEGIN
    SET NOCOUNT ON;

    DECLARE 
        @WID      INT,
        @RegionID INT,
        @X        INT,
        @Y        INT,
        @Z        INT,
        @Level    INT;

    -- Ø§Ø³Ø­Ø¨ Ø¨ÙŠØ§Ù†Ø§Øª Ø§Ù„Ù„Ø§Ø¹Ø¨ Ù…Ø±Ø© ÙˆØ§Ø­Ø¯Ø©
    SELECT
        @WID      = WorldID,
        @RegionID = LatestRegion,
        @X        = PosX,
        @Y        = PosY,
        @Z        = PosZ,
        @Level    = CurLevel
    FROM SRO_VT_SHARD.._Char WITH (NOLOCK)
    WHERE CharID = @CharID;

    -- Ù„Ùˆ CharID Ù…Ø´ Ù…ÙˆØ¬ÙˆØ¯
    IF @WID IS NULL
    BEGIN
        EXEC [KMT].[Command_NoticeByID]
             @CharID     = @CharID,
             @NoticeType = 3,
             @Notice     = 'Character data was not found. Please relog and try again.';
        RETURN;
    END

    --------------------------------------------------------------------
    -- NEW) Ù„Ùˆ Ù„ÙÙ„ Ø§Ù„Ù„Ø§Ø¹Ø¨ Ø£Ø¹Ù„Ù‰ Ù…Ù† 90: Ù…ØªØ¹Ù…Ù„Ø´ Auto Equip ÙˆØ§Ø¨Ø¹Ø« Ù…Ø³Ø¯Ø¬
    --------------------------------------------------------------------
    IF @Level > 90
    BEGIN
        EXEC [KMT].[Command_NoticeByID]
             @CharID     = @CharID,
             @NoticeType = 3,
             @Notice     = 'Auto-Equip is available only up to level 90.';
        RETURN;
    END

    --------------------------------------------------------------------
    -- 1) Ø§Ø³ØªØ¯Ø¹Ø§Ø¡ Ø¨Ø±ÙˆØ³ÙŠØ¯ Ø§Ù„Ø£ÙˆØªÙˆ Ø¥ÙŠÙƒÙˆÙŠØ¨ ÙÙŠ hema_system ÙˆØªÙ…Ø±ÙŠØ± Ø§Ù„Ù„ÙŠÙÙ„ Ø§Ù„Ø­Ø§Ù„ÙŠ
    --------------------------------------------------------------------
    EXEC hema_system.dbo._AutoEquipt
         @CharID = @CharID,
         @data2  = @Level;

    --------------------------------------------------------------------
    -- 2) (Ø§Ø®ØªÙŠØ§Ø±ÙŠ) Ø¥Ø±Ø³Ø§Ù„ Ø£Ù…Ø± Ù„Ù„Ù€ GameServer (Ù„Ùˆ Ù…Ø­ØªØ§Ø¬)
    -- Ø³ÙŠØ¨Ù‡ Ø²ÙŠ Ù…Ø§ Ù‡Ùˆ Ù„Ùˆ Ù…Ø´ Ù…Ø­ØªØ§Ø¬
    --------------------------------------------------------------------
END

GO

-- ========================================
-- KMT.Hook_CharacterGetUp
-- ========================================
CREATE   PROCEDURE [KMT].[Hook_CharacterGetUp]
	@CharID int,
	@CharName varchar(25),
	@LatestRegion int,
	@LatestWorld int,
	@PVPState tinyint
AS
	
	return 1;


	/*
		Bu prosedÃ¼rÃ¼ dÃ¼zenleyerek bir karakterin Ã¶lÃ¼ iken ayaÄŸa kalkmasÄ±nÄ± kontrol edebilirsiniz.
		ProsedÃ¼rden 0 dÃ¶ndÃ¼rÃ¼rseniz ayaÄŸa kalkmasÄ± engellenecektir.
		ProsedÃ¼rden 1 dÃ¶ndÃ¼rÃ¼rseniz ayaÄŸa kalkmasÄ±na izin verilecektir.
		DÃ¶ndÃ¼rdÃ¼ÄŸÃ¼nÃ¼z sonuca gÃ¶re bir mesaj gÃ¶ndermeniz gerekmektedir. Filter herhangi bir mesaj gÃ¶rÃ¼ntÃ¼lemez.

		@CharID = Kalkmaya Ã§alÄ±ÅŸan karakterin Char IDsi.
		@Charname = Kalkmaya Ã§alÄ±ÅŸan karakterin Char AdÄ±.
		@LatestRegion = Kalkmaya Ã§alÄ±ÅŸan karakterin Region IDsi. (Teleportla son spawn olduÄŸu region gÃ¶nderilir.)
		@LatestWorld = Kalkmaya Ã§alÄ±ÅŸan karakterin World IDsi.
		@PVPState = Kalkmaya Ã§alÄ±ÅŸan karakterin PVP State rengi.
		
		Bridge Command 84'Ã¼ kullanarak karaterin istediÄŸiniz lokasyonda doÄŸmasÄ±nÄ± saÄŸlayabilirsiniz. Bu durumda return 0 yapmanÄ±z gerekecektir.

		Bu veritabanÄ±ndaki diÄŸer prosedÃ¼rlerin aksine bu prosedÃ¼r her restartta orjinal haline dÃ¶ndÃ¼rÃ¼lmeyecektir. Dikkatli dÃ¼zenleyiniz!
	*/

	/*
		You may modify this procedure to have control over a character getting up while dead.
		By returning 0 from this procedure, character's getting up will be blocked.
		By returning 1 from this procedure, character's getting up will be allowed.
		You have to notify the player yourself according to the action you're taking. Filter will not show any messages itself.

		@CharID = The CharID of the character trying to use get up.
		@Charname = The Charname of the character trying to get up.
		@LatestRegion = The Region ID of the character trying to get up. (The latest spawned Region ID will be sent.)
		@LatestWorld = The World ID of the character trying to get up.
		@PVPState = The PVP State (color) of the character trying to get up.
		
		By using the Bridge Command 84 through here, you can make character spawn at your desired position. In this case you must return 0.

		Unlike other Stored Procedures in this database, this stored procedure will not be restored back to default on restarts. Modify it carefully!
	*/

GO

-- ========================================
-- KMT.Hook_CharacterKill
-- ========================================

CREATE   PROCEDURE [KMT].[Hook_CharacterKill]
    @WorldID         int,
    @RegionID        int,
    @KillerCharID    int,
    @KillerCharName  varchar(25),  -- Ù„Ù„ØªÙˆØ§ÙÙ‚ ÙÙ‚Ø· (ØºÙŠØ± Ù…Ø³ØªØ®Ø¯Ù…)
    @KillerPVPState  tinyint,
    @KillerJobStatus tinyint,
    @KillerGuildID   int,
    @KillerGuildName varchar(25),  -- ØºÙŠØ± Ù…Ø³ØªØ®Ø¯Ù…
    @DeadCharID      int,
    @DeadCharName    varchar(25),  -- ØºÙŠØ± Ù…Ø³ØªØ®Ø¯Ù…
    @DeadPVPState    tinyint,
    @DeadJobStatus   tinyint,
    @DeadGuildID     int,
    @DeadGuildName   varchar(25)   -- ØºÙŠØ± Ù…Ø³ØªØ®Ø¯Ù…
AS
BEGIN
    SET NOCOUNT ON;

    DECLARE @EventID int = 10;
    DECLARE @Active bit = 0, @Start datetime2(3), @End datetime2(3);

    /* ØªØ­Ù‚Ù‘Ù‚ Ø¥Ù† Ø­Ø¯Ø« LMS Ø´ØºØ§Ù„ Ø­Ø§Ù„ÙŠÙ‹Ø§ */
    IF OBJECT_ID('Events.dbo.Event_Control','U') IS NOT NULL
    BEGIN
        SELECT TOP (1)
               @Active =
                 CASE
                   WHEN IsOpen = 1 THEN 1
                   WHEN (StartUtc IS NOT NULL AND EndUtc IS NOT NULL
                         AND SYSUTCDATETIME() BETWEEN StartUtc AND EndUtc) THEN 1
                   ELSE 0
                 END,
               @Start = StartUtc,
               @End   = EndUtc
        FROM Events.dbo.Event_Control WITH (NOLOCK)
        WHERE EventID = @EventID;
    END

    /* Ù„Ùˆ Ø§Ù„Ø­Ø¯Ø« Ù…Ø´ Ø´ØºØ§Ù„: Ø§Ø®Ø±Ø¬ Ø¨Ø¯ÙˆÙ† Ø£ÙŠ INSERT */
    IF (@Active <> 1 OR @Start IS NULL)
        RETURN;

    /* 1) Ø³Ø¬Ù„ ÙÙŠ [KMT].[Log_PlayerKills] â€” IDs ÙÙ‚Ø· */
  

    /* 2) Ø²ÙˆÙ‘Ø¯ Ø¹Ø¯Ù‘Ø§Ø¯ Ø§Ù„Ù‚Ø§ØªÙ„ ÙÙŠ Events..Event_LMS_Deaths (UPsert Ù„ÙƒÙ„ Ø¬ÙˆÙ„Ø©) */
    BEGIN TRY
        IF OBJECT_ID('Events.dbo.Event_LMS_Deaths','U') IS NOT NULL
        BEGIN
            MERGE Events.dbo.Event_LMS_Deaths AS T
            USING (SELECT @EventID AS EventID, @Start AS EventStartUtc, @KillerCharID AS KillerCharID) AS S
            ON (T.EventID = S.EventID AND T.EventStartUtc = S.EventStartUtc AND T.KillerCharID = S.KillerCharID)
            WHEN MATCHED THEN
                UPDATE SET KillCount = T.KillCount + 1
            WHEN NOT MATCHED THEN
                INSERT (EventID, EventStartUtc, KillerCharID, KillCount)
                VALUES (S.EventID, S.EventStartUtc, S.KillerCharID, 1);
        END
    END TRY
    BEGIN CATCH
        -- ØªØ¬Ø§Ù‡Ù„ Ø£Ø®Ø·Ø§Ø¡ Ø§Ù„Ø¹Ø¯Ù‘Ø§Ø¯ Ø¹Ø´Ø§Ù† Ù…Ø§ Ù†Ø¹Ø·Ù„Ø´ Ø§Ù„Ù„ÙˆØ¬ Ø§Ù„Ø£Ø³Ø§Ø³ÙŠ
        -- Ù„Ùˆ Ø¹Ø§ÙŠØ²: Ø£Ù‚Ø¯Ø± Ø£Ø¶ÙŠÙ Ù„ÙˆØ¬ Ø£Ø®Ø·Ø§Ø¡ Ù‡Ù†Ø§ ÙÙŠ Ø¬Ø¯ÙˆÙ„ Ù…Ù†ÙØµÙ„.
    END CATCH

    /* 3) Ø±Ø¬Ù‘Ø¹ Ø§Ù„Ù…Ù‚ØªÙˆÙ„ Ù„Ù„Ù…Ø¯ÙŠÙ†Ø© ÙÙˆØ±Ù‹Ø§ */
    BEGIN TRY
        EXEC [KMT].[Teleport_PlayerToTown] @CharID = @DeadCharID;
    END TRY
    BEGIN CATCH
        -- ØªØ¬Ø§Ù‡Ù„ Ø£Ø®Ø·Ø§Ø¡ Ø§Ù„ØªÙ„ÙŠ Ø¨ÙˆØ±Øª
    END CATCH
END

GO

-- ========================================
-- KMT.Hook_EventCancel
-- ========================================

/* Ø¥Ù„ØºØ§Ø¡ ØªØ³Ø¬ÙŠÙ„ Ù„Ø§Ø¹Ø¨ ÙÙŠ Ø¬ÙˆÙ„Ø© Ø§Ù„Ø¥ÙŠÙÙ†Øª Ø§Ù„Ø­Ø§Ù„ÙŠØ© + Ø±Ø¯Ù‘ Ø§Ù„Ø±Ø³ÙˆÙ… Ø¥Ù† Ù„Ø²Ù… (Gold/Silk Ø¹Ø¨Ø± Ø¥Ø¬Ø±Ø§Ø¡Ø§Øª Live) */
CREATE   PROCEDURE [KMT].[Hook_EventCancel]
    @CharID        INT,
    @CharName16    VARCHAR(25),
    @EventID       SMALLINT,
    @LiveRegionID  INT,
    @WorldID       INT
AS
BEGIN
    SET NOCOUNT ON;
    SET XACT_ABORT ON;

    /* Ù‚ÙÙ„ Ø¹Ù„Ù‰ Ù…Ø³ØªÙˆÙ‰ Ø§Ù„Ù„Ø§Ø¹Ø¨/Ø§Ù„Ø¥ÙŠÙÙ†Øª Ù„Ù…Ù†Ø¹ Ø§Ø²Ø¯ÙˆØ§Ø¬ Ø§Ù„Ø¥Ù„ØºØ§Ø¡/Ø§Ù„Ø±Ø¯ */
    DECLARE @LockRes INT;
    DECLARE @LockResource NVARCHAR(100) = N'EventCancel_' + CONVERT(NVARCHAR(10), @EventID) + N'_' + CONVERT(NVARCHAR(20), @CharID);

    EXEC @LockRes = sys.sp_getapplock
        @Resource    = @LockResource,
        @LockMode    = N'Exclusive',
        @LockOwner   = N'Session',
        @LockTimeout = 5000;
    IF (@LockRes < 0) RETURN;

    BEGIN TRY
        /* 1) Ù…Ø¹Ù„ÙˆÙ…Ø§Øª Ø§Ù„Ø¬ÙˆÙ„Ø© Ø§Ù„Ø­Ø§Ù„ÙŠØ© Ù…Ù† Events..Event_Control */
        DECLARE @nowUtc DATETIME2(3) = SYSUTCDATETIME();
        DECLARE @isOpen BIT, @s DATETIME2(3), @e DATETIME2(3), @evtCode SYSNAME;

        SELECT
            @isOpen  = EC.IsOpen,
            @s       = EC.StartUtc,
            @e       = EC.EndUtc,
            @evtCode = EC.EventCode
        FROM [Events].[dbo].[Event_Control] AS EC WITH (NOLOCK)
        WHERE EC.EventID = @EventID;

        /* Ù„Ùˆ Ù…ÙÙŠØ´ Ù†Ø§ÙØ°Ø© Ù†Ø´Ø·Ø© Ø­Ø§Ù„ÙŠÙ‹Ø§: Ø§Ù„Ø¥Ù„ØºØ§Ø¡ ØºÙŠØ± Ù…Ø³Ù…ÙˆØ­ */
        IF (@s IS NULL OR @e IS NULL OR @nowUtc < @s OR @nowUtc >= @e)
        BEGIN
            INSERT INTO [KMT].[Command_FilterQueue] (CommandID, Data1, Data2, Data3, Status)
            VALUES (28, CONVERT(NVARCHAR(50), @CharID),
                    N'Event: cancellation is not available right now.', N'1', 1);
            GOTO _EXIT;
        END

        /* 2) ØµÙ ØªØ³Ø¬ÙŠÙ„ Ø§Ù„Ù„Ø§Ø¹Ø¨ ÙÙŠ Ø§Ù„Ø¬ÙˆÙ„Ø© Ø§Ù„Ø­Ø§Ù„ÙŠØ© */
        DECLARE @amount BIGINT, @currency NVARCHAR(16), @eventStart DATETIME2(3);

        SELECT TOP (1)
            @amount     = RP.Amount,
            @currency   = RP.Currency,
            @eventStart = RP.EventStartUtc
        FROM [Events].[dbo].[Event_RegPlayers] AS RP WITH (UPDLOCK, HOLDLOCK)
        WHERE RP.EventID = @EventID
          AND RP.EventStartUtc = @s
          AND RP.CharID = @CharID;

        IF (@eventStart IS NULL)
        BEGIN
            INSERT INTO [KMT].[Command_FilterQueue] (CommandID, Data1, Data2, Data3, Status)
            VALUES (28, CONVERT(NVARCHAR(50), @CharID),
                    N'You are not registered for the current round.', N'1', 1);
            GOTO _EXIT;
        END

        /* 3) ØªÙ†ÙÙŠØ° Ø§Ù„Ø¥Ù„ØºØ§Ø¡ + Ø±Ø¯ Ø§Ù„Ø±Ø³ÙˆÙ… Ø¹Ù†Ø¯ Ø§Ù„Ø­Ø§Ø¬Ø© Ø¯Ø§Ø®Ù„ ØªØ±Ø§Ù†Ø²Ø§ÙƒØ´Ù† */
        BEGIN TRAN;

            IF (ISNULL(@amount,0) > 0)
            BEGIN
                /* Lottery Gold (EventID=6 Ø£Ùˆ Currency='gold') */
                IF (@EventID = 6 OR LOWER(@currency) = N'gold')
                BEGIN
                    BEGIN TRY
                        EXEC [KMT].[Live_Gold]
                             @CharID=@CharID, @Gold=@amount, @AddOrRemove=1; -- 1 = Add (Refund)
                    END TRY
                    BEGIN CATCH
                        ROLLBACK TRAN;
                        INSERT INTO [KMT].[Command_FilterQueue] (CommandID, Data1, Data2, Data3, Status)
                        VALUES (28, CONVERT(NVARCHAR(50), @CharID),
                                N'Cancel failed: refund (gold) error. Try again later.', N'1', 1);
                        GOTO _EXIT;
                    END CATCH
                END
                /* Lottery Silk (EventID=5 Ø£Ùˆ Currency='silk') Ø¹Ø¨Ø± __LiveSilk */
                ELSE IF (@EventID = 5 OR LOWER(@currency) = N'silk')
                BEGIN
                    /* __LiveSilk Ø¨ÙŠØ§Ø®Ø¯ INTØŒ ÙÙ†ØªØ£ÙƒØ¯ Ø¥Ù† Ø§Ù„Ù…Ø¨Ù„Øº Ø¶Ù…Ù† Ø§Ù„Ù…Ø¯Ù‰ */
                    IF (@amount < 0 OR @amount > 2147483647)
                    BEGIN
                        ROLLBACK TRAN;
                        INSERT INTO [KMT].[Command_FilterQueue] (CommandID, Data1, Data2, Data3, Status)
                        VALUES (28, CONVERT(NVARCHAR(50), @CharID),
                                N'Cancel failed: refund (silk) amount out of range.', N'1', 1);
                        GOTO _EXIT;
                    END

                    DECLARE @nSilkRefund INT = CAST(@amount AS INT);
                    DECLARE @rc INT;

                    EXEC @rc = [KMT].[Live_Silk]
                         @CharID     = @CharID,
                         @nSilk      = @nSilkRefund,  -- Ø§Ø³ØªØ±Ø¬Ø§Ø¹
                         @nSilkGift  = 0,
                         @nSilkPoint = 0;

                    IF (@rc <> 0)
                    BEGIN
                        ROLLBACK TRAN;
                        INSERT INTO [KMT].[Command_FilterQueue] (CommandID, Data1, Data2, Data3, Status)
                        VALUES (28, CONVERT(NVARCHAR(50), @CharID),
                                N'Cancel failed: refund (silk) error.', N'1', 1);
                        GOTO _EXIT;
                    END
                END
                /* Ø¨Ø§Ù‚ÙŠ Ø§Ù„Ø¥ÙŠÙÙ†ØªØ§Øª Ù…Ø¬Ø§Ù†ÙŠØ© â†’ Ù„Ø§ Ø­Ø§Ø¬Ø© Ù„Ø±Ø¯Ù‘ */
            END

            /* Ø§Ø­Ø°Ù ØªØ³Ø¬ÙŠÙ„ Ø§Ù„Ù„Ø§Ø¹Ø¨ Ù…Ù† Ø§Ù„Ø¬ÙˆÙ„Ø© */
            DELETE FROM [Events].[dbo].[Event_RegPlayers]
            WHERE EventID = @EventID
              AND EventStartUtc = @s
              AND CharID = @CharID;

        COMMIT TRAN;

        /* 4) Ø±Ø³Ø§Ù„Ø© Ù†Ø¬Ø§Ø­ */
        DECLARE @okMsg NVARCHAR(MAX) =
            CASE
                WHEN ISNULL(@amount,0) > 0 AND (LOWER(ISNULL(@currency,N'')) IN (N'gold', N'silk') OR @EventID IN (5,6))
                    THEN N'Registration canceled successfully. Refund: ' + CONVERT(NVARCHAR(30), @amount) + N' ' + ISNULL(@currency,N'coins') + N'.'
                ELSE N'Registration canceled successfully.'
            END;

        INSERT INTO [KMT].[Command_FilterQueue] (CommandID, Data1, Data2, Data3, Status)
        VALUES (28, CONVERT(NVARCHAR(50), @CharID), @okMsg, N'1', 1);

    END TRY
    BEGIN CATCH
        IF (XACT_STATE() <> 0) ROLLBACK TRAN;

        DECLARE @Err NVARCHAR(MAX) = CONCAT(
            N'_OnEventCancel_EDIT ERROR: ', ERROR_MESSAGE(),
            N' [', ERROR_NUMBER(), '/', ERROR_SEVERITY(), '/', ERROR_STATE(), N' @ ', ERROR_LINE(), N']'
        );

        INSERT INTO [KMT].[Command_FilterQueue] (CommandID, Data1, Data2, Data3, Status)
        VALUES (28, CONVERT(NVARCHAR(50), @CharID),
                N'Cancel failed due to an internal error. Please try again later.', N'1', 1);

        PRINT @Err;
        EXEC sys.sp_releaseapplock @Resource=@LockResource, @LockOwner=N'Session';
        ;THROW;
    END CATCH

_EXIT:
    EXEC sys.sp_releaseapplock @Resource=@LockResource, @LockOwner=N'Session';
END

GO

-- ========================================
-- KMT.Hook_EventRegister
-- ========================================

CREATE   PROCEDURE [KMT].[Hook_EventRegister]
    @CharID       INT,
    @CharName16   VARCHAR(16),
    @EventID      SMALLINT,
    @LiveRegionID INT,
    @WorldID      INT
AS
BEGIN
    SET NOCOUNT ON;


	-------------------------------------------------------------------------
    IF (@EventID = 5)
    BEGIN
        DECLARE @CharName64 NVARCHAR(64) = CONVERT(NVARCHAR(64), @CharName16);

        EXEC [Events].[dbo].[_LotterySilk_Register]
             @CharID   = @CharID,
             @CharName = @CharName64;
        RETURN;
    END

    -------------------------------------------------------------------------
    -- Lottery Gold (ID=6)
    -------------------------------------------------------------------------
    IF (@EventID = 6)
    BEGIN
        DECLARE @CharName64_ NVARCHAR(64) = CONVERT(NVARCHAR(64), @CharName16);

        EXEC [Events].[dbo].[_LotteryGold_Register]
             @CharID   = @CharID,
             @CharName = @CharName64_;
        RETURN;
    END

    -------------------------------------------------------------------------
    -- Beast Fury (ID=9)
    -------------------------------------------------------------------------
    IF (@EventID = 9)
    BEGIN
        DECLARE @CharName64_BF NVARCHAR(64) = CONVERT(NVARCHAR(64), @CharName16);

        IF OBJECT_ID(N'[Events].[dbo].[_BeastFury_Regist]', N'P') IS NOT NULL
        BEGIN
            BEGIN TRY
                EXEC [Events].[dbo].[_BeastFury_Regist]
                     @CharID   = @CharID,
                     @CharName = @CharName64_BF;
            END TRY
            BEGIN CATCH
                DECLARE @MsgBF NVARCHAR(MAX) =
                    N'Beast Fury: registration failed (' + CONVERT(NVARCHAR(10), ERROR_NUMBER()) + N').';
                INSERT INTO [KMT].[Command_FilterQueue](CommandID, Data1, Data2, Data3, Status)
                VALUES (28, CONVERT(NVARCHAR(50), @CharID), @MsgBF, N'1', 1);
            END CATCH;
            RETURN;
        END
        ELSE
        BEGIN
            INSERT INTO [KMT].[Command_FilterQueue](CommandID, Data1, Data2, Data3, Status)
            VALUES (28, CONVERT(NVARCHAR(50), @CharID),
                    N'Beast Fury: registration is temporarily unavailable.', N'1', 1);
            RETURN;
        END
    END

    -------------------------------------------------------------------------
    -- Last Man Standing (ID=10)
    -------------------------------------------------------------------------
    IF (@EventID = 10)
    BEGIN
        DECLARE @CharName64_LMS NVARCHAR(64) = CONVERT(NVARCHAR(64), @CharName16);

        IF OBJECT_ID(N'[Events].[dbo].[_LMS_Regist]', N'P') IS NOT NULL
        BEGIN
            BEGIN TRY
                EXEC [Events].[dbo].[_LMS_Regist]
                     @CharID   = @CharID,
                     @CharName = @CharName64_LMS;
            END TRY
            BEGIN CATCH
                DECLARE @MsgLMS NVARCHAR(MAX) =
                    N'LMS: Registration failed (' + CONVERT(NVARCHAR(10), ERROR_NUMBER()) + N').';
                INSERT INTO [KMT].[Command_FilterQueue](CommandID, Data1, Data2, Data3, Status)
                VALUES (28, CONVERT(NVARCHAR(50), @CharID), @MsgLMS, N'1', 1);
            END CATCH;
            RETURN;
        END
        ELSE
        BEGIN
            INSERT INTO [KMT].[Command_FilterQueue](CommandID, Data1, Data2, Data3, Status)
            VALUES (28, CONVERT(NVARCHAR(50), @CharID),
                    N'LMS: Registration is temporarily unavailable.', N'1', 1);
            RETURN;
        END
    END

    -------------------------------------------------------------------------
    -- Survival Solo (ID=11)
    -------------------------------------------------------------------------
    IF (@EventID = 11)
    BEGIN
        DECLARE @CharName64_SURV NVARCHAR(64) = CONVERT(NVARCHAR(64), @CharName16);

        IF OBJECT_ID(N'[Events].[dbo].[_Survival_Solo_Regist]', N'P') IS NOT NULL
        BEGIN
            BEGIN TRY
                EXEC [Events].[dbo].[_Survival_Solo_Regist]
                     @CharID   = @CharID,
                     @CharName = @CharName64_SURV;
            END TRY
            BEGIN CATCH
                DECLARE @MsgSURV NVARCHAR(MAX) =
                    N'Survival Solo: Registration failed (' + CONVERT(NVARCHAR(10), ERROR_NUMBER()) + N').';
                INSERT INTO [KMT].[Command_FilterQueue](CommandID, Data1, Data2, Data3, Status)
                VALUES (28, CONVERT(NVARCHAR(50), @CharID), @MsgSURV, N'1', 1);
            END CATCH;
            RETURN;
        END
        ELSE
        BEGIN
            INSERT INTO [KMT].[Command_FilterQueue](CommandID, Data1, Data2, Data3, Status)
            VALUES (28, CONVERT(NVARCHAR(50), @CharID),
                    N'Survival Solo: Registration is temporarily unavailable.', N'1', 1);
            RETURN;
        END
    END

    -------------------------------------------------------------------------
    -- Survival Party (ID=12)
    -------------------------------------------------------------------------
    IF (@EventID = 12)
    BEGIN
        DECLARE @CharName64_SVP NVARCHAR(64) = CONVERT(NVARCHAR(64), @CharName16);

        IF NOT EXISTS (
            SELECT 1
            FROM [KMT].[Party_Members] WITH (NOLOCK)
            WHERE CharID = @CharID AND PartyID IS NOT NULL AND PartyID <> 0
        )
        BEGIN
            INSERT INTO [KMT].[Command_FilterQueue](CommandID, Data1, Data2, Data3, Status)
            VALUES (28, CONVERT(NVARCHAR(50), @CharID),
                    N'Survival party: You must be in a Party to register.',
                    N'1', 1);
            RETURN;
        END

        IF OBJECT_ID(N'[Events].[dbo].[_SURVIVAL_PARTY_Regist]', N'P') IS NOT NULL
        BEGIN
            BEGIN TRY
                EXEC [Events].[dbo].[_SURVIVAL_PARTY_Regist]
                     @CharID   = @CharID,
                     @CharName = @CharName64_SVP;
            END TRY
            BEGIN CATCH
                DECLARE @MsgSVP NVARCHAR(MAX) =
                    N'Survival party: Registration failed (' + CONVERT(NVARCHAR(10), ERROR_NUMBER()) + N').';
                INSERT INTO [KMT].[Command_FilterQueue](CommandID, Data1, Data2, Data3, Status)
                VALUES (28, CONVERT(NVARCHAR(50), @CharID), @MsgSVP, N'1', 1);
            END CATCH;
            RETURN;
        END
        ELSE
        BEGIN
            INSERT INTO [KMT].[Command_FilterQueue](CommandID, Data1, Data2, Data3, Status)
            VALUES (28, CONVERT(NVARCHAR(50), @CharID),
                    N'Survival party: Registration is temporarily unavailable.', N'1', 1);
            RETURN;
        END
    END

    
    -------------------------------------------------------------------------
    -- ... (Ø§Ù„Ø£Ø¬Ø²Ø§Ø¡ Ø§Ù„Ù‚Ø¯ÙŠÙ…Ø©: Lottery / BeastFury / LMS / Survival Solo / Survival Party)
    -- Ø§Ø¹ØªØ¨Ø± Ø¥Ù† Ø§Ù„Ø¨Ù„ÙˆÙƒØ§Øª Ø§Ù„Ø³Ø§Ø¨Ù‚Ø© Ù…ÙˆØ¬ÙˆØ¯Ø© ÙƒÙ…Ø§ Ø¹Ù†Ø¯ÙƒØŒ Ù‡Ù†Ø¶ÙŠÙ ÙÙŠ Ø§Ù„Ø¢Ø®Ø± Tower Defender ÙÙ‚Ø·.
    -------------------------------------------------------------------------

    -------------------------------------------------------------------------
    -- Tower Defender (ID=13)
    -------------------------------------------------------------------------
    IF (@EventID = 13)
    BEGIN
        DECLARE @CharName64_TD NVARCHAR(64) = CONVERT(NVARCHAR(64), @CharName16);

        IF OBJECT_ID(N'[Events].[dbo].[_TowerDefender_Regist]', N'P') IS NOT NULL
        BEGIN
            BEGIN TRY
                EXEC [Events].[dbo].[_TowerDefender_Regist]
                     @CharID   = @CharID,
                     @CharName = @CharName64_TD;
            END TRY
            BEGIN CATCH
                DECLARE @MsgTD NVARCHAR(MAX) =
                    N'Tower Defender: Registration failed (' + CONVERT(NVARCHAR(10), ERROR_NUMBER()) + N').';
                INSERT INTO [KMT].[Command_FilterQueue](CommandID, Data1, Data2, Data3, Status)
                VALUES (28, CONVERT(NVARCHAR(50), @CharID), @MsgTD, N'1', 1);
            END CATCH;
            RETURN;
        END
        ELSE
        BEGIN
            INSERT INTO [KMT].[Command_FilterQueue](CommandID, Data1, Data2, Data3, Status)
            VALUES (28, CONVERT(NVARCHAR(50), @CharID),
                    N'Tower Defender: Registration is temporarily unavailable.', N'1', 1);
            RETURN;
        END
    END
END

GO

-- ========================================
-- KMT.Hook_GameServerStart
-- ========================================
CREATE   PROCEDURE [KMT].[Hook_GameServerStart]
	@GameServerExeName varchar(128)
AS
INSERT INTO [KMT].[Command_FilterQueue](CommandID, Status) VALUES(1000, 1)

/*
	ProsedÃ¼r, Ã§oklu gameserver kullanÄ±yorsanÄ±z her gameserver tarafÄ±ndan Ã§aÄŸÄ±rÄ±lacaktÄ±r.
	Sadece bir gameserverdan gelen veriyle iÅŸlem yapabilmek iÃ§in @GameServerExeName parametresini kullanarak filtreleme yapabilirsiniz.

	Ã–rnek:

	IF(@GameServerExeName != 'SR_GameServer_1')
		return;
*/

/*
	If you are using multiple gameservers, procedure will be called from every single one of them.
	To process the data only once, you can use the @GameServerExename parameter to filter the data.

	Example:

	IF(@GameServerExeName != 'SR_GameServer_1')
		return;
*/
GO

-- ========================================
-- KMT.Hook_ItemMallBuy
-- ========================================

CREATE   PROCEDURE [KMT].[Hook_ItemMallBuy]
    @JID int,
    @CharID int,
    @CharName16 varchar(16),
    @ItemID int,
    @Silk int
AS
BEGIN
    SET NOCOUNT ON;
    
    IF(@ItemID BETWEEN 45837 AND 45844)
        RETURN;

    EXEC [KMT].[Achievement_Update] @CharID, 9, 9, @Silk;

    -- Example Silk Rank
    IF NOT EXISTS (SELECT 1 FROM [KMT].[Rank_Silk] with (nolock) WHERE CharID = @CharID)
    BEGIN
        -- Silk Rank'da icon gÃ¶stermek istiyorsanÄ±z _RefMediaIconPath'daki karÅŸÄ±lÄ±ÄŸÄ±nÄ± Silk Rank'a ekleyebilirsiniz.
        INSERT INTO [KMT].[Rank_Silk] (JID, CharID, SilkHistory, SilkRank) VALUES (@JID, @CharID, @Silk, 6);
    END
    ELSE
    BEGIN
        UPDATE [KMT].[Rank_Silk] 
        SET SilkHistory = SilkHistory + @Silk 
        WHERE CharID = @CharID; -- update 
    END

    DECLARE @SilkHistory int;
    SELECT @SilkHistory = SilkHistory FROM [KMT].[Rank_Silk] WHERE CharID = @CharID;

    -- Rank gÃ¼ncellemeleri
    IF (@SilkHistory < 100)
    BEGIN
	    UPDATE [KMT].[Rank_Silk] 
        SET SilkRank  = 6
        WHERE CharID = @CharID; -- update 

        INSERT INTO [KMT].[Command_FilterQueue](CommandID, Data1, Data2, Data3, Data4, Status) 
        VALUES(27, @CharID, @JID, @Silk, 6, 1);
        EXEC [KMT].[Style_UpdateRightIcon] @CharName16, 6; -- iron
    END

    ELSE IF (@SilkHistory >= 100 AND @SilkHistory < 300)
    BEGIN
	    UPDATE [KMT].[Rank_Silk] 
        SET SilkRank  = 5
        WHERE CharID = @CharID; -- update 

        INSERT INTO [KMT].[Command_FilterQueue](CommandID, Data1, Data2, Data3, Data4, Status) 
        VALUES(27, @CharID, @JID, @Silk, 5, 1);
        EXEC [KMT].[Style_UpdateRightIcon] @CharName16, 5; -- bronze
    END

    ELSE IF (@SilkHistory >= 300 AND @SilkHistory < 1000)
    BEGIN
	    UPDATE [KMT].[Rank_Silk] 
        SET SilkRank  = 4
        WHERE CharID = @CharID; -- update 
        INSERT INTO [KMT].[Command_FilterQueue](CommandID, Data1, Data2, Data3, Data4, Status) 
        VALUES(27, @CharID, @JID, @Silk, 4, 1);
        EXEC [KMT].[Style_UpdateRightIcon] @CharName16, 4; -- silver
    END
    ELSE IF (@SilkHistory >= 1000 AND @SilkHistory < 5000)
    BEGIN
	    UPDATE [KMT].[Rank_Silk] 
        SET SilkRank  = 3
        WHERE CharID = @CharID; -- update 
        INSERT INTO [KMT].[Command_FilterQueue](CommandID, Data1, Data2, Data3, Data4, Status) 
        VALUES(27, @CharID, @JID, @Silk, 3, 1);
        EXEC [KMT].[Style_UpdateRightIcon] @CharName16, 3; --- gold
    END
    ELSE IF (@SilkHistory >= 5000 AND @SilkHistory < 7500)
    BEGIN
	    UPDATE [KMT].[Rank_Silk] 
        SET SilkRank  = 2
        WHERE CharID = @CharID; -- update 
        INSERT INTO [KMT].[Command_FilterQueue](CommandID, Data1, Data2, Data3, Data4, Status) 
        VALUES(27, @CharID, @JID, @Silk, 2, 2);
        EXEC [KMT].[Style_UpdateRightIcon] @CharName16, 2; --- platinum
    END
    ELSE IF (@SilkHistory >= 7500)
    BEGIN
	    UPDATE [KMT].[Rank_Silk] 
        SET SilkRank  = 1
        WHERE CharID = @CharID; -- update 
        INSERT INTO [KMT].[Command_FilterQueue](CommandID, Data1, Data2, Data3, Data4, Status) 
        VALUES(27, @CharID, @JID, @Silk, 1, 1);
        EXEC [KMT].[Style_UpdateRightIcon] @CharName16, 1; --- vip
    END
  
END

GO

-- ========================================
-- KMT.Hook_NPCBuy
-- ========================================
-- =============================================
-- Author:		<Author,,Name>
-- Create date: <Create Date,,>
-- Description:	<Description,,>
-- =============================================
CREATE   PROCEDURE [KMT].[Hook_NPCBuy]
	-- Add the parameters for the stored procedure here
	@CharID INT,
	@Slot_From tinyint,
	@BuyedItemID int

	AS
BEGIN
	-- SET NOCOUNT ON added to prevent extra result sets from
	-- interfering with SELECT statements.
	SET NOCOUNT ON;
	
	DECLARE @CodeName128 varchar (128);
	select @CodeName128=CodeName128 from SRO_VT_SHARD.._RefObjCommon
	where ID = @BuyedItemID
	

	declare @RefPackageItemCodeName varchar(128);
	select top (1) @RefPackageItemCodeName = RefPackageItemCodeName from SRO_VT_SHARD.._RefShopGoods with (nolock) where RefPackageItemCodeName = 'PACKAGE_'  +  @CodeName128 and SlotIndex = @Slot_From
	
    DECLARE @TotalPrice bigint;
    SELECT TOP (1) @TotalPrice = Cost FROM SRO_VT_SHARD.._RefPricePolicyOfItem WITH (NOLOCK) WHERE RefPackageItemCodeName = @RefPackageItemCodeName AND PaymentDevice = 1;

	EXEC [KMT].[Achievement_Update] @CharID, 8, 8, @TotalPrice
END


GO

-- ========================================
-- KMT.Hook_PartyJoin
-- ========================================
CREATE   PROCEDURE [KMT].[Hook_PartyJoin]
	@CharID int,
	@CharName varchar(25),
	@CurrentRegionId smallint,
	@CurrentWorldId int,
	@PVPCapeType tinyint,
	@CurrentJobType tinyint
AS


	/*
		Bu prosedÃ¼rÃ¼ dÃ¼zenleyerek bir karakter partiye katÄ±ldÄ±ÄŸÄ±nda bir iÅŸlem yapabilirsiniz.
		- Partiyi kuran karakter iÃ§in bu prosedÃ¼r Ã§alÄ±ÅŸmayacaktÄ±r.

		@CharID = Partiye katÄ±lan karakterin IDsi.
		@CharName = Partiye katÄ±lan karakterin ismi.
		@RegionID = Partiye katÄ±lan karakterin anlÄ±k Region IDsi.
		@WorldID = Partiye katÄ±lan karakterin anlÄ±k World IDsi.
		@PVPCapeType = Pvp cape
		@JobType = Job bilgisi
		Bu veritabanÄ±ndaki diÄŸer prosedÃ¼rlerin aksine bu prosedÃ¼r her restartta orjinal haline dÃ¶ndÃ¼rÃ¼lmeyecektir. Dikkatli dÃ¼zenleyiniz!
	*/

	/*
		You may modify this procedure to take some action once a character has joined a party.

		@CharID = ID of the Character.
		@CharName = Name of the Character.
		@RegionID = Live Region ID of the Character.
		@WorldID = Live World ID of the Character.

		Unlike other Stored Procedures in this database, this stored procedure will not be restored back to default on restarts. Modify it carefully!
	*/
GO

-- ========================================
-- KMT.Hook_PartyMatchCreate
-- ========================================

CREATE   PROCEDURE [KMT].[Hook_PartyMatchCreate]
    @CharID           INT,
    @CharName         VARCHAR(25),
    @CurrentRegionId  SMALLINT,
    @CurrentWorldID   INT,
    @PartyNo          INT
AS
BEGIN
    SET NOCOUNT ON;

    BEGIN TRY
        -- Ù„Ùˆ Ø¹Ø§ÙŠØ² ØªÙ…Ù†Ø¹ ØªÙƒØ±Ø§Ø± Ù†ÙØ³ PartyNoØŒ ÙÙƒ Ø§Ù„ÙƒÙˆÙ…Ù†ØªÙŠÙ† Ø¯ÙˆÙ„:
        --IF EXISTS (SELECT 1 FROM dbo.PartyMatchingCreated WITH (NOLOCK) WHERE PartyNo = @PartyNo)
        --    RETURN;

        INSERT INTO [KMT].[Party_MatchingLog] (CharID, CharName, RegionID, WorldID, PartyNo)
        VALUES (@CharID, @CharName, @CurrentRegionId, @CurrentWorldID, @PartyNo);
    END TRY
    BEGIN CATCH
        PRINT '[_OnPartyMatchingCreated_EDIT] Error: '
              + ERROR_MESSAGE() + ' (Line ' + CAST(ERROR_LINE() AS VARCHAR(10)) + ')';
    END CATCH
END

GO

-- ========================================
-- KMT.Hook_SelectScroll
-- ========================================
CREATE   PROCEDURE [KMT].[Hook_SelectScroll] 
	@CharName16 varchar(128),
	@CharID int,
	@UsedItemSlot tinyint,
	@TargetItemSlot tinyint
as
	BEGIN
	SET NOCOUNT ON;
/*
	TypeID1 = 3, TypeID2 = 3, TypeID3 = 13, TypeID4 = 11 olan item kullanÄ±ldÄ±ÄŸÄ±nda bu prosedÃ¼rÃ¼ Ã§alÄ±ÅŸtÄ±racaktir. -- bu item her 5 saniyede bir kullanÄ±labilir.

	KullanÄ±lan itemin silinmesi iÃ§in __LiveItemRemove prosedÃ¼rÃ¼ kullanÄ±lÄ±r.
	Hedef itemin deÄŸiÅŸtirilmesi iÃ§in __LiveMutateItem prosedÃ¼rÃ¼ kullanÄ±lÄ±r.
	Model Switcher, Glow Switcher ve Upgrade kullanÄ±mÄ± iÃ§in uygundur
	
*/
---EXAMPLE
DECLARE @UsedItemID64 bigint = (SELECT ItemID FROM SRO_VT_SHARD.._Inventory with(nolock) where CharID = @CharID and Slot = @UsedItemSlot)

DECLARE @TargetItemID64 bigint = (SELECT ItemID FROM SRO_VT_SHARD.._Inventory with(nolock) where CharID = @CharID and Slot = @TargetItemSlot)


if(@UsedItemID64 > 0 and @TargetItemID64 > 0)
BEGIN
DECLARE @UsedItemID int = (Select RefItemID FROM SRO_VT_SHARD.._Items with(nolock) where ID64 = @UsedItemID64)
DECLARE @TargetItemID int = (Select RefItemID FROM SRO_VT_SHARD.._Items with(nolock) where ID64 = @TargetItemID64)

DECLARE @TargetItemCodeName varchar(500) = (Select CodeName128 FROM SRO_VT_SHARD.._RefObjCommon with(nolock) where ID = @TargetItemID)

IF(@UsedItemID = 46324) -- GLOW 1
BEGIN

IF @TargetItemCodeName LIKE '%_08_C_RARE%' OR @TargetItemCodeName LIKE '%_08_B_RARE%'
BEGIN
    EXEC [KMT].[Style_ChangeGlow] @CharName16, @CharID, @TargetItemCodeName, 'GLOW_1', @TargetItemSlot, @UsedItemSlot
END
END
IF(@UsedItemID = 46325) -- GLOW 2
BEGIN

IF @TargetItemCodeName LIKE '%_08_C_RARE%' OR @TargetItemCodeName LIKE '%_08_B_RARE%'
BEGIN
    EXEC [KMT].[Style_ChangeGlow] @CharName16, @CharID, @TargetItemCodeName, 'GLOW_2', @TargetItemSlot, @UsedItemSlot
END

END
IF(@UsedItemID = 46326) -- GLOW 3
BEGIN

IF @TargetItemCodeName LIKE '%_08_C_RARE%' OR @TargetItemCodeName LIKE '%_08_B_RARE%'
BEGIN
    EXEC [KMT].[Style_ChangeGlow] @CharName16, @CharID, @TargetItemCodeName, 'GLOW_3', @TargetItemSlot, @UsedItemSlot
END

END
IF(@UsedItemID = 46327) -- GLOW 4
BEGIN

IF @TargetItemCodeName LIKE '%_08_C_RARE%' OR @TargetItemCodeName LIKE '%_08_B_RARE%'
BEGIN
    EXEC [KMT].[Style_ChangeGlow] @CharName16, @CharID, @TargetItemCodeName, 'GLOW_4', @TargetItemSlot, @UsedItemSlot
END

END

IF(@UsedItemID = 46328) -- MODEL 09
BEGIN

IF @TargetItemCodeName LIKE '%_08_C_RARE%' OR @TargetItemCodeName LIKE '%_08_B_RARE%'
BEGIN
    EXEC [KMT].[Style_ChangeModel] @CharName16, @CharID, @TargetItemCodeName, 'MODEL09', @TargetItemSlot, @UsedItemSlot
END

END

IF(@UsedItemID = 46329) -- MODEL 09
BEGIN

IF @TargetItemCodeName LIKE '%_08_C_RARE%' OR @TargetItemCodeName LIKE '%_08_B_RARE%'
BEGIN
    EXEC [KMT].[Style_ChangeModel] @CharName16, @CharID, @TargetItemCodeName, 'MODEL10', @TargetItemSlot, @UsedItemSlot
END

END

IF(@UsedItemID = 46330) -- MODEL 09
BEGIN

IF @TargetItemCodeName LIKE '%_08_C_RARE%' OR @TargetItemCodeName LIKE '%_08_B_RARE%'
BEGIN
    EXEC [KMT].[Style_ChangeModel] @CharName16, @CharID, @TargetItemCodeName, 'MODEL11', @TargetItemSlot, @UsedItemSlot
END

END

IF(@UsedItemID = 46331) -- MODEL 09
BEGIN

IF @TargetItemCodeName LIKE '%_08_C_RARE%' OR @TargetItemCodeName LIKE '%_08_B_RARE%'
BEGIN
    EXEC [KMT].[Style_ChangeModel] @CharName16, @CharID, @TargetItemCodeName, 'MODEL11_A', @TargetItemSlot, @UsedItemSlot
END

END

IF(@UsedItemID = 46332) -- MODEL 09
BEGIN

IF @TargetItemCodeName LIKE '%_08_C_RARE%' OR @TargetItemCodeName LIKE '%_08_B_RARE%'
BEGIN
    EXEC [KMT].[Style_ChangeModel] @CharName16, @CharID, @TargetItemCodeName, 'MODEL11_B', @TargetItemSlot, @UsedItemSlot
END

END

IF(@UsedItemID = 46333) -- MODEL 09
BEGIN

IF @TargetItemCodeName LIKE '%_08_C_RARE%' OR @TargetItemCodeName LIKE '%_08_B_RARE%'
BEGIN
    EXEC [KMT].[Style_ChangeModel] @CharName16, @CharID, @TargetItemCodeName, 'MODEL12', @TargetItemSlot, @UsedItemSlot
END

END


IF(@UsedItemID = 46334) -- MODEL 09
BEGIN

IF @TargetItemCodeName LIKE '%_08_C_RARE%' OR @TargetItemCodeName LIKE '%_08_B_RARE%'
BEGIN
    EXEC [KMT].[Style_ChangeModel] @CharName16, @CharID, @TargetItemCodeName, 'MODEL13', @TargetItemSlot, @UsedItemSlot
END

END

END

end
GO

-- ========================================
-- KMT.Hook_StallCreate
-- ========================================

CREATE   PROCEDURE [KMT].[Hook_StallCreate]
    @CharID       INT,
    @CharName     NVARCHAR(64),
    @UniqueCharID BIGINT,
    @RegionID     SMALLINT,
    @WorldID      INT,
    @StallTitle   NVARCHAR(128)
AS
BEGIN
    SET NOCOUNT ON;

    INSERT INTO [KMT].[Stall_CreateLog]
        (CharID, CharName, UniqueCharID, RegionID, WorldID, StallTitle)
    VALUES
        (@CharID, @CharName, @UniqueCharID, @RegionID, @WorldID, @StallTitle);
END

GO

-- ========================================
-- KMT.Hook_Teleport
-- ========================================
CREATE   PROCEDURE [KMT].[Hook_Teleport]
    @CharID INT,
    @RefTeleportID INT,
    @CanTeleport BIT OUTPUT
AS
BEGIN
    SET NOCOUNT ON;

    -- Ø§ÙØªØ±Ø§Ø¶ÙŠ ÙŠØ®Ù„ÙŠ Ø§Ù„ØªÙŠÙ„Ø¨ÙˆØ±Øª Ø´ØºØ§Ù„
    SET @CanTeleport = 1;

    -- Ù„Ùˆ Ø§Ù„ØªÙŠÙ„Ø¨ÙˆØ±Øª Ù‡Ùˆ Ø±Ù‚Ù… 1 ÙŠÙ‚ÙÙ„Ù‡
    IF (@RefTeleportID = 323232)
        SET @CanTeleport = 0;
END

GO

-- ========================================
-- KMT.Hook_UniqueKill
-- ========================================

CREATE   PROCEDURE [KMT].[Hook_UniqueKill]
    @RefObjID       INT,
    @KillerCharName VARCHAR(16)
AS
BEGIN
    SET NOCOUNT ON;

    DECLARE @CharID INT =
    (
        SELECT CharID
        FROM SRO_VT_SHARD.._Char
        WHERE CharName16 = @KillerCharName
    );
    IF @CharID IS NULL RETURN;

    -------------------------------------------------------------------------
    -- CallsÙƒ Ø§Ù„Ù‚Ø¯ÙŠÙ…Ø© (Rank, Honor, Guild name sync ... Ø¥Ù„Ø®) Ø³ÙŠØ¨Ù‡Ø§ ÙƒÙ…Ø§ Ù‡ÙŠ
    -------------------------------------------------------------------------
    EXEC hema_system.dbo.Unique_Rank
         @RefObjID   = @RefObjID,
         @KillerName = @KillerCharName;

    EXEC hema_system.dbo.HonorRank_Uniques
         @RefObjID   = @RefObjID,
         @KillerName = @KillerCharName;

    -- ... Ø¨Ø§Ù‚ÙŠ Ø§Ù„ØªØ¹Ø¯ÙŠÙ„Ø§Øª Ø§Ù„Ù„ÙŠ Ø¹Ù†Ø¯Ùƒ (GuildName sync + Achievements ...)
    EXEC [KMT].[Achievement_UniqueKill]
         @RefObjID   = @RefObjID,
         @KillerrName= @KillerCharName;

    INSERT INTO [KMT].[Command_FilterQueue](CommandID, Data1, Data2, Status)
    VALUES (40, 40, 40, 1);

    -------------------------------------------------------------------------
    -- Tower Defender â€“ Tower kill detection
    -- RedTowerRefObjID / BlueTowerRefObjID Ù„Ø§Ø²Ù… ØªÙƒÙˆÙ† Ù…Ø·Ø§Ø¨Ù‚Ø© Ù„Ù„Ø£ÙŠÙÙ†Øª
    -------------------------------------------------------------------------
    DECLARE
          @RedTowerRefObjID  INT = 175001515
        , @BlueTowerRefObjID INT = 175001514;

    IF (@RefObjID IN (@RedTowerRefObjID, @BlueTowerRefObjID))
    BEGIN
        DECLARE
              @nowUtc13    DATETIME2(3) = SYSUTCDATETIME()
            , @KillerTeam  TINYINT      = NULL
            , @WinningTeam TINYINT      = NULL
            , @WinReason   NVARCHAR(32) = NULL;

        -- ØªØ­Ø¯ÙŠØ¯ ØªÙŠÙ… Ø§Ù„Ù‚Ø§ØªÙ„
        SELECT @KillerTeam = cet.Team
        FROM [KMT].[Event_CurrentTeams] AS cet WITH (NOLOCK)
        WHERE cet.CharName = @KillerCharName
          AND cet.EventName = 'Tower Defender';

        -- Ù„Ùˆ ÙŠØ³Ø§Ø± Tower Blue Ù…Ø§Øª â†’ Red Team (1) ÙƒØ³Ø¨Øª
        IF (@RefObjID = @BlueTowerRefObjID)
        BEGIN
            SET @WinningTeam = 1;      -- Red
            SET @WinReason   = N'BlueTowerDown';
        END
        -- Ù„Ùˆ Tower Red Ù…Ø§Øª â†’ Blue Team (3) ÙƒØ³Ø¨Øª
        ELSE IF (@RefObjID = @RedTowerRefObjID)
        BEGIN
            SET @WinningTeam = 3;      -- Blue
            SET @WinReason   = N'RedTowerDown';
        END

        IF (@WinningTeam IN (1,3))
        BEGIN
            UPDATE Events.dbo.TowerDefender_Runtime
            SET WinningTeam   = @WinningTeam,
                WinReason     = @WinReason,
                LastUpdateUtc = @nowUtc13
            WHERE SingletonID = 1
              AND RegStartUtc IS NOT NULL;

            -- Ø±Ø³Ø§Ù„Ø© Ø¹Ù„Ù‰ Ø§Ù„Ø³ÙŠØ±ÙØ± Ø¥Ù† Ø§Ù„ØªØ§ÙˆØ± ÙˆÙ‚Ø¹
            DECLARE @loserName NVARCHAR(16) =
                    CASE WHEN @WinningTeam = 1 THEN N'Blue' ELSE N'Red' END;
            DECLARE @winnerName NVARCHAR(16) =
                    CASE WHEN @WinningTeam = 1 THEN N'Red'  ELSE N'Blue' END;

            DECLARE @towerMsg NVARCHAR(MAX) =
                N'[ Tower Defender ] ' + @KillerCharName +
                N' has destroyed the ' + @loserName + N' Tower! ' +
                @winnerName + N' Team is now dominating the battlefield.';

            EXEC [KMT].[Command_NoticeAll]
                 @NoticeType = 2,
                 @Notice     = @towerMsg;
        END
    END
END

GO

-- ========================================
-- KMT.Hook_UniqueSpawn
-- ========================================

CREATE   PROCEDURE [KMT].[Hook_UniqueSpawn]
    @RefObjID INT
AS
BEGIN
    SET NOCOUNT ON;

    ----------------------------------------------------
    -- Ø§Ù„Ø¬Ø²Ø¡ Ø§Ù„Ø£ØµÙ„ÙŠ Ø§Ù„Ø®Ø§Øµ Ø¨Ù€ KMTGuard (Ø³ÙŠØ¨Ù‡ Ø²ÙŠ Ù…Ø§ Ù‡Ùˆ)
    ----------------------------------------------------
    INSERT INTO [KMT].[Command_FilterQueue] (CommandID, Data1, Status)
    VALUES (9999, @RefObjID, 1);

    ----------------------------------------------------
    -- Ø±Ø¨Ø· Ø§Ù„Ù€ Unique Ø¨Ø¨ÙˆØª Ø§Ù„Ø¯ÙŠØ³ÙƒÙˆØ±Ø¯ (Ø±Ø³Ø§Ù„Ø© Ù„Ù„Ø§Ø¹Ø¨ÙŠÙ†)
    ----------------------------------------------------
    DECLARE @CodeName    VARCHAR(128);
    DECLARE @DisplayName NVARCHAR(128);
    DECLARE @Msg         NVARCHAR(MAX);

    -- Ù†Ø¬ÙŠØ¨ CodeName Ù…Ù† DB Ø§Ù„Ù„Ø¹Ø¨Ø© + DisplayName Ù…Ù† Ø¬Ø¯ÙˆÙ„ UniqueNames Ù„Ùˆ Ù…ÙˆØ¬ÙˆØ¯
    SELECT TOP 1
        @CodeName    = RC.CodeName128,
        @DisplayName = UN.DisplayName
    FROM SRO_VT_SHARD.dbo._RefObjCommon RC WITH (NOLOCK)
    LEFT JOIN casy_discord.dbo.UniqueNames UN
        ON UN.CodeName = RC.CodeName128
    WHERE RC.ID = @RefObjID;

    -- Ù„Ùˆ Ù„Ø£ÙŠ Ø³Ø¨Ø¨ Ø§Ù„ÙƒÙˆØ¯ Ù…Ø´ Ù…ÙˆØ¬ÙˆØ¯
    IF (@CodeName IS NULL)
        SET @CodeName = 'UNKNOWN_UNIQUE';

    -- Ù„Ùˆ Ù…ÙÙŠØ´ DisplayName ÙÙŠ Ø§Ù„Ø¬Ø¯ÙˆÙ„ØŒ Ø®Ù„ÙŠÙ‡Ø§ Ù†ÙØ³ Ø§Ù„ÙƒÙˆØ¯ (Ø¨Ø³ Ø§Ù„Ù…ÙØ±ÙˆØ¶ Ø¹Ù†Ø¯Ùƒ Ø§Ù„ÙƒÙ„)
    IF (@DisplayName IS NULL)
        SET @DisplayName = @CodeName;

    ----------------------------------------------------
    -- Ù‡Ù†Ø§ Ø´ÙƒÙ„ Ø§Ù„Ø±Ø³Ø§Ù„Ø© Ø§Ù„Ù„ÙŠ Ù‡ØªØ¸Ù‡Ø± ÙÙŠ Ø§Ù„Ø¯ÙŠØ³ÙƒÙˆØ±Ø¯
    ----------------------------------------------------
    SET @Msg =
        N'ðŸ§¿ **' + @DisplayName + N'** has spawned!' + CHAR(13) + CHAR(10) +
        N'Get ready, hunters!';

    -- Ù„Ùˆ Ø­Ø§Ø¨Ø¨ Øªmention Ø±ÙˆÙ„ Ù…Ø¹ÙŠÙ†Ø© (Ù…Ø«Ø§Ù„):
    -- SET @Msg = N'<@&ROLE_ID_HERE> ' + @Msg;

    ----------------------------------------------------
    -- Ø¥Ø¯Ø®Ø§Ù„ Ø§Ù„Ø±Ø³Ø§Ù„Ø© ÙÙŠ Ø¬Ø¯ÙˆÙ„ Messages
    ----------------------------------------------------
    INSERT INTO casy_discord.dbo.Messages (ChannelId, Message)
    VALUES ('1074805202059804782', @Msg);
END

GO

-- ========================================
-- KMT.Item_AddChest
-- ========================================

CREATE   PROCEDURE [KMT].[Item_AddChest]
    @CharID        INT,
    @ItemRefObjID  INT,            -- <-- call by Item ID (RefObjCommon.ID)
    @Quantity      INT,
    @From          VARCHAR(100),
    @Plus          INT
AS
BEGIN
    SET NOCOUNT ON;
    SET XACT_ABORT ON;

    DECLARE
          @ItemCodeName   VARCHAR(128) = NULL
        , @TypeID1        TINYINT      = 0
        , @NewID          INT
        , @FormattedDate  VARCHAR(10);

    -- Lookup the item by ID (instead of CodeName)
    SELECT
          @ItemCodeName = ROC.CodeName128
        , @TypeID1      = ROC.TypeID1
    FROM SRO_VT_SHARD.dbo._RefObjCommon AS ROC WITH (NOLOCK)
    WHERE ROC.ID = @ItemRefObjID;

    IF @ItemCodeName IS NULL
        RETURN -1;      -- unknown item id

    IF @TypeID1 <> 3
        RETURN -2;      -- not an item

    -- dd/MM/yyyy (103)
    SET @FormattedDate = CONVERT(VARCHAR(10), GETDATE(), 103);

    -- Insert into chest (same column order as your original code)
    INSERT INTO [KMT].[Item_Chest]
    VALUES (@CharID, @ItemCodeName, @ItemRefObjID, @Quantity, @FormattedDate, @From, @Plus);

    SET @NewID = SCOPE_IDENTITY();

    -- Async command for the filter (unchanged)
    INSERT INTO [KMT].[Command_FilterQueue]
        (CommandID, Data1,  Data2,   Data3,         Data4,           Data5,      Data6,           Data7,  Data8, Status)
    VALUES
        (17,        @NewID, @CharID, @ItemCodeName, @ItemRefObjID,   @Quantity,  @FormattedDate,  @From,  @Plus,  1);
END

GO

-- ========================================
-- KMT.Item_AddChestOld
-- ========================================
CREATE   PROCEDURE [KMT].[Item_AddChestOld]
    @CharID INT,
    @ItemID INT,
    @Quantity INT,
    @Type VARCHAR(100),
    @Plus INT
AS
BEGIN
    SET NOCOUNT ON;
    EXEC [KMT].[Item_AddChest]
        @CharID = @CharID,
        @ItemRefObjID = @ItemID,
        @Quantity = @Quantity,
        @From = @Type,
        @Plus = @Plus;
END;
GO

-- ========================================
-- KMT.Item_GetInfo
-- ========================================
-- =============================================
-- Author:		<Author,,Name>
-- Create date: <Create Date,,>
-- Description:	<Description,,>
-- =============================================
CREATE   PROCEDURE [KMT].[Item_GetInfo]
	-- Add the parameters for the stored procedure here
	@CharID INT,
	@SlotIndex TINYINT,

	@RefItemID INT OUTPUT,
	@OptLevel TINYINT OUTPUT,
	@CodeName VARCHAR(128) OUTPUT,
	@ItemDBID INT OUTPUT,
	@AdvOptLevel TINYINT OUTPUT
AS
BEGIN
	-- SET NOCOUNT ON added to prevent extra result sets from
	-- interfering with SELECT statements.
	SET NOCOUNT ON;

	SET NOCOUNT ON;

	DECLARE @InvItemID BIGINT = 0
	SET @InvItemID = (SELECT ItemID FROM SRO_VT_SHARD.dbo._Inventory WHERE CharID = @CharID AND Slot = @SlotIndex)

	SELECT 
	@RefItemID =	ISNULL(RefItemID, 0),
	@OptLevel =	ISNULL(OptLevel, 0),
	@ItemDBID = ISNULL(ID64, 0)

	FROM SRO_VT_SHARD.dbo._Items WHERE ID64 = @InvItemID

	SET @AdvOptLevel = ISNULL((SELECT nOptValue FROM SRO_VT_SHARD.dbo._BindingOptionWithItem 
		WHERE bOptType = 2 
		AND (nOptID = 25873 OR nOptID = 25969)
		AND nItemDBID = @ItemDBID), 0)


	SELECT @CodeName = CodeName128 FROM SRO_VT_SHARD.dbo._RefObjCommon WHERE ID=@RefItemID
END


GO

-- ========================================
-- KMT.Live_AddBuff
-- ========================================
CREATE   PROCEDURE [KMT].[Live_AddBuff]
	@CharID int,
	@SkillCodeName varchar(200)
AS		

INSERT INTO [KMT].[Command_GameServerQueue](Action_ID, Data1, Data2) VALUES(8, @CharID, @SkillCodeName)


GO

-- ========================================
-- KMT.Live_AddBuffNoLimit
-- ========================================
CREATE   PROCEDURE [KMT].[Live_AddBuffNoLimit]
	@CharID int,
	@SkillID int
AS		

INSERT INTO [KMT].[Command_GameServerQueue](Action_ID, Data1, Data2) VALUES(6, @CharID, @SkillID)


GO

-- ========================================
-- KMT.Live_Cape
-- ========================================

CREATE   PROCEDURE [KMT].[Live_Cape]
    @CharID INT,
    @CapeID INT
AS
BEGIN
    SET NOCOUNT ON;

    INSERT INTO [KMT].[Command_GameServerQueue] (Action_ID, Data1, Data2)
    VALUES (13, @CharID, @CapeID);
END

GO

-- ========================================
-- KMT.Live_ChangeItem
-- ========================================
CREATE   PROCEDURE [KMT].[Live_ChangeItem]
    @CharID INT,
	@Slot tinyint,
    @CodeName128 VARCHAR(128)
AS
BEGIN
    INSERT INTO [KMT].[Command_GameServerQueue](Action_ID, Data1, Data2, Data3) VALUES(17, @CharID, @Slot, @CodeName128)
END

GO

-- ========================================
-- KMT.Live_Gold
-- ========================================

CREATE   PROCEDURE [KMT].[Live_Gold]
    @CharID int,
    @Gold bigint,
    @AddOrRemove int
AS
BEGIN
    SET NOCOUNT ON;

    INSERT INTO [KMT].[Command_GameServerQueue](Action_ID, Data1, Data2, Data3)
    VALUES(21, @CharID, @Gold, @AddOrRemove);

    -- maximum 2b
END

GO

-- ========================================
-- KMT.Live_RemoveBuff
-- ========================================
CREATE   PROCEDURE [KMT].[Live_RemoveBuff]
		@CharID int,
	@SkillID int
AS		

INSERT INTO [KMT].[Command_GameServerQueue](Action_ID, Data1, Data2) VALUES(7, @CharID, @SkillID)

GO

-- ========================================
-- KMT.Live_Silk
-- ========================================
CREATE   PROCEDURE [KMT].[Live_Silk]
    @CharID INT,
	@nSilk int,
	@nSilkGift int,
	@nSilkPoint int
AS
    SET NOCOUNT ON;

    -- KullanÄ±cÄ±yÄ± bul
    DECLARE @JID INT = (SELECT UserJID FROM SRO_VT_SHARD.._User WITH (NOLOCK) WHERE CharID = @CharID);

    -- EÄŸer @JID geÃ§erli bir deÄŸer ise iÅŸlemlere devam et
    IF (@JID > 0)
    BEGIN
        IF EXISTS (SELECT 1 FROM SRO_VT_ACCOUNT..SK_Silk WHERE JID = @JID)
        BEGIN
            -- SK_Silk tablosundaki mevcut veriyi deÄŸiÅŸkenlere Ã§ek
            DECLARE @CurrentSilkOwn INT, @CurrentSilkGift INT, @CurrentSilkPoint INT;

            SELECT 
                @CurrentSilkOwn = silk_own,
                @CurrentSilkGift = silk_gift,
                @CurrentSilkPoint = silk_point
            FROM SRO_VT_ACCOUNT..SK_Silk 
            WHERE JID = @JID;

            -- GÃ¼ncelleme iÅŸlemi
            UPDATE SRO_VT_ACCOUNT..SK_Silk 
            SET 
                silk_own = @CurrentSilkOwn + @nSilk,
                silk_gift = @CurrentSilkGift + @nSilkGift,
                silk_point = @CurrentSilkPoint + @nSilkPoint
            WHERE JID = @JID;

            -- GÃ¼ncellenmiÅŸ deÄŸerleri ekle
            INSERT INTO [KMT].[Command_GameServerQueue](Action_ID, Data1, Data2, Data3, Data4)
            VALUES(20, @CharID, @CurrentSilkOwn + @nSilk, @CurrentSilkGift + @nSilkGift, @CurrentSilkPoint + @nSilkPoint);
        END
        ELSE
        BEGIN
            -- Yeni kayÄ±t ekleme iÅŸlemi
            INSERT INTO SRO_VT_ACCOUNT..SK_Silk (JID, silk_own, silk_gift, silk_point)
            VALUES (@JID, @nSilk, @nSilkGift, @nSilkPoint);

            -- Yeni eklenen verileri [KMT].[Command_GameServerQueue] tablosuna ekle
            INSERT INTO [KMT].[Command_GameServerQueue](Action_ID, Data1, Data2, Data3, Data4)
            VALUES(20, @CharID, @nSilk, @nSilkGift, @nSilkPoint);
        END
    END


GO

-- ========================================
-- KMT.Live_Skill
-- ========================================
CREATE   PROCEDURE [KMT].[Live_Skill]
    @WorldID INT,
	@State bit
AS 
BEGIN
	if(@State = 1)
	INSERT INTO [KMT].[Command_FilterQueue] (CommandID, Data1, Data2, Status) VALUES (20, @WorldID, 'True', 1)
	else
	INSERT INTO [KMT].[Command_FilterQueue] (CommandID, Data1, Data2, Status) VALUES (20, @WorldID, 'False', 1)
END

GO

-- ========================================
-- KMT.Live_Teleport
-- ========================================
CREATE   PROCEDURE [KMT].[Live_Teleport]
    @Gate INT,
	@State bit
AS 
BEGIN
	if(@State = 1)
	INSERT INTO [KMT].[Command_FilterQueue] (CommandID, Data1, Data2, Status) VALUES (18, @Gate, 'True', 1)
	else
	INSERT INTO [KMT].[Command_FilterQueue] (CommandID, Data1, Data2, Status) VALUES (18, @Gate, 'False', 1)
END

GO

-- ========================================
-- KMT.Live_UseAndChangeItem
-- ========================================
CREATE   PROCEDURE [KMT].[Live_UseAndChangeItem]
    @CharID INT,
	@MutateSlot tinyint,
    @CodeName128 VARCHAR(128),
	@ConsumeSlot tinyint,
	@ConsumeAmount int
AS
BEGIN
    INSERT INTO [KMT].[Command_GameServerQueue](Action_ID, Data1, Data2, Data3, Data4, Data5) VALUES(19, @CharID, @MutateSlot, @CodeName128, @ConsumeSlot, @ConsumeAmount)
END

GO

-- ========================================
-- KMT.Live_UseItem
-- ========================================
CREATE   PROCEDURE [KMT].[Live_UseItem]
    @CharID INT,
	@Slot tinyint,
    @ReduceAmount int
AS
BEGIN
    INSERT INTO [KMT].[Command_GameServerQueue](Action_ID, Data1, Data2, Data3) VALUES(18, @CharID, @Slot, @ReduceAmount)
END

GO

-- ========================================
-- KMT.Log_Character
-- ========================================

CREATE   PROCEDURE [KMT].[Log_Character]
@CharID		int,
@EventID	tinyint,
@Data1		int,
@Data2		int,
@strPos		varchar(64),
@Desc		varchar(128)
AS

--return -- -- Put the SRO_VT_LOG AddLogChar -- EXEC LexaShield_User.._ProcessAddLogChar @CharID, @EventID, @Data1, @Data2, @strPos, @Desc
/*
if(@EventID = 9)
begin

--EXEC LexaShield_User.._VIPSystem_EDIT @CharID

/*DECLARE @RefObjID int = (SELECT RefObjID From SRO_VT_SHARD.._Char with(nolock) where CharID = @CharID)
if(@RefObjID < 14875)
begin
EXEC [KMT].[Live_AddBuffNoLimit] @CharID, 35406
end
*/
--EXEC LexaShield_User.._LiveHonorBuff_EDIT @CharID

*/
IF(@EventID = 9)
BEGIN
    DECLARE @SilkRank int = (SELECT SilkRank FROM [KMT].[Rank_Silk] with (nolock) WHERE CharID = @CharID);

    IF(@SilkRank = 1)
    BEGIN
        EXEC [KMT].[Live_AddBuff] @CharID, 'SKILL_VIP_BUFF_01';
		EXEC [KMT].[Command_NoticeByID] @CharID, 8, 'You''re VIP (VIP) User.'
    END
    ELSE IF(@SilkRank = 2)
    BEGIN
        EXEC [KMT].[Live_AddBuff] @CharID, 'SKILL_VIP_BUFF_02';
		EXEC [KMT].[Command_NoticeByID] @CharID, 8, 'You''re VIP (Platinum) User.'
    END
    ELSE IF(@SilkRank = 3)
    BEGIN
        EXEC [KMT].[Live_AddBuff] @CharID, 'SKILL_VIP_BUFF_03';
		EXEC [KMT].[Command_NoticeByID] @CharID, 8, 'You''re VIP (Gold) User.'
    END
    ELSE IF(@SilkRank = 4)
    BEGIN
        EXEC [KMT].[Live_AddBuff] @CharID, 'SKILL_VIP_BUFF_04';
		EXEC [KMT].[Command_NoticeByID] @CharID, 8, 'You''re VIP (Silver) User.'
    END
    ELSE IF(@SilkRank = 5)
    BEGIN
        EXEC [KMT].[Live_AddBuff] @CharID, 'SKILL_VIP_BUFF_05';
		EXEC [KMT].[Command_NoticeByID] @CharID, 8, 'You''re VIP (Bronze) User.'
    END
    ELSE IF(@SilkRank = 6 OR @SilkRank IS NULL)
    BEGIN
        EXEC [KMT].[Live_AddBuff] @CharID, 'SKILL_VIP_BUFF_06';
		EXEC [KMT].[Command_NoticeByID] @CharID, 8, 'You''re New (Iron) User.'
    END
END



if(@EventID = 20 AND @Data2 = 0  and @Desc like '%freebattle%') -- PVP Kill
BEGIN
	DECLARE @WorldID int
	DECLARE @CharnameDead varchar(16)
	DECLARE @CharnameKiller varchar(16)
	
	if (CHARINDEX('His(', @Desc) <= 0) return -- Not killed by player?

	DECLARE @startindex int = CHARINDEX('(', @Desc) + 1
	DECLARE @endindex int = CHARINDEX(')', @Desc) 
	SET @CharnameKiller = (SELECT SUBSTRING(@Desc, @startindex, @endindex - @startindex))
	SET @CharnameDead =  (SELECT CharName16 from SRO_VT_SHARD.dbo._Char WITH (NOLOCK) WHERE CharID = @CharID)



	DECLARE @GuildID int, @KillerCharID int
	SELECT @WorldID = WorldID, @GuildID = GuildID, @KillerCharID = CharID FROM SRO_VT_SHARD.dbo._Char WITH (NOLOCK) WHERE CharName16 = @CharnameKiller 

	IF(@WorldID BETWEEN 2 AND 9) -- Kill In Fortress War // BUT FTW RUNNING? if i want Kill? i not want freebattle amk
	BEGIN
		DECLARE @UnionMasterGuildID int = (SELECT Ally1 FROM SRO_VT_SHARD.dbo._AlliedClans WITH(NOLOCK) WHERE @GuildID IN (Ally1, Ally2, Ally3, Ally4, Ally5, Ally6, Ally7, Ally8))
		DECLARE @GuildName varchar(16), @UnionName varchar(16)

		if(@GuildID = @UnionMasterGuildID OR (@UnionMasterGuildID = 0 OR @UnionMasterGuildID IS NULL))
		BEGIN
			SELECT @GuildName = Name FROM SRO_VT_SHARD.dbo._Guild WITH(NOLOCK) WHERE ID = @GuildID
			SET @UnionName = @GuildName
		END
		ELSE
		BEGIN
			SELECT @GuildName = Name FROM SRO_VT_SHARD.dbo._Guild WITH(NOLOCK) WHERE ID = @GuildID
			SELECT @UnionName = Name FROM SRO_VT_SHARD.dbo._Guild WITH(NOLOCK) WHERE ID = @UnionMasterGuildID
			
		END

		DECLARE @TotalText varchar(128) = FORMATMESSAGE('%s;%s;%s', @GuildName, @CharnameKiller, @UnionName)
		
		INSERT INTO [KMT].[Command_FilterQueue](CommandID, Data1, Data2, Data3, Data4, Status) VALUES(35, @CharnameKiller, @GuildName, @UnionName, @WorldID, 1)

	END
	END

GO

-- ========================================
-- KMT.Log_Drops
-- ========================================

CREATE   PROCEDURE [KMT].[Log_Drops]
    @Page INT = 1,
    @PageSize INT = 10,
    @SearchText NVARCHAR(64) = N'',
    @Filter TINYINT = 0
AS
BEGIN
    SET NOCOUNT ON;

    IF (@Page < 1)
        SET @Page = 1;

    IF (@PageSize < 1 OR @PageSize > 50)
        SET @PageSize = 10;

    SET @SearchText = ISNULL(LTRIM(RTRIM(@SearchText)), N'');

    ;WITH LatestLogs AS
    (
        SELECT TOP (500)
            CAST(ch.CharName16 AS NVARCHAR(64)) COLLATE DATABASE_DEFAULT AS Player,

            CAST(ref.CodeName128 AS NVARCHAR(128)) COLLATE DATABASE_DEFAULT AS ItemCode,

            CAST(
                CASE 
                    WHEN plusData.PlusPos > 0
                        THEN N'+' + SUBSTRING(
                            descData.DescText,
                            plusData.PlusPos + 1,
                            CASE 
                                WHEN digitData.NonDigitPos > 1 
                                    THEN digitData.NonDigitPos - 1
                                ELSE 1
                            END
                        )
                    ELSE N'+0'
                END
            AS NVARCHAR(8)) COLLATE DATABASE_DEFAULT AS PlusAmount,

            CAST(
                CASE
                    WHEN mobData.MobPos > 0
                        THEN SUBSTRING(
                            descData.DescText,
                            mobData.MobPos,
                            CASE
                                WHEN mobEndData.CommaAfterMob > mobData.MobPos
                                    THEN mobEndData.CommaAfterMob - mobData.MobPos
                                ELSE 128
                            END
                        )
                    ELSE N'-'
                END
            AS NVARCHAR(128)) COLLATE DATABASE_DEFAULT AS Monster,

            CAST(
                CASE
                    WHEN varData.VarPos = 0 
                         OR bracketStartData.BracketStart = 0 
                         OR bracketEndData.BracketEnd = 0
                        THEN N'-'

                    WHEN CAST(ch.CharName16 AS NVARCHAR(64)) COLLATE DATABASE_DEFAULT 
                         LIKE N'%' + infoData.RawInfo COLLATE DATABASE_DEFAULT + N'%'
                        THEN N'-'

                    ELSE UPPER(infoData.RawInfo)
                END
            AS NVARCHAR(64)) COLLATE DATABASE_DEFAULT AS Info,

            CONVERT(NVARCHAR(32), elog.EventTime, 120) COLLATE DATABASE_DEFAULT AS [Date],

            CAST(N'-' AS NVARCHAR(64)) COLLATE DATABASE_DEFAULT AS [Location],

            elog.EventTime AS SortDate

        FROM [SRO_VT_SHARD].dbo._Items AS items WITH (NOLOCK)

        INNER JOIN [SRO_VT_SHARDLOG].dbo._LogEventItem AS elog WITH (NOLOCK)
            ON items.Serial64 = elog.Serial64

        INNER JOIN [SRO_VT_SHARD].dbo._Char AS ch WITH (NOLOCK)
            ON elog.CharID = ch.CharID

        INNER JOIN [SRO_VT_SHARD].dbo._RefObjCommon AS ref WITH (NOLOCK)
            ON elog.ItemRefID = ref.ID

        CROSS APPLY
        (
            SELECT CAST(ISNULL(elog.strDesc, N'') AS NVARCHAR(MAX)) AS DescText
        ) AS descData

        CROSS APPLY
        (
            SELECT PATINDEX(N'%+[0-9]%', descData.DescText) AS PlusPos
        ) AS plusData

        CROSS APPLY
        (
            SELECT 
                CASE 
                    WHEN plusData.PlusPos > 0
                        THEN PATINDEX(
                            N'%[^0-9]%',
                            SUBSTRING(descData.DescText, plusData.PlusPos + 1, 10) + N'X'
                        )
                    ELSE 0
                END AS NonDigitPos
        ) AS digitData

        CROSS APPLY
        (
            SELECT CHARINDEX(N'MOB', descData.DescText) AS MobPos
        ) AS mobData

        CROSS APPLY
        (
            SELECT 
                CASE 
                    WHEN mobData.MobPos > 0
                        THEN CHARINDEX(N',', descData.DescText, mobData.MobPos)
                    ELSE 0
                END AS CommaAfterMob
        ) AS mobEndData

        CROSS APPLY
        (
            SELECT CHARINDEX(N'Var', descData.DescText) AS VarPos
        ) AS varData

        CROSS APPLY
        (
            SELECT 
                CASE 
                    WHEN varData.VarPos > 0
                        THEN CHARINDEX(N'[', descData.DescText, varData.VarPos)
                    ELSE 0
                END AS BracketStart
        ) AS bracketStartData

        CROSS APPLY
        (
            SELECT 
                CASE 
                    WHEN bracketStartData.BracketStart > 0
                        THEN CHARINDEX(N']', descData.DescText, bracketStartData.BracketStart + 1)
                    ELSE 0
                END AS BracketEnd
        ) AS bracketEndData

        CROSS APPLY
        (
            SELECT 
                CASE
                    WHEN bracketStartData.BracketStart > 0
                         AND bracketEndData.BracketEnd > bracketStartData.BracketStart
                        THEN SUBSTRING(
                            descData.DescText,
                            bracketStartData.BracketStart + 1,
                            bracketEndData.BracketEnd - bracketStartData.BracketStart - 1
                        )
                    ELSE N''
                END AS RawInfo
        ) AS infoData

        WHERE
            descData.DescText LIKE N'%MOB%'

            -- Ø§Ø³ØªØ¨Ø¹Ø§Ø¯ Ø§Ù„Ø­Ø§Ø¬Ø§Øª Ø§Ù„Ù„ÙŠ Ù…Ø´ Ù„Ø¨Ø³/Ø³Ù„Ø§Ø­
            AND ref.CodeName128 NOT LIKE '%ARCHEMY%'
            AND ref.CodeName128 NOT LIKE '%ALCHEMY%'
            AND ref.CodeName128 NOT LIKE '%MAGICSTONE%'
            AND ref.CodeName128 NOT LIKE '%MAGICSTONE%'
            AND ref.CodeName128 NOT LIKE '%STONE%'
            AND ref.CodeName128 NOT LIKE '%ELIXIR%'
            AND ref.CodeName128 NOT LIKE '%ETC%'
            AND ref.CodeName128 NOT LIKE '%MALL%'

            -- Ù‡Ù†Ø§ Ø§Ù„Ù…Ù‡Ù…: ÙƒÙ„ Ø£ÙŠØªÙ…Ø§Øª Ø§Ù„ØµÙŠÙ† ÙˆØ£ÙˆØ±ÙˆØ¨Ø§ØŒ Ù…Ø´ RARE Ø¨Ø³
            AND
            (
                ref.CodeName128 LIKE 'ITEM_CH_%'
                OR ref.CodeName128 LIKE 'ITEM_EU_%'
            )

        ORDER BY elog.EventTime DESC
    ),
    Filtered AS
    (
        SELECT *
        FROM LatestLogs
        WHERE
            (
                @SearchText = N''
                OR Player LIKE N'%' + @SearchText + N'%'
                OR ItemCode LIKE N'%' + @SearchText + N'%'
                OR Monster LIKE N'%' + @SearchText + N'%'
                OR Info LIKE N'%' + @SearchText + N'%'
                OR PlusAmount LIKE N'%' + @SearchText + N'%'
            )
            AND
            (
                @Filter = 0
                OR (@Filter = 1 AND ItemCode LIKE N'ITEM_CH_%')
                OR (@Filter = 2 AND ItemCode LIKE N'ITEM_EU_%')
            )
    ),
    Numbered AS
    (
        SELECT
            ROW_NUMBER() OVER (ORDER BY SortDate DESC) AS RowNum,
            COUNT(*) OVER () AS TotalCount,
            Player,
            ItemCode,
            PlusAmount,
            Monster,
            Info,
            [Date],
            [Location]
        FROM Filtered
    )
    SELECT
        TotalCount,
        Player,
        ItemCode,
        PlusAmount,
        Monster,
        Info,
        [Date],
        [Location]
    FROM Numbered
    WHERE RowNum BETWEEN ((@Page - 1) * @PageSize + 1) AND (@Page * @PageSize)
    ORDER BY RowNum;
END

GO

-- ========================================
-- KMT.Log_MonsterDrops
-- ========================================

CREATE   PROCEDURE [KMT].[Log_MonsterDrops]
    @MonsterID INT
AS
BEGIN
    SET NOCOUNT ON;

    SELECT
        M.ID AS MonsterID,
        M.CodeName128 AS MonsterCodeName,

        I.ID AS ItemID,
        I.CodeName128 AS ItemCodeName,
        I.NameStrID128 AS ItemNameStrID,

        D.DropAmountMin,
        D.DropAmountMax,
        D.DropRatio

    FROM [SRO_VT_SHARD].[dbo].[_RefMonster_AssignedItemRndDrop] D

    INNER JOIN [SRO_VT_SHARD].[dbo].[_RefObjCommon] M
        ON D.RefMonsterID = M.ID

    INNER JOIN [SRO_VT_SHARD].[dbo].[_RefDropItemGroup] G
        ON D.RefItemGroupID = G.RefItemGroupID

    INNER JOIN [SRO_VT_SHARD].[dbo].[_RefObjCommon] I
        ON G.RefItemID = I.ID

    WHERE
        D.RefMonsterID = @MonsterID
        AND M.Service = 1
        AND I.Service = 1

    ORDER BY
        D.DropRatio DESC,
        I.CodeName128;
END

GO

-- ========================================
-- KMT.Log_RareDrops
-- ========================================

CREATE   PROCEDURE [KMT].[Log_RareDrops]
    @TopCount INT = 500
AS
BEGIN
    SET NOCOUNT ON;

    SELECT TOP (@TopCount)
        chart.CharName16 AS Player,
        ref.CodeName128 AS ItemCode,

        CASE 
            WHEN CHARINDEX('Opt', elog.strDesc) != 0 
                THEN SUBSTRING(elog.strDesc, PATINDEX('%+%', elog.strDesc), 2)
            ELSE '+0'
        END AS PlusAmount,

        SUBSTRING(
            elog.strDesc,
            CHARINDEX('MOB', elog.strDesc),
            CHARINDEX(',', elog.strDesc, CHARINDEX('MOB', elog.strDesc)) - CHARINDEX('MOB', elog.strDesc)
        ) AS Monster,

        CASE 
            WHEN chart.CharName16 COLLATE SQL_Latin1_General_CP1_CI_AS LIKE '%' + (
                SUBSTRING(
                    elog.strDesc,
                    CHARINDEX('[', elog.strDesc, CHARINDEX('Var', elog.strDesc)) + 1,
                    CHARINDEX(']', elog.strDesc, CHARINDEX('[', elog.strDesc, CHARINDEX('Var', elog.strDesc)) + 1)
                    - CHARINDEX('[', elog.strDesc, CHARINDEX('Var', elog.strDesc)) - 1
                )
            ) + '%'
                THEN '-'

            WHEN CHARINDEX('Var', elog.strDesc) != 0
                THEN UPPER(
                    SUBSTRING(
                        elog.strDesc,
                        CHARINDEX('[', elog.strDesc, CHARINDEX('Var', elog.strDesc)) + 1,
                        CHARINDEX(']', elog.strDesc, CHARINDEX('[', elog.strDesc, CHARINDEX('Var', elog.strDesc)) + 1)
                        - CHARINDEX('[', elog.strDesc, CHARINDEX('Var', elog.strDesc)) - 1
                    )
                )
            ELSE '-'
        END AS Info,

        elog.EventTime AS [Date]

    FROM _Items AS items
    JOIN SRO_VT_SHARDLOG.dbo._LogEventItem AS elog 
        ON items.Serial64 = elog.Serial64

    JOIN _Char AS chart 
        ON elog.CharID = chart.CharID

    JOIN _RefObjCommon AS ref 
        ON elog.ItemRefID = ref.ID

    WHERE 
        elog.strDesc LIKE '%MOB%'
        AND ref.CodeName128 NOT LIKE '%ARCHEMY%'
        AND ref.CodeName128 LIKE '%RARE%'

    ORDER BY elog.EventTime DESC;
END

GO

-- ========================================
-- KMT.NPC_Kill
-- ========================================
CREATE   PROCEDURE [KMT].[NPC_Kill]
	@CodeName128 varchar(128)
AS
	
	DECLARE @MobRefObjID int = (SELECT ID FROM SRO_VT_SHARD.dbo._RefObjCommon with (nolock) WHERE CodeName128 = @CodeName128)
	IF(@MobRefObjID IS NOT NULL AND @MobRefObjID != 0)
	BEGIN
		INSERT INTO [KMT].[Command_GameServerQueue](Action_ID, Data1) VALUES(4, @MobRefObjID)
	END
GO

-- ========================================
-- KMT.NPC_KillByWorld
-- ========================================
CREATE   PROCEDURE [KMT].[NPC_KillByWorld]
	@CodeName128 varchar(128),
	@WorldID int

AS
	
	DECLARE @MobRefObjID int = (SELECT ID FROM SRO_VT_SHARD.dbo._RefObjCommon with (nolock) WHERE CodeName128 = @CodeName128)
	IF(@MobRefObjID IS NOT NULL AND @MobRefObjID != 0)
	BEGIN
		INSERT INTO [KMT].[Command_GameServerQueue](Action_ID, Data1, Data2) VALUES(5, @WorldID, @MobRefObjID)
	END

GO

-- ========================================
-- KMT.NPC_Spawn
-- ========================================
CREATE   PROCEDURE [KMT].[NPC_Spawn]
	@CodeName128 varchar(128),
	@GameWorldID int,
	@RegionId int,
	@PosX int,
	@PosY int,
	@PosZ int,
	@GenerateRadius int
AS		

	DECLARE @MobRefObjID int = (SELECT ID FROM SRO_VT_SHARD.dbo._RefObjCommon WHERE CodeName128 = @CodeName128)
	IF(@MobRefObjID IS NOT NULL AND @MobRefObjID != 0)
	BEGIN
		INSERT INTO [KMT].[Command_GameServerQueue](Action_ID, Data1, Data2, Data3, Data4, Data5, Data6, Data7) VALUES(2, @MobRefObjID, @GameWorldID, @RegionId, @PosX, @PosY, @PosZ, @GenerateRadius)
	END
GO

-- ========================================
-- KMT.NPC_SpawnAtPosition
-- ========================================
CREATE   PROCEDURE [KMT].[NPC_SpawnAtPosition]
	@MonsterID int,
	@GameWorldID int,
	@RegionId int,
	@PosX int,
	@PosY int,
	@PosZ int,
	@GenerateRadius int
AS		

INSERT INTO [KMT].[Command_GameServerQueue](Action_ID, Data1, Data2, Data3, Data4, Data5, Data6, Data7) VALUES(2, @MonsterID, @GameWorldID, @RegionId, @PosX, @PosY, @PosZ, @GenerateRadius)


GO

-- ========================================
-- KMT.NPC_SpawnNearPlayer
-- ========================================
CREATE   PROCEDURE [KMT].[NPC_SpawnNearPlayer]
	@CharID int,
	@MonsterID int
AS		

INSERT INTO [KMT].[Command_GameServerQueue](Action_ID, Data1, Data2) VALUES(3, @CharID, @MonsterID)


GO

-- ========================================
-- KMT.NPC_Sync
-- ========================================
-- =============================================
-- Author:		<Author,,Name>
-- Create date: <Create Date,,>
-- Description:	<Description,,>
-- =============================================
CREATE   PROCEDURE [KMT].[NPC_Sync]
	-- Add the parameters for the stored procedure here
	@UniqueID int,
	@RefObjId int,
	@CodeName128 VARCHAR(128)
AS
BEGIN
	-- SET NOCOUNT ON added to prevent extra result sets from
	-- interfering with SELECT statements.
	SET NOCOUNT ON;

	IF EXISTS ( Select 1 from [KMT].[NPC_GameServerIDs] where CodeName128 = @CodeName128)
	BEGIN
		UPDATE [KMT].[NPC_GameServerIDs] set UniqueID = @UniqueID, RefObjId = @RefObjId where CodeName128 = @CodeName128
	END
	ELSE
	BEGIN
	INSERT INTO [KMT].[NPC_GameServerIDs] (UniqueID, RefObjId, CodeName128) VALUES (@UniqueID, @RefObjId, @CodeName128)
	END
END


GO

-- ========================================
-- KMT.Player_GetFellowPet
-- ========================================
-- =============================================
-- Author:		<Author,,Name>
-- Create date: <Create Date,,>
-- Description:	<Description,,>
-- =============================================
CREATE   PROCEDURE [KMT].[Player_GetFellowPet]
	-- Add the parameters for the stored procedure here
	@CharID INT,
	@Slot smallint,
	@ItemID64 BIGINT OUTPUT,
	@RefItemID INT OUTPUT
AS
BEGIN
	-- SET NOCOUNT ON added to prevent extra result sets from
	-- interfering with SELECT statements.
	SET NOCOUNT ON;

	-- =============================================
	-- TODO: Check if the player is a game master.

	-- =============================================
	-- Set variables to defaults, just to be sure.

	SET @ItemID64 = 0
	set @RefItemID = 0

	DECLARE @ItemID int;
	select @ItemID=ItemID from SRO_VT_SHARD.._Inventory
	where Slot = @Slot and CharID =  @CharID
	
	select @ItemID64=ID64, @RefItemID=RefItemID from SRO_VT_SHARD.._Items
	where ID64 = @ItemID
	
	SET @ItemID64 = @ItemID64
	set @RefItemID = @RefItemID

END


GO

-- ========================================
-- KMT.Player_SaveConfig
-- ========================================
CREATE   PROCEDURE [KMT].[Player_SaveConfig]
    @CharID int,
    @SlotSeq tinyint,
    @SlotType tinyint,
    @Data int
AS
SET NOCOUNT ON;

    BEGIN
    IF EXISTS(Select CharID from SRO_VT_SHARD.._ClientConfig with(nolock) where CharID = @CharID and SlotSeq = @SlotSeq)
    BEGIN
    	UPDATE SRO_VT_SHARD.._ClientConfig SET SlotType = @SlotType, Data = @Data where CharID = @CharID and SlotSeq = @SlotSeq
    END
    ELSE
    BEGIN
    	INSERT INTO SRO_VT_SHARD.._ClientConfig VALUES(@CharID, 1, @SlotSeq, @SlotType, @Data)
    END
    END

GO

-- ========================================
-- KMT.Player_SaveFellow
-- ========================================
-- =============================================
-- Author:		<Author,,Name>
-- Create date: <Create Date,,>
-- Description:	<Description,,>
-- =============================================
CREATE   PROCEDURE [KMT].[Player_SaveFellow]
	-- Add the parameters for the stored procedure here
	@ID64 bigint,
	@Enable_Skill_1 tinyint, 
	@Enable_Skill_2 tinyint, 
	@Enable_Skill_3 tinyint, 
	@Enable_Skill_4 tinyint, 
	@Enable_Skill_5 tinyint
AS
BEGIN
	-- SET NOCOUNT ON added to prevent extra result sets from
	-- interfering with SELECT statements.
	SET NOCOUNT ON;
			IF EXISTS (Select 1 from [KMT].[Fellow_Skills] where ID64 = @ID64)
		BEGIN
			UPDATE [KMT].[Fellow_Skills] set Enable_Skill_1=@Enable_Skill_1, Enable_Skill_2=@Enable_Skill_2 
			,Enable_Skill_3=@Enable_Skill_3, Enable_Skill_4=@Enable_Skill_4, Enable_Skill_5=@Enable_Skill_5 where ID64 = @ID64
		END
		ELSE
		BEGIN
		INSERT INTO [KMT].[Fellow_Skills] VALUES (@ID64, @Enable_Skill_1,@Enable_Skill_2, @Enable_Skill_3, @Enable_Skill_4, @Enable_Skill_5)
		END
END


GO

-- ========================================
-- KMT.Player_SaveLocation
-- ========================================
-- =============================================
-- Author:		<Author,,Name>
-- Create date: <Create Date,,>
-- Description:	<Description,,>
-- =============================================
CREATE   PROCEDURE [KMT].[Player_SaveLocation]
	-- Add the parameters for the stored procedure here
	@CharID int, 
	@LocationID int, 
	@RegionID int, 
	@PosX int, 
	@PosY int, 
	@PosZ int, 
	@WorldID int,
	@Success BIT OUTPUT
AS
BEGIN
	-- SET NOCOUNT ON added to prevent extra result sets from
	-- interfering with SELECT statements.
	SET NOCOUNT ON;
	SET @Success = 0
	
	IF NOT EXISTS (select 1 from [KMT].[Teleport_SavedLocations] with (nolock) WHERE CharID = @CharID AND LocationID = @LocationID)
	BEGIN
	INSERT INTO [KMT].[Teleport_SavedLocations] (CharID, LocationID, RegionID, PosX, PosY, PosZ, WorldID) VALUES (@CharID, @LocationID, @RegionID, @PosX, @PosY, @PosZ, @WorldID)
	SET @Success = 1
	END
END


GO

-- ========================================
-- KMT.Style_AddIcon
-- ========================================
CREATE   PROCEDURE [KMT].[Style_AddIcon]
	@CharID int,
	@IconID int,
	@Side tinyint
AS		


IF NOT EXISTS(SELECT 1 FROM [KMT].[Style_PlayerIcons] WITH(NOLOCK) WHERE  CharID = @CharID AND IconID = @IconID AND Side = @Side)
	BEGIN
		DECLARE @NewID INT;
		INSERT INTO [KMT].[Style_PlayerIcons] VALUES(@CharID, @IconID, @Side)
		SET @NewID = SCOPE_IDENTITY();
		INSERT INTO [KMT].[Command_FilterQueue](CommandID, Data1, Data2,Data3,Data4, Status) VALUES(6, @CharID, @IconID, @Side, @NewID, 1)
END

GO

-- ========================================
-- KMT.Style_AddTitle
-- ========================================
CREATE   PROCEDURE [KMT].[Style_AddTitle]
	@CharID int,
	@TitleID tinyint
AS		


IF NOT EXISTS(SELECT 1 FROM [KMT].[Style_PlayerTitles] WHERE CharID = @CharID AND TitleID = @TitleID)
BEGIN
	INSERT INTO [KMT].[Style_PlayerTitles] VALUES(@CharID, @TitleID)
	INSERT INTO [KMT].[Command_FilterQueue](CommandID, Data1, Data2, Status) VALUES(4, @CharID, @TitleID, 1)
END

GO

-- ========================================
-- KMT.Style_AddTitleColor
-- ========================================
CREATE   PROCEDURE [KMT].[Style_AddTitleColor]
	@CharID int,
	@ColorCode varchar(100),
	@ColorName varchar(100)
AS		


IF NOT EXISTS(SELECT 1 FROM [KMT].[Style_PlayerTitleColors] WITH(NOLOCK) WHERE CharID = @CharID AND ColorCode = @ColorCode AND ColorName = @ColorName)
	BEGIN
		INSERT INTO [KMT].[Style_PlayerTitleColors] (CharID, ColorName, ColorCode) VALUES (@CharID, @ColorName, @ColorCode)
		DECLARE @NewID INT;
		SET @NewID = SCOPE_IDENTITY();
		INSERT INTO [KMT].[Command_FilterQueue](CommandID, Data1, Data2, Data3, Data4, Status) VALUES(5, @CharID, @ColorName, @ColorCode, @NewID, 1)
END

GO

-- ========================================
-- KMT.Style_ChangeGlow
-- ========================================
CREATE   PROCEDURE [KMT].[Style_ChangeGlow]
	@CharName16 varchar(25),
	@CharID int,

	@ItemCodeName NVARCHAR(255),
	@NewGlow NVARCHAR(255),
	@TargetSlot tinyint,
	@UsedItemSlot tinyint
AS

--DECLARE @ItemCodeName NVARCHAR(255) = 'ITEM_CH_SWORD_08_B_RARE_GLOW_66';
--DECLARE @NewGlow NVARCHAR(255) = 'GLOW_1'; -- Tek bir GLOW deÄŸeri

DECLARE @BaseName NVARCHAR(255);
DECLARE @Result NVARCHAR(255);
DECLARE @GlowIndex INT;

SET @GlowIndex = CHARINDEX('_GLOW_', @ItemCodeName);


IF @GlowIndex > 0
BEGIN

    SET @BaseName = SUBSTRING(@ItemCodeName, 1, @GlowIndex - 1);

    SET @Result = @BaseName + '_' + @NewGlow;

	IF (@Result = @ItemCodeName)
	BEGIN
	EXEC [KMT].[Command_NoticeByName] @CharName16, 3, N'Bu iteme daha Ã¶nce aynÄ± glow eklenmiÅŸtir.'
	return;
	END
	IF EXISTS (Select 1 from SRO_VT_SHARD.._RefObjCommon
	where CodeName128 = @Result)
	BEGIN
	EXEC [KMT].[Live_UseAndChangeItem] @CharID, @TargetSlot, @Result, @UsedItemSlot, 1
	END

END
ELSE
BEGIN
    SET @Result = @ItemCodeName + '_' + @NewGlow;

	IF (@Result = @ItemCodeName)
	BEGIN
	EXEC [KMT].[Command_NoticeByName] @CharName16, 3, N'Bu iteme daha Ã¶nce aynÄ± glow eklenmiÅŸtir.'
	return;
	END
	IF EXISTS (Select 1 from SRO_VT_SHARD.._RefObjCommon
	where CodeName128 = @Result)
	BEGIN
	EXEC [KMT].[Live_UseAndChangeItem] @CharID, @TargetSlot, @Result, @UsedItemSlot, 1
	END

END
Select @Result AS Final

GO

-- ========================================
-- KMT.Style_ChangeModel
-- ========================================
CREATE   PROCEDURE [KMT].[Style_ChangeModel]
	@CharName16 varchar(25),
	@CharID int,

	@ItemCodeName NVARCHAR(255),
	@NewModel NVARCHAR(255),
	@TargetSlot tinyint,
	@UsedItemSlot tinyint
AS


DECLARE @ModelStart INT;
DECLARE @ModelEnd INT;
DECLARE @GlowStart INT;
DECLARE @GlowEnd INT;
DECLARE @RareEnd INT;
DECLARE @GlowPart NVARCHAR(255); 
DECLARE @CurrentModel NVARCHAR(255);

DECLARE @TargetItemCodeName NVARCHAR(255) = (@ItemCodeName)


-- _MODEL ifadesinin baÅŸlangÄ±Ã§ konumunu bul
SET @ModelStart = CHARINDEX('_MODEL', @TargetItemCodeName);

-- _GLOW ifadesinin baÅŸlangÄ±Ã§ konumunu bul
SET @GlowStart = CHARINDEX('_GLOW', @TargetItemCodeName);

-- _RARE ifadesinin son konumunu bul
SET @RareEnd = CHARINDEX('_RARE', @TargetItemCodeName) + LEN('_RARE') - 1;

-- EÄŸer _GLOW ifadesi varsa, onu ayÄ±kla ve geri bÄ±rak
IF @GlowStart > 0
BEGIN
    SET @GlowEnd = LEN(@TargetItemCodeName);
    SET @GlowPart = SUBSTRING(@TargetItemCodeName, @GlowStart, @GlowEnd - @GlowStart + 1);
    -- _GLOW kÄ±smÄ±nÄ± ayÄ±kla
    SET @TargetItemCodeName = LEFT(@TargetItemCodeName, @GlowStart - 1);
END
ELSE
BEGIN
    SET @GlowPart = '';
END

-- EÄŸer _MODEL ifadesi varsa
IF @ModelStart > 0
BEGIN
    -- MODEL ifadesinin bitiÅŸ konumunu bul
    SET @ModelEnd = CHARINDEX('_', @TargetItemCodeName, @ModelStart + LEN('_MODEL'));

    -- EÄŸer bitiÅŸ konumu bulunamazsa, son konumu kullan
    IF @ModelEnd = 0
    BEGIN
        SET @ModelEnd = LEN(@TargetItemCodeName) + 1;
    END

    -- Mevcut MODEL deÄŸerini ayÄ±kla
    SET @CurrentModel = SUBSTRING(@TargetItemCodeName, @ModelStart + LEN('_MODEL'), @ModelEnd - @ModelStart - LEN('_MODEL'));

    -- EÄŸer mevcut MODEL ve yeni MODEL aynÄ±ysa, FAIL mesajÄ± gÃ¶nder
    IF @CurrentModel = REPLACE(@NewModel, 'MODEL', '')
    BEGIN
		EXEC [KMT].[Command_NoticeByName] @CharName16, 3, N'Bu iteme daha Ã¶nce aynÄ± model eklenmiÅŸtir.'
        RETURN;
    END

    -- Temel isim kÄ±smÄ±nÄ± al (RARE kÄ±smÄ±ndan sonrasÄ±nÄ± da korur)
    SET @TargetItemCodeName = LEFT(@TargetItemCodeName, @ModelStart - 1);

    -- Yeni MODEL deÄŸerini ekleme
    SET @TargetItemCodeName = @TargetItemCodeName + '_' + @NewModel;

    -- EÄŸer daha Ã¶nce _GLOW varsa, onu tekrar ekle
    IF @GlowPart <> ''
    BEGIN
        SET @TargetItemCodeName = @TargetItemCodeName + @GlowPart;
    END
END
ELSE
BEGIN
    -- MODEL kÄ±smÄ± yoksa, yeni MODEL deÄŸeri ekle
    -- RARE kÄ±smÄ±ndan sonrasÄ±na ekleme yap
    SET @TargetItemCodeName = LEFT(@TargetItemCodeName, @RareEnd) + '_' + @NewModel;

    -- EÄŸer daha Ã¶nce _GLOW varsa, onu tekrar ekle
    IF @GlowPart <> ''
    BEGIN
        SET @TargetItemCodeName = @TargetItemCodeName + '_' + @GlowPart;
    END
END

	IF EXISTS (Select 1 from SRO_VT_SHARD.._RefObjCommon with(nolock) where CodeName128 = @TargetItemCodeName)
	BEGIN
	EXEC [KMT].[Live_UseAndChangeItem] @CharID, @TargetSlot, @TargetItemCodeName, @UsedItemSlot, 1
	END

GO

-- ========================================
-- KMT.Style_ChooseIcon
-- ========================================
-- =============================================
-- Author:		<Author,,Name>
-- Create date: <Create Date,,>
-- Description:	<Description,,>
-- =============================================
CREATE   PROCEDURE [KMT].[Style_ChooseIcon]
	-- Add the parameters for the stored procedure here
	@CharName16 VARCHAR(32),
	@IconID int,
	@Side tinyint 
AS
BEGIN
	-- SET NOCOUNT ON added to prevent extra result sets from
	-- interfering with SELECT statements.
	SET NOCOUNT ON;
	IF(@Side = 0) -- left
	BEGIN
	IF EXISTS ( Select 1 from [KMT].[Style_ActiveLeftIcons] where CharName16 = @CharName16)
	BEGIN
		UPDATE [KMT].[Style_ActiveLeftIcons] set IconID = @IconID where CharName16 = @CharName16
	END
	ELSE
	BEGIN
	INSERT INTO [KMT].[Style_ActiveLeftIcons] (CharName16, IconID) VALUES (@CharName16, @IconID)
	END
	END
	ELSE IF(@Side = 1) -- right
	BEGIN
	IF EXISTS ( Select 1 from [KMT].[Style_ActiveRightIcons] where CharName16 = @CharName16)
	BEGIN
		UPDATE [KMT].[Style_ActiveRightIcons] set IconID = @IconID where CharName16 = @CharName16
	END
	ELSE
	BEGIN
	INSERT INTO [KMT].[Style_ActiveRightIcons] (CharName16, IconID) VALUES (@CharName16, @IconID)
	END
	END
END


GO

-- ========================================
-- KMT.Style_ChooseTitle
-- ========================================
-- =============================================
-- Author:		<Author,,Name>
-- Create date: <Create Date,,>
-- Description:	<Description,,>
-- =============================================
CREATE   PROCEDURE [KMT].[Style_ChooseTitle]
	-- Add the parameters for the stored procedure here
	@CharName16 VARCHAR(32),
	@TitleID tinyint
AS
BEGIN
	-- SET NOCOUNT ON added to prevent extra result sets from
	-- interfering with SELECT statements.
	SET NOCOUNT ON;
		IF EXISTS (Select 1 from [KMT].[Style_ActiveTitles] where CharName16 = @CharName16)
		BEGIN
			UPDATE [KMT].[Style_ActiveTitles] set RefTitleNameNewID = @TitleID where CharName16 = @CharName16
		END
		ELSE
		BEGIN
		INSERT INTO [KMT].[Style_ActiveTitles] (CharName16, RefTitleNameNewID) VALUES (@CharName16, @TitleID)
		END
END


GO

-- ========================================
-- KMT.Style_ChooseTitleColor
-- ========================================
-- =============================================
-- Author:		<Author,,Name>
-- Create date: <Create Date,,>
-- Description:	<Description,,>
-- =============================================
CREATE   PROCEDURE [KMT].[Style_ChooseTitleColor]
	-- Add the parameters for the stored procedure here
	@CharName16 VARCHAR(32),
	@ColorCode varchar(32)
AS
BEGIN
	-- SET NOCOUNT ON added to prevent extra result sets from
	-- interfering with SELECT statements.
	SET NOCOUNT ON;
		IF EXISTS (Select 1 from [KMT].[Style_ActiveTitleColors] where CharName16 = @CharName16)
		BEGIN
			UPDATE [KMT].[Style_ActiveTitleColors] set ColorCode = @ColorCode where CharName16 = @CharName16
		END
		ELSE
		BEGIN
		INSERT INTO [KMT].[Style_ActiveTitleColors] (CharName16, ColorCode) VALUES (@CharName16, @ColorCode)
		END
END


GO

-- ========================================
-- KMT.Style_RemoveLeftIcon
-- ========================================
CREATE   PROCEDURE [KMT].[Style_RemoveLeftIcon]
	@CharName16 as varchar(16)
AS		

	IF EXISTS(SELECT IconID From [KMT].[Style_ActiveLeftIcons] with(nolock) WHERE CharName16 = @CharName16)
	BEGIN
		DELETE FROM [KMT].[Style_ActiveLeftIcons]  WHERE CharName16 = @CharName16
		INSERT INTO [KMT].[Command_FilterQueue](CommandID, Data1, Status) VALUES(12, @CharName16, 1)
	END


GO

-- ========================================
-- KMT.Style_RemoveNameColor
-- ========================================
CREATE   PROCEDURE [KMT].[Style_RemoveNameColor]
	@CharName16 varchar(16)
AS		


IF EXISTS(SELECT 1 FROM [KMT].[Style_ActiveNameColors] WITH(NOLOCK) WHERE  CharName16 = @CharName16)
	BEGIN
		DELETE FROM [KMT].[Style_ActiveNameColors] where CharName16 = @CharName16
		INSERT INTO [KMT].[Command_FilterQueue](CommandID, Data1, Status) VALUES(38, @CharName16, 1)
END


GO

-- ========================================
-- KMT.Style_RemoveRightIcon
-- ========================================
CREATE   PROCEDURE [KMT].[Style_RemoveRightIcon]
	@CharName16 as varchar(16)
AS		

	IF EXISTS(SELECT IconID From [KMT].[Style_ActiveRightIcons] with(nolock) WHERE CharName16 = @CharName16)
	BEGIN
		DELETE FROM [KMT].[Style_ActiveRightIcons] WHERE CharName16 = @CharName16
		INSERT INTO [KMT].[Command_FilterQueue](CommandID, Data1, Status) VALUES(13, @CharName16, 1)
	END




GO

-- ========================================
-- KMT.Style_RemoveTitle
-- ========================================
CREATE   PROCEDURE [KMT].[Style_RemoveTitle]
	@CharName16 varchar(16)
AS		


IF EXISTS(SELECT 1 FROM [KMT].[Style_ActiveTitles] WITH(NOLOCK) WHERE  CharName16 = @CharName16)
	BEGIN
		DELETE FROM [KMT].[Style_ActiveTitles] where CharName16 = @CharName16
		INSERT INTO [KMT].[Command_FilterQueue](CommandID, Data1, Status) VALUES(16, @CharName16, 1)
END


GO

-- ========================================
-- KMT.Style_RemoveTitleColor
-- ========================================
CREATE   PROCEDURE [KMT].[Style_RemoveTitleColor]
	@CharName16 varchar(16)
AS		


IF EXISTS(SELECT 1 FROM [KMT].[Style_ActiveTitleColors] WITH(NOLOCK) WHERE  CharName16 = @CharName16)
	BEGIN
		DELETE FROM [KMT].[Style_ActiveTitleColors] where CharName16 = @CharName16
		INSERT INTO [KMT].[Command_FilterQueue](CommandID, Data1, Status) VALUES(9, @CharName16, 1)
END


GO

-- ========================================
-- KMT.Style_UpdateLeftIcon
-- ========================================
CREATE   PROCEDURE [KMT].[Style_UpdateLeftIcon]
	@CharName16 as varchar(16),
	@IconID int
AS		

	IF EXISTS(SELECT IconID From [KMT].[Style_ActiveLeftIcons] with(nolock) WHERE CharName16 = @CharName16)
	BEGIN
		UPDATE [KMT].[Style_ActiveLeftIcons] SET IconID = @IconID WHERE CharName16 = @CharName16
		INSERT INTO [KMT].[Command_FilterQueue](CommandID, Data1, Data2, Status) VALUES(10, @CharName16, @IconID, 1)
	END
	ELSE
	BEGIN
		INSERT INTO [KMT].[Style_ActiveLeftIcons] VALUES(@CharName16, @IconID)
		INSERT INTO [KMT].[Command_FilterQueue](CommandID, Data1, Data2, Status) VALUES(10, @CharName16, @IconID, 1)
	END


GO

-- ========================================
-- KMT.Style_UpdateNameColor
-- ========================================
CREATE   PROCEDURE [KMT].[Style_UpdateNameColor]
            @CharName16 varchar(16),
            @ColorCode varchar(100)
        AS
        BEGIN
            IF NOT EXISTS(SELECT 1 FROM [KMT].[Style_ActiveNameColors] WITH(NOLOCK) WHERE CharName16 = @CharName16)
            BEGIN
                INSERT INTO [KMT].[Style_ActiveNameColors] VALUES(@CharName16, @ColorCode)
                INSERT INTO [KMT].[Command_FilterQueue](CommandID, Data1, Data2, Status) VALUES(37, @CharName16, @ColorCode, 1)
            END
            ELSE
            BEGIN
                UPDATE [KMT].[Style_ActiveNameColors] SET ColorCode = @ColorCode WHERE CharName16 = @CharName16
                INSERT INTO [KMT].[Command_FilterQueue](CommandID, Data1, Data2, Status) VALUES(37, @CharName16, @ColorCode, 1)
            END
        END
        
GO

-- ========================================
-- KMT.Style_UpdateRightIcon
-- ========================================
CREATE   PROCEDURE [KMT].[Style_UpdateRightIcon]
	@CharName16 as varchar(16),
	@IconID int
AS		

	IF EXISTS(SELECT IconID From [KMT].[Style_ActiveRightIcons] with(nolock) WHERE CharName16 = @CharName16)
	BEGIN
		UPDATE [KMT].[Style_ActiveRightIcons] SET IconID = @IconID WHERE CharName16 = @CharName16
		INSERT INTO [KMT].[Command_FilterQueue](CommandID, Data1, Data2, Status) VALUES(11, @CharName16, @IconID, 1)
	END
	ELSE
	BEGIN
		INSERT INTO [KMT].[Style_ActiveRightIcons] VALUES(@CharName16, @IconID)
		INSERT INTO [KMT].[Command_FilterQueue](CommandID, Data1, Data2, Status) VALUES(11, @CharName16, @IconID, 1)
	END

GO

-- ========================================
-- KMT.Style_UpdateTitle
-- ========================================
CREATE   PROCEDURE [KMT].[Style_UpdateTitle]
	@CharName16 varchar(16),
	@TitleID tinyint
AS		


IF NOT EXISTS(SELECT 1 FROM [KMT].[Style_ActiveTitles] WITH(NOLOCK) WHERE  CharName16 = @CharName16)
	BEGIN
		INSERT INTO [KMT].[Style_ActiveTitles] VALUES(@CharName16, @TitleID)
		INSERT INTO [KMT].[Command_FilterQueue](CommandID, Data1, Data2, Status) VALUES(15, @CharName16, @TitleID, 1)
END
ELSE
BEGIN
	Update [KMT].[Style_ActiveTitles] Set RefTitleNameNewID = @TitleID
	where CharName16 = @CharName16
	INSERT INTO [KMT].[Command_FilterQueue](CommandID, Data1, Data2, Status) VALUES(15, @CharName16, @TitleID, 1)
END

GO

-- ========================================
-- KMT.Style_UpdateTitleColor
-- ========================================
CREATE   PROCEDURE [KMT].[Style_UpdateTitleColor]
            @CharName16 varchar(16),
            @ColorCode varchar(100)
        AS
        BEGIN
            IF NOT EXISTS(SELECT 1 FROM [KMT].[Style_ActiveTitleColors] WITH(NOLOCK) WHERE CharName16 = @CharName16)
            BEGIN
                INSERT INTO [KMT].[Style_ActiveTitleColors] VALUES(@CharName16, @ColorCode)
                INSERT INTO [KMT].[Command_FilterQueue](CommandID, Data1, Data2, Status) VALUES(8, @CharName16, @ColorCode, 1)
            END
            ELSE
            BEGIN
                UPDATE [KMT].[Style_ActiveTitleColors] SET ColorCode = @ColorCode WHERE CharName16 = @CharName16
                INSERT INTO [KMT].[Command_FilterQueue](CommandID, Data1, Data2, Status) VALUES(8, @CharName16, @ColorCode, 1)
            END
        END
        
GO

-- ========================================
-- KMT.System_Start
-- ========================================
CREATE   PROCEDURE [KMT].[System_Start]
 
AS
    SET NOCOUNT ON;

    
GO

-- ========================================
-- KMT.Teleport_AllToTown
-- ========================================
CREATE   PROCEDURE [KMT].[Teleport_AllToTown]
	@WorldID int
AS		

INSERT INTO [KMT].[Command_GameServerQueue](Action_ID, Data1) VALUES(11, @WorldID)


GO

-- ========================================
-- KMT.Teleport_PlayerToTown
-- ========================================

CREATE   PROCEDURE [KMT].[Teleport_PlayerToTown]
    @CharID int
AS
BEGIN
    SET NOCOUNT ON;

    DECLARE @GameWorldID int;
    DECLARE @RegionID int;
    DECLARE @PosX int;
    DECLARE @PosY int;
    DECLARE @PosZ int;

    SELECT TOP (1)
        @GameWorldID = TownGameWorldID,
        @RegionID = TownRegionID,
        @PosX = TownPosX,
        @PosY = TownPosY,
        @PosZ = TownPosZ
    FROM [KMT].[PVP_Settings] WITH (NOLOCK)
    ORDER BY ID;

    EXEC [KMT].[Teleport_Position]
        @CharID = @CharID,
        @GameWorldID = @GameWorldID,
        @RegionId = @RegionID,
        @PosX = @PosX,
        @PosY = @PosY,
        @PosZ = @PosZ;
END

GO

-- ========================================
-- KMT.Teleport_Position
-- ========================================

CREATE   PROCEDURE [KMT].[Teleport_Position]
    @CharID int,
    @GameWorldID int,
    @RegionId int,
    @PosX int,
    @PosY int,
    @PosZ int
AS
BEGIN
    SET NOCOUNT ON;

    DECLARE @ShardDB sysname =
        COALESCE(NULLIF((SELECT TOP (1) [Value] FROM [KMT].[System_Settings] WITH (NOLOCK) WHERE SettingName = N'ShardDB'), N''), N'SRO_VT_SHARD');

    DECLARE @FullQueueName nvarchar(300) = QUOTENAME(@ShardDB) + N'.dbo._ExeGameServer';
    DECLARE @FullCharName nvarchar(300) = QUOTENAME(@ShardDB) + N'.dbo._Char';
    DECLARE @Sql nvarchar(max) = N'
IF OBJECT_ID(N''' + REPLACE(@FullQueueName, N'''', N'''''') + N''', N''U'') IS NULL
BEGIN
    CREATE TABLE ' + @FullQueueName + N'
    (
        ID INT IDENTITY(1,1) PRIMARY KEY,
        Action_ID INT NOT NULL,
        Action_Result SMALLINT NOT NULL DEFAULT 0,
        CharName16 VARCHAR(64) NOT NULL,
        Param01 VARCHAR(129) NULL,
        Param02 BIGINT NULL,
        Param03 BIGINT NULL,
        Param04 BIGINT NULL,
        Param05 BIGINT NULL,
        Param06 BIGINT NULL,
        Param07 BIGINT NULL,
        Param08 BIGINT NULL
    );
END;

INSERT INTO ' + @FullQueueName + N'
    (Action_ID, CharName16, Param02, Param03, Param04, Param05, Param06)
SELECT
    5,
    CharName16,
    @GameWorldID,
    @RegionId,
    @PosX,
    @PosY,
    @PosZ
FROM ' + @FullCharName + N' WITH (NOLOCK)
WHERE CharID = @CharID;';

    EXEC sys.sp_executesql
        @Sql,
        N'@CharID int, @GameWorldID int, @RegionId int, @PosX int, @PosY int, @PosZ int',
        @CharID = @CharID,
        @GameWorldID = @GameWorldID,
        @RegionId = @RegionId,
        @PosX = @PosX,
        @PosY = @PosY,
        @PosZ = @PosZ;
END

GO

-- ========================================
-- KMT.Teleport_PositionFreeze
-- ========================================

CREATE   PROCEDURE [KMT].[Teleport_PositionFreeze]
    @CharID int,
    @GameWorldID int,
    @RegionId int,
    @PosX int,
    @PosY int,
    @PosZ int,
    @FreezeSeconds int = 0
AS
BEGIN
    SET NOCOUNT ON;

    IF @CharID <= 0
        THROW 50001, 'CharID must be greater than zero.', 1;

    IF @GameWorldID <= 0
        THROW 50002, 'GameWorldID must be greater than zero.', 1;

    IF @RegionId <= 0
        THROW 50003, 'RegionId must be greater than zero.', 1;

    IF @FreezeSeconds < 0 OR @FreezeSeconds > 600
        THROW 50004, 'FreezeSeconds must be between 0 and 600.', 1;

    INSERT INTO [KMT].[Teleport_FreezeQueue]
        (CharID, GameWorldID, RegionID, PosX, PosY, PosZ, FreezeSeconds, Status)
    VALUES
        (@CharID, @GameWorldID, @RegionId, @PosX, @PosY, @PosZ, @FreezeSeconds, 0);

    SELECT CONVERT(bigint, SCOPE_IDENTITY()) AS CommandID;
END

GO

-- ========================================
-- KMT.Teleport_SafeZone
-- ========================================

CREATE   PROCEDURE [KMT].[Teleport_SafeZone]
	@CHARID int,
	@GameWorldID int,
	@RegionId int,
	@PosX int,
	@PosY int,
	@PosZ int
AS		

INSERT INTO [KMT].[Command_GameServerQueue](Action_ID, Data1, Data2, Data3, Data4, Data5, Data6) VALUES(24, @CHARID, @GameWorldID, @RegionId, @PosX, @PosY, @PosZ)


GO

-- ========================================
-- KMT.Teleport_Self
-- ========================================

CREATE   PROCEDURE [KMT].[Teleport_Self]
    @CharID INT
AS
BEGIN
    SET NOCOUNT ON;
    SET XACT_ABORT ON;

    IF @CharID IS NULL OR @CharID <= 0
        THROW 50001, 'CharID must be a positive integer.', 1;

    BEGIN TRANSACTION;

    IF NOT EXISTS
    (
        SELECT 1
        FROM [KMT].[Command_FilterQueue] WITH (UPDLOCK, HOLDLOCK)
        WHERE CommandID = 41
          AND Status = 1
          AND TRY_CONVERT(INT, Data1) = @CharID
    )
    BEGIN
        INSERT INTO [KMT].[Command_FilterQueue]
            (CommandID, Data1, Status)
        VALUES
            (41, CONVERT(VARCHAR(20), @CharID), 1);
    END;

    COMMIT TRANSACTION;
END;

GO

-- ========================================
-- KMT.Trade_BuyDone
-- ========================================

CREATE   PROCEDURE [KMT].[Trade_BuyDone]
    @CharID INT,
    @Charname VARCHAR(25),
    @NpcID INT,
    @Petid INT,
    @NpcCodename VARCHAR(128),
    @NpcTab TINYINT,
    @NpcSlot TINYINT,
    @Quantity SMALLINT
AS
/*
BEGIN
    SET NOCOUNT ON;

    DECLARE @now DATETIME2(0) = SYSUTCDATETIME();
    DECLARE @hwid NVARCHAR(128);

    SELECT TOP(1) @hwid = UPPER(LTRIM(RTRIM(CONVERT(NVARCHAR(128), Hwid))))
    FROM [KMT].[Auth_HWIDs] WITH (NOLOCK)
    WHERE CharID = @CharID AND Hwid IS NOT NULL AND LEN(Hwid) > 0
    ORDER BY Active DESC, ID DESC;

    BEGIN TRY
        BEGIN TRAN;

            IF EXISTS (SELECT 1 FROM hema_system.dbo._TradeControl WITH (UPDLOCK, HOLDLOCK) WHERE CharID = @CharID)
            BEGIN
                UPDATE hema_system.dbo._TradeControl
                   SET LastBuyNpcID = @NpcID,
                       LastBuyAt    = @now,
                       HWID         = COALESCE(NULLIF(@hwid, N''), HWID),
                       UpdatedAt    = @now
                 WHERE CharID = @CharID;
            END
            ELSE
            BEGIN
                INSERT INTO hema_system.dbo._TradeControl
                    (CharID, HWID, SuccessCount, LastSellSuccessAt, LastBuyNpcID, LastBuyAt, UpdatedAt, BonusTotalGold)
                VALUES
                    (@CharID, @hwid, 0, NULL, @NpcID, @now, @now, 0);
            END

        COMMIT TRAN;
    END TRY
    BEGIN CATCH
        IF @@TRANCOUNT > 0 ROLLBACK TRAN;
    END CATCH
END
*/
GO

-- ========================================
-- KMT.Trade_CheckBuy
-- ========================================

CREATE   PROCEDURE [KMT].[Trade_CheckBuy]
    @CharID INT,
    @Charname VARCHAR(25),
    @NpcID INT,
    @NpcCodename VARCHAR(128),
    @NpcTab TINYINT,
    @NpcSlot TINYINT,
    @Quantity SMALLINT,
    @IsBlocked BIT OUTPUT
AS
	/*
BEGIN
    SET NOCOUNT ON;

    DECLARE @LimitPerChar INT = 10;
    DECLARE @LimitPerHwid INT = 10;

    DECLARE @now DATETIME2(0) = SYSUTCDATETIME();

    DECLARE @SuccessCount INT = NULL,
            @LastSellSuccessAt DATETIME2(0) = NULL,
            @hwid NVARCHAR(128) = NULL;

    SET @IsBlocked = 0;

    BEGIN TRY
        BEGIN TRAN;

            SELECT @SuccessCount = SuccessCount,
                   @LastSellSuccessAt = LastSellSuccessAt,
                   @hwid = HWID
            FROM hema_system.dbo._TradeControl WITH (UPDLOCK, HOLDLOCK)
            WHERE CharID = @CharID;

            IF @SuccessCount IS NULL
            BEGIN
                SET @IsBlocked = 0;
                COMMIT TRAN;
                RETURN;
            END

            -- Backfill HWID if missing
            IF (@hwid IS NULL OR LTRIM(RTRIM(@hwid)) = N'')
            BEGIN
                SELECT TOP(1) @hwid = UPPER(LTRIM(RTRIM(CONVERT(NVARCHAR(128), Hwid))))
                FROM [KMT].[Auth_HWIDs] WITH (NOLOCK)
                WHERE CharID = @CharID AND Hwid IS NOT NULL AND LEN(Hwid) > 0
                ORDER BY Active DESC, ID DESC;

                IF @hwid IS NOT NULL
                BEGIN
                    UPDATE hema_system.dbo._TradeControl
                       SET HWID=@hwid, UpdatedAt=@now
                     WHERE CharID=@CharID;
                END
            END

            -- Rolling effective (char)
            DECLARE @charEffective INT;
            SET @charEffective =
                CASE WHEN @LastSellSuccessAt IS NULL OR DATEDIFF(SECOND, @LastSellSuccessAt, @now) >= 86400
                     THEN 0 ELSE @SuccessCount END;

            -- Rolling effective (HWID)
            DECLARE @hwidEffective INT;
            SET @hwidEffective = 0;

            IF (@hwid IS NOT NULL AND LTRIM(RTRIM(@hwid)) <> N'')
            BEGIN
                SELECT @hwidEffective = ISNULL(SUM(SuccessCount), 0)
                FROM hema_system.dbo._TradeControl WITH (NOLOCK)
                WHERE HWID = @hwid
                  AND LastSellSuccessAt IS NOT NULL
                  AND LastSellSuccessAt > DATEADD(SECOND, -86400, @now);
            END

            -- Basic limit block
            IF (@charEffective >= @LimitPerChar) OR (@hwidEffective >= @LimitPerHwid)
            BEGIN
                SET @IsBlocked = 1;

                DECLARE @notice VARCHAR(MAX);
                SET @notice = 'Trade limit reached (10/24h). Try later.';

                EXEC [KMT].[Command_NoticeByID] @CharID, 6, @notice;

                COMMIT TRAN;
                RETURN;
            END

            -- Party-aware block: Trader cannot buy if any Hunter in party reached limit
            DECLARE @PartyID INT, @BuyerJob TINYINT;
            DECLARE @JOB_TRADER TINYINT; SET @JOB_TRADER = 1;
            DECLARE @JOB_HUNTER TINYINT; SET @JOB_HUNTER = 3;

            SET @PartyID = NULL;
            SET @BuyerJob = NULL;

            SELECT @PartyID = PartyID, @BuyerJob = JobStatus
            FROM [KMT].[Party_Members] WITH (NOLOCK)
            WHERE CharID = @CharID;

            IF (@BuyerJob = @JOB_TRADER AND @PartyID IS NOT NULL)
            BEGIN
                DECLARE @BadHunterID INT;
                SET @BadHunterID = NULL;

                SELECT TOP(1) @BadHunterID = pd.CharID
                FROM [KMT].[Party_Members] pd WITH (NOLOCK)
                LEFT JOIN hema_system.dbo._TradeControl tc WITH (NOLOCK)
                       ON tc.CharID = pd.CharID
                WHERE pd.PartyID = @PartyID
                  AND pd.JobStatus = @JOB_HUNTER
                  AND (
                        CASE
                          WHEN tc.LastSellSuccessAt IS NULL THEN 0
                          WHEN DATEDIFF(SECOND, tc.LastSellSuccessAt, @now) >= 86400 THEN 0
                          ELSE tc.SuccessCount
                        END
                      ) >= @LimitPerChar;

                IF (@BadHunterID IS NOT NULL)
                BEGIN
                    SET @IsBlocked = 1;

                    DECLARE @notice2 VARCHAR(MAX);
                    SET @notice2 = 'Trade blocked: Hunter in your party reached limit (10/24h).';

                    EXEC [KMT].[Command_NoticeByID] @CharID, 6, @notice2;

                    COMMIT TRAN;
                    RETURN;
                END
            END

            SET @IsBlocked = 0;

        COMMIT TRAN;
    END TRY
    BEGIN CATCH
        IF @@TRANCOUNT > 0 ROLLBACK TRAN;
        SET @IsBlocked = 0;
    END CATCH
END
*/
GO

-- ========================================
-- KMT.Trade_CheckSell
-- ========================================

CREATE   PROCEDURE [KMT].[Trade_CheckSell]
    @CharID INT,
    @Charname VARCHAR(25),
    @Petid INT,
    @NpcID INT,
    @NpcCodename VARCHAR(128),
    @PetSlot TINYINT,
    @Quantity SMALLINT,
    @IsBlocked BIT OUTPUT
AS /*
BEGIN
    SET NOCOUNT ON;

    DECLARE @now DATETIME2(0) = SYSUTCDATETIME();

    SET @IsBlocked = 0;

    -- Block sell to same NPC you bought from
    DECLARE @lastBuyNpcID INT;

    SELECT @lastBuyNpcID = LastBuyNpcID
    FROM hema_system.dbo._TradeControl WITH (NOLOCK)
    WHERE CharID = @CharID;

    IF @lastBuyNpcID IS NOT NULL AND @lastBuyNpcID = @NpcID
    BEGIN
        SET @IsBlocked = 1;

        DECLARE @notice VARCHAR(MAX);
        SET @notice = 'You cannot sell to the same NPC you last bought from.';

        EXEC [KMT].[Command_NoticeByID] @CharID, 6, @notice;
        RETURN;
    END

    -- Party-aware block: Trader cannot sell if any Hunter in party reached limit
    DECLARE @PartyID INT, @SellerJob TINYINT;
    DECLARE @JOB_TRADER TINYINT; SET @JOB_TRADER = 1;
    DECLARE @JOB_HUNTER TINYINT; SET @JOB_HUNTER = 3;

    SET @PartyID = NULL;
    SET @SellerJob = NULL;

    SELECT @PartyID = PartyID, @SellerJob = JobStatus
    FROM [KMT].[Party_Members] WITH (NOLOCK)
    WHERE CharID = @CharID;

    IF (@SellerJob = @JOB_TRADER AND @PartyID IS NOT NULL)
    BEGIN
        DECLARE @BadHunterID INT;
        SET @BadHunterID = NULL;

        SELECT TOP(1) @BadHunterID = pd.CharID
        FROM [KMT].[Party_Members] pd WITH (NOLOCK)
        LEFT JOIN hema_system.dbo._TradeControl tc WITH (NOLOCK)
               ON tc.CharID = pd.CharID
        WHERE pd.PartyID = @PartyID
          AND pd.JobStatus = @JOB_HUNTER
          AND (
                CASE
                  WHEN tc.LastSellSuccessAt IS NULL THEN 0
                  WHEN DATEDIFF(SECOND, tc.LastSellSuccessAt, @now) >= 86400 THEN 0
                  ELSE tc.SuccessCount
                END
              ) >= 10;

        IF (@BadHunterID IS NOT NULL)
        BEGIN
            SET @IsBlocked = 1;

            DECLARE @notice2 VARCHAR(MAX);
            SET @notice2 = 'Trade blocked: Hunter in your party exceeded limit (10/24h).';

            EXEC [KMT].[Command_NoticeByID] @CharID, 6, @notice2;
            RETURN;
        END
    END

    SET @IsBlocked = 0;
END
*/
GO

-- ========================================
-- KMT.Trade_GiveBonus
-- ========================================

CREATE   PROCEDURE [KMT].[Trade_GiveBonus]
    @CharID     INT,
    @CharName   VARCHAR(64),
    @AmountGold BIGINT,
    @Reason     VARCHAR(64) = NULL
AS
BEGIN
    SET NOCOUNT ON;

    BEGIN TRY
        IF (@AmountGold IS NULL OR @AmountGold <= 0)
            RETURN;

        DECLARE @nowUTC DATETIME2(0) = SYSUTCDATETIME();
        DECLARE @today  DATE;

        -- Egypt date (fallback +2h Ù„Ùˆ AT TIME ZONE Ù…Ø´ Ù…ÙˆØ¬ÙˆØ¯)
        BEGIN TRY
            DECLARE @nowEG DATETIME2(0) =
                CONVERT(DATETIME2(0), (@nowUTC AT TIME ZONE 'UTC') AT TIME ZONE 'Egypt Standard Time');
            SET @today = CONVERT(DATE, @nowEG);
        END TRY
        BEGIN CATCH
            SET @today = CONVERT(DATE, DATEADD(MINUTE, 120, @nowUTC));
        END CATCH

        DECLARE @c INT = 0;

        BEGIN TRAN;

            IF EXISTS (SELECT 1 FROM hema_system.dbo.TradeBonus WITH (UPDLOCK, HOLDLOCK) WHERE CharID = @CharID)
            BEGIN
                DECLARE @d DATE, @cnt TINYINT;

                SELECT @d = TodayDate, @cnt = TodayCount
                FROM hema_system.dbo.TradeBonus WITH (UPDLOCK, HOLDLOCK)
                WHERE CharID = @CharID;

                IF (@d IS NULL OR @d <> @today)
                BEGIN
                    SET @cnt = 0;
                    UPDATE hema_system.dbo.TradeBonus
                       SET TodayDate = @today,
                           TodayCount = 0
                     WHERE CharID = @CharID;
                END

                IF (@cnt >= 5)
                BEGIN
                    COMMIT TRAN;

                    DECLARE @Name16 VARCHAR(16) = LEFT(@CharName, 16);
                    DECLARE @CapMsg VARCHAR(MAX) = 'Trade Bonus: daily limit reached (5/5). Try again tomorrow.';

                    -- âœ… positional call (NO named params)
                    EXEC [KMT].[Command_NoticeByName] @Name16, 6, @CapMsg;
                    RETURN;
                END

                UPDATE hema_system.dbo.TradeBonus
                   SET CharName       = @CharName,
                       TotalBonusGold = TotalBonusGold + @AmountGold,
                       TodayDate      = @today,
                       TodayCount     = TodayCount + 1,
                       LastBonusGold  = @AmountGold,
                       LastBonusAt    = @nowUTC,
                       UpdatedAt      = @nowUTC
                 WHERE CharID = @CharID;

                SET @c = @cnt + 1;
            END
            ELSE
            BEGIN
                INSERT INTO hema_system.dbo.TradeBonus
                    (CharID, CharName, TotalBonusGold, TodayDate, TodayCount, LastBonusGold, LastBonusAt, UpdatedAt)
                VALUES
                    (@CharID, @CharName, @AmountGold, @today, 1, @AmountGold, @nowUTC, @nowUTC);

                SET @c = 1;
            END

            -- âœ… Add gold (Async) Action_ID=21, Data3=1 add
            INSERT INTO [KMT].[Command_GameServerQueue](Action_ID, Data1, Data2, Data3)
            VALUES(21, @CharID, @AmountGold, 1);

        COMMIT TRAN;

        DECLARE @Name16Ok VARCHAR(16) = LEFT(@CharName, 16);

        DECLARE @msg VARCHAR(MAX) =
            'Trade Bonus: +' + CONVERT(VARCHAR(20), @AmountGold) +
            ' gold (' + CONVERT(VARCHAR(10), @c) + '/5 today)' +
            CASE WHEN @Reason IS NOT NULL THEN ' | ' + @Reason ELSE '' END;

        -- âœ… positional call (NO named params)
        EXEC [KMT].[Command_NoticeByName] @Name16Ok, 6, @msg;

    END TRY
    BEGIN CATCH
        IF @@TRANCOUNT > 0 ROLLBACK TRAN;

        DECLARE @Name16Err VARCHAR(16) = LEFT(@CharName, 16);
        DECLARE @err VARCHAR(4000) = ERROR_MESSAGE();
        DECLARE @errMsg VARCHAR(MAX) = 'Trade Bonus Error: ' + LEFT(@err, 200);

        -- âœ… positional call (NO named params)
        EXEC [KMT].[Command_NoticeByName] @Name16Err, 6, @errMsg;
    END CATCH
END

GO

-- ========================================
-- KMT.Trade_RollingBonus
-- ========================================

CREATE   PROCEDURE [KMT].[Trade_RollingBonus]
    @CharID    INT,
    @CharName  VARCHAR(25),
    @Gold      BIGINT,
    @JobStatus TINYINT = NULL,
    @PartyID   INT = NULL,
    @Reason    VARCHAR(64) = NULL
AS
BEGIN
    SET NOCOUNT ON;

    BEGIN TRY
        IF (@Gold IS NULL OR @Gold <= 0) RETURN;

        DECLARE @now DATETIME2(0) = SYSUTCDATETIME();
        DECLARE @hwid NVARCHAR(128);

        -- Backfill HWID for receiver
        SELECT TOP(1) @hwid = UPPER(LTRIM(RTRIM(CONVERT(NVARCHAR(128), Hwid))))
        FROM [KMT].[Auth_HWIDs] WITH (NOLOCK)
        WHERE CharID = @CharID AND Hwid IS NOT NULL AND LEN(Hwid) > 0
        ORDER BY Active DESC, ID DESC;

        BEGIN TRAN;

            IF NOT EXISTS (SELECT 1 FROM hema_system.dbo._TradeControl WITH (UPDLOCK, HOLDLOCK) WHERE CharID=@CharID)
            BEGIN
                INSERT INTO hema_system.dbo._TradeControl
                    (CharID, HWID, SuccessCount, LastSellSuccessAt, LastBuyNpcID, LastBuyAt, UpdatedAt,
                     BonusTotalGold, LastBonusGold, LastBonusAt, LastBonusReason, LastBonusPartyID, LastBonusJobStatus)
                VALUES
                    (@CharID, @hwid, 0, NULL, NULL, NULL, @now,
                     0, NULL, NULL, NULL, NULL, NULL);
            END
            ELSE
            BEGIN
                UPDATE hema_system.dbo._TradeControl
                   SET HWID = COALESCE(NULLIF(@hwid, N''), HWID),
                       UpdatedAt = @now
                 WHERE CharID = @CharID;
            END

            -- Add gold via official proc
            EXEC [KMT].[Live_Gold] @CharID, @Gold, 1;

            -- Log bonus inside TradeControl
            UPDATE hema_system.dbo._TradeControl
               SET BonusTotalGold      = BonusTotalGold + @Gold,
                   LastBonusGold       = @Gold,
                   LastBonusAt         = @now,
                   LastBonusReason     = @Reason,
                   LastBonusPartyID    = @PartyID,
                   LastBonusJobStatus  = @JobStatus,
                   UpdatedAt           = @now
             WHERE CharID = @CharID;

        COMMIT TRAN;

        -- Count this participation in rolling limit (so hunters can hit limit)
        EXEC [KMT].[Trade_RollingNext] @CharID;

        DECLARE @msg VARCHAR(MAX);
        SET @msg = 'Trade Bonus received.';
        IF (@Reason IS NOT NULL) SET @msg = @msg + ' ' + @Reason;

        EXEC [KMT].[Command_NoticeByID] @CharID, 6, @msg;

    END TRY
    BEGIN CATCH
        IF @@TRANCOUNT > 0 ROLLBACK TRAN;

        DECLARE @err VARCHAR(4000);
        DECLARE @errMsg VARCHAR(MAX);

        SET @err = ERROR_MESSAGE();
        SET @errMsg = 'Trade Bonus Error: ' + LEFT(@err, 200);

        EXEC [KMT].[Command_NoticeByID] @CharID, 6, @errMsg;
    END CATCH
END

GO

-- ========================================
-- KMT.Trade_RollingNext
-- ========================================

CREATE   PROCEDURE [KMT].[Trade_RollingNext]
    @CharID INT
AS
BEGIN
    SET NOCOUNT ON;

    DECLARE @now DATETIME2(0) = SYSUTCDATETIME();
    DECLARE @hwid NVARCHAR(128);

    SELECT TOP(1) @hwid = UPPER(LTRIM(RTRIM(CONVERT(NVARCHAR(128), Hwid))))
    FROM [KMT].[Auth_HWIDs] WITH (NOLOCK)
    WHERE CharID = @CharID AND Hwid IS NOT NULL AND LEN(Hwid) > 0
    ORDER BY Active DESC, ID DESC;

    BEGIN TRY
        BEGIN TRAN;

            IF EXISTS (SELECT 1 FROM hema_system.dbo._TradeControl WITH (UPDLOCK, HOLDLOCK) WHERE CharID=@CharID)
            BEGIN
                DECLARE @cnt INT, @last DATETIME2(0);

                SELECT @cnt = SuccessCount, @last = LastSellSuccessAt
                FROM hema_system.dbo._TradeControl WITH (UPDLOCK, HOLDLOCK)
                WHERE CharID=@CharID;

                -- de-dup within 2 sec
                IF @last IS NOT NULL AND DATEDIFF(SECOND, @last, @now) < 2
                BEGIN
                    UPDATE hema_system.dbo._TradeControl
                       SET UpdatedAt=@now,
                           HWID = COALESCE(NULLIF(@hwid, N''), HWID)
                     WHERE CharID=@CharID;

                    COMMIT TRAN;
                    RETURN;
                END

                IF @last IS NULL OR DATEDIFF(SECOND, @last, @now) >= 86400
                BEGIN
                    UPDATE hema_system.dbo._TradeControl
                       SET SuccessCount=1,
                           LastSellSuccessAt=@now,
                           UpdatedAt=@now,
                           HWID = COALESCE(NULLIF(@hwid, N''), HWID)
                     WHERE CharID=@CharID;
                END
                ELSE
                BEGIN
                    UPDATE hema_system.dbo._TradeControl
                       SET SuccessCount=@cnt + 1,
                           LastSellSuccessAt=@now,
                           UpdatedAt=@now,
                           HWID = COALESCE(NULLIF(@hwid, N''), HWID)
                     WHERE CharID=@CharID;
                END
            END
            ELSE
            BEGIN
                INSERT INTO hema_system.dbo._TradeControl
                    (CharID, HWID, SuccessCount, LastSellSuccessAt, LastBuyNpcID, LastBuyAt, UpdatedAt, BonusTotalGold)
                VALUES
                    (@CharID, @hwid, 1, @now, NULL, NULL, @now, 0);
            END

        COMMIT TRAN;
    END TRY
    BEGIN CATCH
        IF @@TRANCOUNT > 0 ROLLBACK TRAN;
    END CATCH
END

GO

-- ========================================
-- KMT.Trade_SellDone
-- ========================================

CREATE   PROCEDURE [KMT].[Trade_SellDone]
    @CharID INT,
    @Charname VARCHAR(25),
    @Petid INT,
    @NpcID INT,
    @NpcCodename VARCHAR(128),
    @PetSlot TINYINT,
    @Quantity SMALLINT
AS
/*
BEGIN

    SET NOCOUNT ON;

    DECLARE @now DATETIME2(0) = SYSUTCDATETIME();
    DECLARE @hwid NVARCHAR(128);

    -- Job mapping (your system)
    DECLARE @JOB_TRADER TINYINT; SET @JOB_TRADER = 1;
    DECLARE @JOB_THIEF  TINYINT; SET @JOB_THIEF  = 2;
    DECLARE @JOB_HUNTER TINYINT; SET @JOB_HUNTER = 3;

    -- Bonuses
    DECLARE @BONUS_TRADER_WITH_HUNTER BIGINT; SET @BONUS_TRADER_WITH_HUNTER = 20000000; -- 20M
    DECLARE @BONUS_HUNTER_WITH_TRADER BIGINT; SET @BONUS_HUNTER_WITH_TRADER = 35000000; -- 35M
    DECLARE @BONUS_THIEF_PARTY        BIGINT; SET @BONUS_THIEF_PARTY        = 10000000; -- 10M each
    DECLARE @BONUS_THIEF_SOLO         BIGINT; SET @BONUS_THIEF_SOLO         =  5000000; -- 5M

    SELECT TOP(1) @hwid = UPPER(LTRIM(RTRIM(CONVERT(NVARCHAR(128), Hwid))))
    FROM [KMT].[Auth_HWIDs] WITH (NOLOCK)
    WHERE CharID = @CharID AND Hwid IS NOT NULL AND LEN(Hwid) > 0
    ORDER BY Active DESC, ID DESC;

    BEGIN TRY
        -- Rolling Counter Update (exact style)
        BEGIN TRAN;

            IF EXISTS (SELECT 1 FROM hema_system.dbo._TradeControl WITH (UPDLOCK, HOLDLOCK) WHERE CharID = @CharID)
            BEGIN
                DECLARE @cnt INT, @last DATETIME2(0);

                SELECT @cnt = SuccessCount, @last = LastSellSuccessAt
                FROM hema_system.dbo._TradeControl WITH (UPDLOCK, HOLDLOCK)
                WHERE CharID = @CharID;

                IF @last IS NOT NULL AND DATEDIFF(SECOND, @last, @now) < 2
                BEGIN
                    UPDATE hema_system.dbo._TradeControl
                       SET UpdatedAt = @now,
                           HWID = COALESCE(NULLIF(@hwid, N''), HWID)
                     WHERE CharID = @CharID;

                    COMMIT TRAN;
                    RETURN;
                END

                IF @last IS NULL OR DATEDIFF(SECOND, @last, @now) >= 86400
                BEGIN
                    UPDATE hema_system.dbo._TradeControl
                       SET SuccessCount = 1,
                           LastSellSuccessAt = @now,
                           UpdatedAt = @now,
                           HWID = COALESCE(NULLIF(@hwid, N''), HWID)
                     WHERE CharID = @CharID;
                END
                ELSE
                BEGIN
                    UPDATE hema_system.dbo._TradeControl
                       SET SuccessCount = @cnt + 1,
                           LastSellSuccessAt = @now,
                           UpdatedAt = @now,
                           HWID = COALESCE(NULLIF(@hwid, N''), HWID)
                     WHERE CharID = @CharID;
                END
            END
            ELSE
            BEGIN
                INSERT INTO hema_system.dbo._TradeControl
                    (CharID, HWID, SuccessCount, LastSellSuccessAt, LastBuyNpcID, LastBuyAt, UpdatedAt, BonusTotalGold)
                VALUES
                    (@CharID, @hwid, 1, @now, NULL, NULL, @now, 0);
            END

            DECLARE @PartyID INT, @SellerJob TINYINT;
            SET @PartyID = NULL;
            SET @SellerJob = NULL;

            SELECT @PartyID = PartyID, @SellerJob = JobStatus
            FROM [KMT].[Party_Members] WITH (NOLOCK)
            WHERE CharID = @CharID;

        COMMIT TRAN;

        -- Bonus logic (outside locks)

        -- THIEF
        IF (@SellerJob = @JOB_THIEF)
        BEGIN
            IF (@PartyID IS NOT NULL AND EXISTS (
                    SELECT 1
                    FROM [KMT].[Party_Members] WITH (NOLOCK)
                    WHERE PartyID = @PartyID AND JobStatus = @JOB_THIEF AND CharID <> @CharID
                ))
            BEGIN
                DECLARE @TCharID INT, @TCharName VARCHAR(25);

                DECLARE curThieves CURSOR LOCAL FAST_FORWARD FOR
                    SELECT CharID, CharName
                    FROM [KMT].[Party_Members] WITH (NOLOCK)
                    WHERE PartyID = @PartyID AND JobStatus = @JOB_THIEF;

                OPEN curThieves;
                FETCH NEXT FROM curThieves INTO @TCharID, @TCharName;

                WHILE @@FETCH_STATUS = 0
                BEGIN
                    EXEC [KMT].[Trade_RollingBonus]
                        @CharID = @TCharID,
                        @CharName = @TCharName,
                        @Gold = @BONUS_THIEF_PARTY,
                        @JobStatus = @JOB_THIEF,
                        @PartyID = @PartyID,
                        @Reason = 'Thief Party Bonus (+10M)';

                    FETCH NEXT FROM curThieves INTO @TCharID, @TCharName;
                END

                CLOSE curThieves;
                DEALLOCATE curThieves;
            END
            ELSE
            BEGIN
                EXEC [KMT].[Trade_RollingBonus]
                    @CharID = @CharID,
                    @CharName = @Charname,
                    @Gold = @BONUS_THIEF_SOLO,
                    @JobStatus = @JOB_THIEF,
                    @PartyID = @PartyID,
                    @Reason = 'Thief Solo Bonus (+5M)';
            END

            RETURN;
        END

        -- Trader/Hunter require party
        IF (@PartyID IS NULL)
            RETURN;

        -- TRADER sells -> needs at least one hunter
        IF (@SellerJob = @JOB_TRADER)
        BEGIN
            IF EXISTS (SELECT 1 FROM [KMT].[Party_Members] WITH (NOLOCK) WHERE PartyID=@PartyID AND JobStatus=@JOB_HUNTER)
            BEGIN
                EXEC [KMT].[Trade_RollingBonus]
                    @CharID = @CharID,
                    @CharName = @Charname,
                    @Gold = @BONUS_TRADER_WITH_HUNTER,
                    @JobStatus = @JOB_TRADER,
                    @PartyID = @PartyID,
                    @Reason = 'Trader Bonus (+20M)';

                DECLARE @HCharID INT, @HCharName VARCHAR(25);

                DECLARE curHunters CURSOR LOCAL FAST_FORWARD FOR
                    SELECT CharID, CharName
                    FROM [KMT].[Party_Members] WITH (NOLOCK)
                    WHERE PartyID=@PartyID AND JobStatus=@JOB_HUNTER;

                OPEN curHunters;
                FETCH NEXT FROM curHunters INTO @HCharID, @HCharName;

                WHILE @@FETCH_STATUS = 0
                BEGIN
                    EXEC [KMT].[Trade_RollingBonus]
                        @CharID = @HCharID,
                        @CharName = @HCharName,
                        @Gold = @BONUS_HUNTER_WITH_TRADER,
                        @JobStatus = @JOB_HUNTER,
                        @PartyID = @PartyID,
                        @Reason = 'Hunter Bonus (+35M)';

                    FETCH NEXT FROM curHunters INTO @HCharID, @HCharName;
                END

                CLOSE curHunters;
                DEALLOCATE curHunters;
            END
            RETURN;
        END

        -- HUNTER sells (optional) -> only if trader exists
        IF (@SellerJob = @JOB_HUNTER)
        BEGIN
            IF EXISTS (SELECT 1 FROM [KMT].[Party_Members] WITH (NOLOCK) WHERE PartyID=@PartyID AND JobStatus=@JOB_TRADER)
            BEGIN
                EXEC [KMT].[Trade_RollingBonus]
                    @CharID = @CharID,
                    @CharName = @Charname,
                    @Gold = @BONUS_HUNTER_WITH_TRADER,
                    @JobStatus = @JOB_HUNTER,
                    @PartyID = @PartyID,
                    @Reason = 'Hunter Bonus (+35M)';
            END
        END

    END TRY
    BEGIN CATCH
        DECLARE @err VARCHAR(4000);
        DECLARE @msg VARCHAR(MAX);

        SET @err = ERROR_MESSAGE();
        SET @msg = 'Trade System Error: ' + LEFT(@err, 200);

        EXEC [KMT].[Command_NoticeByID] @CharID, 6, @msg;
    END CATCH
END
*/
GO

-- ========================================
-- KMT.UI_SaveMacro
-- ========================================
CREATE   PROCEDURE [KMT].[UI_SaveMacro]
    @CharID int,
    @AutoPotion bit,
    @AutoSkill bit,
    @AutoHunt bit,
	@AutoPickup bit,
	@AutoScroll bit
AS


    BEGIN
	SET NOCOUNT ON;
    IF EXISTS(Select 1 from [KMT].[Macro_Settings] with(nolock) where CharID = @CharID)
    BEGIN
    	UPDATE [KMT].[Macro_Settings] SET AutoPotion=@AutoPotion, AutoSkill=@AutoSkill, AutoHunt=@AutoHunt, AutoPickup=@AutoPickup, 
		AutoScroll=@AutoScroll where CharID = @CharID
    END
    ELSE
    BEGIN
    	INSERT INTO [KMT].[Macro_Settings] VALUES(@CharID, @AutoPotion, @AutoSkill, @AutoHunt, @AutoPickup, @AutoScroll)
    END
    END

GO

-- ========================================
-- KMT.UI_SavePlayer
-- ========================================
-- =============================================
-- Author:		<Author,,Name>
-- Create date: <Create Date,,>
-- Description:	<Description,,>
-- =============================================
CREATE   PROCEDURE [KMT].[UI_SavePlayer]
	-- Add the parameters for the stored procedure here
	@CharName16 VARCHAR(32),
	@Value bit
AS
BEGIN
	-- SET NOCOUNT ON added to prevent extra result sets from
	-- interfering with SELECT statements.
	SET NOCOUNT ON;
		IF EXISTS (Select 1 from [KMT].[Player_Settings] where CharName16 = @CharName16)
		BEGIN
			UPDATE [KMT].[Player_Settings] set HideItemInfo = @Value where CharName16 = @CharName16
		END
		ELSE
		BEGIN
		INSERT INTO [KMT].[Player_Settings] (CharName16, HideItemInfo) VALUES (@CharName16, @Value)
		END
END


GO

-- ========================================
-- KMT.UI_SavePotion
-- ========================================
-- =============================================
-- Author:		<Author,,Name>
-- Create date: <Create Date,,>
-- Description:	<Description,,>
-- =============================================
CREATE   PROCEDURE [KMT].[UI_SavePotion]
	-- Add the parameters for the stored procedure here
	@CharID int,
	@Slot tinyint,
	@Active bit,
	@Value tinyint

AS
BEGIN
	-- SET NOCOUNT ON added to prevent extra result sets from
	-- interfering with SELECT statements.
	SET NOCOUNT ON;
	IF EXISTS (SELECT 1 FROM [KMT].[Macro_AutoPotion] WHERE CharID = @CharID AND Slot = @Slot)
    UPDATE [KMT].[Macro_AutoPotion] SET Active = @Active, Value = @Value WHERE CharID = @CharID AND Slot = @Slot
    ELSE
    INSERT INTO [KMT].[Macro_AutoPotion] (CharID, Slot, Active, Value) VALUES (@CharID, @Slot, @Active, @Value)

END


GO

