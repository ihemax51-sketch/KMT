/*
    Repair and harden the VIP Silk-spend pipeline.

    Run on the KMTGuard database after the configurable VIP and player-style
    migrations. This update:
      - removes the stale dependency on the retired Style_UpdateRightIcon API;
      - installs or repairs the global AccountDB Silk-spend trigger;
      - makes Rank_Silk one-row-per-character and records the character name;
      - adds an idempotent, durable VIP spend/rank history;
      - requeues failed spend events after the runtime dependency is repaired.
*/

SET ANSI_NULLS ON;
SET QUOTED_IDENTIFIER ON;
SET ANSI_PADDING ON;
SET ANSI_WARNINGS ON;
SET ARITHABORT ON;
SET CONCAT_NULL_YIELDS_NULL ON;
SET NUMERIC_ROUNDABORT OFF;
GO

SET NOCOUNT ON;
SET XACT_ABORT ON;

IF OBJECT_ID(N'dbo.Rank_Silk', N'U') IS NULL
    THROW 51040, 'dbo.Rank_Silk is missing. Install the VIP base migration first.', 1;

IF OBJECT_ID(N'dbo.Vip_Tiers', N'U') IS NULL
    THROW 51041, 'dbo.Vip_Tiers is missing. Install the VIP base migration first.', 1;

IF OBJECT_ID(N'dbo.Vip_SilkSpendEvents', N'U') IS NULL
    THROW 51042, 'dbo.Vip_SilkSpendEvents is missing. Install the VIP base migration first.', 1;

IF OBJECT_ID(N'dbo.RightIcon_Add', N'P') IS NULL
   OR OBJECT_ID(N'dbo.RightIcon_Activate', N'P') IS NULL
   OR OBJECT_ID(N'dbo.RightIcon_Deactivate', N'P') IS NULL
    THROW 51043, 'The current right-icon procedures are missing. Install the player-style migration first.', 1;

/*
    Do not assume the base VIP package left its cross-database trigger in
    place. A disabled or missing trigger means no Silk-spend event can enter
    the queue even though all KMTGuard-side VIP objects are healthy.
*/
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
    THROW 51044, 'VIP Silk spend tracker could not find the configured AccountDB.', 1;

