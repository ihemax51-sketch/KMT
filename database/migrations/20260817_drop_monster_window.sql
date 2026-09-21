USE [KMTGuard];
GO

SET ANSI_NULLS ON;
GO
SET QUOTED_IDENTIFIER ON;
GO

CREATE OR ALTER PROCEDURE [dbo].[DropMonster_Window]
    @MonsterID INT
AS
BEGIN
    SET NOCOUNT ON;

    ;WITH MonsterDrops AS
    (
        SELECT
            M.ID AS MonsterID,
            M.CodeName128 AS MonsterCodeName,
            I.ID AS ItemID,
            I.CodeName128 AS ItemCodeName,
            I.NameStrID128 AS ItemNameStrID,
            CAST(D.DropAmountMin AS INT) AS DropAmountMin,
            CAST(D.DropAmountMax AS INT) AS DropAmountMax,
            CAST(D.DropRatio AS DECIMAL(18, 6)) AS DropRatio
        FROM [SRO_VT_SHARD].[dbo].[_RefMonster_AssignedItemRndDrop] AS D WITH (NOLOCK)
        INNER JOIN [SRO_VT_SHARD].[dbo].[_RefObjCommon] AS M WITH (NOLOCK)
            ON D.RefMonsterID = M.ID
        INNER JOIN [SRO_VT_SHARD].[dbo].[_RefDropItemGroup] AS G WITH (NOLOCK)
            ON D.RefItemGroupID = G.RefItemGroupID
        INNER JOIN [SRO_VT_SHARD].[dbo].[_RefObjCommon] AS I WITH (NOLOCK)
            ON G.RefItemID = I.ID
        WHERE D.RefMonsterID = @MonsterID
          AND M.Service = 1
          AND I.Service = 1

        UNION ALL

        SELECT
            M.ID AS MonsterID,
            M.CodeName128 AS MonsterCodeName,
            I.ID AS ItemID,
            I.CodeName128 AS ItemCodeName,
            I.NameStrID128 AS ItemNameStrID,
            CAST(D.DropAmountMin AS INT) AS DropAmountMin,
            CAST(D.DropAmountMax AS INT) AS DropAmountMax,
            CAST(D.DropRatio AS DECIMAL(18, 6)) AS DropRatio
        FROM [SRO_VT_SHARD].[dbo].[_RefMonster_AssignedItemDrop] AS D WITH (NOLOCK)
        INNER JOIN [SRO_VT_SHARD].[dbo].[_RefObjCommon] AS M WITH (NOLOCK)
            ON D.RefMonsterID = M.ID
        INNER JOIN [SRO_VT_SHARD].[dbo].[_RefObjCommon] AS I WITH (NOLOCK)
            ON D.RefItemID = I.ID
        WHERE D.RefMonsterID = @MonsterID
          AND M.Service = 1
          AND I.Service = 1
    )
    SELECT
        MonsterID,
        MonsterCodeName,
        ItemID,
        ItemCodeName,
        ItemNameStrID,
        DropAmountMin,
        DropAmountMax,
        DropRatio
    FROM MonsterDrops
    ORDER BY DropRatio DESC, ItemCodeName;
END;
GO
