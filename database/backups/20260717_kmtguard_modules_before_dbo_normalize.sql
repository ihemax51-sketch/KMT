-- ========================================
-- dbo.Achievement_UniqueKill SQL_STORED_PROCEDURE
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
-- dbo.Event_RegisterGoldLottery SQL_STORED_PROCEDURE
-- ========================================

/* ÙŠØ³Ø¬Ù‘Ù„ Ø§Ù„Ù„Ø§Ø¹Ø¨ ÙÙŠ Lottery Gold (EventID=6) ÙˆÙŠØ®ØµÙ… Ø§Ù„ØªØ°ÙƒØ±Ø©.
   ÙŠÙØ³ØªØ¯Ø¹Ù‰ ÙÙ‚Ø· Ù…Ù† _OnEventRegister_EDIT.
*/
CREATE PROCEDURE [dbo].[Event_RegisterGoldLottery]
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
-- dbo.Hook_EventCancel SQL_STORED_PROCEDURE
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
-- dbo.Hook_EventRegister SQL_STORED_PROCEDURE
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
-- dbo.Hook_PartyJoin SQL_STORED_PROCEDURE
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
-- dbo.Hook_SelectScroll SQL_STORED_PROCEDURE
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
-- dbo.Live_AddBuff SQL_STORED_PROCEDURE
-- ========================================
CREATE   PROCEDURE [KMT].[Live_AddBuff]
	@CharID int,
	@SkillCodeName varchar(200)
AS		

INSERT INTO [KMT].[Command_GameServerQueue](Action_ID, Data1, Data2) VALUES(8, @CharID, @SkillCodeName)


GO

-- ========================================
-- dbo.Live_AddBuffNoLimit SQL_STORED_PROCEDURE
-- ========================================
CREATE   PROCEDURE [KMT].[Live_AddBuffNoLimit]
	@CharID int,
	@SkillID int
AS		

INSERT INTO [KMT].[Command_GameServerQueue](Action_ID, Data1, Data2) VALUES(6, @CharID, @SkillID)


GO

-- ========================================
-- dbo.Live_Cape SQL_STORED_PROCEDURE
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
-- dbo.Live_ChangeItem SQL_STORED_PROCEDURE
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
-- dbo.Live_Gold SQL_STORED_PROCEDURE
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
-- dbo.Live_RemoveBuff SQL_STORED_PROCEDURE
-- ========================================
CREATE   PROCEDURE [KMT].[Live_RemoveBuff]
		@CharID int,
	@SkillID int
AS		

INSERT INTO [KMT].[Command_GameServerQueue](Action_ID, Data1, Data2) VALUES(7, @CharID, @SkillID)

GO

-- ========================================
-- dbo.Live_Silk SQL_STORED_PROCEDURE
-- ========================================
CREATE PROCEDURE [dbo].[Live_Silk]
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
-- dbo.Live_Skill SQL_STORED_PROCEDURE
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
-- dbo.Live_Teleport SQL_STORED_PROCEDURE
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
-- dbo.Live_UseAndChangeItem SQL_STORED_PROCEDURE
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
-- dbo.Live_UseItem SQL_STORED_PROCEDURE
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
-- dbo.Log_Character SQL_STORED_PROCEDURE
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
-- dbo.Style_ChangeGlow SQL_STORED_PROCEDURE
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
-- dbo.Style_ChangeModel SQL_STORED_PROCEDURE
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
-- dbo.Trade_RollingBonus SQL_STORED_PROCEDURE
-- ========================================

CREATE PROCEDURE [dbo].[Trade_RollingBonus]
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
-- dbo.vw_Settings_InvalidValues VIEW
-- ========================================

CREATE   VIEW [dbo].[vw_Settings_InvalidValues]
AS
WITH typed AS
(
    SELECT
        SettingName,
        Value,
        CASE
            WHEN SettingName IN
            (
                'AccountDB', 'ShardDB', 'FacebookURL', 'DiscordURL', 'WebsiteURL', 'ServerName', 'CaptchaValue'
            ) THEN 'string'
            WHEN SettingName IN
            (
                'MasteryLimit', 'ServerMaxLevel', 'FakePlayerCount', 'HWID_LIMIT', 'HWID_JOB_LIMIT',
                'AlchemyItemLinkMinLevel', 'ReverseDelay', 'MaxPlus', 'AutoAttackMaxLevel', 'StallDelay',
                'StallLevel', 'ExchangeDelay', 'ExchangeLevel', 'GuildInviteDelay', 'UnionInviteDelay',
                'GlobalDelay', 'GlobalLevel', 'LiveItemDelay', 'TradePetSpawnDelay', 'RestartDelay',
                'ExitDelay', 'ItemTranslationPayment', 'ItemTranslationPrice', 'SHOW_CHAR_INFO_DELAY',
                'IPLimit', 'MaxPlusDevil', 'LuckySpinPrice', 'TradeSellCaptchaTimeoutSeconds',
                'TradeSellCaptchaMaxAttempts'
            ) THEN 'int'
            ELSE 'bool'
        END AS ExpectedType
    FROM [KMT].[System_Settings]
)
SELECT SettingName, Value, ExpectedType
FROM typed
WHERE
    (ExpectedType = 'bool' AND Value NOT IN ('True', 'False'))
    OR (ExpectedType = 'int' AND TRY_CONVERT(INT, Value) IS NULL);

GO

