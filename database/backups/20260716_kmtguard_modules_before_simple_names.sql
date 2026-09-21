USE [KMTGuard];
GO
-- Generated before simplifying KMTGuard table and procedure names.
-- The verified COPY_ONLY database backup is the authoritative rollback source.

SET ANSI_NULLS ON;
GO
SET QUOTED_IDENTIFIER ON;
GO

CREATE   PROCEDURE [achievements].[AddCharacter]
    @CharID INT
AS
BEGIN
    SET NOCOUNT ON;
    SET XACT_ABORT ON;

    -- تحقق من الـ CharID
    IF (@CharID IS NULL OR @CharID <= 0)
        RETURN -100;

    ---------------------------------------------------------------------
    -- جمع كل الـ RefAchievement + RefConditions المفعّلة (Service <> 0)
    ---------------------------------------------------------------------
    DECLARE @AchievementData TABLE
    (
        RefAchievementID INT,
        RefConditionID   INT
    );

    INSERT @AchievementData (RefAchievementID, RefConditionID)
    SELECT RAC.RefAchievementID, RAC.ID
    FROM dbo._RefAchievementCondition AS RAC
    JOIN dbo._RefAchievement AS RA
      ON RA.ID = RAC.RefAchievementID
     AND RA.Service <> 0;

    IF @@ERROR <> 0 
        RETURN -1;

    IF NOT EXISTS (SELECT 1 FROM @AchievementData)
        RETURN 2;

    ---------------------------------------------------------------------
    -- TRY/CATCH مع ترانزاكشن
    ---------------------------------------------------------------------
    BEGIN TRY
        BEGIN TRAN;

        -- خريطة بين RefAchievementID والـ ID الفعلي في جدول _Achievement
        DECLARE @Map TABLE
        (
            RefAchievementID INT PRIMARY KEY,
            AchievementID    INT
        );

        -----------------------------------------------------------------
        -- الموجودين أصلًا للشخصية
        -----------------------------------------------------------------
        INSERT INTO @Map (RefAchievementID, AchievementID)
        SELECT A.RefAchievementID, A.ID
        FROM dbo._Achievement AS A WITH (UPDLOCK, HOLDLOCK)
        JOIN @AchievementData AS AD
          ON AD.RefAchievementID = A.RefAchievementID
        WHERE A.CharID = @CharID;

        -----------------------------------------------------------------
        -- إدخال الناقص في _Achievement + تجميع IDs المُضافة
        -----------------------------------------------------------------
        INSERT dbo._Achievement (CharID, RefAchievementID, State)
        OUTPUT inserted.RefAchievementID, inserted.ID 
            INTO @Map (RefAchievementID, AchievementID)
        SELECT @CharID, AD.RefAchievementID, 0
        FROM @AchievementData AS AD
        WHERE NOT EXISTS (
            SELECT 1
            FROM dbo._Achievement AS A
            WHERE A.CharID = @CharID
              AND A.RefAchievementID = AD.RefAchievementID
        )
        GROUP BY AD.RefAchievementID;

        -----------------------------------------------------------------
        -- إدخال شروط الإنجاز مع الـ AchievementID الصحيح
        -----------------------------------------------------------------
        INSERT dbo._AchievementCondition
              (CharID, AchievementID, RefAchievementConditionID, ProgressCount)
        SELECT @CharID, M.AchievementID, AD.RefConditionID, 0
        FROM @AchievementData AS AD
        JOIN @Map AS M
          ON M.RefAchievementID = AD.RefAchievementID
        WHERE NOT EXISTS (
            SELECT 1
            FROM dbo._AchievementCondition AS AC
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

SET ANSI_NULLS ON;
GO
SET QUOTED_IDENTIFIER ON;
GO

CREATE   PROCEDURE [achievements].[HandleUniqueKill]
    @RefObjID    INT,
    @KillerrName VARCHAR(16)
AS
BEGIN
    SET NOCOUNT ON;

    ----------------------------------------------------
    -- 1) تنظيف الاسم + تحقق مبدئي من البراميترز
    ----------------------------------------------------
    SET @KillerrName = LTRIM(RTRIM(@KillerrName));

    IF (@RefObjID IS NULL OR @KillerrName IS NULL OR @KillerrName = '')
        RETURN;

    ----------------------------------------------------
    -- 2) جلب CharID من الشارد بالاسم (شخصية القاتل)
    --    TOP(1) مع ORDER BY لتجنب اختيار عشوائي لو فيه
    --    أكثر من شخصية بنفس الاسم لأي سبب
    ----------------------------------------------------
    DECLARE @CharID INT;

    SELECT TOP (1) 
           @CharID = C.CharID
    FROM   SRO_VT_SHARD.._Char AS C WITH (NOLOCK)
    WHERE  C.CharName16 COLLATE DATABASE_DEFAULT = @KillerrName COLLATE DATABASE_DEFAULT
    ORDER BY C.CharID DESC;   -- آخر CharID (تقديرياً أحدث واحدة)

    IF (@CharID IS NULL)
        RETURN;  -- اسم الشخصية مش موجود

    ----------------------------------------------------
    -- 3) نقطة ثابتة لكل قتلة (تقدر تعدلها لاحقاً لو حبيت)
    ----------------------------------------------------
    DECLARE @PointGive INT = 1;

    ----------------------------------------------------
    -- 4) تجهيز جدول مؤقت يربط RefObjID بالـ ConditionID
    --    بحيث:
    --    - نقرأ IDs مرة واحدة من _RefObjCommon
    --    - ندعم أكتر من CodeName لنفس الـ Unique
    --    - نطلّع ConditionID على حسب @RefObjID
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

    -- Captain Ivy (أكتر من CodeName لنفس الـ Unique)
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
    -- 5) تحديد الـ ConditionID بناءً على @RefObjID
    ----------------------------------------------------
    DECLARE @ConditionID INT;

    SELECT @ConditionID = U.ConditionID
    FROM   @Uniques AS U
    WHERE  U.RefObjID = @RefObjID;

    -- لو الـ RefObjID مش من ضمن الـ uniques اللي حاططها، مفيش إنجاز يتحدث
    IF (@ConditionID IS NULL)
        RETURN;

    ----------------------------------------------------
    -- 6) تحديث الإنجاز للشخصية اللي قتلت فقط
    --    NOTE: تأكد إن dbo._UpdateAchievement مبني على CharID
    --          مش على JID لو عايز الإنجازات تكون per-char
    ----------------------------------------------------
    EXEC [achievements].[UpdateProgress] 
        @CharID,          -- CharID الخاص بالشخصية القاتلة
        @ConditionID,     -- AchievementCondition (نفسه min/max)
        @ConditionID,     -- AchievementCondition
        @PointGive;       -- النقاط المعطاة

    ----------------------------------------------------
    -- 7) إدخال أمر في جدول الـ AsyncFilterCommands
    --    لو عندك تصميم معيّن لبيانات Data1/Data2 غير ثابتة
    --    تقدر تستخدم @CharID أو @ConditionID بدل الـ 40 الثابتة
    ----------------------------------------------------
    INSERT INTO dbo._AsyncFilterCommands (CommandID, Data1, Data2, Status)
    VALUES (40, 40, 40, 1);
    -- مثال بديل لو حبيت تستخدم معلومات حقيقية:
    -- VALUES (40, @CharID, @ConditionID, 1);

END

GO

SET ANSI_NULLS ON;
GO
SET QUOTED_IDENTIFIER ON;
GO

CREATE   PROCEDURE [achievements].[HandleUniqueKillByRefId]
    @RefObjID        INT,
    @KillerCharName  VARCHAR(16)
AS
BEGIN
    SET NOCOUNT ON;

    -- سياق أساسي
    DECLARE @CharID INT = (
        SELECT CharID 
        FROM SRO_VT_SHARD.._Char 
        WHERE CharName16 = @KillerCharName
    );
    IF @CharID IS NULL RETURN;

    ---------------------------------------------------------
    -- ✅ T i g e r  G i r l  (إنجاز فقط)
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
        EXEC [achievements].[UpdateProgress] 
            @CharID,
            @TigerAchievementCondition,
            @TigerAchievementCondition,
            @AchievementKillPoint_Tiger;

        INSERT INTO _AsyncFilterCommands(CommandID, Data1, Data2, Status)
        VALUES (40, 40, 40, 1);
    END

    ---------------------------------------------------------
    -- ✅ U r u c h i  (إنجاز فقط)
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
        EXEC [achievements].[UpdateProgress] 
            @CharID,
            @UruchiAchievementCondition,
            @UruchiAchievementCondition,
            @AchievementKillPoint_Uruchi;

        INSERT INTO _AsyncFilterCommands(CommandID, Data1, Data2, Status)
        VALUES (40, 40, 40, 1);
    END

    ---------------------------------------------------------
    -- ✅ I s y u t a r u  (إنجاز فقط)
    ---------------------------------------------------------
    DECLARE @UniqueIsyutaru INT = (
        SELECT ID 
        FROM SRO_VT_SHARD.._RefObjCommon 
        WHERE CodeName128 LIKE 'MOB_KK_ISYUTARU'
    );
    DECLARE @IsyutaruAchievementCondition INT = 5;
    DECLARE @AchievementKillPoint_Isyutaru INT = 5;  -- زي ما كنت كاتب

    IF (@RefObjID = @UniqueIsyutaru)
    BEGIN
        EXEC [achievements].[UpdateProgress] 
            @CharID,
            @IsyutaruAchievementCondition,
            @IsyutaruAchievementCondition,
            @AchievementKillPoint_Isyutaru;

        INSERT INTO _AsyncFilterCommands(CommandID, Data1, Data2, Status)
        VALUES (40, 40, 40, 1);
    END
END

GO

SET ANSI_NULLS ON;
GO
SET QUOTED_IDENTIFIER ON;
GO

CREATE   PROCEDURE [achievements].[UpdateProgress]
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
    -- لو مفيش Achievement للكاركتر ده – نطلع مباشرة
    ---------------------------------------------------------------------
    IF NOT EXISTS (
        SELECT 1 
        FROM dbo._Achievement 
        WHERE CharID = @CharID 
          AND RefAchievementID = @RefAchievementID
    )
        RETURN;

    ---------------------------------------------------------------------
    -- لو مفيش Condition للكاركتر ده – نطلع مباشرة
    ---------------------------------------------------------------------
    IF NOT EXISTS (
        SELECT 1 
        FROM dbo._AchievementCondition 
        WHERE CharID = @CharID 
          AND RefAchievementConditionID = @RefAchievementConditionID
    )
        RETURN;

    ---------------------------------------------------------------------
    -- قراءات أساسية من جداول الـ Ref والجداول الحقيقية
    ---------------------------------------------------------------------
    SELECT 
        @RefAchievementRewardType = RewardType
    FROM dbo._RefAchievement
    WHERE ID = @RefAchievementID;

    SELECT 
        @CompleteCount = CompleteCount
    FROM dbo._RefAchievementCondition
    WHERE ID = @RefAchievementConditionID;

    SELECT 
        @CurrentCompleteCount = ProgressCount
    FROM dbo._AchievementCondition
    WHERE CharID = @CharID
      AND RefAchievementConditionID = @RefAchievementConditionID;

    SELECT 
        @AchievementState = State
    FROM dbo._Achievement
    WHERE CharID = @CharID
      AND RefAchievementID = @RefAchievementID;

    ---------------------------------------------------------------------
    -- حماية بسيطة لو القيم NULL أو الـ Progress <= 0
    ---------------------------------------------------------------------
    SET @CurrentCompleteCount = ISNULL(@CurrentCompleteCount, 0);
    SET @CompleteCount        = ISNULL(@CompleteCount, 0);
    SET @Progress             = ISNULL(@Progress, 0);

    IF (@Progress <= 0 OR @CompleteCount <= 0)
        RETURN;

    ---------------------------------------------------------------------
    -- حساب التقدم الجديد مع الكاب عند CompleteCount
    ---------------------------------------------------------------------
    SET @NewProgress = @CurrentCompleteCount + @Progress;

    IF (@NewProgress > @CompleteCount)
        SET @NewProgress = @CompleteCount;

    ---------------------------------------------------------------------
    -- تحديث الـ Condition
    ---------------------------------------------------------------------
    UPDATE dbo._AchievementCondition
    SET ProgressCount = @NewProgress
    WHERE CharID = @CharID
      AND RefAchievementConditionID = @RefAchievementConditionID;

    ---------------------------------------------------------------------
    -- هل كل الـ Conditions الخاصة بالـ Achievement اكتملت؟
    ---------------------------------------------------------------------
    IF NOT EXISTS (
        SELECT 1
        FROM dbo._AchievementCondition AC
        JOIN dbo._RefAchievementCondition RAC 
             ON AC.RefAchievementConditionID = RAC.ID
        WHERE AC.CharID = @CharID
          AND RAC.RefAchievementID = @RefAchievementID
          AND AC.ProgressCount < RAC.CompleteCount
    )
    BEGIN
        -- كل الشروط خلصت
        IF (@AchievementState = 0)
        BEGIN
            IF (@RefAchievementRewardType = 0)
            BEGIN
                -- حالة 1: Achievement بدون Reward أو نوع 0
                UPDATE dbo._Achievement
                SET State = 1
                WHERE CharID = @CharID
                  AND RefAchievementID = @RefAchievementID;

                EXEC [commands].[SendNoticeByCharacterId] @CharID, 8, 'Achievement Completed.';

                INSERT INTO dbo._AsyncFilterCommands
                    (CommandID, Data1, Data2, Data3, Data4, Data5, Status)
                VALUES
                    (29, @CharID, @RefAchievementID, @RefAchievementConditionID, @NewProgress, 1, 1);
            END
            ELSE
            BEGIN
                -- حالة 2: Achievement له نوع Reward مختلف
                UPDATE dbo._Achievement
                SET State = 2
                WHERE CharID = @CharID
                  AND RefAchievementID = @RefAchievementID;

                EXEC [commands].[SendNoticeByCharacterId] @CharID, 8, 'Achievement Completed.';

                INSERT INTO dbo._AsyncFilterCommands
                    (CommandID, Data1, Data2, Data3, Data4, Data5, Status)
                VALUES
                    (29, @CharID, @RefAchievementID, @RefAchievementConditionID, @NewProgress, 2, 1);
            END
        END
        -- لو AchievementState مش 0 (يعني متكمل قبل كده)، مش هنغير حاجة تاني
    END
    ELSE
    BEGIN
        -----------------------------------------------------------------
        -- لسه في Conditions ناقصة – نخلي الـ Achievement State = 0
        -----------------------------------------------------------------
        UPDATE dbo._Achievement
        SET State = 0
        WHERE CharID = @CharID
          AND RefAchievementID = @RefAchievementID;

        INSERT INTO dbo._AsyncFilterCommands
            (CommandID, Data1, Data2, Data3, Data4, Data5, Status)
        VALUES
            (29, @CharID, @RefAchievementID, @RefAchievementConditionID, @NewProgress, 0, 1);
    END

    SET NOCOUNT OFF;
