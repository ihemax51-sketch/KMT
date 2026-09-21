param(
    [switch]$Apply
)

$ErrorActionPreference = "Stop"

$repoRoot = Split-Path -Parent $PSScriptRoot
$clientSourceRoot = Join-Path $repoRoot "JTClientLibrary\source\libs\ClientLib\src"
$referenceTextUi = Join-Path $repoRoot "lang\textuisystem.txt"
$localizationRoot = Join-Path $repoRoot "JTClientLibrary\localization"
$kmtTextUi = Join-Path $localizationRoot "textuisystem_kmtguard.txt"
$kmtTextUiEnglish = Join-Path $localizationRoot "textuisystem_kmtguard_english.txt"
$kmtTextUiTurkish = Join-Path $localizationRoot "textuisystem_kmtguard_turkish.txt"
$turkishOverrides = Join-Path $localizationRoot "turkish_overrides.tsv"

$displayCallPattern = [regex]::new(
    "SetText|ShowMessage|WriteSystemMessage|StyleText|StyleStatic|" +
    "StyleButton|SetStatus|swprintf|SetMessage|SetPlaceholder|" +
    "ShowSystemMessage|SetTooltip|CreateStatic|CreateButton|SetFormattedText|" +
    "CreateLabel|ShowTitleMessage|ShowLogMessage|StyleMainFrame|WriteMessage|" +
    "StopAutomation|SetDescBoxText|ConfigureLegacyButton|AddGridButton|" +
    "SetupInventoryHeading|MessageBoxW")
$displayStoragePattern = [regex]::new(
    'std::n_wstring\s+(?:msg\w*|category|hwan\d+|sec\d+|ret\d+|rep\d+|' +
    'Text\d+|EmojiList|strmsg\d+|mymsg\d+|GlowType|ModelType)\b|' +
    '\b(?:aa|aax|aaxx|aaxxc|aaxxcc|aaxxccc|aaxxccx|aaxxcccc)\.Name\s*=|' +
    '\baa\.Code\s*=')
$wideLiteralPattern = [regex]::new('L"((?:[^"\\]|\\.)*)"')
$formatPattern = [regex]::new(
    '%(?:[-+ #0]*)(?:\d+|\*)?(?:\.(?:\d+|\*))?' +
    '(?:hh|h|ll|l|I32|I64|w|L)?[cCdiouxXeEfgGaAnpsSZ]')

function Test-IsDisplayText {
    param([string]$Value)

    if ([string]::IsNullOrWhiteSpace($Value)) {
        return $false
    }
    if ($Value -match '\\|\.(ddj|dll|json|txt|bsr|dat)$') {
        return $false
    }
    if ($Value -match '^(NATTR_)') {
        return $false
    }
    if ($Value -match '^(F\d+|SP\+\d+|:[0-9]{2})$') {
        return $false
    }

    if ($Value -match '^(UIIT_|UIO_|VFILTER_|LEXA_|PARAM_)') {
        return $true
    }

    $probe = $formatPattern.Replace($Value, "")
    $probe = $probe -replace '\\[nrt]', ' '
    return $probe -match '[A-Za-z]{2}'
}

function Get-StableHash {
    param([string]$Value)

    $sha = [System.Security.Cryptography.SHA256]::Create()
    try {
        $bytes = [System.Text.Encoding]::UTF8.GetBytes($Value)
        $hash = $sha.ComputeHash($bytes)
        return ([System.BitConverter]::ToString($hash) -replace '-', '').Substring(0, 8)
    }
    finally {
        $sha.Dispose()
    }
}

function Get-KeyBase {
    param([string]$Value)

    $slug = $Value
    $slug = $slug -replace '%I64d', ' VALUE64 '
    $slug = $slug -replace '%ld', ' VALUE '
    $slug = $slug -replace '%ls', ' TEXT '
    $slug = $slug -replace '%d', ' VALUE '
    $slug = $slug -replace '%s', ' TEXT '
    $slug = $slug -replace '%%', ' PERCENT '
    $slug = $formatPattern.Replace($slug, ' VALUE ')
    $slug = $slug -replace '\+', ' PLUS '
    $slug = $slug.ToUpperInvariant()
    $slug = $slug -replace '[^A-Z0-9]+', '_'
    $slug = $slug.Trim('_')
    if (-not $slug) {
        $slug = "TEXT"
    }

    $key = "UIIT_KMT_$slug"
    if ($key.Length -gt 96) {
        $key = $key.Substring(0, 87).TrimEnd('_') + "_" + (Get-StableHash $Value)
    }
    return $key
}

