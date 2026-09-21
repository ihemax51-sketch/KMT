[CmdletBinding(SupportsShouldProcess)]
param()

$ErrorActionPreference = "Stop"
Set-StrictMode -Version Latest

$workspaceRoot = [System.IO.Path]::GetFullPath((Join-Path $PSScriptRoot ".." )).TrimEnd('\')
$workspacePrefix = $workspaceRoot + '\'
$cleanupCmdlet = $PSCmdlet

function Remove-WorkspacePath([string]$RelativePath) {
    $targetPath = [System.IO.Path]::GetFullPath((Join-Path $workspaceRoot $RelativePath))
    if (-not $targetPath.StartsWith($workspacePrefix, [System.StringComparison]::OrdinalIgnoreCase)) {
        throw "Cleanup target resolved outside the workspace: $RelativePath"
    }
    if (-not (Test-Path -LiteralPath $targetPath)) {
        return
    }
    if ($cleanupCmdlet.ShouldProcess($targetPath, "Remove generated workspace artifact")) {
        Remove-Item -LiteralPath $targetPath -Recurse -Force
    }
}

function Get-WorkspaceRelativePath([string]$FullPath) {
    $resolvedPath = [System.IO.Path]::GetFullPath($FullPath)
    if (-not $resolvedPath.StartsWith($workspacePrefix, [System.StringComparison]::OrdinalIgnoreCase)) {
        throw "Path resolved outside the workspace: $FullPath"
    }
    return $resolvedPath.Substring($workspacePrefix.Length)
}

$loaderSource = Join-Path $workspaceRoot "JTClientLibrary\BinOut\RelWithDebInfo\WebView2Loader.dll"
$loaderDestination = Join-Path $workspaceRoot "JTClientLibrary\BinOut\Release\WebView2Loader.dll"
if ((Test-Path -LiteralPath $loaderSource -PathType Leaf) -and
    -not (Test-Path -LiteralPath $loaderDestination -PathType Leaf) -and
    $cleanupCmdlet.ShouldProcess($loaderDestination, "Keep the release WebView2 loader in the canonical output folder")) {
    New-Item -ItemType Directory -Path (Split-Path $loaderDestination) -Force | Out-Null
    Copy-Item -LiteralPath $loaderSource -Destination $loaderDestination
}

$generatedDirectories = @(
    ".artifacts",
    "artifacts",
    ".build-check",
    ".codex-temp",
    ".tmpbuild",
    ".agents",
    "build",
    "build_verify",
    "build_logs",
    "dump",
    "New folder",
    "_build_obj",
    "_build_out",
    "_menu_crop_candidates",
    "filter\KMTGuardnew\publish",
    "filter\KMTGuardnew\build-logs",
    "filter\KMTGuardnew\.idea",
    "filter\KMTGuardnew\KMTGuard.AdminDesktop",
    "JTClientLibrary\build-vc80",
    "JTClientLibrary\build-vc80-d",
    "JTClientLibrary\build-client-dll",
    "JTClientLibrary\BinOut",
    "JTClientLibrary\BinOut\RelWithDebInfo",
    "JTClientLibrary\source\out",
    "JTClientLibrary\source\BinOut",
    "JTClientLibrary\source\.vs",
    "JTClientLibrary\source\libs\ClientLib\src\.vs",
    "JTClientLibrary\.idea",
    "gameserver\build-kmtguard-vc80",
    "gameserver\build-kmtguard-vc80-nmake",
    "gameserver\cmake-build-relwithdebinfo",
    "gameserver\source\out",
    "gameserver\source\.vs",
    "gameserver\.idea",
    "gameserver\55",
    "gameserver\2222",
    "ShardManager\Release",
    "ShardManager\vSRO-ShardManager\KMTGuard.c8fab369",
    "ShardManager\vSRO-ShardManager\Release",
    "ShardManager\vSRO-ShardManager\x64",
    "ShardManager\vSRO-ShardManager\Outpus",
    "WebViewerBridge\WebViewerBridge\Win32",
    "BinOut\JTGuard_Clientless_Final"
)

foreach ($relativePath in $generatedDirectories) {
    Remove-WorkspacePath $relativePath
}

Get-ChildItem -LiteralPath (Join-Path $workspaceRoot "JTClientLibrary") -Directory -Filter "build-*" -ErrorAction SilentlyContinue |
    ForEach-Object { Remove-WorkspacePath (Get-WorkspaceRelativePath $_.FullName) }

Get-ChildItem -LiteralPath (Join-Path $workspaceRoot "gameserver") -Directory -Filter "build-*" -ErrorAction SilentlyContinue |
    ForEach-Object { Remove-WorkspacePath (Get-WorkspaceRelativePath $_.FullName) }

Get-ChildItem -LiteralPath (Join-Path $workspaceRoot "KMTGuard.AdminDesktop") -Directory -Filter "publish*" -ErrorAction SilentlyContinue |
    ForEach-Object { Remove-WorkspacePath (Get-WorkspaceRelativePath $_.FullName) }

$projectFiles = Get-ChildItem -LiteralPath $workspaceRoot -Recurse -File -Filter "*.csproj" -ErrorAction SilentlyContinue |
    Where-Object {
        $_.FullName -notlike "$workspacePrefix`build_verify\*" -and
        $_.FullName -notlike "$workspacePrefix`.codex-temp\*"
    }
foreach ($projectFile in $projectFiles) {
    Remove-WorkspacePath (Get-WorkspaceRelativePath (Join-Path $projectFile.Directory.FullName "bin"))
    Remove-WorkspacePath (Get-WorkspaceRelativePath (Join-Path $projectFile.Directory.FullName "obj"))
}

$generatedFiles = @(
    "cl_stderr.txt",
    "cl_stderr2.txt",
    "cl_stdout.txt",
    "cl_stdout2.txt",
    "full-rebuild-warnings.log",
    "publish-v3.0.7.stderr.log",
    "publish-v3.0.7.stdout.log",
    "pk2-list.obj",
    "vc80.pdb",
    "_test.dds",
    "JTClientLibrary\client_dll_build_check.log",
    "JTClientLibrary\client_dll_build_fresh_check.log",
    "JTClientLibrary\client_dll_build_root_check.log",
    "BinOut\Win32\Release\WebView2Loader.dll",
    "BinOut\Win32\Release\WebViewerBridge.exp",
    "BinOut\Win32\Release\WebViewerBridge.lib",
    "BinOut\Win32\Release\WebViewerBridge.pdb",
    "JTClientLibrary\BinOut\Release\KMTGuardKit.exp",
    "JTClientLibrary\BinOut\Release\KMTGuardKit.lib",
    "ShardManager\vSRO-ShardManager\Outpus\KMTGuard_ShardManager.exp",
    "ShardManager\vSRO-ShardManager\Outpus\KMTGuard_ShardManager.lib",
    "ShardManager\vSRO-ShardManager\Outpus\KMTGuard_ShardManager.pdb"
)
foreach ($relativePath in $generatedFiles) {
    Remove-WorkspacePath $relativePath
}

Get-ChildItem -LiteralPath $workspaceRoot -Recurse -Force -File -Filter "*.user" -ErrorAction SilentlyContinue |
    ForEach-Object { Remove-WorkspacePath (Get-WorkspaceRelativePath $_.FullName) }

Get-ChildItem -LiteralPath $workspaceRoot -Force -File -Filter "*.obj" -ErrorAction SilentlyContinue |
    ForEach-Object { Remove-WorkspacePath (Get-WorkspaceRelativePath $_.FullName) }

Write-Host "Workspace cleanup completed. Official output in D:\KMTGuard-build was not touched."