END

GO

SET ANSI_NULLS ON;
GO
SET QUOTED_IDENTIFIER ON;
GO
CREATE   PROCEDURE [appearance].[AddIcon]
	@CharID int,
	@IconID int,
	@Side tinyint
AS		


IF NOT EXISTS(SELECT 1 FROM _CharacterIconManager WITH(NOLOCK) WHERE  CharID = @CharID AND IconID = @IconID AND Side = @Side)
	BEGIN
		DECLARE @NewID INT;
		INSERT INTO _CharacterIconManager VALUES(@CharID, @IconID, @Side)
		SET @NewID = SCOPE_IDENTITY();
		INSERT INTO _AsyncFilterCommands(CommandID, Data1, Data2,Data3,Data4, Status) VALUES(6, @CharID, @IconID, @Side, @NewID, 1)
END

GO

SET ANSI_NULLS ON;
GO
SET QUOTED_IDENTIFIER ON;
GO
CREATE   PROCEDURE [appearance].[AddTitle]
	@CharID int,
	@TitleID tinyint
AS		


IF NOT EXISTS(SELECT 1 FROM _CharacterTitleManager WHERE CharID = @CharID AND TitleID = @TitleID)
BEGIN
	INSERT INTO _CharacterTitleManager VALUES(@CharID, @TitleID)
	INSERT INTO _AsyncFilterCommands(CommandID, Data1, Data2, Status) VALUES(4, @CharID, @TitleID, 1)
END

GO

SET ANSI_NULLS ON;
GO
SET QUOTED_IDENTIFIER ON;
GO
CREATE   PROCEDURE [appearance].[AddTitleColor]
	@CharID int,
	@ColorCode varchar(100),
	@ColorName varchar(100)
AS		


IF NOT EXISTS(SELECT 1 FROM _CharacterTitleManagerColor WITH(NOLOCK) WHERE CharID = @CharID AND ColorCode = @ColorCode AND ColorName = @ColorName)
	BEGIN
		INSERT INTO _CharacterTitleManagerColor (CharID, ColorName, ColorCode) VALUES (@CharID, @ColorName, @ColorCode)
		DECLARE @NewID INT;
		SET @NewID = SCOPE_IDENTITY();
		INSERT INTO _AsyncFilterCommands(CommandID, Data1, Data2, Data3, Data4, Status) VALUES(5, @CharID, @ColorName, @ColorCode, @NewID, 1)
END

GO

SET ANSI_NULLS ON;
GO
SET QUOTED_IDENTIFIER ON;
GO
CREATE   PROCEDURE [appearance].[RemoveLeftIcon]
	@CharName16 as varchar(16)
AS		

	IF EXISTS(SELECT IconID From _ActiveIconsLeftSide with(nolock) WHERE CharName16 = @CharName16)
	BEGIN
		DELETE FROM _ActiveIconsLeftSide  WHERE CharName16 = @CharName16
		INSERT INTO _AsyncFilterCommands(CommandID, Data1, Status) VALUES(12, @CharName16, 1)
	END


GO

SET ANSI_NULLS ON;
GO
SET QUOTED_IDENTIFIER ON;
GO
CREATE   PROCEDURE [appearance].[RemoveNameColor]
	@CharName16 varchar(16)
AS		


IF EXISTS(SELECT 1 FROM _ActiveNameColors WITH(NOLOCK) WHERE  CharName16 = @CharName16)
	BEGIN
		DELETE FROM _ActiveNameColors where CharName16 = @CharName16
		INSERT INTO _AsyncFilterCommands(CommandID, Data1, Status) VALUES(38, @CharName16, 1)
END


GO

SET ANSI_NULLS ON;
GO
SET QUOTED_IDENTIFIER ON;
GO
CREATE   PROCEDURE [appearance].[RemoveRightIcon]
	@CharName16 as varchar(16)
AS		

	IF EXISTS(SELECT IconID From _ActiveIconsRightSide with(nolock) WHERE CharName16 = @CharName16)
	BEGIN
		DELETE FROM _ActiveIconsRightSide WHERE CharName16 = @CharName16
		INSERT INTO _AsyncFilterCommands(CommandID, Data1, Status) VALUES(13, @CharName16, 1)
	END




GO

SET ANSI_NULLS ON;
GO
SET QUOTED_IDENTIFIER ON;
GO
CREATE   PROCEDURE [appearance].[RemoveTitle]
	@CharName16 varchar(16)
AS		


IF EXISTS(SELECT 1 FROM _ActiveTitleNameNew WITH(NOLOCK) WHERE  CharName16 = @CharName16)
	BEGIN
		DELETE FROM _ActiveTitleNameNew where CharName16 = @CharName16
		INSERT INTO _AsyncFilterCommands(CommandID, Data1, Status) VALUES(16, @CharName16, 1)
END


GO

SET ANSI_NULLS ON;
GO
SET QUOTED_IDENTIFIER ON;
GO
CREATE   PROCEDURE [appearance].[RemoveTitleColor]
	@CharName16 varchar(16)
AS		


IF EXISTS(SELECT 1 FROM _ActiveTitleColors WITH(NOLOCK) WHERE  CharName16 = @CharName16)
	BEGIN
		DELETE FROM _ActiveTitleColors where CharName16 = @CharName16
		INSERT INTO _AsyncFilterCommands(CommandID, Data1, Status) VALUES(9, @CharName16, 1)
END


GO

SET ANSI_NULLS ON;
GO
SET QUOTED_IDENTIFIER ON;
GO
-- =============================================
-- Author:		<Author,,Name>
-- Create date: <Create Date,,>
-- Description:	<Description,,>
-- =============================================
CREATE   PROCEDURE [appearance].[SelectIcon]
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
	IF EXISTS ( Select 1 from _ActiveIconsLeftSide where CharName16 = @CharName16)
	BEGIN
		UPDATE _ActiveIconsLeftSide set IconID = @IconID where CharName16 = @CharName16
	END
	ELSE
	BEGIN
	INSERT INTO _ActiveIconsLeftSide (CharName16, IconID) VALUES (@CharName16, @IconID)
	END
	END
	ELSE IF(@Side = 1) -- right
	BEGIN
	IF EXISTS ( Select 1 from _ActiveIconsRightSide where CharName16 = @CharName16)
	BEGIN
		UPDATE _ActiveIconsRightSide set IconID = @IconID where CharName16 = @CharName16
	END
	ELSE
	BEGIN
	INSERT INTO _ActiveIconsRightSide (CharName16, IconID) VALUES (@CharName16, @IconID)
	END
	END
END


GO

SET ANSI_NULLS ON;
GO
SET QUOTED_IDENTIFIER ON;
GO
-- =============================================
-- Author:		<Author,,Name>
-- Create date: <Create Date,,>
-- Description:	<Description,,>
-- =============================================
CREATE   PROCEDURE [appearance].[SelectTitle]
	-- Add the parameters for the stored procedure here
	@CharName16 VARCHAR(32),
	@TitleID tinyint
AS
BEGIN
	-- SET NOCOUNT ON added to prevent extra result sets from
	-- interfering with SELECT statements.
	SET NOCOUNT ON;
		IF EXISTS (Select 1 from _ActiveTitleNameNew where CharName16 = @CharName16)
		BEGIN
			UPDATE _ActiveTitleNameNew set RefTitleNameNewID = @TitleID where CharName16 = @CharName16
		END
		ELSE
		BEGIN
		INSERT INTO _ActiveTitleNameNew (CharName16, RefTitleNameNewID) VALUES (@CharName16, @TitleID)
		END
END


GO

SET ANSI_NULLS ON;
GO
SET QUOTED_IDENTIFIER ON;
GO
-- =============================================
-- Author:		<Author,,Name>
-- Create date: <Create Date,,>
-- Description:	<Description,,>
-- =============================================
CREATE   PROCEDURE [appearance].[SelectTitleColor]
	-- Add the parameters for the stored procedure here
	@CharName16 VARCHAR(32),
	@ColorCode varchar(32)
AS
BEGIN
	-- SET NOCOUNT ON added to prevent extra result sets from
	-- interfering with SELECT statements.
	SET NOCOUNT ON;
		IF EXISTS (Select 1 from _ActiveTitleColors where CharName16 = @CharName16)
		BEGIN
			UPDATE _ActiveTitleColors set ColorCode = @ColorCode where CharName16 = @CharName16
		END
		ELSE
		BEGIN
		INSERT INTO _ActiveTitleColors (CharName16, ColorCode) VALUES (@CharName16, @ColorCode)
		END
END


GO

SET ANSI_NULLS ON;
GO
SET QUOTED_IDENTIFIER ON;
GO
CREATE   PROCEDURE [appearance].[SwitchCustomGlow]
	@CharName16 varchar(25),
	@CharID int,

	@ItemCodeName NVARCHAR(255),
	@NewGlow NVARCHAR(255),
	@TargetSlot tinyint,
	@UsedItemSlot tinyint
AS

--DECLARE @ItemCodeName NVARCHAR(255) = 'ITEM_CH_SWORD_08_B_RARE_GLOW_66';
--DECLARE @NewGlow NVARCHAR(255) = 'GLOW_1'; -- Tek bir GLOW değeri

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
	EXEC [commands].[SendNoticeByCharacterName] @CharName16, 3, N'Bu iteme daha önce aynı glow eklenmiştir.'
	return;
	END
	IF EXISTS (Select 1 from SRO_VT_SHARD.._RefObjCommon
	where CodeName128 = @Result)
	BEGIN
	EXEC [live].[ConsumeAndMutateItem] @CharID, @TargetSlot, @Result, @UsedItemSlot, 1
	END

END
ELSE
BEGIN
    SET @Result = @ItemCodeName + '_' + @NewGlow;

	IF (@Result = @ItemCodeName)
	BEGIN
	EXEC [commands].[SendNoticeByCharacterName] @CharName16, 3, N'Bu iteme daha önce aynı glow eklenmiştir.'
	return;
	END
	IF EXISTS (Select 1 from SRO_VT_SHARD.._RefObjCommon
	where CodeName128 = @Result)
	BEGIN
	EXEC [live].[ConsumeAndMutateItem] @CharID, @TargetSlot, @Result, @UsedItemSlot, 1
	END

END
Select @Result AS Final

GO

SET ANSI_NULLS ON;
GO
SET QUOTED_IDENTIFIER ON;
GO
CREATE   PROCEDURE [appearance].[SwitchCustomModel]
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


-- _MODEL ifadesinin başlangıç konumunu bul
SET @ModelStart = CHARINDEX('_MODEL', @TargetItemCodeName);

-- _GLOW ifadesinin başlangıç konumunu bul
SET @GlowStart = CHARINDEX('_GLOW', @TargetItemCodeName);

-- _RARE ifadesinin son konumunu bul
SET @RareEnd = CHARINDEX('_RARE', @TargetItemCodeName) + LEN('_RARE') - 1;

-- Eğer _GLOW ifadesi varsa, onu ayıkla ve geri bırak
IF @GlowStart > 0
BEGIN
    SET @GlowEnd = LEN(@TargetItemCodeName);
    SET @GlowPart = SUBSTRING(@TargetItemCodeName, @GlowStart, @GlowEnd - @GlowStart + 1);
    -- _GLOW kısmını ayıkla
    SET @TargetItemCodeName = LEFT(@TargetItemCodeName, @GlowStart - 1);
END
ELSE
BEGIN
    SET @GlowPart = '';
END

-- Eğer _MODEL ifadesi varsa
IF @ModelStart > 0
BEGIN
    -- MODEL ifadesinin bitiş konumunu bul
    SET @ModelEnd = CHARINDEX('_', @TargetItemCodeName, @ModelStart + LEN('_MODEL'));

    -- Eğer bitiş konumu bulunamazsa, son konumu kullan
    IF @ModelEnd = 0
    BEGIN
        SET @ModelEnd = LEN(@TargetItemCodeName) + 1;
    END

    -- Mevcut MODEL değerini ayıkla
    SET @CurrentModel = SUBSTRING(@TargetItemCodeName, @ModelStart + LEN('_MODEL'), @ModelEnd - @ModelStart - LEN('_MODEL'));

    -- Eğer mevcut MODEL ve yeni MODEL aynıysa, FAIL mesajı gönder
    IF @CurrentModel = REPLACE(@NewModel, 'MODEL', '')
    BEGIN
		EXEC [commands].[SendNoticeByCharacterName] @CharName16, 3, N'Bu iteme daha önce aynı model eklenmiştir.'
        RETURN;
    END

    -- Temel isim kısmını al (RARE kısmından sonrasını da korur)
    SET @TargetItemCodeName = LEFT(@TargetItemCodeName, @ModelStart - 1);

    -- Yeni MODEL değerini ekleme
    SET @TargetItemCodeName = @TargetItemCodeName + '_' + @NewModel;

    -- Eğer daha önce _GLOW varsa, onu tekrar ekle
    IF @GlowPart <> ''
    BEGIN
        SET @TargetItemCodeName = @TargetItemCodeName + @GlowPart;
    END
END
ELSE
BEGIN
    -- MODEL kısmı yoksa, yeni MODEL değeri ekle
    -- RARE kısmından sonrasına ekleme yap
    SET @TargetItemCodeName = LEFT(@TargetItemCodeName, @RareEnd) + '_' + @NewModel;

    -- Eğer daha önce _GLOW varsa, onu tekrar ekle
    IF @GlowPart <> ''
    BEGIN
        SET @TargetItemCodeName = @TargetItemCodeName + '_' + @GlowPart;
    END
END

	IF EXISTS (Select 1 from SRO_VT_SHARD.._RefObjCommon with(nolock) where CodeName128 = @TargetItemCodeName)
	BEGIN
	EXEC [live].[ConsumeAndMutateItem] @CharID, @TargetSlot, @TargetItemCodeName, @UsedItemSlot, 1
	END

GO

SET ANSI_NULLS ON;
GO
SET QUOTED_IDENTIFIER ON;
GO
CREATE   PROCEDURE [appearance].[UpdateLeftIcon]
	@CharName16 as varchar(16),
	@IconID int
AS		

	IF EXISTS(SELECT IconID From _ActiveIconsLeftSide with(nolock) WHERE CharName16 = @CharName16)
	BEGIN
		UPDATE _ActiveIconsLeftSide SET IconID = @IconID WHERE CharName16 = @CharName16
		INSERT INTO _AsyncFilterCommands(CommandID, Data1, Data2, Status) VALUES(10, @CharName16, @IconID, 1)
	END
	ELSE
	BEGIN
		INSERT INTO _ActiveIconsLeftSide VALUES(@CharName16, @IconID)
		INSERT INTO _AsyncFilterCommands(CommandID, Data1, Data2, Status) VALUES(10, @CharName16, @IconID, 1)
	END


GO

