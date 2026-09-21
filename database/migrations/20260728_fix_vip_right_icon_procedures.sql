/*
    Move the VIP purchase hook to the rebuilt player-style procedures.

    Run this script on the KMTGuard database after
    20260727_player_style_system_rebuild.sql.
*/

SET NOCOUNT ON;
SET XACT_ABORT ON;
GO

IF OBJECT_ID(N'dbo.RightIcon_Add', N'P') IS NULL
    THROW 51020, 'dbo.RightIcon_Add is missing. Install the player style system rebuild first.', 1;

IF OBJECT_ID(N'dbo.RightIcon_Activate', N'P') IS NULL
    THROW 51021, 'dbo.RightIcon_Activate is missing. Install the player style system rebuild first.', 1;

IF OBJECT_ID(N'dbo.RightIcon_Deactivate', N'P') IS NULL
    THROW 51022, 'dbo.RightIcon_Deactivate is missing. Install the player style system rebuild first.', 1;
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
        EXEC dbo.RightIcon_Deactivate
            @CharName16 = @CharName16;
    END;

    EXEC dbo.Vip_ApplyConfiguredBuff
        @CharID = @CharID,
        @RankCode = @NewRankCode,
        @ForceRefresh = 0;
END;
GO

PRINT 'VIP right-icon procedure migration completed.';
