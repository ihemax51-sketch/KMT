#include "IFDropLogsWnd.h"

#include "ClientNet/MsgStreamBuffer.h"
#include <GFXMainFrame/Controler.h>
#include "GInterface.h"
#include "Game.h"
#include "GlobalDataManager.h"
#include "IFSlotWithHelp.h"
#include "SOItem.h"
#include <BSLib/multibyte.h>
#include <Windows.h>
#include <cstdio>

#define GDR_DROPLOG_MAINFRAME 13432
#define GDR_DROPLOG_OPEN_ICON 13791
#define GDR_DROPLOG_BTN_CLOSE 100
#define GDR_DROPLOG_EDIT_SEARCH 101
#define GDR_DROPLOG_EDIT_SEARCH_BG 601
#define GDR_DROPLOG_BTN_SEARCH 102
#define GDR_DROPLOG_BTN_REFRESH 103
#define GDR_DROPLOG_FILTER_ALL 104
#define GDR_DROPLOG_FILTER_CH 105
#define GDR_DROPLOG_FILTER_EU 106
#define GDR_DROPLOG_BTN_VIEW 107
#define GDR_DROPLOG_BTN_PREV 108
#define GDR_DROPLOG_BTN_NEXT 109
#define GDR_DROPLOG_PAGE_TEXT 110
#define GDR_DROPLOG_NO_DATA_TEXT 111
#define GDR_DROPLOG_TOTAL_TEXT 112
#define GDR_DROPLOG_HEADER_ITEM 120
#define GDR_DROPLOG_HEADER_PLAYER 121
#define GDR_DROPLOG_HEADER_PLUS 122
#define GDR_DROPLOG_HEADER_INFO 123
#define GDR_DROPLOG_HEADER_MONSTER 124
#define GDR_DROPLOG_HEADER_DATE 125
#define GDR_DROPLOG_HEADER_LOCATION 126
#define GDR_DROPLOG_TABLE_BG 127
#define GDR_DROPLOG_BODY_BG 129
#define GDR_DROPLOG_BACKGROUND (GDR_DROPLOG_MAINFRAME + 1)
#define GDR_DROPLOG_HEADER_BAR_BASE 2000
#define GDR_DROPLOG_ROW_BASE 3000
#define GDR_DROPLOG_ROW_BAR_BASE 4000
#define GDR_DROPLOG_ITEM_HELP_SLOT 13434

namespace {
const int COL_COUNT = 7;
const int WINDOW_WIDTH = 760;
const int WINDOW_HEIGHT = 420;
const int GDR_DROPLOG_LABEL_SEARCH = 1000;
const int GDR_DROPLOG_LABEL_FILTER = 1001;
const int COL_MAX_CHARS[COL_COUNT] = {0, 32, 0, 0, 0, 0, 0};
const wchar_t* COL_TEXT_KEYS[COL_COUNT] = {
    L"UIIT_KMT_ITEM", L"UIIT_KMT_PLAYER", L"", L"", L"", L"", L""
};
const D3DCOLOR COLOR_HEADER = D3DCOLOR_ARGB(255, 255, 216, 83);
const D3DCOLOR COLOR_LABEL = D3DCOLOR_ARGB(255, 239, 218, 164);
const D3DCOLOR COLOR_TEXT = D3DCOLOR_ARGB(255, 245, 242, 232);
const D3DCOLOR COLOR_MUTED = D3DCOLOR_ARGB(255, 205, 196, 176);
const D3DCOLOR COLOR_SELECTED = D3DCOLOR_ARGB(255, 255, 236, 170);
const D3DCOLOR COLOR_GREEN = D3DCOLOR_ARGB(255, 114, 255, 114);
const int ROW_ICON_ID = 1;
const int ROW_TEXT_IDS[COL_COUNT] = {2, 3, 4, 5, 6, 7, 8};

void StyleText(CIFStatic* control, const wchar_t* text, D3DCOLOR color, CTextBoard::eJustifyHorizontal justify)
{
    if (!control) {
        return;
    }
    control->SetText(text);
    control->SetFont(theApp.GetFont(0));
    control->m_FontTexture.SetColor(color);
    control->JustifyHorizontal(justify);
    control->JustifyVertical(CTextBoard::JUSTIFY_MIDDLE);
    control->ShowGWnd(true);
    control->BringToFront();
}

void StyleButton(CIFButton* button, const wchar_t* text)
{
    if (!button) {
        return;
    }
    button->TB_Func_13("interface\\mall\\mall_coice_button.ddj", 1, 1);
    button->SetText(text);
    button->SetFont(theApp.GetFont(0));
    button->m_FontTexture.SetColor(COLOR_TEXT);
    button->ShowGWnd(true);
    button->BringToFront();
}

void WritePreviewItemPayload(CMsgStreamBuffer& packet, const SItemData& data)
{
    const u_short typeID2 = data.m_typeId.getTypeID2();
    const u_short typeID3 = data.m_typeId.getTypeID3();
    const u_short typeID4 = data.m_typeId.getTypeID4();

    switch (typeID2) {
        case 1:
            packet << UINT8(0) << UINT64(0) << UINT32(1) << UINT8(0) << UINT8(1) << UINT8(0) << UINT8(2) << UINT8(0);
            break;
        case 2:
            switch (typeID3) {
                case 1:
                    packet << UINT8(0x01);
                    break;
                case 2:
                    packet << UINT32(0x00);
                    break;
                default:
                    if (typeID4 == 3) {
                        packet << UINT32(0x01);
                    }
                    break;
            }
            break;
        case 3:
            packet << UINT16(0x01);
            if (typeID3 == 11) {
                if (typeID4 == 1 || typeID4 == 2) {
                    packet << UINT8(0x00);
                }
            } else if (typeID3 == 14 && typeID4 == 2) {
                packet << UINT8(0x00);
            }
            break;
    }
}
}

