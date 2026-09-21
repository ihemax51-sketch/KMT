USE [SRO_VT_SHARD];
GO
SET NOCOUNT ON;
GO

IF OBJECT_ID(N'dbo.KMTGuard_RefSkillByItemOptLevel_OrphanBackup', N'U') IS NULL
    THROW 51110, 'The orphan mapping backup table is missing.', 1;

IF EXISTS
(
    SELECT 1
    FROM dbo._RefSkillByItemOptLevel AS Mapping
    WHERE NOT EXISTS
    (
        SELECT 1
        FROM dbo._RefSkill AS Skill
        WHERE Skill.ID = Mapping.RefSkillID
    )
)
    THROW 51111, '_RefSkillByItemOptLevel still contains references to missing skills.', 1;

SELECT N'GameServer item-skill mapping integrity validation passed.' AS Result;
GO
