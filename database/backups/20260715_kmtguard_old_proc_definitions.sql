-- ========================================
-- dbo._LotteryGold_Register_JT
-- ========================================

/* يسجّل اللاعب في Lottery Gold (EventID=6) ويخصم التذكرة.
   يُستدعى فقط من _OnEventRegister_EDIT.
*/
create PROCEDURE [dbo].[_LotteryGold_Register_JT]
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

    /* قفل لكل لاعب لمنع السباق/الدبل كليك */
    DECLARE @LockResource NVARCHAR(100) = N'LotteryGold_Reg_' + CONVERT(NVARCHAR(20), @CharID);
    DECLARE @lockRes INT;
    EXEC @lockRes = sys.sp_getapplock
        @Resource=@LockResource, @LockMode=N'Exclusive', @LockOwner=N'Session', @LockTimeout=5000;
    IF (@lockRes < 0) RETURN;

    BEGIN TRY
        /* 0) قراءة كنترول الحدث من Events */
        DECLARE @isOpen BIT, @s DATETIME2(3), @e DATETIME2(3), @cost BIGINT, @now DATETIME2(3)=SYSUTCDATETIME();
        SELECT @isOpen=IsOpen, @s=StartUtc, @e=EndUtc, @cost=TicketCost
        FROM [Events].[dbo].[_LotteryGold_Control] WITH (NOLOCK)
        WHERE EventID = 6;

        IF (@cost IS NULL) SET @cost = 10000000;

        /* السماح لو IsOpen=1 أو (@now بين Start..End) */
        IF ( ISNULL(@isOpen,0)=0 AND NOT (@s IS NOT NULL AND @e IS NOT NULL AND @now>=@s AND @now<@e) )
        BEGIN
            INSERT INTO JTGuard.._AsyncFilterCommands (CommandID, Data1, Data2, Data3, Status)
            VALUES (28, CONVERT(NVARCHAR(50), @CharID), @ClosedMsg, N'1', 1);
            GOTO _EXIT;
        END

        /* 1) لو مسجّل بالفعل في جدول Events → رسالة دبل */
        IF EXISTS (SELECT 1 FROM [Events].[dbo].[_LotteryGold_RegPlayers] WITH (NOLOCK) WHERE CharID=@CharID)
        BEGIN
            INSERT INTO JTGuard.._AsyncFilterCommands (CommandID, Data1, Data2, Data3, Status)
            VALUES (28, CONVERT(NVARCHAR(50), @CharID), @DupMsg, N'1', 1);
            GOTO _EXIT;
        END

        /* 2) تأكيد الرصيد من الشارد */
        DECLARE @CurrentGold BIGINT;
        SELECT @CurrentGold = C.RemainGold  -- غيّر الاسم لو عمودك مختلف
        FROM [SRO_VT_SHARD].[dbo].[_Char] AS C WITH (NOLOCK)
        WHERE C.CharID = @CharID;

        IF (@CurrentGold IS NULL OR @CurrentGold < @cost)
        BEGIN
            INSERT INTO JTGuard.._AsyncFilterCommands (CommandID, Data1, Data2, Data3, Status)
            VALUES (28, CONVERT(NVARCHAR(50), @CharID), @NoGoldMsg, N'1', 1);
            GOTO _EXIT;
        END

        /* 3) إدراج التسجيل أولًا في Events (نتعامل مع PK/Unique بشكل صريح) */
        BEGIN TRY
            INSERT INTO [Events].[dbo].[_LotteryGold_RegPlayers] (CharID, CharName, GoldDeducted)
            VALUES (@CharID, @CharName, @cost);
        END TRY
        BEGIN CATCH
            IF ERROR_NUMBER() IN (2627, 2601)
            BEGIN
                INSERT INTO JTGuard.._AsyncFilterCommands (CommandID, Data1, Data2, Data3, Status)
                VALUES (28, CONVERT(NVARCHAR(50), @CharID), @DupMsg, N'1', 1);
                GOTO _EXIT;
            END
            ELSE
            BEGIN
                DECLARE @Err1 NVARCHAR(MAX) = CONCAT(N'LotteryGold_Register_JT INSERT ERROR: ', ERROR_MESSAGE(),
                                      N' [', ERROR_NUMBER(), '/', ERROR_SEVERITY(), '/', ERROR_STATE(),
                                      N' @ line ', ERROR_LINE(), N']');
                PRINT @Err1;

                INSERT INTO JTGuard.._AsyncFilterCommands (CommandID, Data1, Data2, Data3, Status)
                VALUES (28, CONVERT(NVARCHAR(50), @CharID), @FailMsg, N'1', 1);
                GOTO _EXIT;
            END
        END CATCH

        /* 4) خصم الجولد عبر __LiveGold */
        BEGIN TRY
            EXEC JTGuard.dbo.__LiveGold @CharID=@CharID, @Gold=@cost, @AddOrRemove=0;  -- 0=Remove
        END TRY
        BEGIN CATCH
            /* فشل الخصم → نشيل التسجيل علشان الاتساق */
            DELETE FROM [Events].[dbo].[_LotteryGold_RegPlayers] WHERE CharID=@CharID;

            DECLARE @Err2 NVARCHAR(MAX) = CONCAT(N'LotteryGold_Register_JT DEDUCT ERROR: ', ERROR_MESSAGE(),
                                  N' [', ERROR_NUMBER(), '/', ERROR_SEVERITY(), '/', ERROR_STATE(),
                                  N' @ line ', ERROR_LINE(), N']');
            PRINT @Err2;

            INSERT INTO JTGuard.._AsyncFilterCommands (CommandID, Data1, Data2, Data3, Status)
            VALUES (28, CONVERT(NVARCHAR(50), @CharID), @FailMsg, N'1', 1);
            GOTO _EXIT;
        END CATCH

        /* 5) رسالة نجاح */
        INSERT INTO JTGuard.._AsyncFilterCommands (CommandID, Data1, Data2, Data3, Status)
        VALUES (28, CONVERT(NVARCHAR(50), @CharID), @OkMsg, N'1', 1);

    END TRY
    BEGIN CATCH
        DECLARE @Err NVARCHAR(MAX) = CONCAT(N'LotteryGold_Register_JT ERROR: ', ERROR_MESSAGE(),
                            N' [', ERROR_NUMBER(), '/', ERROR_SEVERITY(), '/', ERROR_STATE(),
                            N' @ line ', ERROR_LINE(), N']');
        PRINT @Err;

        INSERT INTO JTGuard.._AsyncFilterCommands (CommandID, Data1, Data2, Data3, Status)
        VALUES (28, CONVERT(NVARCHAR(50), @CharID), @FailMsg, N'1', 1);
    END CATCH

