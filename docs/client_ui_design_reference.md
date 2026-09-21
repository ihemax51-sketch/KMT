# Client UI Design Reference

This document is the general visual and implementation reference for custom
Silkroad client windows in Filter-KMTGuard. Read it before creating a new client
window or visually redesigning an existing one.

## Named Style Routing

The project has two separate named client UI styles:

- `Item Mall` uses this document and the Killer Animation Studio as its
  canonical reference.
- `Old School` uses `docs/client_ui_old_school_reference.md` and the Secondary
  Password window as its canonical reference.

When the user explicitly names either style, follow that style's reference and
do not mix its chrome, panels, fields, buttons, or close control with the other
style. Existing window-specific references still take precedence.

The canonical implementation is the Killer Animation Studio completed on
2026-07-13. It combines native Silkroad Item Mall media with restrained custom
layout and state styling. New work should feel like it belongs to the same
client, not like an external desktop or web interface placed over the game.

Window-specific references still take precedence. For example, Drop Logs must
also follow `docs/droplogs_ui_reference.md` and remain resinfo-driven.

## Canonical Implementation

Primary files:

```text
JTClientLibrary/source/libs/ClientLib/src/CustomInterface/IFKillerAnimationWnd.h
JTClientLibrary/source/libs/ClientLib/src/CustomInterface/IFKillerAnimationWnd.cpp
JTClientLibrary/source/libs/ClientLib/src/GInterface.cpp
```

Database seed, not a visual implementation file:

```text
database/migrations/20260713_killer_animations.sql
```

Do not change the database file for visual polish.

## Visual Direction

Use the native Item Mall language:

- Dark textured surfaces rather than flat gray boxes.
- Thin gold framing and section accents.
- Warm ivory body text.
- Cyan for the current selection.
- Green for successful or active states.
- Restrained red for errors only.
- Compact information density appropriate for an in-game tool.
- One strong outer surface with clearly separated working regions.
- Stable control sizes; hover, disabled, and dynamic text must not move layout.

Avoid:

- Generic `interface\\outer\\button.ddj` everywhere when a domain-specific
  Item Mall control already exists.
- Several identical tiled boxes with no visual hierarchy.
- Oversized headings, marketing copy, tutorial copy, or decorative clutter.
- Bright single-color themes that do not match Silkroad.
- ImGui, Win32 controls, HTML overlays, or ad hoc Direct3D drawing for a normal
  Silkroad client window.
- Creating a second header under a mismatched default title frame.

## Window Chrome

The outer title frame, title text, and close button must be designed as one
unit. The canonical combination is:

```cpp
TB_Func_13("interface\\frame\\mall_sub_wnd04_", 0, 0);
SetText(L"Window Title");

m_pTitleText->m_FontTexture.SetColor(COLOR_LABEL);

m_pCloseBtn->TB_Func_13(
    "clientlibrary\\mall\\mall_web_close_button.ddj", 1, 0);
m_pCloseBtn->SetGWndSize(28, 32);
m_pCloseBtn->MoveGWnd(windowWidth - 36, 2);
m_pCloseBtn->ShowGWnd(true);
m_pCloseBtn->BringToFront();
```

Apply close-button size and position after `SetGWndSize`, because the inherited
main-frame sizing logic can reposition its controls.

Do not combine `mframe_wnd_` with the Item Mall body. The glossy black default
header is visually inconsistent with the gold Item Mall surface.

## Canonical Media

All media below already exists in the repository. Reuse it instead of creating
duplicate files.

Outer surfaces:

```text
clientlibrary\mall\win_bg.ddj
clientlibrary\mall\header.ddj
clientlibrary\mall\leftbg.ddj
clientlibrary\mall\mall_pre_start.ddj
```

List rows, including every button state:

```text
clientlibrary\title\title_list.ddj
clientlibrary\title\title_list_focus.ddj
clientlibrary\title\title_list_press.ddj
clientlibrary\title\title_list_disable.ddj
```

Primary actions, including every button state:

```text
clientlibrary\mall\mall_pre_big_button.ddj
clientlibrary\mall\mall_pre_big_button_focus.ddj
clientlibrary\mall\mall_pre_big_button_press.ddj
clientlibrary\mall\mall_pre_big_button_disable.ddj
```

Pagination, including every button state:

```text
clientlibrary\mall\mall_pre_left_button.ddj
clientlibrary\mall\mall_pre_left_button_focus.ddj
clientlibrary\mall\mall_pre_left_button_press.ddj
clientlibrary\mall\mall_pre_left_button_disable.ddj

clientlibrary\mall\mall_pre_right_button.ddj
clientlibrary\mall\mall_pre_right_button_focus.ddj
clientlibrary\mall\mall_pre_right_button_press.ddj
clientlibrary\mall\mall_pre_right_button_disable.ddj
```

