# Client UI Old School Reference

This document defines the named **Old School** visual style for custom
Silkroad client windows in Filter-KMTGuard.

Use this reference whenever the user explicitly asks for:

```text
Old School
old-school
Old School style
ستايل Old School
ديزاين Old School
```

The phrase **Old School** is a style selector. It must not be interpreted as
the Item Mall style or as a request to restore an arbitrary older revision.

## Style Routing

The project has two separate named client UI styles:

| Requested style | Required reference | Visual family |
| --- | --- | --- |
| `Item Mall` | `docs/client_ui_design_reference.md` | Item Mall chrome, panels, and buttons |
| `Old School` | This document | Classic Silkroad frame, inventory panels, and `ifcommon` controls |

Do not mix the two visual families in one window. In particular:

- Do not place Item Mall panels or buttons inside an Old School frame.
- Do not place classic `ifcommon` form controls inside an Item Mall window
  unless an existing window-specific reference explicitly requires them.
- A window-specific reference still takes precedence over either general
  named style.

If the user does not name a style, preserve the existing window's visual
family. For a brand-new window with no requested style, follow the general
Item Mall reference.

## Canonical Implementation

The Secondary Password window completed on 2026-07-30 is the canonical Old
School implementation.

Primary implementation files:

```text
JTClientLibrary/source/libs/ClientLib/src/SecondPW/IFSecondaryPassword.h
JTClientLibrary/source/libs/ClientLib/src/SecondPW/IFSecondaryPassword.cpp
JTClientLibrary/clientlibrary/resinfo/ifsecondarypassword.txt
```

Use these files as the source of truth for chrome, panel hierarchy, form
fields, button dimensions, close-button alignment, spacing, colors, and
absolute child positioning.

Reuse the visual system, not Secondary Password behavior. Do not copy its
packets, passcode validation, modes, focus rules, or security behavior into an
unrelated window.

## Visual Signature

An Old School window must look like a native compact Silkroad utility window:

- Classic `mframe_wnd_` title frame.
- One inset inventory-style framed panel.
- Dark native tiled body surface.
- Black recessed form fields with thin native borders.
- Compact `76 x 24` classic action buttons.
- A small native `16 x 16` close button centered in the title bar.
- White body text, warm gold labels, and restrained muted help text.
- Tight, deliberate spacing with no decorative marketing copy.
- A footer action row inside the outer window.

Avoid:

- Item Mall chrome, headers, panels, buttons, or close controls.
- Large glossy buttons.
- Cyan selection bars or Mall list-row textures.
- Flat Win32, HTML, ImGui, or desktop-style controls.
- Custom bitmap art when the native classic Silkroad assets already exist.
- Oversized windows with large unused regions.
- Moving individual controls with unadjusted local coordinates after the
  parent window has been centered.

## Canonical Media

Use the following existing game assets.

Outer frame:

```text
interface\frame\mframe_wnd_
```

Inset content frame:

```text
interface\inventory\int_window_
```

Main panel surface:

```text
interface\ifcommon\bg_tile\com_bg_tile_b.ddj
```

Recessed field fill:

```text
interface\ifcommon\bg_tile\com_bg_tile_e.ddj
```

Resinfo-driven recessed field border:

```text
interface\ifcommon\com_blacksquare_
```

Programmatic recessed field surface:

```text
interface\ifcommon\com_grad_gage_form.ddj
```

Action buttons:

```text
interface\ifcommon\com_button.ddj
```

Close button:

```text
interface\ifcommon\com_windowclose.ddj
```

These assets belong to the base client media. Do not duplicate or rename them.

## Canonical Window Chrome

Use `CIFMainFrame` with the classic main-frame texture:

```cpp
CIFMainFrame::OnCreate(ln);
TB_Func_13("interface\\frame\\mframe_wnd_", 0, 1);
SetGWndSize(windowWidth, windowHeight);
```

The close button is a precise part of the title chrome:

```cpp
m_pCloseBtn->TB_Func_13(
    "interface\\ifcommon\\com_windowclose.ddj", 0, 0);
m_pCloseBtn->SetGWndSize(16, 16);
m_pCloseBtn->MoveGWnd(
    GetPos().x + windowWidth - 26,
    GetPos().y + 9);
m_pCloseBtn->ShowGWnd(true);
m_pCloseBtn->BringToFront();
```