GFX_IMPLEMENT_DYNCREATE(CIFDropLogsWnd, CIFMainFrame)
GFX_BEGIN_MESSAGE_MAP(CIFDropLogsWnd, CIFMainFrame)
                    ONG_COMMAND(GDR_DROPLOG_BTN_CLOSE, &CIFDropLogsWnd::OnClickClose)
                    ONG_COMMAND(GDR_DROPLOG_BTN_SEARCH, &CIFDropLogsWnd::OnClickSearch)
                    ONG_COMMAND(GDR_DROPLOG_BTN_REFRESH, &CIFDropLogsWnd::OnClickRefresh)
                    ONG_COMMAND(GDR_DROPLOG_FILTER_ALL, &CIFDropLogsWnd::OnClickFilterAll)
                    ONG_COMMAND(GDR_DROPLOG_FILTER_CH, &CIFDropLogsWnd::OnClickFilterCH)
                    ONG_COMMAND(GDR_DROPLOG_FILTER_EU, &CIFDropLogsWnd::OnClickFilterEU)
                    ONG_COMMAND(GDR_DROPLOG_BTN_VIEW, &CIFDropLogsWnd::OnClickView)
                    ONG_COMMAND(GDR_DROPLOG_BTN_PREV, &CIFDropLogsWnd::OnClickPrevPage)
                    ONG_COMMAND(GDR_DROPLOG_BTN_NEXT, &CIFDropLogsWnd::OnClickNextPage)
                    ONG_COMMAND(GDR_DROPLOG_ROW_BASE + 0, &CIFDropLogsWnd::OnClickRow0)
                    ONG_COMMAND(GDR_DROPLOG_ROW_BASE + 20, &CIFDropLogsWnd::OnClickRow1)
                    ONG_COMMAND(GDR_DROPLOG_ROW_BASE + 40, &CIFDropLogsWnd::OnClickRow2)
                    ONG_COMMAND(GDR_DROPLOG_ROW_BASE + 60, &CIFDropLogsWnd::OnClickRow3)
                    ONG_COMMAND(GDR_DROPLOG_ROW_BASE + 80, &CIFDropLogsWnd::OnClickRow4)
                    ONG_COMMAND(GDR_DROPLOG_ROW_BASE + 100, &CIFDropLogsWnd::OnClickRow5)
                    ONG_COMMAND(GDR_DROPLOG_ROW_BASE + 120, &CIFDropLogsWnd::OnClickRow6)
                    ONG_COMMAND(GDR_DROPLOG_ROW_BASE + 140, &CIFDropLogsWnd::OnClickRow7)
                    ONG_COMMAND(GDR_DROPLOG_ROW_BASE + 160, &CIFDropLogsWnd::OnClickRow8)
                    ONG_COMMAND(GDR_DROPLOG_ROW_BASE + 180, &CIFDropLogsWnd::OnClickRow9)
GFX_END_MESSAGE_MAP()

GFX_IMPLEMENT_DYNCREATE(CIFDropLogsGuide, CIFDecoratedStatic)

CIFDropLogsWnd::CIFDropLogsWnd()
    : m_background(0), m_searchLabel(0), m_searchEdit(0), m_searchButton(0), m_refreshButton(0),
      m_filterAllButton(0), m_filterCHButton(0), m_filterEUButton(0), m_viewButton(0), m_closeButton(0),
      m_tableBackground(0), m_prevButton(0), m_nextButton(0), m_pageText(0), m_totalText(0), m_noDataText(0),
      m_currentPage(1), m_pageSize(10), m_totalCount(0), m_totalPages(1), m_selectedRow(-1),
      m_filter(0), m_isLoading(false), m_lastMouseDown(false), m_requestTick(0)
{
    for (int i = 0; i < COL_COUNT; ++i) {
        m_headerBars[i] = 0;
        m_headers[i] = 0;
    }
    for (int row = 0; row < DROPLOG_VISIBLE_ROWS; ++row) {
        m_rowBars[row] = 0;
        m_rowButtons[row] = 0;
        m_itemIcons[row] = 0;
        for (int col = 0; col < COL_COUNT; ++col) {
            m_rowTexts[row][col] = 0;
        }
    }
}

