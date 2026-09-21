[CmdletBinding()]
param(
    [string]$SettingsPath = 'D:\KMTGuard-build\Filter\Settings.json',
    [string]$MapPath = 'D:\Filter-Venom\database\kmtguard_procedure_name_map.csv',
    [string]$MigrationPath = 'D:\Filter-Venom\database\migrations\20260716_organize_kmtguard_procedures.sql',
    [string]$DefinitionsBackupPath = 'D:\Filter-Venom\database\backups\20260716_kmtguard_procedures_before_organize.sql'
)

$ErrorActionPreference = 'Stop'
Set-StrictMode -Version Latest

function Quote-SqlIdentifier([string]$value) {
    return '[' + $value.Replace(']', ']]') + ']'
}

function Quote-SqlLiteral([string]$value) {
    return "N'" + $value.Replace("'", "''") + "'"
}

function Rewrite-ProcedureHeader(
    [string]$definition,
    [string]$oldSchema,
    [string]$oldName,
    [string]$newSchema,
    [string]$newName
) {
    $schemaPattern = [regex]::Escape($oldSchema)
    $namePattern = [regex]::Escape($oldName)
    $pattern = "(?is)\b(?:CREATE\s+(?:OR\s+ALTER\s+)?|ALTER\s+)PROC(?:EDURE)?\s+(?:(?:\[$schemaPattern\]|$schemaPattern)\s*\.\s*)?(?:\[$namePattern\]|$namePattern)(?![A-Za-z0-9_])"
    $replacement = "CREATE OR ALTER PROCEDURE $(Quote-SqlIdentifier $newSchema).$(Quote-SqlIdentifier $newName)"
    $regex = [regex]::new($pattern)

    if ($regex.IsMatch($definition)) {
        return $regex.Replace($definition, $replacement, 1)
    }

    # Previous sp_rename operations can leave the old CREATE PROCEDURE header
    # inside sys.sql_modules even though the catalog object has another name.
    $genericPattern = '(?is)\b(?:CREATE\s+(?:OR\s+ALTER\s+)?|ALTER\s+)PROC(?:EDURE)?\s+(?:(?:\[[^\]]+\]|[A-Za-z0-9_]+)\s*\.\s*)?(?:\[[^\]]+\]|[A-Za-z0-9_]+)'
    $genericRegex = [regex]::new($genericPattern)
    if (-not $genericRegex.IsMatch($definition)) {
        throw "Could not locate procedure header for $oldSchema.$oldName"
    }

    Write-Warning "Correcting stale procedure header for $oldSchema.$oldName"
    return $genericRegex.Replace($definition, $replacement, 1)
}

function Rewrite-ExecReferences([string]$definition, [array]$map) {
    $rewritten = $definition

    foreach ($entry in ($map | Sort-Object { $_.OldName.Length } -Descending)) {
        $oldNamePattern = [regex]::Escape($entry.OldName)
        $objectPattern = "(?:(?:\[?KMTGuard\]?\s*\.\s*\[?dbo\]?\s*\.|\[?KMTGuard\]?\s*\.\s*\.|\[?dbo\]?\s*\.)\s*)?(?:\[$oldNamePattern\]|$oldNamePattern)(?![A-Za-z0-9_])"
        $pattern = "(?im)(?<exec>\bEXEC(?:UTE)?\s+(?:(?:\[?@[A-Za-z0-9_]+\]?|@[A-Za-z0-9_]+)\s*=\s*)?)$objectPattern"
        $replacement = '${exec}' + (Quote-SqlIdentifier $entry.NewSchema) + '.' + (Quote-SqlIdentifier $entry.NewName)
        $rewritten = [regex]::Replace($rewritten, $pattern, $replacement)
    }

    return $rewritten
}

if (-not (Test-Path -LiteralPath $SettingsPath)) {
    throw "Settings file was not found: $SettingsPath"
}

if (-not (Test-Path -LiteralPath $MapPath)) {
    throw "Procedure map was not found: $MapPath"
}