Required close-button rules:

- Size is exactly `16 x 16`.
- Right inset is `10` pixels: `windowWidth - 26`.
- Top offset is `9` pixels so the button is vertically centered in the
  classic title frame.
- Position it after the window size is final.
- Use the standard `com_windowclose.ddj`, not
  `com_d_windowclose.ddj` and not an Item Mall close texture.

## Canonical Compact Layout

The Secondary Password reference window is `430 x 300`.

```text
Window                         0,   0, 430, 300
Inset frame                    7,  37, 416, 220
Panel background              20,  49, 390, 195
Section title                 28,  52, 374,  20
Description                   28,  76, 374,  32

Field 1 outer border         172, 112, 220,  28
Field 1 inner tile          176, 116, 212,  20
Field 1 label                38, 117, 126,  18
Field 1 edit                181, 117, 202,  18

Field 2 outer border         172, 148, 220,  28
Field 2 inner tile          176, 152, 212,  20
Field 2 label                38, 153, 126,  18
Field 2 edit                181, 153, 202,  18

Field 3 outer border         172, 184, 220,  28
Field 3 inner tile          176, 188, 212,  20
Field 3 label                38, 189, 126,  18
Field 3 edit                181, 189, 202,  18

Hint                           34, 218, 362,  18
Remember checkbox             104, 219,  16,  16
Remember label                126, 217, 200,  20
Action row Y                           266
Action button size                  76,  24
```

For a two-button action row in a `430`-pixel window:

```text
First button                  135, 266, 76, 24
Second button                 219, 266, 76, 24
```

For a three-button action row:

```text
First button                   93, 266, 76, 24
Second button                 177, 266, 76, 24
Third button                  261, 266, 76, 24
```

These positions produce consistent `8`-pixel gaps and a centered group.

For other window sizes, preserve these relationships:

- `7`-pixel outer inset for the inventory frame.
- `13`-pixel inset from the frame to the dark panel surface.
- `8` to `14` pixels between related controls.
- `24`-pixel classic button height.
- Action buttons centered as one group.
- Labels aligned as one column and fields aligned as one column.
- The action row remains inside the outer window and below the inset panel.

## Classic Field Construction

For a resinfo-driven window, every editable field is a three-layer native
control:

1. `CIFStretchWnd` using `com_blacksquare_`.
2. `CIFNormalTile` inset by `4` pixels using `com_bg_tile_e.ddj`.
3. `CIFEdit` inset again and brought to the front.

The canonical geometry is:

```text
Outer border      x,     y,     220, 28
Inner tile        x + 4, y + 4, 212, 20
Edit              x + 9, y + 5, 202, 18
```

Decorative borders and tiles must not consume clicks:

```cpp
fieldBorder->SetClickable(false);
fieldTile->SetClickable(false);
```

Do not construct `CIFStretchWnd` with `com_blacksquare_` by calling
`TB_Func_13` directly from C++. That bypasses the resinfo stretch setup and
renders as a thick white frame on the supported client.

For a programmatic C++ window, use the proven native two-layer field:

1. `CIFStatic` using `com_grad_gage_form.ddj`.
2. `CIFEdit` inset over it and brought to the front.

```text
Field surface     x,     y + 2, 220, 24
Edit              x + 7, y + 5, 206, 18
```

The field surface must be non-clickable. For large recessed work areas, use
an inset `CIFFrame` with `int_window_` around a dark `CIFNormalTile`; do not
stretch the small field surface vertically.

Set an edit's text area width explicitly. A visually wide edit can otherwise
accept only one character in this client:

```cpp
edit->SetTextmode(edit->GetSize().width - 8);
```

## Canonical Color System

Use this palette as the Old School default:

```cpp
const D3DCOLOR COLOR_ACCENT =
    D3DCOLOR_ARGB(255, 255, 216, 117);
const D3DCOLOR COLOR_LABEL =
    D3DCOLOR_ARGB(255, 238, 215, 168);
const D3DCOLOR COLOR_TEXT =
    D3DCOLOR_ARGB(255, 255, 255, 255);
const D3DCOLOR COLOR_MUTED =
    D3DCOLOR_ARGB(255, 198, 190, 174);
const D3DCOLOR COLOR_SELECTED =
    D3DCOLOR_ARGB(255, 255, 244, 205);
const D3DCOLOR COLOR_WARNING =
    D3DCOLOR_ARGB(255, 255, 132, 118);
```

