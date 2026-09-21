///******************************************************************************
/// \File IFExtQuickSlot.cpp
///
/// \Desc
///
/// \Author kyuubi09 on 2/16/2022.
///
/// \Copyright Copyright © 2023 SRO_DevKit.
///
///******************************************************************************

#include "IFExtQuickSlot.h"

#include "GInterface.h"
#include "Game.h"
#include "IFExtQuickSlotSlot.h"

#define SR_EXT_OPTION_DATA_COUNT 6
#define SR_EXT_OPTION_SLOT_VISIBLE_OFFSET 0
#define SR_EXT_OPTION_VERTICAL_OFFSET 1
#define SR_EXT_OPTION_2LINES_OFFSET 2
#define SR_EXT_OPTION_TRANSPARENT_OFFSET 3
#define SR_EXT_OPTION_LOCKED_OFFSET 4
#define SR_EXT_OPTION_BAR_LOCKED_OFFSET 5

GFX_BEGIN_MESSAGE_MAP(CIFExtQuickSlot, CIFWnd)
                    ONG_COMMAND(GDR_EXT_QUICKSLOT_OPT_BTN, &CIFExtQuickSlot::OnToggleSlotsOption)
                    ONG_COMMAND(GDR_EXT_QUICKSLOT_ONOFF_BTN, &CIFExtQuickSlot::OnDropDownButton)
                    ONG_COMMAND(GDR_EXT_QUICKSLOT_HORI_BTN, &CIFExtQuickSlot::OnClickSlotsHorizontal)
                    ONG_COMMAND(GDR_EXT_QUICKSLOT_VERT_BTN, &CIFExtQuickSlot::OnClickSlotsVertical)
GFX_END_MESSAGE_MAP()

CIFExtQuickSlot::CIFExtQuickSlot() :
        m_bVertical(false),
        m_bTwoLines(false),
        m_bTransparent(false),
        m_bSlotsLocked(false),
        m_bToolBarLocked(false),
        m_pMyUnderMenuBar(NULL) {
    m_bSlotsVisible = true;
    m_btSectionTypeID = DUMMY;
}

GFX_IMPLEMENT_DYNCREATE_FN(CIFExtQuickSlot, CIFWnd)

void CIFExtQuickSlot::SetBarVerticalState(bool bState) {
    m_bVertical = bState;
}

void CIFExtQuickSlot::SetBarTransparentState(bool bState) {
    m_bTransparent = bState;
}

void CIFExtQuickSlot::SetSlotLockState(bool bState) {
    m_bSlotsLocked = bState;
}

void CIFExtQuickSlot::SetBarLockState(bool bState) {
    m_bToolBarLocked = bState;
}

void CIFExtQuickSlot::SetBarTwoLinesState(bool bState) {
    m_bTwoLines = bState;
}

GFX_IMPLEMENT_DYNAMIC_EXISTING(CIFExtQuickSlot, 0x00ee9a28)

void CIFExtQuickSlot::SaveExtSlotSetting() {
    char buffer[0x200];

    sprintf_s(buffer, "%s\\Setting\\SRExtQSOption.dat", theApp.GetWorkingDir());

    FILE *file = fopen(buffer, "wb+");

    int local_208 = 1; // hmm setting is saved? // or Config type?
    if (file != NULL) {
        bool bData[SR_EXT_OPTION_DATA_COUNT];
        fwrite(&local_208, 4, 1, file);
        bData[SR_EXT_OPTION_SLOT_VISIBLE_OFFSET] = m_bSlotsVisible;
        bData[SR_EXT_OPTION_VERTICAL_OFFSET] = m_bVertical;
        bData[SR_EXT_OPTION_2LINES_OFFSET] = m_bTwoLines;
        bData[SR_EXT_OPTION_TRANSPARENT_OFFSET] = m_bTransparent;
        bData[SR_EXT_OPTION_LOCKED_OFFSET] = m_bSlotsLocked;
        bData[SR_EXT_OPTION_BAR_LOCKED_OFFSET] = m_bToolBarLocked;
        fwrite(&bData[0], sizeof(bool), SR_EXT_OPTION_DATA_COUNT, file);
        fclose(file);
    }
}

CIFExtQuickSlot::~CIFExtQuickSlot() {
    SaveExtSlotSetting();
}

void CIFExtQuickSlot::ToggleExtQuickSlotOption(bool bVisible) {
    reinterpret_cast<void (__thiscall *)(CIFExtQuickSlot *, bool)>(0x0065eb10)(this, bVisible);
}

