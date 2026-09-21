#include "IFDropLogWnd.h"

#include "ClientNet/MsgStreamBuffer.h"
#include "EntityManagerClient.h"
#include "GInterface.h"
#include "GlobalDataManager.h"
#include "GlobalHelpersThatHaveNoHomeYet.h"
#include "IItem.h"
#include "ICPlayer.h"
#include <Windows.h>
#include <algorithm>

#define ID_LOG_BG 10
#define ID_LOG_LATTICE 11
#define ID_LOG_FRAME 12
#define ID_LOG_BTN_PREV 161
#define ID_LOG_BTN_NEXT 162
#define ID_LOG_PAGE_TEXT 163
#define ID_LOG_SLOT_BG_START 300
#define ID_LOG_SLOT_START 200

static CIItem* FindDropLogItemByUniqueId(unsigned int uniqueId) {
    if (!g_pGfxEttManager || uniqueId == 0) {
        return 0;
    }

    for (std::map<int, CIObject*>::const_iterator it = g_pGfxEttManager->entities.begin();
         it != g_pGfxEttManager->entities.end();
         ++it) {
        if (!it->second || !it->second->IsSame(GFX_RUNTIME_CLASS(CIItem))) {
            continue;
        }

        CIItem* item = (CIItem*)it->second;
        if ((unsigned int)item->GetUniqueId() == uniqueId) {
            return item;
        }
    }

    return 0;
}

GFX_IMPLEMENT_DYNCREATE(CIFDropLogWnd, CIFMainFrame)

GFX_BEGIN_MESSAGE_MAP(CIFDropLogWnd, CIFMainFrame)
                    ONG_COMMAND(ID_LOG_BTN_PREV, &CIFDropLogWnd::OnPageClick)
                    ONG_COMMAND(ID_LOG_BTN_NEXT, &CIFDropLogWnd::OnPageClick)
                    ONG_CHAR()
GFX_END_MESSAGE_MAP()

CIFDropLogWnd::CIFDropLogWnd() {
    m_currentPage = 0;
    m_bLastLButton = false;
    m_bLastRButton = false;
    m_pBtnPrev = 0;
    m_pBtnNext = 0;
    m_pPageText = 0;
    m_pBackground = 0;
    m_pSlotFrame = 0;
    m_lastPickupTick = 0;
    m_displayMode = MODE_GROUND_DROPS;
    m_possibleDropsMonsterId = 0;
    m_possibleDropsLoading = false;

    for (int i = 0; i < 90; ++i) {
        m_pSlotBackgrounds[i] = 0;
        m_pSlots[i] = 0;
        m_slotGIDs[i] = 0;
    }
}

CIFDropLogWnd::~CIFDropLogWnd() {
}

