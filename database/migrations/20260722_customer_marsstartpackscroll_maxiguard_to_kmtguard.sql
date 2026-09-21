USE [SRO_VT_LOG]
GO

SET ANSI_NULLS ON
GO
SET QUOTED_IDENTIFIER ON
GO

ALTER PROCEDURE [dbo].[_MarsStartPackScroll]
    @CharID int,
    @ItemRefID int,
    @Operation tinyint
AS
BEGIN
    SET NOCOUNT ON;

    DECLARE @CharRefObjID int;

    SELECT @CharRefObjID = RefObjID
    FROM [SRO_VT_SHARD].[dbo].[_Char]
    WHERE CharID = @CharID;

    DECLARE @Rewards table
    (
        CodeName128 varchar(128) NOT NULL,
        Quantity int NOT NULL
    );

    IF (@CharRefObjID BETWEEN 1907 AND 1919 OR @CharRefObjID BETWEEN 14875 AND 14887)
    BEGIN
        INSERT INTO @Rewards (CodeName128, Quantity)
        VALUES
            ('ITEM_MALL_PREMIUM_VIETNAM_GOLDTIME_PLUS', 1),
            ('ITEM_MALL_AVATAR_M_NASRUN', 1),
            ('ITEM_MARS_28GUN_BLESS', 1);
    END;

    IF (@CharRefObjID BETWEEN 1920 AND 1932 OR @CharRefObjID BETWEEN 14888 AND 14900)
    BEGIN
        INSERT INTO @Rewards (CodeName128, Quantity)
        VALUES
            ('ITEM_MALL_PREMIUM_VIETNAM_GOLDTIME_PLUS', 1),
            ('ITEM_MALL_AVATAR_W_NASRUN', 1),
            ('ITEM_MARS_28GUN_BLESS', 1);
    END;

    DECLARE @ItemRefObjID int;
    DECLARE @Quantity int;

    DECLARE reward_cursor CURSOR LOCAL FAST_FORWARD FOR
        SELECT R.ID, Rewards.Quantity
        FROM @Rewards Rewards
        INNER JOIN [SRO_VT_SHARD].[dbo].[_RefObjCommon] R
            ON R.CodeName128 = Rewards.CodeName128
        WHERE R.Service = 1;

    OPEN reward_cursor;
    FETCH NEXT FROM reward_cursor INTO @ItemRefObjID, @Quantity;

    WHILE @@FETCH_STATUS = 0
    BEGIN
        EXEC [KMTGuard].[dbo].[Item_AddChest]
            @CharID = @CharID,
            @ItemRefObjID = @ItemRefObjID,
            @Quantity = @Quantity,
            @From = 'StarterPack',
            @Plus = 0;

        FETCH NEXT FROM reward_cursor INTO @ItemRefObjID, @Quantity;
    END;

    CLOSE reward_cursor;
    DEALLOCATE reward_cursor;
END
GO