Close button:

```text
clientlibrary\mall\mall_web_close_button.ddj
clientlibrary\mall\mall_web_close_button_focus.ddj
clientlibrary\mall\mall_web_close_button_press.ddj
```

Character preview frame supplied by the base client media:

```text
interface\mall\mall_charac_frame.ddj
```

Before using a button texture, verify its focus, press, and disabled companions.
A disabled control must not cause the client to request a nonexistent DDJ.

## Color System

Use the following palette as the default starting point:

```cpp
const D3DCOLOR COLOR_ACCENT = D3DCOLOR_ARGB(255, 255, 216, 83);
const D3DCOLOR COLOR_LABEL = D3DCOLOR_ARGB(255, 239, 218, 164);
const D3DCOLOR COLOR_TEXT = D3DCOLOR_ARGB(255, 245, 242, 232);
const D3DCOLOR COLOR_MUTED = D3DCOLOR_ARGB(255, 185, 184, 174);
const D3DCOLOR COLOR_SELECTED = D3DCOLOR_ARGB(255, 132, 225, 221);
const D3DCOLOR COLOR_ACTIVE = D3DCOLOR_ARGB(255, 114, 255, 154);
const D3DCOLOR COLOR_WARNING = D3DCOLOR_ARGB(255, 255, 132, 118);
```

Semantic usage:

```text
ACCENT    Section headings, selected prices, important neutral highlights
LABEL     Secondary headings, owned labels, pagination, title-bar text
TEXT      Normal names and button labels
MUTED     Inactive metadata and neutral status messages
SELECTED  Current row and current preview title
ACTIVE    Active state and successful request result
WARNING   Error, timeout, or failed request only
```

Selection must not depend only on a text prefix such as `>`. Use color and the
button focus surface so the label width and alignment remain stable.

## Canonical Layout

The Killer Animation Studio uses an `800 x 500` window. It fits completely at
the minimum common `800 x 600` game resolution.

```text
Window                         0,   0, 800, 500
Item Mall body                 0,  31, 800, 469
Item Mall header band          0,  31, 800,  45

Collection panel              20,  82, 334, 350
Collection heading            36,  88, 300,  24
Rows start                     36, 119
Row name button size                  206,  40
Row vertical step                       49
Row status                    250, rowY + 3,  88, 16
Row price                     250, rowY + 21, 88, 16

Preview panel                366,  82, 414, 350
Preview section heading      386,  88, 374,  18
Selected animation title     392, 108, 362,  22
Character render             392, 132, 362, 242
Action buttons Y             389
Action button size                    112,  32

Pagination arrows            120/232, 440, 12, 42
Pagination text              144, 449,  76, 24
Status area                  286, 449, 474, 24
```

These exact coordinates are not mandatory for every window. Preserve the
relationships:

- Title chrome first.
- One header band.
- Left navigation or collection region.
- Larger right work or preview region.
- Actions grouped directly under their target region.
- Pagination and request status on a dedicated footer line.
- Consistent internal margins between 12 and 20 pixels.
- A visible gap between unrelated controls.

Do not let text overlap a neighboring status or price region. Long names must
be truncated, resized, or allocated more width without shifting other rows.

## Control Construction Pattern

Use native Silkroad controls and create them in visual stacking order:

1. Call `CIFMainFrame::OnCreate`.
2. Apply outer frame texture and title.
3. Create the full background surface.
4. Create header and panel surfaces.
5. Create row or content controls.
6. Create preview or primary work controls.
7. Create action and pagination controls.
8. Create status text last.
9. Call `SetGWndSize`.
10. Restyle and position inherited title/close controls.
11. Center the window, hide it, and return.

Decorative controls must not consume clicks:

```cpp
background->SetClickable(false);
```

Every pointer returned from `CreateInstance` or `m_IRM.GetResObj` must be
checked before use. Use unique child IDs within the parent window.

Text styling should be centralized:

```cpp
void StyleStatic(CIFStatic* control, const wchar_t* text,
                 D3DCOLOR color,
                 CTextBoard::eJustifyHorizontal justify);

void StyleButton(CIFButton* button, const wchar_t* text,
                 const char* texture);
```

Set the font, color, horizontal alignment, vertical alignment, visibility, and
front order in these helpers. Do not repeat partial styling across controls.

## State Design

Rows should communicate state without changing their dimensions:

```text
FOR SALE  Muted label plus price
OWNED     Warm label, no price
ACTIVE    Green label, no price
SELECTED  Cyan name text and focused row surface
```

