# Source layout

The workspace keeps source, generated files, and final delivery in separate locations.

## Canonical source folders

- `filter/KMTGuardnew`: filter services, runtime hosts, and tests.
- `KMTGuard.AdminDesktop`: desktop control application source. Old `publish-*` folders are not source.
- `JTClientLibrary`: client DLL source and media resources.
- `gameserver` and `ShardManager`: server add-on source.
- `WebViewerBridge`: native web-view bridge source.
- `database/migrations`: current database migrations.
- `database/backups`: intentional database recovery backups; workspace cleanup does not remove them.
- `build-scripts`: the unified full-build script and the component build script.
- `scripts`: database packaging, deployment, validation, and maintenance scripts.
- `deployment`: deployment templates.
- `docs`: engineering and UI references.

## Generated files

Build trees, `bin`, `obj`, IDE caches, temporary staging folders, logs, crash dumps, and historical desktop `publish-*` folders are disposable. Run `scripts/Clean-Workspace.ps1` to remove them.

Build outputs are produced from source and are not retained as release inputs.

## Final delivery

The only official runnable delivery is outside the source tree:

- `D:/KMTGuard-build/Filter`
- `D:/KMTGuard-build/DLL`
- `D:/KMTGuard-build/Media`
- `D:/KMTGuard-build/ServerAddons/GameServer`
- `D:/KMTGuard-build/ServerAddons/ShardManager`

Do not create additional desktop release folders inside the source tree.
