param(
    [Parameter(Mandatory = $true)]
    [string]$Destination
)

$ErrorActionPreference = "Stop"
Set-StrictMode -Version Latest

$repoRoot = [System.IO.Path]::GetFullPath((Join-Path $PSScriptRoot ".."))
$databaseRoot = Join-Path $repoRoot "database"
$migrationsSource = Join-Path $databaseRoot "migrations"
$validationSource = Join-Path $databaseRoot "tests"
$manifestPath = Join-Path $databaseRoot "package-versions.psd1"
$readmePath = Join-Path $databaseRoot "package-readme-ar.md"
$changelogPath = Join-Path $repoRoot "CHANGELOG.md"
$destinationRoot = [System.IO.Path]::GetFullPath($Destination)
$destinationDatabase = Join-Path $destinationRoot "database"

foreach ($requiredPath in @(
    $migrationsSource,
    $validationSource,
    $manifestPath,
    $readmePath,
    $changelogPath
)) {
    if (-not (Test-Path -LiteralPath $requiredPath)) {
        throw "Required database packaging source is missing: $requiredPath"
    }
}

$manifest = Import-PowerShellDataFile -LiteralPath $manifestPath
if ($manifest.SchemaVersion -ne 1) {
    throw "Unsupported database package manifest schema: $($manifest.SchemaVersion)"
}

$changelog = Get-Content -LiteralPath $changelogPath -Raw
$changelogVersions = [regex]::Matches(
    $changelog,
    '(?m)^## Update (v\d+\.\d+\.\d+)\s*$') |
    ForEach-Object { $_.Groups[1].Value }

$assignedMigrations = New-Object System.Collections.Generic.List[string]
$assignedValidation = New-Object System.Collections.Generic.List[string]
$assignedBundles = New-Object System.Collections.Generic.List[string]
$seenVersions = New-Object System.Collections.Generic.HashSet[string](
    [System.StringComparer]::OrdinalIgnoreCase)

foreach ($versionEntry in $manifest.Versions) {
    $version = [string]$versionEntry.Version
    if ($version -notmatch '^v\d+\.\d+\.\d+$') {
        throw "Invalid database package version '$version'. Use vMAJOR.MINOR.PATCH."
    }
    if (-not $seenVersions.Add($version)) {
        throw "Database package version '$version' is listed more than once."
    }
    if ($changelogVersions -notcontains $version) {
        throw "Database package version '$version' has no matching Update entry in CHANGELOG.md."
    }

    foreach ($fileName in @($versionEntry.Migrations)) {
        $assignedMigrations.Add([string]$fileName)
    }
    foreach ($fileName in @($versionEntry.Validation)) {
        $assignedValidation.Add([string]$fileName)
    }
    if ($versionEntry.ContainsKey("FullBundle")) {
        $bundleName = [string]$versionEntry.FullBundle
        $bundleBaseVersion = if ($versionEntry.ContainsKey("FullBundleBaseVersion")) {
            [string]$versionEntry.FullBundleBaseVersion
        } else {
            string.Empty
        }
        if ([string]::IsNullOrWhiteSpace($bundleName) -or
            [System.IO.Path]::GetFileName($bundleName) -ne $bundleName -or
            -not $bundleName.EndsWith(".sql", [System.StringComparison]::OrdinalIgnoreCase)) {
            throw "Invalid full database bundle name '$bundleName' for $version."
        }
        if ([string]::IsNullOrWhiteSpace($bundleBaseVersion) -or
            $bundleBaseVersion -notmatch '^v\d+\.\d+\.\d+$') {
            throw "Full database bundle '$bundleName' requires a valid FullBundleBaseVersion."
        }
        $assignedBundles.Add($bundleName)
    }
}

$duplicateBundles = @(
    $assignedBundles |
        Group-Object |
        Where-Object Count -gt 1 |
        Select-Object -ExpandProperty Name
)
if ($duplicateBundles.Count -gt 0) {
    throw "Full database bundle names are assigned more than once: $($duplicateBundles -join ', ')"
}

function Assert-CompleteAssignment(
    [string]$Kind,
    [string]$Source,
    [System.Collections.Generic.List[string]]$Assigned
) {
    $actual = @(
        Get-ChildItem -LiteralPath $Source -Filter "*.sql" -File |
            Select-Object -ExpandProperty Name |
            Sort-Object
    )
    $duplicates = @(
        $Assigned |
            Group-Object |
            Where-Object Count -gt 1 |
            Select-Object -ExpandProperty Name
    )
    if ($duplicates.Count -gt 0) {
        throw "$Kind files are assigned more than once: $($duplicates -join ', ')"
    }

    $missingAssignments = @(Compare-Object $actual @($Assigned | Sort-Object) |
        Where-Object SideIndicator -eq '<=' |
        Select-Object -ExpandProperty InputObject)
    $missingFiles = @(Compare-Object $actual @($Assigned | Sort-Object) |
        Where-Object SideIndicator -eq '=>' |
        Select-Object -ExpandProperty InputObject)

    if ($missingAssignments.Count -gt 0) {
        throw "New $Kind files must be assigned to a filter version in database\package-versions.psd1: $($missingAssignments -join ', ')"
    }
    if ($missingFiles.Count -gt 0) {
        throw "$Kind manifest entries do not exist in the source folder: $($missingFiles -join ', ')"
    }
}