SET ANSI_NULLS ON;
GO
SET QUOTED_IDENTIFIER ON;
GO
CREATE   PROCEDURE [appearance].[UpdateNameColor]
            @CharName16 varchar(16),
            @ColorCode varchar(100)
        AS
        BEGIN
            IF NOT EXISTS(SELECT 1 FROM _ActiveNameColors WITH(NOLOCK) WHERE CharName16 = @CharName16)
            BEGIN
                INSERT INTO _ActiveNameColors VALUES(@CharName16, @ColorCode)
                INSERT INTO _AsyncFilterCommands(CommandID, Data1, Data2, Status) VALUES(37, @CharName16, @ColorCode, 1)
            END
            ELSE
            BEGIN
                UPDATE _ActiveNameColors SET ColorCode = @ColorCode WHERE CharName16 = @CharName16
                INSERT INTO _AsyncFilterCommands(CommandID, Data1, Data2, Status) VALUES(37, @CharName16, @ColorCode, 1)
            END
        END
        
GO

SET ANSI_NULLS ON;
GO
SET QUOTED_IDENTIFIER ON;
GO
CREATE   PROCEDURE [appearance].[UpdateRightIcon]
	@CharName16 as varchar(16),
	@IconID int
AS		

	IF EXISTS(SELECT IconID From _ActiveIconsRightSide with(nolock) WHERE CharName16 = @CharName16)
	BEGIN
		UPDATE _ActiveIconsRightSide SET IconID = @IconID WHERE CharName16 = @CharName16
		INSERT INTO _AsyncFilterCommands(CommandID, Data1, Data2, Status) VALUES(11, @CharName16, @IconID, 1)
	END
	ELSE
	BEGIN
		INSERT INTO _ActiveIconsRightSide VALUES(@CharName16, @IconID)
		INSERT INTO _AsyncFilterCommands(CommandID, Data1, Data2, Status) VALUES(11, @CharName16, @IconID, 1)
	END

GO

SET ANSI_NULLS ON;
GO
SET QUOTED_IDENTIFIER ON;
GO
CREATE   PROCEDURE [appearance].[UpdateTitle]
	@CharName16 varchar(16),
	@TitleID tinyint
AS		


IF NOT EXISTS(SELECT 1 FROM _ActiveTitleNameNew WITH(NOLOCK) WHERE  CharName16 = @CharName16)
	BEGIN
		INSERT INTO _ActiveTitleNameNew VALUES(@CharName16, @TitleID)
		INSERT INTO _AsyncFilterCommands(CommandID, Data1, Data2, Status) VALUES(15, @CharName16, @TitleID, 1)
END
ELSE
BEGIN
	Update _ActiveTitleNameNew Set RefTitleNameNewID = @TitleID
	where CharName16 = @CharName16
	INSERT INTO _AsyncFilterCommands(CommandID, Data1, Data2, Status) VALUES(15, @CharName16, @TitleID, 1)
END

GO

SET ANSI_NULLS ON;
GO
SET QUOTED_IDENTIFIER ON;
GO
CREATE   PROCEDURE [appearance].[UpdateTitleColor]
            @CharName16 varchar(16),
            @ColorCode varchar(100)
        AS
        BEGIN
            IF NOT EXISTS(SELECT 1 FROM _ActiveTitleColors WITH(NOLOCK) WHERE CharName16 = @CharName16)
            BEGIN
                INSERT INTO _ActiveTitleColors VALUES(@CharName16, @ColorCode)
                INSERT INTO _AsyncFilterCommands(CommandID, Data1, Data2, Status) VALUES(8, @CharName16, @ColorCode, 1)
            END
            ELSE
            BEGIN
                UPDATE _ActiveTitleColors SET ColorCode = @ColorCode WHERE CharName16 = @CharName16
                INSERT INTO _AsyncFilterCommands(CommandID, Data1, Data2, Status) VALUES(8, @CharName16, @ColorCode, 1)
            END
        END
        
GO

SET ANSI_NULLS ON;
GO
SET QUOTED_IDENTIFIER ON;
GO
-- =============================================
-- Author:		<Author,,Name>
-- Create date: <Create Date,,>
-- Description:	<Description,,>
-- =============================================
CREATE   PROCEDURE [auth].[AuthenticateCharacter]
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

SET ANSI_NULLS ON;
GO
SET QUOTED_IDENTIFIER ON;
GO
CREATE   PROCEDURE [auth].[UpdateHwidList]
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
    -- Ekstra sonuç kümelerinin SELECT ifadelerini etkilemesini önlemek için NOCOUNT ON kullanıldı.
    SET NOCOUNT ON;

    -- Benzersiz web token oluştur
    DECLARE @WebToken varchar(64) = LOWER(CONVERT(varchar(64), NEWID()));

    -- WebToken çakışmasını önlemek için kontrol ekleyelim
    WHILE EXISTS (SELECT 1 FROM _HwidList with (nolock) WHERE webtoken = @WebToken)
    BEGIN
        SET @WebToken = LOWER(CONVERT(varchar(64), NEWID()));
    END

    IF EXISTS (SELECT 1 FROM _HwidList WHERE CharID = @CharID)
    BEGIN
        UPDATE _HwidList 
        SET Active = @Active, 
            CharName16 = @CharName16, 
            [IP] = @IP, 
            Hwid = @Hwid, 
            JobStatus = @JobStatus, 
            LatestRegionId = @LatestRegionId, 
            LatestWorldId = @LatestWorldId, 
            CurLevel = @CurLevel,
            WebToken = @WebToken -- Güncelleme sırasında yeni benzersiz WebToken oluştur
        WHERE CharID = @CharID;
    END
    ELSE
    BEGIN
        INSERT INTO _HwidList (Active, CharID, CharName16, [IP], Hwid, JobStatus, LatestRegionId, LatestWorldId, CurLevel, WebToken) 
        VALUES (@Active, @CharID, @CharName16, @IP, @Hwid, @JobStatus, @LatestRegionId, @LatestWorldId, @CurLevel, @WebToken);
    END
END
GO

SET ANSI_NULLS ON;
GO
SET QUOTED_IDENTIFIER ON;
GO
CREATE   PROCEDURE [battlepass].[CreateMission]
    @SeasonID INT,
    @MissionType VARCHAR(12),
    @EventType VARCHAR(40),
    @TargetValue INT = 0,
    @RequiredCount BIGINT,
    @RewardXP INT,
    @Title NVARCHAR(100),
    @Description NVARCHAR(300) = N'',
    @StartDateUtc DATETIME2(0) = NULL,
    @EndDateUtc DATETIME2(0) = NULL,
    @SortOrder SMALLINT = 0
AS
BEGIN
    SET NOCOUNT ON;
    SET @MissionType = UPPER(@MissionType);
    SET @EventType = UPPER(@EventType);

    IF NOT EXISTS (SELECT 1 FROM dbo._BattlePassSeasons WHERE ID=@SeasonID)
        THROW 51000, 'Unknown Battle Pass season.', 1;
    IF NOT EXISTS (SELECT 1 FROM dbo._BattlePassMissionTypes WHERE Code=@EventType AND IsImplemented=1)
        THROW 51001, 'Unknown or unsupported Battle Pass mission type.', 1;
    IF @MissionType NOT IN ('DAILY','WEEKLY','SEASON')
        THROW 51002, 'MissionType must be DAILY, WEEKLY, or SEASON.', 1;

    INSERT dbo._BattlePassMissions
        (SeasonID,MissionType,EventType,TargetValue,RequiredCount,RewardXP,Title,Description,StartDateUtc,EndDateUtc,SortOrder)
    VALUES
        (@SeasonID,@MissionType,@EventType,@TargetValue,@RequiredCount,@RewardXP,@Title,@Description,@StartDateUtc,@EndDateUtc,@SortOrder);

    SELECT CAST(SCOPE_IDENTITY() AS INT) AS MissionID;
END;

GO

SET ANSI_NULLS ON;
GO
SET QUOTED_IDENTIFIER ON;
GO

CREATE   PROCEDURE [battlepass].[CreateMissionByCodeName]
    @SeasonID INT,
    @MissionType VARCHAR(12),
    @EventType VARCHAR(40),
    @TargetCodeName VARCHAR(128),
    @RequiredCount BIGINT,
    @RewardXP INT,
    @Title NVARCHAR(100),
    @Description NVARCHAR(300) = N'',
    @SortOrder SMALLINT = 0,
    @ShardDB SYSNAME = N'SRO_VT_SHARD'
AS
BEGIN
    SET NOCOUNT ON;
    DECLARE @TargetValue INT;
    DECLARE @Sql NVARCHAR(MAX) =
        N'SELECT TOP(1) @FoundID=ID FROM ' + QUOTENAME(@ShardDB) +
        N'.dbo._RefObjCommon WITH (NOLOCK) WHERE CodeName128=@CodeName AND Service=1;';
    EXEC sys.sp_executesql @Sql,
        N'@CodeName VARCHAR(128), @FoundID INT OUTPUT',
        @CodeName=@TargetCodeName, @FoundID=@TargetValue OUTPUT;
    IF @TargetValue IS NULL
        THROW 51003, 'Target CodeName was not found in SRO_VT_SHARD.', 1;

    EXEC [battlepass].[CreateMission]
        @SeasonID=@SeasonID, @MissionType=@MissionType, @EventType=@EventType,
        @TargetValue=@TargetValue, @RequiredCount=@RequiredCount, @RewardXP=@RewardXP,
        @Title=@Title, @Description=@Description, @SortOrder=@SortOrder;
END;

GO

SET ANSI_NULLS ON;
GO
SET QUOTED_IDENTIFIER ON;
GO

CREATE   PROCEDURE [battlepass].[GrantExperience]
    @SeasonID INT,
    @JID INT,
    @CharID INT,
    @Amount BIGINT
AS
BEGIN
    SET NOCOUNT ON;
    SET XACT_ABORT ON;

    IF @Amount <= 0
        THROW 51010, 'XP amount must be greater than zero.', 1;

    DECLARE @Scope VARCHAR(16), @OwnerKey BIGINT, @NewXP BIGINT, @NewLevel SMALLINT;
    SELECT @Scope=ProgressScope FROM dbo._BattlePassSeasons WHERE ID=@SeasonID;
    IF @Scope IS NULL
        THROW 51011, 'Unknown Battle Pass season.', 1;

    SET @OwnerKey = CASE WHEN @Scope='CHARACTER' THEN -1 * CONVERT(BIGINT,@CharID) ELSE @JID END;

    BEGIN TRANSACTION;

    IF NOT EXISTS
    (
        SELECT 1 FROM dbo._BattlePassPlayers WITH (UPDLOCK,HOLDLOCK)
        WHERE SeasonID=@SeasonID AND OwnerKey=@OwnerKey
    )
        INSERT dbo._BattlePassPlayers(SeasonID,OwnerKey,JID,LastCharID)
        VALUES(@SeasonID,@OwnerKey,@JID,@CharID);

    SELECT @NewXP=CurrentXP+@Amount
    FROM dbo._BattlePassPlayers WITH (UPDLOCK,HOLDLOCK)
    WHERE SeasonID=@SeasonID AND OwnerKey=@OwnerKey;

    ;WITH Thresholds AS
    (
        SELECT LevelNo,
               SUM(CONVERT(BIGINT,RequiredXP)) OVER
                   (ORDER BY LevelNo ROWS UNBOUNDED PRECEDING) AS RequiredTotal
        FROM dbo._BattlePassLevels
        WHERE SeasonID=@SeasonID
    )
    SELECT @NewLevel=ISNULL(MAX(LevelNo),0)
    FROM Thresholds
    WHERE RequiredTotal<=@NewXP;

    UPDATE dbo._BattlePassPlayers
    SET CurrentXP=@NewXP,CurrentLevel=@NewLevel,LastCharID=@CharID,UpdatedAtUtc=SYSUTCDATETIME()
    WHERE SeasonID=@SeasonID AND OwnerKey=@OwnerKey;

    COMMIT TRANSACTION;
    SELECT @OwnerKey AS OwnerKey,@NewXP AS CurrentXP,@NewLevel AS CurrentLevel;
END;

GO

SET ANSI_NULLS ON;
GO
SET QUOTED_IDENTIFIER ON;
GO
CREATE   PROCEDURE [commands].[BroadcastNotice]
	@NoticeType int,
	@Notice varchar(max)
AS		

INSERT INTO _AsyncFilterCommands(CommandID, Data1, Data2, Data3,Status) VALUES(3, 'sendall', @NoticeType, @Notice, 1)


GO

SET ANSI_NULLS ON;
GO
SET QUOTED_IDENTIFIER ON;
GO
CREATE   PROCEDURE [commands].[ChangeGrantName]
    @CharID INT,
    @GrantName VARCHAR(16)
AS
BEGIN
    INSERT INTO _AsyncGameServerCommands(Action_ID, Data1, Data2) VALUES(1, @CharID, @GrantName)
END

GO

SET ANSI_NULLS ON;
GO
SET QUOTED_IDENTIFIER ON;
GO
CREATE   PROCEDURE [commands].[DisconnectAll]

AS		

INSERT INTO _AsyncFilterCommands(CommandID, Status) VALUES(2, 1)


GO

SET ANSI_NULLS ON;
GO
SET QUOTED_IDENTIFIER ON;
GO
CREATE   PROCEDURE [commands].[DisconnectByCharacterId]
	@CharID int
AS		

INSERT INTO _AsyncFilterCommands(CommandID, Data1, Status) VALUES(34, @CharID, 1)


GO

SET ANSI_NULLS ON;
GO
SET QUOTED_IDENTIFIER ON;
GO
CREATE   PROCEDURE [commands].[DisconnectByCharacterName]
	@CharName16 varchar(16)
AS		

INSERT INTO _AsyncFilterCommands(CommandID, Data1, Status) VALUES(1, @CharName16, 1)


GO

SET ANSI_NULLS ON;
GO
SET QUOTED_IDENTIFIER ON;
GO
CREATE   PROCEDURE [commands].[GetUp]
		@CharID int
AS		

INSERT INTO _AsyncGameServerCommands(Action_ID, Data1) VALUES(15, @CharID)

GO

SET ANSI_NULLS ON;
GO
SET QUOTED_IDENTIFIER ON;
GO

CREATE   PROCEDURE [commands].[GetUpAtPosition]
	@CHARID int,
	@GameWorldID int,
	@RegionId int,
	@PosX int,
	@PosY int,
	@PosZ int
AS		

INSERT INTO _AsyncGameServerCommands(Action_ID, Data1, Data2, Data3, Data4, Data5, Data6) VALUES(23, @CHARID, @GameWorldID, @RegionId, @PosX, @PosY, @PosZ)



GO

SET ANSI_NULLS ON;
GO
SET QUOTED_IDENTIFIER ON;
GO
CREATE   PROCEDURE [commands].[SendMapPingByCharacterId]
	@CharID int,
	@RegionID int,
	@PosX int,
	@PosZ int,
	@PosY int,
	@Seconds int
