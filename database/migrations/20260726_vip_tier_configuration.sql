/*
    Configurable VIP/Silk loyalty tiers for KMTGuard.

    Run on the KMTGuard database. The migration:
      - preserves the existing six tier thresholds and icon IDs;
      - replaces the hard-coded Item Mall rank calculation;
      - makes the optional tier buff data-driven;
      - fixes the Platinum queue row that previously used Status = 2;
      - keeps Rank_Silk authoritative and refreshes filter/client rank caches.
*/

SET NOCOUNT ON;
SET XACT_ABORT ON;

IF OBJECT_ID(N'dbo.Vip_Tiers', N'U') IS NULL
BEGIN
    CREATE TABLE dbo.Vip_Tiers
    (
        RankCode INT NOT NULL,
        DisplayName NVARCHAR(32) NOT NULL,
        MinSilk INT NOT NULL,
        IconID INT NOT NULL,
        BuffSkillCode VARCHAR(128) NULL,
        UpdatedAt DATETIME2(0) NOT NULL
            CONSTRAINT DF_VipTiers_UpdatedAt DEFAULT (SYSUTCDATETIME()),
        CONSTRAINT PK_VipTiers PRIMARY KEY CLUSTERED (RankCode),
        CONSTRAINT UQ_VipTiers_MinSilk UNIQUE (MinSilk),
        CONSTRAINT UQ_VipTiers_IconID UNIQUE (IconID),
        CONSTRAINT CK_VipTiers_RankCode CHECK (RankCode BETWEEN 1 AND 6),
        CONSTRAINT CK_VipTiers_MinSilk CHECK (MinSilk >= 0),
        CONSTRAINT CK_VipTiers_IconID CHECK (IconID > 0)
    );
END;

MERGE dbo.Vip_Tiers AS target
USING
(
    VALUES
        (6, N'Iron',        0, 6, CAST('SKILL_VIP_BUFF_06' AS VARCHAR(128))),
        (5, N'Bronze',    100, 5, CAST('SKILL_VIP_BUFF_05' AS VARCHAR(128))),
        (4, N'Silver',    300, 4, CAST('SKILL_VIP_BUFF_04' AS VARCHAR(128))),
        (3, N'Gold',     1000, 3, CAST('SKILL_VIP_BUFF_03' AS VARCHAR(128))),
        (2, N'Platinum', 5000, 2, CAST('SKILL_VIP_BUFF_02' AS VARCHAR(128))),
        (1, N'VIP',      7500, 1, CAST('SKILL_VIP_BUFF_01' AS VARCHAR(128)))
) AS source(RankCode, DisplayName, MinSilk, IconID, BuffSkillCode)
    ON target.RankCode = source.RankCode
WHEN NOT MATCHED THEN
    INSERT (RankCode, DisplayName, MinSilk, IconID, BuffSkillCode)
    VALUES (source.RankCode, source.DisplayName, source.MinSilk, source.IconID, source.BuffSkillCode);

/*
    Legacy installations often contain the SKILL_VIP_BUFF_* text without the
    corresponding _RefSkill rows. Keep valid legacy skills, but turn missing
    ones into the safe "no buff" state exposed by the dashboard.
*/
DECLARE @SeedShardDB SYSNAME = COALESCE
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

IF DB_ID(@SeedShardDB) IS NOT NULL
BEGIN
    DECLARE @CleanInvalidSeedSkills NVARCHAR(MAX) =
        N'UPDATE tier
          SET BuffSkillCode = NULL,
              UpdatedAt = SYSUTCDATETIME()
          FROM dbo.Vip_Tiers AS tier
          WHERE tier.BuffSkillCode IS NOT NULL
            AND NOT EXISTS
            (
                SELECT 1
                FROM ' + QUOTENAME(@SeedShardDB) + N'.dbo._RefSkill AS skill WITH (NOLOCK)
                WHERE skill.Basic_Code COLLATE DATABASE_DEFAULT = tier.BuffSkillCode
                  AND skill.Service = 1
            );';
    EXEC sys.sp_executesql @CleanInvalidSeedSkills;
END;