_EXIT:
    EXEC sys.sp_releaseapplock @Resource=@LockResource, @LockOwner=N'Session';
END

GO

-- ========================================
-- dbo._OnAutoEquipClick_EDIT
-- ========================================

CREATE PROCEDURE [dbo].[_OnAutoEquipClick_EDIT]
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

    -- اسحب بيانات اللاعب مرة واحدة
    SELECT
        @WID      = WorldID,
        @RegionID = LatestRegion,
        @X        = PosX,
        @Y        = PosY,
        @Z        = PosZ,
        @Level    = CurLevel
    FROM SRO_VT_SHARD.._Char WITH (NOLOCK)
    WHERE CharID = @CharID;

    -- لو CharID مش موجود
    IF @WID IS NULL
    BEGIN
        EXEC JTGuard.dbo._SendNoticeByCharId
             @CharID     = @CharID,
             @NoticeType = 3,
             @Notice     = 'Character data was not found. Please relog and try again.';
        RETURN;
    END

    --------------------------------------------------------------------
    -- NEW) لو لفل اللاعب أعلى من 90: متعملش Auto Equip وابعث مسدج
    --------------------------------------------------------------------
    IF @Level > 90
    BEGIN
        EXEC JTGuard.dbo._SendNoticeByCharId
             @CharID     = @CharID,
             @NoticeType = 3,
             @Notice     = 'Auto-Equip is available only up to level 90.';
        RETURN;
    END

    --------------------------------------------------------------------
    -- 1) استدعاء بروسيد الأوتو إيكويب في hema_system وتمرير الليفل الحالي
    --------------------------------------------------------------------
    EXEC hema_system.dbo._AutoEquipt
         @CharID = @CharID,
         @data2  = @Level;

    --------------------------------------------------------------------
    -- 2) (اختياري) إرسال أمر للـ GameServer (لو محتاج)
    -- سيبه زي ما هو لو مش محتاج
    --------------------------------------------------------------------
