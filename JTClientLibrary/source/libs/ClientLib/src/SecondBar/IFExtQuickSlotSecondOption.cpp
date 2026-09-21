#include "IFExtQuickSlotSecondOption.h"
#include "GInterface.h"
#include "GlobalDataManager.h"
#include "TextStringManager.h"
#include "IFExtQuickSlotSecond.h"
#include <Game.h>
#include <BSLib/Debug.h>

GFX_IMPLEMENT_DYNCREATE(CIFExtQuickSlotSecondOption, CIFMainFrame)
GFX_BEGIN_MESSAGE_MAP(CIFExtQuickSlotSecondOption, CIFMainFrame)
                    ONG_COMMAND(35, &CIFExtQuickSlotSecondOption::On_BtnClick)
                    ONG_COMMAND(36, &CIFExtQuickSlotSecondOption::On_BtnClick)
GFX_END_MESSAGE_MAP()
CIFExtQuickSlotSecondOption::CIFExtQuickSlotSecondOption(void) {
    //printf("> " __FUNCTION__ "\n");
    BS_DEBUG_LOW(">" __FUNCTION__);
}

CIFExtQuickSlotSecondOption::~CIFExtQuickSlotSecondOption(void) {
    //printf("> " __FUNCTION__ "\n");
    BS_DEBUG_LOW(">" __FUNCTION__);
}
void CIFExtQuickSlotSecondOption::LoadCheckBoxState() {
    char buffer[0x200];
    char buffer2[0x200];
    sprintf_s(buffer, "%s\\Setting\\SRExtQSOption2.dat", theApp.GetWorkingDir());
    sprintf_s(buffer2, "%s\\Setting\\SRExtQSOptionSize2.dat", theApp.GetWorkingDir());
    FILE* file = fopen(buffer, "rb+");
    FILE* file2 = fopen(buffer2, "rb+");
    if (file != NULL) {
        char config[4];
        fread_s(&config[0], sizeof(config), 1, 4, file);
        m_IRM.GetResObj<CIFCheckBox>(GDR_EXT_QUICKSLOT_OPT_TRANS_CHECKBOX, 1)->SetCheckBoxState(config[0] != 0);
        m_IRM.GetResObj<CIFCheckBox>(GDR_EXT_QUICKSLOT_OPT_LOCK_CHECKBOX, 1)->SetCheckBoxState(config[1] != 0);
        m_IRM.GetResObj<CIFCheckBox>(GDR_EXT_QUICKSLOT_OPT_FIX_CHECKBOX, 1)->SetCheckBoxState(config[2] != 0);
        m_IRM.GetResObj<CIFCheckBox>(GDR_EXT_QUICKSLOT_OPT_DOUBLELINE_CHECKBOX, 1)->SetCheckBoxState(config[3] != 0);
        QUICKSLOT_OPT_TRANS_CHECKBOX = config[0] != 0;
        QUICKSLOT_OPT_LOCK_CHECKBOX = config[1] != 0;
        QUICKSLOT_OPT_FIX_CHECKBOX = config[2] != 0;
        QUICKSLOT_OPT_DOUBLELINE_CHECKBOX = config[3] != 0;
        fclose(file);
    }
    else {
        m_IRM.GetResObj<CIFCheckBox>(GDR_EXT_QUICKSLOT_OPT_TRANS_CHECKBOX, 1)->SetCheckBoxState(false);
        m_IRM.GetResObj<CIFCheckBox>(GDR_EXT_QUICKSLOT_OPT_LOCK_CHECKBOX, 1)->SetCheckBoxState(false);
        m_IRM.GetResObj<CIFCheckBox>(GDR_EXT_QUICKSLOT_OPT_FIX_CHECKBOX, 1)->SetCheckBoxState(false);
        m_IRM.GetResObj<CIFCheckBox>(GDR_EXT_QUICKSLOT_OPT_DOUBLELINE_CHECKBOX, 1)->SetCheckBoxState(false);
        QUICKSLOT_OPT_TRANS_CHECKBOX = false;
        QUICKSLOT_OPT_LOCK_CHECKBOX = false;
        QUICKSLOT_OPT_FIX_CHECKBOX = false;
        QUICKSLOT_OPT_DOUBLELINE_CHECKBOX = false;
    }
    if (file2 != NULL) {
        char config[1];
        fread_s(&config[0], sizeof(config), 1, 1, file2);
        QUICKSLOT_OPT_YATAY = config[0] != 0;
        fclose(file2);
    }
    else {
        QUICKSLOT_OPT_YATAY = false;
    }
    if (QUICKSLOT_OPT_TRANS_CHECKBOX) {
        char N00009BB9 = 110;
        g_pCGInterface->m_IRM.GetResObj<CIFExtQuickSlotSecond>(1351, 1)->Drag->TB_Func_9(N00009BB9);
        g_pCGInterface->m_IRM.GetResObj<CIFExtQuickSlotSecond>(1351, 1)->OptBtn->TB_Func_9(N00009BB9);
        g_pCGInterface->m_IRM.GetResObj<CIFExtQuickSlotSecond>(1351, 1)->HoriBtn->TB_Func_9(N00009BB9);
        g_pCGInterface->m_IRM.GetResObj<CIFExtQuickSlotSecond>(1351, 1)->VertBtn->TB_Func_9(N00009BB9);
        g_pCGInterface->m_IRM.GetResObj<CIFExtQuickSlotSecond>(1351, 1)->OnOffBtn->TB_Func_9(N00009BB9);
        g_pCGInterface->m_IRM.GetResObj<CIFExtQuickSlotSecond>(1351, 1)->End->TB_Func_9(N00009BB9);
        g_pCGInterface->m_IRM.GetResObj<CIFExtQuickSlotSecond>(1351, 1)->buttontile->TB_Func_9(N00009BB9);
        g_pCGInterface->m_IRM.GetResObj<CIFExtQuickSlotSecond>(1351, 1)->button_up->TB_Func_9(N00009BB9);
        g_pCGInterface->m_IRM.GetResObj<CIFExtQuickSlotSecond>(1351, 1)->button_down->TB_Func_9(N00009BB9);
        g_pCGInterface->m_IRM.GetResObj<CIFExtQuickSlotSecond>(1351, 1)->hotkey->TB_Func_9(N00009BB9);

        for (int i = 0; i < 10; i++)
        {
            g_pCGInterface->m_IRM.GetResObj<CIFExtQuickSlotSecond>(1351, 1)->SlotBoard[i]->TB_Func_9(N00009BB9);
        }
    }
    else {
        char N00009BB9 = -1;
        g_pCGInterface->m_IRM.GetResObj<CIFExtQuickSlotSecond>(1351, 1)->Drag->TB_Func_9(N00009BB9);
        g_pCGInterface->m_IRM.GetResObj<CIFExtQuickSlotSecond>(1351, 1)->OptBtn->TB_Func_9(N00009BB9);
        g_pCGInterface->m_IRM.GetResObj<CIFExtQuickSlotSecond>(1351, 1)->HoriBtn->TB_Func_9(N00009BB9);
        g_pCGInterface->m_IRM.GetResObj<CIFExtQuickSlotSecond>(1351, 1)->VertBtn->TB_Func_9(N00009BB9);
        g_pCGInterface->m_IRM.GetResObj<CIFExtQuickSlotSecond>(1351, 1)->OnOffBtn->TB_Func_9(N00009BB9);
        g_pCGInterface->m_IRM.GetResObj<CIFExtQuickSlotSecond>(1351, 1)->End->TB_Func_9(N00009BB9);
        g_pCGInterface->m_IRM.GetResObj<CIFExtQuickSlotSecond>(1351, 1)->buttontile->TB_Func_9(N00009BB9);
        g_pCGInterface->m_IRM.GetResObj<CIFExtQuickSlotSecond>(1351, 1)->button_up->TB_Func_9(255);
        g_pCGInterface->m_IRM.GetResObj<CIFExtQuickSlotSecond>(1351, 1)->button_down->TB_Func_9(255);
        g_pCGInterface->m_IRM.GetResObj<CIFExtQuickSlotSecond>(1351, 1)->hotkey->TB_Func_9(255);

        for (int i = 0; i < 10; i++)
        {
            g_pCGInterface->m_IRM.GetResObj<CIFExtQuickSlotSecond>(1351, 1)->SlotBoard[i]->TB_Func_9(N00009BB9);
        }
    }
    if (QUICKSLOT_OPT_FIX_CHECKBOX) {
        g_pCGInterface->m_IRM.GetResObj<CIFExtQuickSlotSecond>(1351, 1)->Drag->SetClickable(false);
    }
    else {
        g_pCGInterface->m_IRM.GetResObj<CIFExtQuickSlotSecond>(1351, 1)->Drag->SetClickable(true);
    }
    if (!QUICKSLOT_OPT_LOCK_CHECKBOX) {
        for (int i = 0; i < 10; i++)
        {
            g_pCGInterface->m_IRM.GetResObj<CIFExtQuickSlotSecond>(1351, 1)->myslots[i]->m_pMySlot->m_pSlot->SetDragable(true);
        }
    }
    else {
        for (int i = 0; i < 10; i++)
        {
            g_pCGInterface->m_IRM.GetResObj<CIFExtQuickSlotSecond>(1351, 1)->myslots[i]->m_pMySlot->m_pSlot->SetDragable(false);
        }
    }
    if (QUICKSLOT_OPT_DOUBLELINE_CHECKBOX) {
        if (QUICKSLOT_OPT_YATAY) {
            g_pCGInterface->m_IRM.GetResObj<CIFExtQuickSlotSecond>(1351, 1)->UpdateBarStyle(2);
            // g_pCGInterface->GetCIFExtQuickSlotSecond()->UpdateSizeQuick(1, 1);
        }
        else {
            //g_pCGInterface->GetCIFExtQuickSlotSecond()->UpdateSizeQuick(1, 2);
            g_pCGInterface->m_IRM.GetResObj<CIFExtQuickSlotSecond>(1351, 1)->UpdateBarStyle(4);
        }
    }
    else {
        if (QUICKSLOT_OPT_YATAY) {
            //g_pCGInterface->GetCIFExtQuickSlotSecond()->UpdateSizeQuick(2, 1);
            g_pCGInterface->m_IRM.GetResObj<CIFExtQuickSlotSecond>(1351, 1)->UpdateBarStyle(1);
        }
        else {
            // g_pCGInterface->GetCIFExtQuickSlotSecond()->UpdateSizeQuick(2, 2);
            g_pCGInterface->m_IRM.GetResObj<CIFExtQuickSlotSecond>(1351, 1)->UpdateBarStyle(3);
        }
    }

    char buffer3[0x200];
    sprintf_s(buffer3, "%s\\Setting\\wndpos2.txt", theApp.GetWorkingDir());
    FILE *file3 = fopen(buffer3, "r");
    if (file3 != NULL) {

        int array[4] = {0};
        int i;

        for (i = 0; i < 4; i++) {
            fscanf(file3, "%d", &array[i]);
        }
        fclose(file3);

        mainx = array[0];
        mainy = array[1];
        barx = array[2];
        bary = array[3];


    }
    else {
        int PosX = 0, PosY = 0;
        PosY = (g_CGame->GetRes().res->height / 2) - (this->GetSize().height / 10);
        PosX = (g_CGame->GetRes().res->width / 2) - (this->GetSize().width / 2);
        g_pCGInterface->m_IRM.GetResObj<CIFExtQuickSlotSecond>(1351, 1)->MoveGWnd(PosX, PosX);
    }

    if (mainx != g_CGame->GetRes().res->height && mainy != g_CGame->GetRes().res->width) {
        int PosX = 0, PosY = 0;
        PosY = (g_CGame->GetRes().res->height / 2) - (this->GetSize().height / 10);
        PosX = (g_CGame->GetRes().res->width / 2) - (this->GetSize().width / 2);
        g_pCGInterface->m_IRM.GetResObj<CIFExtQuickSlotSecond>(1351, 1)->MoveGWnd(PosX, PosX);
    } else {

        g_pCGInterface->m_IRM.GetResObj<CIFExtQuickSlotSecond>(1351, 1)->MoveGWnd(barx, bary);
    }
}
bool CIFExtQuickSlotSecondOption::OnCreate(long ln) {
    CIFMainFrame::OnCreate(ln);
    m_IRM.LoadFromFile("clientlibrary\\resinfo\\ifextquickslotoptionsecond.txt");
    m_IRM.CreateInterfaceSection("Create", this);
    this->m_pTitleText->SetText(TSM_GETTEXTPTR(L"UIIT_STT_ADD_QUICKSLOT_ADD"));
    TB_Func_13("interface\\messagebox\\msgbox2_window_", 0, 0);
    LoadCheckBoxState();
    QUICKSLOT_OPT_TRANS_SETTINGS = true;
    this->ShowGWnd(false);
    return true;
}
void CIFExtQuickSlotSecondOption::ResetPosition() {

    wnd_pos sz = g_pCGInterface->m_IRM.GetResObj<CIFExtQuickSlotSecond>(1351, 1)->GetPos();
    this->MoveGWnd(sz.x, sz.y-224);
}

