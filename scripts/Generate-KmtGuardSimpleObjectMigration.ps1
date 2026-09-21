[CmdletBinding()]
param(
    [string]$SettingsPath = 'D:\KMTGuard-build\Filter\Settings.json',
    [string]$TableMapPath = 'D:\Filter-Venom\database\kmtguard_simple_table_name_map.csv',
    [string]$ProcedureMapPath = 'D:\Filter-Venom\database\kmtguard_simple_procedure_name_map.csv',
    [string]$LegacyProcedureMapPath = 'D:\Filter-Venom\database\kmtguard_procedure_name_map.csv',
    [string]$MigrationPath = 'D:\Filter-Venom\database\migrations\20260716_simplify_kmtguard_object_names.sql',
    [string]$DefinitionsBackupPath = 'D:\Filter-Venom\database\backups\20260716_kmtguard_modules_before_simple_names.sql'
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
    [string]$currentSchema,
    [string]$currentName,
    [string]$newSchema,
    [string]$newName
) {
    $schemaPattern = [regex]::Escape($currentSchema)
    $namePattern = [regex]::Escape($currentName)
    $pattern = "(?is)\b(?:CREATE\s+(?:OR\s+ALTER\s+)?|ALTER\s+)PROC(?:EDURE)?\s+(?:(?:\[$schemaPattern\]|$schemaPattern)\s*\.\s*)?(?:\[$namePattern\]|$namePattern)(?![A-Za-z0-9_])"
    $replacement = "CREATE OR ALTER PROCEDURE $(Quote-SqlIdentifier $newSchema).$(Quote-SqlIdentifier $newName)"
    $regex = [regex]::new($pattern)
    if ($regex.IsMatch($definition)) {
        return $regex.Replace($definition, $replacement, 1)
    }

    $genericPattern = '(?is)\b(?:CREATE\s+(?:OR\s+ALTER\s+)?|ALTER\s+)PROC(?:EDURE)?\s+(?:(?:\[[^\]]+\]|[A-Za-z0-9_]+)\s*\.\s*)?(?:\[[^\]]+\]|[A-Za-z0-9_]+)'
    $genericRegex = [regex]::new($genericPattern)
    if (-not $genericRegex.IsMatch($definition)) {
        throw "Could not locate procedure header for $currentSchema.$currentName"
    }

    Write-Warning "Correcting stale procedure header for $currentSchema.$currentName"
    return $genericRegex.Replace($definition, $replacement, 1)
}

function Rewrite-ViewHeader([string]$definition, [string]$schema, [string]$name) {
    $schemaPattern = [regex]::Escape($schema)
    $namePattern = [regex]::Escape($name)
    $pattern = "(?is)\b(?:CREATE\s+(?:OR\s+ALTER\s+)?|ALTER\s+)VIEW\s+(?:(?:\[$schemaPattern\]|$schemaPattern)\s*\.\s*)?(?:\[$namePattern\]|$namePattern)(?![A-Za-z0-9_])"
    $replacement = "CREATE OR ALTER VIEW $(Quote-SqlIdentifier $schema).$(Quote-SqlIdentifier $name)"
    $regex = [regex]::new($pattern)
    if (-not $regex.IsMatch($definition)) {
        throw "Could not locate view header for $schema.$name"
    }
    return $regex.Replace($definition, $replacement, 1)
}

function Rewrite-TriggerHeader([string]$definition, [string]$newSchema, [string]$name) {
    $pattern = '(?is)\b(?:CREATE\s+(?:OR\s+ALTER\s+)?|ALTER\s+)TRIGGER\s+(?:(?:\[[^\]]+\]|[A-Za-z0-9_]+)\s*\.\s*)?(?:\[[^\]]+\]|[A-Za-z0-9_]+)'
    $replacement = "CREATE OR ALTER TRIGGER $(Quote-SqlIdentifier $newSchema).$(Quote-SqlIdentifier $name)"
    $regex = [regex]::new($pattern)
    if (-not $regex.IsMatch($definition)) {
        throw "Could not locate trigger header for $name"
    }
    return $regex.Replace($definition, $replacement, 1)
}

function Rewrite-TableReferences([string]$definition, [array]$tableMap) {
    $rewritten = $definition
    foreach ($entry in ($tableMap | Sort-Object { $_.OldName.Length } -Descending)) {
        $oldName = [regex]::Escape([string]$entry.OldName)
        $oldSchema = [regex]::Escape([string]$entry.OldSchema)
        $token = "(?:\[$oldName\]|$oldName)(?![A-Za-z0-9_])"
        $boundary = '(?<![A-Za-z0-9_\].])'
        $replacement = (Quote-SqlIdentifier $entry.NewSchema) + '.' + (Quote-SqlIdentifier $entry.NewName)

        $patterns = @(
            "(?i)$boundary(?:\[?KMTGuard\]?)\s*\.\s*(?:\[?$oldSchema\]?)\s*\.\s*$token",
            "(?i)$boundary(?:\[?KMTGuard\]?)\s*\.\s*\.\s*$token",
            "(?i)$boundary(?:\[?$oldSchema\]?)\s*\.\s*$token",
            "(?i)$boundary$token"
        )
        foreach ($pattern in $patterns) {
            $rewritten = [regex]::Replace($rewritten, $pattern, $replacement)
        }
    }
    return $rewritten
}