void CIFExtQuickSlot::SetSlotsVisible(bool bVisible) {
    m_bSlotsVisible = bVisible;

    CIFButton *pOnOffBtn = m_IRM.GetResObj<CIFButton>(GDR_EXT_QUICKSLOT_ONOFF_BTN, 1);
    if (m_btSectionTypeID == TYPE_HORIZONTAL_1LINE || m_btSectionTypeID == TYPE_HORIZONTAL_2LINE) {
        if (bVisible)
            pOnOffBtn->TB_Func_13(
                    "interface\\quick_slot\\qsl_hclose_button.ddj", NULL, NULL);
        else
            pOnOffBtn->TB_Func_13(
                    "interface\\quick_slot\\qsl_hopen_button.ddj", NULL, NULL);
    } else {
        if (bVisible)
            pOnOffBtn->TB_Func_13(
                    "interface\\quick_slot\\qsl_vclose_button.ddj", NULL, NULL);
        else
            pOnOffBtn->TB_Func_13(
                    "interface\\quick_slot\\qsl_vopen_button.ddj", NULL, NULL);
    }

    if (m_btSectionTypeID == TYPE_HORIZONTAL_2LINE || m_btSectionTypeID == TYPE_VERTICAL_2LINE) {
        for (int i = 0; i < 10; i++) {
            m_IRM.GetResObj<CIFExtQuickSlotSlot>(GDR_EXT_QUICKSLOT_SLOT_1 + i, 1)->ShowGWnd(m_bSlotsVisible);
            if (i < 5)
                m_IRM.GetResObj<CIFStatic>(GDR_EXT_QUICKSLOT_SLOT_BOARD_1 + i, 1)->ShowGWnd(m_bSlotsVisible);
        }
    } else if (m_btSectionTypeID == TYPE_HORIZONTAL_1LINE || m_btSectionTypeID == TYPE_VERTICAL_1LINE) {
        for (int i = 0; i < 10; i++) {
            m_IRM.GetResObj<CIFExtQuickSlotSlot>(GDR_EXT_QUICKSLOT_SLOT_1 + i, 1)->ShowGWnd(m_bSlotsVisible);
            m_IRM.GetResObj(GDR_EXT_QUICKSLOT_SLOT_BOARD_1 + i, 1)->ShowGWnd(m_bSlotsVisible);
        }
    }

    m_IRM.GetResObj<CIFStatic>(GDR_EXT_QUICKSLOT_SLOT_END, 1)->ShowGWnd(m_bSlotsVisible);
}

void CIFExtQuickSlot::OnToggleSlotsOption() {
    ToggleExtQuickSlotOption(!m_IRM.GetResObj<CIFExtQuickSlotOption>(GDR_EXT_QUICKSLOT_OPTION, 1));
}

void CIFExtQuickSlot::OnDropDownButton() {
    SetSlotsVisible(!m_bSlotsVisible);
}

void CIFExtQuickSlot::EnsureSlots() {
    for (int i = 0; i < 10; i++) {
        CIFSlotWithHelp *pSlot = m_IRM.GetResObj<CIFExtQuickSlotSlot>(GDR_EXT_QUICKSLOT_SLOT_1 + i,
                                                                      1)->GetSlot();

        pSlot->SetSlotData(NULL);
        pSlot->TB_Func_13("", 0, 0);
    }
}