IF COL_LENGTH(N'dbo.Rank_Silk', N'AppliedVipBuffSkillID') IS NULL
    ALTER TABLE dbo.Rank_Silk ADD AppliedVipBuffSkillID INT NULL;

IF COL_LENGTH(N'dbo.Rank_Silk', N'AppliedVipBuffCode') IS NULL
    ALTER TABLE dbo.Rank_Silk ADD AppliedVipBuffCode VARCHAR(128) NULL;
GO

CREATE OR ALTER PROCEDURE dbo.Vip_ResolveSkillID
    @SkillCode VARCHAR(128),
    @SkillID INT OUTPUT
AS
BEGIN
    SET NOCOUNT ON;
    SET @SkillID = NULL;

    IF NULLIF(LTRIM(RTRIM(@SkillCode)), '') IS NULL
        RETURN;

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

    IF DB_ID(@ShardDB) IS NULL
        RETURN;

    DECLARE @Sql NVARCHAR(MAX) =
        N'SELECT TOP (1) @ResolvedID = ID
          FROM ' + QUOTENAME(@ShardDB) + N'.dbo._RefSkill WITH (NOLOCK)
          WHERE Basic_Code = @Code AND Service = 1;';

    EXEC sys.sp_executesql
        @Sql,
        N'@Code VARCHAR(128), @ResolvedID INT OUTPUT',
        @Code = @SkillCode,
        @ResolvedID = @SkillID OUTPUT;
END;
GO

CREATE OR ALTER PROCEDURE dbo.Vip_ApplyConfiguredBuff
    @CharID INT,
    @RankCode INT,
    @ForceRefresh BIT = 0
AS
BEGIN
    SET NOCOUNT ON;

    DECLARE @BuffCode VARCHAR(128);
    DECLARE @NewSkillID INT;
    DECLARE @OldSkillID INT;
    DECLARE @OldBuffCode VARCHAR(128);

    SELECT @BuffCode = NULLIF(LTRIM(RTRIM(BuffSkillCode)), '')
    FROM dbo.Vip_Tiers WITH (NOLOCK)
    WHERE RankCode = @RankCode;

    SELECT
        @OldSkillID = AppliedVipBuffSkillID,
        @OldBuffCode = AppliedVipBuffCode
    FROM dbo.Rank_Silk WITH (UPDLOCK)
    WHERE CharID = @CharID;

    IF @BuffCode IS NOT NULL
        EXEC dbo.Vip_ResolveSkillID @BuffCode, @NewSkillID OUTPUT;

    /*
        Invalid codes are ignored here so a bad manual DB edit can never roll
        back an Item Mall purchase. The Admin Desktop validates codes on save.
    */
    IF @BuffCode IS NOT NULL AND @NewSkillID IS NULL
        RETURN;

    IF @OldSkillID IS NOT NULL
       AND
       (
           @NewSkillID IS NULL
           OR @OldSkillID <> @NewSkillID
           OR @ForceRefresh = 1
       )
    BEGIN
        EXEC dbo.Live_RemoveBuff @CharID, @OldSkillID;
    END;

    IF @NewSkillID IS NOT NULL
       AND
       (
           @OldSkillID IS NULL
           OR @OldSkillID <> @NewSkillID
           OR ISNULL(@OldBuffCode, '') <> @BuffCode
           OR @ForceRefresh = 1
       )
    BEGIN
        EXEC dbo.Live_AddBuff @CharID, @BuffCode;
    END;

    UPDATE dbo.Rank_Silk
    SET AppliedVipBuffSkillID = @NewSkillID,
        AppliedVipBuffCode = @BuffCode
    WHERE CharID = @CharID;
END;
GO

CREATE OR ALTER PROCEDURE dbo.Vip_OnCharacterLogin
    @CharID INT