$settings = Get-Content -LiteralPath $SettingsPath -Raw | ConvertFrom-Json
$server = [string]$settings.Address
if ($settings.Port) {
    $server = "$server,$($settings.Port)"
}

$map = @(Import-Csv -LiteralPath $MapPath)
if ($map.Count -eq 0) {
    throw 'The procedure map is empty.'
}

$duplicateOld = @($map | Group-Object OldSchema, OldName | Where-Object Count -gt 1)
$duplicateNew = @($map | Group-Object NewSchema, NewName | Where-Object Count -gt 1)
if ($duplicateOld.Count -gt 0 -or $duplicateNew.Count -gt 0) {
    throw 'The procedure map contains duplicate old or new names.'
}

$query = @'
SET NOCOUNT ON;
SELECT
    s.name AS OldSchema,
    p.name AS OldName,
    m.definition AS Definition,
    m.uses_ansi_nulls AS UsesAnsiNulls,
    m.uses_quoted_identifier AS UsesQuotedIdentifier
FROM sys.procedures AS p
INNER JOIN sys.schemas AS s ON s.schema_id = p.schema_id
INNER JOIN sys.sql_modules AS m ON m.object_id = p.object_id
WHERE p.is_ms_shipped = 0
ORDER BY s.name, p.name;
'@

$procedures = @(Invoke-Sqlcmd `
    -ServerInstance $server `
    -Database 'KMTGuard' `
    -Username ([string]$settings.Username) `
    -Password ([string]$settings.Password) `
    -Query $query `
    -QueryTimeout 120 `
    -MaxCharLength 1048576)

$procedureLookup = @{}
foreach ($procedure in $procedures) {
    $procedureLookup["$($procedure.OldSchema).$($procedure.OldName)"] = $procedure
}

$mapKeys = @($map | ForEach-Object { "$($_.OldSchema).$($_.OldName)" })
$databaseKeys = @($procedures | ForEach-Object { "$($_.OldSchema).$($_.OldName)" })
$differences = @(Compare-Object $mapKeys $databaseKeys)
if ($differences.Count -gt 0) {
    $details = $differences | ForEach-Object { "$($_.SideIndicator) $($_.InputObject)" }
    throw "The map does not exactly match the database procedures:`n$($details -join "`n")"
}

$backup = [System.Text.StringBuilder]::new()
[void]$backup.AppendLine('USE [KMTGuard];')
[void]$backup.AppendLine('GO')
[void]$backup.AppendLine('-- Generated before the KMTGuard procedure organization migration.')
[void]$backup.AppendLine('-- A verified COPY_ONLY database backup is the authoritative rollback source.')
[void]$backup.AppendLine()

foreach ($entry in $map) {
    $procedure = $procedureLookup["$($entry.OldSchema).$($entry.OldName)"]
    [void]$backup.AppendLine($(if ([bool]$procedure.UsesAnsiNulls) { 'SET ANSI_NULLS ON;' } else { 'SET ANSI_NULLS OFF;' }))
    [void]$backup.AppendLine('GO')
    [void]$backup.AppendLine($(if ([bool]$procedure.UsesQuotedIdentifier) { 'SET QUOTED_IDENTIFIER ON;' } else { 'SET QUOTED_IDENTIFIER OFF;' }))
    [void]$backup.AppendLine('GO')
    [void]$backup.AppendLine([string]$procedure.Definition)
    [void]$backup.AppendLine('GO')
    [void]$backup.AppendLine()
}