CIFDropLogsWnd::~CIFDropLogsWnd() {
}

bool CIFDropLogsWnd::OnCreate(long ln) {
    CIFMainFrame::OnCreate(ln);

    TB_Func_13("interface\\frame\\mall_sub_wnd04_", 0, 0);
    SetText(KmtGetText(L"UIIT_KMT_DROP_LOGS"));
    SetGWndSize(WINDOW_WIDTH, WINDOW_HEIGHT);

    m_IRM.LoadFromFile("clientlibrary\\resinfo\\ifdroplogswnd.txt");
    m_IRM.CreateInterfaceSection("Create", this);

    m_background = m_IRM.GetResObj<CIFNormalTile>(GDR_DROPLOG_BACKGROUND, 1);
    m_closeButton = m_IRM.GetResObj<CIFButton>(GDR_DROPLOG_BTN_CLOSE, 1);
    CIFStatic* searchEditBackground = m_IRM.GetResObj<CIFStatic>(GDR_DROPLOG_EDIT_SEARCH_BG, 1);
    m_searchLabel = m_IRM.GetResObj<CIFStatic>(GDR_DROPLOG_LABEL_SEARCH, 1);
    m_searchEdit = m_IRM.GetResObj<CIFEdit>(GDR_DROPLOG_EDIT_SEARCH, 1);
    m_searchButton = m_IRM.GetResObj<CIFButton>(GDR_DROPLOG_BTN_SEARCH, 1);
    m_refreshButton = m_IRM.GetResObj<CIFButton>(GDR_DROPLOG_BTN_REFRESH, 1);
    m_filterAllButton = m_IRM.GetResObj<CIFButton>(GDR_DROPLOG_FILTER_ALL, 1);
    m_filterCHButton = m_IRM.GetResObj<CIFButton>(GDR_DROPLOG_FILTER_CH, 1);
    m_filterEUButton = m_IRM.GetResObj<CIFButton>(GDR_DROPLOG_FILTER_EU, 1);
    m_viewButton = m_IRM.GetResObj<CIFButton>(GDR_DROPLOG_BTN_VIEW, 1);
    m_tableBackground = m_IRM.GetResObj<CIFStatic>(GDR_DROPLOG_TABLE_BG, 1);
    m_noDataText = m_IRM.GetResObj<CIFStatic>(GDR_DROPLOG_NO_DATA_TEXT, 1);
    m_totalText = m_IRM.GetResObj<CIFStatic>(GDR_DROPLOG_TOTAL_TEXT, 1);
    m_prevButton = m_IRM.GetResObj<CIFButton>(GDR_DROPLOG_BTN_PREV, 1);
    m_pageText = m_IRM.GetResObj<CIFStatic>(GDR_DROPLOG_PAGE_TEXT, 1);
    m_nextButton = m_IRM.GetResObj<CIFButton>(GDR_DROPLOG_BTN_NEXT, 1);
    CIFNormalTile* bodyBackground = m_IRM.GetResObj<CIFNormalTile>(GDR_DROPLOG_BODY_BG, 1);

    if (m_closeButton) {
        m_closeButton->ShowGWnd(false);
    }
    if (m_pCloseBtn) {
        m_pCloseBtn->TB_Func_13("interface\\ifcommon\\com_d_windowclose.ddj", 0, 0);
        m_pCloseBtn->FUN_00656590("interface\\ifcommon\\com_d_windowclose_press.ddj");
        m_pCloseBtn->ShowGWnd(true);
        m_pCloseBtn->BringToFront();
    }
    if (m_background) {
        m_background->TB_Func_13("interface\\ifcommon\\bg_tile\\com_bg_tile_m.ddj", 0, 1);
        m_background->ShowGWnd(true);
    }
    if (bodyBackground) {
        bodyBackground->ShowGWnd(false);
    }
    if (m_tableBackground) {
        m_tableBackground->TB_Func_13("interface\\ifcommon\\com_blacksquare_", 0, 0);
        m_tableBackground->SetClickable(false);
        m_tableBackground->ShowGWnd(true);
    }
    StyleText(m_searchLabel, KmtGetText(L"UIIT_KMT_SEARCH"), COLOR_LABEL, CTextBoard::JUSTIFY_LEFT);
    if (searchEditBackground) {
        searchEditBackground->TB_Func_13("interface\\ifcommon\\com_grad_gage_form.ddj", 0, 0);
        searchEditBackground->ShowGWnd(true);
        searchEditBackground->BringToFront();
    }
    StyleButton(m_searchButton, KmtGetText(L"UIIT_KMT_SEARCH"));
    StyleButton(m_refreshButton, KmtGetText(L"UIIT_KMT_REFRESH"));
    StyleText(m_IRM.GetResObj<CIFStatic>(GDR_DROPLOG_LABEL_FILTER, 1), KmtGetText(L"UIIT_KMT_FILTER"), COLOR_LABEL, CTextBoard::JUSTIFY_LEFT);
    StyleButton(m_filterAllButton, KmtGetText(L"UIIT_KMT_ALL"));
    StyleButton(m_filterCHButton, KmtGetText(L"UIIT_KMT_CH"));
    StyleButton(m_filterEUButton, KmtGetText(L"UIIT_KMT_EU"));
    StyleButton(m_viewButton, KmtGetText(L"UIIT_KMT_VIEW"));
    if (m_prevButton) {
        m_prevButton->TB_Func_13("interface\\ifcommon\\com_left_arrow.ddj", 0, 1);
        m_prevButton->SetText(L"");
        m_prevButton->ShowGWnd(true);
        m_prevButton->BringToFront();
    }
    if (m_nextButton) {
        m_nextButton->TB_Func_13("interface\\ifcommon\\com_right_arrow.ddj", 0, 1);
        m_nextButton->SetText(L"");
        m_nextButton->ShowGWnd(true);
        m_nextButton->BringToFront();
    }
    if (m_searchEdit) {
        m_searchEdit->SetMaxLength(64);
        m_searchEdit->SetTextmode(m_searchEdit->GetSize().width);
        m_searchEdit->SetFont(theApp.GetFont(0));
        m_searchEdit->m_FontTexture.SetColor(COLOR_TEXT);
        m_searchEdit->ShowGWnd(true);
        m_searchEdit->BringToFront();
    }

    for (int col = 0; col < COL_COUNT; ++col) {
        m_headerBars[col] = m_IRM.GetResObj<CIFBarWnd>(GDR_DROPLOG_HEADER_BAR_BASE + col, 1);
        if (m_headerBars[col]) {
            m_headerBars[col]->TB_Func_13("clientlibrary\\mall\\com_bar05_", 1, 1);
            m_headerBars[col]->SetClickable(false);
            m_headerBars[col]->ShowGWnd(col < 2);
        }
        m_headers[col] = m_IRM.GetResObj<CIFStatic>(GDR_DROPLOG_HEADER_ITEM + col, 1);
        if (m_headers[col]) {
            m_headers[col]->TB_Func_13("", 0, 0);
            StyleText(
                m_headers[col],
                COL_TEXT_KEYS[col][0] ? KmtGetText(COL_TEXT_KEYS[col]) : L"",
                COLOR_HEADER,
                CTextBoard::JUSTIFY_CENTER);
            m_headers[col]->ShowGWnd(col < 2);
            if (col < 2) {
                m_headers[col]->BringToFront();
            }
        }
    }

    for (int row = 0; row < DROPLOG_VISIBLE_ROWS; ++row) {
        const int rowBase = GDR_DROPLOG_ROW_BASE + (row * 20);
        m_rowBars[row] = m_IRM.GetResObj<CIFBarWnd>(GDR_DROPLOG_ROW_BAR_BASE + row, 1);
        if (m_rowBars[row]) {
            m_rowBars[row]->TB_Func_13(GetRowTexture(row), row == m_selectedRow ? 1 : 0, row == m_selectedRow ? 1 : 0);
            m_rowBars[row]->SetClickable(false);
            m_rowBars[row]->ShowGWnd(true);
        }
        m_rowButtons[row] = m_IRM.GetResObj<CIFButton>(rowBase, 1);
        if (m_rowButtons[row]) {
            m_rowButtons[row]->TB_Func_13(GetRowTexture(row), row == m_selectedRow ? 1 : 0, row == m_selectedRow ? 1 : 0);
            m_rowButtons[row]->SetText(L"");
            m_rowButtons[row]->ShowGWnd(false);
        }

        m_itemIcons[row] = m_IRM.GetResObj<CIFStatic>(rowBase + ROW_ICON_ID, 1);
        if (m_itemIcons[row]) {
            m_itemIcons[row]->SetClickable(true);
            m_itemIcons[row]->ShowGWnd(false);
        }

        for (int col = 0; col < COL_COUNT; ++col) {
            m_rowTexts[row][col] = m_IRM.GetResObj<CIFStatic>(rowBase + ROW_TEXT_IDS[col], 1);
            StyleText(m_rowTexts[row][col], L"", COLOR_TEXT,
                      col == 2 ? CTextBoard::JUSTIFY_CENTER : CTextBoard::JUSTIFY_LEFT);
            if (m_rowTexts[row][col]) {
                m_rowTexts[row][col]->SetClickable(col == 1);
                m_rowTexts[row][col]->ShowGWnd(false);
                if (col == 1) {
                    m_rowTexts[row][col]->BringToFront();
                }
            }
        }
    }

    StyleText(m_noDataText, KmtGetText(L"UIIT_KMT_NO_DROP_LOGS_FOUND"), COLOR_HEADER, CTextBoard::JUSTIFY_CENTER);
    StyleText(m_totalText, KmtGetText(L"UIIT_KMT_TOTAL_0"), COLOR_MUTED, CTextBoard::JUSTIFY_LEFT);
    StyleText(m_pageText, KmtGetText(L"UIIT_KMT_PAGE_1_1"), D3DCOLOR_ARGB(255, 236, 226, 196), CTextBoard::JUSTIFY_CENTER);
    if (m_totalText) {
        m_totalText->SetClickable(false);
    }
    if (m_pageText) {
        m_pageText->SetClickable(false);
    }
    if (m_prevButton) {
        m_prevButton->BringToFront();
    }
    if (m_nextButton) {
        m_nextButton->BringToFront();
    }

    ClearRows();
    SetNoData(true);
    UpdateFilterButtons();
    UpdateWindowPos();
    ShowGWnd(false);
    return true;
}

