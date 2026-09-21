# Drop Logs UI Reference

This document is the stable reference for working on the Drop Logs client interface.
Read this before changing `CIFDropLogsWnd`.

## Goal

The Drop Logs window must look and behave like a native Silkroad interface, using the same style as Item Chest.

Do not redesign it with ImGui, Win32 controls, or ad hoc drawing.

## Source Files

Client window:

```text
JTClientLibrary/source/libs/ClientLib/src/CustomInterface/IFDropLogsWnd.h
JTClientLibrary/source/libs/ClientLib/src/CustomInterface/IFDropLogsWnd.cpp
```

Primary resinfo file:

```text
JTClientLibrary/media/clientlibrary/resinfo/ifdroplogswnd.txt
```

Runtime/media copy used for testing:

```text
JTClientLibrary/clientlibrary/resinfo/ifdroplogswnd.txt
```

Item Chest reference files:

```text
JTClientLibrary/clientlibrary/resinfo/ifchest.txt
JTClientLibrary/clientlibrary/resinfo/ifchestslot.txt
JTClientLibrary/source/libs/ClientLib/src/Guides/IFChest.cpp
JTClientLibrary/source/libs/ClientLib/src/Guides/IFChestSlot.cpp
```

## Hard Rules

Do not touch these when only working on UI polish:

```text
Packets
Opcodes
Filter side
SQL
Stored procedures
Drop log request/response parsing
Search/Refresh/Filter/Pagination logic
Open icon logic
```

Current packet values:

```text
Client request opcode: 0x169A
DropLogs subtype: 33
Filter/client response opcode: 0x2074
```

## Resinfo Loading Pattern

`CIFDropLogsWnd::OnCreate` loads the layout from resinfo:

```cpp
m_IRM.LoadFromFile("clientlibrary\\resinfo\\ifdroplogswnd.txt");
m_IRM.CreateInterfaceSection("Create", this);
```

Controls are then retrieved by ID:

```cpp
m_searchButton = m_IRM.GetResObj<CIFButton>(GDR_DROPLOG_BTN_SEARCH, 1);
m_pageText = m_IRM.GetResObj<CIFStatic>(GDR_DROPLOG_PAGE_TEXT, 1);
```

Keep positions and sizes in `ifdroplogswnd.txt` as much as possible.
Only use C++ for behavior, state, colors, and dynamic show/hide.

## Important IDs

Window and main controls:

```text
GDR_DROPLOG_MAINFRAME        13432
GDR_DROPLOG_OPEN_ICON        13791
GDR_DROPLOG_BTN_CLOSE        100
GDR_DROPLOG_EDIT_SEARCH      101
GDR_DROPLOG_BTN_SEARCH       102
GDR_DROPLOG_BTN_REFRESH      103
GDR_DROPLOG_FILTER_ALL       104
GDR_DROPLOG_FILTER_CH        105
GDR_DROPLOG_FILTER_EU        106
GDR_DROPLOG_BTN_VIEW         107
GDR_DROPLOG_BTN_PREV         108
GDR_DROPLOG_BTN_NEXT         109
GDR_DROPLOG_PAGE_TEXT        110
GDR_DROPLOG_NO_DATA_TEXT     111
GDR_DROPLOG_TOTAL_TEXT       112
GDR_DROPLOG_TABLE_BG         127
GDR_DROPLOG_BODY_BG          129
```

Headers:

```text
GDR_DROPLOG_HEADER_ITEM      120
GDR_DROPLOG_HEADER_PLAYER    121
GDR_DROPLOG_HEADER_PLUS      122
GDR_DROPLOG_HEADER_INFO      123
GDR_DROPLOG_HEADER_MONSTER   124
GDR_DROPLOG_HEADER_DATE      125
GDR_DROPLOG_HEADER_LOCATION  126
```

Header bars:

```text
GDR_DROPLOG_HEADER_BAR_BASE  2000
IDs: 2000 through 2006
```

Rows:

```text
GDR_DROPLOG_ROW_BASE         3000
GDR_DROPLOG_ROW_BAR_BASE     4000
```

Row ID formula:

```text
rowBase = 3000 + rowIndex * 20

rowBase + 0 = row button placeholder, normally hidden
rowBase + 2 = item text
rowBase + 3 = player text
rowBase + 4 = plus text
rowBase + 5 = info text
rowBase + 6 = monster text
rowBase + 7 = date text
rowBase + 8 = location text

row bar = 4000 + rowIndex
```

## Item Chest Style Mapping

Use the same textures and layout idea as Item Chest:

