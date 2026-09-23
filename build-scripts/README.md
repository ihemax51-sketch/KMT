# Unified build scripts

KMTGuard now has one unrestricted edition. All components are built into one
delivery tree with no edition-specific variants.

Build and package every component:

```powershell
powershell -ExecutionPolicy Bypass -File .\build-scripts\Build-All.ps1
powershell -ExecutionPolicy Bypass -File .\build-scripts\Build-All.ps1 -Configuration Debug
```

Build and package one component:

```powershell
powershell -ExecutionPolicy Bypass -File .\build-scripts\Build-Component.ps1 -Component Filter
powershell -ExecutionPolicy Bypass -File .\build-scripts\Build-Component.ps1 -Component ClientDll
powershell -ExecutionPolicy Bypass -File .\build-scripts\Build-Component.ps1 -Component GameServer
powershell -ExecutionPolicy Bypass -File .\build-scripts\Build-Component.ps1 -Component ShardManager
powershell -ExecutionPolicy Bypass -File .\build-scripts\Build-Component.ps1 -Component Media
```

All artifacts are published to the fixed `D:\KMTGuard-build` delivery tree.
Existing Filter `Settings.json` is preserved when rebuilding the Filter package.
Running Filter logs are preserved and are not included in `SHA256SUMS.txt`.
The mutable `Filter\Settings.json` file is also excluded from the checksum manifest.
If a changed Filter executable is currently running, publishing stops before
replacing any Filter files and reports that the affected process must be stopped.