function Rewrite-ExecReferences([string]$definition, [array]$procedureMap, [array]$legacyMap) {
    $rewritten = $definition

    foreach ($entry in ($procedureMap | Sort-Object { $_.CurrentName.Length } -Descending)) {
        $schemaPattern = [regex]::Escape([string]$entry.CurrentSchema)
        $namePattern = [regex]::Escape([string]$entry.CurrentName)
        $objectPattern = "(?:(?:\[?KMTGuard\]?\s*\.\s*\[?$schemaPattern\]?\s*\.|\[?$schemaPattern\]?\s*\.)\s*)?(?:\[$namePattern\]|$namePattern)(?![A-Za-z0-9_])"
        $pattern = "(?im)(?<exec>\bEXEC(?:UTE)?\s+(?:(?:\[?@[A-Za-z0-9_]+\]?|@[A-Za-z0-9_]+)\s*=\s*)?)$objectPattern"
        $replacement = '${exec}' + (Quote-SqlIdentifier $entry.NewSchema) + '.' + (Quote-SqlIdentifier $entry.NewName)
        $rewritten = [regex]::Replace($rewritten, $pattern, $replacement)
    }

    foreach ($entry in ($legacyMap | Sort-Object { $_.OldName.Length } -Descending)) {
        $namePattern = [regex]::Escape([string]$entry.OldName)
        $objectPattern = "(?:(?:\[?KMTGuard\]?\s*\.\s*\[?dbo\]?\s*\.|\[?KMTGuard\]?\s*\.\s*\.|\[?dbo\]?\s*\.)\s*)?(?:\[$namePattern\]|$namePattern)(?![A-Za-z0-9_])"
        $pattern = "(?im)(?<exec>\bEXEC(?:UTE)?\s+(?:(?:\[?@[A-Za-z0-9_]+\]?|@[A-Za-z0-9_]+)\s*=\s*)?)$objectPattern"
        $replacement = '${exec}' + (Quote-SqlIdentifier $entry.NewSchema) + '.' + (Quote-SqlIdentifier $entry.NewName)
        $rewritten = [regex]::Replace($rewritten, $pattern, $replacement)
    }

    return $rewritten
}

function Append-MapValues(
    [System.Text.StringBuilder]$builder,
    [array]$rows,
    [string[]]$columns
) {
    for ($index = 0; $index -lt $rows.Count; $index++) {
        $row = $rows[$index]
        $values = foreach ($column in $columns) {
            Quote-SqlLiteral ([string]$row.$column)
        }
        $suffix = if ($index -eq $rows.Count - 1) { ';' } else { ',' }
        [void]$builder.AppendLine('    (' + ($values -join ', ') + ')' + $suffix)
    }
}

foreach ($path in @($SettingsPath, $TableMapPath, $ProcedureMapPath, $LegacyProcedureMapPath)) {
    if (-not (Test-Path -LiteralPath $path)) {
        throw "Required input was not found: $path"
    }
}

$settings = Get-Content -LiteralPath $SettingsPath -Raw | ConvertFrom-Json
$server = [string]$settings.Address
if ($settings.Port -and [int]$settings.Port -ne 1433 -and $server -notmatch ',') {
    $server = "$server,$($settings.Port)"
}

$tableMap = @(Import-Csv -LiteralPath $TableMapPath)
$procedureMap = @(Import-Csv -LiteralPath $ProcedureMapPath)
$oldProcedureMap = @(Import-Csv -LiteralPath $LegacyProcedureMapPath)
if ($tableMap.Count -eq 0 -or $procedureMap.Count -eq 0 -or $oldProcedureMap.Count -eq 0) {
    throw 'One or more object maps are empty.'
}

$duplicateTableOld = @($tableMap | Group-Object OldSchema, OldName | Where-Object Count -gt 1)
$duplicateTableNew = @($tableMap | Group-Object NewSchema, NewName | Where-Object Count -gt 1)
$duplicateProcedureOld = @($procedureMap | Group-Object CurrentSchema, CurrentName | Where-Object Count -gt 1)
$duplicateProcedureNew = @($procedureMap | Group-Object NewSchema, NewName | Where-Object Count -gt 1)
if ($duplicateTableOld.Count -or $duplicateTableNew.Count -or $duplicateProcedureOld.Count -or $duplicateProcedureNew.Count) {
    throw 'An object map contains duplicate source or target names.'
}

$finalTableNames = @($tableMap | ForEach-Object { "$($_.NewSchema).$($_.NewName)" })
$finalProcedureNames = @($procedureMap | ForEach-Object { "$($_.NewSchema).$($_.NewName)" })
if (@(Compare-Object $finalTableNames $finalProcedureNames -IncludeEqual -ExcludeDifferent).Count -gt 0) {
    throw 'A final table name conflicts with a final procedure name.'
}

$procedureByCurrent = @{}
foreach ($entry in $procedureMap) {
    $procedureByCurrent["$($entry.CurrentSchema).$($entry.CurrentName)"] = $entry
}

$legacyMap = foreach ($legacy in $oldProcedureMap) {
    $key = "$($legacy.NewSchema).$($legacy.NewName)"
    if (-not $procedureByCurrent.ContainsKey($key)) {
        throw "The old procedure map does not resolve to the current map: $key"
    }
    $target = $procedureByCurrent[$key]
    [pscustomobject]@{
        OldSchema = [string]$legacy.OldSchema
        OldName = [string]$legacy.OldName
        NewSchema = [string]$target.NewSchema
        NewName = [string]$target.NewName
    }
}