$migration = [System.Text.StringBuilder]::new()
[void]$migration.AppendLine('USE [KMTGuard];')
[void]$migration.AppendLine('GO')
[void]$migration.AppendLine('SET NOCOUNT ON;')
[void]$migration.AppendLine('SET XACT_ABORT ON;')
[void]$migration.AppendLine('GO')
[void]$migration.AppendLine('-- Canonical procedure names are grouped by schema. Legacy dbo names become synonyms.')
[void]$migration.AppendLine('-- This migration intentionally changes KMTGuard only.')
[void]$migration.AppendLine('CREATE TABLE #KmtProcedureMap')
[void]$migration.AppendLine('(')
[void]$migration.AppendLine('    OldSchema SYSNAME COLLATE DATABASE_DEFAULT NOT NULL,')
[void]$migration.AppendLine('    OldName SYSNAME COLLATE DATABASE_DEFAULT NOT NULL,')
[void]$migration.AppendLine('    NewSchema SYSNAME COLLATE DATABASE_DEFAULT NOT NULL,')
[void]$migration.AppendLine('    NewName SYSNAME COLLATE DATABASE_DEFAULT NOT NULL,')
[void]$migration.AppendLine('    CONSTRAINT PK_KmtProcedureMap PRIMARY KEY (OldSchema, OldName),')
[void]$migration.AppendLine('    CONSTRAINT UQ_KmtProcedureMap_New UNIQUE (NewSchema, NewName)')
[void]$migration.AppendLine(');')
[void]$migration.AppendLine()
[void]$migration.AppendLine('INSERT INTO #KmtProcedureMap (OldSchema, OldName, NewSchema, NewName)')
[void]$migration.AppendLine('VALUES')
for ($index = 0; $index -lt $map.Count; $index++) {
    $entry = $map[$index]
    $suffix = if ($index -eq $map.Count - 1) { ';' } else { ',' }
    [void]$migration.AppendLine("    ($(Quote-SqlLiteral $entry.OldSchema), $(Quote-SqlLiteral $entry.OldName), $(Quote-SqlLiteral $entry.NewSchema), $(Quote-SqlLiteral $entry.NewName))$suffix")
}
[void]$migration.AppendLine()
[void]$migration.AppendLine('BEGIN TRY')
[void]$migration.AppendLine('    BEGIN TRANSACTION;')
[void]$migration.AppendLine()
[void]$migration.AppendLine('    DECLARE @SchemaName SYSNAME;')
[void]$migration.AppendLine('    DECLARE @SchemaSql NVARCHAR(MAX);')
[void]$migration.AppendLine('    DECLARE SchemaCursor CURSOR LOCAL FAST_FORWARD FOR')
[void]$migration.AppendLine('        SELECT DISTINCT NewSchema FROM #KmtProcedureMap ORDER BY NewSchema;')
[void]$migration.AppendLine('    OPEN SchemaCursor;')
[void]$migration.AppendLine('    FETCH NEXT FROM SchemaCursor INTO @SchemaName;')
[void]$migration.AppendLine('    WHILE @@FETCH_STATUS = 0')
[void]$migration.AppendLine('    BEGIN')
[void]$migration.AppendLine('        IF SCHEMA_ID(@SchemaName) IS NULL')
[void]$migration.AppendLine('        BEGIN')
[void]$migration.AppendLine("            SET @SchemaSql = N'CREATE SCHEMA ' + QUOTENAME(@SchemaName) + N' AUTHORIZATION [dbo];';")
[void]$migration.AppendLine('            EXEC sys.sp_executesql @SchemaSql;')
[void]$migration.AppendLine('        END;')
[void]$migration.AppendLine('        FETCH NEXT FROM SchemaCursor INTO @SchemaName;')
[void]$migration.AppendLine('    END;')
[void]$migration.AppendLine('    CLOSE SchemaCursor;')
[void]$migration.AppendLine('    DEALLOCATE SchemaCursor;')
[void]$migration.AppendLine()
[void]$migration.AppendLine('    DECLARE @OldSchema SYSNAME, @OldName SYSNAME, @NewSchema SYSNAME, @NewName SYSNAME;')
[void]$migration.AppendLine('    DECLARE @OldObject NVARCHAR(517), @NewObject NVARCHAR(517), @Sql NVARCHAR(MAX);')
[void]$migration.AppendLine('    DECLARE ProcedureCursor CURSOR LOCAL FAST_FORWARD FOR')
[void]$migration.AppendLine('        SELECT OldSchema, OldName, NewSchema, NewName FROM #KmtProcedureMap ORDER BY OldSchema, OldName;')
[void]$migration.AppendLine('    OPEN ProcedureCursor;')
[void]$migration.AppendLine('    FETCH NEXT FROM ProcedureCursor INTO @OldSchema, @OldName, @NewSchema, @NewName;')
[void]$migration.AppendLine('    WHILE @@FETCH_STATUS = 0')
[void]$migration.AppendLine('    BEGIN')
[void]$migration.AppendLine('        SET @OldObject = QUOTENAME(@OldSchema) + N''.'' + QUOTENAME(@OldName);')
[void]$migration.AppendLine('        SET @NewObject = QUOTENAME(@NewSchema) + N''.'' + QUOTENAME(@NewName);')
[void]$migration.AppendLine()
[void]$migration.AppendLine('        IF OBJECT_ID(@NewObject, N''P'') IS NULL')
[void]$migration.AppendLine('        BEGIN')
[void]$migration.AppendLine('            IF OBJECT_ID(@OldObject, N''P'') IS NULL')
[void]$migration.AppendLine('                THROW 51000, ''A mapped source procedure is missing.'', 1;')
[void]$migration.AppendLine()
[void]$migration.AppendLine('            EXEC sys.sp_rename @objname = @OldObject, @newname = @NewName, @objtype = N''OBJECT'';')
[void]$migration.AppendLine('            SET @Sql = N''ALTER SCHEMA '' + QUOTENAME(@NewSchema) + N'' TRANSFER '' + QUOTENAME(@OldSchema) + N''.'' + QUOTENAME(@NewName) + N'';'';')
[void]$migration.AppendLine('            EXEC sys.sp_executesql @Sql;')
[void]$migration.AppendLine('        END;')
[void]$migration.AppendLine()
[void]$migration.AppendLine('        IF OBJECT_ID(@NewObject, N''P'') IS NULL')
[void]$migration.AppendLine('            THROW 51001, ''A canonical procedure could not be created.'', 1;')
[void]$migration.AppendLine()
[void]$migration.AppendLine('        IF EXISTS')
[void]$migration.AppendLine('        (')
[void]$migration.AppendLine('            SELECT 1 FROM sys.synonyms AS sy')
[void]$migration.AppendLine('            INNER JOIN sys.schemas AS ss ON ss.schema_id = sy.schema_id')
[void]$migration.AppendLine('            WHERE ss.name = @OldSchema AND sy.name = @OldName')
[void]$migration.AppendLine('        )')
[void]$migration.AppendLine('        BEGIN')
[void]$migration.AppendLine('            SET @Sql = N''DROP SYNONYM '' + @OldObject + N'';'';')
[void]$migration.AppendLine('            EXEC sys.sp_executesql @Sql;')
[void]$migration.AppendLine('        END;')
[void]$migration.AppendLine()
[void]$migration.AppendLine('        IF EXISTS')
[void]$migration.AppendLine('        (')
[void]$migration.AppendLine('            SELECT 1 FROM sys.objects AS o')
[void]$migration.AppendLine('            INNER JOIN sys.schemas AS ss ON ss.schema_id = o.schema_id')
[void]$migration.AppendLine('            WHERE ss.name = @OldSchema AND o.name = @OldName')
[void]$migration.AppendLine('        )')
[void]$migration.AppendLine('            THROW 51002, ''The legacy compatibility name is occupied by another object.'', 1;')
[void]$migration.AppendLine()
[void]$migration.AppendLine('        SET @Sql = N''CREATE SYNONYM '' + @OldObject + N'' FOR '' + @NewObject + N'';'';')
[void]$migration.AppendLine('        EXEC sys.sp_executesql @Sql;')
[void]$migration.AppendLine()
[void]$migration.AppendLine('        FETCH NEXT FROM ProcedureCursor INTO @OldSchema, @OldName, @NewSchema, @NewName;')
[void]$migration.AppendLine('    END;')
[void]$migration.AppendLine('    CLOSE ProcedureCursor;')
[void]$migration.AppendLine('    DEALLOCATE ProcedureCursor;')
[void]$migration.AppendLine()

