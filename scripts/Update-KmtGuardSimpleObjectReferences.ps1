[CmdletBinding()]
param(
    [string]$TableMapPath = 'D:\Filter-Venom\database\kmtguard_simple_table_name_map.csv',
    [string]$ProcedureMapPath = 'D:\Filter-Venom\database\kmtguard_simple_procedure_name_map.csv',
    [string[]]$Roots = @(
        'D:\Filter-Venom\filter\KMTGuardnew\KMTGuard',
        'D:\Filter-Venom\KMTGuard.AdminDesktop',
        'D:\Filter-Venom\gameserver\source',
        'D:\Filter-Venom\ShardManager\vSRO-ShardManager'
    )
)

$ErrorActionPreference = 'Stop'
Set-StrictMode -Version Latest

function Quote-SqlIdentifier([string]$value) {
    return '[' + $value.Replace(']', ']]') + ']'
}

function Read-SourceText([string]$path) {
    $bytes = [System.IO.File]::ReadAllBytes($path)
    $encoding = $null
    $preambleLength = 0

    if ($bytes.Length -ge 3 -and $bytes[0] -eq 0xEF -and $bytes[1] -eq 0xBB -and $bytes[2] -eq 0xBF) {
        $encoding = [System.Text.UTF8Encoding]::new($true, $true)
        $preambleLength = 3
    }
    elseif ($bytes.Length -ge 2 -and $bytes[0] -eq 0xFF -and $bytes[1] -eq 0xFE) {
        $encoding = [System.Text.Encoding]::Unicode
        $preambleLength = 2
    }
    elseif ($bytes.Length -ge 2 -and $bytes[0] -eq 0xFE -and $bytes[1] -eq 0xFF) {
        $encoding = [System.Text.Encoding]::BigEndianUnicode
        $preambleLength = 2
    }
    else {
        try {
            $encoding = [System.Text.UTF8Encoding]::new($false, $true)
            [void]$encoding.GetString($bytes)
        }
        catch {
            # ISO-8859-1 is byte preserving for old native source files.
            $encoding = [System.Text.Encoding]::GetEncoding(28591)
        }
    }

    $text = $encoding.GetString($bytes, $preambleLength, $bytes.Length - $preambleLength)
    return [pscustomobject]@{
        Text = $text
        Encoding = $encoding
        HasPreamble = ($preambleLength -gt 0)
    }
}

function Write-SourceText([string]$path, [string]$text, $source) {
    $body = $source.Encoding.GetBytes($text)
    if (-not $source.HasPreamble) {
        [System.IO.File]::WriteAllBytes($path, $body)
        return
    }

    $preamble = $source.Encoding.GetPreamble()
    $bytes = [byte[]]::new($preamble.Length + $body.Length)
    [Array]::Copy($preamble, 0, $bytes, 0, $preamble.Length)
    [Array]::Copy($body, 0, $bytes, $preamble.Length, $body.Length)
    [System.IO.File]::WriteAllBytes($path, $bytes)
}

function Rewrite-SqlString([string]$value, [array]$tableMap, [array]$procedureMap) {
    if (-not $script:CandidateRegex.IsMatch($value)) {
        return $value
    }

    $rewritten = $value
    $sqlLike = $value -match '(?i)\b(?:SELECT|FROM|JOIN|UPDATE|INSERT|INTO|DELETE|MERGE|CREATE|ALTER|DROP|TRUNCATE|OBJECT_ID|COL_LENGTH|EXEC|EXECUTE|TABLE|WITH|VALUES)\b'

    foreach ($entry in ($tableMap | Sort-Object { $_.OldName.Length } -Descending)) {
        $oldName = [regex]::Escape([string]$entry.OldName)
        $oldSchema = [regex]::Escape([string]$entry.OldSchema)
        $token = "(?:\[$oldName\]|$oldName)(?![A-Za-z0-9_])"
        $boundary = '(?<![A-Za-z0-9_\].])'
        $localTarget = (Quote-SqlIdentifier $entry.NewSchema) + '.' + (Quote-SqlIdentifier $entry.NewName)
        $databaseTarget = '[KMTGuard].' + $localTarget

        $rewritten = [regex]::Replace(
            $rewritten,
            "(?i)$boundary(?:\[?KMTGuard\]?)\s*\.\s*(?:\[?$oldSchema\]?)\s*\.\s*$token",
            $databaseTarget)
        $rewritten = [regex]::Replace(
            $rewritten,
            "(?i)$boundary(?:\[?KMTGuard\]?)\s*\.\s*\.\s*$token",
            $databaseTarget)
        $rewritten = [regex]::Replace(
            $rewritten,
            "(?i)$boundary(?:\[?$oldSchema\]?)\s*\.\s*$token",
            $localTarget)

        if ($sqlLike) {
            $rewritten = [regex]::Replace($rewritten, "(?i)$boundary$token", $localTarget)
        }
    }

    foreach ($entry in ($procedureMap | Sort-Object { $_.CurrentName.Length } -Descending)) {
        $schema = [regex]::Escape([string]$entry.CurrentSchema)
        $name = [regex]::Escape([string]$entry.CurrentName)
        $token = "(?:\[$name\]|$name)(?![A-Za-z0-9_])"
        $boundary = '(?<![A-Za-z0-9_\].])'
        $localTarget = (Quote-SqlIdentifier $entry.NewSchema) + '.' + (Quote-SqlIdentifier $entry.NewName)
        $databaseTarget = '[KMTGuard].' + $localTarget

        $rewritten = [regex]::Replace(
            $rewritten,
            "(?i)$boundary(?:\[?KMTGuard\]?)\s*\.\s*(?:\[?$schema\]?)\s*\.\s*$token",
            $databaseTarget)
        $rewritten = [regex]::Replace(
            $rewritten,
            "(?i)$boundary(?:\[?$schema\]?)\s*\.\s*$token",
            $localTarget)
        $rewritten = [regex]::Replace(
            $rewritten,
            "(?im)(?<exec>\bEXEC(?:UTE)?\s+(?:(?:\[?@[A-Za-z0-9_]+\]?|@[A-Za-z0-9_]+)\s*=\s*)?)$token",
            '${exec}' + $localTarget)
    }

    return $rewritten
}