Action labels should reflect state:

```text
Purchase -> Owned after purchase
Activate -> Active after activation
Preview remains available whenever a row is selected
```

Disable unavailable actions with `SetEnabledState`. Never hide a primary action
only because it is currently unavailable; the disabled button explains the
available workflow and keeps the layout stable.

Pagination arrows use full state texture families. Enable them only when the
corresponding page exists and no request is pending.

## Positioning and Resolution Safety

Read the active game resolution and center the complete window. Clamp vertical
position so the title bar and footer remain visible:

```cpp
int width = 1024;
int height = 768;
if (g_CGame && g_CGame->GetRes().res) {
    width = g_CGame->GetRes().res->width;
    height = g_CGame->GetRes().res->height;
}

int y = ((height - GetSize().height) / 2) + 35;
// Apply top and bottom clamps before MoveGWnd.
MoveGWnd((width - GetSize().width) / 2, y);
```

Keep fixed-format controls at explicit dimensions. Dynamic labels, hover
states, or disabled textures must never resize the window or push another
control.

## Crash-Prevention Rules

Visual polish must not change behavior contracts.

For UI-only work, do not change:

```text
Packets or opcodes
Filter handlers
SQL or stored procedures
Request/response parsing
Purchase or activation rules
Animation IDs
Preview object ownership
Timers or refresh cadence
Guide-icon open/close behavior
```

For the Killer Animation Studio specifically, do not alter these functions
during a visual-only task:

```text
PrepareKillerRenderPreview
PlayKillerRenderPreview
BuildKillerRenderPreview
RequestAnimations
RefreshPreview
OnTimer
OnBuy
OnActivate
```

The live preview owns its own render character. Never animate the world player
to simulate a preview. Object index `0` is valid; use `0xFFFFFFFFu` as the
invalid preview-object sentinel.

Do not clear and rebuild the render object on every UI refresh. Replay the
motion on the existing preview animation object when possible. Rebuild only
when the preview object does not exist.

Use existing, verified DDJ assets when possible. A new DDJ must be validated in
the actual client and supplied in every required state before it can become a
dependency.

## Resinfo Ownership

Respect the ownership style of the existing window:

- If a window is already resinfo-driven, keep layout and media paths in its
  resinfo file.
- If a stable custom window already creates controls in C++, keep a visual-only
  change narrowly scoped unless the user explicitly requests migration.
- Do not mix two competing layout sources for the same control.

Drop Logs is a hard resinfo-driven exception. Its layout stays in:

```text
JTClientLibrary/media/clientlibrary/resinfo/ifdroplogswnd.txt
```

## Review Checklist

Before final delivery, verify all of the following:

```text
[ ] Window fits at 800 x 600 without clipping.
[ ] Header, title, close button, and body use one visual language.
[ ] No text overlaps names, status, price, buttons, or neighboring controls.
[ ] Every button has normal, focus, press, and disabled media when needed.
[ ] Decorative surfaces are non-clickable.
[ ] Empty rows are hidden and do not remain clickable.
[ ] Selected, owned, active, disabled, success, and error states are distinct.
[ ] Opening and closing repeatedly does not crash.
[ ] Selection changes do not move layout.
[ ] Live preview still builds, plays, and replays.
[ ] Purchase and activation behavior is unchanged.
[ ] No packet, opcode, filter, SQL, or stored-procedure change was made.
[ ] The client DLL builds successfully.
[ ] Final output is copied to D:\KMTGuard-build.
```

## Client Build

The current client build uses the Visual Studio 2005 environment and the CMake
binary installed with Visual Studio 2022 Build Tools:

```bat
cmd.exe /d /s /c "\"C:\Program Files (x86)\Microsoft Visual Studio 8\Common7\Tools\vsvars32.bat\" >nul && \"C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe\" --build build-client-dll"
```

Run it from:

```text
<repo root>\JTClientLibrary
```

Expected DLL:

```text
<repo root>\JTClientLibrary\BinOut\RelWithDebInfo\KMTGuardKit.dll
```

## Final Delivery

The fixed final path is:

```text
D:\KMTGuard-build
```

Required structure:

```text
D:\KMTGuard-build\Filter\KMTGuard.exe
D:\KMTGuard-build\DLL\KMTGuardKit.dll
D:\KMTGuard-build\Media\clientlibrary
D:\KMTGuard-build\Media\media
D:\KMTGuard-build\Media\client-resources
```

Do not create a different final-output directory unless the user explicitly
changes this rule.

When the redesign only reuses media already present in the project, the user
usually needs the new `KMTGuardKit.dll` only. Still verify that every referenced
asset exists in the delivery media before reporting this.
