# KMTGuard Agent Instructions

These instructions apply to every agent working inside the KMTGuard repository.

Follow the most specific instruction available for the current task. A window-specific or feature-specific reference takes precedence over general project instructions.

---

## 1. Project Reference

Before analyzing, debugging, modifying, or adding anything to the project, read:

```text
PROJECT_REFERENCE.md
```

Use it to understand:

* Project architecture and component relationships.
* Important applications, DLLs, services, and entry points.
* Startup and shutdown flows.
* Client hooks and custom UI classes.
* Packets, opcodes, and handlers.
* Database tables, stored procedures, and command queues.
* Feature locations, configuration files, and logs.
* Known issues and sensitive areas.

`PROJECT_REFERENCE.md` is a navigation and context reference only.

Do not treat it as a replacement for the current source code. Before making any change, verify all task-related information directly from the latest source files.

If the reference conflicts with the current source code, the current source code is authoritative.

Do not modify `PROJECT_REFERENCE.md` unless the user explicitly requests a documentation update or the current task requires keeping the reference synchronized.

---

## 2. Player-Facing Text and Localization

Before adding or changing any player-facing text in the Client DLL, Filter, custom packets, notices, events, or language files, read:

```text
docs/player_localization_reference.md
```

Do not introduce hardcoded player-facing wording.

Use the correct text owner:

```text
Client-owned interface text -> Client TextUISystem resources
Filter-generated text       -> External Filter language JSON files
```

This applies to:

* Client windows.
* Buttons and labels.
* Notices.
* Event messages.
* Custom packet text.
* Errors and confirmations.
* Player-facing system messages.

---

## 3. Client UI Design

Before creating or visually changing any Client UI, read:

```text
docs/client_ui_design_reference.md
```

Named UI styles are routed explicitly:

```text
Item Mall  -> docs/client_ui_design_reference.md
Old School -> docs/client_ui_old_school_reference.md
```

### Item Mall Style

The Killer Animation Studio is the canonical Item Mall design reference.

### Old School Style

When the user explicitly requests `Old School`, read the following file completely:

```text
docs/client_ui_old_school_reference.md
```

Use the current Secondary Password window as the canonical Old School implementation.

Do not mix Item Mall and Old School elements in the same window, including:

* Window chrome.
* Background panels.
* Input fields.
* Buttons.
* Close controls.
* Borders.
* Titles.
* Decorative elements.

An existing window-specific design reference always takes precedence over the general UI references.

---

## 4. Drop Logs UI

Before changing the Drop Logs Client UI, read:

```text
docs/droplogs_ui_reference.md
```

When the request is UI polish only, do not modify:

* Packets.
* Opcodes.
* Filter code.
* SQL.
* Stored procedures.
* Existing feature behavior.

Keep the Drop Logs layout resinfo-driven through:

```text
JTClientLibrary/media/clientlibrary/resinfo/ifdroplogswnd.txt
```

Do not move layout ownership into hardcoded C++ unless explicitly required by the task.

---

## 5. Final Build Output

The final delivery path is fixed:

```text
D:\KMTGuard-build
```

Do not create or use an alternative final delivery path unless the user explicitly changes this rule.

Any agent producing a final build must copy the latest outputs into:

```text
D:\KMTGuard-build\Filter
D:\KMTGuard-build\DLL
D:\KMTGuard-build\Media
D:\KMTGuard-build\ServerAddons\GameServer
D:\KMTGuard-build\ServerAddons\ShardManager
```

### Filter Output

The latest built Filter output must be copied to:

```text
D:\KMTGuard-build\Filter
```

The main executable must be available as:

```text
D:\KMTGuard-build\Filter\KMTGuard.exe
```

### Client DLL Output

The latest built Client DLL must be copied to:

```text
D:\KMTGuard-build\DLL
```

The main DLL must be available as:

```text
D:\KMTGuard-build\DLL\KMTGuardKit.dll
```

### Generated Media Output

The latest generated Client media and resource files must be copied while preserving their folder structure:

```text
D:\KMTGuard-build\Media\clientlibrary
D:\KMTGuard-build\Media\media
D:\KMTGuard-build\Media\client-resources
```

Only produce a build when the user requests one or when building is explicitly required by the task.

---

## 6. Database Package Layout

The unified Filter package must use this database update layout:

```text
Filter\database\vMAJOR.MINOR.PATCH
```

Example:

```text
Filter\database\v2.7.12
```

Executable update scripts must be placed directly inside their owning version folder.

Optional verification scripts must be placed inside:

```text
Filter\database\vMAJOR.MINOR.PATCH\validation
```

Do not:

* Place SQL files directly in the `Filter` root.
* Generate a packaged `Filter\database\migrations` folder.
* Assign one SQL source file to multiple releases.
* Package SQL files that are not assigned to a release.

The required package assignment manifest is:

```text
database\package-versions.psd1
```

Every SQL source file must be assigned exactly once to a version that exists in:

```text
CHANGELOG.md
```

Publishing must fail when a SQL file is:

* Unassigned.
* Assigned more than once.
* Missing.
* Assigned to an unknown version.
* Assigned to a version not present in `CHANGELOG.md`.

Keep these checks enforced for the unified delivery.

---

## 7. Unified Build Workflow

After source changes, use:

```text
build-scripts\Build-All.ps1
```

For one component only, use:

```text
build-scripts\Build-Component.ps1 -Component <Filter|ClientDll|GameServer|ShardManager|Media>
```

There is one unrestricted edition and one delivery tree. Do not create edition-specific variants or alternate package trees.

---

## 8. Required Changelog

The repository-root file below is required:

```text
CHANGELOG.md
```

No change is complete until it includes a customer-facing changelog entry.

This applies to:

* Bug fixes.
* New features.
* Performance improvements.
* System changes.
* Database changes.
* Security improvements.
* Client changes.
* Operational changes.

Keep the newest version first and preserve all previous versions.

Use semantic versioning:

```text
Patch -> Small fixes and compatible corrections
Minor -> New features or grouped improvements
Major -> Large or incompatible changes
```

Use the actual release date.

Write only clear customer-facing outcomes.

Do not include:

* Source code.
* Internal class names.
* Internal function names.
* Implementation details.
* Repository paths.
* Memory addresses.
* Hook details.
* Information that could expose how security mechanisms work.

Clearly state when an update requires:

* Server restart.
* Filter restart.
* SQL update.
* Client DLL replacement.
* Media update.
* Additional customer action.

Omit empty sections and do not duplicate entries.

---

## 9. General Safety Rules

Before changing anything:

1. Read `PROJECT_REFERENCE.md`.
2. Read the current task-related source files.
3. Read any relevant feature-specific reference.
4. Confirm the actual current implementation.
5. Keep the modification limited to the requested scope.

Do not:

* Replace complete files to solve a small localized issue.
* Change unrelated systems.
* Change packets or opcodes during a UI-only task.
* Change SQL during a Client-only task.
* Change Hook order without verifying dependencies.
* Assume documentation is newer than the source.
* Invent missing classes, procedures, columns, packets, or settings.
* Build, publish, package, or deploy unless the task requires it.

When information cannot be verified, clearly mark it as:

```text
Unknown / Needs Verification
```

---