void CIFDropLogsWnd::ShowGWnd(bool bVisible) {
    CIFMainFrame::ShowGWnd(bVisible);
    if (bVisible) {
        UpdateWindowPos();
        BringToFront();
        m_currentPage = 1;
        m_pageSize = 10;
        m_filter = 0;
        m_searchText = std::n_string();
        if (m_searchEdit) {
            m_searchEdit->SetText(L"");
        }
        RequestDropLogs();
    }
}

void CIFDropLogsWnd::OnUpdate() {
    CIFMainFrame::OnUpdate();

    if (IsVisible()) {
        const bool mouseDown = (GetKeyState(VK_LBUTTON) & 0x8000) != 0;
        if (m_lastMouseDown && !mouseDown) {
            CGWndBase* target = g_CurrentIfUnderCursor;
            int id = target ? target->UniqueID() : -1;

            if ((id < GDR_DROPLOG_ROW_BASE || id >= GDR_DROPLOG_ROW_BASE + DROPLOG_VISIBLE_ROWS * 20) &&
                (id < GDR_DROPLOG_ROW_BAR_BASE || id >= GDR_DROPLOG_ROW_BAR_BASE + DROPLOG_VISIBLE_ROWS) &&
                g_pOnMouseDownClickCtrl) {
                id = g_pOnMouseDownClickCtrl->UniqueID();
            }

            int row = -1;
            if (id >= GDR_DROPLOG_ROW_BAR_BASE && id < GDR_DROPLOG_ROW_BAR_BASE + DROPLOG_VISIBLE_ROWS) {
                row = id - GDR_DROPLOG_ROW_BAR_BASE;
            } else if (id >= GDR_DROPLOG_ROW_BASE && id < GDR_DROPLOG_ROW_BASE + DROPLOG_VISIBLE_ROWS * 20) {
                const int offset = id - GDR_DROPLOG_ROW_BASE;
                row = offset / 20;
            }

            if (row >= 0 && row < DROPLOG_VISIBLE_ROWS) {
                OnClickRow(row);
            }
        }
        m_lastMouseDown = mouseDown;
    }

    if (m_isLoading && GetTickCount() - m_requestTick > 8000) {
        m_isLoading = false;
        ClearRows();
        SetNoData(true);
        if (m_noDataText) {
            m_noDataText->SetText(KmtGetText(L"UIIT_KMT_FAILED_TO_LOAD_DROP_LOGS"));
            m_noDataText->m_FontTexture.SetColor(D3DCOLOR_ARGB(255, 255, 118, 118));
        }
    }
}