void CIFExtQuickSlot::UpdateSlotsBarDirection() {
    CIFExtQuickSlot::TYPE btSections = TYPE_HORIZONTAL_1LINE;

    if (!m_bVertical) {
        if (m_bTwoLines)
            btSections = TYPE_HORIZONTAL_2LINE;
    } else {
        if (!m_bTwoLines)
            btSections = TYPE_VERTICAL_1LINE;
        else
            btSections = TYPE_VERTICAL_2LINE;
    }

    if (btSections == m_btSectionTypeID) {
        for (int iVar5 = 0; iVar5 < 10; iVar5++)
            m_IRM.GetResObj<CIFExtQuickSlotSlot>(GDR_EXT_QUICKSLOT_SLOT_1 + iVar5, 1)->UpdatehgWndSlot(
                    m_pMyUnderMenuBar->m_pMySlots[41 + iVar5]->m_pSlot);
    }

    // Lets delete our old one
    switch (m_btSectionTypeID) {
        case TYPE_HORIZONTAL_1LINE:
            m_IRM.DeleteCreatedSection("Type1");
            break;
        case TYPE_HORIZONTAL_2LINE:
            m_IRM.DeleteCreatedSection("Type2");
            break;
        case TYPE_VERTICAL_1LINE:
            m_IRM.DeleteCreatedSection("Type3");
            break;
        case TYPE_VERTICAL_2LINE:
        default:
            m_IRM.DeleteCreatedSection("Type4");
            break;
    }

    // Create the new type style
    switch (btSections) {
        case TYPE_HORIZONTAL_1LINE:
            m_IRM.CreateInterfaceSection("Type1", this);
            break;
        case TYPE_HORIZONTAL_2LINE:
            m_IRM.CreateInterfaceSection("Type2", this);
            break;
        case TYPE_VERTICAL_1LINE:
            m_IRM.CreateInterfaceSection("Type3", this);
            break;
        case TYPE_VERTICAL_2LINE:
        default:
            m_IRM.CreateInterfaceSection("Type4", this);
            break;
    }

    wnd_size szDragWmd = m_IRM.GetResObj(GDR_EXT_QUICKSLOT_WND, 1)->GetSize();
    this->SetGWndSize(szDragWmd.width, szDragWmd.height);

    m_btSectionTypeID = btSections;
    wnd_pos psMagicCube = this->GetPos();

    switch (m_btSectionTypeID) {
        // When it's not vertical, and it's one line too
        case TYPE_HORIZONTAL_1LINE: {
            int nSlotPosX = 0;
            for (int i = 0; i < 10; i++) {
                CIFExtQuickSlotSlot *pExtSlot = m_IRM.GetResObj<CIFExtQuickSlotSlot>(GDR_EXT_QUICKSLOT_SLOT_1 + i,
                                                                                     1);
                pExtSlot->UpdatehgWndSlot(
                        m_pMyUnderMenuBar->m_pMySlots[41 + i]->m_pSlot);
                pExtSlot->MoveGWnd(nSlotPosX + (psMagicCube.x + 43), psMagicCube.y + 4);
                nSlotPosX += 36;
            }
            break;
        }
        // When it's not vertical, and it's Two lines too
        case TYPE_HORIZONTAL_2LINE: {
            int nSlotPosX = 0;
            for (int i = 0; i < 10; i++) {
                CIFExtQuickSlotSlot *pExtSlot = m_IRM.GetResObj<CIFExtQuickSlotSlot>(GDR_EXT_QUICKSLOT_SLOT_1 + i,
                                                                                     1);
                pExtSlot->UpdatehgWndSlot(
                        m_pMyUnderMenuBar->m_pMySlots[41 + i]->m_pSlot);
                if (i >= 5)
                    pExtSlot->MoveGWnd(nSlotPosX + (psMagicCube.x - 149), psMagicCube.y + 40);
                else
                    pExtSlot->MoveGWnd(nSlotPosX + (psMagicCube.x + 31), psMagicCube.y + 4);
                nSlotPosX += 36;
            }
            break;
        }
        // When it's vertical and it's one line too
        case TYPE_VERTICAL_1LINE: {
            int nSlotPosY = 0;
            for (int i = 0; i < 10; i++) {
                CIFExtQuickSlotSlot *pExtSlot = m_IRM.GetResObj<CIFExtQuickSlotSlot>(GDR_EXT_QUICKSLOT_SLOT_1 + i,
                                                                                     1);
                pExtSlot->UpdatehgWndSlot(
                        m_pMyUnderMenuBar->m_pMySlots[41 + i]->m_pSlot);
                pExtSlot->MoveGWnd(psMagicCube.x + 5, nSlotPosY + (psMagicCube.y + 43));
                nSlotPosY += 36;
            }
            break;
        }
        // When it's vertical and it's Two lines too
        case TYPE_VERTICAL_2LINE:
        default: {
            int nSlotPosY = 0;
            for (int i = 0; i < 10; i++) {
                CIFExtQuickSlotSlot *pExtSlot = m_IRM.GetResObj<CIFExtQuickSlotSlot>(GDR_EXT_QUICKSLOT_SLOT_1 + i,
                                                                                     1);
                pExtSlot->UpdatehgWndSlot(
                        m_pMyUnderMenuBar->m_pMySlots[41 + i]->m_pSlot);
                if (i >= 5)
                    pExtSlot->MoveGWnd(psMagicCube.x + 41, nSlotPosY + (psMagicCube.y - 149));
                else
                    pExtSlot->MoveGWnd(psMagicCube.x + 5, nSlotPosY + (psMagicCube.y + 31));
                nSlotPosY += 36;
            }
            break;
        }
    }

    SetSlotsVisible(m_bSlotsVisible);
}

