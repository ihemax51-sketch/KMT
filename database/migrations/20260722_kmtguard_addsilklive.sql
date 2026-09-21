USE [KMTGuard]
GO

SET ANSI_NULLS ON
GO
SET QUOTED_IDENTIFIER OFF
GO

CREATE OR ALTER PROCEDURE [dbo].[_addsilklive]
    @CharID INT,
    @Amount INT = 0,
    @GiftAmount INT = 0,
    @PointAmount INT = 0
AS
BEGIN
    SET NOCOUNT ON;

    DECLARE @JID INT;
    DECLARE @Remain INT = 0;
    DECLARE @GiftRemain INT = 0;
    DECLARE @PointRemain INT = 0;

    SELECT TOP 1 @JID = UserJID
    FROM [SRO_VT_SHARD].[dbo].[_User]
    WHERE CharID = @CharID;

    IF (@JID IS NULL)
    BEGIN
        PRINT 'Invalid CharID: Could not find associated JID.';
        RETURN;
    END;

    IF (NOT EXISTS (SELECT 1 FROM [SRO_VT_ACCOUNT].[dbo].[SK_Silk] WITH (NOLOCK) WHERE [JID] = @JID))
    BEGIN
        INSERT INTO [SRO_VT_ACCOUNT].[dbo].[SK_Silk] ([JID], [silk_own], [silk_gift], [silk_point])
        VALUES (@JID, @Amount, @GiftAmount, @PointAmount);
    END
    ELSE
    BEGIN
        SET @Remain = [SRO_VT_ACCOUNT].[CGI].[getSilkOwn](@JID);
        SET @GiftRemain = [SRO_VT_ACCOUNT].[CGI].[getSilkGift](@JID);
        SET @PointRemain = [SRO_VT_ACCOUNT].[CGI].[getSilkPoint](@JID);

        IF ((@Remain + @Amount) < 0)
            SET @Amount = -@Remain;

        IF ((@GiftRemain + @GiftAmount) < 0)
            SET @GiftAmount = -@GiftRemain;

        IF ((@PointRemain + @PointAmount) < 0)
            SET @PointAmount = -@PointRemain;

        UPDATE [SRO_VT_ACCOUNT].[dbo].[SK_Silk]
        SET [silk_own] += @Amount,
            [silk_gift] += @GiftAmount,
            [silk_point] += @PointAmount
        WHERE [JID] = @JID;
    END;

    IF (@Amount <> 0)
        INSERT INTO [SRO_VT_ACCOUNT].[dbo].[SK_SilkChange_BY_Web] ([JID], [silk_remain], [silk_offset], [silk_type], [reason])
        VALUES (@JID, (@Remain + @Amount), @Amount, 0, 0);

    IF (@GiftAmount <> 0)
        INSERT INTO [SRO_VT_ACCOUNT].[dbo].[SK_SilkChange_BY_Web] ([JID], [silk_remain], [silk_offset], [silk_type], [reason])
        VALUES (@JID, (@GiftRemain + @GiftAmount), @GiftAmount, 1, 0);

    IF (@PointAmount <> 0)
        INSERT INTO [SRO_VT_ACCOUNT].[dbo].[SK_SilkChange_BY_Web] ([JID], [silk_remain], [silk_offset], [silk_type], [reason])
        VALUES (@JID, (@PointRemain + @PointAmount), @PointAmount, 2, 0);
END
GO