int CIFDropLogsWnd::OnMouseLeftUp(int a1, int x, int y) {
    return CIFMainFrame::OnMouseLeftUp(a1, x, y);
}

undefined1 CIFDropLogsWnd::OnCloseWnd() {
    ShowGWnd(false);
    return true;
}

void CIFDropLogsWnd::HandleDropLogsResponse(bool success, int totalCount, int page, int pageSize, const std::vector<DropLogEntry>& rows) {
    m_isLoading = false;
    m_totalCount = totalCount < 0 ? 0 : totalCount;
    m_currentPage = page < 1 ? 1 : page;
    m_pageSize = pageSize < 1 ? 10 : pageSize;
    m_totalPages = (m_totalCount + m_pageSize - 1) / m_pageSize;
    if (m_totalPages < 1) {
        m_totalPages = 1;
    }
    m_currentRows = rows;
    m_selectedRow = -1;

    ClearRows();
    if (!success) {
        SetNoData(true);
        if (m_noDataText) {
            m_noDataText->SetText(KmtGetText(L"UIIT_KMT_FAILED_TO_LOAD_DROP_LOGS"));
            m_noDataText->m_FontTexture.SetColor(D3DCOLOR_ARGB(255, 255, 118, 118));
        }
    } else {
        FillRows();
        SetNoData(m_currentRows.empty());
        if (m_noDataText && m_currentRows.empty()) {
            m_noDataText->SetText(KmtGetText(L"UIIT_KMT_NO_DROP_LOGS_FOUND"));
            m_noDataText->m_FontTexture.SetColor(COLOR_HEADER);
        }
    }
    UpdatePageText();
}