bool CIFDropLogWnd::OnCreate(long ln) {
    if (!CIFMainFrame::OnCreate(ln))
        return false;

    TB_Func_13("interface\\frame\\mframe_wnd_", 0, 1);
    SetText(KmtGetText(L"UIIT_KMT_PICK_INVENTORY_O"));

    RECT bgRect = {10, 31, 350, 374};
    m_pBackground = (CIFNormalTile*)CreateInstance(this, GFX_RUNTIME_CLASS(CIFNormalTile), bgRect, ID_LOG_BG, 0);
    if (m_pBackground) {
        m_pBackground->TB_Func_13("interface\\ifcommon\\bg_tile\\com_bg_tile_e.ddj", 0, 1);
        m_pBackground->ShowGWnd(true);
    }

    for (int i = 0; i < 90; ++i) {
        int row = i / 10;
        int col = i % 10;
        RECT slotRect = {7 + (col * 36), 43 + (row * 36), 32, 32};

        m_pSlotBackgrounds[i] = (CIFStatic*)CreateInstance(this, GFX_RUNTIME_CLASS(CIFStatic), slotRect, ID_LOG_SLOT_BG_START + i, 0);
        if (m_pSlotBackgrounds[i]) {
            m_pSlotBackgrounds[i]->TB_Func_13("interface\\store\\str_slot_02.ddj", 0, 0);
            m_pSlotBackgrounds[i]->ShowGWnd(true);
        }

        m_pSlots[i] = (CIFSlotWithHelp*)CreateInstance(this, GFX_RUNTIME_CLASS(CIFSlotWithHelp), slotRect, ID_LOG_SLOT_START + i, 0);
        if (m_pSlots[i]) {
            m_pSlots[i]->SetType(19);
            m_pSlots[i]->SetSlot(ID_LOG_SLOT_START + i);
            m_pSlots[i]->SetSlotData(NULL);
            m_pSlots[i]->SetClickable(true);
            m_pSlots[i]->ShowGWnd(true);
        }
    }

    RECT prevRect = {160, 382, 16, 16};
    m_pBtnPrev = (CIFButton*)CreateInstance(this, GFX_RUNTIME_CLASS(CIFButton), prevRect, ID_LOG_BTN_PREV, 0);
    if (m_pBtnPrev) {
        m_pBtnPrev->TB_Func_13("interface\\ifcommon\\com_left_arrow.ddj", 0, 1);
        m_pBtnPrev->ShowGWnd(true);
    }

    RECT nextRect = {194, 382, 16, 16};
    m_pBtnNext = (CIFButton*)CreateInstance(this, GFX_RUNTIME_CLASS(CIFButton), nextRect, ID_LOG_BTN_NEXT, 0);
    if (m_pBtnNext) {
        m_pBtnNext->TB_Func_13("interface\\ifcommon\\com_right_arrow.ddj", 0, 1);
        m_pBtnNext->ShowGWnd(true);
    }

    RECT textRect = {175, 381, 20, 18};
    m_pPageText = (CIFStatic*)CreateInstance(this, GFX_RUNTIME_CLASS(CIFStatic), textRect, ID_LOG_PAGE_TEXT, 0);
    if (m_pPageText) {
        m_pPageText->SetText(L"1");
        m_pPageText->ShowGWnd(true);
    }

    UpdateWindowPos();
    ShowGWnd(false);
    return true;
}

void CIFDropLogWnd::OnUpdate() {
    CIFMainFrame::OnUpdate();

    if (IsVisible()) {
        bool curL = (GetKeyState(VK_LBUTTON) & 0x8000) != 0;
        bool curR = (GetKeyState(VK_RBUTTON) & 0x8000) != 0;

        if (m_displayMode == MODE_GROUND_DROPS && ((m_bLastLButton && !curL) || (m_bLastRButton && !curR))) {
            CGWndBase* target = g_CurrentIfUnderCursor;
            int id = target ? target->UniqueID() : -1;

            if ((id < ID_LOG_SLOT_START || id >= ID_LOG_SLOT_START + 90) && g_pOnMouseDownClickCtrl) {
                id = g_pOnMouseDownClickCtrl->UniqueID();
            }

            if (id >= ID_LOG_SLOT_START && id < ID_LOG_SLOT_START + 90) {
                HandleSlotClick(id);
            }
        }

        m_bLastLButton = curL;
        m_bLastRButton = curR;

        static DWORD lastUpdate = 0;
        if (m_displayMode == MODE_GROUND_DROPS && GetTickCount() - lastUpdate > 1000) {
            RefreshDropList();
            lastUpdate = GetTickCount();
        }
    }
}

void CIFDropLogWnd::ShowGWnd(bool bVisible) {
    CIFMainFrame::ShowGWnd(bVisible);
    if (bVisible) {
        if (m_displayMode == MODE_POSSIBLE_DROPS) {
            if (m_possibleDropItemIds.empty() && !m_possibleDropsLoading) {
                RequestPossibleDrops();
            } else {
                FillPossibleDrops();
            }
        } else {
            RefreshDropList();
        }
        BringToFront();
    }
}