Semantic usage:

```text
ACCENT    Section heading
LABEL     Field labels and secondary controls
TEXT      Title, body text, edits, and button labels
MUTED     Hints and neutral status text
SELECTED  Focused field label
WARNING   Validation and failure messages only
```

Do not recolor the classic textures. Let the native DDJ surfaces provide the
window's material and depth.

## Positioning Rule

`MoveGWnd` uses absolute screen coordinates for child controls when it is
called after the parent has been positioned. Always add the parent position:

```cpp
button->MoveGWnd(
    GetPos().x + localButtonX,
    GetPos().y + localButtonY);
```

Never do this after centering the parent:

```cpp
button->MoveGWnd(localButtonX, localButtonY);
```

The incorrect form can place buttons outside the window at the left side of
the screen.

The canonical window position is horizontally centered and slightly above the
old lower login-screen offset:

```cpp
int x = (screenWidth - windowWidth) / 2;
int y = ((screenHeight - windowHeight) / 2) + 10;
```

Clamp the final position to the active resolution so the title bar, content,
and footer remain visible.

## Resinfo Ownership and Layering

The canonical Old School window is resinfo-driven. For an existing
resinfo-driven window:

- Keep geometry and control types in its resinfo file.
- Create decorative controls before labels, edits, and buttons.
- Use unique child IDs.
- Keep the exact `Interface Text` header and valid section syntax.
- Keep opening and closing brace counts balanced.
- Check every required pointer before use.

Recommended creation order:

1. Inset frame.
2. Main tiled panel.
3. Section title and description.
4. Field borders, field surfaces, and inner tiles as required by ownership.
5. Field labels and edits.
6. Hint, checkbox, and status controls.
7. Action buttons.
8. Modal child message box.

Do not mix programmatic and resinfo geometry for the same static control unless
runtime modes genuinely require repositioning.

## Button State Rules

Classic Old School actions use:

```text
interface\ifcommon\com_button.ddj
```

Keep the button visible and validate when pressed when the base client does
not provide a visually consistent disabled surface. Do not allow an enabled
button to turn into a flat white rectangle.

All action labels must remain centered. Button state changes must not change
button size, position, or spacing.

## Behavior and Crash Prevention

Visual work must not change behavior contracts.

Do not change packets, opcodes, Filter handlers, SQL, validation rules,
security behavior, focus behavior, or request timing merely to apply Old
School styling.

For keyboard edits:

- Numeric filtering must add each digit exactly once.
- Masked fields must preserve the real underlying value.
- `Backspace`, `Delete`, `Tab`, and `Enter` must retain their documented
  behavior.
- Focus changes must target the native edit handle when required.

Every resinfo object retrieved in C++ must be checked before it is used.

## Review Checklist

Before delivering an Old School window, verify:

```text
[ ] The user explicitly requested Old School, or the existing window already uses it.
[ ] No Item Mall texture is referenced.
[ ] The title frame uses mframe_wnd_.
[ ] The content uses int_window_ and ifcommon tiles.
[ ] Fields use black border, inset tile, and edit layers.
[ ] Buttons use com_button.ddj and remain inside the window.
[ ] Two- and three-button groups are centered with stable gaps.
[ ] The close button is com_windowclose.ddj at 16 x 16.
[ ] The close button is vertically centered at title-bar offset 9.
[ ] Child MoveGWnd calls include the parent window position.
[ ] The window fits at 800 x 600 and is clamped to the active resolution.
[ ] Decorative controls are not clickable.
[ ] All required pointers are checked.
[ ] Resinfo IDs are unique and braces are balanced.
[ ] Opening, closing, focusing, typing, and submitting do not crash.
[ ] Existing behavior, packets, SQL, and Filter logic are unchanged.
[ ] The client DLL builds successfully when code changed.
[ ] Final outputs are copied to D:\KMTGuard-build when a build is required.
```