END

GO

-- ========================================
-- dbo._OnCharKillLog_EDIT
-- ========================================

CREATE PROCEDURE [dbo].[_OnCharKillLog_EDIT]
    @WorldID         int,
    @RegionID        int,
    @KillerCharID    int,
    @KillerCharName  varchar(25),  -- للتوافق فقط (غير مستخدم)
    @KillerPVPState  tinyint,
    @KillerJobStatus tinyint,
    @KillerGuildID   int,
    @KillerGuildName varchar(25),  -- غير مستخدم
    @DeadCharID      int,
    @DeadCharName    varchar(25),  -- غير مستخدم
    @DeadPVPState    tinyint,
    @DeadJobStatus   tinyint,
    @DeadGuildID     int,
    @DeadGuildName   varchar(25)   -- غير مستخدم
AS
BEGIN
    SET NOCOUNT ON;

    DECLARE @EventID int = 10;
    DECLARE @Active bit = 0, @Start datetime2(3), @End datetime2(3);

    /* تحقّق إن حدث LMS شغال حاليًا */
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

    /* لو الحدث مش شغال: اخرج بدون أي INSERT */
    IF (@Active <> 1 OR @Start IS NULL)
        RETURN;

    /* 1) سجل في CharKillLog — IDs فقط */
  

    /* 2) زوّد عدّاد القاتل في Events..Event_LMS_Deaths (UPsert لكل جولة) */
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
        -- تجاهل أخطاء العدّاد عشان ما نعطلش اللوج الأساسي
        -- لو عايز: أقدر أضيف لوج أخطاء هنا في جدول منفصل.
    END CATCH

    /* 3) رجّع المقتول للمدينة فورًا */
    BEGIN TRY
        EXEC JTGuard.dbo.__TeleportToTownbyCharID @CharID = @DeadCharID;
    END TRY
    BEGIN CATCH
        -- تجاهل أخطاء التلي بورت
    END CATCH
END

GO

-- ========================================
-- dbo._OnEventCancel_EDIT
-- ========================================

/* إلغاء تسجيل لاعب في جولة الإيفنت الحالية + ردّ الرسوم إن لزم (Gold/Silk عبر إجراءات Live) */
CREATE PROCEDURE [dbo].[_OnEventCancel_EDIT]
    @CharID        INT,
    @CharName16    VARCHAR(25),
    @EventID       SMALLINT,
    @LiveRegionID  INT,
    @WorldID       INT
