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

#include "IFExtQuickSlotSecond.h"
#include "GInterface.h"
#include "Game.h"
#include "IFExtQuickSlotSlot.h"
#include "IFExtQuickSlotOption.h"
#include "IFExtQuickSlotSecondOption.h"
#include <BSLib/Debug.h>
#include <CustomData/CustomSettingManager.h>

#define SR_EXT_OPTION_DATA_COUNT 6
#define SR_EXT_OPTION_SLOT_VISIBLE_OFFSET 0
#define SR_EXT_OPTION_VERTICAL_OFFSET 1
#define SR_EXT_OPTION_2LINES_OFFSET 2
#define SR_EXT_OPTION_TRANSPARENT_OFFSET 3
#define SR_EXT_OPTION_LOCKED_OFFSET 4
#define SR_EXT_OPTION_BAR_LOCKED_OFFSET 5
#define GDR_STATIC1 16

GFX_IMPLEMENT_DYNCREATE(CIFExtQuickSlotSecond, CIFWnd)
GFX_BEGIN_MESSAGE_MAP(CIFExtQuickSlotSecond, CIFWnd)
                    ONG_COMMAND(11, &CIFExtQuickSlotSecond::On_BtnClick)
                    ONG_COMMAND(12, &CIFExtQuickSlotSecond::On_BtnClick)
                    ONG_COMMAND(13, &CIFExtQuickSlotSecond::On_BtnClick)
                    ONG_COMMAND(14, &CIFExtQuickSlotSecond::On_BtnClick)

                    ONG_COMMAND(303, &CIFExtQuickSlotSecond::On_BtnClickDown)
                    ONG_COMMAND(304, &CIFExtQuickSlotSecond::On_BtnClickUp)

GFX_END_MESSAGE_MAP()

CIFExtQuickSlotSecond::CIFExtQuickSlotSecond() {
    OnOffBtn = 0;
    OptBtn = 0;
    HoriBtn = 0;
    VertBtn = 0;
    End = 0;
    Drag = 0;
    SlotsOpened = true;
    ActivePageNo = 1;
    BS_DEBUG_LOW(">" __FUNCTION__);
}

CIFExtQuickSlotSecond::~CIFExtQuickSlotSecond() {

    BS_DEBUG_LOW(">" __FUNCTION__);
}

void CIFExtQuickSlotSecond::EnsureSlots() {
    for (int i = 0; i < 10; i++) {
        CIFSlotWithHelp* pSlot = myslots[i]->m_pMySlot->m_pSlot;

        pSlot->SetSlotData(NULL);
        pSlot->TB_Func_13("", 0, 0);
    }
}


bool CIFExtQuickSlotSecond::OnCreate(long ln) {

    CIFWnd::OnCreate(ln);
    //This Type 1

    RECT FrameRect = { 2, 0, 0, 0 };
    Drag = (CIFDragableArea*)CGWnd::CreateInstance(this, GFX_RUNTIME_CLASS(CIFDragableArea), FrameRect, GDR_EXT_QUICKSLOT_WND, 0);

    OptBtn = (CIFButton*)CGWnd::CreateInstance(this, GFX_RUNTIME_CLASS(CIFButton), FrameRect, GDR_EXT_QUICKSLOT_OPT_BTN, 0);
    HoriBtn = (CIFButton*)CGWnd::CreateInstance(this, GFX_RUNTIME_CLASS(CIFButton), FrameRect, GDR_EXT_QUICKSLOT_HORI_BTN, 0);
    VertBtn = (CIFButton*)CGWnd::CreateInstance(this, GFX_RUNTIME_CLASS(CIFButton), FrameRect, GDR_EXT_QUICKSLOT_VERT_BTN, 0);
    OnOffBtn = (CIFButton*)CGWnd::CreateInstance(this, GFX_RUNTIME_CLASS(CIFButton), FrameRect, GDR_EXT_QUICKSLOT_ONOFF_BTN, 0);

    End = (CIFStatic*)CGWnd::CreateInstance(this, GFX_RUNTIME_CLASS(CIFStatic), FrameRect, GDR_STATIC1, 0);

    buttontile = (CIFStatic*)CGWnd::CreateInstance(this, GFX_RUNTIME_CLASS(CIFStatic), FrameRect, 305, 0);

    button_up = (CIFButton*)CGWnd::CreateInstance(this, GFX_RUNTIME_CLASS(CIFButton), FrameRect, 304, 0);
    button_down = (CIFButton*)CGWnd::CreateInstance(this, GFX_RUNTIME_CLASS(CIFButton), FrameRect, 303, 0);

    hotkey = (CIFStatic*)CGWnd::CreateInstance(this, GFX_RUNTIME_CLASS(CIFStatic), FrameRect, 302, 0);

    for (int i = 0; i < 10; i++) {
        SlotBoard[i] = (CIFStatic*)CGWnd::CreateInstance(this, GFX_RUNTIME_CLASS(CIFStatic), FrameRect, 250 + i, 0);

    }
    for (int i = 0; i < 50; i++) {
        myslots[i] = (CIFMySlot*)CGWnd::CreateInstance(this, GFX_RUNTIME_CLASS(CIFMySlot), FrameRect, 250 + i, 0);
        myslots[i]->m_pMySlot->m_pSlot->SetType(0xC);
        myslots[i]->m_pMySlot->m_pSlot->SetSlot(51 + i);
    }
    UpdateBarStyle(1);
    // m_pMyUnderMenuBar = g_pCGInterface->GetUnderMenuBar();
    //  this->ResetPosition();
    hotkey->SetText(L"1");
    if(m_Settings->SecondarySlot)
    {
        this->ShowGWnd(true);
    }
    else
    {
        this->ShowGWnd(false);
    }

    return true;
}