void CIFDropLogsWnd::OnClickClose() { OnCloseWnd(); }
void CIFDropLogsWnd::OnClickSearch() { m_currentPage = 1; m_searchText = GetSearchText(); RequestDropLogs(); }
void CIFDropLogsWnd::OnClickRefresh() { RequestDropLogs(); }
void CIFDropLogsWnd::OnClickFilterAll() { m_filter = 0; m_currentPage = 1; UpdateFilterButtons(); RequestDropLogs(); }
void CIFDropLogsWnd::OnClickFilterCH() { m_filter = 1; m_currentPage = 1; UpdateFilterButtons(); RequestDropLogs(); }
void CIFDropLogsWnd::OnClickFilterEU() { m_filter = 2; m_currentPage = 1; UpdateFilterButtons(); RequestDropLogs(); }
void CIFDropLogsWnd::OnClickPrevPage() { if (m_currentPage > 1) { --m_currentPage; RequestDropLogs(); } }
void CIFDropLogsWnd::OnClickNextPage() { if (m_currentPage < m_totalPages) { ++m_currentPage; RequestDropLogs(); } }
void CIFDropLogsWnd::OnClickRow0() { OnClickRow(0); }
void CIFDropLogsWnd::OnClickRow1() { OnClickRow(1); }
void CIFDropLogsWnd::OnClickRow2() { OnClickRow(2); }
void CIFDropLogsWnd::OnClickRow3() { OnClickRow(3); }
void CIFDropLogsWnd::OnClickRow4() { OnClickRow(4); }
void CIFDropLogsWnd::OnClickRow5() { OnClickRow(5); }
void CIFDropLogsWnd::OnClickRow6() { OnClickRow(6); }
void CIFDropLogsWnd::OnClickRow7() { OnClickRow(7); }
void CIFDropLogsWnd::OnClickRow8() { OnClickRow(8); }
void CIFDropLogsWnd::OnClickRow9() { OnClickRow(9); }

void CIFDropLogsWnd::OnClickView() {
    if (m_selectedRow < 0 || m_selectedRow >= (int)m_currentRows.size()) {
        ShowSystemMessage(KmtGetText(L"UIIT_KMT_PLEASE_SELECT_A_DROP_LOG_FIRST"));
        return;
    }

    ShowItemDetails(m_selectedRow);
}

void CIFDropLogsWnd::OnClickRow(int rowIndex) {
    if (rowIndex < 0 || rowIndex >= (int)m_currentRows.size()) {
        return;
    }
    SetSelectedRow(rowIndex);
    ShowItemDetails(rowIndex);
}

void CIFDropLogsWnd::UpdateWindowPos() {
    int width = 1024;
    int height = 768;
    if (g_CGame && g_CGame->GetRes().res) {
        width = g_CGame->GetRes().res->width;
        height = g_CGame->GetRes().res->height;
    }
    MoveGWnd((width - GetSize().width) / 2, (height - GetSize().height) / 2);
}

void CIFDropLogsWnd::RequestDropLogs() {
    m_searchText = GetSearchText();
    SetLoading(true);

    CMsgStreamBuffer request(0x169A);
    request << byte(33);
    request << INT32(m_currentPage);
    request << INT32(m_pageSize);
    request << m_searchText;
    request << byte(m_filter);
    SendMsg(request);
}

void CIFDropLogsWnd::ClearRows() {
    for (int row = 0; row < DROPLOG_VISIBLE_ROWS; ++row) {
        if (m_rowButtons[row]) {
            m_rowButtons[row]->ShowGWnd(false);
        }
        if (m_rowBars[row]) {
            m_rowBars[row]->TB_Func_13(GetRowTexture(row), 0, 0);
            m_rowBars[row]->ShowGWnd(true);
            m_rowBars[row]->BringToFront();
        }
        if (m_itemIcons[row]) {
            m_itemIcons[row]->TB_Func_13("", 0, 0);
            m_itemIcons[row]->ShowGWnd(false);
        }
        for (int col = 0; col < COL_COUNT; ++col) {
            if (m_rowTexts[row][col]) {
                m_rowTexts[row][col]->SetText(L"");
                m_rowTexts[row][col]->ShowGWnd(false);
            }
        }
    }
}