AS
BEGIN
    SET NOCOUNT ON;

    DECLARE @SilkHistory INT;
    DECLARE @RankCode INT;
    DECLARE @DisplayName NVARCHAR(32);

    SELECT @SilkHistory = SilkHistory
    FROM dbo.Rank_Silk WITH (UPDLOCK)
    WHERE CharID = @CharID;

    IF @SilkHistory IS NULL
        RETURN;

    SELECT TOP (1)
        @RankCode = RankCode,
        @DisplayName = DisplayName
    FROM dbo.Vip_Tiers WITH (NOLOCK)
    WHERE MinSilk <= @SilkHistory
    ORDER BY MinSilk DESC, RankCode;

    SET @RankCode = ISNULL(@RankCode, 0);

    UPDATE dbo.Rank_Silk
    SET SilkRank = @RankCode
    WHERE CharID = @CharID
      AND SilkRank <> @RankCode;

    EXEC dbo.Vip_ApplyConfiguredBuff
        @CharID = @CharID,
        @RankCode = @RankCode,
        @ForceRefresh = 1;

    IF @RankCode > 0
    BEGIN
        DECLARE @Notice VARCHAR(128) =
            'Your VIP rank is ' + CONVERT(VARCHAR(32), @DisplayName) + '.';
        EXEC dbo.Command_NoticeByID
            @CharID,
            8,
            @Notice;
    END;
END;
GO

CREATE OR ALTER PROCEDURE dbo.Vip_RecalculateAll
AS
BEGIN
    SET NOCOUNT ON;
    SET XACT_ABORT ON;

    CREATE TABLE #VipState
    (
        CharID INT NOT NULL PRIMARY KEY,
        CharName16 VARCHAR(16) COLLATE DATABASE_DEFAULT NULL,
        RankCode INT NOT NULL,
        IconID INT NULL
    );

    INSERT #VipState (CharID, RankCode, IconID)
    SELECT
        rankRow.CharID,
        ISNULL(tier.RankCode, 0),
        tier.IconID
    FROM dbo.Rank_Silk AS rankRow WITH (UPDLOCK)
    OUTER APPLY
    (
        SELECT TOP (1) configured.RankCode, configured.IconID
        FROM dbo.Vip_Tiers AS configured WITH (NOLOCK)
        WHERE configured.MinSilk <= rankRow.SilkHistory
        ORDER BY configured.MinSilk DESC, configured.RankCode
    ) AS tier;

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
    BEGIN
        DECLARE @Sql NVARCHAR(MAX) =
            N'UPDATE state
              SET CharName16 = characterRow.CharName16
              FROM #VipState AS state
              INNER JOIN ' + QUOTENAME(@ShardDB) + N'.dbo._Char AS characterRow WITH (NOLOCK)
                  ON characterRow.CharID = state.CharID;';
        EXEC sys.sp_executesql @Sql;
    END;

    UPDATE rankRow
    SET SilkRank = state.RankCode
    FROM dbo.Rank_Silk AS rankRow
    INNER JOIN #VipState AS state ON state.CharID = rankRow.CharID;

    UPDATE activeIcon
    SET IconID = state.IconID
    FROM dbo.ActiveRightIcons AS activeIcon
    INNER JOIN #VipState AS state ON state.CharName16 = activeIcon.CharName16
    WHERE state.RankCode > 0
      AND activeIcon.IconID <> state.IconID;

    INSERT dbo.ActiveRightIcons (CharName16, IconID)
    SELECT state.CharName16, state.IconID
    FROM #VipState AS state
    WHERE state.RankCode > 0
      AND state.CharName16 IS NOT NULL
      AND NOT EXISTS
      (
          SELECT 1
          FROM dbo.ActiveRightIcons AS activeIcon
          WHERE activeIcon.CharName16 = state.CharName16
      );

    DELETE activeIcon
    FROM dbo.ActiveRightIcons AS activeIcon
    INNER JOIN #VipState AS state ON state.CharName16 = activeIcon.CharName16
    WHERE state.RankCode = 0
      AND
      (
          activeIcon.IconID BETWEEN 1 AND 6
          OR EXISTS
          (
              SELECT 1
              FROM dbo.Vip_Tiers AS tier
              WHERE tier.IconID = activeIcon.IconID
          )
      );

    INSERT dbo.Command_FilterQueue (CommandID, Data1, Data2, Status)
    SELECT 11, state.CharName16, state.IconID, 1
    FROM #VipState AS state
    WHERE state.RankCode > 0
      AND state.CharName16 IS NOT NULL;

    INSERT dbo.Command_FilterQueue (CommandID, Data1, Status)
    SELECT 13, state.CharName16, 1
    FROM #VipState AS state
    WHERE state.RankCode = 0
      AND state.CharName16 IS NOT NULL;

    INSERT dbo.Command_FilterQueue (CommandID, Data1, Status)
    SELECT 27, state.CharID, 1
    FROM #VipState AS state;