AS		

INSERT INTO _AsyncFilterCommands(CommandID, Data1, Data2, Data3,Data4,Data5, Data6, Status) VALUES(36, @CharID, @RegionID, @PosX, @PosY, @PosZ, @Seconds, 1)


GO

SET ANSI_NULLS ON;
GO
SET QUOTED_IDENTIFIER ON;
GO
CREATE   PROCEDURE [commands].[SendNoticeByCharacterId]
	@CharID int,
	@NoticeType int,
	@Notice varchar(max)
AS		

INSERT INTO _AsyncFilterCommands(CommandID, Data1, Data2, Data3, Status) VALUES(28, @CharID, @Notice, @NoticeType, 1)


GO

SET ANSI_NULLS ON;
GO
SET QUOTED_IDENTIFIER ON;
GO
CREATE   PROCEDURE [commands].[SendNoticeByCharacterName]
	@CharName16 varchar(16),
	@NoticeType int,
	@Notice varchar(max)
AS		

INSERT INTO _AsyncFilterCommands(CommandID, Data1, Data2, Data3, Data4, Status) VALUES(3, 'sendchar', @NoticeType, @Notice, @CharName16, 1)


GO

SET ANSI_NULLS ON;
GO
SET QUOTED_IDENTIFIER ON;
GO
CREATE   PROCEDURE [core].[Startup]
 
AS
    SET NOCOUNT ON;

    
GO

SET ANSI_NULLS ON;
GO
SET QUOTED_IDENTIFIER ON;
GO

CREATE   PROCEDURE [events].[CreateJobKillCounterByWorldId]
@WorldID int
AS

INSERT INTO _AsyncFilterCommands(CommandID, Data1, Status) VALUES(32, @WorldID,  1)


	/* Bu prosedür ile kill counter oluşturduğunuz bölgeler için _IncreaseKillCounter komutunu kullanabilirsiniz. Bu prosedürü aynı bölge için ikinci kez çağırırsanız kill counter sıfırlanır. */
GO

SET ANSI_NULLS ON;
GO
SET QUOTED_IDENTIFIER ON;
GO

CREATE   PROCEDURE [events].[CreateKillCounterByWorldId]
@Title varchar(50),
@WorldID int
AS

INSERT INTO _AsyncFilterCommands(CommandID, Data1, Data2, Status) VALUES(25, @Title, @WorldID,  1)


	/* Bu prosedür ile kill counter oluşturduğunuz bölgeler için _IncreaseKillCounter komutunu kullanabilirsiniz. Bu prosedürü aynı bölge için ikinci kez çağırırsanız kill counter sıfırlanır. */
GO

SET ANSI_NULLS ON;
GO
SET QUOTED_IDENTIFIER ON;
GO

CREATE   PROCEDURE [events].[CreateTeamKillCounterByWorldId]
@WorldID int
AS

INSERT INTO _AsyncFilterCommands(CommandID, Data1, Status) VALUES(30, @WorldID,  1)


	/* Bu prosedür ile kill counter oluşturduğunuz bölgeler için _IncreaseKillCounter komutunu kullanabilirsiniz. Bu prosedürü aynı bölge için ikinci kez çağırırsanız kill counter sıfırlanır. */
GO

SET ANSI_NULLS ON;
GO
SET QUOTED_IDENTIFIER ON;
GO

CREATE   PROCEDURE [events].[CreateTimerByRegionId]
@Seconds int,
@RegionID int
AS

	INSERT INTO _AsyncFilterCommands(CommandID, Data1, Data2, Status) VALUES(24, @Seconds, @RegionID,  1)


	/* Bu prosedür ile timer oluşturduğunuz takdirde süre bitene kadar aynı region'a teleport olduğunda timer kaldığı yerden devam edecektir. */
GO

SET ANSI_NULLS ON;
GO
SET QUOTED_IDENTIFIER ON;
GO

CREATE   PROCEDURE [events].[CreateTimerByWorldId]
@Seconds int,
@WorldID int
AS

	INSERT INTO _AsyncFilterCommands(CommandID, Data1, Data2, Status) VALUES(23, @Seconds, @WorldID,  1)


	/* Bu prosedür ile timer oluşturduğunuz takdirde süre bitene kadar aynı world id'e teleport olduğunda timer kaldığı yerden devam edecektir. */
GO

SET ANSI_NULLS ON;
GO
SET QUOTED_IDENTIFIER ON;
GO

CREATE   PROCEDURE [events].[EnqueueReload]
    @RequestedBy nvarchar(64) = N'Scheduler'
AS
BEGIN
    SET NOCOUNT ON;

    INSERT INTO dbo._AutoEventCommandQueue (CommandType, EventCode, RequestedBy)
    VALUES (N'RELOAD', N'', @RequestedBy);
END

GO

SET ANSI_NULLS ON;
GO
SET QUOTED_IDENTIFIER ON;
GO

CREATE   PROCEDURE [events].[EnqueueStart]
    @EventCode nvarchar(32),
    @RequestedBy nvarchar(64) = N'Scheduler'
AS
BEGIN
    SET NOCOUNT ON;

    INSERT INTO dbo._AutoEventCommandQueue (CommandType, EventCode, RequestedBy)
    VALUES (N'START', @EventCode, @RequestedBy);
END

GO

SET ANSI_NULLS ON;
GO
SET QUOTED_IDENTIFIER ON;
GO

CREATE   PROCEDURE [events].[EnqueueStop]
    @RequestedBy nvarchar(64) = N'Scheduler'
AS
BEGIN
    SET NOCOUNT ON;

    INSERT INTO dbo._AutoEventCommandQueue (CommandType, EventCode, RequestedBy)
    VALUES (N'STOP', N'', @RequestedBy);
END

GO

SET ANSI_NULLS ON;
GO
SET QUOTED_IDENTIFIER ON;
GO

CREATE   PROCEDURE [events].[HandleMobKilled] 
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

    -- احسب فقط لو الموب المطلوب
    IF (@RefObjID = 175001116)
    BEGIN
        -- (اختياري) تأكد إن حدث BeastFury (ID=9) شغّال دلوقتي
        IF EXISTS (
            SELECT 1
            FROM Events.dbo.Event_Control WITH (NOLOCK)
            WHERE EventID = 9 AND IsOpen = 1
        )
        BEGIN
            BEGIN TRY
                BEGIN TRAN;

                -- حاول تزود لو اللاعب موجود
                UPDATE Events.dbo.Player_BeastFury WITH (ROWLOCK)
                SET KillCount      = KillCount + 1,
                    LastUpdatedUtc = SYSUTCDATETIME()
                WHERE CharID = @CharID;

                -- لو مفيش صف، اعمله Insert كبداية بـ 1
                IF (@@ROWCOUNT = 0)
                BEGIN
                    INSERT INTO Events.dbo.Player_BeastFury (CharID, KillCount, LastUpdatedUtc)
                    VALUES (@CharID, 1, SYSUTCDATETIME());
                END

                COMMIT;
            END TRY
            BEGIN CATCH
                IF (XACT_STATE() <> 0) ROLLBACK;
                -- تقدر تضيف لوج هنا لو حابب
                -- RAISERROR('HandleMobKilled failed: %s', 16, 1, ERROR_MESSAGE());
            END CATCH
        END
    END
END

GO

SET ANSI_NULLS ON;
GO
SET QUOTED_IDENTIFIER ON;
GO

CREATE   PROCEDURE [events].[IncreaseJobKillCounterByWorldId]
@WorldID int,
@CharName16 varchar(12),
@Kill int,
@Job int
AS

INSERT INTO _AsyncFilterCommands(CommandID, Data1, Data2, Data3, Data4, Status) VALUES(33, @WorldID, @CharName16, @Kill, @Job, 1)


	--- 2 = thief 1-3 hunter trader
GO

SET ANSI_NULLS ON;
GO
SET QUOTED_IDENTIFIER ON;
GO

CREATE   PROCEDURE [events].[IncreaseKillCounterByWorldId]
@WorldID int,
@CharName16 varchar(12),
@Kill int
AS

INSERT INTO _AsyncFilterCommands(CommandID, Data1, Data2, Data3, Status) VALUES(26, @WorldID, @CharName16, @Kill, 1)


	/* Bu prosedür ile _MakeNewKillCounterbyRegionID prosedürü ile oluşturduğunuz countera ekleme yapabilirsiniz. */
GO

SET ANSI_NULLS ON;
GO
SET QUOTED_IDENTIFIER ON;
GO

CREATE   PROCEDURE [events].[IncreaseTeamKillCounterByWorldId]
@WorldID int,
@CharName16 varchar(12),
@Kill int,
@Team int
AS

INSERT INTO _AsyncFilterCommands(CommandID, Data1, Data2, Data3, Data4, Status) VALUES(31, @WorldID, @CharName16, @Kill, @Team, 1)


	/* Bu prosedür ile _MakeNewTeamKillCounterbyWorldID prosedürü ile oluşturduğunuz countera ekleme yapabilirsiniz. */
GO

SET ANSI_NULLS ON;
GO
SET QUOTED_IDENTIFIER ON;
GO
CREATE   PROCEDURE [events].[ProcessAttendance]
    @CharID int,
    @Date date,
    @ReturnValue INT OUTPUT