AS
BEGIN
    SET NOCOUNT ON;
    SET XACT_ABORT ON;

    /* قفل على مستوى اللاعب/الإيفنت لمنع ازدواج الإلغاء/الرد */
    DECLARE @LockRes INT;
    DECLARE @LockResource NVARCHAR(100) = N'EventCancel_' + CONVERT(NVARCHAR(10), @EventID) + N'_' + CONVERT(NVARCHAR(20), @CharID);

    EXEC @LockRes = sys.sp_getapplock
        @Resource    = @LockResource,
        @LockMode    = N'Exclusive',
        @LockOwner   = N'Session',
        @LockTimeout = 5000;
    IF (@LockRes < 0) RETURN;

    BEGIN TRY
        /* 1) معلومات الجولة الحالية من Events..Event_Control */
        DECLARE @nowUtc DATETIME2(3) = SYSUTCDATETIME();
        DECLARE @isOpen BIT, @s DATETIME2(3), @e DATETIME2(3), @evtCode SYSNAME;

        SELECT
            @isOpen  = EC.IsOpen,
            @s       = EC.StartUtc,
            @e       = EC.EndUtc,
            @evtCode = EC.EventCode
        FROM [Events].[dbo].[Event_Control] AS EC WITH (NOLOCK)
        WHERE EC.EventID = @EventID;

        /* لو مفيش نافذة نشطة حاليًا: الإلغاء غير مسموح */
        IF (@s IS NULL OR @e IS NULL OR @nowUtc < @s OR @nowUtc >= @e)
        BEGIN
            INSERT INTO [JTGuard].._AsyncFilterCommands (CommandID, Data1, Data2, Data3, Status)
            VALUES (28, CONVERT(NVARCHAR(50), @CharID),
                    N'Event: cancellation is not available right now.', N'1', 1);
            GOTO _EXIT;
        END

        /* 2) صف تسجيل اللاعب في الجولة الحالية */
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
            INSERT INTO [JTGuard].._AsyncFilterCommands (CommandID, Data1, Data2, Data3, Status)
            VALUES (28, CONVERT(NVARCHAR(50), @CharID),
                    N'You are not registered for the current round.', N'1', 1);
            GOTO _EXIT;
        END

        /* 3) تنفيذ الإلغاء + رد الرسوم عند الحاجة داخل ترانزاكشن */
        BEGIN TRAN;

            IF (ISNULL(@amount,0) > 0)
            BEGIN
                /* Lottery Gold (EventID=6 أو Currency='gold') */
                IF (@EventID = 6 OR LOWER(@currency) = N'gold')
                BEGIN
                    BEGIN TRY
                        EXEC [JTGuard].[dbo].[__LiveGold]
                             @CharID=@CharID, @Gold=@amount, @AddOrRemove=1; -- 1 = Add (Refund)
                    END TRY
                    BEGIN CATCH
                        ROLLBACK TRAN;
                        INSERT INTO [JTGuard].._AsyncFilterCommands (CommandID, Data1, Data2, Data3, Status)
                        VALUES (28, CONVERT(NVARCHAR(50), @CharID),
                                N'Cancel failed: refund (gold) error. Try again later.', N'1', 1);
                        GOTO _EXIT;
                    END CATCH
                END
                /* Lottery Silk (EventID=5 أو Currency='silk') عبر __LiveSilk */
                ELSE IF (@EventID = 5 OR LOWER(@currency) = N'silk')
                BEGIN
                    /* __LiveSilk بياخد INT، فنتأكد إن المبلغ ضمن المدى */
                    IF (@amount < 0 OR @amount > 2147483647)
                    BEGIN
                        ROLLBACK TRAN;
                        INSERT INTO [JTGuard].._AsyncFilterCommands (CommandID, Data1, Data2, Data3, Status)
                        VALUES (28, CONVERT(NVARCHAR(50), @CharID),
                                N'Cancel failed: refund (silk) amount out of range.', N'1', 1);
                        GOTO _EXIT;
                    END

                    DECLARE @nSilkRefund INT = CAST(@amount AS INT);
                    DECLARE @rc INT;

                    EXEC @rc = [JTGuard].[dbo].[__LiveSilk]
                         @CharID     = @CharID,
                         @nSilk      = @nSilkRefund,  -- استرجاع
                         @nSilkGift  = 0,
                         @nSilkPoint = 0;

                    IF (@rc <> 0)
                    BEGIN
                        ROLLBACK TRAN;
                        INSERT INTO [JTGuard].._AsyncFilterCommands (CommandID, Data1, Data2, Data3, Status)
                        VALUES (28, CONVERT(NVARCHAR(50), @CharID),
                                N'Cancel failed: refund (silk) error.', N'1', 1);
                        GOTO _EXIT;
                    END
                END
                /* باقي الإيفنتات مجانية → لا حاجة لردّ */
            END

            /* احذف تسجيل اللاعب من الجولة */
            DELETE FROM [Events].[dbo].[Event_RegPlayers]
            WHERE EventID = @EventID
              AND EventStartUtc = @s
              AND CharID = @CharID;

        COMMIT TRAN;

        /* 4) رسالة نجاح */
        DECLARE @okMsg NVARCHAR(MAX) =
            CASE
                WHEN ISNULL(@amount,0) > 0 AND (LOWER(ISNULL(@currency,N'')) IN (N'gold', N'silk') OR @EventID IN (5,6))
                    THEN N'Registration canceled successfully. Refund: ' + CONVERT(NVARCHAR(30), @amount) + N' ' + ISNULL(@currency,N'coins') + N'.'
                ELSE N'Registration canceled successfully.'
            END;

        INSERT INTO [JTGuard].._AsyncFilterCommands (CommandID, Data1, Data2, Data3, Status)
        VALUES (28, CONVERT(NVARCHAR(50), @CharID), @okMsg, N'1', 1);

    END TRY
    BEGIN CATCH
        IF (XACT_STATE() <> 0) ROLLBACK TRAN;

        DECLARE @Err NVARCHAR(MAX) = CONCAT(
            N'_OnEventCancel_EDIT ERROR: ', ERROR_MESSAGE(),
            N' [', ERROR_NUMBER(), '/', ERROR_SEVERITY(), '/', ERROR_STATE(), N' @ ', ERROR_LINE(), N']'
        );

        INSERT INTO [JTGuard].._AsyncFilterCommands (CommandID, Data1, Data2, Data3, Status)
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
-- dbo._OnEventRegister_EDIT
-- ========================================

