SET NOCOUNT ON;

IF OBJECT_ID(N'dbo.Clientless_Accounts', N'U') IS NULL
    THROW 51110, 'dbo.Clientless_Accounts is missing.', 1;

IF NOT EXISTS
(
    SELECT 1 FROM sys.columns
    WHERE object_id = OBJECT_ID(N'dbo.Clientless_Accounts')
      AND name = N'ProfileAvatarsEnabled' AND system_type_id = 104 AND is_nullable = 0
)
    THROW 51111, 'ProfileAvatarsEnabled is missing or invalid.', 1;

IF NOT EXISTS
(
    SELECT 1 FROM sys.columns
    WHERE object_id = OBJECT_ID(N'dbo.Clientless_Accounts')
      AND name = N'ProfilePetsEnabled' AND system_type_id = 104 AND is_nullable = 0
)
    THROW 51112, 'ProfilePetsEnabled is missing or invalid.', 1;

IF EXISTS
(
    SELECT 1 FROM dbo.Clientless_Accounts
    WHERE ProfileAvatarsEnabled IS NULL OR ProfilePetsEnabled IS NULL
)
    THROW 51113, 'A Clientless account has an undefined avatar or pet option.', 1;

SELECT N'Clientless creation options validation passed.' AS Result;