void CIFDropLogWnd::OnPageClick() {
    int id = GetCurrentEventMsgCtrlId();
    int maxPage = GetMaxPage(GetCurrentItemCount());

    if (id == ID_LOG_BTN_PREV && m_currentPage > 0) {
        --m_currentPage;
    } else if (id == ID_LOG_BTN_NEXT && m_currentPage < maxPage) {
        ++m_currentPage;
    }

    if (m_displayMode == MODE_POSSIBLE_DROPS) {
        FillPossibleDrops();
    } else {
        RefreshDropList();
    }
}

void CIFDropLogWnd::HandleSlotClick(int slotID) {
    if (m_displayMode == MODE_POSSIBLE_DROPS) {
        return;
    }

    if (!g_pCGInterface || !g_pMyPlayerObj) {
        return;
    }

    if (GetTickCount() - m_lastPickupTick < 500) {
        return;
    }

    int slotIdx = slotID - ID_LOG_SLOT_START;
    if (slotIdx < 0 || slotIdx >= 90) {
        return;
    }

    unsigned int gid = m_slotGIDs[slotIdx];
    if (gid == 0 || !FindDropLogItemByUniqueId(gid)) {
        return;
    }

    CMsgStreamBuffer buf(0x7074);
    buf << UINT8(1) << UINT8(2) << UINT8(1) << UINT32(gid);
    SendMsg(buf);

    m_lastPickupTick = GetTickCount();
    m_slotGIDs[slotIdx] = 0;
    ClearDisplaySlot(slotIdx);
}

void CIFDropLogWnd::RefreshDropList() {
    if (!g_pCGInterface || !g_pMyPlayerObj || !g_pGfxEttManager) {
        return;
    }

    std::vector<unsigned int> gids;
    for (std::map<int, CIObject*>::const_iterator it = g_pGfxEttManager->entities.begin();
         it != g_pGfxEttManager->entities.end();
         ++it) {
        if (it->second && it->second->IsSame(GFX_RUNTIME_CLASS(CIItem))) {
            CIItem* item = (CIItem*)it->second;
            gids.push_back((unsigned int)item->GetUniqueId());
        }
    }

    int groundCount = 0;
    int displayCount = 0;
    int startIdx = m_currentPage * 90;

    for (size_t i = 0; i < gids.size(); ++i) {
        unsigned int gid = gids[i];
        CIItem* pItemObj = FindDropLogItemByUniqueId(gid);
        if (!pItemObj) {
            continue;
        }

        const SCommonData* pCommon = pItemObj->GetCommonData();
        if (!pCommon || pCommon->RefObjectId <= 0 || pCommon->CodeName.find(L"GOLD") != std::n_wstring::npos) {
            continue;
        }

        if (groundCount >= startIdx && displayCount < 90) {
            FillDisplaySlot(displayCount, gid, pCommon->RefObjectId, true);
            ++displayCount;
        }

        ++groundCount;
    }

    int maxPage = GetMaxPage(groundCount);
    if (m_currentPage > maxPage) {
        m_currentPage = maxPage;
        RefreshDropList();
        return;
    }

    for (int j = displayCount; j < 90; ++j) {
        ClearDisplaySlot(j);
        m_slotGIDs[j] = 0;
    }

    if (m_pPageText) {
        wchar_t pageText[8];
        swprintf(pageText, L"%d", m_currentPage + 1);
        m_pPageText->SetText(pageText);
    }

    wchar_t title[128];
    swprintf(title, KmtGetText(L"UIIT_KMT_PICK_INVENTORY_VALUE_ITEMS"), groundCount);
    SetText(title);
}

void CIFDropLogWnd::UpdateWindowPos() {
    MoveGWnd(300, 200);
}

undefined1 CIFDropLogWnd::OnCloseWnd() {
    ShowGWnd(false);
    return true;
}

int CIFDropLogWnd::OnChar(UINT nChar, UINT a2, UINT a3) {
    if (nChar == VK_ESCAPE) {
        OnCloseWnd();
        return 1;
    }

    return 0;
}