CREATE PROCEDURE [dbo].[_OnEventRegister_EDIT]
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
                INSERT INTO JTGuard.._AsyncFilterCommands(CommandID, Data1, Data2, Data3, Status)
                VALUES (28, CONVERT(NVARCHAR(50), @CharID), @MsgBF, N'1', 1);
            END CATCH;
            RETURN;
        END
        ELSE
        BEGIN
            INSERT INTO JTGuard.._AsyncFilterCommands(CommandID, Data1, Data2, Data3, Status)
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
                INSERT INTO JTGuard.._AsyncFilterCommands(CommandID, Data1, Data2, Data3, Status)
                VALUES (28, CONVERT(NVARCHAR(50), @CharID), @MsgLMS, N'1', 1);
            END CATCH;
            RETURN;
        END
        ELSE
        BEGIN
            INSERT INTO JTGuard.._AsyncFilterCommands(CommandID, Data1, Data2, Data3, Status)
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
                INSERT INTO JTGuard.._AsyncFilterCommands(CommandID, Data1, Data2, Data3, Status)
                VALUES (28, CONVERT(NVARCHAR(50), @CharID), @MsgSURV, N'1', 1);
            END CATCH;
            RETURN;
        END
        ELSE
        BEGIN
            INSERT INTO JTGuard.._AsyncFilterCommands(CommandID, Data1, Data2, Data3, Status)
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
            FROM JTGuard.dbo._PartyData WITH (NOLOCK)
            WHERE CharID = @CharID AND PartyID IS NOT NULL AND PartyID <> 0
        )
        BEGIN
            INSERT INTO JTGuard.._AsyncFilterCommands(CommandID, Data1, Data2, Data3, Status)
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
                INSERT INTO JTGuard.._AsyncFilterCommands(CommandID, Data1, Data2, Data3, Status)
                VALUES (28, CONVERT(NVARCHAR(50), @CharID), @MsgSVP, N'1', 1);
            END CATCH;
            RETURN;
        END
        ELSE
        BEGIN
            INSERT INTO JTGuard.._AsyncFilterCommands(CommandID, Data1, Data2, Data3, Status)
            VALUES (28, CONVERT(NVARCHAR(50), @CharID),
                    N'Survival party: Registration is temporarily unavailable.', N'1', 1);
            RETURN;
        END
    END

    
    -------------------------------------------------------------------------
    -- ... (الأجزاء القديمة: Lottery / BeastFury / LMS / Survival Solo / Survival Party)
    -- اعتبر إن البلوكات السابقة موجودة كما عندك، هنضيف في الآخر Tower Defender فقط.
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
                INSERT INTO JTGuard.._AsyncFilterCommands(CommandID, Data1, Data2, Data3, Status)
                VALUES (28, CONVERT(NVARCHAR(50), @CharID), @MsgTD, N'1', 1);
            END CATCH;
            RETURN;
        END
        ELSE
        BEGIN
            INSERT INTO JTGuard.._AsyncFilterCommands(CommandID, Data1, Data2, Data3, Status)
            VALUES (28, CONVERT(NVARCHAR(50), @CharID),
                    N'Tower Defender: Registration is temporarily unavailable.', N'1', 1);
            RETURN;
        END
    END
END

GO

-- ========================================
-- dbo._OnTradeGoodsBuyingRequest_EDIT
-- ========================================

CREATE PROCEDURE [dbo].[_OnTradeGoodsBuyingRequest_EDIT]
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
                FROM dbo._HwidList WITH (NOLOCK)
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

                EXEC JTGuard.dbo._SendNoticeByCharId @CharID, 6, @notice;

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
            FROM JTGuard.dbo._PartyData WITH (NOLOCK)
            WHERE CharID = @CharID;

            IF (@BuyerJob = @JOB_TRADER AND @PartyID IS NOT NULL)
            BEGIN
                DECLARE @BadHunterID INT;
                SET @BadHunterID = NULL;

                SELECT TOP(1) @BadHunterID = pd.CharID
                FROM JTGuard.dbo._PartyData pd WITH (NOLOCK)
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

                    EXEC JTGuard.dbo._SendNoticeByCharId @CharID, 6, @notice2;

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
-- dbo._OnTradeGoodsSellingComplete_EDIT
-- ========================================