Assert-CompleteAssignment "migration" $migrationsSource $assignedMigrations
Assert-CompleteAssignment "validation" $validationSource $assignedValidation

if (Test-Path -LiteralPath $destinationDatabase) {
    Remove-Item -LiteralPath $destinationDatabase -Recurse -Force
}
New-Item -ItemType Directory -Path $destinationDatabase -Force | Out-Null
Copy-Item -LiteralPath $readmePath -Destination (Join-Path $destinationDatabase "README-AR.md") -Force

foreach ($versionEntry in $manifest.Versions) {
    $versionDestination = Join-Path $destinationDatabase ([string]$versionEntry.Version)
    New-Item -ItemType Directory -Path $versionDestination -Force | Out-Null

    foreach ($fileName in @($versionEntry.Migrations)) {
        Copy-Item -LiteralPath (Join-Path $migrationsSource $fileName) `
            -Destination (Join-Path $versionDestination $fileName) -Force
    }

    $validationFiles = @($versionEntry.Validation)
    if ($validationFiles.Count -gt 0) {
        $validationDestination = Join-Path $versionDestination "validation"
        New-Item -ItemType Directory -Path $validationDestination -Force | Out-Null
        foreach ($fileName in $validationFiles) {
            Copy-Item -LiteralPath (Join-Path $validationSource $fileName) `
                -Destination (Join-Path $validationDestination $fileName) -Force
        }
    }

    if ($versionEntry.ContainsKey("FullBundle")) {
        $bundleName = [string]$versionEntry.FullBundle
        $bundleBaseVersion = [string]$versionEntry.FullBundleBaseVersion
        $bundlePath = Join-Path $versionDestination $bundleName
        $bundle = New-Object System.Text.StringBuilder
        [void]$bundle.AppendLine("/*")
        [void]$bundle.AppendLine("    KMTGuard complete database update from $bundleBaseVersion through $([string]$versionEntry.Version).")
        [void]$bundle.AppendLine("    Generated from the version manifest. Run this one file instead of the individual migration files.")
        [void]$bundle.AppendLine("    Back up the KMTGuard database before execution. Do not run this bundle and the individual files together.")
        [void]$bundle.AppendLine("*/")
        [void]$bundle.AppendLine("SET NOCOUNT ON;")
        [void]$bundle.AppendLine("SET XACT_ABORT ON;")
        [void]$bundle.AppendLine("GO")

        $baseVersionFound = $false
        foreach ($includedVersion in $manifest.Versions) {
            if ([string]$includedVersion.Version -eq $bundleBaseVersion) {
                $baseVersionFound = $true
                continue
            }
            if (-not $baseVersionFound) {
                continue
            }

            foreach ($migrationName in @($includedVersion.Migrations)) {
                $migrationPath = Join-Path $migrationsSource ([string]$migrationName)
                [void]$bundle.AppendLine("")
                [void]$bundle.AppendLine("PRINT N'KMTGuard update $([string]$includedVersion.Version): $([string]$migrationName)';")
                [void]$bundle.AppendLine("GO")
                [void]$bundle.AppendLine((Get-Content -LiteralPath $migrationPath -Raw))
                [void]$bundle.AppendLine("")
                [void]$bundle.AppendLine("GO")
            }

            if ([string]$includedVersion.Version -eq [string]$versionEntry.Version) {
                break
            }
        }

        if (-not $baseVersionFound) {
            throw "Full database bundle base version '$bundleBaseVersion' is not present in the manifest."
        }

        [void]$bundle.AppendLine("")
        [void]$bundle.AppendLine("PRINT N'KMTGuard complete database update finished successfully.';")
        [void]$bundle.AppendLine("GO")
        [System.IO.File]::WriteAllText(
            $bundlePath,
            $bundle.ToString(),
            [System.Text.UTF8Encoding]::new($true))
    }
}

$packagedMigrations = @(
    Get-ChildItem -LiteralPath $destinationDatabase -Filter "*.sql" -File -Recurse |
        Where-Object { $_.Directory.Name -ne "validation" }
)
$expectedSqlFiles = $assignedMigrations.Count + $assignedBundles.Count
if ($packagedMigrations.Count -ne $expectedSqlFiles) {
    throw "Database package verification failed. Expected $expectedSqlFiles migration/bundle files and found $($packagedMigrations.Count)."
}

foreach ($bundleName in $assignedBundles) {
    $bundleMatches = @($packagedMigrations | Where-Object Name -eq $bundleName)
    if ($bundleMatches.Count -ne 1 -or $bundleMatches[0].Length -le 0) {
        throw "Full database bundle '$bundleName' was not packaged exactly once or is empty."
    }
}

Write-Host "Packaged $($assignedMigrations.Count) database updates and $($assignedBundles.Count) complete bundle(s) by filter version." -ForegroundColor Green
