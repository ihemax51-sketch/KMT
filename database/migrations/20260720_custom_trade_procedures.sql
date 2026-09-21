USE [KMTGuard]
GO

SET NOCOUNT ON;
GO

/*
    KMTGuard Customer Trade Procedures

    These procedures are owned by the customer and run only while the managed
    Trade workspace is disabled. Updates never alter an existing customer
    procedure.

    On upgraded installations, the matching legacy TradeGoods procedure is
    cloned and only its name is changed. On clean installations, a documented
    allow-by-default procedure shell is created for customer implementation.
*/

IF OBJECT_ID(N'dbo._CustomTradeGoodsBuying', N'P') IS NULL
BEGIN
    IF OBJECT_ID(N'dbo._OnTradeGoodsBuyingComplete_EDIT', N'P') IS NOT NULL
    BEGIN
        DECLARE @BuyingDefinition nvarchar(max) = OBJECT_DEFINITION(OBJECT_ID(N'dbo._OnTradeGoodsBuyingComplete_EDIT', N'P'));
        SET @BuyingDefinition = REPLACE(@BuyingDefinition,
            N'[dbo].[_OnTradeGoodsBuyingComplete_EDIT]', N'[dbo].[_CustomTradeGoodsBuying]');
        EXEC sys.sp_executesql @BuyingDefinition;
    END
    ELSE
        EXEC(N'CREATE PROCEDURE dbo._CustomTradeGoodsBuying
            @CharID int, @Charname varchar(25), @NpcID int, @Petid int,
            @NpcCodename varchar(128), @NpcTab tinyint, @NpcSlot tinyint, @Quantity smallint
        AS
        BEGIN
            SET NOCOUNT ON;
            /* Runs after a successful trade-goods purchase in Customer Mode. */
        END');
END
GO

IF OBJECT_ID(N'dbo._CustomTradeGoodsSelling', N'P') IS NULL
BEGIN
    IF OBJECT_ID(N'dbo._OnTradeGoodsSellingComplete_EDIT', N'P') IS NOT NULL
    BEGIN
        DECLARE @SellingDefinition nvarchar(max) = OBJECT_DEFINITION(OBJECT_ID(N'dbo._OnTradeGoodsSellingComplete_EDIT', N'P'));
        SET @SellingDefinition = REPLACE(@SellingDefinition,
            N'[dbo].[_OnTradeGoodsSellingComplete_EDIT]', N'[dbo].[_CustomTradeGoodsSelling]');
        EXEC sys.sp_executesql @SellingDefinition;
    END
    ELSE
        EXEC(N'CREATE PROCEDURE dbo._CustomTradeGoodsSelling
            @CharID int, @Charname varchar(25), @Petid int, @NpcID int,
            @NpcCodename varchar(128), @PetSlot tinyint, @Quantity smallint
        AS
        BEGIN
            SET NOCOUNT ON;
            /* Runs after a successful trade-goods sale in Customer Mode. */
        END');
END
GO

IF OBJECT_ID(N'dbo._CustomTradeGoodsSellingRequest', N'P') IS NULL
BEGIN
    IF OBJECT_ID(N'dbo._OnTradeGoodsSellingRequest_EDIT', N'P') IS NOT NULL
    BEGIN
        DECLARE @SellRequestDefinition nvarchar(max) = OBJECT_DEFINITION(OBJECT_ID(N'dbo._OnTradeGoodsSellingRequest_EDIT', N'P'));
        SET @SellRequestDefinition = REPLACE(@SellRequestDefinition,
            N'[dbo].[_OnTradeGoodsSellingRequest_EDIT]', N'[dbo].[_CustomTradeGoodsSellingRequest]');
        EXEC sys.sp_executesql @SellRequestDefinition;
    END
    ELSE
        EXEC(N'CREATE PROCEDURE dbo._CustomTradeGoodsSellingRequest
            @CharID int, @Charname varchar(25), @Petid int, @NpcID int,
            @NpcCodename varchar(128), @PetSlot tinyint, @Quantity smallint,
            @IsBlocked bit OUTPUT
        AS
        BEGIN
            SET NOCOUNT ON;
            SET @IsBlocked = 0;
            /* Set @IsBlocked to 1 to reject this sale in Customer Mode. */
        END');
END
GO

IF OBJECT_ID(N'dbo._CustomTradeGoodsBuyingRequest', N'P') IS NULL
BEGIN
    IF OBJECT_ID(N'dbo._OnTradeGoodsBuyingRequest_EDIT', N'P') IS NOT NULL
    BEGIN
        DECLARE @BuyRequestDefinition nvarchar(max) = OBJECT_DEFINITION(OBJECT_ID(N'dbo._OnTradeGoodsBuyingRequest_EDIT', N'P'));
        SET @BuyRequestDefinition = REPLACE(@BuyRequestDefinition,
            N'[dbo].[_OnTradeGoodsBuyingRequest_EDIT]', N'[dbo].[_CustomTradeGoodsBuyingRequest]');
        EXEC sys.sp_executesql @BuyRequestDefinition;
    END
    ELSE
        EXEC(N'CREATE PROCEDURE dbo._CustomTradeGoodsBuyingRequest
            @CharID int, @Charname varchar(25), @NpcID int,
            @NpcCodename varchar(128), @NpcTab tinyint, @NpcSlot tinyint,
            @Quantity smallint, @IsBlocked bit OUTPUT
        AS
        BEGIN
            SET NOCOUNT ON;
            SET @IsBlocked = 0;
            /* Set @IsBlocked to 1 to reject this purchase in Customer Mode. */
        END');
END
GO