END;
GO

CREATE OR ALTER PROCEDURE dbo.Hook_ItemMallBuy
    @JID INT,
    @CharID INT,
    @CharName16 VARCHAR(16),
    @ItemID INT,
    @Silk INT
AS
BEGIN
    SET NOCOUNT ON;
    SET XACT_ABORT ON;

    IF @Silk <= 0
        RETURN;

    IF @ItemID BETWEEN 45837 AND 45844
        RETURN;

    EXEC dbo.Achievement_Update @CharID, 9, 9, @Silk;

    DECLARE @OldRankCode INT = 0;
    DECLARE @NewRankCode INT = 0;
    DECLARE @NewIconID INT;

    SELECT @OldRankCode = SilkRank
    FROM dbo.Rank_Silk WITH (UPDLOCK, HOLDLOCK)
    WHERE CharID = @CharID;

    IF @@ROWCOUNT = 0
    BEGIN
        INSERT dbo.Rank_Silk (JID, CharID, SilkHistory, SilkRank)
        VALUES (@JID, @CharID, @Silk, 0);
    END
    ELSE
    BEGIN
        UPDATE dbo.Rank_Silk
        SET SilkHistory =
                CASE
                    WHEN SilkHistory > 2147483647 - @Silk THEN 2147483647
                    ELSE SilkHistory + @Silk
                END,
            JID = @JID
        WHERE CharID = @CharID;
    END;

    SELECT TOP (1)
        @NewRankCode = tier.RankCode,
        @NewIconID = tier.IconID
    FROM dbo.Rank_Silk AS rankRow
    INNER JOIN dbo.Vip_Tiers AS tier WITH (NOLOCK)
        ON tier.MinSilk <= rankRow.SilkHistory
    WHERE rankRow.CharID = @CharID
    ORDER BY tier.MinSilk DESC, tier.RankCode;

    SET @NewRankCode = ISNULL(@NewRankCode, 0);

    UPDATE dbo.Rank_Silk
    SET SilkRank = @NewRankCode
    WHERE CharID = @CharID;

    INSERT dbo.Command_FilterQueue
        (CommandID, Data1, Data2, Data3, Data4, Status)
    VALUES
        (27, @CharID, @JID, @Silk, @NewRankCode, 1);

    IF @NewRankCode > 0
    BEGIN
        EXEC dbo.RightIcon_Add @CharID, @NewIconID;
        EXEC dbo.RightIcon_Activate @CharID, @CharName16, @NewIconID;
    END
    ELSE IF EXISTS
    (
        SELECT 1
        FROM dbo.ActiveRightIcons AS activeIcon
        WHERE activeIcon.CharName16 = @CharName16
          AND
          (
              activeIcon.IconID BETWEEN 1 AND 6
              OR EXISTS
              (
                  SELECT 1
                  FROM dbo.Vip_Tiers AS tier
                  WHERE tier.IconID = activeIcon.IconID
              )
          )
    )
    BEGIN
        EXEC dbo.RightIcon_Deactivate @CharName16;
    END;

    EXEC dbo.Vip_ApplyConfiguredBuff
        @CharID = @CharID,
        @RankCode = @NewRankCode,
        @ForceRefresh = 0;
END;
GO