function Escape-CppLiteral {
    param([string]$Value)

    return $Value.Replace('\', '\\').Replace('"', '\"')
}

$turkishByEnglish = @{}
if (Test-Path -LiteralPath $referenceTextUi -PathType Leaf) {
    foreach ($line in [System.IO.File]::ReadLines($referenceTextUi)) {
        $columns = $line.Split([char]9)
        if ($columns.Count -ge 7 -and
            -not [string]::IsNullOrWhiteSpace($columns[5]) -and
            -not [string]::IsNullOrWhiteSpace($columns[6]) -and
            -not $turkishByEnglish.ContainsKey($columns[5])) {
            $turkishByEnglish[$columns[5]] = $columns[6]
        }
    }
}

$keyByEnglish = @{}
$englishByKey = @{}
$changedFiles = New-Object System.Collections.Generic.List[string]
$replacementCount = 0

if (Test-Path -LiteralPath $kmtTextUi -PathType Leaf) {
    foreach ($line in [System.IO.File]::ReadLines($kmtTextUi)) {
        $columns = $line.Split([char]9)
        if ($columns.Count -lt 7 -or
            [string]::IsNullOrWhiteSpace($columns[1]) -or
            [string]::IsNullOrWhiteSpace($columns[5])) {
            continue
        }

        $existingKey = $columns[1]
        $existingEnglish = $columns[5]
        $keyByEnglish[$existingEnglish] = $existingKey
        $englishByKey[$existingKey] = $existingEnglish
        if (-not [string]::IsNullOrWhiteSpace($columns[6]) -and
            $columns[6] -ne $existingEnglish) {
            $turkishByEnglish[$existingEnglish] = $columns[6]
        }
    }
}

if (Test-Path -LiteralPath $turkishOverrides -PathType Leaf) {
    foreach ($line in [System.IO.File]::ReadLines($turkishOverrides)) {
        $columns = $line.Split([char]9)
        if ($columns.Count -ge 2 -and
            $columns[0] -ne "English" -and
            -not [string]::IsNullOrWhiteSpace($columns[0]) -and
            -not [string]::IsNullOrWhiteSpace($columns[1])) {
            $turkishByEnglish[$columns[0]] = $columns[1]
        }
    }
}

# These strings are composed or stored before they reach a display control, so
# the same-line display-call scanner cannot discover their English source text.
$manualEnglishValues = @(
    "Item",
    "Player",
    "Info",
    "Plus",
    "Monster",
    "Gold",
    "Silk",
    "This character",
    "Offline Stall activated. You may close the client.",
    "Offline Stall activation failed.",
    "Enter the opponent character and the gold wager amount.",
    "%d attempt left",
    "%d attempts left",
    "EXPIRES %02d:%02d",
    "%d %ls",
    "%ls (+%d)",
    "%s  Lv.%u",
    "%ls [%d%%]",
    "Code: %d\n\nEnter the code within %d seconds.",
    "<Unknown>",
    "<None>",
    "<No Title>",
    "<Default Color>",
    "Character-bound",
    "Default",
    "<<Left>>",
    "Target",
    "Page",
    "Restart failed",
    "Trade goods verification required.",
    "Code: %d",
    "Enter the code within %d seconds.",
    "[{charname}] has succeeded in augmenting [{wpname}]'s [+{plus}] level.",
    "[{charname}] has succeeded in augmenting [{wpname}]'s [+{plus}] level with Adv. Elixir.",
    "Secondary Password (Remember PC)",
    "Infinity Zoom : ON",
    "Infinity Zoom : OFF",
    "Always Active Window : ON",
    "Always Active Window : OFF",
    "Extend Background Limit : ON",
    "Extend Background Limit : OFF",
    "Other players cannot show your items anymore!",
    "Other players can show your items!",
    "Sword",
    "Blade",
    "Spear",
    "Glavie",
    "Bow",
    "Onehand sword",
    "Twohand sword",
    "Axe",
    "Dark staff",
    "Twohand staff",
    "Crossbow",
    "Dagger",
    "Harp",
    "Cleric rod"
)

foreach ($value in $manualEnglishValues) {
    if ($keyByEnglish.ContainsKey($value)) {
        continue
    }

    $key = Get-KeyBase $value
    if ($englishByKey.ContainsKey($key) -and $englishByKey[$key] -ne $value) {
        $key = $key + "_" + (Get-StableHash $value)
    }
    $keyByEnglish[$value] = $key
    $englishByKey[$key] = $value
}

$sourceFiles = Get-ChildItem -LiteralPath $clientSourceRoot -Recurse -File |
    Where-Object { $_.Extension -in @(".cpp", ".h") }

foreach ($file in $sourceFiles) {
    $original = [System.IO.File]::ReadAllText($file.FullName)
    $lines = $original -split "(?<=`n)"
    $fileChanged = $false

    for ($lineIndex = 0; $lineIndex -lt $lines.Count; $lineIndex++) {
        $line = $lines[$lineIndex]
        if (-not $displayCallPattern.IsMatch($line) -and
            -not $displayStoragePattern.IsMatch($line)) {
            continue
        }
        if ($line.TrimStart().StartsWith("//")) {
            continue
        }

        $literalMatches = $wideLiteralPattern.Matches($line)
        for ($matchIndex = $literalMatches.Count - 1; $matchIndex -ge 0; $matchIndex--) {
            $literalMatch = $literalMatches[$matchIndex]
            $value = $literalMatch.Groups[1].Value
            if (-not (Test-IsDisplayText $value)) {
                continue
            }

            $before = $line.Substring(0, $literalMatch.Index)
            if ($before -match '(TSM_GETTEXTPTR|KmtGetText)\s*\([^\)]*$') {
                continue
            }

            if ($value -match '^(UIIT_|UIO_|VFILTER_|LEXA_|PARAM_)') {
                $key = $value
            }
            else {
                if (-not $keyByEnglish.ContainsKey($value)) {
                    $key = Get-KeyBase $value
                    if ($englishByKey.ContainsKey($key) -and
                        $englishByKey[$key] -ne $value) {
                        $key = $key + "_" + (Get-StableHash $value)
                    }
                    $keyByEnglish[$value] = $key
                    $englishByKey[$key] = $value
                }
                $key = $keyByEnglish[$value]
            }

            $replacement = 'KmtGetText(L"' + (Escape-CppLiteral $key) + '")'
            $line = $line.Substring(0, $literalMatch.Index) +
                $replacement +
                $line.Substring($literalMatch.Index + $literalMatch.Length)
            $fileChanged = $true
            $replacementCount++
        }
        $lines[$lineIndex] = $line
    }

    if ($fileChanged) {
        $updated = $lines -join ""
        $changedFiles.Add($file.FullName)
        if ($Apply) {
            [System.IO.File]::WriteAllText(
                $file.FullName,
                $updated,
                [System.Text.UTF8Encoding]::new($false))
        }
    }
}

$rows = New-Object System.Collections.Generic.List[string]
$englishRows = New-Object System.Collections.Generic.List[string]
$turkishRows = New-Object System.Collections.Generic.List[string]
foreach ($entry in ($keyByEnglish.GetEnumerator() | Sort-Object Value)) {
    $english = $entry.Key
    $key = $entry.Value
    $turkish = if ($turkishByEnglish.ContainsKey($english)) {
        $turkishByEnglish[$english]
    }
    else {
        $english
    }

    # Layout follows the multilingual TextUISystem reference supplied for this
    # work: service, key, base text, two metadata columns, EN, TR, ES, AR, DE.
    $columns = @(
        "1",
        $key,
        $english,
        "",
        "",
        $english,
        $turkish,
        $english,
        $english,
        $english
    )
    $rows.Add(($columns -join "`t"))

    # Single-language variants intentionally repeat one language in every
    # language column. Customers can swap these files without depending on
    # the client's configured language-column index.
    $englishColumns = @(
        "1", $key, $english, "", "",
        $english, $english, $english, $english, $english
    )
    $turkishColumns = @(
        "1", $key, $turkish, "", "",
        $turkish, $turkish, $turkish, $turkish, $turkish
    )
    $englishRows.Add(($englishColumns -join "`t"))
    $turkishRows.Add(($turkishColumns -join "`t"))
}

Write-Host ("Candidate files: {0}" -f $sourceFiles.Count)
Write-Host ("Changed files: {0}" -f $changedFiles.Count)
Write-Host ("Text replacements: {0}" -f $replacementCount)
Write-Host ("New UIIT_KMT keys: {0}" -f $rows.Count)

if (-not $Apply) {
    Write-Host "Dry run only. Re-run with -Apply to change source files."
    exit 0
}

if (-not (Test-Path -LiteralPath $localizationRoot)) {
    New-Item -ItemType Directory -Path $localizationRoot -Force | Out-Null
}
[System.IO.File]::WriteAllLines(
    $kmtTextUi,
    $rows,
    [System.Text.Encoding]::Unicode)
[System.IO.File]::WriteAllLines(
    $kmtTextUiEnglish,
    $englishRows,
    [System.Text.Encoding]::Unicode)
[System.IO.File]::WriteAllLines(
    $kmtTextUiTurkish,
    $turkishRows,
    [System.Text.Encoding]::Unicode)

Write-Host ("Wrote: {0}" -f $kmtTextUi)
Write-Host ("Wrote: {0}" -f $kmtTextUiEnglish)
Write-Host ("Wrote: {0}" -f $kmtTextUiTurkish)