int LastBarStyle;
void CIFExtQuickSlotSecond::On_BtnClick() {
    g_pCGInterface->m_IRM.GetResObj<CIFExtQuickSlotSecondOption>(1352, 1)->SetCheckBoxState(3);
    int id = GetCurrentEventMsgCtrlId();
    switch (id) {
        case GDR_EXT_QUICKSLOT_OPT_BTN: {
            if (!g_pCGInterface->m_IRM.GetResObj<CIFExtQuickSlotSecondOption>(1352, 1)->IsVisible()) {
                g_pCGInterface->m_IRM.GetResObj<CIFExtQuickSlotSecondOption>(1352, 1)->ShowGWnd(true);
                g_pCGInterface->m_IRM.GetResObj<CIFExtQuickSlotSecondOption>(1352, 1)->ResetPosition();

            }
        } break;
        case GDR_EXT_QUICKSLOT_HORI_BTN: /// yataya çevir
        {
            g_pCGInterface->m_IRM.GetResObj<CIFExtQuickSlotSecondOption>(1352, 1)->QUICKSLOT_OPT_YATAY = true;
            if (g_pCGInterface->m_IRM.GetResObj<CIFExtQuickSlotSecondOption>(1352, 1)->QUICKSLOT_OPT_DOUBLELINE_CHECKBOX) {
                UpdateBarStyle(2);
            }
            else {
                UpdateBarStyle(1);
            }
            g_pCGInterface->m_IRM.GetResObj<CIFExtQuickSlotSecondOption>(1352, 1)->SetCheckBoxState(2);
        } break;
        case GDR_EXT_QUICKSLOT_VERT_BTN: /// dikeye çevir
        {
            g_pCGInterface->m_IRM.GetResObj<CIFExtQuickSlotSecondOption>(1352, 1)->QUICKSLOT_OPT_YATAY = false;
            if (g_pCGInterface->m_IRM.GetResObj<CIFExtQuickSlotSecondOption>(1352, 1)->QUICKSLOT_OPT_DOUBLELINE_CHECKBOX) {
                UpdateBarStyle(4);
            }
            else {
                UpdateBarStyle(3);
            }
            g_pCGInterface->m_IRM.GetResObj<CIFExtQuickSlotSecondOption>(1352, 1)->SetCheckBoxState(2);
        } break;
        case GDR_EXT_QUICKSLOT_ONOFF_BTN: /// göster kapat
        {
            if (SlotsOpened == true) {
                SlotsOpened = false;
                UpdateBarStyle(LastBarStyle);

            }
            else if(SlotsOpened == false)
            {
                SlotsOpened = true;
                UpdateBarStyle(LastBarStyle);

            }
        } break;
    }
}