/*
    The original Item Mall is purchased by GameServer, not by the custom
    CLIENT_NEW_ITEM_MALL_BUY_REQUEST handler. GameServer records successful
    purchases in SK_ItemSaleLog / SK_PackageItemSaleLog.

    The account triggers only append a durable event. The filter drains these
    events afterwards and runs Hook_ItemMallBuy exactly once.
*/
IF OBJECT_ID(N'dbo.Vip_ItemMallPurchaseEvents', N'U') IS NULL
BEGIN
    CREATE TABLE dbo.Vip_ItemMallPurchaseEvents
    (
        EventID BIGINT IDENTITY(1, 1) NOT NULL,
        SourceType VARCHAR(16) NOT NULL,
        SourceLogID INT NOT NULL,
        JID INT NOT NULL,
        CharID INT NOT NULL,
        CharName16 VARCHAR(16) NULL,
        ItemID INT NOT NULL,
        SilkOwn INT NOT NULL,
        SilkGift INT NOT NULL,
        SilkPoint INT NOT NULL,
        PurchasedAt DATETIME2(0) NOT NULL,
        Status TINYINT NOT NULL
            CONSTRAINT DF_VipItemMallEvents_Status DEFAULT (0),
        AttemptCount INT NOT NULL
            CONSTRAINT DF_VipItemMallEvents_AttemptCount DEFAULT (0),
        NextAttemptAt DATETIME2(0) NOT NULL
            CONSTRAINT DF_VipItemMallEvents_NextAttemptAt DEFAULT (SYSUTCDATETIME()),
        ProcessedAt DATETIME2(0) NULL,
        LastError NVARCHAR(1000) NULL,
        CreatedAt DATETIME2(0) NOT NULL
            CONSTRAINT DF_VipItemMallEvents_CreatedAt DEFAULT (SYSUTCDATETIME()),
        CONSTRAINT PK_VipItemMallPurchaseEvents
            PRIMARY KEY CLUSTERED (EventID),
        CONSTRAINT UQ_VipItemMallPurchaseEvents_Source
            UNIQUE (SourceType, SourceLogID),
        CONSTRAINT CK_VipItemMallPurchaseEvents_SourceType
            CHECK (SourceType IN ('ITEM', 'PACKAGE')),
        CONSTRAINT CK_VipItemMallPurchaseEvents_Status
            CHECK (Status IN (0, 1, 2)),
        CONSTRAINT CK_VipItemMallPurchaseEvents_Silk
            CHECK (SilkOwn >= 0 AND SilkGift >= 0 AND SilkPoint >= 0)
    );

    CREATE INDEX IX_VipItemMallPurchaseEvents_Pending
        ON dbo.Vip_ItemMallPurchaseEvents (Status, NextAttemptAt, EventID);
END;
GO

CREATE OR ALTER PROCEDURE dbo.Vip_ProcessPendingItemMallPurchases
    @BatchSize INT = 50
AS
BEGIN
    SET NOCOUNT ON;
    SET XACT_ABORT OFF;

    SET @BatchSize =
        CASE WHEN @BatchSize < 1 THEN 1
             WHEN @BatchSize > 500 THEN 500
             ELSE @BatchSize
        END;

    DECLARE @Processed INT = 0;
    DECLARE @EventID BIGINT;
    DECLARE @JID INT;
    DECLARE @CharID INT;
    DECLARE @CharName16 VARCHAR(16);
    DECLARE @ItemID INT;
    DECLARE @SilkOwn INT;

    WHILE @Processed < @BatchSize
    BEGIN
        SET @EventID = NULL;

        BEGIN TRANSACTION;
        BEGIN TRY
            SELECT TOP (1)
                @EventID = purchase.EventID,
                @JID = purchase.JID,
                @CharID = purchase.CharID,
                @CharName16 = purchase.CharName16,
                @ItemID = purchase.ItemID,
                @SilkOwn = purchase.SilkOwn
            FROM dbo.Vip_ItemMallPurchaseEvents AS purchase
                WITH (UPDLOCK, READPAST, ROWLOCK)
            WHERE purchase.Status = 0
              AND purchase.NextAttemptAt <= SYSUTCDATETIME()
            ORDER BY purchase.EventID;

            IF @EventID IS NULL
            BEGIN
                COMMIT TRANSACTION;
                BREAK;
            END;

            IF NULLIF(@CharName16, '') IS NULL
                THROW 51001, 'VIP purchase character name could not be resolved.', 1;

            EXEC dbo.Hook_ItemMallBuy
                @JID = @JID,
                @CharID = @CharID,
                @CharName16 = @CharName16,
                @ItemID = @ItemID,
                @Silk = @SilkOwn;

            UPDATE dbo.Vip_ItemMallPurchaseEvents
            SET Status = 1,
                AttemptCount = AttemptCount + 1,
                ProcessedAt = SYSUTCDATETIME(),
                LastError = NULL
            WHERE EventID = @EventID;

            COMMIT TRANSACTION;
            SET @Processed += 1;
        END TRY
        BEGIN CATCH
            IF XACT_STATE() <> 0
                ROLLBACK TRANSACTION;

            IF @EventID IS NOT NULL
            BEGIN
                UPDATE dbo.Vip_ItemMallPurchaseEvents
                SET Status = CASE WHEN AttemptCount + 1 >= 10 THEN 2 ELSE 0 END,
                    AttemptCount = AttemptCount + 1,
                    NextAttemptAt = DATEADD(SECOND, 15, SYSUTCDATETIME()),
                    LastError = LEFT(ERROR_MESSAGE(), 1000)
                WHERE EventID = @EventID;
            END;

            SET @Processed += 1;
        END CATCH;
    END;