CREATE PROCEDURE [dbo].[_OnTradeGoodsSellingComplete_EDIT]
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
    FROM dbo._HwidList WITH (NOLOCK)
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
            FROM JTGuard.dbo._PartyData WITH (NOLOCK)
            WHERE CharID = @CharID;

        COMMIT TRAN;

        -- Bonus logic (outside locks)

        -- THIEF
        IF (@SellerJob = @JOB_THIEF)
        BEGIN
            IF (@PartyID IS NOT NULL AND EXISTS (
                    SELECT 1
                    FROM JTGuard.dbo._PartyData WITH (NOLOCK)
                    WHERE PartyID = @PartyID AND JobStatus = @JOB_THIEF AND CharID <> @CharID
                ))
            BEGIN
                DECLARE @TCharID INT, @TCharName VARCHAR(25);

                DECLARE curThieves CURSOR LOCAL FAST_FORWARD FOR
                    SELECT CharID, CharName
                    FROM JTGuard.dbo._PartyData WITH (NOLOCK)
                    WHERE PartyID = @PartyID AND JobStatus = @JOB_THIEF;

                OPEN curThieves;
                FETCH NEXT FROM curThieves INTO @TCharID, @TCharName;

                WHILE @@FETCH_STATUS = 0
                BEGIN
                    EXEC JTGuard.dbo._TradeBonus_Grant_Rolling
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
                EXEC JTGuard.dbo._TradeBonus_Grant_Rolling
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
            IF EXISTS (SELECT 1 FROM JTGuard.dbo._PartyData WITH (NOLOCK) WHERE PartyID=@PartyID AND JobStatus=@JOB_HUNTER)
            BEGIN
                EXEC JTGuard.dbo._TradeBonus_Grant_Rolling
                    @CharID = @CharID,
                    @CharName = @Charname,
                    @Gold = @BONUS_TRADER_WITH_HUNTER,
                    @JobStatus = @JOB_TRADER,
                    @PartyID = @PartyID,
                    @Reason = 'Trader Bonus (+20M)';

                DECLARE @HCharID INT, @HCharName VARCHAR(25);

                DECLARE curHunters CURSOR LOCAL FAST_FORWARD FOR
                    SELECT CharID, CharName
                    FROM JTGuard.dbo._PartyData WITH (NOLOCK)
                    WHERE PartyID=@PartyID AND JobStatus=@JOB_HUNTER;

                OPEN curHunters;
                FETCH NEXT FROM curHunters INTO @HCharID, @HCharName;

                WHILE @@FETCH_STATUS = 0
                BEGIN
                    EXEC JTGuard.dbo._TradeBonus_Grant_Rolling
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
            IF EXISTS (SELECT 1 FROM JTGuard.dbo._PartyData WITH (NOLOCK) WHERE PartyID=@PartyID AND JobStatus=@JOB_TRADER)
            BEGIN
                EXEC JTGuard.dbo._TradeBonus_Grant_Rolling
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

        EXEC JTGuard.dbo._SendNoticeByCharId @CharID, 6, @msg;
    END CATCH
END
*/
GO

-- ========================================
-- dbo._OnTradeGoodsSellingRequest_EDIT
-- ========================================

CREATE PROCEDURE [dbo].[_OnTradeGoodsSellingRequest_EDIT]
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

        EXEC JTGuard.dbo._SendNoticeByCharId @CharID, 6, @notice;
        RETURN;
    END

    -- Party-aware block: Trader cannot sell if any Hunter in party reached limit
    DECLARE @PartyID INT, @SellerJob TINYINT;
    DECLARE @JOB_TRADER TINYINT; SET @JOB_TRADER = 1;
    DECLARE @JOB_HUNTER TINYINT; SET @JOB_HUNTER = 3;

    SET @PartyID = NULL;
    SET @SellerJob = NULL;

    SELECT @PartyID = PartyID, @SellerJob = JobStatus
    FROM JTGuard.dbo._PartyData WITH (NOLOCK)
    WHERE CharID = @CharID;

    IF (@SellerJob = @JOB_TRADER AND @PartyID IS NOT NULL)
    BEGIN
        DECLARE @BadHunterID INT;
        SET @BadHunterID = NULL;

        SELECT TOP(1) @BadHunterID = pd.CharID
        FROM JTGuard.dbo._PartyData pd WITH (NOLOCK)
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

            EXEC JTGuard.dbo._SendNoticeByCharId @CharID, 6, @notice2;
            RETURN;
        END
    END

    SET @IsBlocked = 0;
