SET XACT_ABORT ON;
BEGIN TRANSACTION;

IF COL_LENGTH('_AccountSecondaryPassword', 'PasswordHash') IS NULL
    ALTER TABLE _AccountSecondaryPassword
        ADD PasswordHash VARBINARY(32) NULL;

IF COL_LENGTH('_AccountSecondaryPassword', 'PasswordSalt') IS NULL
    ALTER TABLE _AccountSecondaryPassword
        ADD PasswordSalt VARBINARY(16) NULL;

COMMIT TRANSACTION;

-- Existing plaintext secondary passwords are migrated automatically after
-- their first successful verification. New and changed passwords are stored
-- with PBKDF2-SHA256 and a unique random salt.