END;
GO

DECLARE @VipAccountDB SYSNAME = COALESCE
(
    NULLIF
    (
        (
            SELECT TOP (1) CONVERT(NVARCHAR(128), Value)
            FROM dbo.System_Settings WITH (NOLOCK)
            WHERE SettingName = N'AccountDB'
        ),
        N''
    ),
    N'SRO_VT_ACCOUNT'
);
DECLARE @VipShardDB SYSNAME = COALESCE
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
DECLARE @VipGuardDB SYSNAME = DB_NAME();

IF DB_ID(@VipAccountDB) IS NULL
    THROW 51002, 'VIP bridge could not find the configured AccountDB.', 1;

IF DB_ID(@VipShardDB) IS NULL
    THROW 51003, 'VIP bridge could not find the configured ShardDB.', 1;

DECLARE @VipItemTrigger NVARCHAR(MAX) =
    N'CREATE OR ALTER TRIGGER dbo.trg_KMTGuard_Vip_ItemSaleLog
      ON dbo.SK_ItemSaleLog
      AFTER INSERT
      AS
      BEGIN
          SET NOCOUNT ON;

          INSERT ' + QUOTENAME(@VipGuardDB) + N'.dbo.Vip_ItemMallPurchaseEvents
          (
              SourceType, SourceLogID, JID, CharID, CharName16, ItemID,
              SilkOwn, SilkGift, SilkPoint, PurchasedAt
          )
          SELECT
              ''ITEM'', sale.ID, sale.JID, sale.CharID, character.CharName16,
              sale.ItemID, sale.Silk_Own, sale.Silk_Gift, sale.Silk_Point,
              CONVERT(DATETIME2(0), sale.RegDate)
          FROM inserted AS sale
          LEFT JOIN ' + QUOTENAME(@VipShardDB) + N'.dbo._Char AS character
              ON character.CharID = sale.CharID
          WHERE sale.Silk_Own > 0
            AND NOT EXISTS
            (
                SELECT 1
                FROM ' + QUOTENAME(@VipGuardDB) + N'.dbo.Vip_ItemMallPurchaseEvents AS existing
                WHERE existing.SourceType = ''ITEM''
                  AND existing.SourceLogID = sale.ID
            );
      END;';

DECLARE @VipPackageTrigger NVARCHAR(MAX) =
    N'CREATE OR ALTER TRIGGER dbo.trg_KMTGuard_Vip_PackageItemSaleLog
      ON dbo.SK_PackageItemSaleLog
      AFTER INSERT
      AS
      BEGIN
          SET NOCOUNT ON;

          INSERT ' + QUOTENAME(@VipGuardDB) + N'.dbo.Vip_ItemMallPurchaseEvents
          (
              SourceType, SourceLogID, JID, CharID, CharName16, ItemID,
              SilkOwn, SilkGift, SilkPoint, PurchasedAt
          )
          SELECT
              ''PACKAGE'', sale.ID, sale.JID, sale.CharID, character.CharName16,
              sale.PackageItemID, sale.Silk_Own, sale.Silk_Gift,
              sale.Silk_Point, CONVERT(DATETIME2(0), sale.RegDate)
          FROM inserted AS sale
          LEFT JOIN ' + QUOTENAME(@VipShardDB) + N'.dbo._Char AS character
              ON character.CharID = sale.CharID
          WHERE sale.Silk_Own > 0
            AND NOT EXISTS
            (
                SELECT 1
                FROM ' + QUOTENAME(@VipGuardDB) + N'.dbo.Vip_ItemMallPurchaseEvents AS existing
                WHERE existing.SourceType = ''PACKAGE''
                  AND existing.SourceLogID = sale.ID
            );
      END;';

