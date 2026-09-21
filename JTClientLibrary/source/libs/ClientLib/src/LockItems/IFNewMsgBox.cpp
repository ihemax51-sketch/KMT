#include <Game.h>
#include "IFNewMsgBox.h"
#include <NetProcessIn.h>
#include <BSLib/Debug.h>
#include <GInterface.h>
#include <TextStringManager.h>
#include <SRIFLib/NIFEnchantWnd.h>

#define GDR_STATIC 10
#define GDR_OK_BTN 11
#define GDR_CANCEL_BTN 12

GFX_IMPLEMENT_DYNCREATE(CIFNewMsgBox, CIFMainFrame)
GFX_BEGIN_MESSAGE_MAP(CIFNewMsgBox, CIFMainFrame)
                    ONG_COMMAND(GDR_OK_BTN, &CIFNewMsgBox::On_BtnClick_1)
                    ONG_COMMAND(GDR_CANCEL_BTN, &CIFNewMsgBox::On_BtnClick_2)
GFX_END_MESSAGE_MAP()

CIFNewMsgBox::CIFNewMsgBox(void) {
    ok = 0;
    cancel = 0;
    write = 0;
    //UniqueID = 0;
    //TeleportType = 0;
    //targetTeleport = 0;
    BS_DEBUG_LOW(">" __FUNCTION__);
}
CIFNewMsgBox::~CIFNewMsgBox(void) {
    BS_DEBUG_LOW(">" __FUNCTION__);
}

bool CIFNewMsgBox::OnCreate(long ln) {
    //	printf("> " __FUNCTION__ "(%d)\n", ln);

    // Populate inherited members
    CIFMainFrame::OnCreate(ln);
    // TB_Func_13("interface\\frame\\mframe_wnd_", 0, 0);


    this->m_pTitleText->SetText(KmtGetText(L"UIIT_KMT_CONFIRMATION_WINDOW"));
    this->TB_Func_13("interface\\messagebox\\msgbox2_window_", 0, 0);
    wnd_rect sz;
    sz.pos.x= 16;
    sz.pos.y = 40;
    sz.size.width = 286;
    sz.size.height = 120;

    CIFNormalTile*mytile1 = (CIFNormalTile* ) CGWnd::CreateInstance(this, GFX_RUNTIME_CLASS(CIFNormalTile), sz, 101, 1);
    mytile1->TB_Func_13("interface\\ifcommon\\bg_tile\\com_bg_tile_b.ddj", 0, 0);


    sz.pos.x= 8;
    sz.pos.y = 50;
    sz.size.width = 292;
    sz.size.height = 25;
    write = (CIFTextBox*) CGWnd::CreateInstance(this, GFX_RUNTIME_CLASS(CIFTextBox), sz, GDR_STATIC, 0);
    write->SetLineHeight(14);

    //write->m_FontTexture.SetColor(D3DCOLOR_ARGB(255,239,218,164));




    sz.pos.x = 84;
    sz.pos.y = 115;
    sz.size.width = 76;
    sz.size.height = 24;
    ok = (CIFButton*) CGWnd::CreateInstance(this, GFX_RUNTIME_CLASS(CIFButton), sz, GDR_OK_BTN, 0);
    ok->TB_Func_13("interface\\ifcommon\\com_mid_button.ddj", 0, 0);
    ok->SetText(KmtGetText(L"UIIT_KMT_CONFIRM"));



    sz.pos.x = 164;
    sz.pos.y = 115;
    sz.size.width = 76;
    sz.size.height = 24;

    cancel = (CIFButton*) CGWnd::CreateInstance(this, GFX_RUNTIME_CLASS(CIFButton), sz, GDR_CANCEL_BTN, 0);
    cancel->TB_Func_13("interface\\ifcommon\\com_mid_button.ddj", 0, 0);
    cancel->SetText(KmtGetText(L"UIIT_KMT_CANCEL"));






    m_pCloseBtn->ShowGWnd(false);

    this->ShowGWnd(false);
    UpdateMenuSize();


    return true;
}

void CIFNewMsgBox::OnUpdate() {
}
int itemSlotID = 0;
unsigned short itemTypeID = 0;
void CIFNewMsgBox::SetItemInfo(int ItemSlotIDs, unsigned short ItemTypeIDs) {
    itemSlotID = ItemSlotIDs;
    itemTypeID = ItemTypeIDs;
}
void CIFNewMsgBox::UpdateMenuSize() {

    USHORT PosX = 0, PosY = 0;
    PosY = (g_CGame->GetRes().res->height/2) - (this->GetSize().height/2);
    PosX = (g_CGame->GetRes().res->width/2) - (this->GetSize().width/2);
    this->MoveGWnd(PosX, PosY);
    BringToFront();

}

void CIFNewMsgBox::On_BtnClick_1() {
    if(Type == 0)
    {
        if(g_pCGInterface->GetGuiFromList<CNIFEnchantWnd>(168) != NULL)
        {
            g_pCGInterface->GetGuiFromList<CNIFEnchantWnd>(168)->StartAlchemyWOProofStone();
        }
        this->OnCloseWnd();
    }
    else if(Type == 1)
    {
        /// SHUT DOWN
        ExitProcess(0);
    }
}

void CIFNewMsgBox::On_BtnClick_2() {
    itemSlotID = -1;
    itemTypeID = -1;
    this->ShowGWnd(false);

}
void CIFNewMsgBox::SetType(byte WndFrom){
    Type = WndFrom;
    if(Type == 0)
    {
        write->SetText(KmtGetText(L"UIIT_KMT_UNABLE_TO_FIND_MAGIC_PROOF_STONE_IN_ALCHEMY_WINDOW_WHEN_THE_REINFORCEMENT_IS_F_C84EDB9D"));
    }
    else if(Type == 1)
    {
        write->SetText(KmtGetText(L"UIIT_KMT_DO_YOU_WANT_CLOSE_CLIENT"));
    }
}