AS
BEGIN
    SET NOCOUNT ON;
    -- _Attendance tablosunda kayıt yoksa ekle
    IF NOT EXISTS (SELECT 1 FROM _Attendance WHERE CharID = @CharID)
    BEGIN
        INSERT INTO _Attendance (CharID, DayCount, LastAttendedDate) 
        VALUES (@CharID, 1, @Date);
        
        -- İlk defa eklenen kayıtta CanTake değeri 1 olacak şekilde ekle
        INSERT INTO _AttendanceRewardLog (RefRewardID, CharID, DayCount, CanTake, AlreadyTaken)
        SELECT r.ID, @CharID, r.DayCount, 
               CASE WHEN r.DayCount = 1 THEN 1 ELSE 0 END AS CanTake,
               0 AS AlreadyTaken
        FROM _RefAttendanceReward r
        WHERE NOT EXISTS (
            SELECT 1 
            FROM _AttendanceRewardLog l 
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
        FROM _Attendance
        WHERE CharID = @CharID;

        IF @LastDate < @Date
        BEGIN
            UPDATE _Attendance 
            SET DayCount = @CurrentDayCount + 1, LastAttendedDate = @Date 
            WHERE CharID = @CharID;
            
            -- Mevcut kayıtları güncelle
            UPDATE _AttendanceRewardLog
            SET CanTake = 1
            WHERE CharID = @CharID
              AND EXISTS (
                  SELECT 1
                  FROM _Attendance a
                  WHERE a.CharID = @CharID AND a.DayCount >= _AttendanceRewardLog.DayCount AND _AttendanceRewardLog.AlreadyTaken != 1
              )
              AND EXISTS (
                  SELECT 1
                  FROM _RefAttendanceReward r
                  WHERE r.DayCount = _AttendanceRewardLog.DayCount AND r.ID = _AttendanceRewardLog.RefRewardID
              );
            
            -- Eksik kayıtları ekle
            INSERT INTO _AttendanceRewardLog (RefRewardID, CharID, DayCount, CanTake, AlreadyTaken)
            SELECT r.ID, @CharID, r.DayCount,
                   CASE WHEN EXISTS (
                             SELECT 1
                             FROM _Attendance a
                             WHERE a.CharID = @CharID AND a.DayCount >= r.DayCount
                         )
                         THEN 1
                         ELSE 0
                   END AS CanTake,
                   0 AS AlreadyTaken
            FROM _RefAttendanceReward r
            WHERE NOT EXISTS (
                SELECT 1 
                FROM _AttendanceRewardLog l 
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

SET ANSI_NULLS ON;
GO
SET QUOTED_IDENTIFIER ON;
GO

/* يسجّل اللاعب في Lottery Gold (EventID=6) ويخصم التذكرة.
   يُستدعى فقط من _OnEventRegister_EDIT.
*/
CREATE   PROCEDURE [events].[RegisterGoldLottery]
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
            INSERT INTO KMTGuard.._AsyncFilterCommands (CommandID, Data1, Data2, Data3, Status)
            VALUES (28, CONVERT(NVARCHAR(50), @CharID), @ClosedMsg, N'1', 1);
            GOTO _EXIT;
        END

        /* 1) لو مسجّل بالفعل في جدول Events → رسالة دبل */
        IF EXISTS (SELECT 1 FROM [Events].[dbo].[_LotteryGold_RegPlayers] WITH (NOLOCK) WHERE CharID=@CharID)
        BEGIN
            INSERT INTO KMTGuard.._AsyncFilterCommands (CommandID, Data1, Data2, Data3, Status)
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
            INSERT INTO KMTGuard.._AsyncFilterCommands (CommandID, Data1, Data2, Data3, Status)
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
                INSERT INTO KMTGuard.._AsyncFilterCommands (CommandID, Data1, Data2, Data3, Status)
                VALUES (28, CONVERT(NVARCHAR(50), @CharID), @DupMsg, N'1', 1);
                GOTO _EXIT;
            END
            ELSE
            BEGIN
                DECLARE @Err1 NVARCHAR(MAX) = CONCAT(N'LotteryGold_Register_JT INSERT ERROR: ', ERROR_MESSAGE(),
                                      N' [', ERROR_NUMBER(), '/', ERROR_SEVERITY(), '/', ERROR_STATE(),
                                      N' @ line ', ERROR_LINE(), N']');
                PRINT @Err1;

                INSERT INTO KMTGuard.._AsyncFilterCommands (CommandID, Data1, Data2, Data3, Status)
                VALUES (28, CONVERT(NVARCHAR(50), @CharID), @FailMsg, N'1', 1);
                GOTO _EXIT;
            END
        END CATCH

        /* 4) خصم الجولد عبر __LiveGold */
        BEGIN TRY
            EXEC [live].[AdjustGold] @CharID=@CharID, @Gold=@cost, @AddOrRemove=0;  -- 0=Remove
        END TRY
        BEGIN CATCH
            /* فشل الخصم → نشيل التسجيل علشان الاتساق */
            DELETE FROM [Events].[dbo].[_LotteryGold_RegPlayers] WHERE CharID=@CharID;

            DECLARE @Err2 NVARCHAR(MAX) = CONCAT(N'LotteryGold_Register_JT DEDUCT ERROR: ', ERROR_MESSAGE(),
                                  N' [', ERROR_NUMBER(), '/', ERROR_SEVERITY(), '/', ERROR_STATE(),
                                  N' @ line ', ERROR_LINE(), N']');
            PRINT @Err2;

            INSERT INTO KMTGuard.._AsyncFilterCommands (CommandID, Data1, Data2, Data3, Status)
            VALUES (28, CONVERT(NVARCHAR(50), @CharID), @FailMsg, N'1', 1);
            GOTO _EXIT;
        END CATCH

        /* 5) رسالة نجاح */
        INSERT INTO KMTGuard.._AsyncFilterCommands (CommandID, Data1, Data2, Data3, Status)
        VALUES (28, CONVERT(NVARCHAR(50), @CharID), @OkMsg, N'1', 1);

    END TRY
    BEGIN CATCH
        DECLARE @Err NVARCHAR(MAX) = CONCAT(N'LotteryGold_Register_JT ERROR: ', ERROR_MESSAGE(),
                            N' [', ERROR_NUMBER(), '/', ERROR_SEVERITY(), '/', ERROR_STATE(),
                            N' @ line ', ERROR_LINE(), N']');
        PRINT @Err;

        INSERT INTO KMTGuard.._AsyncFilterCommands (CommandID, Data1, Data2, Data3, Status)
        VALUES (28, CONVERT(NVARCHAR(50), @CharID), @FailMsg, N'1', 1);
    END CATCH

_EXIT:
    EXEC sys.sp_releaseapplock @Resource=@LockResource, @LockOwner=N'Session';
END

GO

SET ANSI_NULLS ON;
GO
SET QUOTED_IDENTIFIER ON;
GO
CREATE   PROCEDURE [hooks].[OnAlchemySucceeded]
	@CharID int,
	@CharName varchar(25),
	@ItemID int,
	@Plus tinyint,
	@AdvLevel tinyint,
	@Slot tinyint
AS
-- Alchemy Plus Notice Link
	--INSERT INTO _AsyncGameServerCommands(Action_ID, Data1, Data2, Data3, Data4, Data5) VALUES(131, @CharID, @ItemID, @Plus + @AdvLevel, @Slot, @AdvLevel)

	/*
		Bu prosedürü düzenleyerek bir karakter Alchemy ile başarılı bir şekilde artı bastığında işlem yapabilirsiniz.

		@CharID = Eşya geliştiren karakterin IDsi.
		@CharName = Eşya geliştiren karakterin ismi.
		@ItemID = Geliştirilen eşyanın IDsi.
		@Plus = Geliştirilen eşyanın geldiği + seviyesi. Adv. Elixir dahil edilmeden bildirilir.
		@AdvLevel = Var ise, eşyadaki Advanced Elixir seviyesi.
		@Slot = Geliştirilen eşyanın bulunduğu slot.
		Bu veritabanındaki diğer prosedürlerin aksine bu prosedür her restartta orjinal haline döndürülmeyecektir. Dikkatli düzenleyiniz!
	*/

GO

SET ANSI_NULLS ON;
GO
SET QUOTED_IDENTIFIER ON;
GO

CREATE   PROCEDURE [hooks].[OnAutoEquipRequested]
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
        EXEC [commands].[SendNoticeByCharacterId]
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
        EXEC [commands].[SendNoticeByCharacterId]
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

SET ANSI_NULLS ON;
GO
SET QUOTED_IDENTIFIER ON;
GO
CREATE   PROCEDURE [hooks].[OnCharacterGetUp]
	@CharID int,
	@CharName varchar(25),
	@LatestRegion int,
	@LatestWorld int,
	@PVPState tinyint
AS
	
	return 1;


	/*
		Bu prosedürü düzenleyerek bir karakterin ölü iken ayağa kalkmasını kontrol edebilirsiniz.
		Prosedürden 0 döndürürseniz ayağa kalkması engellenecektir.
		Prosedürden 1 döndürürseniz ayağa kalkmasına izin verilecektir.
		Döndürdüğünüz sonuca göre bir mesaj göndermeniz gerekmektedir. Filter herhangi bir mesaj görüntülemez.

		@CharID = Kalkmaya çalışan karakterin Char IDsi.
		@Charname = Kalkmaya çalışan karakterin Char Adı.
		@LatestRegion = Kalkmaya çalışan karakterin Region IDsi. (Teleportla son spawn olduğu region gönderilir.)
		@LatestWorld = Kalkmaya çalışan karakterin World IDsi.
		@PVPState = Kalkmaya çalışan karakterin PVP State rengi.
		
		Bridge Command 84'ü kullanarak karaterin istediğiniz lokasyonda doğmasını sağlayabilirsiniz. Bu durumda return 0 yapmanız gerekecektir.

		Bu veritabanındaki diğer prosedürlerin aksine bu prosedür her restartta orjinal haline döndürülmeyecektir. Dikkatli düzenleyiniz!
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

SET ANSI_NULLS ON;
GO
SET QUOTED_IDENTIFIER ON;
GO

CREATE   PROCEDURE [hooks].[OnCharacterKilled]
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
        EXEC [teleport].[ToTownByCharacterId] @CharID = @DeadCharID;
    END TRY
    BEGIN CATCH
        -- تجاهل أخطاء التلي بورت
    END CATCH
END

GO

SET ANSI_NULLS ON;
GO
SET QUOTED_IDENTIFIER ON;
GO

/* إلغاء تسجيل لاعب في جولة الإيفنت الحالية + ردّ الرسوم إن لزم (Gold/Silk عبر إجراءات Live) */
CREATE   PROCEDURE [hooks].[OnEventCancelled]
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
            INSERT INTO [KMTGuard].._AsyncFilterCommands (CommandID, Data1, Data2, Data3, Status)
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
            INSERT INTO [KMTGuard].._AsyncFilterCommands (CommandID, Data1, Data2, Data3, Status)
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
                        EXEC [live].[AdjustGold]
                             @CharID=@CharID, @Gold=@amount, @AddOrRemove=1; -- 1 = Add (Refund)
                    END TRY
                    BEGIN CATCH
                        ROLLBACK TRAN;
                        INSERT INTO [KMTGuard].._AsyncFilterCommands (CommandID, Data1, Data2, Data3, Status)
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
                        INSERT INTO [KMTGuard].._AsyncFilterCommands (CommandID, Data1, Data2, Data3, Status)
                        VALUES (28, CONVERT(NVARCHAR(50), @CharID),
                                N'Cancel failed: refund (silk) amount out of range.', N'1', 1);
                        GOTO _EXIT;
                    END

                    DECLARE @nSilkRefund INT = CAST(@amount AS INT);
                    DECLARE @rc INT;

                    EXEC @rc = [live].[AdjustSilk]
                         @CharID     = @CharID,
                         @nSilk      = @nSilkRefund,  -- استرجاع
                         @nSilkGift  = 0,
                         @nSilkPoint = 0;

                    IF (@rc <> 0)
                    BEGIN
                        ROLLBACK TRAN;
                        INSERT INTO [KMTGuard].._AsyncFilterCommands (CommandID, Data1, Data2, Data3, Status)
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

        INSERT INTO [KMTGuard].._AsyncFilterCommands (CommandID, Data1, Data2, Data3, Status)
        VALUES (28, CONVERT(NVARCHAR(50), @CharID), @okMsg, N'1', 1);

    END TRY
    BEGIN CATCH
        IF (XACT_STATE() <> 0) ROLLBACK TRAN;

        DECLARE @Err NVARCHAR(MAX) = CONCAT(
            N'_OnEventCancel_EDIT ERROR: ', ERROR_MESSAGE(),
            N' [', ERROR_NUMBER(), '/', ERROR_SEVERITY(), '/', ERROR_STATE(), N' @ ', ERROR_LINE(), N']'
        );

        INSERT INTO [KMTGuard].._AsyncFilterCommands (CommandID, Data1, Data2, Data3, Status)
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

SET ANSI_NULLS ON;
GO
SET QUOTED_IDENTIFIER ON;
GO

CREATE   PROCEDURE [hooks].[OnEventRegistered]
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
                INSERT INTO KMTGuard.._AsyncFilterCommands(CommandID, Data1, Data2, Data3, Status)
                VALUES (28, CONVERT(NVARCHAR(50), @CharID), @MsgBF, N'1', 1);
            END CATCH;
            RETURN;
        END
        ELSE
        BEGIN
            INSERT INTO KMTGuard.._AsyncFilterCommands(CommandID, Data1, Data2, Data3, Status)
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
                INSERT INTO KMTGuard.._AsyncFilterCommands(CommandID, Data1, Data2, Data3, Status)
                VALUES (28, CONVERT(NVARCHAR(50), @CharID), @MsgLMS, N'1', 1);
            END CATCH;
            RETURN;
        END
        ELSE
        BEGIN
            INSERT INTO KMTGuard.._AsyncFilterCommands(CommandID, Data1, Data2, Data3, Status)
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
                INSERT INTO KMTGuard.._AsyncFilterCommands(CommandID, Data1, Data2, Data3, Status)
                VALUES (28, CONVERT(NVARCHAR(50), @CharID), @MsgSURV, N'1', 1);
            END CATCH;
            RETURN;
        END
        ELSE
        BEGIN
            INSERT INTO KMTGuard.._AsyncFilterCommands(CommandID, Data1, Data2, Data3, Status)
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
            FROM KMTGuard.dbo._PartyData WITH (NOLOCK)
            WHERE CharID = @CharID AND PartyID IS NOT NULL AND PartyID <> 0
        )
        BEGIN
            INSERT INTO KMTGuard.._AsyncFilterCommands(CommandID, Data1, Data2, Data3, Status)
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
                INSERT INTO KMTGuard.._AsyncFilterCommands(CommandID, Data1, Data2, Data3, Status)
                VALUES (28, CONVERT(NVARCHAR(50), @CharID), @MsgSVP, N'1', 1);
            END CATCH;
            RETURN;
        END
        ELSE
        BEGIN
            INSERT INTO KMTGuard.._AsyncFilterCommands(CommandID, Data1, Data2, Data3, Status)
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
                INSERT INTO KMTGuard.._AsyncFilterCommands(CommandID, Data1, Data2, Data3, Status)
                VALUES (28, CONVERT(NVARCHAR(50), @CharID), @MsgTD, N'1', 1);
            END CATCH;
            RETURN;
        END
        ELSE
        BEGIN
            INSERT INTO KMTGuard.._AsyncFilterCommands(CommandID, Data1, Data2, Data3, Status)
            VALUES (28, CONVERT(NVARCHAR(50), @CharID),
                    N'Tower Defender: Registration is temporarily unavailable.', N'1', 1);
            RETURN;
        END
    END
END

GO

SET ANSI_NULLS ON;
GO
SET QUOTED_IDENTIFIER ON;
GO
CREATE   PROCEDURE [hooks].[OnGameServerStarted]
	@GameServerExeName varchar(128)
AS
INSERT INTO _AsyncFilterCommands(CommandID, Status) VALUES(1000, 1)

/*
	Prosedür, çoklu gameserver kullanıyorsanız her gameserver tarafından çağırılacaktır.
	Sadece bir gameserverdan gelen veriyle işlem yapabilmek için @GameServerExeName parametresini kullanarak filtreleme yapabilirsiniz.

	Örnek:

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

SET ANSI_NULLS ON;
GO
SET QUOTED_IDENTIFIER ON;
GO

CREATE   PROCEDURE [hooks].[OnItemMallPurchased]
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

    EXEC [achievements].[UpdateProgress] @CharID, 9, 9, @Silk;

    -- Example Silk Rank
    IF NOT EXISTS (SELECT 1 FROM _SilkRank with (nolock) WHERE CharID = @CharID)
    BEGIN
        -- Silk Rank'da icon göstermek istiyorsanız _RefMediaIconPath'daki karşılığını Silk Rank'a ekleyebilirsiniz.
        INSERT INTO _SilkRank (JID, CharID, SilkHistory, SilkRank) VALUES (@JID, @CharID, @Silk, 6);
    END
    ELSE
    BEGIN
        UPDATE _SilkRank 
        SET SilkHistory = SilkHistory + @Silk 
        WHERE CharID = @CharID; -- update 
    END

    DECLARE @SilkHistory int;
    SELECT @SilkHistory = SilkHistory FROM _SilkRank WHERE CharID = @CharID;

    -- Rank güncellemeleri
    IF (@SilkHistory < 100)
    BEGIN
	    UPDATE _SilkRank 
        SET SilkRank  = 6
        WHERE CharID = @CharID; -- update 

        INSERT INTO _AsyncFilterCommands(CommandID, Data1, Data2, Data3, Data4, Status) 
        VALUES(27, @CharID, @JID, @Silk, 6, 1);
        EXEC [appearance].[UpdateRightIcon] @CharName16, 6; -- iron
    END

    ELSE IF (@SilkHistory >= 100 AND @SilkHistory < 300)
    BEGIN
	    UPDATE _SilkRank 
        SET SilkRank  = 5
        WHERE CharID = @CharID; -- update 

        INSERT INTO _AsyncFilterCommands(CommandID, Data1, Data2, Data3, Data4, Status) 
        VALUES(27, @CharID, @JID, @Silk, 5, 1);
        EXEC [appearance].[UpdateRightIcon] @CharName16, 5; -- bronze
    END

    ELSE IF (@SilkHistory >= 300 AND @SilkHistory < 1000)
    BEGIN
	    UPDATE _SilkRank 
        SET SilkRank  = 4
        WHERE CharID = @CharID; -- update 
        INSERT INTO _AsyncFilterCommands(CommandID, Data1, Data2, Data3, Data4, Status) 
        VALUES(27, @CharID, @JID, @Silk, 4, 1);
        EXEC [appearance].[UpdateRightIcon] @CharName16, 4; -- silver
    END
    ELSE IF (@SilkHistory >= 1000 AND @SilkHistory < 5000)
    BEGIN
	    UPDATE _SilkRank 
        SET SilkRank  = 3
        WHERE CharID = @CharID; -- update 
        INSERT INTO _AsyncFilterCommands(CommandID, Data1, Data2, Data3, Data4, Status) 
        VALUES(27, @CharID, @JID, @Silk, 3, 1);
        EXEC [appearance].[UpdateRightIcon] @CharName16, 3; --- gold
    END
    ELSE IF (@SilkHistory >= 5000 AND @SilkHistory < 7500)
    BEGIN
	    UPDATE _SilkRank 
        SET SilkRank  = 2
        WHERE CharID = @CharID; -- update 
        INSERT INTO _AsyncFilterCommands(CommandID, Data1, Data2, Data3, Data4, Status) 
        VALUES(27, @CharID, @JID, @Silk, 2, 2);
        EXEC [appearance].[UpdateRightIcon] @CharName16, 2; --- platinum
    END
    ELSE IF (@SilkHistory >= 7500)
    BEGIN
	    UPDATE _SilkRank 
        SET SilkRank  = 1
        WHERE CharID = @CharID; -- update 
        INSERT INTO _AsyncFilterCommands(CommandID, Data1, Data2, Data3, Data4, Status) 
        VALUES(27, @CharID, @JID, @Silk, 1, 1);
        EXEC [appearance].[UpdateRightIcon] @CharName16, 1; --- vip
    END
  
END

GO

SET ANSI_NULLS ON;
GO
SET QUOTED_IDENTIFIER ON;
GO
-- =============================================
-- Author:		<Author,,Name>
-- Create date: <Create Date,,>
-- Description:	<Description,,>
-- =============================================
CREATE   PROCEDURE [hooks].[OnNpcItemPurchased]
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

	EXEC [achievements].[UpdateProgress] @CharID, 8, 8, @TotalPrice
END


GO

SET ANSI_NULLS ON;
GO
SET QUOTED_IDENTIFIER ON;
GO
CREATE   PROCEDURE [hooks].[OnPartyJoined]
	@CharID int,
	@CharName varchar(25),
	@CurrentRegionId smallint,
	@CurrentWorldId int,
	@PVPCapeType tinyint,
	@CurrentJobType tinyint
AS


	/*
		Bu prosedürü düzenleyerek bir karakter partiye katıldığında bir işlem yapabilirsiniz.
		- Partiyi kuran karakter için bu prosedür çalışmayacaktır.

		@CharID = Partiye katılan karakterin IDsi.
		@CharName = Partiye katılan karakterin ismi.
		@RegionID = Partiye katılan karakterin anlık Region IDsi.
		@WorldID = Partiye katılan karakterin anlık World IDsi.
		@PVPCapeType = Pvp cape
		@JobType = Job bilgisi
		Bu veritabanındaki diğer prosedürlerin aksine bu prosedür her restartta orjinal haline döndürülmeyecektir. Dikkatli düzenleyiniz!
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

SET ANSI_NULLS ON;
GO
SET QUOTED_IDENTIFIER ON;
GO

CREATE   PROCEDURE [hooks].[OnPartyMatchingCreated]
    @CharID           INT,
    @CharName         VARCHAR(25),
    @CurrentRegionId  SMALLINT,
    @CurrentWorldID   INT,
    @PartyNo          INT
AS
BEGIN
    SET NOCOUNT ON;

    BEGIN TRY
        -- لو عايز تمنع تكرار نفس PartyNo، فك الكومنتين دول:
        --IF EXISTS (SELECT 1 FROM dbo.PartyMatchingCreated WITH (NOLOCK) WHERE PartyNo = @PartyNo)
        --    RETURN;

        INSERT INTO dbo._PartyMatchingCreated (CharID, CharName, RegionID, WorldID, PartyNo)
        VALUES (@CharID, @CharName, @CurrentRegionId, @CurrentWorldID, @PartyNo);
    END TRY
    BEGIN CATCH
        PRINT '[_OnPartyMatchingCreated_EDIT] Error: '
              + ERROR_MESSAGE() + ' (Line ' + CAST(ERROR_LINE() AS VARCHAR(10)) + ')';
    END CATCH
END

GO

SET ANSI_NULLS ON;
GO
SET QUOTED_IDENTIFIER ON;
GO
CREATE   PROCEDURE [hooks].[OnSelectableScrollUsed] 
	@CharName16 varchar(128),
	@CharID int,
	@UsedItemSlot tinyint,
	@TargetItemSlot tinyint
as
	BEGIN
	SET NOCOUNT ON;
/*
	TypeID1 = 3, TypeID2 = 3, TypeID3 = 13, TypeID4 = 11 olan item kullanıldığında bu prosedürü çalıştıracaktir. -- bu item her 5 saniyede bir kullanılabilir.

	Kullanılan itemin silinmesi için __LiveItemRemove prosedürü kullanılır.
	Hedef itemin değiştirilmesi için __LiveMutateItem prosedürü kullanılır.
	Model Switcher, Glow Switcher ve Upgrade kullanımı için uygundur
	
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
    EXEC [appearance].[SwitchCustomGlow] @CharName16, @CharID, @TargetItemCodeName, 'GLOW_1', @TargetItemSlot, @UsedItemSlot
END
END
IF(@UsedItemID = 46325) -- GLOW 2
BEGIN

IF @TargetItemCodeName LIKE '%_08_C_RARE%' OR @TargetItemCodeName LIKE '%_08_B_RARE%'
BEGIN
    EXEC [appearance].[SwitchCustomGlow] @CharName16, @CharID, @TargetItemCodeName, 'GLOW_2', @TargetItemSlot, @UsedItemSlot
END

END
IF(@UsedItemID = 46326) -- GLOW 3
BEGIN

IF @TargetItemCodeName LIKE '%_08_C_RARE%' OR @TargetItemCodeName LIKE '%_08_B_RARE%'
BEGIN
    EXEC [appearance].[SwitchCustomGlow] @CharName16, @CharID, @TargetItemCodeName, 'GLOW_3', @TargetItemSlot, @UsedItemSlot
END

END
IF(@UsedItemID = 46327) -- GLOW 4
BEGIN

IF @TargetItemCodeName LIKE '%_08_C_RARE%' OR @TargetItemCodeName LIKE '%_08_B_RARE%'
BEGIN
    EXEC [appearance].[SwitchCustomGlow] @CharName16, @CharID, @TargetItemCodeName, 'GLOW_4', @TargetItemSlot, @UsedItemSlot
END

END

IF(@UsedItemID = 46328) -- MODEL 09
BEGIN

IF @TargetItemCodeName LIKE '%_08_C_RARE%' OR @TargetItemCodeName LIKE '%_08_B_RARE%'
BEGIN
    EXEC [appearance].[SwitchCustomModel] @CharName16, @CharID, @TargetItemCodeName, 'MODEL09', @TargetItemSlot, @UsedItemSlot
END

END

IF(@UsedItemID = 46329) -- MODEL 09
BEGIN

IF @TargetItemCodeName LIKE '%_08_C_RARE%' OR @TargetItemCodeName LIKE '%_08_B_RARE%'
BEGIN
    EXEC [appearance].[SwitchCustomModel] @CharName16, @CharID, @TargetItemCodeName, 'MODEL10', @TargetItemSlot, @UsedItemSlot
END

END

IF(@UsedItemID = 46330) -- MODEL 09
BEGIN

IF @TargetItemCodeName LIKE '%_08_C_RARE%' OR @TargetItemCodeName LIKE '%_08_B_RARE%'
BEGIN
    EXEC [appearance].[SwitchCustomModel] @CharName16, @CharID, @TargetItemCodeName, 'MODEL11', @TargetItemSlot, @UsedItemSlot
END

END

IF(@UsedItemID = 46331) -- MODEL 09
BEGIN

IF @TargetItemCodeName LIKE '%_08_C_RARE%' OR @TargetItemCodeName LIKE '%_08_B_RARE%'
BEGIN
    EXEC [appearance].[SwitchCustomModel] @CharName16, @CharID, @TargetItemCodeName, 'MODEL11_A', @TargetItemSlot, @UsedItemSlot
END

END

IF(@UsedItemID = 46332) -- MODEL 09
BEGIN

IF @TargetItemCodeName LIKE '%_08_C_RARE%' OR @TargetItemCodeName LIKE '%_08_B_RARE%'
BEGIN
    EXEC [appearance].[SwitchCustomModel] @CharName16, @CharID, @TargetItemCodeName, 'MODEL11_B', @TargetItemSlot, @UsedItemSlot
END

END

IF(@UsedItemID = 46333) -- MODEL 09
BEGIN

IF @TargetItemCodeName LIKE '%_08_C_RARE%' OR @TargetItemCodeName LIKE '%_08_B_RARE%'
BEGIN
    EXEC [appearance].[SwitchCustomModel] @CharName16, @CharID, @TargetItemCodeName, 'MODEL12', @TargetItemSlot, @UsedItemSlot
END

END


IF(@UsedItemID = 46334) -- MODEL 09
BEGIN

IF @TargetItemCodeName LIKE '%_08_C_RARE%' OR @TargetItemCodeName LIKE '%_08_B_RARE%'
BEGIN
    EXEC [appearance].[SwitchCustomModel] @CharName16, @CharID, @TargetItemCodeName, 'MODEL13', @TargetItemSlot, @UsedItemSlot
END

END

END

end
GO

SET ANSI_NULLS ON;
GO
SET QUOTED_IDENTIFIER ON;
GO

CREATE   PROCEDURE [hooks].[OnStallCreated]
    @CharID       INT,
    @CharName     NVARCHAR(64),
    @UniqueCharID BIGINT,
    @RegionID     SMALLINT,
    @WorldID      INT,
    @StallTitle   NVARCHAR(128)
AS
BEGIN
    SET NOCOUNT ON;

    INSERT INTO dbo._StallCreateLog
        (CharID, CharName, UniqueCharID, RegionID, WorldID, StallTitle)
    VALUES
        (@CharID, @CharName, @UniqueCharID, @RegionID, @WorldID, @StallTitle);
END

GO

SET ANSI_NULLS ON;
GO
SET QUOTED_IDENTIFIER ON;
GO
CREATE   PROCEDURE [hooks].[OnTeleportControl]
    @CharID INT,
    @RefTeleportID INT,
    @CanTeleport BIT OUTPUT
AS
BEGIN
    SET NOCOUNT ON;

    -- افتراضي يخلي التيلبورت شغال
    SET @CanTeleport = 1;

    -- لو التيلبورت هو رقم 1 يقفله
    IF (@RefTeleportID = 323232)
        SET @CanTeleport = 0;
END

GO

SET ANSI_NULLS ON;
GO
SET QUOTED_IDENTIFIER ON;
GO

CREATE   PROCEDURE [hooks].[OnUniqueEntered]
    @RefObjID INT
AS
BEGIN
    SET NOCOUNT ON;

    ----------------------------------------------------
    -- الجزء الأصلي الخاص بـ KMTGuard (سيبه زي ما هو)
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

SET ANSI_NULLS ON;
GO
SET QUOTED_IDENTIFIER ON;
GO

CREATE   PROCEDURE [hooks].[OnUniqueKilled]
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
    EXEC [achievements].[HandleUniqueKill]
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
        FROM KMTGuard.dbo._CurrentEventTeamList AS cet WITH (NOLOCK)
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

            EXEC [commands].[BroadcastNotice]
                 @NoticeType = 2,
                 @Notice     = @towerMsg;
        END
    END
END

GO

SET ANSI_NULLS ON;
GO
SET QUOTED_IDENTIFIER ON;
GO

CREATE   PROCEDURE [inventory].[AddItemToChest]
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
    INSERT INTO _ItemChest
    VALUES (@CharID, @ItemCodeName, @ItemRefObjID, @Quantity, @FormattedDate, @From, @Plus);

    SET @NewID = SCOPE_IDENTITY();

    -- Async command for the filter (unchanged)
    INSERT INTO _AsyncFilterCommands
        (CommandID, Data1,  Data2,   Data3,         Data4,           Data5,      Data6,           Data7,  Data8, Status)
    VALUES
        (17,        @NewID, @CharID, @ItemCodeName, @ItemRefObjID,   @Quantity,  @FormattedDate,  @From,  @Plus,  1);
END

GO

SET ANSI_NULLS ON;
GO
SET QUOTED_IDENTIFIER ON;
GO
CREATE   PROCEDURE [inventory].[AddItemToChestLegacy]
    @CharID INT,
    @ItemID INT,
    @Quantity INT,
    @Type VARCHAR(100),
    @Plus INT
AS
BEGIN
    SET NOCOUNT ON;
    EXEC [inventory].[AddItemToChest]
        @CharID = @CharID,
        @ItemRefObjID = @ItemID,
        @Quantity = @Quantity,
        @From = @Type,
        @Plus = @Plus;
END;
GO

SET ANSI_NULLS ON;
GO
SET QUOTED_IDENTIFIER ON;
GO
-- =============================================
-- Author:		<Author,,Name>
-- Create date: <Create Date,,>
-- Description:	<Description,,>
-- =============================================
CREATE   PROCEDURE [inventory].[GetItemInfo]
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

SET ANSI_NULLS ON;
GO
SET QUOTED_IDENTIFIER ON;
GO

CREATE   PROCEDURE [live].[AdjustGold]
    @CharID int,
    @Gold bigint,
    @AddOrRemove int
AS
BEGIN
    SET NOCOUNT ON;

    INSERT INTO _AsyncGameServerCommands(Action_ID, Data1, Data2, Data3)
    VALUES(21, @CharID, @Gold, @AddOrRemove);

    -- maximum 2b
END

GO

SET ANSI_NULLS ON;
GO
SET QUOTED_IDENTIFIER ON;
GO
CREATE   PROCEDURE [live].[AdjustSilk]
    @CharID INT,
	@nSilk int,
	@nSilkGift int,
	@nSilkPoint int
AS
    SET NOCOUNT ON;

    -- Kullanıcıyı bul
    DECLARE @JID INT = (SELECT UserJID FROM SRO_VT_SHARD.._User WITH (NOLOCK) WHERE CharID = @CharID);

    -- Eğer @JID geçerli bir değer ise işlemlere devam et
    IF (@JID > 0)
    BEGIN
        IF EXISTS (SELECT 1 FROM SRO_VT_ACCOUNT..SK_Silk WHERE JID = @JID)
        BEGIN
            -- SK_Silk tablosundaki mevcut veriyi değişkenlere çek
            DECLARE @CurrentSilkOwn INT, @CurrentSilkGift INT, @CurrentSilkPoint INT;

            SELECT 
                @CurrentSilkOwn = silk_own,
                @CurrentSilkGift = silk_gift,
                @CurrentSilkPoint = silk_point
            FROM SRO_VT_ACCOUNT..SK_Silk 
            WHERE JID = @JID;

            -- Güncelleme işlemi
            UPDATE SRO_VT_ACCOUNT..SK_Silk 
            SET 
                silk_own = @CurrentSilkOwn + @nSilk,
                silk_gift = @CurrentSilkGift + @nSilkGift,
                silk_point = @CurrentSilkPoint + @nSilkPoint
            WHERE JID = @JID;

            -- Güncellenmiş değerleri ekle
            INSERT INTO _AsyncGameServerCommands(Action_ID, Data1, Data2, Data3, Data4)
            VALUES(20, @CharID, @CurrentSilkOwn + @nSilk, @CurrentSilkGift + @nSilkGift, @CurrentSilkPoint + @nSilkPoint);
        END
        ELSE
        BEGIN
            -- Yeni kayıt ekleme işlemi
            INSERT INTO SRO_VT_ACCOUNT..SK_Silk (JID, silk_own, silk_gift, silk_point)
            VALUES (@JID, @nSilk, @nSilkGift, @nSilkPoint);

            -- Yeni eklenen verileri _AsyncGameServerCommands tablosuna ekle
            INSERT INTO _AsyncGameServerCommands(Action_ID, Data1, Data2, Data3, Data4)
            VALUES(20, @CharID, @nSilk, @nSilkGift, @nSilkPoint);
        END
    END


GO

SET ANSI_NULLS ON;
GO
SET QUOTED_IDENTIFIER ON;
GO
CREATE   PROCEDURE [live].[ApplyBuffByCodeName]
	@CharID int,
	@SkillCodeName varchar(200)
AS		

INSERT INTO _AsyncGameServerCommands(Action_ID, Data1, Data2) VALUES(8, @CharID, @SkillCodeName)


GO

SET ANSI_NULLS ON;
GO
SET QUOTED_IDENTIFIER ON;
GO
CREATE   PROCEDURE [live].[ApplyBuffWithoutLimit]
	@CharID int,
	@SkillID int
AS		

INSERT INTO _AsyncGameServerCommands(Action_ID, Data1, Data2) VALUES(6, @CharID, @SkillID)


GO

SET ANSI_NULLS ON;
GO
SET QUOTED_IDENTIFIER ON;
GO

CREATE   PROCEDURE [live].[ApplyCape]
    @CharID INT,
    @CapeID INT
AS
BEGIN
    SET NOCOUNT ON;

    INSERT INTO _AsyncGameServerCommands (Action_ID, Data1, Data2)
    VALUES (13, @CharID, @CapeID);
END

GO

SET ANSI_NULLS ON;
GO
SET QUOTED_IDENTIFIER ON;
GO
CREATE   PROCEDURE [live].[ConsumeAndMutateItem]
    @CharID INT,
	@MutateSlot tinyint,
    @CodeName128 VARCHAR(128),
	@ConsumeSlot tinyint,
	@ConsumeAmount int
AS
BEGIN
    INSERT INTO _AsyncGameServerCommands(Action_ID, Data1, Data2, Data3, Data4, Data5) VALUES(19, @CharID, @MutateSlot, @CodeName128, @ConsumeSlot, @ConsumeAmount)
END

GO

SET ANSI_NULLS ON;
GO
SET QUOTED_IDENTIFIER ON;
GO
CREATE   PROCEDURE [live].[ConsumeItem]
    @CharID INT,
	@Slot tinyint,
    @ReduceAmount int
AS
BEGIN
    INSERT INTO _AsyncGameServerCommands(Action_ID, Data1, Data2, Data3) VALUES(18, @CharID, @Slot, @ReduceAmount)
END

GO

SET ANSI_NULLS ON;
GO
SET QUOTED_IDENTIFIER ON;
GO
CREATE   PROCEDURE [live].[MutateItem]
    @CharID INT,
	@Slot tinyint,
    @CodeName128 VARCHAR(128)
AS
BEGIN
    INSERT INTO _AsyncGameServerCommands(Action_ID, Data1, Data2, Data3) VALUES(17, @CharID, @Slot, @CodeName128)
END

GO

SET ANSI_NULLS ON;
GO
SET QUOTED_IDENTIFIER ON;
GO
CREATE   PROCEDURE [live].[RemoveBuffBySkillId]
		@CharID int,
	@SkillID int
AS		

INSERT INTO _AsyncGameServerCommands(Action_ID, Data1, Data2) VALUES(7, @CharID, @SkillID)

GO

SET ANSI_NULLS ON;
GO
SET QUOTED_IDENTIFIER ON;
GO
CREATE   PROCEDURE [live].[SetSkillStateByWorldId]
    @WorldID INT,
	@State bit
AS 
BEGIN
	if(@State = 1)
	INSERT INTO _AsyncFilterCommands (CommandID, Data1, Data2, Status) VALUES (20, @WorldID, 'True', 1)
	else
	INSERT INTO _AsyncFilterCommands (CommandID, Data1, Data2, Status) VALUES (20, @WorldID, 'False', 1)
END

GO

SET ANSI_NULLS ON;
GO
SET QUOTED_IDENTIFIER ON;
GO
CREATE   PROCEDURE [live].[SetTeleportStateByGateId]
    @Gate INT,
	@State bit
AS 
BEGIN
	if(@State = 1)
	INSERT INTO _AsyncFilterCommands (CommandID, Data1, Data2, Status) VALUES (18, @Gate, 'True', 1)
	else
	INSERT INTO _AsyncFilterCommands (CommandID, Data1, Data2, Status) VALUES (18, @Gate, 'False', 1)
END

GO

SET ANSI_NULLS ON;
GO
SET QUOTED_IDENTIFIER ON;
GO

CREATE   PROCEDURE [logs].[GetDropLogsPaged]
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

            -- استبعاد الحاجات اللي مش لبس/سلاح
            AND ref.CodeName128 NOT LIKE '%ARCHEMY%'
            AND ref.CodeName128 NOT LIKE '%ALCHEMY%'
            AND ref.CodeName128 NOT LIKE '%MAGICSTONE%'
            AND ref.CodeName128 NOT LIKE '%MAGICSTONE%'
            AND ref.CodeName128 NOT LIKE '%STONE%'
            AND ref.CodeName128 NOT LIKE '%ELIXIR%'
            AND ref.CodeName128 NOT LIKE '%ETC%'
            AND ref.CodeName128 NOT LIKE '%MALL%'

            -- هنا المهم: كل أيتمات الصين وأوروبا، مش RARE بس
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

SET ANSI_NULLS ON;
GO
SET QUOTED_IDENTIFIER ON;
GO

CREATE   PROCEDURE [logs].[GetMonsterPossibleDrops]
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

SET ANSI_NULLS ON;
GO
SET QUOTED_IDENTIFIER ON;
GO

CREATE   PROCEDURE [logs].[GetRareDropLogs]
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

SET ANSI_NULLS ON;
GO
SET QUOTED_IDENTIFIER ON;
GO

CREATE   PROCEDURE [logs].[ProcessCharacterLog]
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
EXEC [live].[ApplyBuffWithoutLimit] @CharID, 35406
end
*/
--EXEC LexaShield_User.._LiveHonorBuff_EDIT @CharID

*/
IF(@EventID = 9)
BEGIN
    DECLARE @SilkRank int = (SELECT SilkRank FROM _SilkRank with (nolock) WHERE CharID = @CharID);

    IF(@SilkRank = 1)
    BEGIN
        EXEC [live].[ApplyBuffByCodeName] @CharID, 'SKILL_VIP_BUFF_01';
		EXEC [commands].[SendNoticeByCharacterId] @CharID, 8, 'You''re VIP (VIP) User.'
    END
    ELSE IF(@SilkRank = 2)
    BEGIN
        EXEC [live].[ApplyBuffByCodeName] @CharID, 'SKILL_VIP_BUFF_02';
		EXEC [commands].[SendNoticeByCharacterId] @CharID, 8, 'You''re VIP (Platinum) User.'
    END
    ELSE IF(@SilkRank = 3)
    BEGIN
        EXEC [live].[ApplyBuffByCodeName] @CharID, 'SKILL_VIP_BUFF_03';
		EXEC [commands].[SendNoticeByCharacterId] @CharID, 8, 'You''re VIP (Gold) User.'
    END
    ELSE IF(@SilkRank = 4)
    BEGIN
        EXEC [live].[ApplyBuffByCodeName] @CharID, 'SKILL_VIP_BUFF_04';
		EXEC [commands].[SendNoticeByCharacterId] @CharID, 8, 'You''re VIP (Silver) User.'
    END
    ELSE IF(@SilkRank = 5)
    BEGIN
        EXEC [live].[ApplyBuffByCodeName] @CharID, 'SKILL_VIP_BUFF_05';
		EXEC [commands].[SendNoticeByCharacterId] @CharID, 8, 'You''re VIP (Bronze) User.'
    END
    ELSE IF(@SilkRank = 6 OR @SilkRank IS NULL)
    BEGIN
        EXEC [live].[ApplyBuffByCodeName] @CharID, 'SKILL_VIP_BUFF_06';
		EXEC [commands].[SendNoticeByCharacterId] @CharID, 8, 'You''re New (Iron) User.'
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
		
		INSERT INTO _AsyncFilterCommands(CommandID, Data1, Data2, Data3, Data4, Status) VALUES(35, @CharnameKiller, @GuildName, @UnionName, @WorldID, 1)

	END
	END

GO

SET ANSI_NULLS ON;
GO
SET QUOTED_IDENTIFIER ON;
GO
CREATE   PROCEDURE [npc].[Kill]
	@CodeName128 varchar(128)
AS
	
	DECLARE @MobRefObjID int = (SELECT ID FROM SRO_VT_SHARD.dbo._RefObjCommon with (nolock) WHERE CodeName128 = @CodeName128)
	IF(@MobRefObjID IS NOT NULL AND @MobRefObjID != 0)
	BEGIN
		INSERT INTO _AsyncGameServerCommands(Action_ID, Data1) VALUES(4, @MobRefObjID)
	END
GO

SET ANSI_NULLS ON;
GO
SET QUOTED_IDENTIFIER ON;
GO
CREATE   PROCEDURE [npc].[KillByWorldId]
	@CodeName128 varchar(128),
	@WorldID int

AS
	
	DECLARE @MobRefObjID int = (SELECT ID FROM SRO_VT_SHARD.dbo._RefObjCommon with (nolock) WHERE CodeName128 = @CodeName128)
	IF(@MobRefObjID IS NOT NULL AND @MobRefObjID != 0)
	BEGIN
		INSERT INTO _AsyncGameServerCommands(Action_ID, Data1, Data2) VALUES(5, @WorldID, @MobRefObjID)
	END

GO

SET ANSI_NULLS ON;
GO
SET QUOTED_IDENTIFIER ON;
GO
CREATE   PROCEDURE [npc].[Spawn]
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
		INSERT INTO _AsyncGameServerCommands(Action_ID, Data1, Data2, Data3, Data4, Data5, Data6, Data7) VALUES(2, @MobRefObjID, @GameWorldID, @RegionId, @PosX, @PosY, @PosZ, @GenerateRadius)
	END
GO

SET ANSI_NULLS ON;
GO
SET QUOTED_IDENTIFIER ON;
GO
CREATE   PROCEDURE [npc].[SpawnMonsterAtPosition]
	@MonsterID int,
	@GameWorldID int,
	@RegionId int,
	@PosX int,
	@PosY int,
	@PosZ int,
	@GenerateRadius int
AS		

INSERT INTO _AsyncGameServerCommands(Action_ID, Data1, Data2, Data3, Data4, Data5, Data6, Data7) VALUES(2, @MonsterID, @GameWorldID, @RegionId, @PosX, @PosY, @PosZ, @GenerateRadius)


GO

SET ANSI_NULLS ON;
GO
SET QUOTED_IDENTIFIER ON;
GO
CREATE   PROCEDURE [npc].[SpawnMonsterNearCharacter]
	@CharID int,
	@MonsterID int
AS		

INSERT INTO _AsyncGameServerCommands(Action_ID, Data1, Data2) VALUES(3, @CharID, @MonsterID)


GO

SET ANSI_NULLS ON;
GO
SET QUOTED_IDENTIFIER ON;
GO
-- =============================================
-- Author:		<Author,,Name>
-- Create date: <Create Date,,>
-- Description:	<Description,,>
-- =============================================
CREATE   PROCEDURE [npc].[SynchronizeList]
	-- Add the parameters for the stored procedure here
	@UniqueID int,
	@RefObjId int,
	@CodeName128 VARCHAR(128)
AS
BEGIN
	-- SET NOCOUNT ON added to prevent extra result sets from
	-- interfering with SELECT statements.
	SET NOCOUNT ON;

	IF EXISTS ( Select 1 from ___SR_GSNpcUniqueIdList where CodeName128 = @CodeName128)
	BEGIN
		UPDATE ___SR_GSNpcUniqueIdList set UniqueID = @UniqueID, RefObjId = @RefObjId where CodeName128 = @CodeName128
	END
	ELSE
	BEGIN
	INSERT INTO ___SR_GSNpcUniqueIdList (UniqueID, RefObjId, CodeName128) VALUES (@UniqueID, @RefObjId, @CodeName128)
	END
END


GO

SET ANSI_NULLS ON;
GO
SET QUOTED_IDENTIFIER ON;
GO
-- =============================================
-- Author:		<Author,,Name>
-- Create date: <Create Date,,>
-- Description:	<Description,,>
-- =============================================
CREATE   PROCEDURE [player].[GetFellowPetId64]
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

SET ANSI_NULLS ON;
GO
SET QUOTED_IDENTIFIER ON;
GO
-- =============================================
-- Author:		<Author,,Name>
-- Create date: <Create Date,,>
-- Description:	<Description,,>
-- =============================================
CREATE   PROCEDURE [player].[SaveLocation]
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
	
	IF NOT EXISTS (select 1 from _NewReverseSavedLocations with (nolock) WHERE CharID = @CharID AND LocationID = @LocationID)
	BEGIN
	INSERT INTO _NewReverseSavedLocations (CharID, LocationID, RegionID, PosX, PosY, PosZ, WorldID) VALUES (@CharID, @LocationID, @RegionID, @PosX, @PosY, @PosZ, @WorldID)
	SET @Success = 1
	END
END


GO

SET ANSI_NULLS ON;
GO
SET QUOTED_IDENTIFIER ON;
GO
CREATE   PROCEDURE [player].[UpdateClientConfig]
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

SET ANSI_NULLS ON;
GO
SET QUOTED_IDENTIFIER ON;
GO
-- =============================================
-- Author:		<Author,,Name>
-- Create date: <Create Date,,>
-- Description:	<Description,,>
-- =============================================
CREATE   PROCEDURE [player].[UpdateFellowData]
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
			IF EXISTS (Select 1 from _FellowSkillData where ID64 = @ID64)
		BEGIN
			UPDATE _FellowSkillData set Enable_Skill_1=@Enable_Skill_1, Enable_Skill_2=@Enable_Skill_2 
			,Enable_Skill_3=@Enable_Skill_3, Enable_Skill_4=@Enable_Skill_4, Enable_Skill_5=@Enable_Skill_5 where ID64 = @ID64
		END
		ELSE
		BEGIN
		INSERT INTO _FellowSkillData VALUES (@ID64, @Enable_Skill_1,@Enable_Skill_2, @Enable_Skill_3, @Enable_Skill_4, @Enable_Skill_5)
		END
END


GO

SET ANSI_NULLS ON;
GO
SET QUOTED_IDENTIFIER ON;
GO
CREATE   PROCEDURE [teleport].[AllToTownByWorldId]
	@WorldID int
AS		

INSERT INTO _AsyncGameServerCommands(Action_ID, Data1) VALUES(11, @WorldID)


GO

SET ANSI_NULLS ON;
GO
SET QUOTED_IDENTIFIER ON;
GO

CREATE   PROCEDURE [teleport].[IfSafeZoneAndNoJob]
	@CHARID int,
	@GameWorldID int,
	@RegionId int,
	@PosX int,
	@PosY int,
	@PosZ int
AS		

INSERT INTO _AsyncGameServerCommands(Action_ID, Data1, Data2, Data3, Data4, Data5, Data6) VALUES(24, @CHARID, @GameWorldID, @RegionId, @PosX, @PosY, @PosZ)


GO

SET ANSI_NULLS ON;
GO
SET QUOTED_IDENTIFIER ON;
GO

CREATE   PROCEDURE [teleport].[SelfTeleport]
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
        FROM dbo._AsyncFilterCommands WITH (UPDLOCK, HOLDLOCK)
        WHERE CommandID = 41
          AND Status = 1
          AND TRY_CONVERT(INT, Data1) = @CharID
    )
    BEGIN
        INSERT INTO dbo._AsyncFilterCommands
            (CommandID, Data1, Status)
        VALUES
            (41, CONVERT(VARCHAR(20), @CharID), 1);
    END;

    COMMIT TRANSACTION;