DECLARE @VipInstallItemTrigger NVARCHAR(MAX) =
    N'USE ' + QUOTENAME(@VipAccountDB) + N';
      EXEC sys.sp_executesql N''' +
      REPLACE(@VipItemTrigger, N'''', N'''''') + N''';';
DECLARE @VipInstallPackageTrigger NVARCHAR(MAX) =
    N'USE ' + QUOTENAME(@VipAccountDB) + N';
      EXEC sys.sp_executesql N''' +
      REPLACE(@VipPackageTrigger, N'''', N'''''') + N''';';

EXEC sys.sp_executesql @VipInstallItemTrigger;
EXEC sys.sp_executesql @VipInstallPackageTrigger;
GO

/*
    Global Silk-spend tracking.

    Item Mall mode is deliberately irrelevant here. Every committed decrease
    in silk_own or silk_gift is queued, regardless of whether it originated
    from the stock Item Mall, an old/new custom mall, an NPC, Silk Stall, or
    another feature. Balance increases and silk_point changes are ignored.
*/
IF OBJECT_ID(N'dbo.Vip_SilkSpendEvents', N'U') IS NULL
BEGIN
    CREATE TABLE dbo.Vip_SilkSpendEvents
    (
        EventID BIGINT IDENTITY(1, 1) NOT NULL,
        JID INT NOT NULL,
        SilkSpent INT NOT NULL,
        SilkOwnBefore INT NOT NULL,
        SilkOwnAfter INT NOT NULL,
        SilkGiftBefore INT NOT NULL,
        SilkGiftAfter INT NOT NULL,
        CharID INT NULL,
        CharName16 VARCHAR(16) NULL,
        Status TINYINT NOT NULL
            CONSTRAINT DF_VipSilkSpendEvents_Status DEFAULT (0),
        AttemptCount INT NOT NULL
            CONSTRAINT DF_VipSilkSpendEvents_AttemptCount DEFAULT (0),
        NextAttemptAt DATETIME2(0) NOT NULL
            CONSTRAINT DF_VipSilkSpendEvents_NextAttemptAt
                DEFAULT (DATEADD(SECOND, -1, SYSUTCDATETIME())),
        ProcessedAt DATETIME2(0) NULL,
        LastError NVARCHAR(1000) NULL,
        CreatedAt DATETIME2(0) NOT NULL
            CONSTRAINT DF_VipSilkSpendEvents_CreatedAt DEFAULT (SYSUTCDATETIME()),
        CONSTRAINT PK_VipSilkSpendEvents PRIMARY KEY CLUSTERED (EventID),
        CONSTRAINT CK_VipSilkSpendEvents_SilkSpent CHECK (SilkSpent > 0),
        CONSTRAINT CK_VipSilkSpendEvents_Status CHECK (Status IN (0, 1, 2))
    );

    CREATE INDEX IX_VipSilkSpendEvents_Pending
        ON dbo.Vip_SilkSpendEvents (Status, NextAttemptAt, EventID);
END;
GO

DECLARE @SpendAccountDB SYSNAME = COALESCE
(
    NULLIF
    (
        (
            SELECT TOP (1) CONVERT(NVARCHAR(128), Value)
            FROM dbo.System_Settings WITH (NOLOCK)
            WHERE SettingName = N'AccountDB'
        ),
        N''
    ),
    N'SRO_VT_ACCOUNT'
);
DECLARE @SpendGuardDB SYSNAME = DB_NAME();