END
*/
GO

-- ========================================
-- dbo._OnUniqueEntered
-- ========================================

CREATE PROCEDURE [dbo].[_OnUniqueEntered]
    @RefObjID INT
AS
BEGIN
    SET NOCOUNT ON;

    ----------------------------------------------------
    -- الجزء الأصلي الخاص بـ JTGuard (سيبه زي ما هو)
    ----------------------------------------------------
    INSERT INTO _AsyncFilterCommands (CommandID, Data1, Status)
    VALUES (9999, @RefObjID, 1);

    ----------------------------------------------------
    -- ربط الـ Unique ببوت الديسكورد (رسالة للاعبين)
    ----------------------------------------------------
    DECLARE @CodeName    VARCHAR(128);
    DECLARE @DisplayName NVARCHAR(128);
    DECLARE @Msg         NVARCHAR(MAX);

    -- نجيب CodeName من DB اللعبة + DisplayName من جدول UniqueNames لو موجود
    SELECT TOP 1
        @CodeName    = RC.CodeName128,
        @DisplayName = UN.DisplayName
    FROM SRO_VT_SHARD.dbo._RefObjCommon RC WITH (NOLOCK)
    LEFT JOIN casy_discord.dbo.UniqueNames UN
        ON UN.CodeName = RC.CodeName128
    WHERE RC.ID = @RefObjID;

    -- لو لأي سبب الكود مش موجود
    IF (@CodeName IS NULL)
        SET @CodeName = 'UNKNOWN_UNIQUE';

    -- لو مفيش DisplayName في الجدول، خليها نفس الكود (بس المفروض عندك الكل)
    IF (@DisplayName IS NULL)
        SET @DisplayName = @CodeName;

    ----------------------------------------------------
    -- هنا شكل الرسالة اللي هتظهر في الديسكورد
    ----------------------------------------------------
    SET @Msg =
        N'🧿 **' + @DisplayName + N'** has spawned!' + CHAR(13) + CHAR(10) +
        N'Get ready, hunters!';

    -- لو حابب تmention رول معينة (مثال):
    -- SET @Msg = N'<@&ROLE_ID_HERE> ' + @Msg;

    ----------------------------------------------------
    -- إدخال الرسالة في جدول Messages
    ----------------------------------------------------
    INSERT INTO casy_discord.dbo.Messages (ChannelId, Message)
    VALUES ('1074805202059804782', @Msg);
END

GO

-- ========================================
-- dbo._OnUniqueKilled
-- ========================================