$tableQuery = @'
SET NOCOUNT ON;
SELECT s.name AS CurrentSchema, t.name AS CurrentName
FROM sys.tables AS t
INNER JOIN sys.schemas AS s ON s.schema_id = t.schema_id
WHERE t.is_ms_shipped = 0
ORDER BY s.name, t.name;
'@

$moduleQuery = @'
SET NOCOUNT ON;
SELECT
    N'P' AS ObjectType,
    s.name AS CurrentSchema,
    p.name AS CurrentName,
    CAST(NULL AS sysname) AS ParentSchema,
    CAST(NULL AS sysname) AS ParentName,
    m.definition AS Definition,
    m.uses_ansi_nulls AS UsesAnsiNulls,
    m.uses_quoted_identifier AS UsesQuotedIdentifier
FROM sys.procedures AS p
INNER JOIN sys.schemas AS s ON s.schema_id = p.schema_id
INNER JOIN sys.sql_modules AS m ON m.object_id = p.object_id
WHERE p.is_ms_shipped = 0
UNION ALL
SELECT
    N'V', s.name, v.name, NULL, NULL, m.definition,
    m.uses_ansi_nulls, m.uses_quoted_identifier
FROM sys.views AS v
INNER JOIN sys.schemas AS s ON s.schema_id = v.schema_id
INNER JOIN sys.sql_modules AS m ON m.object_id = v.object_id
WHERE v.is_ms_shipped = 0
UNION ALL
SELECT
    N'TR', OBJECT_SCHEMA_NAME(tr.object_id), tr.name,
    OBJECT_SCHEMA_NAME(tr.parent_id), OBJECT_NAME(tr.parent_id), m.definition,
    m.uses_ansi_nulls, m.uses_quoted_identifier
FROM sys.triggers AS tr
INNER JOIN sys.sql_modules AS m ON m.object_id = tr.object_id
WHERE tr.is_ms_shipped = 0
ORDER BY ObjectType, CurrentSchema, CurrentName;
'@

$invokeArgs = @{
    ServerInstance = $server
    Database = 'KMTGuard'
    Username = [string]$settings.Username
    Password = [string]$settings.Password
    QueryTimeout = 120
    MaxCharLength = 1048576
}
$tables = @(Invoke-Sqlcmd @invokeArgs -Query $tableQuery)
$modules = @(Invoke-Sqlcmd @invokeArgs -Query $moduleQuery)
$procedures = @($modules | Where-Object ObjectType -eq 'P')
$views = @($modules | Where-Object ObjectType -eq 'V')
$triggers = @($modules | Where-Object ObjectType -eq 'TR')