void CIFExtQuickSlotSecond::UpdateBarStyle(int Type) { /// horiz yatay amk
    switch (Type) {
        case 1: /// yatay tek satır
        {
            if (SlotsOpened)
            {
                OnOffBtn->TB_Func_13("interface\\quick_slot\\qsl_hclose_button.ddj", 0, 0);
                OnOffBtn->SetGWndSize(16, 40);
                OnOffBtn->MoveGWnd(Drag->GetPos().x + 26, Drag->GetPos().y + 1);


                Drag->TB_Func_13("interface\\quick_slot\\qsl_hriz01_windo.ddj", 0, 0);
                Drag->SetGWndSize(44, 40);


                OptBtn->TB_Func_13("interface\\quick_slot\\qsl_opt_button.ddj", 0, 0);
                OptBtn->SetGWndSize(16, 16);
                OptBtn->MoveGWnd(Drag->GetPos().x + 14, Drag->GetPos().y + 1);


                HoriBtn->TB_Func_13("interface\\quick_slot\\qsl_hriz_button.ddj", 0, 0);
                HoriBtn->SetGWndSize(16, 16);
                HoriBtn->MoveGWnd(Drag->GetPos().x + 14, Drag->GetPos().y + 13);


                VertBtn->TB_Func_13("interface\\quick_slot\\qsl_vrtl_button.ddj", 0, 0);
                VertBtn->SetGWndSize(16, 16);
                VertBtn->MoveGWnd(Drag->GetPos().x + 14, Drag->GetPos().y + 25);
                wnd_pos psMagicCube = this->GetPos();



                End->MoveGWnd(psMagicCube.x + 423, psMagicCube.y);

                End->TB_Func_13("interface\\quick_slot\\qsl_hriz01_end.ddj", 1, 0);
                End->SetGWndSize(4, 40);
                End->ShowGWnd(true);
                End->BringToFront();

                buttontile->MoveGWnd(psMagicCube.x + 403, psMagicCube.y);

                buttontile->TB_Func_13("interface\\quick_slot\\qsl_hriz01_slot_tile.ddj", 1, 0);
                buttontile->SetGWndSize(20, 40);
                buttontile->ShowGWnd(true);
                buttontile->BringToFront();
                button_up->MoveGWnd(Drag->GetPos().x + 401, Drag->GetPos().y+1);

                button_up->TB_Func_13("interface\\underbar\\ub_up_arrow.ddj", 1, 0);
                button_up->SetGWndSize(20, 12);
                button_up->ShowGWnd(true);
                button_up->BringToFront();

                button_down->MoveGWnd(Drag->GetPos().x + 401, Drag->GetPos().y + 26);

                button_down->TB_Func_13("interface\\underbar\\ub_down_arrow.ddj", 1, 0);
                button_down->SetGWndSize(20, 12);
                button_down->ShowGWnd(true);
                button_down->BringToFront();

                hotkey->MoveGWnd(Drag->GetPos().x + 402, Drag->GetPos().y + 16);
                hotkey->SetGWndSize(17, 7);
                hotkey->ShowGWnd(true);
                hotkey->BringToFront();
                int nSlotPosX = 0;
                for(int i = 0; i < 50; i++)
                {
                    CIFMySlot *pExtSlot = myslots[i];
                    pExtSlot->SetGWndSize(32, 32);
                    if ((i) % 10 == 0) {
                        nSlotPosX = 0;
                    }
                    pExtSlot->MoveGWnd(nSlotPosX + (psMagicCube.x + 45), psMagicCube.y + 4);
                    nSlotPosX += 36;
                }
                nSlotPosX = 0;
                for (int i = 0; i < 10; i++)
                {

                    CIFStatic* pBoards = SlotBoard[i];
                    pBoards->MoveGWnd(nSlotPosX + (psMagicCube.x + 43), psMagicCube.y);
                    pBoards->SetGWndSize(36, 40);
                    pBoards->ShowGWnd(true);
                    pBoards->TB_Func_13("interface\\quick_slot\\qsl_hriz01_slot_tile.ddj", 1, 1);
                    nSlotPosX += 36;
                }
            }
            else
            {
                OnOffBtn->TB_Func_13("interface\\quick_slot\\qsl_hopen_button.ddj", 0, 0);

                OnOffBtn->SetGWndSize(16, 40);
                OnOffBtn->MoveGWnd(Drag->GetPos().x + 26, Drag->GetPos().y + 1);


                Drag->TB_Func_13("interface\\quick_slot\\qsl_hriz01_windo.ddj", 0, 0);
                Drag->SetGWndSize(44, 40);

                OptBtn->TB_Func_13("interface\\quick_slot\\qsl_opt_button.ddj", 0, 0);
                OptBtn->SetGWndSize(16, 16);
                OptBtn->MoveGWnd(Drag->GetPos().x + 14, Drag->GetPos().y + 1);


                HoriBtn->TB_Func_13("interface\\quick_slot\\qsl_hriz_button.ddj", 0, 0);
                HoriBtn->SetGWndSize(16, 16);
                HoriBtn->MoveGWnd(Drag->GetPos().x + 14, Drag->GetPos().y + 13);


                VertBtn->TB_Func_13("interface\\quick_slot\\qsl_vrtl_button.ddj", 0, 0);
                VertBtn->SetGWndSize(16, 16);
                VertBtn->MoveGWnd(Drag->GetPos().x + 14, Drag->GetPos().y + 25);

                wnd_pos psMagicCube = this->GetPos();



                End->MoveGWnd(psMagicCube.x + 60, psMagicCube.y);

                End->TB_Func_13("interface\\quick_slot\\qsl_hriz01_end.ddj", 1, 0);
                End->SetGWndSize(4, 40);
                End->ShowGWnd(true);
                End->BringToFront();

                buttontile->MoveGWnd(psMagicCube.x + 40, psMagicCube.y);

                buttontile->TB_Func_13("interface\\quick_slot\\qsl_hriz01_slot_tile.ddj", 1, 0);
                buttontile->SetGWndSize(20, 40);
                buttontile->ShowGWnd(true);
                buttontile->BringToFront();
                button_up->MoveGWnd(Drag->GetPos().x + 38, Drag->GetPos().y+1);

                button_up->TB_Func_13("interface\\underbar\\ub_up_arrow.ddj", 1, 0);
                button_up->SetGWndSize(20, 12);
                button_up->ShowGWnd(true);
                button_up->BringToFront();

                button_down->MoveGWnd(Drag->GetPos().x + 38, Drag->GetPos().y + 26);

                button_down->TB_Func_13("interface\\underbar\\ub_down_arrow.ddj", 1, 0);
                button_down->SetGWndSize(20, 12);
                button_down->ShowGWnd(true);
                button_down->BringToFront();

                hotkey->MoveGWnd(Drag->GetPos().x + 39, Drag->GetPos().y + 16);
                hotkey->SetGWndSize(17, 7);
                hotkey->ShowGWnd(true);
                hotkey->BringToFront();

                int nSlotPosX = 0;
                for(int i = 0; i < 50; i++)
                {
                    CIFMySlot *pExtSlot = myslots[i];
                    pExtSlot->SetGWndSize(32, 32);
                    pExtSlot->ShowGWnd(false);
                    if ((i) % 10 == 0) {
                        nSlotPosX = 0;
                    }
                    pExtSlot->MoveGWnd(nSlotPosX + (psMagicCube.x + 45), psMagicCube.y + 4);
                    nSlotPosX += 36;
                }
                nSlotPosX = 0;
                for (int i = 0; i < 10; i++)
                {
                    CIFStatic* pBoards = SlotBoard[i];
                    pBoards->MoveGWnd(nSlotPosX + (psMagicCube.x + 43), psMagicCube.y);
                    pBoards->SetGWndSize(36, 40);
                    pBoards->ShowGWnd(false);
                    pBoards->TB_Func_13("interface\\quick_slot\\qsl_hriz01_slot_tile.ddj", 1, 1);
                    nSlotPosX += 36;
                }
            }

            LastBarStyle = 1;
            UpdateSlots();
        }
            break;
        case 2: /// dikey iki satır
        {

            if (SlotsOpened) {
                OnOffBtn->TB_Func_13("interface\\quick_slot\\qsl_hclose_button.ddj", 0, 0);
                Drag->TB_Func_13("interface\\quick_slot\\qsl_hriz02_windo.ddj", 0, 0);
                Drag->SetGWndSize(32, 76);

                OnOffBtn->SetGWndSize(16, 40);
                OnOffBtn->MoveGWnd(Drag->GetPos().x + 14, Drag->GetPos().y + 1);


                OptBtn->TB_Func_13("interface\\quick_slot\\qsl_opt_button.ddj", 0, 0);
                OptBtn->SetGWndSize(16, 16);
                OptBtn->MoveGWnd(Drag->GetPos().x + 14, Drag->GetPos().y + 37);


                HoriBtn->TB_Func_13("interface\\quick_slot\\qsl_hriz_button.ddj", 0, 0);
                HoriBtn->SetGWndSize(16, 16);
                HoriBtn->MoveGWnd(Drag->GetPos().x + 14, Drag->GetPos().y + 49);


                VertBtn->TB_Func_13("interface\\quick_slot\\qsl_vrtl_button.ddj", 0, 0);
                VertBtn->SetGWndSize(16, 16);
                VertBtn->MoveGWnd(Drag->GetPos().x + 14, Drag->GetPos().y + 61);

                wnd_pos psMagicCube = this->GetPos();



                End->MoveGWnd(psMagicCube.x + 231, psMagicCube.y);
                End->SetGWndSize(4, 76);
                End->TB_Func_13("interface\\quick_slot\\qsl_hriz02_end.ddj", 0, 0);
                End->ShowGWnd(true);
                End->BringToFront();

                buttontile->MoveGWnd(Drag->GetPos().x + 209, psMagicCube.y);

                buttontile->TB_Func_13("interface\\ifcommon\\bg_tile\\com_bg_tile_e.ddj", 1, 0);
                buttontile->SetGWndSize(20, 76);
                buttontile->ShowGWnd(true);
                buttontile->BringToFront();

                button_up->MoveGWnd(Drag->GetPos().x + 209, Drag->GetPos().y + 1);

                button_up->TB_Func_13("interface\\underbar\\ub_up_arrow.ddj", 1, 0);
                button_up->SetGWndSize(20, 12);
                button_up->ShowGWnd(true);
                button_up->BringToFront();

                button_down->MoveGWnd(Drag->GetPos().x + 209, Drag->GetPos().y + 62);

                button_down->TB_Func_13("interface\\underbar\\ub_down_arrow.ddj", 1, 0);
                button_down->SetGWndSize(20, 12);
                button_down->ShowGWnd(true);
                button_down->BringToFront();

                hotkey->MoveGWnd(Drag->GetPos().x + 210, Drag->GetPos().y + 35);
                hotkey->SetGWndSize(17, 7);
                hotkey->ShowGWnd(true);
                hotkey->BringToFront();


                int nSlotPosX = 0; // Başlangıçta x pozisyonu sıfırlandı
                for(int i = 0; i < 50; i++) {
                    CIFMySlot *pExtSlot = myslots[i];
                    pExtSlot->ShowGWnd(true);
                    pExtSlot->SetGWndSize(32, 32);

                    // Her sayfa başında x pozisyonunu sıfırla
                    if ((i) % 10 == 0) {
                        nSlotPosX = 0;
                    }

                    // Slotların konumlandırılması
                    if (i >= 0 && i <= 4) {
                        pExtSlot->MoveGWnd(nSlotPosX + (psMagicCube.x + 33), psMagicCube.y + 4);
                    } else if (i >= 5 && i <= 9) {
                        pExtSlot->MoveGWnd(nSlotPosX + (psMagicCube.x - 149), psMagicCube.y + 40);
                    } else if (i >= 10 && i <= 14) {
                        pExtSlot->MoveGWnd(nSlotPosX + (psMagicCube.x + 33), psMagicCube.y + 4);
                    } else if (i >= 15 && i <= 19) {
                        pExtSlot->MoveGWnd(nSlotPosX + (psMagicCube.x - 149), psMagicCube.y + 40);
                    } else if (i >= 20 && i <= 24) {
                        pExtSlot->MoveGWnd(nSlotPosX + (psMagicCube.x + 33), psMagicCube.y + 4);
                    } else if (i >= 25 && i <= 29) {
                        pExtSlot->MoveGWnd(nSlotPosX + (psMagicCube.x - 149), psMagicCube.y + 40);
                    } else if (i >= 30 && i <= 34) {
                        pExtSlot->MoveGWnd(nSlotPosX + (psMagicCube.x + 33), psMagicCube.y + 4);
                    } else if (i >= 35 && i <= 39) {
                        pExtSlot->MoveGWnd(nSlotPosX + (psMagicCube.x - 149), psMagicCube.y + 40);
                    } else if (i >= 40 && i <= 44) {
                        pExtSlot->MoveGWnd(nSlotPosX + (psMagicCube.x + 33), psMagicCube.y + 4);
                    } else if (i >= 45 && i <= 49) {
                        pExtSlot->MoveGWnd(nSlotPosX + (psMagicCube.x - 149), psMagicCube.y + 40);
                    }

                    nSlotPosX += 36; // Her döngü adımında artış yapıldı
                }


                nSlotPosX = 0;


                for (int i = 0; i < 10; i++)
                {
                    CIFStatic* pBoards = SlotBoard[i];
                    if (i >= 5)
                    {
                        pBoards->ShowGWnd(false);
                    }
                    else
                    {
                        pBoards->MoveGWnd(nSlotPosX + (psMagicCube.x + 31), psMagicCube.y);
                        pBoards->SetGWndSize(36, 76);
                        pBoards->ShowGWnd(true);
                        pBoards->TB_Func_13("interface\\quick_slot\\qsl_hriz02_slot_tile.ddj", 1, 1);
                    }
                    nSlotPosX += 36;
                }
            }
            else {
                OnOffBtn->TB_Func_13("interface\\quick_slot\\qsl_hopen_button.ddj", 0, 0);

                Drag->TB_Func_13("interface\\quick_slot\\qsl_hriz02_windo.ddj", 0, 0);
                Drag->SetGWndSize(32, 76);

                OnOffBtn->SetGWndSize(16, 40);
                OnOffBtn->MoveGWnd(Drag->GetPos().x + 14, Drag->GetPos().y + 1);


                OptBtn->TB_Func_13("interface\\quick_slot\\qsl_opt_button.ddj", 0, 0);
                OptBtn->SetGWndSize(16, 16);
                OptBtn->MoveGWnd(Drag->GetPos().x + 14, Drag->GetPos().y + 37);


                HoriBtn->TB_Func_13("interface\\quick_slot\\qsl_hriz_button.ddj", 0, 0);
                HoriBtn->SetGWndSize(16, 16);
                HoriBtn->MoveGWnd(Drag->GetPos().x + 14, Drag->GetPos().y + 49);


                VertBtn->TB_Func_13("interface\\quick_slot\\qsl_vrtl_button.ddj", 0, 0);
                VertBtn->SetGWndSize(16, 16);
                VertBtn->MoveGWnd(Drag->GetPos().x + 14, Drag->GetPos().y + 61);

                wnd_pos psMagicCube = this->GetPos();



                End->MoveGWnd(psMagicCube.x + 49, psMagicCube.y);
                End->SetGWndSize(4, 76);
                End->TB_Func_13("interface\\quick_slot\\qsl_hriz02_end.ddj", 0, 0);
                End->ShowGWnd(true);
                End->BringToFront();

                buttontile->MoveGWnd(Drag->GetPos().x + 27, psMagicCube.y);

                buttontile->TB_Func_13("interface\\ifcommon\\bg_tile\\com_bg_tile_e.ddj", 1, 0);
                buttontile->SetGWndSize(20, 76);
                buttontile->ShowGWnd(true);
                buttontile->BringToFront();

                button_up->MoveGWnd(Drag->GetPos().x + 27, Drag->GetPos().y + 1);

                button_up->TB_Func_13("interface\\underbar\\ub_up_arrow.ddj", 1, 0);
                button_up->SetGWndSize(20, 12);
                button_up->ShowGWnd(true);
                button_up->BringToFront();

                button_down->MoveGWnd(Drag->GetPos().x + 27, Drag->GetPos().y + 62);

                button_down->TB_Func_13("interface\\underbar\\ub_down_arrow.ddj", 1, 0);
                button_down->SetGWndSize(20, 12);
                button_down->ShowGWnd(true);
                button_down->BringToFront();

                hotkey->MoveGWnd(Drag->GetPos().x + 29, Drag->GetPos().y + 34);
                hotkey->SetGWndSize(17, 7);
                hotkey->ShowGWnd(true);
                hotkey->BringToFront();

                int nSlotPosX = 0;
                for(int i = 0; i < 50; i++)
                {
                    CIFMySlot *pExtSlot = myslots[i];
                    pExtSlot->ShowGWnd(false);
                    pExtSlot->SetGWndSize(32, 32);
                    if ((i) % 10 == 0) {
                        nSlotPosX = 0;
                    }
                    if (i >= 0 && i <= 4) {
                        // 1. sayfada slotlar 1'den 5'e kadar
                        pExtSlot->MoveGWnd(nSlotPosX + (psMagicCube.x - 149), psMagicCube.y + 40);
                    } else if (i >= 5 && i <= 9) {
                        // 1. sayfada slotlar 6'dan 10'a kadar
                        pExtSlot->MoveGWnd(nSlotPosX + (psMagicCube.x + 33), psMagicCube.y + 4);
                    } else if (i >= 10 && i <= 14) {
                        // 2. sayfada slotlar 11'den 15'e kadar
                        pExtSlot->MoveGWnd(nSlotPosX + (psMagicCube.x - 149), psMagicCube.y + 40);
                    } else if (i >= 15 && i <= 19) {
                        // 2. sayfada slotlar 16'dan 20'ye kadar
                        pExtSlot->MoveGWnd(nSlotPosX + (psMagicCube.x + 33), psMagicCube.y + 4);
                    } else if (i >= 20 && i <= 24) {
                        // 3. sayfada slotlar 21'den 25'e kadar
                        pExtSlot->MoveGWnd(nSlotPosX + (psMagicCube.x - 149), psMagicCube.y + 40);
                    } else if (i >= 25 && i <= 29) {
                        // 3. sayfada slotlar 26'dan 30'a kadar
                        pExtSlot->MoveGWnd(nSlotPosX + (psMagicCube.x + 33), psMagicCube.y + 4);
                    } else if (i >= 30 && i <= 34) {
                        // 4. sayfada slotlar 31'den 35'e kadar
                        pExtSlot->MoveGWnd(nSlotPosX + (psMagicCube.x - 149), psMagicCube.y + 40);
                    } else if (i >= 35 && i <= 39) {
                        // 4. sayfada slotlar 36'dan 40'a kadar
                        pExtSlot->MoveGWnd(nSlotPosX + (psMagicCube.x + 33), psMagicCube.y + 4);
                    } else if (i >= 40 && i <= 44) {
                        // 5. sayfada slotlar 41'den 45'e kadar
                        pExtSlot->MoveGWnd(nSlotPosX + (psMagicCube.x - 149), psMagicCube.y + 40);
                    } else if (i >= 45 && i <= 49) {
                        // 5. sayfada slotlar 46'dan 50'ye kadar
                        pExtSlot->MoveGWnd(nSlotPosX + (psMagicCube.x + 33), psMagicCube.y + 4);
                    }
                    nSlotPosX += 36;
                }
                nSlotPosX = 0;
                for (int i = 0; i < 10; i++)
                {
                    CIFStatic* pBoards = SlotBoard[i];
                    if (i >= 5)
                    {
                        pBoards->ShowGWnd(false);
                    }
                    else
                    {
                        pBoards->MoveGWnd(nSlotPosX + (psMagicCube.x + 31), psMagicCube.y);
                        pBoards->SetGWndSize(36, 76);
                        pBoards->ShowGWnd(false);
                        pBoards->TB_Func_13("interface\\quick_slot\\qsl_hriz02_slot_tile.ddj", 1, 1);
                    }
                    nSlotPosX += 36;
                }
            }

            LastBarStyle = 2;
            UpdateSlots();
        }
            break;
        case 3: /// dikey tek satır
        {
            if (SlotsOpened) {
                OnOffBtn->TB_Func_13("interface\\quick_slot\\qsl_vclose_button.ddj", 0, 0);
                Drag->TB_Func_13("interface\\quick_slot\\qsl_vrtl01_windo.ddj", 0, 0);
                Drag->SetGWndSize(44, 44);

                OnOffBtn->SetGWndSize(40, 14);
                OnOffBtn->MoveGWnd(Drag->GetPos().x + 2, Drag->GetPos().y + 26);


                OptBtn->TB_Func_13("interface\\quick_slot\\qsl_opt_button.ddj", 0, 0);
                OptBtn->SetGWndSize(16, 16);
                OptBtn->MoveGWnd(Drag->GetPos().x + 2, Drag->GetPos().y + 14);


                HoriBtn->TB_Func_13("interface\\quick_slot\\qsl_hriz_button.ddj", 0, 0);
                HoriBtn->SetGWndSize(16, 16);
                HoriBtn->MoveGWnd(Drag->GetPos().x + 14, Drag->GetPos().y + 14);


                VertBtn->TB_Func_13("interface\\quick_slot\\qsl_vrtl_button.ddj", 0, 0);
                VertBtn->SetGWndSize(16, 16);
                VertBtn->MoveGWnd(Drag->GetPos().x + 26, Drag->GetPos().y + 14);

                wnd_pos psMagicCube = this->GetPos();


                End->MoveGWnd(psMagicCube.x + 2, psMagicCube.y + 421);
                End->TB_Func_13("interface\\quick_slot\\qsl_vrtl01_end.ddj", 0, 0);
                End->SetGWndSize(44, 4);
                End->ShowGWnd(true);
                End->BringToFront();


                buttontile->MoveGWnd(psMagicCube.x + 2, psMagicCube.y + 401);

                buttontile->TB_Func_13("interface\\ifcommon\\bg_tile\\com_bg_tile_e.ddj", 1, 0);
                buttontile->SetGWndSize(42, 20);
                buttontile->ShowGWnd(true);
                buttontile->BringToFront();

                button_up->MoveGWnd(Drag->GetPos().x + 28, Drag->GetPos().y + 401);

                button_up->TB_Func_13("interface\\quick_slot\\qsl_right_arrow.ddj", 1, 0);
                button_up->SetGWndSize(12, 20);
                button_up->ShowGWnd(true);
                button_up->BringToFront();

                button_down->MoveGWnd(Drag->GetPos().x + 1, Drag->GetPos().y + 401);

                button_down->TB_Func_13("interface\\quick_slot\\qsl_left_arrow.ddj", 1, 0);
                button_down->SetGWndSize(12, 20);
                button_down->ShowGWnd(true);
                button_down->BringToFront();


                hotkey->MoveGWnd(Drag->GetPos().x + 12, Drag->GetPos().y + 407);
                hotkey->SetGWndSize(17, 7);
                hotkey->ShowGWnd(true);
                hotkey->BringToFront();

                int nSlotPosY = 0;
                for(int i = 0; i < 50; i++)
                {
                    CIFMySlot *pExtSlot = myslots[i];
                    pExtSlot->SetGWndSize(32, 32);
                    if ((i) % 10 == 0) {
                        nSlotPosY = 0;
                    }
                    pExtSlot->MoveGWnd(psMagicCube.x + 7, nSlotPosY + (psMagicCube.y + 43));
                    nSlotPosY += 36;
                }

                nSlotPosY = 0;

                for (int i = 0; i < 10; i++)
                {
                    CIFStatic* pBoards = SlotBoard[i];
                    pBoards->MoveGWnd(psMagicCube.x + 2, nSlotPosY + (psMagicCube.y + 41));
                    pBoards->SetGWndSize(44, 36);
                    pBoards->ShowGWnd(true);
                    pBoards->TB_Func_13("interface\\quick_slot\\qsl_vrtl01_slot_tile.ddj", 1, 1);
                    nSlotPosY += 36;
                }
            }
            else {
                OnOffBtn->TB_Func_13("interface\\quick_slot\\qsl_vopen_button.ddj", 0, 0);

                Drag->TB_Func_13("interface\\quick_slot\\qsl_vrtl01_windo.ddj", 0, 0);
                Drag->SetGWndSize(44, 44);

                OnOffBtn->SetGWndSize(40, 14);
                OnOffBtn->MoveGWnd(Drag->GetPos().x + 2, Drag->GetPos().y + 26);


                OptBtn->TB_Func_13("interface\\quick_slot\\qsl_opt_button.ddj", 0, 0);
                OptBtn->SetGWndSize(16, 16);
                OptBtn->MoveGWnd(Drag->GetPos().x + 2, Drag->GetPos().y + 14);


                HoriBtn->TB_Func_13("interface\\quick_slot\\qsl_hriz_button.ddj", 0, 0);
                HoriBtn->SetGWndSize(16, 16);
                HoriBtn->MoveGWnd(Drag->GetPos().x + 14, Drag->GetPos().y + 14);


                VertBtn->TB_Func_13("interface\\quick_slot\\qsl_vrtl_button.ddj", 0, 0);
                VertBtn->SetGWndSize(16, 16);
                VertBtn->MoveGWnd(Drag->GetPos().x + 26, Drag->GetPos().y + 14);

                wnd_pos psMagicCube = this->GetPos();


                End->MoveGWnd(psMagicCube.x + 2, psMagicCube.y + 58);
                End->TB_Func_13("interface\\quick_slot\\qsl_vrtl01_end.ddj", 0, 0);
                End->SetGWndSize(44, 4);
                End->ShowGWnd(true);
                End->BringToFront();


                buttontile->MoveGWnd(psMagicCube.x + 2, psMagicCube.y + 39);

                buttontile->TB_Func_13("interface\\ifcommon\\bg_tile\\com_bg_tile_e.ddj", 1, 0);
                buttontile->SetGWndSize(42, 20);
                buttontile->ShowGWnd(true);
                buttontile->BringToFront();

                button_up->MoveGWnd(Drag->GetPos().x + 28, Drag->GetPos().y + 39);

                button_up->TB_Func_13("interface\\quick_slot\\qsl_right_arrow.ddj", 1, 0);
                button_up->SetGWndSize(12, 20);
                button_up->ShowGWnd(true);
                button_up->BringToFront();

                button_down->MoveGWnd(Drag->GetPos().x + 1, Drag->GetPos().y + 39);

                button_down->TB_Func_13("interface\\quick_slot\\qsl_left_arrow.ddj", 1, 0);
                button_down->SetGWndSize(12, 20);
                button_down->ShowGWnd(true);
                button_down->BringToFront();


                hotkey->MoveGWnd(Drag->GetPos().x + 12, Drag->GetPos().y + 45);
                hotkey->SetGWndSize(17, 7);
                hotkey->ShowGWnd(true);
                hotkey->BringToFront();


                int nSlotPosY = 0;
                for(int i = 0; i < 50; i++)
                {
                    CIFMySlot *pExtSlot = myslots[i];
                    pExtSlot->SetGWndSize(32, 32);
                    pExtSlot->ShowGWnd(false);
                    if ((i) % 10 == 0) {
                        nSlotPosY = 0;
                    }
                    pExtSlot->MoveGWnd(psMagicCube.x + 7, nSlotPosY + (psMagicCube.y + 43));
                    nSlotPosY += 36;
                }
                nSlotPosY = 0;
                for (int i = 0; i < 10; i++)
                {
                    CIFStatic* pBoards = SlotBoard[i];
                    pBoards->MoveGWnd(psMagicCube.x + 2, nSlotPosY + (psMagicCube.y + 41));
                    pBoards->SetGWndSize(44, 36);
                    pBoards->ShowGWnd(false);
                    pBoards->TB_Func_13("interface\\quick_slot\\qsl_vrtl01_slot_tile.ddj", 1, 1);
                    nSlotPosY += 36;
                }
            }

            LastBarStyle = 3;
            UpdateSlots();
        }
            break;
        case 4: /// yatay çift satır
        {
            if (SlotsOpened) {
                OnOffBtn->TB_Func_13("interface\\quick_slot\\qsl_vclose_button.ddj", 0, 0);
                Drag->TB_Func_13("interface\\quick_slot\\qsl_vrtl02_windo.ddj", 0, 0);
                Drag->SetGWndSize(80, 32);

                OnOffBtn->SetGWndSize(40, 14);
                OnOffBtn->MoveGWnd(Drag->GetPos().x + 2, Drag->GetPos().y + 14);


                OptBtn->TB_Func_13("interface\\quick_slot\\qsl_opt_button.ddj", 0, 0);
                OptBtn->SetGWndSize(16, 16);
                OptBtn->MoveGWnd(Drag->GetPos().x + 38, Drag->GetPos().y + 14);


                HoriBtn->TB_Func_13("interface\\quick_slot\\qsl_hriz_button.ddj", 0, 0);
                HoriBtn->SetGWndSize(16, 16);
                HoriBtn->MoveGWnd(Drag->GetPos().x + 50, Drag->GetPos().y + 14);


                VertBtn->TB_Func_13("interface\\quick_slot\\qsl_vrtl_button.ddj", 0, 0);
                VertBtn->SetGWndSize(16, 16);
                VertBtn->MoveGWnd(Drag->GetPos().x + 62, Drag->GetPos().y + 14);

                wnd_pos psMagicCube = this->GetPos();


                End->MoveGWnd(Drag->GetPos().x, Drag->GetPos().y+229);
                End->SetGWndSize(80, 4);
                End->TB_Func_13("interface\\quick_slot\\qsl_vrtl02_end.ddj", 0, 0);
                End->ShowGWnd(true);
                End->BringToFront();




                buttontile->MoveGWnd(Drag->GetPos().x, Drag->GetPos().y+209);

                buttontile->TB_Func_13("interface\\ifcommon\\bg_tile\\com_bg_tile_e.ddj", 1, 0);
                buttontile->SetGWndSize(77, 20);
                buttontile->ShowGWnd(true);
                buttontile->BringToFront();

                button_up->MoveGWnd(Drag->GetPos().x + 63, Drag->GetPos().y + 209);

                button_up->TB_Func_13("interface\\quick_slot\\qsl_right_arrow.ddj", 1, 0);
                button_up->SetGWndSize(12, 20);
                button_up->ShowGWnd(true);
                button_up->BringToFront();

                button_down->MoveGWnd(Drag->GetPos().x + 2, Drag->GetPos().y + 209);

                button_down->TB_Func_13("interface\\quick_slot\\qsl_left_arrow.ddj", 1, 0);
                button_down->SetGWndSize(12, 20);
                button_down->ShowGWnd(true);
                button_down->BringToFront();



                hotkey->MoveGWnd(Drag->GetPos().x + 26, Drag->GetPos().y + 214);
                hotkey->SetGWndSize(25, 10);
                hotkey->ShowGWnd(true);
                hotkey->BringToFront();

                int nSlotPosY = 0;

                for(int i = 0; i < 50; i++)
                {
                    CIFMySlot *pExtSlot = myslots[i];
                    pExtSlot->SetGWndSize(32, 32);
                    pExtSlot->ShowGWnd(true);

                    if ((i) % 10 == 0) {
                        nSlotPosY = 0;
                    }

                    if (i >= 0 && i <= 4) {
                        // 1. sayfada slotlar 1'den 5'e kadar
                        pExtSlot->MoveGWnd(psMagicCube.x + 7, nSlotPosY + (psMagicCube.y + 31));
                    } else if (i >= 5 && i <= 9) {
                        // 1. sayfada slotlar 6'dan 10'a kadar
                        pExtSlot->MoveGWnd(psMagicCube.x + 41, nSlotPosY + (psMagicCube.y - 149));
                    } else if (i >= 10 && i <= 14) {
                        // 2. sayfada slotlar 11'den 15'e kadar
                        pExtSlot->MoveGWnd(psMagicCube.x + 7, nSlotPosY + (psMagicCube.y + 31));
                    } else if (i >= 15 && i <= 19) {
                        // 2. sayfada slotlar 16'dan 20'ye kadar
                        pExtSlot->MoveGWnd(psMagicCube.x + 41, nSlotPosY + (psMagicCube.y - 149));
                    } else if (i >= 20 && i <= 24) {
                        // 3. sayfada slotlar 21'den 25'e kadar
                        pExtSlot->MoveGWnd(psMagicCube.x + 7, nSlotPosY + (psMagicCube.y + 31));
                    } else if (i >= 25 && i <= 29) {
                        // 3. sayfada slotlar 26'dan 30'a kadar
                        pExtSlot->MoveGWnd(psMagicCube.x + 41, nSlotPosY + (psMagicCube.y - 149));
                    } else if (i >= 30 && i <= 34) {
                        // 4. sayfada slotlar 31'den 35'e kadar
                        pExtSlot->MoveGWnd(psMagicCube.x + 7, nSlotPosY + (psMagicCube.y + 31));
                    } else if (i >= 35 && i <= 39) {
                        // 4. sayfada slotlar 36'dan 40'a kadar
                        pExtSlot->MoveGWnd(psMagicCube.x + 41, nSlotPosY + (psMagicCube.y - 149));
                    } else if (i >= 40 && i <= 44) {
                        // 5. sayfada slotlar 41'den 45'e kadar
                        pExtSlot->MoveGWnd(psMagicCube.x + 7, nSlotPosY + (psMagicCube.y + 31));
                    } else if (i >= 45 && i <= 49) {
                        // 5. sayfada slotlar 46'dan 50'ye kadar
                        pExtSlot->MoveGWnd(psMagicCube.x + 41, nSlotPosY + (psMagicCube.y - 149));
                    }
                    nSlotPosY += 36;
                }

                nSlotPosY = 0;

                for (int i = 0; i < 10; i++)
                {
                    CIFStatic* pBoards = SlotBoard[i];

                    if (i >= 5)
                    {
                        pBoards->ShowGWnd(false);
                    }
                    else
                    {
                        pBoards->MoveGWnd(psMagicCube.x + 2, psMagicCube.y + (nSlotPosY + 29));
                        pBoards->SetGWndSize(80, 36);
                        pBoards->ShowGWnd(true);
                        pBoards->TB_Func_13("interface\\quick_slot\\qsl_vrtl02_slot_tile.ddj", 1, 1);
                    }
                    nSlotPosY += 36;
                }
            }
            else {
                OnOffBtn->TB_Func_13("interface\\quick_slot\\qsl_vopen_button.ddj", 0, 0);

                Drag->TB_Func_13("interface\\quick_slot\\qsl_vrtl02_windo.ddj", 0, 0);
                Drag->SetGWndSize(80, 32);

                OnOffBtn->SetGWndSize(40, 14);
                OnOffBtn->MoveGWnd(Drag->GetPos().x + 2, Drag->GetPos().y + 14);


                OptBtn->TB_Func_13("interface\\quick_slot\\qsl_opt_button.ddj", 0, 0);
                OptBtn->SetGWndSize(16, 16);
                OptBtn->MoveGWnd(Drag->GetPos().x + 38, Drag->GetPos().y + 14);


                HoriBtn->TB_Func_13("interface\\quick_slot\\qsl_hriz_button.ddj", 0, 0);
                HoriBtn->SetGWndSize(16, 16);
                HoriBtn->MoveGWnd(Drag->GetPos().x + 50, Drag->GetPos().y + 14);


                VertBtn->TB_Func_13("interface\\quick_slot\\qsl_vrtl_button.ddj", 0, 0);
                VertBtn->SetGWndSize(16, 16);
                VertBtn->MoveGWnd(Drag->GetPos().x + 62, Drag->GetPos().y + 14);

                wnd_pos psMagicCube = this->GetPos();

                End->MoveGWnd(Drag->GetPos().x, Drag->GetPos().y+48);
                End->SetGWndSize(80, 4);
                End->TB_Func_13("interface\\quick_slot\\qsl_vrtl02_end.ddj", 0, 0);
                End->ShowGWnd(true);
                End->BringToFront();




                buttontile->MoveGWnd(Drag->GetPos().x, Drag->GetPos().y+28);

                buttontile->TB_Func_13("interface\\ifcommon\\bg_tile\\com_bg_tile_e.ddj", 1, 0);
                buttontile->SetGWndSize(77, 20);
                buttontile->ShowGWnd(true);
                buttontile->BringToFront();

                button_up->MoveGWnd(Drag->GetPos().x + 63, Drag->GetPos().y + 28);

                button_up->TB_Func_13("interface\\quick_slot\\qsl_right_arrow.ddj", 1, 0);
                button_up->SetGWndSize(12, 20);
                button_up->ShowGWnd(true);
                button_up->BringToFront();

                button_down->MoveGWnd(Drag->GetPos().x + 2, Drag->GetPos().y + 28);

                button_down->TB_Func_13("interface\\quick_slot\\qsl_left_arrow.ddj", 1, 0);
                button_down->SetGWndSize(12, 20);
                button_down->ShowGWnd(true);
                button_down->BringToFront();



                hotkey->MoveGWnd(Drag->GetPos().x + 26, Drag->GetPos().y + 33);
                hotkey->SetGWndSize(25, 10);
                hotkey->ShowGWnd(true);
                hotkey->BringToFront();

                int nSlotPosY = 0;

                for(int i = 0; i < 50; i++)
                {
                    CIFMySlot *pExtSlot = myslots[i];
                    pExtSlot->ShowGWnd(false);
                    pExtSlot->SetGWndSize(32, 32);

                    if ((i) % 10 == 0) {
                        nSlotPosY = 0;
                    }

                    if (i >= 0 && i <= 4) {
                        // 1. sayfada slotlar 1'den 5'e kadar
                        pExtSlot->MoveGWnd(psMagicCube.x + 7, nSlotPosY + (psMagicCube.y + 31));
                    } else if (i >= 5 && i <= 9) {
                        // 1. sayfada slotlar 6'dan 10'a kadar
                        pExtSlot->MoveGWnd(psMagicCube.x + 41, nSlotPosY + (psMagicCube.y - 149));
                    } else if (i >= 10 && i <= 14) {
                        // 2. sayfada slotlar 11'den 15'e kadar
                        pExtSlot->MoveGWnd(psMagicCube.x + 7, nSlotPosY + (psMagicCube.y + 31));
                    } else if (i >= 15 && i <= 19) {
                        // 2. sayfada slotlar 16'dan 20'ye kadar
                        pExtSlot->MoveGWnd(psMagicCube.x + 41, nSlotPosY + (psMagicCube.y - 149));
                    } else if (i >= 20 && i <= 24) {
                        // 3. sayfada slotlar 21'den 25'e kadar
                        pExtSlot->MoveGWnd(psMagicCube.x + 7, nSlotPosY + (psMagicCube.y + 31));
                    } else if (i >= 25 && i <= 29) {
                        // 3. sayfada slotlar 26'dan 30'a kadar
                        pExtSlot->MoveGWnd(psMagicCube.x + 41, nSlotPosY + (psMagicCube.y - 149));
                    } else if (i >= 30 && i <= 34) {
                        // 4. sayfada slotlar 31'den 35'e kadar
                        pExtSlot->MoveGWnd(psMagicCube.x + 7, nSlotPosY + (psMagicCube.y + 31));
                    } else if (i >= 35 && i <= 39) {
                        // 4. sayfada slotlar 36'dan 40'a kadar
                        pExtSlot->MoveGWnd(psMagicCube.x + 41, nSlotPosY + (psMagicCube.y - 149));
                    } else if (i >= 40 && i <= 44) {
                        // 5. sayfada slotlar 41'den 45'e kadar
                        pExtSlot->MoveGWnd(psMagicCube.x + 7, nSlotPosY + (psMagicCube.y + 31));
                    } else if (i >= 45 && i <= 49) {
                        // 5. sayfada slotlar 46'dan 50'ye kadar
                        pExtSlot->MoveGWnd(psMagicCube.x + 41, nSlotPosY + (psMagicCube.y - 149));
                    }
                    nSlotPosY += 36;
                }
                nSlotPosY = 0;
                for (int i = 0; i < 10; i++)
                {
                    CIFStatic* pBoards = SlotBoard[i];
                    if (i >= 5)
                    {
                        pBoards->ShowGWnd(false);
                    }
                    else
                    {
                        pBoards->MoveGWnd(psMagicCube.x + 2, psMagicCube.y + (nSlotPosY + 29));
                        pBoards->SetGWndSize(80, 36);
                        pBoards->ShowGWnd(false);
                        pBoards->TB_Func_13("interface\\quick_slot\\qsl_vrtl02_slot_tile.ddj", 1, 1);
                    }
                    nSlotPosY += 36;
                }
            }

            LastBarStyle = 4;
            UpdateSlots();
        }
            break;
    }

}