function Rewrite-QuotedSource([string]$text, [array]$tableMap, [array]$procedureMap) {
    $result = [System.Text.StringBuilder]::new($text.Length)
    $last = 0
    $index = 0

    while ($index -lt $text.Length) {
        $current = $text[$index]

        if ($current -eq '/' -and $index + 1 -lt $text.Length -and $text[$index + 1] -eq '/') {
            $newline = $text.IndexOf("`n", $index + 2)
            if ($newline -lt 0) { break }
            $index = $newline + 1
            continue
        }
        if ($current -eq '/' -and $index + 1 -lt $text.Length -and $text[$index + 1] -eq '*') {
            $endComment = $text.IndexOf('*/', $index + 2, [StringComparison]::Ordinal)
            if ($endComment -lt 0) { break }
            $index = $endComment + 2
            continue
        }
        if ($current -eq "'") {
            $index++
            while ($index -lt $text.Length) {
                if ($text[$index] -eq '\') { $index += 2; continue }
                if ($text[$index] -eq "'") { $index++; break }
                $index++
            }
            continue
        }
        if ($current -ne '"') {
            $index++
            continue
        }

        $start = $index
        $isVerbatim = ($start -ge 1 -and $text[$start - 1] -eq '@') -or
            ($start -ge 2 -and ($text.Substring($start - 2, 2) -eq '@$' -or $text.Substring($start - 2, 2) -eq '$@'))
        $index++
        while ($index -lt $text.Length) {
            if ($text[$index] -ne '"') {
                if (-not $isVerbatim -and $text[$index] -eq '\') { $index += 2 } else { $index++ }
                continue
            }

            if ($isVerbatim -and $index + 1 -lt $text.Length -and $text[$index + 1] -eq '"') {
                $index += 2
                continue
            }

            $end = $index
            $segment = $text.Substring($start, $end - $start + 1)
            $updated = Rewrite-SqlString $segment $tableMap $procedureMap
            if ($updated -cne $segment) {
                [void]$result.Append($text.Substring($last, $start - $last))
                [void]$result.Append($updated)
                $last = $end + 1
            }
            $index = $end + 1
            break
        }
    }

    [void]$result.Append($text.Substring($last))
    return $result.ToString()
}

foreach ($path in @($TableMapPath, $ProcedureMapPath)) {
    if (-not (Test-Path -LiteralPath $path)) {
        throw "Required map was not found: $path"
    }
}

$tableMap = @(Import-Csv -LiteralPath $TableMapPath)
$procedureMap = @(Import-Csv -LiteralPath $ProcedureMapPath)
$candidateNames = @($tableMap.OldName) + @($procedureMap.CurrentName)
$orderedCandidateNames = @($candidateNames | Sort-Object -Unique | Sort-Object { $_.Length } -Descending)
$candidatePattern = '(?i)(?<![A-Za-z0-9_])(?:' + (($orderedCandidateNames | ForEach-Object { [regex]::Escape($_) }) -join '|') + ')(?![A-Za-z0-9_])'
$script:CandidateRegex = [regex]::new($candidatePattern, [System.Text.RegularExpressions.RegexOptions]::Compiled)
$extensions = @('.cs', '.cpp', '.c', '.h', '.hpp')
$excludedSegments = @('\bin\', '\obj\', '\publish', '\.vs\', '\build', '\RelWithDebInfo\', '\Release\', '\Outpus\')
$files = foreach ($root in $Roots) {
    if (-not (Test-Path -LiteralPath $root)) { continue }
    Get-ChildItem -LiteralPath $root -Recurse -File | Where-Object {
        $filePath = $_.FullName
        $extensions -contains $_.Extension.ToLowerInvariant() -and
        -not ($excludedSegments | Where-Object { $filePath.IndexOf($_, [StringComparison]::OrdinalIgnoreCase) -ge 0 })
    }
}

$changed = [System.Collections.Generic.List[string]]::new()
foreach ($file in ($files | Sort-Object FullName -Unique)) {
    $source = Read-SourceText $file.FullName
    if (-not $script:CandidateRegex.IsMatch($source.Text)) { continue }
    $updated = Rewrite-QuotedSource $source.Text $tableMap $procedureMap
    if ($updated -ceq $source.Text) { continue }
    Write-SourceText $file.FullName $updated $source
    $changed.Add($file.FullName)
}

Write-Output "Changed source files: $($changed.Count)"
$changed | ForEach-Object { Write-Output $_ }