CREATE PROCEDURE [dbo].[_OnUniqueKilled]
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
    -- Callsك القديمة (Rank, Honor, Guild name sync ... إلخ) سيبها كما هي
    -------------------------------------------------------------------------
    EXEC hema_system.dbo.Unique_Rank
         @RefObjID   = @RefObjID,
         @KillerName = @KillerCharName;

    EXEC hema_system.dbo.HonorRank_Uniques
         @RefObjID   = @RefObjID,
         @KillerName = @KillerCharName;

    -- ... باقي التعديلات اللي عندك (GuildName sync + Achievements ...)
    EXEC dbo.[_HandleUniqueAchievements]
         @RefObjID   = @RefObjID,
         @KillerrName= @KillerCharName;

    INSERT INTO _AsyncFilterCommands(CommandID, Data1, Data2, Status)
    VALUES (40, 40, 40, 1);

    -------------------------------------------------------------------------
    -- Tower Defender – Tower kill detection
    -- RedTowerRefObjID / BlueTowerRefObjID لازم تكون مطابقة للأيفنت
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

        -- تحديد تيم القاتل
        SELECT @KillerTeam = cet.Team
        FROM JTGuard.dbo._CurrentEventTeamList AS cet WITH (NOLOCK)
        WHERE cet.CharName = @KillerCharName
          AND cet.EventName = 'Tower Defender';

        -- لو يسار Tower Blue مات → Red Team (1) كسبت
        IF (@RefObjID = @BlueTowerRefObjID)
        BEGIN
            SET @WinningTeam = 1;      -- Red
            SET @WinReason   = N'BlueTowerDown';
        END
        -- لو Tower Red مات → Blue Team (3) كسبت
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

            -- رسالة على السيرفر إن التاور وقع
            DECLARE @loserName NVARCHAR(16) =
                    CASE WHEN @WinningTeam = 1 THEN N'Blue' ELSE N'Red' END;
            DECLARE @winnerName NVARCHAR(16) =
                    CASE WHEN @WinningTeam = 1 THEN N'Red'  ELSE N'Blue' END;

            DECLARE @towerMsg NVARCHAR(MAX) =
                N'[ Tower Defender ] ' + @KillerCharName +
                N' has destroyed the ' + @loserName + N' Tower! ' +
                @winnerName + N' Team is now dominating the battlefield.';

            EXEC JTGuard.._SendNoticeToServer
                 @NoticeType = 2,
                 @Notice     = @towerMsg;
        END
    END
END

GO

-- ========================================
-- dbo._TradeBonus_Give
-- ========================================

CREATE   PROCEDURE dbo._TradeBonus_Give
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

        -- Egypt date (fallback +2h لو AT TIME ZONE مش موجود)
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

                    -- ✅ positional call (NO named params)
                    EXEC JTGuard.._SendNoticeByCharName16 @Name16, 6, @CapMsg;
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

            -- ✅ Add gold (Async) Action_ID=21, Data3=1 add
            INSERT INTO JTGuard.dbo._AsyncGameServerCommands(Action_ID, Data1, Data2, Data3)
            VALUES(21, @CharID, @AmountGold, 1);

        COMMIT TRAN;

        DECLARE @Name16Ok VARCHAR(16) = LEFT(@CharName, 16);

        DECLARE @msg VARCHAR(MAX) =
            'Trade Bonus: +' + CONVERT(VARCHAR(20), @AmountGold) +
            ' gold (' + CONVERT(VARCHAR(10), @c) + '/5 today)' +
            CASE WHEN @Reason IS NOT NULL THEN ' | ' + @Reason ELSE '' END;

        -- ✅ positional call (NO named params)
        EXEC JTGuard.._SendNoticeByCharName16 @Name16Ok, 6, @msg;

    END TRY
    BEGIN CATCH
        IF @@TRANCOUNT > 0 ROLLBACK TRAN;

        DECLARE @Name16Err VARCHAR(16) = LEFT(@CharName, 16);
        DECLARE @err VARCHAR(4000) = ERROR_MESSAGE();
        DECLARE @errMsg VARCHAR(MAX) = 'Trade Bonus Error: ' + LEFT(@err, 200);

        -- ✅ positional call (NO named params)
        EXEC JTGuard.._SendNoticeByCharName16 @Name16Err, 6, @errMsg;
    END CATCH
END

GO

-- ========================================
-- dbo._TradeBonus_Grant_Rolling
-- ========================================

CREATE PROCEDURE dbo._TradeBonus_Grant_Rolling
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
        FROM dbo._HwidList WITH (NOLOCK)
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
            EXEC JTGuard.dbo.__LiveGold @CharID, @Gold, 1;

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
        EXEC JTGuard.dbo._TradeRolling_BumpMember @CharID;

        DECLARE @msg VARCHAR(MAX);
        SET @msg = 'Trade Bonus received.';
        IF (@Reason IS NOT NULL) SET @msg = @msg + ' ' + @Reason;

        EXEC JTGuard.dbo._SendNoticeByCharId @CharID, 6, @msg;

    END TRY
    BEGIN CATCH
        IF @@TRANCOUNT > 0 ROLLBACK TRAN;

        DECLARE @err VARCHAR(4000);
        DECLARE @errMsg VARCHAR(MAX);

        SET @err = ERROR_MESSAGE();
        SET @errMsg = 'Trade Bonus Error: ' + LEFT(@err, 200);

        EXEC JTGuard.dbo._SendNoticeByCharId @CharID, 6, @errMsg;
    END CATCH
END

GO