END;

GO

SET ANSI_NULLS ON;
GO
SET QUOTED_IDENTIFIER ON;
GO

CREATE   PROCEDURE [teleport].[ToPosition]
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
        COALESCE(NULLIF((SELECT TOP (1) [Value] FROM dbo.__Settings WITH (NOLOCK) WHERE SettingName = N'ShardDB'), N''), N'SRO_VT_SHARD');

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

SET ANSI_NULLS ON;
GO
SET QUOTED_IDENTIFIER ON;
GO

CREATE   PROCEDURE [teleport].[ToPositionWithFreeze]
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

    INSERT INTO dbo._TeleportFreezeCommand
        (CharID, GameWorldID, RegionID, PosX, PosY, PosZ, FreezeSeconds, Status)
    VALUES
        (@CharID, @GameWorldID, @RegionId, @PosX, @PosY, @PosZ, @FreezeSeconds, 0);

    SELECT CONVERT(bigint, SCOPE_IDENTITY()) AS CommandID;
END

GO

SET ANSI_NULLS ON;
GO
SET QUOTED_IDENTIFIER ON;
GO

CREATE   PROCEDURE [teleport].[ToTownByCharacterId]
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
    FROM dbo._PvpChallengeConfig WITH (NOLOCK)
    ORDER BY ID;

    EXEC [teleport].[ToPosition]
        @CharID = @CharID,
        @GameWorldID = @GameWorldID,
        @RegionId = @RegionID,
        @PosX = @PosX,
        @PosY = @PosY,
        @PosZ = @PosZ;
