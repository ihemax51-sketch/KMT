/* Run on the KMTGuard database after the v2.7.5 VIP update. */

SET NOCOUNT ON;

IF OBJECT_ID(N'dbo.Vip_CurrentRankings', N'V') IS NULL
    THROW 51140, 'dbo.Vip_CurrentRankings is missing.', 1;

IF OBJECT_ID(N'dbo.Vip_RankHistory', N'U') IS NULL
    THROW 51141, 'dbo.Vip_RankHistory is missing.', 1;

IF OBJECT_DEFINITION(OBJECT_ID(N'dbo.Hook_ItemMallBuy')) LIKE N'%Style_UpdateRightIcon%'
    THROW 51142, 'dbo.Hook_ItemMallBuy still references the retired style procedure.', 1;

IF OBJECT_DEFINITION(OBJECT_ID(N'dbo.Hook_ItemMallBuy')) NOT LIKE N'%RightIcon_Activate%'
    THROW 51143, 'dbo.Hook_ItemMallBuy is not using the current right-icon procedure.', 1;

IF OBJECT_DEFINITION(OBJECT_ID(N'dbo.Vip_RecalculateAll')) LIKE N'%Style[_]ActiveRightIcons%'
    THROW 51147, 'dbo.Vip_RecalculateAll still references the retired style tables.', 1;

IF OBJECT_DEFINITION(OBJECT_ID(N'dbo.Vip_RecalculateAll')) NOT LIKE N'%dbo.ActiveRightIcons%'
    THROW 51148, 'dbo.Vip_RecalculateAll is not using the current right-icon table.', 1;

IF NOT EXISTS
(
    SELECT 1
    FROM sys.indexes
    WHERE object_id = OBJECT_ID(N'dbo.Rank_Silk')
      AND name = N'UX_RankSilk_CharID'
      AND is_unique = 1
)
    THROW 51144, 'Rank_Silk does not enforce one row per character.', 1;

IF EXISTS
(
    SELECT CharID
    FROM dbo.Rank_Silk
    GROUP BY CharID
    HAVING COUNT(*) > 1
)
    THROW 51145, 'Rank_Silk contains duplicate character rows.', 1;

DECLARE @AccountDB SYSNAME = COALESCE
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

DECLARE @TriggerEnabled BIT = 0;
DECLARE @TriggerSql NVARCHAR(MAX) =
    N'USE ' + QUOTENAME(@AccountDB) + N';
      SELECT @Enabled = CASE
          WHEN EXISTS
          (
              SELECT 1
              FROM sys.triggers
              WHERE name = N''trg_KMTGuard_Vip_SilkSpend''
                AND is_disabled = 0
                AND OBJECT_NAME(parent_id) = N''SK_Silk''
          ) THEN 1 ELSE 0 END;';

EXEC sys.sp_executesql
    @TriggerSql,
    N'@Enabled BIT OUTPUT',
    @Enabled = @TriggerEnabled OUTPUT;

IF @TriggerEnabled = 0
    THROW 51146, 'The VIP Silk-spend trigger is missing or disabled on AccountDB.dbo.SK_Silk.', 1;

SELECT
    (SELECT COUNT(*) FROM dbo.Vip_Tiers) AS TierCount,
    (SELECT COUNT(*) FROM dbo.Vip_CurrentRankings) AS RankedPlayerCount,
    (SELECT COUNT(*) FROM dbo.Vip_RankHistory) AS HistoryCount,
    (SELECT COUNT(*) FROM dbo.Vip_SilkSpendEvents WHERE Status = 0) AS PendingEventCount,
    (SELECT COUNT(*) FROM dbo.Vip_SilkSpendEvents WHERE Status = 2) AS RetryingFailureCount;

PRINT 'VIP runtime validation passed.';