void CIFDropLogWnd::ClearDisplaySlot(int slotIdx) {
    if (slotIdx < 0 || slotIdx >= 90 || !m_pSlots[slotIdx]) {
        return;
    }

    m_pSlots[slotIdx]->ClearSlot();
    m_pSlots[slotIdx]->SetSlotData(NULL);
    m_pSlots[slotIdx]->SetClickable(m_displayMode == MODE_GROUND_DROPS);
}

void CIFDropLogWnd::FillDisplaySlot(int slotIdx, unsigned int gid, int refObjId, bool clickable) {
    if (slotIdx < 0 || slotIdx >= 90 || !m_pSlots[slotIdx]) {
        return;
    }

    if (gid != 0 && m_slotGIDs[slotIdx] == gid && m_pSlots[slotIdx]->ItemInfo) {
        return;
    }

    const CItemData* pItemData = g_CGlobalDataManager->GetItem(refObjId);
    if (!pItemData) {
        ClearDisplaySlot(slotIdx);
        m_slotGIDs[slotIdx] = 0;
        return;
    }

    const SItemData& data = pItemData->GetData();
    CMsgStreamBuffer buf(0xB034);
    buf << INT32(0) << INT32(refObjId);

    u_short typeID2 = data.m_typeId.getTypeID2();
    u_short typeID3 = data.m_typeId.getTypeID3();
    u_short typeID4 = data.m_typeId.getTypeID4();

    switch (typeID2) {
        case 1:
            buf << UINT8(0) << UINT64(0) << UINT32(1) << UINT8(0) << UINT8(1) << UINT8(0) << UINT8(2) << UINT8(0);
            break;
        case 2:
            switch (typeID3) {
                case 1:
                    buf << UINT8(0x01);
                    break;
                case 2:
                    buf << UINT32(0x00);
                    break;
                default:
                    if (typeID4 == 3) {
                        buf << UINT32(0x01);
                    }
                    break;
            }
            break;
        case 3:
            buf << UINT16(0x01);
            if (typeID3 == 11) {
                if (typeID4 == 1 || typeID4 == 2) {
                    buf << UINT8(0x00);
                }
            } else if (typeID3 == 14 && typeID4 == 2) {
                buf << UINT8(0x00);
            }
            break;
    }

    CSOItem* itemInfo = new CSOItem();
    itemInfo->ReadFromPacket(&buf, 1);
    itemInfo->SetEnabled(true);

    m_pSlots[slotIdx]->SetSlotData(itemInfo);
    if (m_pSlots[slotIdx]->ItemInfo) {
        m_pSlots[slotIdx]->ItemInfo->m_quantity = 1;
        m_pSlots[slotIdx]->ItemInfo->m_OptLevel = 0;
    }
    m_pSlots[slotIdx]->SetType(19);
    m_pSlots[slotIdx]->SetSlot(ID_LOG_SLOT_START + slotIdx);
    m_pSlots[slotIdx]->SetClickable(clickable);
    m_pSlots[slotIdx]->TB_Func_13(data.AssocFileIcon.c_str(), 0, 0);
    m_pSlots[slotIdx]->ShowGWnd(true);
    m_pSlots[slotIdx]->BringToFront();
    m_slotGIDs[slotIdx] = gid;
}

void CIFDropLogWnd::ClearAllDisplaySlots() {
    for (int i = 0; i < 90; ++i) {
        ClearDisplaySlot(i);
        m_slotGIDs[i] = 0;
    }
}

void CIFDropLogWnd::OpenGroundDropList() {
    m_displayMode = MODE_GROUND_DROPS;
    m_possibleDropsLoading = false;
    m_possibleDropsMonsterId = 0;
    m_possibleDropsMonsterName.clear();
    m_possibleDropItemIds.clear();
    m_currentPage = 0;

    if (!IsVisible()) {
        ShowGWnd(true);
    } else {
        RefreshDropList();
        BringToFront();
    }
}