void CIFExtQuickSlotSecond::OnUpdate() {
    if (g_pCGInterface->m_IRM.GetResObj<CIFExtQuickSlotSecondOption>(1352, 1)->mainx != g_CGame->GetRes().res->height
        && g_pCGInterface->m_IRM.GetResObj<CIFExtQuickSlotSecondOption>(1352, 1)->mainy != g_CGame->GetRes().res->width) {
        int PosX = 0, PosY = 0;
        PosY = (g_CGame->GetRes().res->height / 2) - (this->GetSize().height / 10);
        PosX = (g_CGame->GetRes().res->width / 2) - (this->GetSize().width / 2);
        g_pCGInterface->m_IRM.GetResObj<CIFExtQuickSlotSecond>(1351, 1)->MoveGWnd(PosX, PosX);
        g_pCGInterface->m_IRM.GetResObj<CIFExtQuickSlotSecondOption>(1352, 1)->SetCheckBoxState(3);
    }
}


void CIFExtQuickSlotSecond::ResetPosition() {

    int PosX = 0, PosY = 0;
    PosY = (g_CGame->GetRes().res->height / 2) - (this->GetSize().height/10);
    PosX = (g_CGame->GetRes().res->width / 2) - (this->GetSize().width / 2);
    this->MoveGWnd(PosX, PosY);
    BringToFront();

}
void CIFExtQuickSlotSecond::On_BtnClickDown() {
    // Aktif sayfa numarasını azalt
    if (ActivePageNo > 1) {
        ActivePageNo--;
        wchar_t buffer[250];
        swprintf_s(buffer, L"%d", ActivePageNo);
        hotkey->SetText(buffer);
        // Slotları güncelle (10 slot gösterilecek, diğerleri gizlenecek)

        UpdateSlots();
    }
}

void CIFExtQuickSlotSecond::On_BtnClickUp() {
    // Aktif sayfa numarasını arttır
    if (ActivePageNo < 5) { // 5 sayfa olduğunu varsayalım, gerekirse sayıyı değiştirin
        ActivePageNo++;
        wchar_t buffer[250];
        swprintf_s(buffer, L"%d", ActivePageNo);
        hotkey->SetText(buffer);
        // Slotları güncelle (10 slot gösterilecek, diğerleri gizlenecek)
        UpdateSlots();
    }
}
void CIFExtQuickSlotSecond::UpdateSlots() {
    int startIndex = (ActivePageNo - 1) * 10;
    int endIndex = startIndex + 9;

    // Tüm slotları gizle
    for (int i = 0; i < 50; i++) {
        myslots[i]->ShowGWnd(false);
    }
    if(SlotsOpened)
    {
        // Belirli aralıktaki slotları göster
        for (int i = startIndex; i <= endIndex; i++) {
            myslots[i]->ShowGWnd(true);
            myslots[i]->BringToFront();
            myslots[i]->SetGWndSize(32, 32);
        }
    }
}