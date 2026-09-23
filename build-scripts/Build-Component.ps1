param(
    [Parameter(Mandatory = $true)]
    [ValidateSet("Filter", "ClientDll", "GameServer", "ShardManager", "Media", "All")]
    [string]$Component,
    [ValidateSet("Debug", "Release")]
    [string]$Configuration = "Release"
)

$ErrorActionPreference = "Stop"
Set-StrictMode -Version Latest

$repoRoot = [IO.Path]::GetFullPath((Join-Path $PSScriptRoot ".."))
$buildRootFull = "D:\KMTGuard-build"
$artifactRoot = Join-Path $repoRoot ".artifacts"
$stageRoot = Join-Path $artifactRoot "unified-build-$PID"
$packageRoot = Join-Path $stageRoot "package"
$workRoot = Join-Path $stageRoot "work"
$versionPath = Join-Path $repoRoot "VERSION.txt"
$changelogPath = Join-Path $repoRoot "CHANGELOG.md"
$nativeConfiguration = if ($Configuration -eq "Release") { "RelWithDebInfo" } else { "Debug" }

function Assert-SafePath([string]$Path, [string]$AllowedRoot) {
    $full = [IO.Path]::GetFullPath($Path)
    $root = [IO.Path]::GetFullPath($AllowedRoot).TrimEnd('\') + '\'
    if (-not $full.StartsWith($root, [StringComparison]::OrdinalIgnoreCase)) {
        throw "Path resolves outside the allowed root: $full"
    }
}

function Find-Tool([string]$Name, [string[]]$Candidates) {
    $command = Get-Command $Name -ErrorAction SilentlyContinue
    if ($null -ne $command) { return $command.Source }
    foreach ($candidate in $Candidates) {
        if (Test-Path -LiteralPath $candidate -PathType Leaf) { return $candidate }
    }
    throw "$Name was not found."
}

function Invoke-Checked([string]$Executable, [string[]]$Arguments, [string]$Description) {
    Write-Host "[$Description]" -ForegroundColor Cyan
    & $Executable @Arguments
    if ($LASTEXITCODE -ne 0) { throw "$Description failed with exit code $LASTEXITCODE." }
}

function Invoke-LegacyBuild([string[]]$Commands, [string]$Name) {
    $batchPath = Join-Path $workRoot "$Name.cmd"
    $lines = @("@echo off", "setlocal EnableExtensions", "call `"$script:Vs2005Vars`"") + $Commands
    [IO.File]::WriteAllLines($batchPath, $lines, [Text.Encoding]::ASCII)
    Invoke-Checked $env:ComSpec @("/d", "/c", $batchPath) $Name
}

function Copy-Directory([string]$Source, [string]$Destination) {
    if (-not (Test-Path -LiteralPath $Source -PathType Container)) {
        throw "Required directory is missing: $Source"
    }
    New-Item -ItemType Directory -Path $Destination -Force | Out-Null
    Get-ChildItem -LiteralPath $Source -Force | Copy-Item -Destination $Destination -Recurse -Force
}

function Publish-Directory([string]$Source, [string]$Destination) {
    Assert-SafePath $Destination $buildRootFull
    if (Test-Path -LiteralPath $Destination) {
        Remove-Item -LiteralPath $Destination -Recurse -Force
    }
    New-Item -ItemType Directory -Path (Split-Path $Destination) -Force | Out-Null
    Copy-Directory $Source $Destination
}

function Get-RelativePath([string]$Path, [string]$Root) {
    $rootFull = [IO.Path]::GetFullPath($Root).TrimEnd('\') + '\'
    $pathFull = [IO.Path]::GetFullPath($Path)
    if (-not $pathFull.StartsWith($rootFull, [StringComparison]::OrdinalIgnoreCase)) {
        throw "Path resolves outside the expected root: $pathFull"
    }
    return $pathFull.Substring($rootFull.Length).Replace('\', '/')
}

function Test-MutableFilterPath([string]$RelativePath) {
    return $RelativePath.Equals("Settings.json", [StringComparison]::OrdinalIgnoreCase) -or
        $RelativePath.StartsWith("logs/", [StringComparison]::OrdinalIgnoreCase)
}

function Test-FilesEqual([string]$Left, [string]$Right) {
    if (-not (Test-Path -LiteralPath $Left -PathType Leaf) -or
        -not (Test-Path -LiteralPath $Right -PathType Leaf)) {
        return $false
    }
    $leftItem = Get-Item -LiteralPath $Left
    $rightItem = Get-Item -LiteralPath $Right
    if ($leftItem.Length -ne $rightItem.Length) { return $false }
    return (Get-FileHash -LiteralPath $Left -Algorithm SHA256).Hash -eq
        (Get-FileHash -LiteralPath $Right -Algorithm SHA256).Hash
}

function Assert-FileReplaceable([string]$Path) {
    if (-not (Test-Path -LiteralPath $Path -PathType Leaf)) { return }
    $stream = $null
    try {
        $stream = [IO.File]::Open($Path, [IO.FileMode]::Open, [IO.FileAccess]::ReadWrite, [IO.FileShare]::None)
    }
    catch {
        throw "Cannot replace '$Path' because it is in use or not writable. Stop the affected KMTGuard process and retry."
    }
    finally {
        if ($null -ne $stream) { $stream.Dispose() }
    }
}

function Publish-FilterDirectory([string]$Source, [string]$Destination) {
    Assert-SafePath $Destination $buildRootFull
    New-Item -ItemType Directory -Path $Destination -Force | Out-Null

    $sourceFiles = @(Get-ChildItem -LiteralPath $Source -File -Recurse)
    $sourceRelativePaths = [Collections.Generic.HashSet[string]]::new([StringComparer]::OrdinalIgnoreCase)
    $filesToCopy = [Collections.Generic.List[object]]::new()
    foreach ($sourceFile in $sourceFiles) {
        $relative = Get-RelativePath $sourceFile.FullName $Source
        [void]$sourceRelativePaths.Add($relative)
        $target = Join-Path $Destination $relative.Replace('/', '\')
        if (-not (Test-FilesEqual $sourceFile.FullName $target)) {
            if (Test-Path -LiteralPath $target -PathType Leaf) {
                Assert-FileReplaceable $target
            }
            $filesToCopy.Add([pscustomobject]@{ Source = $sourceFile.FullName; Destination = $target })
        }
    }

    $staleFiles = @(
        Get-ChildItem -LiteralPath $Destination -File -Recurse | Where-Object {
            $relative = Get-RelativePath $_.FullName $Destination
            -not (Test-MutableFilterPath $relative) -and -not $sourceRelativePaths.Contains($relative)
        }
    )
    foreach ($staleFile in $staleFiles) {
        Assert-FileReplaceable $staleFile.FullName
    }

    foreach ($entry in $filesToCopy) {
        New-Item -ItemType Directory -Path (Split-Path $entry.Destination) -Force | Out-Null
        Copy-Item -LiteralPath $entry.Source -Destination $entry.Destination -Force
    }
    foreach ($staleFile in $staleFiles) {
        Remove-Item -LiteralPath $staleFile.FullName -Force
    }
    Get-ChildItem -LiteralPath $Destination -Directory -Recurse |
        Sort-Object { $_.FullName.Length } -Descending |
        Where-Object {
            $relative = Get-RelativePath $_.FullName $Destination
            -not $relative.Equals("logs", [StringComparison]::OrdinalIgnoreCase) -and
            $null -eq (Get-ChildItem -LiteralPath $_.FullName -Force | Select-Object -First 1)
        } |
        Remove-Item -Force
}

function Assert-Output([string]$Path) {
    if (-not (Test-Path -LiteralPath $Path -PathType Leaf)) {
        throw "Expected build output was not produced: $Path"
    }
}

function Build-Filter {
    $destination = Join-Path $packageRoot "Filter"
    New-Item -ItemType Directory -Path $destination -Force | Out-Null
    $projects = @(
        @{ Name = "Desktop"; Project = "KMTGuard.AdminDesktop\KMTGuard.AdminDesktop.csproj"; Exe = "KMTGuard.AdminDesktop.exe"; Output = "KMTGuard.exe" },
        @{ Name = "Agent"; Project = "filter\KMTGuardnew\KMTGuard.AgentHost\KMTGuard.AgentHost.csproj"; Exe = "KMTGuard.Agent.exe"; Output = "KMTGuard.Agent.exe" },
        @{ Name = "Gateway"; Project = "filter\KMTGuardnew\KMTGuard.GatewayHost\KMTGuard.GatewayHost.csproj"; Exe = "KMTGuard.Gateway.exe"; Output = "KMTGuard.Gateway.exe" },
        @{ Name = "Download"; Project = "filter\KMTGuardnew\KMTGuard.DownloadHost\KMTGuard.DownloadHost.csproj"; Exe = "KMTGuard.Download.exe"; Output = "KMTGuard.Download.exe" }
    )
    foreach ($entry in $projects) {
        $output = Join-Path $workRoot "filter-$($entry.Name)"
        $arguments = @(
            "publish", (Join-Path $repoRoot $entry.Project), "-c", $Configuration,
            "-r", "win-x64", "--self-contained", "false", "-p:PublishSingleFile=true",
            "-p:DebugType=None", "-o", $output
        )
        Invoke-Checked "dotnet" $arguments "Publish Filter $($entry.Name)"
        $sourceExe = Join-Path $output $entry.Exe
        Assert-Output $sourceExe
        Copy-Item -LiteralPath $sourceExe -Destination (Join-Path $destination $entry.Output) -Force
    }

    $existingSettings = Join-Path $buildRootFull "Filter\Settings.json"
    $settingsSource = if (Test-Path -LiteralPath $existingSettings -PathType Leaf) {
        $existingSettings
    } else {
        Join-Path $repoRoot "filter\KMTGuardnew\KMTGuard\config\Settings.example.json"
    }
    Copy-Item -LiteralPath $settingsSource -Destination (Join-Path $destination "Settings.json") -Force
    Copy-Item -LiteralPath $versionPath, $changelogPath -Destination $destination -Force
    Copy-Item -LiteralPath (Join-Path $repoRoot "filter\KMTGuardnew\KMTGuard\webviewer.json") -Destination $destination -Force
    Copy-Directory (Join-Path $repoRoot "filter\KMTGuardnew\KMTGuard\Languages") (Join-Path $destination "Languages")
    & (Join-Path $repoRoot "scripts\Publish-KmtGuardDatabase.ps1") -Destination $destination
    if ($LASTEXITCODE -ne 0) { throw "Database package creation failed." }
}

function Build-ClientDll {
    $destination = Join-Path $packageRoot "DLL"
    $clientBuild = Join-Path $workRoot "client"
    New-Item -ItemType Directory -Path $destination -Force | Out-Null
    Invoke-LegacyBuild @(
        "if errorlevel 1 exit /b %errorlevel%",
        "`"$script:CMake`" -G `"NMake Makefiles`" -DCMAKE_BUILD_TYPE=$Configuration -DPUT_LOGLEVEL=PUT_WARNING -DKMT_DIAGNOSTIC_SKIP_SETUP=OFF -DKMT_CLIENT_OUTPUT_DIRECTORY=`"$destination`" -S `"$(Join-Path $repoRoot 'JTClientLibrary')`" -B `"$clientBuild`"",
        "if errorlevel 1 exit /b %errorlevel%",
        "`"$script:CMake`" --build `"$clientBuild`" --target DevKit_DLL",
        "exit /b %errorlevel%"
    ) "build-client-dll"
    Assert-Output (Join-Path $destination "KMTGuardKit.dll")

    $webViewVersion = "1.0.2849.39"
    $webViewHeader = Join-Path $env:USERPROFILE ".nuget\packages\microsoft.web.webview2\$webViewVersion\build\native\include\WebView2.h"
    if (-not (Test-Path -LiteralPath $webViewHeader -PathType Leaf)) {
        $restoreProject = Join-Path $workRoot "webview2-restore\WebView2Restore.csproj"
        New-Item -ItemType Directory -Path (Split-Path $restoreProject) -Force | Out-Null
        $restoreXml = @"
<Project Sdk="Microsoft.NET.Sdk">
  <PropertyGroup><TargetFramework>net8.0</TargetFramework></PropertyGroup>
  <ItemGroup><PackageReference Include="Microsoft.Web.WebView2" Version="$webViewVersion" /></ItemGroup>
</Project>
"@
        [IO.File]::WriteAllText($restoreProject, $restoreXml, [Text.UTF8Encoding]::new($false))
        Invoke-Checked "dotnet" @("restore", $restoreProject, "--nologo") "Restore WebView2 build dependency"
        Assert-Output $webViewHeader
    }

    $bridgeOutput = Join-Path $workRoot "webviewer"
    New-Item -ItemType Directory -Path $bridgeOutput -Force | Out-Null
    Invoke-Checked $script:MsBuild @(
        (Join-Path $repoRoot "WebViewerBridge\WebViewerBridge.sln"), "/t:Rebuild",
        "/p:Configuration=$Configuration", "/p:Platform=Win32", "/p:OutDir=$bridgeOutput\",
        "/m", "/nologo"
    ) "Build WebViewerBridge"
    $bridgeDll = Join-Path $bridgeOutput "WebViewerBridge.dll"
    if (Test-Path -LiteralPath $bridgeDll -PathType Leaf) {
        Copy-Item -LiteralPath $bridgeDll -Destination $destination -Force
    }
    $webViewLoader = Join-Path $bridgeOutput "WebView2Loader.dll"
    if (Test-Path -LiteralPath $webViewLoader -PathType Leaf) {
        Copy-Item -LiteralPath $webViewLoader -Destination $destination -Force
    }
    Copy-Item -LiteralPath $versionPath -Destination $destination -Force
}

function Build-GameServer {
    $destination = Join-Path $packageRoot "ServerAddons\GameServer"
    $serverBuild = Join-Path $workRoot "gameserver"
    New-Item -ItemType Directory -Path $destination -Force | Out-Null
    Invoke-LegacyBuild @(
        "if errorlevel 1 exit /b %errorlevel%",
        "`"$script:CMake`" -S `"$(Join-Path $repoRoot 'gameserver')`" -B `"$serverBuild`" -G Ninja -DCMAKE_MAKE_PROGRAM:FILEPATH=`"$script:Ninja`" -DCMAKE_BUILD_TYPE=$nativeConfiguration -DKMT_GAMESERVER_OUTPUT_DIRECTORY=`"$destination`"",
        "if errorlevel 1 exit /b %errorlevel%",
        "`"$script:CMake`" --build `"$serverBuild`" --target OutPutGS",
        "exit /b %errorlevel%"
    ) "build-gameserver"
    Assert-Output (Join-Path $destination "KMTGuard_GameServer.dll")
    Copy-Item -LiteralPath $versionPath -Destination $destination -Force
}

function Build-ShardManager {
    $destination = Join-Path $packageRoot "ServerAddons\ShardManager"
    $intermediate = Join-Path $workRoot "shard-obj"
    New-Item -ItemType Directory -Path $destination, $intermediate -Force | Out-Null
    Invoke-Checked $script:MsBuild @(
        (Join-Path $repoRoot "ShardManager\KMTGuard-SM.sln"), "/t:Rebuild",
        "/p:Configuration=$Configuration", "/p:Platform=x86", "/p:OutDir=$destination\",
        "/p:IntDir=$intermediate\", "/m", "/nologo"
    ) "Build ShardManager"
    Assert-Output (Join-Path $destination "KMTGuard_ShardManager.dll")
    $ini = Join-Path $PSScriptRoot "templates\KMTGuard-Addon.ini"
    Assert-Output $ini
    Copy-Item -LiteralPath $ini -Destination $destination -Force
    Copy-Item -LiteralPath $versionPath -Destination $destination -Force
}

function Build-Media {
    $destination = Join-Path $packageRoot "Media"
    Copy-Directory (Join-Path $repoRoot "JTClientLibrary\clientlibrary") (Join-Path $destination "clientlibrary")
    Copy-Directory (Join-Path $repoRoot "JTClientLibrary\media") (Join-Path $destination "media")
    Copy-Directory (Join-Path $repoRoot "JTClientLibrary\client-resources") (Join-Path $destination "client-resources")
}

if (-not (Test-Path -LiteralPath $versionPath -PathType Leaf) -or
    -not (Test-Path -LiteralPath $changelogPath -PathType Leaf)) {
    throw "VERSION.txt and CHANGELOG.md are required."
}
if ([string]::IsNullOrWhiteSpace($buildRootFull) -or $buildRootFull -eq [IO.Path]::GetPathRoot($buildRootFull)) {
    throw "BuildRoot cannot be a drive root."
}
Assert-SafePath $stageRoot $repoRoot
New-Item -ItemType Directory -Path $packageRoot, $workRoot, $buildRootFull -Force | Out-Null

try {
    $components = if ($Component -eq "All") {
        @("Filter", "ClientDll", "GameServer", "ShardManager", "Media")
    } else {
        @($Component)
    }

    if ($components -contains "ClientDll" -or $components -contains "GameServer") {
        $script:CMake = Find-Tool "cmake.exe" @(
            "C:\Program Files\CMake\bin\cmake.exe",
            "C:\Program Files (x86)\CMake\bin\cmake.exe",
            "C:\Program Files\Microsoft Visual Studio\2022\BuildTools\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe",
            "C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe"
        )
        $script:Vs2005Vars = "C:\Program Files (x86)\Microsoft Visual Studio 8\Common7\Tools\vsvars32.bat"
        if (-not (Test-Path -LiteralPath $script:Vs2005Vars -PathType Leaf)) {
            throw "Visual Studio 2005 x86 build tools were not found."
        }
    }
    if ($components -contains "GameServer") {
        $script:Ninja = Find-Tool "ninja.exe" @(
            "C:\Program Files\Microsoft Visual Studio\2022\BuildTools\Common7\IDE\CommonExtensions\Microsoft\CMake\Ninja\ninja.exe",
            "C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\Common7\IDE\CommonExtensions\Microsoft\CMake\Ninja\ninja.exe"
        )
    }
    if ($components -contains "ClientDll" -or $components -contains "ShardManager") {
        $script:MsBuild = Find-Tool "MSBuild.exe" @(
            "C:\Program Files\Microsoft Visual Studio\2022\BuildTools\MSBuild\Current\Bin\MSBuild.exe",
            "C:\Program Files\Microsoft Visual Studio\2022\Community\MSBuild\Current\Bin\MSBuild.exe",
            "C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\MSBuild\Current\Bin\MSBuild.exe"
        )
    }
    foreach ($item in $components) {
        & "Build-$item"
    }

    if ($components -contains "Filter") { Publish-FilterDirectory (Join-Path $packageRoot "Filter") (Join-Path $buildRootFull "Filter") }
    if ($components -contains "ClientDll") { Publish-Directory (Join-Path $packageRoot "DLL") (Join-Path $buildRootFull "DLL") }
    if ($components -contains "GameServer") { Publish-Directory (Join-Path $packageRoot "ServerAddons\GameServer") (Join-Path $buildRootFull "ServerAddons\GameServer") }
    if ($components -contains "ShardManager") { Publish-Directory (Join-Path $packageRoot "ServerAddons\ShardManager") (Join-Path $buildRootFull "ServerAddons\ShardManager") }
    if ($components -contains "Media") { Publish-Directory (Join-Path $packageRoot "Media") (Join-Path $buildRootFull "Media") }

    Copy-Item -LiteralPath $versionPath, $changelogPath -Destination $buildRootFull -Force
    $hashTargets = @("Filter", "DLL", "Media", "ServerAddons", "VERSION.txt", "CHANGELOG.md") |
        ForEach-Object { Join-Path $buildRootFull $_ } |
        Where-Object { Test-Path -LiteralPath $_ }
    $hashLines = Get-ChildItem -LiteralPath $hashTargets -File -Recurse |
        Where-Object {
            $relative = Get-RelativePath $_.FullName $buildRootFull
            $_.Name -ne "SHA256SUMS.txt" -and
                -not $relative.Equals("Filter/Settings.json", [StringComparison]::OrdinalIgnoreCase) -and
                -not $relative.StartsWith("Filter/logs/", [StringComparison]::OrdinalIgnoreCase)
        } |
        Sort-Object FullName |
        ForEach-Object {
            $relative = $_.FullName.Substring($buildRootFull.TrimEnd('\').Length + 1).Replace('\', '/')
            "{0}  {1}" -f (Get-FileHash -LiteralPath $_.FullName -Algorithm SHA256).Hash, $relative
        }
    [IO.File]::WriteAllLines((Join-Path $buildRootFull "SHA256SUMS.txt"), $hashLines, [Text.UTF8Encoding]::new($false))
    Write-Host "Unified $Component package is ready at $buildRootFull" -ForegroundColor Green
}
finally {
    if (Test-Path -LiteralPath $stageRoot) { Remove-Item -LiteralPath $stageRoot -Recurse -Force }
    if ((Test-Path -LiteralPath $artifactRoot) -and
        $null -eq (Get-ChildItem -LiteralPath $artifactRoot -Force | Select-Object -First 1)) {
        Remove-Item -LiteralPath $artifactRoot -Force
    }
}
