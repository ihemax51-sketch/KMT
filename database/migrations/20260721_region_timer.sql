USE [KMTGuard];
GO

/*
    Displays the event timer only for players inside @RegionID.
    Players entering the region while the timer is active receive the remaining
    time. Players leaving the region have this timer removed from their UI.

    @Seconds = 0 cancels the active timer for the region immediately.
*/
CREATE OR ALTER PROCEDURE [dbo].[Event_AddRegionTimer]
      @Seconds  int
    , @RegionID int
AS
BEGIN
    SET NOCOUNT ON;
    SET XACT_ABORT ON;

    IF @Seconds IS NULL OR @Seconds < 0 OR @Seconds > 2147483
        THROW 51000, 'Seconds must be between 0 and 2147483.', 1;

    IF @RegionID IS NULL OR @RegionID <= 0
        THROW 51001, 'RegionID must be greater than zero.', 1;

    INSERT INTO [dbo].[Command_FilterQueue]
        ([CommandID], [Data1], [Data2], [Status])
    VALUES
        (24, @Seconds, @RegionID, 1);
END;
GO

/*
Example:
    EXEC [dbo].[Event_AddRegionTimer] @Seconds = 600, @RegionID = 25000;

Cancel:
    EXEC [dbo].[Event_AddRegionTimer] @Seconds = 0, @RegionID = 25000;
*/