END

GO

SET ANSI_NULLS ON;
GO
SET QUOTED_IDENTIFIER ON;
GO

CREATE   PROCEDURE [trade].[BumpRollingMember]
    @CharID INT
AS
BEGIN
    SET NOCOUNT ON;

    DECLARE @now DATETIME2(0) = SYSUTCDATETIME();
    DECLARE @hwid NVARCHAR(128);

    SELECT TOP(1) @hwid = UPPER(LTRIM(RTRIM(CONVERT(NVARCHAR(128), Hwid))))
    FROM dbo._HwidList WITH (NOLOCK)
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

SET ANSI_NULLS ON;
GO
SET QUOTED_IDENTIFIER ON;
GO

CREATE   PROCEDURE [trade].[CompleteGoodsPurchase]
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
    FROM dbo._HwidList WITH (NOLOCK)
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

SET ANSI_NULLS ON;
GO
SET QUOTED_IDENTIFIER ON;
GO

CREATE   PROCEDURE [trade].[CompleteGoodsSale]
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
            FROM KMTGuard.dbo._PartyData WITH (NOLOCK)
            WHERE CharID = @CharID;

        COMMIT TRAN;

        -- Bonus logic (outside locks)

        -- THIEF
        IF (@SellerJob = @JOB_THIEF)
        BEGIN
            IF (@PartyID IS NOT NULL AND EXISTS (
                    SELECT 1
                    FROM KMTGuard.dbo._PartyData WITH (NOLOCK)
                    WHERE PartyID = @PartyID AND JobStatus = @JOB_THIEF AND CharID <> @CharID
                ))
            BEGIN
                DECLARE @TCharID INT, @TCharName VARCHAR(25);

                DECLARE curThieves CURSOR LOCAL FAST_FORWARD FOR
                    SELECT CharID, CharName
                    FROM KMTGuard.dbo._PartyData WITH (NOLOCK)
                    WHERE PartyID = @PartyID AND JobStatus = @JOB_THIEF;

                OPEN curThieves;
                FETCH NEXT FROM curThieves INTO @TCharID, @TCharName;

                WHILE @@FETCH_STATUS = 0
                BEGIN
                    EXEC [trade].[GrantRollingBonus]
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
                EXEC [trade].[GrantRollingBonus]
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
            IF EXISTS (SELECT 1 FROM KMTGuard.dbo._PartyData WITH (NOLOCK) WHERE PartyID=@PartyID AND JobStatus=@JOB_HUNTER)
            BEGIN
                EXEC [trade].[GrantRollingBonus]
                    @CharID = @CharID,
                    @CharName = @Charname,
                    @Gold = @BONUS_TRADER_WITH_HUNTER,
                    @JobStatus = @JOB_TRADER,
                    @PartyID = @PartyID,
                    @Reason = 'Trader Bonus (+20M)';

                DECLARE @HCharID INT, @HCharName VARCHAR(25);

                DECLARE curHunters CURSOR LOCAL FAST_FORWARD FOR
                    SELECT CharID, CharName
                    FROM KMTGuard.dbo._PartyData WITH (NOLOCK)
                    WHERE PartyID=@PartyID AND JobStatus=@JOB_HUNTER;

                OPEN curHunters;
                FETCH NEXT FROM curHunters INTO @HCharID, @HCharName;

                WHILE @@FETCH_STATUS = 0
                BEGIN
                    EXEC [trade].[GrantRollingBonus]
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
            IF EXISTS (SELECT 1 FROM KMTGuard.dbo._PartyData WITH (NOLOCK) WHERE PartyID=@PartyID AND JobStatus=@JOB_TRADER)
            BEGIN
                EXEC [trade].[GrantRollingBonus]
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

        EXEC [commands].[SendNoticeByCharacterId] @CharID, 6, @msg;
    END CATCH