foreach ($entry in $map) {
    $procedure = $procedureLookup["$($entry.OldSchema).$($entry.OldName)"]
    $definition = Rewrite-ProcedureHeader `
        -definition ([string]$procedure.Definition) `
        -oldSchema $entry.OldSchema `
        -oldName $entry.OldName `
        -newSchema $entry.NewSchema `
        -newName $entry.NewName
    $definition = Rewrite-ExecReferences -definition $definition -map $map

    [void]$migration.AppendLine($(if ([bool]$procedure.UsesAnsiNulls) { '    SET ANSI_NULLS ON;' } else { '    SET ANSI_NULLS OFF;' }))
    [void]$migration.AppendLine($(if ([bool]$procedure.UsesQuotedIdentifier) { '    SET QUOTED_IDENTIFIER ON;' } else { '    SET QUOTED_IDENTIFIER OFF;' }))
    [void]$migration.AppendLine('    EXEC sys.sp_executesql ' + (Quote-SqlLiteral $definition) + ';')
    [void]$migration.AppendLine()
}

[void]$migration.AppendLine('    IF (SELECT COUNT(*) FROM #KmtProcedureMap AS m WHERE OBJECT_ID(QUOTENAME(m.NewSchema) + N''.'' + QUOTENAME(m.NewName), N''P'') IS NOT NULL) <> (SELECT COUNT(*) FROM #KmtProcedureMap)')
[void]$migration.AppendLine('        THROW 51003, ''Canonical procedure validation failed.'', 1;')
[void]$migration.AppendLine()
[void]$migration.AppendLine('    IF')
[void]$migration.AppendLine('    (')
[void]$migration.AppendLine('        SELECT COUNT(*)')
[void]$migration.AppendLine('        FROM #KmtProcedureMap AS m')
[void]$migration.AppendLine('        INNER JOIN sys.schemas AS ss ON ss.name = m.OldSchema')
[void]$migration.AppendLine('        INNER JOIN sys.synonyms AS sy ON sy.schema_id = ss.schema_id AND sy.name = m.OldName')
[void]$migration.AppendLine('    ) <> (SELECT COUNT(*) FROM #KmtProcedureMap)')
[void]$migration.AppendLine('        THROW 51004, ''Legacy synonym validation failed.'', 1;')
[void]$migration.AppendLine()
[void]$migration.AppendLine('    COMMIT TRANSACTION;')
[void]$migration.AppendLine('END TRY')
[void]$migration.AppendLine('BEGIN CATCH')
[void]$migration.AppendLine('    IF @@TRANCOUNT > 0')
[void]$migration.AppendLine('        ROLLBACK TRANSACTION;')
[void]$migration.AppendLine('    THROW;')
[void]$migration.AppendLine('END CATCH;')
[void]$migration.AppendLine('GO')
[void]$migration.AppendLine('DROP TABLE #KmtProcedureMap;')
[void]$migration.AppendLine('GO')
[void]$migration.AppendLine("PRINT 'KMTGuard procedures organized successfully: $($map.Count) canonical procedures and $($map.Count) compatibility synonyms.';")
[void]$migration.AppendLine('GO')

$utf8WithoutBom = [System.Text.UTF8Encoding]::new($false)
[System.IO.Directory]::CreateDirectory((Split-Path -Parent $MigrationPath)) | Out-Null
[System.IO.Directory]::CreateDirectory((Split-Path -Parent $DefinitionsBackupPath)) | Out-Null
[System.IO.File]::WriteAllText($MigrationPath, $migration.ToString(), $utf8WithoutBom)
[System.IO.File]::WriteAllText($DefinitionsBackupPath, $backup.ToString(), $utf8WithoutBom)

Write-Output "Generated migration: $MigrationPath"
Write-Output "Generated definitions backup: $DefinitionsBackupPath"
Write-Output "Mapped procedures: $($map.Count)"
