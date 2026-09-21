SET XACT_ABORT ON;
GO

CREATE OR ALTER PROCEDURE dbo._self_Teleport
    @CharID INT
AS
BEGIN
    SET NOCOUNT ON;
    SET XACT_ABORT ON;

    IF @CharID IS NULL OR @CharID <= 0
        THROW 50001, 'CharID must be a positive integer.', 1;

    BEGIN TRANSACTION;

    IF NOT EXISTS
    (
        SELECT 1
        FROM dbo._AsyncFilterCommands WITH (UPDLOCK, HOLDLOCK)
        WHERE CommandID = 41
          AND Status = 1
          AND TRY_CONVERT(INT, Data1) = @CharID
    )
    BEGIN
        INSERT INTO dbo._AsyncFilterCommands
            (CommandID, Data1, Status)
        VALUES
            (41, CONVERT(VARCHAR(20), @CharID), 1);
    END;

    COMMIT TRANSACTION;
END;
GO

-- Usage:
-- EXEC dbo._self_Teleport @CharID = 1234;
