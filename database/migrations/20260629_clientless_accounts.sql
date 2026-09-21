IF OBJECT_ID('_ClientlessAccounts', 'U') IS NULL
BEGIN
    CREATE TABLE _ClientlessAccounts
    (
        ID INT IDENTITY(1,1) NOT NULL CONSTRAINT PK_ClientlessAccounts PRIMARY KEY,
        Enabled BIT NOT NULL CONSTRAINT DF_ClientlessAccounts_Enabled DEFAULT(1),
        Locale TINYINT NOT NULL CONSTRAINT DF_ClientlessAccounts_Locale DEFAULT(22),
        ShardID SMALLINT NOT NULL,
        AccountName VARCHAR(64) NOT NULL,
        AccountPassword VARCHAR(128) NOT NULL,
        CharacterName VARCHAR(64) NOT NULL,
        AgentAuthMode VARCHAR(32) NOT NULL CONSTRAINT DF_ClientlessAccounts_AuthMode DEFAULT('Auto'),
        AgentAuthDelayMs INT NOT NULL CONSTRAINT DF_ClientlessAccounts_AuthDelay DEFAULT(1500),
        AgentAuthPaddingHex VARCHAR(256) NOT NULL CONSTRAINT DF_ClientlessAccounts_AuthPadding DEFAULT(''),
        LaunchDelayMs INT NOT NULL CONSTRAINT DF_ClientlessAccounts_LaunchDelay DEFAULT(1000),
        ReconnectDelaySeconds INT NOT NULL CONSTRAINT DF_ClientlessAccounts_Reconnect DEFAULT(30),
        LastStatus VARCHAR(32) NOT NULL CONSTRAINT DF_ClientlessAccounts_Status DEFAULT('Pending'),
        LastMessage NVARCHAR(512) NULL,
        LastLoginAt DATETIME2 NULL,
        LastDisconnectAt DATETIME2 NULL,
        CreatedAt DATETIME2 NOT NULL CONSTRAINT DF_ClientlessAccounts_CreatedAt DEFAULT(SYSDATETIME()),
        UpdatedAt DATETIME2 NOT NULL CONSTRAINT DF_ClientlessAccounts_UpdatedAt DEFAULT(SYSDATETIME())
    );

    CREATE UNIQUE INDEX UX_ClientlessAccounts_AccountCharacter
        ON _ClientlessAccounts(AccountName, CharacterName);
END

IF COL_LENGTH('_ClientlessAccounts', 'AgentAuthMode') IS NULL
    ALTER TABLE _ClientlessAccounts
        ADD AgentAuthMode VARCHAR(32) NOT NULL
            CONSTRAINT DF_ClientlessAccounts_AuthMode DEFAULT('Auto');

IF COL_LENGTH('_ClientlessAccounts', 'AgentAuthDelayMs') IS NULL
    ALTER TABLE _ClientlessAccounts
        ADD AgentAuthDelayMs INT NOT NULL
            CONSTRAINT DF_ClientlessAccounts_AuthDelay DEFAULT(1500);

IF COL_LENGTH('_ClientlessAccounts', 'AgentAuthPaddingHex') IS NULL
    ALTER TABLE _ClientlessAccounts
        ADD AgentAuthPaddingHex VARCHAR(256) NOT NULL
            CONSTRAINT DF_ClientlessAccounts_AuthPadding DEFAULT('');

IF COL_LENGTH('_ClientlessAccounts', 'LaunchDelayMs') IS NULL
    ALTER TABLE _ClientlessAccounts
        ADD LaunchDelayMs INT NOT NULL
            CONSTRAINT DF_ClientlessAccounts_LaunchDelay DEFAULT(1000);

-- Example:
-- INSERT INTO _ClientlessAccounts (Enabled, Locale, ShardID, AccountName, AccountPassword, CharacterName, AgentAuthMode, AgentAuthDelayMs, AgentAuthPaddingHex, LaunchDelayMs)
-- VALUES (1, 22, 64, 'account_id', 'account_password', 'CharName16', 'Full', 1500, '000000000000', 1000);
