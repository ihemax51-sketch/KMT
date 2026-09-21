USE [SRO_VT_LOG]
GO

SET ANSI_NULLS ON
GO
SET QUOTED_IDENTIFIER ON
GO

ALTER PROCEDURE [dbo].[_MarsSilkScroll]
    @CharID int,
    @ItemRefID int,
    @Operation tinyint
AS
BEGIN
    SET NOCOUNT ON;

    DECLARE @SilkAmount int;
    DECLARE @Notice varchar(max);

    SELECT @SilkAmount =
        CASE @ItemRefID
            WHEN 43939 THEN 5
            WHEN 43940 THEN 10
            WHEN 43941 THEN 25
            WHEN 43942 THEN 50
            WHEN 43943 THEN 100
            WHEN 43944 THEN 250
            WHEN 43945 THEN 500
            WHEN 43946 THEN 1000
            WHEN 43947 THEN 2500
            WHEN 43948 THEN 5000
        END;

    IF (@Operation <> 41 OR @SilkAmount IS NULL)
        RETURN;

    EXEC [KMTGuard].[dbo].[_addsilklive]
        @CharID = @CharID,
        @Amount = @SilkAmount,
        @GiftAmount = 0,
        @PointAmount = 0;

    SET @Notice = 'Hesabiniza ' + CONVERT(varchar(20), @SilkAmount) + ' silk eklenmistir.';

    EXEC [KMTGuard].[dbo].[Command_NoticeByID]
        @CharID = @CharID,
        @NoticeType = 3,
        @Notice = @Notice;
END
GO