```text
Window frame: interface\\frame\\mall_sub_wnd04_
Main background: interface\\ifcommon\\bg_tile\\com_bg_tile_m.ddj
Table black area: interface\\ifcommon\\com_blacksquare_
Header bars: clientlibrary\\mall\\com_bar05_
Row bars: interface\\ifcommon\\com_bar01_
Selected row: interface\\ifcommon\\com_bar01select_
Buttons: interface\\mall\\mall_coice_button.ddj
Pagination arrows: interface\\ifcommon\\com_left_arrow.ddj
Pagination arrows: interface\\ifcommon\\com_right_arrow.ddj
```

## Layering Rules

This is the most important part.

Correct visual order:

```text
1. CIFMainFrame texture
2. Main background tile
3. Table blacksquare/frame
4. Header CIFBarWnd controls
5. Row CIFBarWnd controls
6. Row text CIFStatic controls
7. Pagination buttons and text
8. Close button
```

Do not draw `CIFButton` row backgrounds on top of the rows.
In this client, using `CIFButton` as a full row background can render as a white block and cover the text.

Current solution:

```text
Use CIFBarWnd for row visuals.
Keep row CIFButton controls hidden.
Detect row selection from mouse target IDs in OnUpdate.
```

The row bars should remain visible for all 10 rows so the table grid stays visible, even when there are fewer than 10 data rows.

## Row Click Selection

Do not rely on a visible full-row `CIFButton`.

The current safe approach:

```text
On mouse release, inspect g_CurrentIfUnderCursor and g_pOnMouseDownClickCtrl.
If the target ID is inside row text ID range or row bar ID range, calculate the row index.
Call OnClickRow(row).
```

This lets the player click row text or row background without adding a visible overlay that can break the design.

## Layout Rules

Use Item Chest proportions:

```text
Header height: 24
Row height: 24
Row vertical step: 23
```

The 23px step with 24px row height is intentional.
It gives the same tight grid/separator look as Item Chest.

Avoid adding extra large black overlays under the table.
If a background looks too dark or stacked, check for duplicated backgrounds:

```text
GDR_DROPLOG_BODY_BG
GDR_DROPLOG_TABLE_BG
Row bars
Row buttons
```

`GDR_DROPLOG_BODY_BG` is currently hidden from C++ to avoid the transparent/stacked background look.

## Known Pitfalls

Do not do this:

```text
Do not use ImGui.
Do not connect the client DLL to SQL.
Do not create a second close button.
Do not use CIFButton as a visible full-row background.
Do not put row bars above row text.
Do not let page arrows sit behind table backgrounds.
Do not leave multiple copied resinfo files out of sync.
Do not claim a UI fix is done before copying the resinfo into the runtime media path.
```

White row block symptom:

```text
Cause: visible full-row CIFButton using row texture.
Fix: hide row buttons and use CIFBarWnd rows only.
```

Missing clicks on rows:

```text
Cause: hidden/non-clickable row buttons or row bars consuming visual layers.
Fix: use mouse target detection in OnUpdate and map target IDs to row index.
```

Page arrows not clickable:

```text
Cause: arrows are behind another control or disabled because page is 1 / 1.
Fix: BringToFront after page text setup and after UpdatePageText.
Note: If TotalPages is 1, arrows being disabled is expected.
```

Transparent strip under title:

```text
Cause: background rect does not cover the body under the title or extra body background is visible.
Fix: make the main background cover the body and hide/remove BODY_BG if it stacks badly.
```

## Build And Copy

Build command:

```text
cmd /c "echo. | scripts\\Build_KMTGuardKit_DLL.cmd build"
```

After changing resinfo, copy it to the build output:

```text
C:\\Users\\Administrator\\Desktop\\KMTGuardKit_DLL_Build\\<timestamp>\\clientlibrary\\resinfo\\ifdroplogswnd.txt
```

Also keep this runtime media copy in sync:

```text
JTClientLibrary/clientlibrary/resinfo/ifdroplogswnd.txt
```

The DLL alone is not enough for UI layout changes.
The matching `ifdroplogswnd.txt` must be copied with it.

## Current Good Build

Last known good build after fixing row selection and white row background:

```text
C:\\Users\\Administrator\\Desktop\\KMTGuardKit_DLL_Build\\20260701_184723
```

DLL:

```text
JTClientLibrary/BinOut/RelWithDebInfo/KMTGuardKit.dll
```

## Testing Checklist

After every UI change:

```text
1. Open Drop Logs from the side icon.
2. Confirm the window opens once only.
3. Confirm there is no white block over rows.
4. Confirm the title area has no transparent missing strip.
5. Confirm header bars look like Item Chest.
6. Confirm 10 row separators are visible.
7. Confirm clicking a row selects/highlights it.
8. Confirm View sees the selected row.
9. Confirm page arrows are centered.
10. Confirm page arrows work when TotalPages > 1.
11. Confirm Search/Refresh/Filters still request data.
12. Confirm no packet/filter/SQL files changed.
13. Build successfully.
14. Copy matching resinfo to output media path.
```

