USE [SRO_VT_SHARD];
GO
SET NOCOUNT ON;
SET XACT_ABORT ON;
GO

IF OBJECT_ID(N'dbo._RefSkillByItemOptLevel', N'U') IS NULL
    THROW 51100, 'dbo._RefSkillByItemOptLevel is required.', 1;
IF OBJECT_ID(N'dbo._RefSkill', N'U') IS NULL
    THROW 51101, 'dbo._RefSkill is required.', 1;
GO

IF OBJECT_ID(N'dbo.KMTGuard_RefSkillByItemOptLevel_OrphanBackup', N'U') IS NULL
BEGIN
    CREATE TABLE dbo.KMTGuard_RefSkillByItemOptLevel_OrphanBackup
    (
        Link int NOT NULL,
        RefSkillID int NOT NULL,
        CapturedAtUtc datetime2(0) NOT NULL
            CONSTRAINT DF_KMTGuard_RefSkillItemOrphan_CapturedAtUtc DEFAULT (SYSUTCDATETIME()),
        CONSTRAINT PK_KMTGuard_RefSkillItemOrphan PRIMARY KEY (Link, RefSkillID)
    );
END;
GO

BEGIN TRANSACTION;

INSERT dbo.KMTGuard_RefSkillByItemOptLevel_OrphanBackup (Link, RefSkillID)
SELECT DISTINCT Mapping.Link, Mapping.RefSkillID
FROM dbo._RefSkillByItemOptLevel AS Mapping
WHERE NOT EXISTS
(
    SELECT 1
    FROM dbo._RefSkill AS Skill
    WHERE Skill.ID = Mapping.RefSkillID
)
AND NOT EXISTS
(
    SELECT 1
    FROM dbo.KMTGuard_RefSkillByItemOptLevel_OrphanBackup AS BackupRow
    WHERE BackupRow.Link = Mapping.Link
      AND BackupRow.RefSkillID = Mapping.RefSkillID
);

DELETE Mapping
FROM dbo._RefSkillByItemOptLevel AS Mapping
WHERE NOT EXISTS
(
    SELECT 1
    FROM dbo._RefSkill AS Skill
    WHERE Skill.ID = Mapping.RefSkillID
);

COMMIT TRANSACTION;
GO

SELECT COUNT_BIG(*) AS PreservedOrphanMappings
FROM dbo.KMTGuard_RefSkillByItemOptLevel_OrphanBackup;
GO
