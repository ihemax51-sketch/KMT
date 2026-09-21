SET NOCOUNT ON;

IF OBJECT_ID(N'dbo.Clientless_Accounts', N'U') IS NULL
    THROW 51122, 'Validation failed: dbo.Clientless_Accounts is missing.', 1;

IF NOT EXISTS
(
    SELECT 1 FROM sys.columns
    WHERE object_id = OBJECT_ID(N'dbo.Clientless_Accounts')
      AND name = N'ProfileAttackPetEnabled'
      AND system_type_id = 104
      AND is_nullable = 0
)
    THROW 51123, 'Validation failed: ProfileAttackPetEnabled is missing or invalid.', 1;

IF NOT EXISTS
(
    SELECT 1 FROM sys.columns
    WHERE object_id = OBJECT_ID(N'dbo.Clientless_Accounts')
      AND name = N'ProfileGrabPetEnabled'
      AND system_type_id = 104
      AND is_nullable = 0
)
    THROW 51124, 'Validation failed: ProfileGrabPetEnabled is missing or invalid.', 1;

IF EXISTS
(
    SELECT 1
    FROM dbo.Clientless_Accounts
    WHERE ProfileAttackPetEnabled IS NULL OR ProfileGrabPetEnabled IS NULL
)
    THROW 51125, 'Validation failed: a Clientless account has an undefined pet option.', 1;

IF
(
    SELECT COUNT(1)
    FROM sys.default_constraints AS D
    INNER JOIN sys.columns AS C
        ON C.object_id = D.parent_object_id AND C.column_id = D.parent_column_id
    WHERE D.parent_object_id = OBJECT_ID(N'dbo.Clientless_Accounts')
      AND C.name IN (N'ProfileAttackPetEnabled', N'ProfileGrabPetEnabled')
) <> 2
    THROW 51126, 'Validation failed: one or more independent pet defaults are missing.', 1;

SELECT N'Independent Clientless pet options validation passed.' AS Result;