void CIFDropLogWnd::OpenPossibleDrops(int monsterRefObjId, const wchar_t* monsterName) {
    m_displayMode = MODE_POSSIBLE_DROPS;
    m_possibleDropsMonsterId = monsterRefObjId;
    m_possibleDropsMonsterName = monsterName ? monsterName : KmtGetText(L"UIIT_KMT_MONSTER");
    m_possibleDropItemIds.clear();
    m_currentPage = 0;
    m_possibleDropsLoading = false;

    ClearAllDisplaySlots();
    SetText(KmtGetText(L"UIIT_KMT_POSSIBLE_DROPS_LOADING"));
    ShowGWnd(true);
}

void CIFDropLogWnd::RequestPossibleDrops() {
    if (m_possibleDropsMonsterId <= 0) {
        SetText(KmtGetText(L"UIIT_KMT_POSSIBLE_DROPS"));
        return;
    }

    m_possibleDropsLoading = true;
    CMsgStreamBuffer request(0x169A);
    request << byte(34);
    request << INT32(m_possibleDropsMonsterId);
    SendMsg(request);
}

void CIFDropLogWnd::HandlePossibleDropsResponse(bool success, int monsterRefObjId, const std::vector<int>& itemRefObjIds) {
    if (m_displayMode != MODE_POSSIBLE_DROPS || monsterRefObjId != m_possibleDropsMonsterId) {
        return;
    }

    m_possibleDropsLoading = false;
    m_possibleDropItemIds = itemRefObjIds;
    m_currentPage = 0;

    if (!success) {
        ClearAllDisplaySlots();
        SetText(KmtGetText(L"UIIT_KMT_POSSIBLE_DROPS_FAILED"));
        return;
    }

    FillPossibleDrops();
}

void CIFDropLogWnd::FillPossibleDrops() {
    if (m_displayMode != MODE_POSSIBLE_DROPS) {
        return;
    }

    int startIdx = m_currentPage * 90;
    int displayCount = 0;
    for (int i = startIdx; i < (int)m_possibleDropItemIds.size() && displayCount < 90; ++i) {
        FillDisplaySlot(displayCount, 0, m_possibleDropItemIds[i], false);
        ++displayCount;
    }

    for (int j = displayCount; j < 90; ++j) {
        ClearDisplaySlot(j);
        m_slotGIDs[j] = 0;
    }

    if (m_pPageText) {
        wchar_t pageText[8];
        swprintf(pageText, L"%d", m_currentPage + 1);
        m_pPageText->SetText(pageText);
    }

    wchar_t title[160];
    swprintf(title, KmtGetText(L"UIIT_KMT_POSSIBLE_DROPS_TEXT_VALUE_ITEMS"), m_possibleDropsMonsterName.c_str(), (int)m_possibleDropItemIds.size());
    SetText(title);
}

int CIFDropLogWnd::GetCurrentItemCount() const {
    if (m_displayMode == MODE_POSSIBLE_DROPS) {
        return (int)m_possibleDropItemIds.size();
    }

    if (!g_pGfxEttManager) {
        return 0;
    }

    int groundCount = 0;
    for (std::map<int, CIObject*>::const_iterator it = g_pGfxEttManager->entities.begin();
         it != g_pGfxEttManager->entities.end();
         ++it) {
        if (!it->second || !it->second->IsSame(GFX_RUNTIME_CLASS(CIItem))) {
            continue;
        }

        CIItem* item = (CIItem*)it->second;
        const SCommonData* common = item->GetCommonData();
        if (common && common->RefObjectId > 0 && common->CodeName.find(L"GOLD") == std::n_wstring::npos) {
            ++groundCount;
        }
    }

    return groundCount;
}

int CIFDropLogWnd::GetMaxPage(int itemCount) const {
    if (itemCount <= 0) {
        return 0;
    }

    return (itemCount - 1) / 90;
}