void CIFDropLogsWnd::FillRows() {
    const int count = m_currentRows.size() < DROPLOG_VISIBLE_ROWS ? (int)m_currentRows.size() : DROPLOG_VISIBLE_ROWS;
    for (int row = 0; row < DROPLOG_VISIBLE_ROWS; ++row) {
        if (m_rowBars[row]) {
            m_rowBars[row]->TB_Func_13(GetRowTexture(row), row == m_selectedRow ? 1 : 0, row == m_selectedRow ? 1 : 0);
            m_rowBars[row]->ShowGWnd(true);
        }
        if (m_rowButtons[row]) {
            m_rowButtons[row]->ShowGWnd(false);
        }
    }

    for (int row = 0; row < count; ++row) {
        const DropLogEntry& entry = m_currentRows[row];
        if (m_rowBars[row]) {
            m_rowBars[row]->ShowGWnd(true);
            m_rowBars[row]->TB_Func_13(GetRowTexture(row), row == m_selectedRow ? 1 : 0, row == m_selectedRow ? 1 : 0);
        }
        if (m_rowButtons[row]) {
            m_rowButtons[row]->ShowGWnd(false);
        }

        const SItemData* itemData = ResolveItemData(entry);
        if (m_itemIcons[row]) {
            if (itemData && !itemData->AssocFileIcon.empty()) {
                m_itemIcons[row]->TB_Func_13(itemData->AssocFileIcon.c_str(), 1, 1);
                m_itemIcons[row]->ShowGWnd(true);
                m_itemIcons[row]->BringToFront();
            } else {
                m_itemIcons[row]->TB_Func_13("", 0, 0);
                m_itemIcons[row]->ShowGWnd(false);
            }
        }

        std::n_wstring values[COL_COUNT];
        values[0] = L"";
        values[1] = entry.Player;

        for (int col = 0; col < COL_COUNT; ++col) {
            if (!m_rowTexts[row][col]) {
                continue;
            }
            if (col != 1) {
                m_rowTexts[row][col]->SetText(L"");
                m_rowTexts[row][col]->ShowGWnd(false);
                continue;
            }
            m_rowTexts[row][col]->SetText(TrimForColumn(values[col], COL_MAX_CHARS[col]).c_str());
            m_rowTexts[row][col]->m_FontTexture.SetColor(row == m_selectedRow ? COLOR_SELECTED : COLOR_TEXT);
            m_rowTexts[row][col]->ShowGWnd(true);
            m_rowTexts[row][col]->BringToFront();
        }
    }
}

void CIFDropLogsWnd::SetLoading(bool loading) {
    m_isLoading = loading;
    m_requestTick = GetTickCount();
    if (loading) {
        ClearRows();
        SetNoData(true);
        if (m_noDataText) {
            m_noDataText->SetText(KmtGetText(L"UIIT_KMT_LOADING"));
            m_noDataText->m_FontTexture.SetColor(COLOR_HEADER);
        }
    }
}

void CIFDropLogsWnd::SetNoData(bool noData) {
    if (m_noDataText) {
        m_noDataText->ShowGWnd(noData);
        if (noData) {
            m_noDataText->BringToFront();
        }
    }
}

void CIFDropLogsWnd::UpdatePageText() {
    if (m_pageText) {
        wchar_t page[40];
        swprintf(page, 40, KmtGetText(L"UIIT_KMT_PAGE_VALUE_VALUE"), m_currentPage, m_totalPages);
        m_pageText->SetText(page);
    }
    if (m_totalText) {
        wchar_t total[40];
        swprintf(total, 40, KmtGetText(L"UIIT_KMT_TOTAL_VALUE"), m_totalCount);
        m_totalText->SetText(total);
    }
    if (m_prevButton) m_prevButton->SetEnabledState(m_currentPage > 1);
    if (m_nextButton) m_nextButton->SetEnabledState(m_currentPage < m_totalPages);
    if (m_prevButton) m_prevButton->BringToFront();
    if (m_nextButton) m_nextButton->BringToFront();
}

void CIFDropLogsWnd::SetSelectedRow(int rowIndex) {
    m_selectedRow = rowIndex;
    FillRows();
}

void CIFDropLogsWnd::UpdateFilterButtons() {
    if (m_filterAllButton) m_filterAllButton->m_FontTexture.SetColor(m_filter == 0 ? COLOR_HEADER : COLOR_TEXT);
    if (m_filterCHButton) m_filterCHButton->m_FontTexture.SetColor(m_filter == 1 ? COLOR_HEADER : COLOR_TEXT);
    if (m_filterEUButton) m_filterEUButton->m_FontTexture.SetColor(m_filter == 2 ? COLOR_HEADER : COLOR_TEXT);
}

const char* CIFDropLogsWnd::GetRowTexture(int rowIndex) const {
    if (rowIndex == m_selectedRow) {
        return "interface\\ifcommon\\com_bar01select_";
    }

    return "interface\\ifcommon\\com_bar01_";
}

std::n_string CIFDropLogsWnd::GetSearchText() const {
    if (!m_searchEdit) {
        return std::n_string();
    }
    std::string value = TO_STRING(m_searchEdit->GetCurrentText());
    if (value.length() > 64) {
        value = value.substr(0, 64);
    }
    return std::n_string(value.c_str());
}

std::n_wstring CIFDropLogsWnd::TrimForColumn(const std::n_wstring& text, size_t maxChars) const {
    if (text.size() <= maxChars) {
        return text;
    }
    if (maxChars <= 3) {
        return text.substr(0, maxChars);
    }
    return text.substr(0, maxChars - 3) + L"...";
}