IF DB_ID(@SpendAccountDB) IS NULL
    THROW 51004, 'VIP Silk spend tracker could not find AccountDB.', 1;

DECLARE @SpendTrigger NVARCHAR(MAX) =
    N'CREATE OR ALTER TRIGGER dbo.trg_KMTGuard_Vip_SilkSpend
      ON dbo.SK_Silk
      AFTER UPDATE
      AS
      BEGIN
          SET NOCOUNT ON;

          INSERT ' + QUOTENAME(@SpendGuardDB) + N'.dbo.Vip_SilkSpendEvents
          (
              JID, SilkSpent,
              SilkOwnBefore, SilkOwnAfter,
              SilkGiftBefore, SilkGiftAfter
          )
          SELECT
              currentBalance.JID,
              CASE
                  WHEN oldBalance.silk_own > currentBalance.silk_own
                      THEN oldBalance.silk_own - currentBalance.silk_own
                  ELSE 0
              END
              +
              CASE
                  WHEN oldBalance.silk_gift > currentBalance.silk_gift
                      THEN oldBalance.silk_gift - currentBalance.silk_gift
                  ELSE 0
              END,
              oldBalance.silk_own,
              currentBalance.silk_own,
              oldBalance.silk_gift,
              currentBalance.silk_gift
          FROM inserted AS currentBalance
          INNER JOIN deleted AS oldBalance
              ON oldBalance.JID = currentBalance.JID
          WHERE oldBalance.silk_own > currentBalance.silk_own
             OR oldBalance.silk_gift > currentBalance.silk_gift;
      END;';

DECLARE @InstallSpendTrigger NVARCHAR(MAX) =
    N'USE ' + QUOTENAME(@SpendAccountDB) + N';
      IF OBJECT_ID(N''dbo.trg_KMTGuard_Vip_ItemSaleLog'', N''TR'') IS NOT NULL
          DROP TRIGGER dbo.trg_KMTGuard_Vip_ItemSaleLog;
      IF OBJECT_ID(N''dbo.trg_KMTGuard_Vip_PackageItemSaleLog'', N''TR'') IS NOT NULL
          DROP TRIGGER dbo.trg_KMTGuard_Vip_PackageItemSaleLog;
      EXEC sys.sp_executesql N''' +
      REPLACE(@SpendTrigger, N'''', N'''''') + N''';';

EXEC sys.sp_executesql @InstallSpendTrigger;
GO

/*
    Replace only the old hard-coded VIP login block and preserve every other
    Log_Character customization in the customer database.
*/
DECLARE @LogDefinition NVARCHAR(MAX) =
    OBJECT_DEFINITION(OBJECT_ID(N'dbo.Log_Character'));

IF @LogDefinition IS NOT NULL
   AND CHARINDEX(N'Vip_OnCharacterLogin', @LogDefinition) = 0
BEGIN
    DECLARE @VipBlockStart INT =
        CHARINDEX(N'IF(@EventID = 9)', @LogDefinition);
    DECLARE @NextBlockStart INT =
        CHARINDEX(N'if(@EventID = 20', @LogDefinition, @VipBlockStart + 1);

    IF @VipBlockStart = 0 OR @NextBlockStart = 0
        THROW 51000, 'Could not locate the legacy VIP block in dbo.Log_Character.', 1;

    SET @LogDefinition =
        STUFF
        (
            @LogDefinition,
            @VipBlockStart,
            @NextBlockStart - @VipBlockStart,
            N'IF(@EventID = 9)
BEGIN
    EXEC dbo.Vip_OnCharacterLogin @CharID;
END

'
        );

    DECLARE @CreateKeyword INT = CHARINDEX(N'CREATE', @LogDefinition);
    IF @CreateKeyword > 0
        SET @LogDefinition = STUFF
        (
            @LogDefinition,
            @CreateKeyword,
            LEN(N'CREATE'),
            N'ALTER'
        );

    EXEC sys.sp_executesql @LogDefinition;
END;
GO

EXEC dbo.Vip_RecalculateAll;

PRINT 'Configurable VIP tier migration completed.';