IF OBJECT_ID(QUOTENAME(@SpendAccountDB) + N'.dbo.SK_Silk', N'U') IS NULL
    THROW 51045, 'VIP Silk spend tracker could not find AccountDB.dbo.SK_Silk.', 1;

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
      REPLACE(@SpendTrigger, N'''', N'''''') + N''';
      ENABLE TRIGGER dbo.trg_KMTGuard_Vip_SilkSpend ON dbo.SK_Silk;';

EXEC sys.sp_executesql @InstallSpendTrigger;
GO

IF COL_LENGTH(N'dbo.Rank_Silk', N'CharName16') IS NULL
    ALTER TABLE dbo.Rank_Silk ADD CharName16 VARCHAR(16) NULL;

IF COL_LENGTH(N'dbo.Rank_Silk', N'LastSilkSpent') IS NULL
    ALTER TABLE dbo.Rank_Silk ADD LastSilkSpent INT NULL;

IF COL_LENGTH(N'dbo.Rank_Silk', N'LastSpendAt') IS NULL
    ALTER TABLE dbo.Rank_Silk ADD LastSpendAt DATETIME2(0) NULL;

IF COL_LENGTH(N'dbo.Rank_Silk', N'RankUpdatedAt') IS NULL
    ALTER TABLE dbo.Rank_Silk ADD RankUpdatedAt DATETIME2(0) NULL;
GO

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
    DECLARE @BackfillNamesSql NVARCHAR(MAX) =
        N'UPDATE rankRow
          SET CharName16 = characterRow.CharName16
          FROM dbo.Rank_Silk AS rankRow
          INNER JOIN ' + QUOTENAME(@ShardDB) + N'.dbo._Char AS characterRow WITH (NOLOCK)
              ON characterRow.CharID = rankRow.CharID
          WHERE NULLIF(rankRow.CharName16, '''') IS NULL
             OR rankRow.CharName16 COLLATE DATABASE_DEFAULT
                <> characterRow.CharName16 COLLATE DATABASE_DEFAULT;';
    EXEC sys.sp_executesql @BackfillNamesSql;
END;

/*
    Legacy Rank_Silk had only an identity primary key. Consolidate accidental
    duplicates before enforcing the actual one-row-per-character contract.
    SilkHistory is cumulative, so the highest row is the safe survivor value.
*/
;WITH AggregateRows AS
(
    SELECT
        CharID,
        MIN(ID) AS KeepID,
        MAX(SilkHistory) AS SilkHistory,
        MAX(JID) AS JID,
        MAX(NULLIF(CharName16, '')) AS CharName16,
        MAX(LastSilkSpent) AS LastSilkSpent,
        MAX(LastSpendAt) AS LastSpendAt,
        MAX(RankUpdatedAt) AS RankUpdatedAt
    FROM dbo.Rank_Silk
    GROUP BY CharID
    HAVING COUNT(*) > 1
)
UPDATE keepRow
SET JID = aggregateRow.JID,
    SilkHistory = aggregateRow.SilkHistory,
    CharName16 = COALESCE(aggregateRow.CharName16, keepRow.CharName16),
    LastSilkSpent = aggregateRow.LastSilkSpent,
    LastSpendAt = aggregateRow.LastSpendAt,
    RankUpdatedAt = aggregateRow.RankUpdatedAt
FROM dbo.Rank_Silk AS keepRow
INNER JOIN AggregateRows AS aggregateRow ON aggregateRow.KeepID = keepRow.ID;

;WITH DuplicateRows AS
(
    SELECT
        ID,
        ROW_NUMBER() OVER (PARTITION BY CharID ORDER BY ID) AS DuplicateNumber
    FROM dbo.Rank_Silk
)
DELETE rankRow
FROM dbo.Rank_Silk AS rankRow
INNER JOIN DuplicateRows AS duplicateRow ON duplicateRow.ID = rankRow.ID
WHERE duplicateRow.DuplicateNumber > 1;

IF NOT EXISTS
(
    SELECT 1
    FROM sys.indexes
    WHERE object_id = OBJECT_ID(N'dbo.Rank_Silk')
      AND name = N'UX_RankSilk_CharID'
)
    CREATE UNIQUE INDEX UX_RankSilk_CharID ON dbo.Rank_Silk (CharID);

IF OBJECT_ID(N'dbo.Vip_RankHistory', N'U') IS NULL
BEGIN
    CREATE TABLE dbo.Vip_RankHistory
    (
        HistoryID BIGINT IDENTITY(1, 1) NOT NULL,
        SourceEventID BIGINT NULL,
        JID INT NOT NULL,
        CharID INT NOT NULL,
        CharName16 VARCHAR(16) NULL,
        SilkSpent INT NOT NULL,
        TotalSilkSpent INT NOT NULL,
        PreviousRankCode INT NOT NULL,
        NewRankCode INT NOT NULL,
        RankChanged BIT NOT NULL,
        ProcessedAt DATETIME2(0) NOT NULL
            CONSTRAINT DF_VipRankHistory_ProcessedAt DEFAULT (SYSUTCDATETIME()),
        CONSTRAINT PK_VipRankHistory PRIMARY KEY CLUSTERED (HistoryID),
        CONSTRAINT CK_VipRankHistory_SilkSpent CHECK (SilkSpent > 0),
        CONSTRAINT CK_VipRankHistory_TotalSilkSpent CHECK (TotalSilkSpent >= SilkSpent),
        CONSTRAINT CK_VipRankHistory_PreviousRank CHECK (PreviousRankCode BETWEEN 0 AND 6),
        CONSTRAINT CK_VipRankHistory_NewRank CHECK (NewRankCode BETWEEN 0 AND 6)
    );

END;

IF NOT EXISTS
(
    SELECT 1 FROM sys.indexes
    WHERE object_id = OBJECT_ID(N'dbo.Vip_RankHistory')
      AND name = N'UX_VipRankHistory_SourceEvent'
)
    CREATE UNIQUE INDEX UX_VipRankHistory_SourceEvent
        ON dbo.Vip_RankHistory (SourceEventID)
        WHERE SourceEventID IS NOT NULL;

IF NOT EXISTS
(
    SELECT 1 FROM sys.indexes
    WHERE object_id = OBJECT_ID(N'dbo.Vip_RankHistory')
      AND name = N'IX_VipRankHistory_Character'
)
    CREATE INDEX IX_VipRankHistory_Character
        ON dbo.Vip_RankHistory (CharID, ProcessedAt DESC, HistoryID DESC);
GO

CREATE OR ALTER PROCEDURE dbo.Hook_ItemMallBuy
    @JID INT,
    @CharID INT,
    @CharName16 VARCHAR(16),
    @ItemID INT,
    @Silk INT,
    @SourceEventID BIGINT = NULL
AS
BEGIN
    SET NOCOUNT ON;
    SET XACT_ABORT ON;

    IF @Silk <= 0
        RETURN;

    IF @ItemID BETWEEN 45837 AND 45844
        RETURN;

    /* A completed durable event must be safe to submit again after a restart. */
    IF @SourceEventID IS NOT NULL
       AND EXISTS
       (
           SELECT 1
           FROM dbo.Vip_RankHistory WITH (UPDLOCK, HOLDLOCK)
           WHERE SourceEventID = @SourceEventID
       )
        RETURN;

    DECLARE @StartedTransaction BIT = 0;
    IF @@TRANCOUNT = 0
    BEGIN
        BEGIN TRANSACTION;
        SET @StartedTransaction = 1;
    END;

    BEGIN TRY
        EXEC dbo.Achievement_Update @CharID, 9, 9, @Silk;

        DECLARE @OldRankCode INT = 0;
        DECLARE @NewRankCode INT = 0;
        DECLARE @NewIconID INT;
        DECLARE @OldSilkHistory INT = 0;
        DECLARE @NewSilkHistory INT;
        DECLARE @Now DATETIME2(0) = SYSUTCDATETIME();

        SELECT
            @OldRankCode = SilkRank,
            @OldSilkHistory = SilkHistory
        FROM dbo.Rank_Silk WITH (UPDLOCK, HOLDLOCK)
        WHERE CharID = @CharID;

        IF @@ROWCOUNT = 0
        BEGIN
            SET @NewSilkHistory = @Silk;
            INSERT dbo.Rank_Silk
                (JID, CharID, SilkHistory, SilkRank, CharName16,
                 LastSilkSpent, LastSpendAt, RankUpdatedAt)
            VALUES
                (@JID, @CharID, @NewSilkHistory, 0, NULLIF(@CharName16, ''),
                 @Silk, @Now, @Now);
        END
        ELSE
        BEGIN
            SET @NewSilkHistory =
                CASE
                    WHEN @OldSilkHistory > 2147483647 - @Silk THEN 2147483647
                    ELSE @OldSilkHistory + @Silk
                END;

            UPDATE dbo.Rank_Silk
            SET SilkHistory = @NewSilkHistory,
                JID = @JID,
                CharName16 = COALESCE(NULLIF(@CharName16, ''), CharName16),
                LastSilkSpent = @Silk,
                LastSpendAt = @Now
            WHERE CharID = @CharID;
        END;

        SELECT TOP (1)
            @NewRankCode = tier.RankCode,
            @NewIconID = tier.IconID
        FROM dbo.Vip_Tiers AS tier WITH (NOLOCK)
        WHERE tier.MinSilk <= @NewSilkHistory
        ORDER BY tier.MinSilk DESC, tier.RankCode;

        SET @NewRankCode = ISNULL(@NewRankCode, 0);

        UPDATE dbo.Rank_Silk
        SET SilkRank = @NewRankCode,
            RankUpdatedAt = CASE
                WHEN SilkRank <> @NewRankCode OR RankUpdatedAt IS NULL THEN @Now
                ELSE RankUpdatedAt
            END
        WHERE CharID = @CharID;

        INSERT dbo.Vip_RankHistory
        (
            SourceEventID, JID, CharID, CharName16, SilkSpent,
            TotalSilkSpent, PreviousRankCode, NewRankCode, RankChanged,
            ProcessedAt
        )
        VALUES
        (
            @SourceEventID, @JID, @CharID, NULLIF(@CharName16, ''), @Silk,
            @NewSilkHistory, @OldRankCode, @NewRankCode,
            CASE WHEN @OldRankCode <> @NewRankCode THEN 1 ELSE 0 END,
            @Now
        );

        INSERT dbo.Command_FilterQueue
            (CommandID, Data1, Data2, Data3, Data4, Status)
        VALUES
            (27, @CharID, @JID, @Silk, @NewRankCode, 1);

        IF @NewRankCode > 0
        BEGIN
            EXEC dbo.RightIcon_Add
                @CharID = @CharID,
                @IconID = @NewIconID;

            EXEC dbo.RightIcon_Activate
                @CharID = @CharID,
                @CharName16 = @CharName16,
                @IconID = @NewIconID;
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
            EXEC dbo.RightIcon_Deactivate @CharName16 = @CharName16;
        END;

        EXEC dbo.Vip_ApplyConfiguredBuff
            @CharID = @CharID,
            @RankCode = @NewRankCode,
            @ForceRefresh = 0;

        IF @StartedTransaction = 1
            COMMIT TRANSACTION;
    END TRY
    BEGIN CATCH
        IF @StartedTransaction = 1 AND XACT_STATE() <> 0
            ROLLBACK TRANSACTION;
        THROW;
    END CATCH;
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

    INSERT #VipState (CharID, CharName16, RankCode, IconID)
    SELECT
        rankRow.CharID,
        rankRow.CharName16,
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
        DECLARE @NameSql NVARCHAR(MAX) =
            N'UPDATE state
              SET CharName16 = characterRow.CharName16
              FROM #VipState AS state
              INNER JOIN ' + QUOTENAME(@ShardDB) + N'.dbo._Char AS characterRow WITH (NOLOCK)
                  ON characterRow.CharID = state.CharID;';
        EXEC sys.sp_executesql @NameSql;
    END;

    DECLARE @Now DATETIME2(0) = SYSUTCDATETIME();

    UPDATE rankRow
    SET SilkRank = state.RankCode,
        CharName16 = COALESCE(state.CharName16, rankRow.CharName16),
        RankUpdatedAt = CASE
            WHEN rankRow.SilkRank <> state.RankCode OR rankRow.RankUpdatedAt IS NULL
                THEN @Now
            ELSE rankRow.RankUpdatedAt
        END
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

/* Current, readable player ordering backed by the authoritative Rank_Silk row. */
CREATE OR ALTER VIEW dbo.Vip_CurrentRankings
AS
    SELECT
        CONVERT(INT, ROW_NUMBER() OVER
        (
            ORDER BY rankRow.SilkHistory DESC, rankRow.CharID
        )) AS Position,
        rankRow.JID,
        rankRow.CharID,
        rankRow.CharName16,
        rankRow.SilkHistory AS TotalSilkSpent,
        rankRow.SilkRank AS RankCode,
        tier.DisplayName AS RankName,
        tier.IconID,
        rankRow.LastSilkSpent,
        rankRow.LastSpendAt,
        rankRow.RankUpdatedAt
    FROM dbo.Rank_Silk AS rankRow
    LEFT JOIN dbo.Vip_Tiers AS tier
        ON tier.RankCode = rankRow.SilkRank;
GO

/*
    Failed events retain their AttemptCount for diagnosis, but are eligible
    immediately now that the broken runtime procedure has been replaced.
*/
UPDATE dbo.Vip_SilkSpendEvents
SET Status = 0,
    NextAttemptAt = DATEADD(SECOND, -1, SYSUTCDATETIME())
WHERE Status = 2;

UPDATE dbo.Rank_Silk
SET RankUpdatedAt = COALESCE(RankUpdatedAt, SYSUTCDATETIME());

EXEC dbo.Vip_RecalculateAll;

PRINT 'VIP runtime recovery and history migration completed.';
