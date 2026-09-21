#pragma once

#include "IFButton.h"
#include "IFBarWnd.h"
#include "IFDecoratedStatic.h"
#include "IFEdit.h"
#include "IFMainFrame.h"
#include "IFNormalTile.h"
#include "IFStatic.h"
#include <vector>

#define DROP_LOGS_WINDOW_ID 13432
#define DROPLOG_VISIBLE_ROWS 10

struct SItemData;
class CSOItem;

struct DropLogEntry {
    std::n_wstring Player;
    std::n_wstring ItemCode;
    int RefObjId;
    int PlusAmount;
    std::n_wstring Monster;
    std::n_wstring Info;
    std::n_wstring Date;
    std::n_wstring Location;
};

class CIFDropLogsWnd : public CIFMainFrame {
    GFX_DECLARE_DYNCREATE(CIFDropLogsWnd)
    GFX_DECLARE_MESSAGE_MAP(CIFDropLogsWnd)

public:
    CIFDropLogsWnd();
    ~CIFDropLogsWnd();

    bool OnCreate(long ln) override;
    void ShowGWnd(bool bVisible) override;
    void OnUpdate() override;
    int OnMouseLeftUp(int a1, int x, int y) override;
    undefined1 OnCloseWnd() override;

    void HandleDropLogsResponse(bool success, int totalCount, int page, int pageSize, const std::vector<DropLogEntry>& rows);

private:
    void OnClickClose();
    void OnClickSearch();
    void OnClickRefresh();
    void OnClickPrevPage();
    void OnClickNextPage();
    void OnClickFilterAll();
    void OnClickFilterCH();
    void OnClickFilterEU();
    void OnClickView();
    void OnClickRow0();
    void OnClickRow1();
    void OnClickRow2();
    void OnClickRow3();
    void OnClickRow4();
    void OnClickRow5();
    void OnClickRow6();
    void OnClickRow7();
    void OnClickRow8();
    void OnClickRow9();
    void OnClickRow(int rowIndex);

    void UpdateWindowPos();
    void RequestDropLogs();
    void ClearRows();
    void FillRows();
    void SetLoading(bool loading);
    void SetNoData(bool noData);
    void UpdatePageText();
    void SetSelectedRow(int rowIndex);
    void UpdateFilterButtons();
    const char* GetRowTexture(int rowIndex) const;
    std::n_string GetSearchText() const;
    std::n_wstring TrimForColumn(const std::n_wstring& text, size_t maxChars) const;
    const SItemData* ResolveItemData(const DropLogEntry& entry) const;
    CSOItem* CreatePreviewItem(const DropLogEntry& entry) const;
    void ShowItemDetails(int rowIndex);
    void ShowSystemMessage(const wchar_t* message);

private:
    CIFNormalTile* m_background;
    CIFStatic* m_searchLabel;
    CIFEdit* m_searchEdit;
    CIFButton* m_searchButton;
    CIFButton* m_refreshButton;
    CIFButton* m_filterAllButton;
    CIFButton* m_filterCHButton;
    CIFButton* m_filterEUButton;
    CIFButton* m_viewButton;
    CIFButton* m_closeButton;
    CIFStatic* m_tableBackground;
    CIFBarWnd* m_headerBars[7];
    CIFStatic* m_headers[7];
    CIFBarWnd* m_rowBars[DROPLOG_VISIBLE_ROWS];
    CIFButton* m_rowButtons[DROPLOG_VISIBLE_ROWS];
    CIFStatic* m_itemIcons[DROPLOG_VISIBLE_ROWS];
    CIFStatic* m_rowTexts[DROPLOG_VISIBLE_ROWS][7];
    CIFButton* m_prevButton;
    CIFButton* m_nextButton;
    CIFStatic* m_pageText;
    CIFStatic* m_totalText;
    CIFStatic* m_noDataText;

    int m_currentPage;
    int m_pageSize;
    int m_totalCount;
    int m_totalPages;
    int m_selectedRow;
    int m_filter;
    std::n_string m_searchText;
    bool m_isLoading;
    bool m_lastMouseDown;
    DWORD m_requestTick;
    std::vector<DropLogEntry> m_currentRows;
};

class CIFDropLogsGuide : public CIFDecoratedStatic {
    GFX_DECLARE_DYNCREATE(CIFDropLogsGuide)

public:
    bool OnCreate(long ln) override;
    int OnMouseLeftUp(int a1, int x, int y) override;
    void OnCIFReady() override;
};