$tableKeys = @($tableMap | ForEach-Object { "$($_.OldSchema).$($_.OldName)" })
$databaseTableKeys = @($tables | ForEach-Object { "$($_.CurrentSchema).$($_.CurrentName)" })
$procedureKeys = @($procedureMap | ForEach-Object { "$($_.CurrentSchema).$($_.CurrentName)" })
$databaseProcedureKeys = @($procedures | ForEach-Object { "$($_.CurrentSchema).$($_.CurrentName)" })
$tableDifferences = @(Compare-Object $tableKeys $databaseTableKeys)
$procedureDifferences = @(Compare-Object $procedureKeys $databaseProcedureKeys)
if ($tableDifferences.Count -or $procedureDifferences.Count) {
    $details = @($tableDifferences + $procedureDifferences | ForEach-Object { "$($_.SideIndicator) $($_.InputObject)" })
    throw "The maps do not exactly match the current KMTGuard objects:`n$($details -join "`n")"
}

$procedureLookup = @{}
foreach ($procedure in $procedures) {
    $procedureLookup["$($procedure.CurrentSchema).$($procedure.CurrentName)"] = $procedure
}

$backup = [System.Text.StringBuilder]::new()
[void]$backup.AppendLine('USE [KMTGuard];')
[void]$backup.AppendLine('GO')
[void]$backup.AppendLine('-- Generated before simplifying KMTGuard table and procedure names.')
[void]$backup.AppendLine('-- The verified COPY_ONLY database backup is the authoritative rollback source.')
[void]$backup.AppendLine()
foreach ($module in $modules) {
    [void]$backup.AppendLine($(if ([bool]$module.UsesAnsiNulls) { 'SET ANSI_NULLS ON;' } else { 'SET ANSI_NULLS OFF;' }))
    [void]$backup.AppendLine('GO')
    [void]$backup.AppendLine($(if ([bool]$module.UsesQuotedIdentifier) { 'SET QUOTED_IDENTIFIER ON;' } else { 'SET QUOTED_IDENTIFIER OFF;' }))
    [void]$backup.AppendLine('GO')
    [void]$backup.AppendLine([string]$module.Definition)
    [void]$backup.AppendLine('GO')
    [void]$backup.AppendLine()
}

$migration = [System.Text.StringBuilder]::new()
[void]$migration.AppendLine('USE [KMTGuard];')
[void]$migration.AppendLine('GO')
[void]$migration.AppendLine('SET NOCOUNT ON;')
[void]$migration.AppendLine('SET XACT_ABORT ON;')
[void]$migration.AppendLine('GO')
[void]$migration.AppendLine('-- Simplifies every KMTGuard table and procedure name without changing any other database.')
[void]$migration.AppendLine('-- Object IDs and table data are preserved; old names remain compatibility synonyms.')
[void]$migration.AppendLine('CREATE TABLE #KmtTableMap')
[void]$migration.AppendLine('(')
[void]$migration.AppendLine('    OldSchema SYSNAME COLLATE DATABASE_DEFAULT NOT NULL,')
[void]$migration.AppendLine('    OldName SYSNAME COLLATE DATABASE_DEFAULT NOT NULL,')
[void]$migration.AppendLine('    NewSchema SYSNAME COLLATE DATABASE_DEFAULT NOT NULL,')
[void]$migration.AppendLine('    NewName SYSNAME COLLATE DATABASE_DEFAULT NOT NULL,')
[void]$migration.AppendLine('    PRIMARY KEY (OldSchema, OldName),')
[void]$migration.AppendLine('    UNIQUE (NewSchema, NewName)')
[void]$migration.AppendLine(');')
[void]$migration.AppendLine('INSERT INTO #KmtTableMap (OldSchema, OldName, NewSchema, NewName) VALUES')
Append-MapValues $migration $tableMap @('OldSchema', 'OldName', 'NewSchema', 'NewName')
[void]$migration.AppendLine()
[void]$migration.AppendLine('CREATE TABLE #KmtProcedureMap')
[void]$migration.AppendLine('(')
[void]$migration.AppendLine('    CurrentSchema SYSNAME COLLATE DATABASE_DEFAULT NOT NULL,')
[void]$migration.AppendLine('    CurrentName SYSNAME COLLATE DATABASE_DEFAULT NOT NULL,')
[void]$migration.AppendLine('    NewSchema SYSNAME COLLATE DATABASE_DEFAULT NOT NULL,')
[void]$migration.AppendLine('    NewName SYSNAME COLLATE DATABASE_DEFAULT NOT NULL,')
[void]$migration.AppendLine('    PRIMARY KEY (CurrentSchema, CurrentName),')
[void]$migration.AppendLine('    UNIQUE (NewSchema, NewName)')
[void]$migration.AppendLine(');')
[void]$migration.AppendLine('INSERT INTO #KmtProcedureMap (CurrentSchema, CurrentName, NewSchema, NewName) VALUES')
Append-MapValues $migration $procedureMap @('CurrentSchema', 'CurrentName', 'NewSchema', 'NewName')
[void]$migration.AppendLine()
[void]$migration.AppendLine('CREATE TABLE #KmtLegacyProcedureMap')
[void]$migration.AppendLine('(')
[void]$migration.AppendLine('    OldSchema SYSNAME COLLATE DATABASE_DEFAULT NOT NULL,')
[void]$migration.AppendLine('    OldName SYSNAME COLLATE DATABASE_DEFAULT NOT NULL,')
[void]$migration.AppendLine('    NewSchema SYSNAME COLLATE DATABASE_DEFAULT NOT NULL,')
[void]$migration.AppendLine('    NewName SYSNAME COLLATE DATABASE_DEFAULT NOT NULL,')
[void]$migration.AppendLine('    PRIMARY KEY (OldSchema, OldName)')
[void]$migration.AppendLine(');')
[void]$migration.AppendLine('INSERT INTO #KmtLegacyProcedureMap (OldSchema, OldName, NewSchema, NewName) VALUES')
Append-MapValues $migration @($legacyMap) @('OldSchema', 'OldName', 'NewSchema', 'NewName')
[void]$migration.AppendLine()
[void]$migration.AppendLine('BEGIN TRY')
[void]$migration.AppendLine('    BEGIN TRANSACTION;')
[void]$migration.AppendLine()
[void]$migration.AppendLine('    IF SCHEMA_ID(N''KMT'') IS NULL')
[void]$migration.AppendLine('        EXEC sys.sp_executesql N''CREATE SCHEMA [KMT] AUTHORIZATION [dbo];'';')
[void]$migration.AppendLine()
[void]$migration.AppendLine('    CREATE TABLE #BeforeTables')
[void]$migration.AppendLine('    (')
[void]$migration.AppendLine('        NewSchema SYSNAME COLLATE DATABASE_DEFAULT NOT NULL,')
[void]$migration.AppendLine('        NewName SYSNAME COLLATE DATABASE_DEFAULT NOT NULL,')
[void]$migration.AppendLine('        ObjectId INT NOT NULL, SavedRows BIGINT NOT NULL, ColumnCount INT NOT NULL, IndexCount INT NOT NULL,')
[void]$migration.AppendLine('        PRIMARY KEY (NewSchema, NewName)')
[void]$migration.AppendLine('    );')
[void]$migration.AppendLine('    INSERT INTO #BeforeTables (NewSchema, NewName, ObjectId, SavedRows, ColumnCount, IndexCount)')
[void]$migration.AppendLine('    SELECT m.NewSchema, m.NewName, ids.ObjectId,')
[void]$migration.AppendLine('           COALESCE((SELECT SUM(ps.row_count) FROM sys.dm_db_partition_stats AS ps WHERE ps.object_id = ids.ObjectId AND ps.index_id IN (0,1)), 0),')
[void]$migration.AppendLine('           (SELECT COUNT(*) FROM sys.columns AS c WHERE c.object_id = ids.ObjectId),')
[void]$migration.AppendLine('           (SELECT COUNT(*) FROM sys.indexes AS i WHERE i.object_id = ids.ObjectId AND i.index_id > 0)')
[void]$migration.AppendLine('    FROM #KmtTableMap AS m')
[void]$migration.AppendLine('    CROSS APPLY (VALUES (COALESCE(OBJECT_ID(QUOTENAME(m.NewSchema)+N''.''+QUOTENAME(m.NewName), N''U''), OBJECT_ID(QUOTENAME(m.OldSchema)+N''.''+QUOTENAME(m.OldName), N''U'')))) AS ids(ObjectId)')
[void]$migration.AppendLine('    WHERE ids.ObjectId IS NOT NULL;')
[void]$migration.AppendLine('    IF (SELECT COUNT(*) FROM #BeforeTables) <> (SELECT COUNT(*) FROM #KmtTableMap)')
[void]$migration.AppendLine('        THROW 52000, ''A mapped source table is missing.'', 1;')
[void]$migration.AppendLine()
[void]$migration.AppendLine('    CREATE TABLE #BeforeProcedures (NewSchema SYSNAME COLLATE DATABASE_DEFAULT, NewName SYSNAME COLLATE DATABASE_DEFAULT, ObjectId INT NOT NULL, PRIMARY KEY (NewSchema, NewName));')
[void]$migration.AppendLine('    INSERT INTO #BeforeProcedures (NewSchema, NewName, ObjectId)')
[void]$migration.AppendLine('    SELECT m.NewSchema, m.NewName, ids.ObjectId')
[void]$migration.AppendLine('    FROM #KmtProcedureMap AS m')
[void]$migration.AppendLine('    CROSS APPLY (VALUES (COALESCE(OBJECT_ID(QUOTENAME(m.NewSchema)+N''.''+QUOTENAME(m.NewName), N''P''), OBJECT_ID(QUOTENAME(m.CurrentSchema)+N''.''+QUOTENAME(m.CurrentName), N''P'')))) AS ids(ObjectId)')
[void]$migration.AppendLine('    WHERE ids.ObjectId IS NOT NULL;')
[void]$migration.AppendLine('    IF (SELECT COUNT(*) FROM #BeforeProcedures) <> (SELECT COUNT(*) FROM #KmtProcedureMap)')
[void]$migration.AppendLine('        THROW 52001, ''A mapped source procedure is missing.'', 1;')
[void]$migration.AppendLine()
[void]$migration.AppendLine('    DECLARE @OldSchema SYSNAME, @OldName SYSNAME, @NewSchema SYSNAME, @NewName SYSNAME;')
[void]$migration.AppendLine('    DECLARE @OldObject NVARCHAR(517), @RenamedObject NVARCHAR(517), @NewObject NVARCHAR(517), @Sql NVARCHAR(MAX);')
[void]$migration.AppendLine('    DECLARE TableCursor CURSOR LOCAL FAST_FORWARD FOR SELECT OldSchema, OldName, NewSchema, NewName FROM #KmtTableMap ORDER BY OldSchema, OldName;')
[void]$migration.AppendLine('    OPEN TableCursor;')
[void]$migration.AppendLine('    FETCH NEXT FROM TableCursor INTO @OldSchema, @OldName, @NewSchema, @NewName;')
[void]$migration.AppendLine('    WHILE @@FETCH_STATUS = 0')
[void]$migration.AppendLine('    BEGIN')
[void]$migration.AppendLine('        SET @OldObject = QUOTENAME(@OldSchema)+N''.''+QUOTENAME(@OldName);')
[void]$migration.AppendLine('        SET @NewObject = QUOTENAME(@NewSchema)+N''.''+QUOTENAME(@NewName);')
[void]$migration.AppendLine('        IF OBJECT_ID(@NewObject, N''U'') IS NULL')
[void]$migration.AppendLine('        BEGIN')
[void]$migration.AppendLine('            IF OBJECT_ID(@OldObject, N''U'') IS NULL THROW 52002, ''A source table disappeared during migration.'', 1;')
[void]$migration.AppendLine('            EXEC sys.sp_rename @objname=@OldObject, @newname=@NewName, @objtype=N''OBJECT'';')
[void]$migration.AppendLine('            SET @RenamedObject = QUOTENAME(@OldSchema)+N''.''+QUOTENAME(@NewName);')
[void]$migration.AppendLine('            SET @Sql = N''ALTER SCHEMA ''+QUOTENAME(@NewSchema)+N'' TRANSFER ''+@RenamedObject+N'';'';')
[void]$migration.AppendLine('            EXEC sys.sp_executesql @Sql;')
[void]$migration.AppendLine('        END;')
[void]$migration.AppendLine('        FETCH NEXT FROM TableCursor INTO @OldSchema, @OldName, @NewSchema, @NewName;')
[void]$migration.AppendLine('    END;')
[void]$migration.AppendLine('    CLOSE TableCursor; DEALLOCATE TableCursor;')
[void]$migration.AppendLine()
[void]$migration.AppendLine('    DECLARE ProcedureCursor CURSOR LOCAL FAST_FORWARD FOR SELECT CurrentSchema, CurrentName, NewSchema, NewName FROM #KmtProcedureMap ORDER BY CurrentSchema, CurrentName;')
[void]$migration.AppendLine('    OPEN ProcedureCursor;')
[void]$migration.AppendLine('    FETCH NEXT FROM ProcedureCursor INTO @OldSchema, @OldName, @NewSchema, @NewName;')
[void]$migration.AppendLine('    WHILE @@FETCH_STATUS = 0')
[void]$migration.AppendLine('    BEGIN')
[void]$migration.AppendLine('        SET @OldObject = QUOTENAME(@OldSchema)+N''.''+QUOTENAME(@OldName);')
[void]$migration.AppendLine('        SET @NewObject = QUOTENAME(@NewSchema)+N''.''+QUOTENAME(@NewName);')
[void]$migration.AppendLine('        IF OBJECT_ID(@NewObject, N''P'') IS NULL')
[void]$migration.AppendLine('        BEGIN')
[void]$migration.AppendLine('            IF OBJECT_ID(@OldObject, N''P'') IS NULL THROW 52003, ''A source procedure disappeared during migration.'', 1;')
[void]$migration.AppendLine('            EXEC sys.sp_rename @objname=@OldObject, @newname=@NewName, @objtype=N''OBJECT'';')
[void]$migration.AppendLine('            SET @RenamedObject = QUOTENAME(@OldSchema)+N''.''+QUOTENAME(@NewName);')
[void]$migration.AppendLine('            SET @Sql = N''ALTER SCHEMA ''+QUOTENAME(@NewSchema)+N'' TRANSFER ''+@RenamedObject+N'';'';')
[void]$migration.AppendLine('            EXEC sys.sp_executesql @Sql;')
[void]$migration.AppendLine('        END;')
[void]$migration.AppendLine('        FETCH NEXT FROM ProcedureCursor INTO @OldSchema, @OldName, @NewSchema, @NewName;')
[void]$migration.AppendLine('    END;')
[void]$migration.AppendLine('    CLOSE ProcedureCursor; DEALLOCATE ProcedureCursor;')
[void]$migration.AppendLine()

foreach ($entry in $procedureMap) {
    $procedure = $procedureLookup["$($entry.CurrentSchema).$($entry.CurrentName)"]
    $definition = Rewrite-ProcedureHeader ([string]$procedure.Definition) $entry.CurrentSchema $entry.CurrentName $entry.NewSchema $entry.NewName
    $definition = Rewrite-TableReferences $definition $tableMap
    $definition = Rewrite-ExecReferences $definition $procedureMap $legacyMap
    [void]$migration.AppendLine($(if ([bool]$procedure.UsesAnsiNulls) { '    SET ANSI_NULLS ON;' } else { '    SET ANSI_NULLS OFF;' }))
    [void]$migration.AppendLine($(if ([bool]$procedure.UsesQuotedIdentifier) { '    SET QUOTED_IDENTIFIER ON;' } else { '    SET QUOTED_IDENTIFIER OFF;' }))
    [void]$migration.AppendLine('    EXEC sys.sp_executesql ' + (Quote-SqlLiteral $definition) + ';')
    [void]$migration.AppendLine()
}

$rewrittenViewCount = 0
foreach ($view in $views) {
    $originalDefinition = [string]$view.Definition
    $definition = Rewrite-TableReferences $originalDefinition $tableMap
    if ($definition -ceq $originalDefinition) {
        continue
    }
    $definition = Rewrite-ViewHeader $definition ([string]$view.CurrentSchema) ([string]$view.CurrentName)
    $rewrittenViewCount++
    [void]$migration.AppendLine($(if ([bool]$view.UsesAnsiNulls) { '    SET ANSI_NULLS ON;' } else { '    SET ANSI_NULLS OFF;' }))
    [void]$migration.AppendLine($(if ([bool]$view.UsesQuotedIdentifier) { '    SET QUOTED_IDENTIFIER ON;' } else { '    SET QUOTED_IDENTIFIER OFF;' }))
    [void]$migration.AppendLine('    EXEC sys.sp_executesql ' + (Quote-SqlLiteral $definition) + ';')
    [void]$migration.AppendLine()
}

foreach ($trigger in $triggers) {
    $parentMap = @($tableMap | Where-Object { $_.OldSchema -eq $trigger.ParentSchema -and $_.OldName -eq $trigger.ParentName })
    if ($parentMap.Count -ne 1) {
        throw "Could not map trigger parent $($trigger.ParentSchema).$($trigger.ParentName)"
    }
    $definition = Rewrite-TriggerHeader ([string]$trigger.Definition) ([string]$parentMap[0].NewSchema) ([string]$trigger.CurrentName)
    $definition = Rewrite-TableReferences $definition $tableMap
    $definition = Rewrite-ExecReferences $definition $procedureMap $legacyMap
    [void]$migration.AppendLine($(if ([bool]$trigger.UsesAnsiNulls) { '    SET ANSI_NULLS ON;' } else { '    SET ANSI_NULLS OFF;' }))
    [void]$migration.AppendLine($(if ([bool]$trigger.UsesQuotedIdentifier) { '    SET QUOTED_IDENTIFIER ON;' } else { '    SET QUOTED_IDENTIFIER OFF;' }))
    [void]$migration.AppendLine('    EXEC sys.sp_executesql ' + (Quote-SqlLiteral $definition) + ';')
    [void]$migration.AppendLine()
}

[void]$migration.AppendLine('    DECLARE TableAliasCursor CURSOR LOCAL FAST_FORWARD FOR SELECT OldSchema, OldName, NewSchema, NewName FROM #KmtTableMap;')
[void]$migration.AppendLine('    OPEN TableAliasCursor; FETCH NEXT FROM TableAliasCursor INTO @OldSchema, @OldName, @NewSchema, @NewName;')
[void]$migration.AppendLine('    WHILE @@FETCH_STATUS = 0')
[void]$migration.AppendLine('    BEGIN')
[void]$migration.AppendLine('        SET @OldObject=QUOTENAME(@OldSchema)+N''.''+QUOTENAME(@OldName); SET @NewObject=QUOTENAME(DB_NAME())+N''.''+QUOTENAME(@NewSchema)+N''.''+QUOTENAME(@NewName);')
[void]$migration.AppendLine('        IF EXISTS (SELECT 1 FROM sys.synonyms sy JOIN sys.schemas s ON s.schema_id=sy.schema_id WHERE s.name=@OldSchema AND sy.name=@OldName) BEGIN SET @Sql=N''DROP SYNONYM ''+@OldObject+N'';''; EXEC sys.sp_executesql @Sql; END;')
[void]$migration.AppendLine('        IF EXISTS (SELECT 1 FROM sys.objects o JOIN sys.schemas s ON s.schema_id=o.schema_id WHERE s.name=@OldSchema AND o.name=@OldName) THROW 52004, ''A table compatibility name is occupied.'', 1;')
[void]$migration.AppendLine('        SET @Sql=N''CREATE SYNONYM ''+@OldObject+N'' FOR ''+@NewObject+N'';''; EXEC sys.sp_executesql @Sql;')
[void]$migration.AppendLine('        FETCH NEXT FROM TableAliasCursor INTO @OldSchema, @OldName, @NewSchema, @NewName;')
[void]$migration.AppendLine('    END;')
[void]$migration.AppendLine('    CLOSE TableAliasCursor; DEALLOCATE TableAliasCursor;')
[void]$migration.AppendLine()
[void]$migration.AppendLine('    DECLARE ProcedureAliasCursor CURSOR LOCAL FAST_FORWARD FOR SELECT CurrentSchema, CurrentName, NewSchema, NewName FROM #KmtProcedureMap;')
[void]$migration.AppendLine('    OPEN ProcedureAliasCursor; FETCH NEXT FROM ProcedureAliasCursor INTO @OldSchema, @OldName, @NewSchema, @NewName;')
[void]$migration.AppendLine('    WHILE @@FETCH_STATUS = 0')
[void]$migration.AppendLine('    BEGIN')
[void]$migration.AppendLine('        SET @OldObject=QUOTENAME(@OldSchema)+N''.''+QUOTENAME(@OldName); SET @NewObject=QUOTENAME(DB_NAME())+N''.''+QUOTENAME(@NewSchema)+N''.''+QUOTENAME(@NewName);')
[void]$migration.AppendLine('        IF EXISTS (SELECT 1 FROM sys.synonyms sy JOIN sys.schemas s ON s.schema_id=sy.schema_id WHERE s.name=@OldSchema AND sy.name=@OldName) BEGIN SET @Sql=N''DROP SYNONYM ''+@OldObject+N'';''; EXEC sys.sp_executesql @Sql; END;')
[void]$migration.AppendLine('        IF EXISTS (SELECT 1 FROM sys.objects o JOIN sys.schemas s ON s.schema_id=o.schema_id WHERE s.name=@OldSchema AND o.name=@OldName) THROW 52005, ''A procedure compatibility name is occupied.'', 1;')
[void]$migration.AppendLine('        SET @Sql=N''CREATE SYNONYM ''+@OldObject+N'' FOR ''+@NewObject+N'';''; EXEC sys.sp_executesql @Sql;')
[void]$migration.AppendLine('        FETCH NEXT FROM ProcedureAliasCursor INTO @OldSchema, @OldName, @NewSchema, @NewName;')
[void]$migration.AppendLine('    END;')
[void]$migration.AppendLine('    CLOSE ProcedureAliasCursor; DEALLOCATE ProcedureAliasCursor;')
[void]$migration.AppendLine()
[void]$migration.AppendLine('    DECLARE LegacyAliasCursor CURSOR LOCAL FAST_FORWARD FOR SELECT OldSchema, OldName, NewSchema, NewName FROM #KmtLegacyProcedureMap;')
[void]$migration.AppendLine('    OPEN LegacyAliasCursor; FETCH NEXT FROM LegacyAliasCursor INTO @OldSchema, @OldName, @NewSchema, @NewName;')
[void]$migration.AppendLine('    WHILE @@FETCH_STATUS = 0')
[void]$migration.AppendLine('    BEGIN')
[void]$migration.AppendLine('        SET @OldObject=QUOTENAME(@OldSchema)+N''.''+QUOTENAME(@OldName); SET @NewObject=QUOTENAME(DB_NAME())+N''.''+QUOTENAME(@NewSchema)+N''.''+QUOTENAME(@NewName);')
[void]$migration.AppendLine('        IF EXISTS (SELECT 1 FROM sys.synonyms sy JOIN sys.schemas s ON s.schema_id=sy.schema_id WHERE s.name=@OldSchema AND sy.name=@OldName) BEGIN SET @Sql=N''DROP SYNONYM ''+@OldObject+N'';''; EXEC sys.sp_executesql @Sql; END;')
[void]$migration.AppendLine('        IF EXISTS (SELECT 1 FROM sys.objects o JOIN sys.schemas s ON s.schema_id=o.schema_id WHERE s.name=@OldSchema AND o.name=@OldName) THROW 52006, ''An original compatibility name is occupied.'', 1;')
[void]$migration.AppendLine('        SET @Sql=N''CREATE SYNONYM ''+@OldObject+N'' FOR ''+@NewObject+N'';''; EXEC sys.sp_executesql @Sql;')
[void]$migration.AppendLine('        FETCH NEXT FROM LegacyAliasCursor INTO @OldSchema, @OldName, @NewSchema, @NewName;')
[void]$migration.AppendLine('    END;')
[void]$migration.AppendLine('    CLOSE LegacyAliasCursor; DEALLOCATE LegacyAliasCursor;')
[void]$migration.AppendLine()
[void]$migration.AppendLine('    IF EXISTS (SELECT 1 FROM #BeforeTables b WHERE OBJECT_ID(QUOTENAME(b.NewSchema)+N''.''+QUOTENAME(b.NewName), N''U'') <> b.ObjectId) THROW 52007, ''A table object ID changed.'', 1;')
[void]$migration.AppendLine('    IF EXISTS (SELECT 1 FROM #BeforeTables b WHERE b.SavedRows <> COALESCE((SELECT SUM(ps.row_count) FROM sys.dm_db_partition_stats ps WHERE ps.object_id=b.ObjectId AND ps.index_id IN (0,1)),0)) THROW 52008, ''A table row count changed.'', 1;')
[void]$migration.AppendLine('    IF EXISTS (SELECT 1 FROM #BeforeTables b WHERE b.ColumnCount <> (SELECT COUNT(*) FROM sys.columns c WHERE c.object_id=b.ObjectId) OR b.IndexCount <> (SELECT COUNT(*) FROM sys.indexes i WHERE i.object_id=b.ObjectId AND i.index_id>0)) THROW 52009, ''Table metadata changed unexpectedly.'', 1;')
[void]$migration.AppendLine('    IF EXISTS (SELECT 1 FROM #BeforeProcedures b WHERE OBJECT_ID(QUOTENAME(b.NewSchema)+N''.''+QUOTENAME(b.NewName), N''P'') <> b.ObjectId) THROW 52010, ''A procedure object ID changed.'', 1;')
[void]$migration.AppendLine('    IF (SELECT COUNT(*) FROM #KmtTableMap m JOIN sys.schemas s ON s.name=m.OldSchema JOIN sys.synonyms sy ON sy.schema_id=s.schema_id AND sy.name=m.OldName) <> (SELECT COUNT(*) FROM #KmtTableMap) THROW 52011, ''Table compatibility validation failed.'', 1;')
[void]$migration.AppendLine('    IF (SELECT COUNT(*) FROM #KmtProcedureMap m JOIN sys.schemas s ON s.name=m.CurrentSchema JOIN sys.synonyms sy ON sy.schema_id=s.schema_id AND sy.name=m.CurrentName) <> (SELECT COUNT(*) FROM #KmtProcedureMap) THROW 52012, ''Procedure compatibility validation failed.'', 1;')
[void]$migration.AppendLine('    IF (SELECT COUNT(*) FROM #KmtLegacyProcedureMap m JOIN sys.schemas s ON s.name=m.OldSchema JOIN sys.synonyms sy ON sy.schema_id=s.schema_id AND sy.name=m.OldName) <> (SELECT COUNT(*) FROM #KmtLegacyProcedureMap) THROW 52013, ''Original compatibility validation failed.'', 1;')
[void]$migration.AppendLine()
[void]$migration.AppendLine('    COMMIT TRANSACTION;')
[void]$migration.AppendLine('END TRY')
[void]$migration.AppendLine('BEGIN CATCH')
[void]$migration.AppendLine('    IF @@TRANCOUNT > 0 ROLLBACK TRANSACTION;')
[void]$migration.AppendLine('    THROW;')
[void]$migration.AppendLine('END CATCH;')
[void]$migration.AppendLine('GO')
[void]$migration.AppendLine('DROP TABLE #KmtLegacyProcedureMap; DROP TABLE #KmtProcedureMap; DROP TABLE #KmtTableMap;')
[void]$migration.AppendLine('GO')
[void]$migration.AppendLine("PRINT 'KMTGuard names simplified: $($tableMap.Count) tables and $($procedureMap.Count) procedures.';")
[void]$migration.AppendLine('GO')

$utf8WithoutBom = [System.Text.UTF8Encoding]::new($false)
[System.IO.Directory]::CreateDirectory((Split-Path -Parent $MigrationPath)) | Out-Null
[System.IO.Directory]::CreateDirectory((Split-Path -Parent $DefinitionsBackupPath)) | Out-Null
[System.IO.File]::WriteAllText($MigrationPath, $migration.ToString(), $utf8WithoutBom)
[System.IO.File]::WriteAllText($DefinitionsBackupPath, $backup.ToString(), $utf8WithoutBom)

Write-Output "Generated migration: $MigrationPath"
Write-Output "Generated definitions backup: $DefinitionsBackupPath"
Write-Output "Mapped tables: $($tableMap.Count)"
Write-Output "Mapped procedures: $($procedureMap.Count)"
Write-Output "Captured views: $($views.Count)"
Write-Output "Rewritten views: $rewrittenViewCount"
Write-Output "Captured triggers: $($triggers.Count)"