const SItemData* CIFDropLogsWnd::ResolveItemData(const DropLogEntry& entry) const {
    if (entry.RefObjId > 0) {
        const CItemData* item = g_CGlobalDataManager->GetItem(entry.RefObjId);
        if (item) {
            return &item->GetData();
        }
    }

    if (!entry.ItemCode.empty()) {
        return g_CGlobalDataManager->FindItemDataByCodeNameSafe(entry.ItemCode);
    }

    return 0;
}

CSOItem* CIFDropLogsWnd::CreatePreviewItem(const DropLogEntry& entry) const {
    const SItemData* data = ResolveItemData(entry);
    if (!data) {
        return 0;
    }

    const int refObjId = entry.RefObjId > 0 ? entry.RefObjId : data->RefObjectId;
    if (refObjId <= 0) {
        return 0;
    }

    CMsgStreamBuffer packet(0xB034);
    packet << INT32(0) << INT32(refObjId);
    WritePreviewItemPayload(packet, *data);

    CSOItem* itemInfo = new CSOItem();
    itemInfo->ReadFromPacket(&packet, 1);
    itemInfo->SetEnabled(true);
    itemInfo->m_quantity = 1;
    itemInfo->m_OptLevel = entry.PlusAmount > 0 ? (BYTE)entry.PlusAmount : 0;
    return itemInfo;
}

void CIFDropLogsWnd::ShowItemDetails(int rowIndex) {
    if (!g_pCGInterface || rowIndex < 0 || rowIndex >= (int)m_currentRows.size()) {
        return;
    }

    CSOItem* itemInfo = CreatePreviewItem(m_currentRows[rowIndex]);
    if (!itemInfo) {
        ShowSystemMessage(KmtGetText(L"UIIT_KMT_UNABLE_TO_LOAD_ITEM_DETAILS"));
        return;
    }

    CIFSlotWithHelp* itemLinkSlot = g_pCGInterface->GetGuiFromList<CIFSlotWithHelp>(GDR_DROPLOG_ITEM_HELP_SLOT);
    if (!itemLinkSlot) {
        RECT rect = {0, 0, 0, 0};
        itemLinkSlot = (CIFSlotWithHelp*)g_pCGInterface->CreateInstance(
            g_pCGInterface, GFX_RUNTIME_CLASS(CIFSlotWithHelp), rect, GDR_DROPLOG_ITEM_HELP_SLOT, 0);
        if (itemLinkSlot) {
            itemLinkSlot->ShowGWnd(false);
        }
    }

    if (!itemLinkSlot || !g_pCGInterface->m_helperWindow) {
        delete itemInfo;
        ShowSystemMessage(KmtGetText(L"UIIT_KMT_UNABLE_TO_LOAD_ITEM_DETAILS"));
        return;
    }

    g_pCGInterface->m_helperWindow->Reset();
    g_pCGInterface->m_helperWindow->ShowGWnd(true);
    itemLinkSlot->ShowGWnd(false);
    itemLinkSlot->ItemInfo = itemInfo;
    itemLinkSlot->SetType(70);
    itemLinkSlot->MoveGWnd(g_Controler->m_CursorPos.x + 25, g_Controler->m_CursorPos.y);
    itemLinkSlot->sub_686DB0();
}

void CIFDropLogsWnd::ShowSystemMessage(const wchar_t* message) {
    if (g_pCGInterface) {
        g_pCGInterface->ShowMessage_Warning(std::n_wstring(message));
    }
}

bool CIFDropLogsGuide::OnCreate(long ln) {
    CIFDecoratedStatic::OnCreate(ln);

    TB_Func_13("clientlibrary\\guides\\kmt_drop_logs_1.ddj", 0, 0);
    sub_634470("clientlibrary\\guides\\kmt_drop_logs_2.ddj");
    set_N00009BD4(2);
    set_N00009BD3(500);

    m_IRM.LoadFromFile("clientlibrary\\resinfo\\ifsimple.txt");
    m_IRM.CreateInterfaceSection("Create", this);
    CIFStatic* label = m_IRM.GetResObj<CIFStatic>(1, 0);
    if (label) {
        label->SetTooltip(KmtGetText(L"UIIT_KMT_DROP_LOGS"));
        label->SetStyleThingy(TOOLTIP);
    }
    return true;
}

int CIFDropLogsGuide::OnMouseLeftUp(int a1, int x, int y) {
    if (!g_pCGInterface) {
        return 0;
    }

    CIFDropLogsWnd* window = g_pCGInterface->GetGuiFromList<CIFDropLogsWnd>(DROP_LOGS_WINDOW_ID);
    if (window) {
        if (window->IsVisible()) {
            window->BringToFront();
        } else {
            window->ShowGWnd(true);
        }
        CGEffSoundBody::get()->PlaySound(L"snd_window_open");
    }
    return 0;
}

void CIFDropLogsGuide::OnCIFReady() {
    CIFDecoratedStatic::OnCIFReady();
    sub_633990();
}
