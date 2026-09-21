#pragma once

#include "IFWnd.h"

typedef std::n_list<CIFWnd*> listGUIDES;

class CAlramGuideMgrWnd : public CIFWnd {
GFX_DECLARE_DYNAMIC_EXISTING(CAlramGuideMgrWnd, 0x00ee99a8)

public:
    /// \address 0065bf50
    CAlramGuideMgrWnd();

    /// \address 0065c430
    void PrepareGuides();

    /// \address 0065c4f0
    void RemoveAllGuides();

    /// \address 0065c540
    bool IsAvailableGuide(DWORD dwID);

    // Ithink as a fox this function getting call as inline fn
    /// \address 0065c5a0
    CIFWnd *GetGuide(DWORD dwID);

    /// \address 0065c660
    ~CAlramGuideMgrWnd();

    /// \address 0065c6f0
    CIFWnd *CreateGuideIcon(int nWndID);

    /// \address 0065c870
    void RemoveGuide(DWORD dwID);

private:
    BYTE m_btGuidesCount; //0x036C
    listGUIDES m_listGuides; //0x0370
private:
BEGIN_FIXTURE()
        ENSURE_SIZE(0x037C)
        ENSURE_OFFSET(m_btGuidesCount, 0x036c)
        ENSURE_OFFSET(m_listGuides, 0x0370)
    END_FIXTURE()

    RUN_FIXTURE(CAlramGuideMgrWnd)
};