END
*/
GO

SET ANSI_NULLS ON;
GO
SET QUOTED_IDENTIFIER ON;
GO

CREATE   PROCEDURE [trade].[GiveBonus]
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
                    EXEC [commands].[SendNoticeByCharacterName] @Name16, 6, @CapMsg;
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
            INSERT INTO KMTGuard.dbo._AsyncGameServerCommands(Action_ID, Data1, Data2, Data3)
            VALUES(21, @CharID, @AmountGold, 1);

        COMMIT TRAN;

        DECLARE @Name16Ok VARCHAR(16) = LEFT(@CharName, 16);

        DECLARE @msg VARCHAR(MAX) =
            'Trade Bonus: +' + CONVERT(VARCHAR(20), @AmountGold) +
            ' gold (' + CONVERT(VARCHAR(10), @c) + '/5 today)' +
            CASE WHEN @Reason IS NOT NULL THEN ' | ' + @Reason ELSE '' END;

        -- ✅ positional call (NO named params)
        EXEC [commands].[SendNoticeByCharacterName] @Name16Ok, 6, @msg;

    END TRY
    BEGIN CATCH
        IF @@TRANCOUNT > 0 ROLLBACK TRAN;

        DECLARE @Name16Err VARCHAR(16) = LEFT(@CharName, 16);
        DECLARE @err VARCHAR(4000) = ERROR_MESSAGE();
        DECLARE @errMsg VARCHAR(MAX) = 'Trade Bonus Error: ' + LEFT(@err, 200);

        -- ✅ positional call (NO named params)
        EXEC [commands].[SendNoticeByCharacterName] @Name16Err, 6, @errMsg;
    END CATCH
END

GO

SET ANSI_NULLS ON;
GO
SET QUOTED_IDENTIFIER ON;
GO

CREATE   PROCEDURE [trade].[GrantRollingBonus]
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
            EXEC [live].[AdjustGold] @CharID, @Gold, 1;

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
        EXEC [trade].[BumpRollingMember] @CharID;

        DECLARE @msg VARCHAR(MAX);
        SET @msg = 'Trade Bonus received.';
        IF (@Reason IS NOT NULL) SET @msg = @msg + ' ' + @Reason;

        EXEC [commands].[SendNoticeByCharacterId] @CharID, 6, @msg;

    END TRY
    BEGIN CATCH
        IF @@TRANCOUNT > 0 ROLLBACK TRAN;

        DECLARE @err VARCHAR(4000);
        DECLARE @errMsg VARCHAR(MAX);

        SET @err = ERROR_MESSAGE();
        SET @errMsg = 'Trade Bonus Error: ' + LEFT(@err, 200);

        EXEC [commands].[SendNoticeByCharacterId] @CharID, 6, @errMsg;
    END CATCH
END

GO

SET ANSI_NULLS ON;
GO
SET QUOTED_IDENTIFIER ON;
GO

CREATE   PROCEDURE [trade].[ValidateGoodsPurchase]
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

                EXEC [commands].[SendNoticeByCharacterId] @CharID, 6, @notice;

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
            FROM KMTGuard.dbo._PartyData WITH (NOLOCK)
            WHERE CharID = @CharID;

            IF (@BuyerJob = @JOB_TRADER AND @PartyID IS NOT NULL)
            BEGIN
                DECLARE @BadHunterID INT;
                SET @BadHunterID = NULL;

                SELECT TOP(1) @BadHunterID = pd.CharID
                FROM KMTGuard.dbo._PartyData pd WITH (NOLOCK)
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

                    EXEC [commands].[SendNoticeByCharacterId] @CharID, 6, @notice2;

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

SET ANSI_NULLS ON;
GO
SET QUOTED_IDENTIFIER ON;
GO

CREATE   PROCEDURE [trade].[ValidateGoodsSale]
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

        EXEC [commands].[SendNoticeByCharacterId] @CharID, 6, @notice;
        RETURN;
    END

    -- Party-aware block: Trader cannot sell if any Hunter in party reached limit
    DECLARE @PartyID INT, @SellerJob TINYINT;
    DECLARE @JOB_TRADER TINYINT; SET @JOB_TRADER = 1;
    DECLARE @JOB_HUNTER TINYINT; SET @JOB_HUNTER = 3;

    SET @PartyID = NULL;
    SET @SellerJob = NULL;

    SELECT @PartyID = PartyID, @SellerJob = JobStatus
    FROM KMTGuard.dbo._PartyData WITH (NOLOCK)
    WHERE CharID = @CharID;

    IF (@SellerJob = @JOB_TRADER AND @PartyID IS NOT NULL)
    BEGIN
        DECLARE @BadHunterID INT;
        SET @BadHunterID = NULL;

        SELECT TOP(1) @BadHunterID = pd.CharID
        FROM KMTGuard.dbo._PartyData pd WITH (NOLOCK)
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

            EXEC [commands].[SendNoticeByCharacterId] @CharID, 6, @notice2;
            RETURN;
        END
    END

    SET @IsBlocked = 0;
END
*/
GO

SET ANSI_NULLS ON;
GO
SET QUOTED_IDENTIFIER ON;
GO
-- =============================================
-- Author:		<Author,,Name>
-- Create date: <Create Date,,>
-- Description:	<Description,,>
-- =============================================
CREATE   PROCEDURE [ui].[SaveAutoPotionMacro]
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
	IF EXISTS (SELECT 1 FROM _MacroAutoPotion WHERE CharID = @CharID AND Slot = @Slot)
    UPDATE _MacroAutoPotion SET Active = @Active, Value = @Value WHERE CharID = @CharID AND Slot = @Slot
    ELSE
    INSERT INTO _MacroAutoPotion (CharID, Slot, Active, Value) VALUES (@CharID, @Slot, @Active, @Value)

END


GO

SET ANSI_NULLS ON;
GO
SET QUOTED_IDENTIFIER ON;
GO
-- =============================================
-- Author:		<Author,,Name>
-- Create date: <Create Date,,>
-- Description:	<Description,,>
-- =============================================
CREATE   PROCEDURE [ui].[SaveCharacterSettings]
	-- Add the parameters for the stored procedure here
	@CharName16 VARCHAR(32),
	@Value bit
AS
BEGIN
	-- SET NOCOUNT ON added to prevent extra result sets from
	-- interfering with SELECT statements.
	SET NOCOUNT ON;
		IF EXISTS (Select 1 from _CharacterSettings where CharName16 = @CharName16)
		BEGIN
			UPDATE _CharacterSettings set HideItemInfo = @Value where CharName16 = @CharName16
		END
		ELSE
		BEGIN
		INSERT INTO _CharacterSettings (CharName16, HideItemInfo) VALUES (@CharName16, @Value)
		END
END


GO

SET ANSI_NULLS ON;
GO
SET QUOTED_IDENTIFIER ON;
GO
CREATE   PROCEDURE [ui].[UpdateMacroSettings]
    @CharID int,
    @AutoPotion bit,
    @AutoSkill bit,
    @AutoHunt bit,
	@AutoPickup bit,
	@AutoScroll bit
AS


    BEGIN
	SET NOCOUNT ON;
    IF EXISTS(Select 1 from _MacroSetting with(nolock) where CharID = @CharID)
    BEGIN
    	UPDATE _MacroSetting SET AutoPotion=@AutoPotion, AutoSkill=@AutoSkill, AutoHunt=@AutoHunt, AutoPickup=@AutoPickup, 
		AutoScroll=@AutoScroll where CharID = @CharID
    END
    ELSE
    BEGIN
    	INSERT INTO _MacroSetting VALUES(@CharID, @AutoPotion, @AutoSkill, @AutoHunt, @AutoPickup, @AutoScroll)
    END
    END

GO

SET ANSI_NULLS ON;
GO
SET QUOTED_IDENTIFIER ON;
GO
CREATE TRIGGER dbo.trg_EventSchedule_VersionBump
ON dbo.EventSchedule
AFTER INSERT, UPDATE, DELETE
AS
BEGIN
    SET NOCOUNT ON;
    UPDATE dbo._EventScheduleMeta SET [Version] = [Version] + 1 WHERE Id=1;
END

GO

SET ANSI_NULLS ON;
GO
SET QUOTED_IDENTIFIER ON;
GO

CREATE   VIEW dbo._BattlePassAdminPlayerStatus
AS
SELECT
    p.SeasonID, s.Name AS SeasonName, p.OwnerKey, p.JID, p.LastCharID,
    p.CurrentXP, p.CurrentLevel, p.IsPremium, p.PremiumPurchasedAtUtc,
    p.CreatedAtUtc, p.UpdatedAtUtc
FROM dbo._BattlePassPlayers p
JOIN dbo._BattlePassSeasons s ON s.ID=p.SeasonID;

GO

SET ANSI_NULLS ON;
GO
SET QUOTED_IDENTIFIER ON;
GO

CREATE VIEW dbo.vw_Settings_InvalidValues
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
    FROM dbo.__Settings
)
SELECT SettingName, Value, ExpectedType
FROM typed
WHERE
    (ExpectedType = 'bool' AND Value NOT IN ('True', 'False'))
    OR (ExpectedType = 'int' AND TRY_CONVERT(INT, Value) IS NULL);

GO

SET ANSI_NULLS ON;
GO
SET QUOTED_IDENTIFIER ON;
GO

CREATE VIEW dbo.vw_Settings_Organized
AS
SELECT
    Category,
    DisplayOrder,
    SettingName,
    Value,
    Description
FROM dbo.__Settings;

GO