void CIFExtQuickSlot::UpdateBarSlotsStyle() {
    UpdateSlotsBarDirection();

    char N00009BB9 = -1;

    if (m_bTransparent)
        N00009BB9 = 110;

    m_IRM.GetResObj<CIFDragableArea>(GDR_EXT_QUICKSLOT_WND, 1)->TB_Func_9(N00009BB9);
    m_IRM.GetResObj<CIFButton>(GDR_EXT_QUICKSLOT_OPT_BTN, 1)->TB_Func_9(N00009BB9);
    m_IRM.GetResObj<CIFButton>(GDR_EXT_QUICKSLOT_HORI_BTN, 1)->TB_Func_9(N00009BB9);
    m_IRM.GetResObj<CIFButton>(GDR_EXT_QUICKSLOT_VERT_BTN, 1)->TB_Func_9(N00009BB9);
    m_IRM.GetResObj<CIFButton>(GDR_EXT_QUICKSLOT_ONOFF_BTN, 1)->TB_Func_9(N00009BB9);

    if (m_bSlotsVisible) {
        m_IRM.GetResObj(GDR_EXT_QUICKSLOT_SLOT_END, 1)->TB_Func_9(N00009BB9);

        if (m_btSectionTypeID == TYPE_HORIZONTAL_1LINE || m_btSectionTypeID == TYPE_VERTICAL_1LINE) {
            for (int i = 0; i < 10; i++) {
                m_IRM.GetResObj(GDR_EXT_QUICKSLOT_SLOT_1 + i, 1)->TB_Func_9(N00009BB9);
                m_IRM.GetResObj(GDR_EXT_QUICKSLOT_SLOT_BOARD_1 + i, 1)->TB_Func_9(N00009BB9);
            }
        } else if (m_btSectionTypeID == TYPE_HORIZONTAL_2LINE || m_btSectionTypeID == TYPE_VERTICAL_2LINE) {
            for (int i = 0; i < 10; i++) {
                m_IRM.GetResObj(GDR_EXT_QUICKSLOT_SLOT_1 + i, 1)->TB_Func_9(N00009BB9);
                if (i < 5)
                    m_IRM.GetResObj(GDR_EXT_QUICKSLOT_SLOT_BOARD_1 + i, 1)->TB_Func_9(N00009BB9);
            }
        }
    }

    for (int v17 = 0; v17 < 10; v17++) {
        m_IRM.GetResObj<CIFExtQuickSlotSlot>(GDR_EXT_QUICKSLOT_SLOT_1 + v17, 1)->SetDragable(!m_bSlotsLocked);
    }

    m_IRM.GetResObj<CIFDragableArea>(GDR_EXT_QUICKSLOT_WND, 1)->SetClickable(!m_bToolBarLocked);
}

void CIFExtQuickSlot::LoadExtSlotSetting() {
    char buffer[0x200];

    sprintf_s(buffer, "%s\\Setting\\SRExtQSOption.dat", theApp.GetWorkingDir());

    FILE *pFileOpt = fopen(buffer, "rb");

    int local_210; // Config type?
    if (pFileOpt == NULL) {
        SetBarVerticalState(true);
        SetBarTransparentState(true);
        SetSlotLockState(false);
        SetBarLockState(false);
        SetBarTwoLinesState(true);
    } else {
        fread_s(&local_210, -1, sizeof(local_210), 1, pFileOpt);
        if (local_210 == 1) {
            bool bConfig[SR_EXT_OPTION_DATA_COUNT];

            fread_s(&bConfig[0], sizeof(bConfig), sizeof(bConfig), SR_EXT_OPTION_DATA_COUNT, pFileOpt);
            m_bSlotsVisible = bConfig[SR_EXT_OPTION_SLOT_VISIBLE_OFFSET];
            SetBarVerticalState(bConfig[SR_EXT_OPTION_VERTICAL_OFFSET]);
            SetBarTwoLinesState(bConfig[SR_EXT_OPTION_2LINES_OFFSET]);
            SetBarTransparentState(bConfig[SR_EXT_OPTION_TRANSPARENT_OFFSET]);
            SetSlotLockState(bConfig[SR_EXT_OPTION_LOCKED_OFFSET]);
            SetBarLockState(bConfig[SR_EXT_OPTION_BAR_LOCKED_OFFSET]);
        }
        fclose(pFileOpt);
    }
    UpdateBarSlotsStyle();
}

void CIFExtQuickSlot::OnClickSlotsHorizontal() {
    SetBarVerticalState(false);
    UpdateBarSlotsStyle();
}

void CIFExtQuickSlot::OnClickSlotsVertical() {
    SetBarVerticalState(true);
    UpdateBarSlotsStyle();
}

bool CIFExtQuickSlot::OnCreate(long ln) {
    m_IRM.LoadFromFile("resinfo\\ifextquickslot.txt");

    m_pMyUnderMenuBar = g_pCGInterface->GetUnderMenuBar();
    LoadExtSlotSetting();

    return true;
}