void CIFExtQuickSlotSecondOption::On_BtnClick() {
    int id = GetCurrentEventMsgCtrlId();
    switch (id) {
        case 35: {
            SetCheckBoxState(1);
            if (this->QUICKSLOT_OPT_DOUBLELINE_CHECKBOX) {
                if (this->QUICKSLOT_OPT_YATAY) {
                    //  g_pCGInterface->GetCIFExtQuickSlotSecond()->UpdateSizeQuick(1, 1);
                    g_pCGInterface->m_IRM.GetResObj<CIFExtQuickSlotSecond>(1351, 1)->UpdateBarStyle(2);
                }
                else {
                    //g_pCGInterface->GetCIFExtQuickSlotSecond()->UpdateSizeQuick(1, 2);
                    g_pCGInterface->m_IRM.GetResObj<CIFExtQuickSlotSecond>(1351, 1)->UpdateBarStyle(4);
                }
            }
            else {
                if (this->QUICKSLOT_OPT_YATAY) {
                    //g_pCGInterface->GetCIFExtQuickSlotSecond()->UpdateSizeQuick(2, 1);
                    g_pCGInterface->m_IRM.GetResObj<CIFExtQuickSlotSecond>(1351, 1)->UpdateBarStyle(1);
                }
                else {
                    // g_pCGInterface->GetCIFExtQuickSlotSecond()->UpdateSizeQuick(2, 2);
                    g_pCGInterface->m_IRM.GetResObj<CIFExtQuickSlotSecond>(1351, 1)->UpdateBarStyle(3);
                }
            }
            this->OnCloseWnd();
        } break;
        case 36: {
            QUICKSLOT_OPT_TRANS_CHECKBOX = m_IRM.GetResObj<CIFCheckBox>(GDR_EXT_QUICKSLOT_OPT_TRANS_CHECKBOX, 1)->GetCheckedState_MAYBE();
            QUICKSLOT_OPT_LOCK_CHECKBOX = m_IRM.GetResObj<CIFCheckBox>(GDR_EXT_QUICKSLOT_OPT_LOCK_CHECKBOX, 1)->GetCheckedState_MAYBE();
            QUICKSLOT_OPT_FIX_CHECKBOX = m_IRM.GetResObj<CIFCheckBox>(GDR_EXT_QUICKSLOT_OPT_FIX_CHECKBOX, 1)->GetCheckedState_MAYBE();
            QUICKSLOT_OPT_DOUBLELINE_CHECKBOX = m_IRM.GetResObj<CIFCheckBox>(GDR_EXT_QUICKSLOT_OPT_DOUBLELINE_CHECKBOX, 1)->GetCheckedState_MAYBE();

            if (this->QUICKSLOT_OPT_DOUBLELINE_CHECKBOX) {
                if (this->QUICKSLOT_OPT_YATAY) {
                    //   g_pCGInterface->GetCIFExtQuickSlotSecond()->UpdateSizeQuick(1, 1);
                    g_pCGInterface->m_IRM.GetResObj<CIFExtQuickSlotSecond>(1351, 1)->UpdateBarStyle(2);
                }
                else {
                    // g_pCGInterface->GetCIFExtQuickSlotSecond()->UpdateSizeQuick(1, 2);
                    g_pCGInterface->m_IRM.GetResObj<CIFExtQuickSlotSecond>(1351, 1)->UpdateBarStyle(4);
                }
            }
            else {
                if (this->QUICKSLOT_OPT_YATAY) {
                    //g_pCGInterface->GetCIFExtQuickSlotSecond()->UpdateSizeQuick(2, 1);
                    g_pCGInterface->m_IRM.GetResObj<CIFExtQuickSlotSecond>(1351, 1)->UpdateBarStyle(1);
                }
                else {
                    //g_pCGInterface->GetCIFExtQuickSlotSecond()->UpdateSizeQuick(2, 2);
                    g_pCGInterface->m_IRM.GetResObj<CIFExtQuickSlotSecond>(1351, 1)->UpdateBarStyle(3);
                }
            }
        } break;
    }
    if (QUICKSLOT_OPT_TRANS_CHECKBOX) {
        char N00009BB9 = 110;
        g_pCGInterface->m_IRM.GetResObj<CIFExtQuickSlotSecond>(1351, 1)->Drag->TB_Func_9(N00009BB9);
        g_pCGInterface->m_IRM.GetResObj<CIFExtQuickSlotSecond>(1351, 1)->OptBtn->TB_Func_9(N00009BB9);
        g_pCGInterface->m_IRM.GetResObj<CIFExtQuickSlotSecond>(1351, 1)->HoriBtn->TB_Func_9(N00009BB9);
        g_pCGInterface->m_IRM.GetResObj<CIFExtQuickSlotSecond>(1351, 1)->VertBtn->TB_Func_9(N00009BB9);
        g_pCGInterface->m_IRM.GetResObj<CIFExtQuickSlotSecond>(1351, 1)->OnOffBtn->TB_Func_9(N00009BB9);
        g_pCGInterface->m_IRM.GetResObj<CIFExtQuickSlotSecond>(1351, 1)->End->TB_Func_9(N00009BB9);
        g_pCGInterface->m_IRM.GetResObj<CIFExtQuickSlotSecond>(1351, 1)->buttontile->TB_Func_9(N00009BB9);
        g_pCGInterface->m_IRM.GetResObj<CIFExtQuickSlotSecond>(1351, 1)->button_up->TB_Func_9(N00009BB9);
        g_pCGInterface->m_IRM.GetResObj<CIFExtQuickSlotSecond>(1351, 1)->button_down->TB_Func_9(N00009BB9);
        g_pCGInterface->m_IRM.GetResObj<CIFExtQuickSlotSecond>(1351, 1)->hotkey->TB_Func_9(N00009BB9);
        for (int i = 0; i < 10; i++)
        {
            g_pCGInterface->m_IRM.GetResObj<CIFExtQuickSlotSecond>(1351, 1)->SlotBoard[i]->TB_Func_9(N00009BB9);
        }
    }
    else {
        char N00009BB9 = -1;
        g_pCGInterface->m_IRM.GetResObj<CIFExtQuickSlotSecond>(1351, 1)->Drag->TB_Func_9(N00009BB9);
        g_pCGInterface->m_IRM.GetResObj<CIFExtQuickSlotSecond>(1351, 1)->OptBtn->TB_Func_9(N00009BB9);
        g_pCGInterface->m_IRM.GetResObj<CIFExtQuickSlotSecond>(1351, 1)->HoriBtn->TB_Func_9(N00009BB9);
        g_pCGInterface->m_IRM.GetResObj<CIFExtQuickSlotSecond>(1351, 1)->VertBtn->TB_Func_9(N00009BB9);
        g_pCGInterface->m_IRM.GetResObj<CIFExtQuickSlotSecond>(1351, 1)->OnOffBtn->TB_Func_9(N00009BB9);
        g_pCGInterface->m_IRM.GetResObj<CIFExtQuickSlotSecond>(1351, 1)->End->TB_Func_9(N00009BB9);
        g_pCGInterface->m_IRM.GetResObj<CIFExtQuickSlotSecond>(1351, 1)->buttontile->TB_Func_9(N00009BB9);
        g_pCGInterface->m_IRM.GetResObj<CIFExtQuickSlotSecond>(1351, 1)->button_up->TB_Func_9(255);
        g_pCGInterface->m_IRM.GetResObj<CIFExtQuickSlotSecond>(1351, 1)->button_down->TB_Func_9(255);
        g_pCGInterface->m_IRM.GetResObj<CIFExtQuickSlotSecond>(1351, 1)->hotkey->TB_Func_9(255);
        for (int i = 0; i < 10; i++)
        {
            g_pCGInterface->m_IRM.GetResObj<CIFExtQuickSlotSecond>(1351, 1)->SlotBoard[i]->TB_Func_9(N00009BB9);
        }
    }
    if (QUICKSLOT_OPT_FIX_CHECKBOX || this->IsVisible()) {
        g_pCGInterface->m_IRM.GetResObj<CIFExtQuickSlotSecond>(1351, 1)->Drag->SetClickable(false);
    }
    else {
        g_pCGInterface->m_IRM.GetResObj<CIFExtQuickSlotSecond>(1351, 1)->Drag->SetClickable(true);
    }
    if (!QUICKSLOT_OPT_LOCK_CHECKBOX) {
        for (int i = 0; i < 10; i++)
        {
            g_pCGInterface->m_IRM.GetResObj<CIFExtQuickSlotSecond>(1351, 1)->myslots[i]->m_pMySlot->m_pSlot->SetDragable(true);
        }
    }
    else {
        for (int i = 0; i < 10; i++)
        {
            g_pCGInterface->m_IRM.GetResObj<CIFExtQuickSlotSecond>(1351, 1)->myslots[i]->m_pMySlot->m_pSlot->SetDragable(false);
        }
    }
}
void CIFExtQuickSlotSecondOption::SetCheckBoxState(int Type) {
    if (Type == 1) {
        char buffer[0x200];
        sprintf_s(buffer, "%s\\Setting\\SRExtQSOption2.dat", theApp.GetWorkingDir());
        FILE *file = fopen(buffer, "wb+");
        if (file != NULL) {
            bool data[5];
            data[0] = m_IRM.GetResObj<CIFCheckBox>(GDR_EXT_QUICKSLOT_OPT_TRANS_CHECKBOX, 1)->GetCheckedState_MAYBE();
            data[1] = m_IRM.GetResObj<CIFCheckBox>(GDR_EXT_QUICKSLOT_OPT_LOCK_CHECKBOX, 1)->GetCheckedState_MAYBE();
            data[2] = m_IRM.GetResObj<CIFCheckBox>(GDR_EXT_QUICKSLOT_OPT_FIX_CHECKBOX, 1)->GetCheckedState_MAYBE();
            data[3] = m_IRM.GetResObj<CIFCheckBox>(GDR_EXT_QUICKSLOT_OPT_DOUBLELINE_CHECKBOX, 1)->GetCheckedState_MAYBE();
            data[4] = QUICKSLOT_OPT_YATAY;
            fwrite(&data[0], 1, 5, file);
            fclose(file);
        }
        QUICKSLOT_OPT_TRANS_CHECKBOX = m_IRM.GetResObj<CIFCheckBox>(GDR_EXT_QUICKSLOT_OPT_TRANS_CHECKBOX, 1)->GetCheckedState_MAYBE();
        QUICKSLOT_OPT_LOCK_CHECKBOX = m_IRM.GetResObj<CIFCheckBox>(GDR_EXT_QUICKSLOT_OPT_LOCK_CHECKBOX, 1)->GetCheckedState_MAYBE();
        QUICKSLOT_OPT_FIX_CHECKBOX = m_IRM.GetResObj<CIFCheckBox>(GDR_EXT_QUICKSLOT_OPT_FIX_CHECKBOX, 1)->GetCheckedState_MAYBE();
        QUICKSLOT_OPT_DOUBLELINE_CHECKBOX = m_IRM.GetResObj<CIFCheckBox>(GDR_EXT_QUICKSLOT_OPT_DOUBLELINE_CHECKBOX, 1)->GetCheckedState_MAYBE();
    }
    if (Type == 2) {
        char buffer[0x200];
        sprintf_s(buffer, "%s\\Setting\\SRExtQSOptionSize2.dat", theApp.GetWorkingDir());
        FILE *file = fopen(buffer, "wb+");
        if (file != NULL) {
            bool data[1];
            data[0] = QUICKSLOT_OPT_YATAY;
            fwrite(&data[0], 1, 1, file);
            fclose(file);
        }
    }

    if (Type == 3) {
        char buffer[0x200];
        sprintf_s(buffer, "%s\\Setting\\wndpos2.txt", theApp.GetWorkingDir());
        FILE *file = fopen(buffer, "wb+");
        if (file != NULL) {

            fprintf(file, "%d\n", g_CGame->GetRes().res->height);
            fprintf(file, "%d\n", g_CGame->GetRes().res->width);
            fprintf(file, "%d\n", g_pCGInterface->m_IRM.GetResObj<CIFExtQuickSlotSecond>(1351, 1)->GetPos().x);
            fprintf(file, "%d\n", g_pCGInterface->m_IRM.GetResObj<CIFExtQuickSlotSecond>(1351, 1)->GetPos().y);

            fclose(file);


            mainx = g_CGame->GetRes().res->height;
            mainy = g_CGame->GetRes().res->width;
            barx = g_pCGInterface->m_IRM.GetResObj<CIFExtQuickSlotSecond>(1351, 1)->GetPos().x;
            bary = g_pCGInterface->m_IRM.GetResObj<CIFExtQuickSlotSecond>(1351, 1)->GetPos().y;


        }
    }
}