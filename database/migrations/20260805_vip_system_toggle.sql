/*
    Adds the durable VIP System enable/disable switch used by the Control
    Center. Missing or unrecognized values remain enabled for compatibility.
*/
USE [KMTGuard];
GO

SET NOCOUNT ON;
SET XACT_ABORT ON;
GO

IF OBJECT_ID(N'dbo.System_Settings', N'U') IS NULL
    THROW 51020, 'KMTGuard.dbo.System_Settings was not found.', 1;

IF NOT EXISTS
(
    SELECT 1
    FROM dbo.System_Settings
    WHERE SettingName = N'VipSystemEnabled'
)
BEGIN
    INSERT dbo.System_Settings (SettingName, Value)
    VALUES (N'VipSystemEnabled', N'True');
END;
GO

CREATE OR ALTER PROCEDURE dbo.Vip_OnCharacterLogin
    @CharID INT
AS
BEGIN
    SET NOCOUNT ON;

    IF EXISTS
    (
        SELECT 1
        FROM dbo.System_Settings WITH (NOLOCK)
        WHERE SettingName = N'VipSystemEnabled'
          AND LOWER(LTRIM(RTRIM(CONVERT(NVARCHAR(512), Value)))) IN
              (N'0', N'false', N'off', N'no')
    )
        RETURN;

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

PRINT 'VIP System toggle migration completed.';
GO
