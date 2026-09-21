# Unified build scripts

KMTGuard now has one unrestricted edition. All components are built into one
delivery tree with no edition-specific variants.

Build and package every component:

```powershell
powershell -ExecutionPolicy Bypass -File .\build-scripts\Build-All.ps1
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
