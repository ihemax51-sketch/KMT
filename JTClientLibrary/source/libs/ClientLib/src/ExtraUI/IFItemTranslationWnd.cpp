#include <Game.h>
#include <BSLib/Debug.h>
#include <GInterface.h>
#include <BSLib/multibyte.h>
#include <CustomData/CustomSettingManager.h>
#include "IFItemTranslationWnd.h"




GFX_IMPLEMENT_DYNCREATE(CIFItemTranslationWnd, CIFMainFrame)

GFX_BEGIN_MESSAGE_MAP(CIFItemTranslationWnd, CIFMainFrame)
                    ONG_COMMAND(11, &On_BtnClick)
                    ONG_COMMAND(12, &Clear)
                    ONG_BSSig_u38(2, 0x806D, 13, &CIFItemTranslationWnd::OnListChatThing)
GFX_END_MESSAGE_MAP()

CIFItemTranslationWnd::CIFItemTranslationWnd(void){
}
CIFItemTranslationWnd::~CIFItemTranslationWnd(void){

}
int CIFItemTranslationWnd::Func_4(int a2) {
    int v1 = 0;
    while (a2 != v1 + 100) {
        if (++v1 >= 5)
            return -1;
    }

    return 100;
}
int CIFItemTranslationWnd::Func_36(int a1, short action, int a3, int a4) {
    if (action <= 0) {
        if (action < 0) {
            m_Scroll->sub_65A5C0(0);
        }
    } else {
        m_Scroll->sub_65A5A0(0);
    }

    return 1;
}
bool CIFItemTranslationWnd::OnCreate(long ln)
{

    // Populate inherited members
    CIFMainFrame::OnCreate(ln);

    m_IRM.LoadFromFile("clientlibrary\\resinfo\\ifitemtranslationwnd.txt");
    m_IRM.CreateInterfaceSection("Create", this);

    this->SetText(KmtGetText(L"UIIT_KMT_EXCHANGE_ITEM_WINDOW"));
    this->m_IRM.GetResObj(5, 1)->SetText(KmtGetText(L"UIIT_KMT_SELECT"));
    this->m_IRM.GetResObj(11, 1)->SetText(KmtGetText(L"UIIT_KMT_EXCHANGE"));
    this->m_IRM.GetResObj(12, 1)->SetText(KmtGetText(L"UIIT_KMT_CANCEL"));

    m_Scroll = this->m_IRM.GetResObj<CIFVerticalScroll>(7, 1);
    m_TextBox = this->m_IRM.GetResObj<CIFTextBox>(13, 1);


    m_TextBox->N00000608 = this;
    m_TextBox->m_LinesOfHistory = 15;

    m_TextBox->m_HeightInLines = 3;
    m_TextBox->BlueWithSelected = 1;
    m_TextBox->SetLineHeight(22);
    m_TextBox->SetBottomAligned(true);
    m_TextBox->SetIndentAfterBreak(true);
    m_TextBox->SetRespondToMouseMove(true);
    m_TextBox->SetHightlineLine(true);
    m_TextBox->sub_638D50(0);
    m_TextBox->SetHighlightColor(D3DCOLOR_RGBA(255, 255, 255, 102));
    m_TextBox->sub_638D40(0);
    m_TextBox->Func_01(0);
    m_TextBox->FUN_00638d20(1);
    this->m_IRM.GetResObj(17, 1)->SetText(L"0");

    m_TextBox->N0000969B = true;
    m_TextBox->LinkScrollbar(m_Scroll);


    mySlot = m_IRM.GetResObj<CIFItemTranslationSlot>(14, 1);
    mySlot->m_pMySlot->m_pSlot->SetSlot(510);
    mySlot->m_pMySlot->m_pSlot->SetType(0xC);


    m_Scroll->sub_65A5A0(9999);
    CIFListCtrl *pList;
    pList = m_TextBox;
    pList->N0000969B = true;

    this->m_IRM.GetResObj(15, 1)->SetStyleThingy(TOOLTIP);


    UpdateMenuSize();
    this->ShowGWnd(false);
    return true;
}
void CIFItemTranslationWnd::OnListChatThing(int a1, int a2) {
    BS_DEBUG_LOW("%s(%d, %d)", __FUNCTION__, a1, a2);

    int id = GetCurrentEventMsgCtrlId();
    CIFListCtrl *pList;

    if (id == m_TextBox->UniqueID()) {
        pList = m_TextBox;
    } else {
        // Joymax was using this here ... why ... how ... ???
        assert(FALSE);
    }

    CIFListCtrl::SLineOfText *line = pList->sub_63A940();

    std::n_wstring str;
    line->m_font->GetText(&str);

    SelectedItemType = str;
    //  line->m_font->m_color_bg = D3DCOLOR_RGBA(255, 255, 255, 255);

}
void CIFItemTranslationWnd::UpdateMenuSize()
{
    if(m_Settings->ItemTranslationPayment == 0)
    {
        this->m_IRM.GetResObj(15, 1)->SetTooltip(KmtGetText(L"UIIT_KMT_SILK"));
    }
    else
    {
        this->m_IRM.GetResObj(15, 1)->SetTooltip(KmtGetText(L"UIIT_KMT_GOLD"));
    }
    int PosX = 0, PosY = 0;
    PosY = (g_CGame->GetRes().res->height/2) - (this->GetSize().height/2);
    PosX = (g_CGame->GetRes().res->width/2) - (this->GetSize().width/2);
    this->MoveGWnd(PosX, PosY);
    BringToFront();
    g_pCGInterface->LockMovement(13);

}


void CIFItemTranslationWnd::OnUpdate()
{
//    m_TextBox->subTest(true);
}
void CIFItemTranslationWnd::Clear()
{
    OnCloseWnd();
}
#define REGION_1
void CIFItemTranslationWnd::On_BtnClick() {
    if(this->mySlot->m_pMySlot->m_pSlot->ItemInfo != NULL)
    {
        if(this->mySlot->m_pMySlot->m_pSlot->ItemInfo->GetItemData() != NULL && !SelectedItemType.empty())
        {
            std::n_wstring TargetCodeName = this->mySlot->m_pMySlot->m_pSlot->ItemInfo->GetItemData()->CodeName;
            #ifdef REGION_1
            if(this->mySlot->m_pMySlot->m_pSlot->ItemInfo->GetItemData()->IsChSword())
            {
                if (SelectedItemType == KmtGetText(L"UIIT_KMT_BOW")) {

                    size_t pos = TargetCodeName.find(L"_SWORD_");
                    if (pos != std::wstring::npos) {
                        TargetCodeName.replace(pos, 7, L"_BOW_");
                    }
                }
                else if (SelectedItemType == KmtGetText(L"UIIT_KMT_BLADE")) {

                    size_t pos = TargetCodeName.find(L"_SWORD_");
                    if (pos != std::wstring::npos) {
                        TargetCodeName.replace(pos, 7, L"_BLADE_");
                    }
                }
                else if (SelectedItemType == KmtGetText(L"UIIT_KMT_SPEAR")) {

                    size_t pos = TargetCodeName.find(L"_SWORD_");
                    if (pos != std::wstring::npos) {
                        TargetCodeName.replace(pos, 7, L"_SPEAR_");
                    }
                }
                else if (SelectedItemType == KmtGetText(L"UIIT_KMT_GLAVIE")) {

                    size_t pos = TargetCodeName.find(L"_SWORD_");
                    if (pos != std::wstring::npos) {
                        TargetCodeName.replace(pos, 7, L"_TBLADE_");
                    }
                }

            }
            else if(this->mySlot->m_pMySlot->m_pSlot->ItemInfo->GetItemData()->IsChBlade())
            {
                if (SelectedItemType == KmtGetText(L"UIIT_KMT_BOW")) {

                    size_t pos = TargetCodeName.find(L"_BLADE_");
                    if (pos != std::wstring::npos) {
                        TargetCodeName.replace(pos, 7, L"_BOW_");
                    }
                }
                else if (SelectedItemType == KmtGetText(L"UIIT_KMT_SWORD")) {

                    size_t pos = TargetCodeName.find(L"_BLADE_");
                    if (pos != std::wstring::npos) {
                        TargetCodeName.replace(pos, 7, L"_SWORD_");
                    }
                }
                else if (SelectedItemType == KmtGetText(L"UIIT_KMT_SPEAR")) {

                    size_t pos = TargetCodeName.find(L"_BLADE_");
                    if (pos != std::wstring::npos) {
                        TargetCodeName.replace(pos, 7, L"_SPEAR_");
                    }
                }
                else if (SelectedItemType == KmtGetText(L"UIIT_KMT_GLAVIE")) {

                    size_t pos = TargetCodeName.find(L"_BLADE_");
                    if (pos != std::wstring::npos) {
                        TargetCodeName.replace(pos, 7, L"_TBLADE_");
                    }
                }
            }
            else if(this->mySlot->m_pMySlot->m_pSlot->ItemInfo->GetItemData()->IsChSpear())
            {
                if (SelectedItemType == KmtGetText(L"UIIT_KMT_BOW")) {

                    size_t pos = TargetCodeName.find(L"_SPEAR_");
                    if (pos != std::wstring::npos) {
                        TargetCodeName.replace(pos, 7, L"_BOW_");
                    }
                }
                else if (SelectedItemType == KmtGetText(L"UIIT_KMT_SWORD")) {

                    size_t pos = TargetCodeName.find(L"_SPEAR_");
                    if (pos != std::wstring::npos) {
                        TargetCodeName.replace(pos, 7, L"_SWORD_");
                    }
                }
                else if (SelectedItemType == KmtGetText(L"UIIT_KMT_BLADE")) {

                    size_t pos = TargetCodeName.find(L"_SPEAR_");
                    if (pos != std::wstring::npos) {
                        TargetCodeName.replace(pos, 7, L"_BLADE_");
                    }
                }
                else if (SelectedItemType == KmtGetText(L"UIIT_KMT_GLAVIE")) {

                    size_t pos = TargetCodeName.find(L"_SPEAR_");
                    if (pos != std::wstring::npos) {
                        TargetCodeName.replace(pos, 7, L"_TBLADE_");
                    }
                }
            }
            else if(this->mySlot->m_pMySlot->m_pSlot->ItemInfo->GetItemData()->IsChTBlade())
            {
                if (SelectedItemType == KmtGetText(L"UIIT_KMT_BOW")) {

                    size_t pos = TargetCodeName.find(L"_TBLADE_");
                    if (pos != std::wstring::npos) {
                        TargetCodeName.replace(pos, 8, L"_BOW_");
                    }
                }
                else if (SelectedItemType == KmtGetText(L"UIIT_KMT_SWORD")) {

                    size_t pos = TargetCodeName.find(L"_TBLADE_");
                    if (pos != std::wstring::npos) {
                        TargetCodeName.replace(pos, 8, L"_SWORD_");
                    }
                }
                else if (SelectedItemType == KmtGetText(L"UIIT_KMT_BLADE")) {

                    size_t pos = TargetCodeName.find(L"_TBLADE_");
                    if (pos != std::wstring::npos) {
                        TargetCodeName.replace(pos, 8, L"_BLADE_");
                    }
                }
                else if (SelectedItemType == KmtGetText(L"UIIT_KMT_SPEAR")) {

                    size_t pos = TargetCodeName.find(L"_TBLADE_");
                    if (pos != std::wstring::npos) {
                        TargetCodeName.replace(pos, 8, L"_SPEAR_");
                    }
                }
            }
            else if(this->mySlot->m_pMySlot->m_pSlot->ItemInfo->GetItemData()->IsChBow())
            {
                if (SelectedItemType == KmtGetText(L"UIIT_KMT_GLAVIE")) {

                    size_t pos = TargetCodeName.find(L"_BOW_");
                    if (pos != std::wstring::npos) {
                        TargetCodeName.replace(pos, 5, L"_TBLADE_");
                    }
                }
                else if (SelectedItemType == KmtGetText(L"UIIT_KMT_SWORD")) {

                    size_t pos = TargetCodeName.find(L"_BOW_");
                    if (pos != std::wstring::npos) {
                        TargetCodeName.replace(pos, 5, L"_SWORD_");
                    }
                }
                else if (SelectedItemType == KmtGetText(L"UIIT_KMT_BLADE")) {

                    size_t pos = TargetCodeName.find(L"_BOW_");
                    if (pos != std::wstring::npos) {
                        TargetCodeName.replace(pos, 5, L"_BLADE_");
                    }
                }
                else if (SelectedItemType == KmtGetText(L"UIIT_KMT_SPEAR")) {

                    size_t pos = TargetCodeName.find(L"_BOW_");
                    if (pos != std::wstring::npos) {
                        TargetCodeName.replace(pos, 5, L"_SPEAR_");
                    }
                }
            }





            else if(this->mySlot->m_pMySlot->m_pSlot->ItemInfo->GetItemData()->IsEuSword())
            {
                if(SelectedItemType == KmtGetText(L"UIIT_KMT_ONEHAND_SWORD"))
                {
                    size_t pos = TargetCodeName.find(L"_SWORD_");
                    if (pos != std::wstring::npos) {
                        TargetCodeName.replace(pos, 7, L"_SWORD_");
                    }
                }
                else if (SelectedItemType == KmtGetText(L"UIIT_KMT_TWOHAND_SWORD")) {

                    size_t pos = TargetCodeName.find(L"_SWORD_");
                    if (pos != std::wstring::npos) {
                        TargetCodeName.replace(pos, 7, L"_TSWORD_");
                    }
                }
                else if (SelectedItemType == KmtGetText(L"UIIT_KMT_AXE")) {

                    size_t pos = TargetCodeName.find(L"_SWORD_");
                    if (pos != std::wstring::npos) {
                        TargetCodeName.replace(pos, 7, L"_AXE_");
                    }
                }
                else if (SelectedItemType == KmtGetText(L"UIIT_KMT_DARK_STAFF")) {

                    size_t pos = TargetCodeName.find(L"_SWORD_");
                    if (pos != std::wstring::npos) {
                        TargetCodeName.replace(pos, 7, L"_DARKSTAFF_");
                    }
                }
                else if (SelectedItemType == KmtGetText(L"UIIT_KMT_TWOHAND_STAFF")) {

                    size_t pos = TargetCodeName.find(L"_SWORD_");
                    if (pos != std::wstring::npos) {
                        TargetCodeName.replace(pos, 7, L"_TSTAFF_");
                    }
                }
                else if (SelectedItemType == KmtGetText(L"UIIT_KMT_CROSSBOW")) {

                    size_t pos = TargetCodeName.find(L"_SWORD_");
                    if (pos != std::wstring::npos) {
                        TargetCodeName.replace(pos, 7, L"_CROSSBOW_");
                    }
                }
                else if (SelectedItemType == KmtGetText(L"UIIT_KMT_DAGGER")) {

                    size_t pos = TargetCodeName.find(L"_SWORD_");
                    if (pos != std::wstring::npos) {
                        TargetCodeName.replace(pos, 7, L"_DAGGER_");
                    }
                }
                else if (SelectedItemType == KmtGetText(L"UIIT_KMT_HARP")) {

                    size_t pos = TargetCodeName.find(L"_SWORD_");
                    if (pos != std::wstring::npos) {
                        TargetCodeName.replace(pos, 7, L"_HARP_");
                    }
                }
                else if (SelectedItemType == KmtGetText(L"UIIT_KMT_CLERIC_ROD")) {

                    size_t pos = TargetCodeName.find(L"_SWORD_");
                    if (pos != std::wstring::npos) {
                        TargetCodeName.replace(pos, 7, L"_STAFF_");
                    }
                }
            }
            else if(this->mySlot->m_pMySlot->m_pSlot->ItemInfo->GetItemData()->IsEuTSword())
            {
                if(SelectedItemType == KmtGetText(L"UIIT_KMT_ONEHAND_SWORD"))
                {
                    size_t pos = TargetCodeName.find(L"_TSWORD_");
                    if (pos != std::wstring::npos) {
                        TargetCodeName.replace(pos, 8, L"_SWORD_");
                    }
                }
                else if (SelectedItemType == KmtGetText(L"UIIT_KMT_TWOHAND_SWORD")) {

                    size_t pos = TargetCodeName.find(L"_TSWORD_");
                    if (pos != std::wstring::npos) {
                        TargetCodeName.replace(pos, 8, L"_TSWORD_");
                    }
                }
                else if (SelectedItemType == KmtGetText(L"UIIT_KMT_AXE")) {

                    size_t pos = TargetCodeName.find(L"_TSWORD_");
                    if (pos != std::wstring::npos) {
                        TargetCodeName.replace(pos, 8, L"_AXE_");
                    }
                }
                else if (SelectedItemType == KmtGetText(L"UIIT_KMT_DARK_STAFF")) {

                    size_t pos = TargetCodeName.find(L"_TSWORD_");
                    if (pos != std::wstring::npos) {
                        TargetCodeName.replace(pos, 8, L"_DARKSTAFF_");
                    }
                }
                else if (SelectedItemType == KmtGetText(L"UIIT_KMT_TWOHAND_STAFF")) {

                    size_t pos = TargetCodeName.find(L"_TSWORD_");
                    if (pos != std::wstring::npos) {
                        TargetCodeName.replace(pos, 8, L"_TSTAFF_");
                    }
                }
                else if (SelectedItemType == KmtGetText(L"UIIT_KMT_CROSSBOW")) {

                    size_t pos = TargetCodeName.find(L"_TSWORD_");
                    if (pos != std::wstring::npos) {
                        TargetCodeName.replace(pos, 8, L"_CROSSBOW_");
                    }
                }
                else if (SelectedItemType == KmtGetText(L"UIIT_KMT_DAGGER")) {

                    size_t pos = TargetCodeName.find(L"_TSWORD_");
                    if (pos != std::wstring::npos) {
                        TargetCodeName.replace(pos, 8, L"_DAGGER_");
                    }
                }
                else if (SelectedItemType == KmtGetText(L"UIIT_KMT_HARP")) {

                    size_t pos = TargetCodeName.find(L"_TSWORD_");
                    if (pos != std::wstring::npos) {
                        TargetCodeName.replace(pos, 8, L"_HARP_");
                    }
                }
                else if (SelectedItemType == KmtGetText(L"UIIT_KMT_CLERIC_ROD")) {

                    size_t pos = TargetCodeName.find(L"_TSWORD_");
                    if (pos != std::wstring::npos) {
                        TargetCodeName.replace(pos, 8, L"_STAFF_");
                    }
                }
            }
            else if(this->mySlot->m_pMySlot->m_pSlot->ItemInfo->GetItemData()->IsEuAxe())
            {
                if(SelectedItemType == KmtGetText(L"UIIT_KMT_ONEHAND_SWORD"))
                {
                    size_t pos = TargetCodeName.find(L"_AXE_");
                    if (pos != std::wstring::npos) {
                        TargetCodeName.replace(pos, 5, L"_SWORD_");
                    }
                }
                else if (SelectedItemType == KmtGetText(L"UIIT_KMT_TWOHAND_SWORD")) {

                    size_t pos = TargetCodeName.find(L"_AXE_");
                    if (pos != std::wstring::npos) {
                        TargetCodeName.replace(pos, 5, L"_TSWORD_");
                    }
                }
                else if (SelectedItemType == KmtGetText(L"UIIT_KMT_AXE")) {

                    size_t pos = TargetCodeName.find(L"_AXE_");
                    if (pos != std::wstring::npos) {
                        TargetCodeName.replace(pos, 5, L"_AXE_");
                    }
                }
                else if (SelectedItemType == KmtGetText(L"UIIT_KMT_DARK_STAFF")) {

                    size_t pos = TargetCodeName.find(L"_AXE_");
                    if (pos != std::wstring::npos) {
                        TargetCodeName.replace(pos, 5, L"_DARKSTAFF_");
                    }
                }
                else if (SelectedItemType == KmtGetText(L"UIIT_KMT_TWOHAND_STAFF")) {

                    size_t pos = TargetCodeName.find(L"_AXE_");
                    if (pos != std::wstring::npos) {
                        TargetCodeName.replace(pos, 5, L"_TSTAFF_");
                    }
                }
                else if (SelectedItemType == KmtGetText(L"UIIT_KMT_CROSSBOW")) {

                    size_t pos = TargetCodeName.find(L"_AXE_");
                    if (pos != std::wstring::npos) {
                        TargetCodeName.replace(pos, 5, L"_CROSSBOW_");
                    }
                }
                else if (SelectedItemType == KmtGetText(L"UIIT_KMT_DAGGER")) {

                    size_t pos = TargetCodeName.find(L"_AXE_");
                    if (pos != std::wstring::npos) {
                        TargetCodeName.replace(pos, 5, L"_DAGGER_");
                    }
                }
                else if (SelectedItemType == KmtGetText(L"UIIT_KMT_HARP")) {

                    size_t pos = TargetCodeName.find(L"_AXE_");
                    if (pos != std::wstring::npos) {
                        TargetCodeName.replace(pos, 5, L"_HARP_");
                    }
                }
                else if (SelectedItemType == KmtGetText(L"UIIT_KMT_CLERIC_ROD")) {

                    size_t pos = TargetCodeName.find(L"_AXE_");
                    if (pos != std::wstring::npos) {
                        TargetCodeName.replace(pos, 5, L"_STAFF_");
                    }
                }
            }
            else if(this->mySlot->m_pMySlot->m_pSlot->ItemInfo->GetItemData()->IsEuDarkStaff())
            {
                if(SelectedItemType == KmtGetText(L"UIIT_KMT_ONEHAND_SWORD"))
                {
                    size_t pos = TargetCodeName.find(L"_DARKSTAFF_");
                    if (pos != std::wstring::npos) {
                        TargetCodeName.replace(pos, 11, L"_SWORD_");
                    }
                }
                else if (SelectedItemType == KmtGetText(L"UIIT_KMT_TWOHAND_SWORD")) {

                    size_t pos = TargetCodeName.find(L"_DARKSTAFF_");
                    if (pos != std::wstring::npos) {
                        TargetCodeName.replace(pos, 11, L"_TSWORD_");
                    }
                }
                else if (SelectedItemType == KmtGetText(L"UIIT_KMT_AXE")) {

                    size_t pos = TargetCodeName.find(L"_DARKSTAFF_");
                    if (pos != std::wstring::npos) {
                        TargetCodeName.replace(pos, 11, L"_AXE_");
                    }
                }
                else if (SelectedItemType == KmtGetText(L"UIIT_KMT_DARK_STAFF")) {

                    size_t pos = TargetCodeName.find(L"_DARKSTAFF_");
                    if (pos != std::wstring::npos) {
                        TargetCodeName.replace(pos, 11, L"_DARKSTAFF_");
                    }
                }
                else if (SelectedItemType == KmtGetText(L"UIIT_KMT_TWOHAND_STAFF")) {

                    size_t pos = TargetCodeName.find(L"_DARKSTAFF_");
                    if (pos != std::wstring::npos) {
                        TargetCodeName.replace(pos, 11, L"_TSTAFF_");
                    }
                }
                else if (SelectedItemType == KmtGetText(L"UIIT_KMT_CROSSBOW")) {

                    size_t pos = TargetCodeName.find(L"_DARKSTAFF_");
                    if (pos != std::wstring::npos) {
                        TargetCodeName.replace(pos, 11, L"_CROSSBOW_");
                    }
                }
                else if (SelectedItemType == KmtGetText(L"UIIT_KMT_DAGGER")) {

                    size_t pos = TargetCodeName.find(L"_DARKSTAFF_");
                    if (pos != std::wstring::npos) {
                        TargetCodeName.replace(pos, 11, L"_DAGGER_");
                    }
                }
                else if (SelectedItemType == KmtGetText(L"UIIT_KMT_HARP")) {

                    size_t pos = TargetCodeName.find(L"_DARKSTAFF_");
                    if (pos != std::wstring::npos) {
                        TargetCodeName.replace(pos, 11, L"_HARP_");
                    }
                }
                else if (SelectedItemType == KmtGetText(L"UIIT_KMT_CLERIC_ROD")) {

                    size_t pos = TargetCodeName.find(L"_DARKSTAFF_");
                    if (pos != std::wstring::npos) {
                        TargetCodeName.replace(pos, 11, L"_STAFF_");
                    }
                }
            }
            else if(this->mySlot->m_pMySlot->m_pSlot->ItemInfo->GetItemData()->IsEuTStaff())
            {
                if(SelectedItemType == KmtGetText(L"UIIT_KMT_ONEHAND_SWORD"))
                {
                    size_t pos = TargetCodeName.find(L"_TSTAFF_");
                    if (pos != std::wstring::npos) {
                        TargetCodeName.replace(pos, 8, L"_SWORD_");
                    }
                }
                else if (SelectedItemType == KmtGetText(L"UIIT_KMT_TWOHAND_SWORD")) {

                    size_t pos = TargetCodeName.find(L"_TSTAFF_");
                    if (pos != std::wstring::npos) {
                        TargetCodeName.replace(pos, 8, L"_TSWORD_");
                    }
                }
                else if (SelectedItemType == KmtGetText(L"UIIT_KMT_AXE")) {

                    size_t pos = TargetCodeName.find(L"_TSTAFF_");
                    if (pos != std::wstring::npos) {
                        TargetCodeName.replace(pos, 8, L"_AXE_");
                    }
                }
                else if (SelectedItemType == KmtGetText(L"UIIT_KMT_DARK_STAFF")) {

                    size_t pos = TargetCodeName.find(L"_TSTAFF_");
                    if (pos != std::wstring::npos) {
                        TargetCodeName.replace(pos, 8, L"_DARKSTAFF_");
                    }
                }
                else if (SelectedItemType == KmtGetText(L"UIIT_KMT_TWOHAND_STAFF")) {

                    size_t pos = TargetCodeName.find(L"_TSTAFF_");
                    if (pos != std::wstring::npos) {
                        TargetCodeName.replace(pos, 8, L"_TSTAFF_");
                    }
                }
                else if (SelectedItemType == KmtGetText(L"UIIT_KMT_CROSSBOW")) {

                    size_t pos = TargetCodeName.find(L"_TSTAFF_");
                    if (pos != std::wstring::npos) {
                        TargetCodeName.replace(pos, 8, L"_CROSSBOW_");
                    }
                }
                else if (SelectedItemType == KmtGetText(L"UIIT_KMT_DAGGER")) {

                    size_t pos = TargetCodeName.find(L"_TSTAFF_");
                    if (pos != std::wstring::npos) {
                        TargetCodeName.replace(pos, 8, L"_DAGGER_");
                    }
                }
                else if (SelectedItemType == KmtGetText(L"UIIT_KMT_HARP")) {

                    size_t pos = TargetCodeName.find(L"_TSTAFF_");
                    if (pos != std::wstring::npos) {
                        TargetCodeName.replace(pos, 8, L"_HARP_");
                    }
                }
                else if (SelectedItemType == KmtGetText(L"UIIT_KMT_CLERIC_ROD")) {

                    size_t pos = TargetCodeName.find(L"_TSTAFF_");
                    if (pos != std::wstring::npos) {
                        TargetCodeName.replace(pos, 8, L"_STAFF_");
                    }
                }
            }

            else if(this->mySlot->m_pMySlot->m_pSlot->ItemInfo->GetItemData()->IsEuCrossbow())
            {
                if(SelectedItemType == KmtGetText(L"UIIT_KMT_ONEHAND_SWORD"))
                {
                    size_t pos = TargetCodeName.find(L"_CROSSBOW_");
                    if (pos != std::wstring::npos) {
                        TargetCodeName.replace(pos, 10, L"_SWORD_");
                    }
                }
                else if (SelectedItemType == KmtGetText(L"UIIT_KMT_TWOHAND_SWORD")) {

                    size_t pos = TargetCodeName.find(L"_CROSSBOW_");
                    if (pos != std::wstring::npos) {
                        TargetCodeName.replace(pos, 10, L"_TSWORD_");
                    }
                }
                else if (SelectedItemType == KmtGetText(L"UIIT_KMT_AXE")) {

                    size_t pos = TargetCodeName.find(L"_CROSSBOW_");
                    if (pos != std::wstring::npos) {
                        TargetCodeName.replace(pos, 10, L"_AXE_");
                    }
                }
                else if (SelectedItemType == KmtGetText(L"UIIT_KMT_DARK_STAFF")) {

                    size_t pos = TargetCodeName.find(L"_CROSSBOW_");
                    if (pos != std::wstring::npos) {
                        TargetCodeName.replace(pos, 10, L"_DARKSTAFF_");
                    }
                }
                else if (SelectedItemType == KmtGetText(L"UIIT_KMT_TWOHAND_STAFF")) {

                    size_t pos = TargetCodeName.find(L"_CROSSBOW_");
                    if (pos != std::wstring::npos) {
                        TargetCodeName.replace(pos, 10, L"_TSTAFF_");
                    }
                }
                else if (SelectedItemType == KmtGetText(L"UIIT_KMT_CROSSBOW")) {

                    size_t pos = TargetCodeName.find(L"_CROSSBOW_");
                    if (pos != std::wstring::npos) {
                        TargetCodeName.replace(pos, 10, L"_CROSSBOW_");
                    }
                }
                else if (SelectedItemType == KmtGetText(L"UIIT_KMT_DAGGER")) {

                    size_t pos = TargetCodeName.find(L"_CROSSBOW_");
                    if (pos != std::wstring::npos) {
                        TargetCodeName.replace(pos, 10, L"_DAGGER_");
                    }
                }
                else if (SelectedItemType == KmtGetText(L"UIIT_KMT_HARP")) {

                    size_t pos = TargetCodeName.find(L"_CROSSBOW_");
                    if (pos != std::wstring::npos) {
                        TargetCodeName.replace(pos, 10, L"_HARP_");
                    }
                }
                else if (SelectedItemType == KmtGetText(L"UIIT_KMT_CLERIC_ROD")) {

                    size_t pos = TargetCodeName.find(L"_CROSSBOW_");
                    if (pos != std::wstring::npos) {
                        TargetCodeName.replace(pos, 10, L"_STAFF_");
                    }
                }
            }

            else if(this->mySlot->m_pMySlot->m_pSlot->ItemInfo->GetItemData()->IsEuDagger())
            {
                if(SelectedItemType == KmtGetText(L"UIIT_KMT_ONEHAND_SWORD"))
                {
                    size_t pos = TargetCodeName.find(L"_DAGGER_");
                    if (pos != std::wstring::npos) {
                        TargetCodeName.replace(pos, 8, L"_SWORD_");
                    }
                }
                else if (SelectedItemType == KmtGetText(L"UIIT_KMT_TWOHAND_SWORD")) {

                    size_t pos = TargetCodeName.find(L"_DAGGER_");
                    if (pos != std::wstring::npos) {
                        TargetCodeName.replace(pos, 8, L"_TSWORD_");
                    }
                }
                else if (SelectedItemType == KmtGetText(L"UIIT_KMT_AXE")) {

                    size_t pos = TargetCodeName.find(L"_DAGGER_");
                    if (pos != std::wstring::npos) {
                        TargetCodeName.replace(pos, 8, L"_AXE_");
                    }
                }
                else if (SelectedItemType == KmtGetText(L"UIIT_KMT_DARK_STAFF")) {

                    size_t pos = TargetCodeName.find(L"_DAGGER_");
                    if (pos != std::wstring::npos) {
                        TargetCodeName.replace(pos, 8, L"_DARKSTAFF_");
                    }
                }
                else if (SelectedItemType == KmtGetText(L"UIIT_KMT_TWOHAND_STAFF")) {

                    size_t pos = TargetCodeName.find(L"_DAGGER_");
                    if (pos != std::wstring::npos) {
                        TargetCodeName.replace(pos, 8, L"_TSTAFF_");
                    }
                }
                else if (SelectedItemType == KmtGetText(L"UIIT_KMT_CROSSBOW")) {

                    size_t pos = TargetCodeName.find(L"_DAGGER_");
                    if (pos != std::wstring::npos) {
                        TargetCodeName.replace(pos, 8, L"_CROSSBOW_");
                    }
                }
                else if (SelectedItemType == KmtGetText(L"UIIT_KMT_DAGGER")) {

                    size_t pos = TargetCodeName.find(L"_DAGGER_");
                    if (pos != std::wstring::npos) {
                        TargetCodeName.replace(pos, 8, L"_DAGGER_");
                    }
                }
                else if (SelectedItemType == KmtGetText(L"UIIT_KMT_HARP")) {

                    size_t pos = TargetCodeName.find(L"_DAGGER_");
                    if (pos != std::wstring::npos) {
                        TargetCodeName.replace(pos, 8, L"_HARP_");
                    }
                }
                else if (SelectedItemType == KmtGetText(L"UIIT_KMT_CLERIC_ROD")) {

                    size_t pos = TargetCodeName.find(L"_DAGGER_");
                    if (pos != std::wstring::npos) {
                        TargetCodeName.replace(pos, 8, L"_STAFF_");
                    }
                }
            }

            else if(this->mySlot->m_pMySlot->m_pSlot->ItemInfo->GetItemData()->IsEuHarp())
            {
                if(SelectedItemType == KmtGetText(L"UIIT_KMT_ONEHAND_SWORD"))
                {
                    size_t pos = TargetCodeName.find(L"_HARP_");
                    if (pos != std::wstring::npos) {
                        TargetCodeName.replace(pos, 6, L"_SWORD_");
                    }
                }
                else if (SelectedItemType == KmtGetText(L"UIIT_KMT_TWOHAND_SWORD")) {

                    size_t pos = TargetCodeName.find(L"_HARP_");
                    if (pos != std::wstring::npos) {
                        TargetCodeName.replace(pos, 6, L"_TSWORD_");
                    }
                }
                else if (SelectedItemType == KmtGetText(L"UIIT_KMT_AXE")) {

                    size_t pos = TargetCodeName.find(L"_HARP_");
                    if (pos != std::wstring::npos) {
                        TargetCodeName.replace(pos, 6, L"_AXE_");
                    }
                }
                else if (SelectedItemType == KmtGetText(L"UIIT_KMT_DARK_STAFF")) {

                    size_t pos = TargetCodeName.find(L"_HARP_");
                    if (pos != std::wstring::npos) {
                        TargetCodeName.replace(pos, 6, L"_DARKSTAFF_");
                    }
                }
                else if (SelectedItemType == KmtGetText(L"UIIT_KMT_TWOHAND_STAFF")) {

                    size_t pos = TargetCodeName.find(L"_HARP_");
                    if (pos != std::wstring::npos) {
                        TargetCodeName.replace(pos, 6, L"_TSTAFF_");
                    }
                }
                else if (SelectedItemType == KmtGetText(L"UIIT_KMT_CROSSBOW")) {

                    size_t pos = TargetCodeName.find(L"_HARP_");
                    if (pos != std::wstring::npos) {
                        TargetCodeName.replace(pos, 6, L"_CROSSBOW_");
                    }
                }
                else if (SelectedItemType == KmtGetText(L"UIIT_KMT_DAGGER")) {
                    size_t pos = TargetCodeName.find(L"_HARP_");
                    if (pos != std::wstring::npos) {
                        TargetCodeName.replace(pos, 6, L"_DAGGER_");
                    }
                }
                else if (SelectedItemType == KmtGetText(L"UIIT_KMT_HARP")) {
                    size_t pos = TargetCodeName.find(L"_HARP_");
                    if (pos != std::wstring::npos) {
                        TargetCodeName.replace(pos, 6, L"_HARP_");
                    }
                }
                else if (SelectedItemType == KmtGetText(L"UIIT_KMT_CLERIC_ROD")) {
                    size_t pos = TargetCodeName.find(L"_HARP_");
                    if (pos != std::wstring::npos) {
                        TargetCodeName.replace(pos, 6, L"_STAFF_");
                    }
                }
            }

            else if(this->mySlot->m_pMySlot->m_pSlot->ItemInfo->GetItemData()->IsEuStaff())
            {
                if(SelectedItemType == KmtGetText(L"UIIT_KMT_ONEHAND_SWORD"))
                {
                    size_t pos = TargetCodeName.find(L"_STAFF_");
                    if (pos != std::wstring::npos) {
                        TargetCodeName.replace(pos, 7, L"_SWORD_");
                    }
                }
                else if (SelectedItemType == KmtGetText(L"UIIT_KMT_TWOHAND_SWORD")) {

                    size_t pos = TargetCodeName.find(L"_STAFF_");
                    if (pos != std::wstring::npos) {
                        TargetCodeName.replace(pos, 7, L"_TSWORD_");
                    }
                }
                else if (SelectedItemType == KmtGetText(L"UIIT_KMT_AXE")) {

                    size_t pos = TargetCodeName.find(L"_STAFF_");
                    if (pos != std::wstring::npos) {
                        TargetCodeName.replace(pos, 7, L"_AXE_");
                    }
                }
                else if (SelectedItemType == KmtGetText(L"UIIT_KMT_DARK_STAFF")) {

                    size_t pos = TargetCodeName.find(L"_STAFF_");
                    if (pos != std::wstring::npos) {
                        TargetCodeName.replace(pos, 7, L"_DARKSTAFF_");
                    }
                }
                else if (SelectedItemType == KmtGetText(L"UIIT_KMT_TWOHAND_STAFF")) {

                    size_t pos = TargetCodeName.find(L"_STAFF_");
                    if (pos != std::wstring::npos) {
                        TargetCodeName.replace(pos, 7, L"_TSTAFF_");
                    }
                }
                else if (SelectedItemType == KmtGetText(L"UIIT_KMT_CROSSBOW")) {

                    size_t pos = TargetCodeName.find(L"_STAFF_");
                    if (pos != std::wstring::npos) {
                        TargetCodeName.replace(pos, 7, L"_CROSSBOW_");
                    }
                }
                else if (SelectedItemType == KmtGetText(L"UIIT_KMT_DAGGER")) {
                    size_t pos = TargetCodeName.find(L"_STAFF_");
                    if (pos != std::wstring::npos) {
                        TargetCodeName.replace(pos, 7, L"_DAGGER_");
                    }
                }
                else if (SelectedItemType == KmtGetText(L"UIIT_KMT_HARP")) {
                    size_t pos = TargetCodeName.find(L"_STAFF_");
                    if (pos != std::wstring::npos) {
                        TargetCodeName.replace(pos, 7, L"_HARP_");
                    }
                }
                else if (SelectedItemType == KmtGetText(L"UIIT_KMT_CLERIC_ROD")) {
                    size_t pos = TargetCodeName.find(L"_STAFF_");
                    if (pos != std::wstring::npos) {
                        TargetCodeName.replace(pos, 7, L"_STAFF_");
                    }
                }
            }
            #endif
            if(!TargetCodeName.empty())
            {
                if(m_Settings->EnableItemTranslation)
                {
                    CMsgStreamBuffer buf(0xB298);
                    buf << byte(this->mySlot->m_pMySlot->m_pSlot->GetInventorySlotIndex() + 13);
                    buf << TO_NSTRING(this->mySlot->m_pMySlot->m_pSlot->ItemInfo->GetItemData()->CodeName);
                    buf << TO_NSTRING(TargetCodeName);
                    SendMsg(buf);
                }
            }
            this->m_IRM.GetResObj(17, 1)->SetText(L"0");
            //printf("%ls %ls %ls %d\n", SelectedItemType.c_str(), TargetCodeName.c_str(), this->mySlot->m_pMySlot->m_pSlot->ItemInfo->GetItemData()->CodeName.c_str(),this->mySlot->m_pMySlot->m_pSlot->GetInventorySlotIndex());
            this->mySlot->m_pMySlot->m_pSlot->ClearSlot();
            ClearTexts();
        }
    }
}

undefined CIFItemTranslationWnd::OnCloseWnd(){
    SelectedItemType = L"";
    g_pCGInterface->UnLockMovement();
    this->mySlot->m_pMySlot->m_pSlot->ClearSlot();
    ClearTexts();
    this->m_IRM.GetResObj(17, 1)->SetText(L"0");
    return CIFWnd::OnCloseWnd();
}
void CIFItemTranslationWnd::ClearTexts()
{
    this->m_IRM.GetResObj(17, 1)->SetText(L"0");
    SelectedItemType = L"";
    int numtoDel =  m_TextBox->GetNumberOfItems() +1;
    for(int i =0;i<=numtoDel;i++) {

        m_TextBox->Removeline(0);
    }
    m_TextBox->m_CurrentLines = 0;
}
void CIFItemTranslationWnd::ChineseWeaponIsPuttedSlot()
{
    SelectedItemType = L"";
    int numtoDel =  m_TextBox->GetNumberOfItems() +1;
    for(int i =0;i<=numtoDel;i++) {

        m_TextBox->Removeline(0);
    }
    m_TextBox->m_CurrentLines = 0;

    std::vector<std::n_wstring> TargetItemNames;
    if(this->mySlot->m_pMySlot->m_pSlot->ItemInfo != NULL)
    {
        if(this->mySlot->m_pMySlot->m_pSlot->ItemInfo->GetItemData() != NULL)
        {
            std::wstring MyPoints = Insert(m_Settings->ItemTranslationPrice);
            this->m_IRM.GetResObj(17, 1)->SetText(MyPoints.c_str());

            if(!this->mySlot->m_pMySlot->m_pSlot->ItemInfo->GetItemData()->IsChSword())
            {
                TargetItemNames.push_back(KmtGetText(L"UIIT_KMT_SWORD"));
            }
            if(!this->mySlot->m_pMySlot->m_pSlot->ItemInfo->GetItemData()->IsChBlade())
            {
                TargetItemNames.push_back(KmtGetText(L"UIIT_KMT_BLADE"));
            }
            if(!this->mySlot->m_pMySlot->m_pSlot->ItemInfo->GetItemData()->IsChSpear())
            {
                TargetItemNames.push_back(KmtGetText(L"UIIT_KMT_SPEAR"));
            }
            if(!this->mySlot->m_pMySlot->m_pSlot->ItemInfo->GetItemData()->IsChTBlade())
            {
                TargetItemNames.push_back(KmtGetText(L"UIIT_KMT_GLAVIE"));
            }
            if(!this->mySlot->m_pMySlot->m_pSlot->ItemInfo->GetItemData()->IsChBow())
            {
                TargetItemNames.push_back(KmtGetText(L"UIIT_KMT_BOW"));
            }

            int i = 0;
            for(std::vector<std::n_wstring>::iterator it = TargetItemNames.begin(); it != TargetItemNames.end(); it++)
            {
                i++;
                m_TextBox->sub_64F8A0((*it), i, 0xffffff, 0xffffff, -1, 0, 0);
            }
            m_Scroll->sub_65A5A0(9999);
        }
    }

}
void CIFItemTranslationWnd::EuropeWeaponIsPuttedSlot()
{
    SelectedItemType = L"";
    int numtoDel =  m_TextBox->GetNumberOfItems() +1;
    for(int i =0;i<=numtoDel;i++) {

        m_TextBox->Removeline(0);
    }
    m_TextBox->m_CurrentLines = 0;

    std::vector<std::n_wstring> TargetItemNames;
    if(this->mySlot->m_pMySlot->m_pSlot->ItemInfo != NULL)
    {
        if(this->mySlot->m_pMySlot->m_pSlot->ItemInfo->GetItemData() != NULL)
        {
            std::wstring MyPoints = Insert(m_Settings->ItemTranslationPrice);
            this->m_IRM.GetResObj(17, 1)->SetText(MyPoints.c_str());
            if(!this->mySlot->m_pMySlot->m_pSlot->ItemInfo->GetItemData()->IsEuSword())
            {
                TargetItemNames.push_back(KmtGetText(L"UIIT_KMT_ONEHAND_SWORD"));
            }
            if(!this->mySlot->m_pMySlot->m_pSlot->ItemInfo->GetItemData()->IsEuTSword())
            {
                TargetItemNames.push_back(KmtGetText(L"UIIT_KMT_TWOHAND_SWORD"));
            }
            if(!this->mySlot->m_pMySlot->m_pSlot->ItemInfo->GetItemData()->IsEuAxe())
            {
                TargetItemNames.push_back(KmtGetText(L"UIIT_KMT_AXE"));
            }
            if(!this->mySlot->m_pMySlot->m_pSlot->ItemInfo->GetItemData()->IsEuDarkStaff())
            {
                TargetItemNames.push_back(KmtGetText(L"UIIT_KMT_DARK_STAFF"));
            }
            if(!this->mySlot->m_pMySlot->m_pSlot->ItemInfo->GetItemData()->IsEuTStaff())
            {
                TargetItemNames.push_back(KmtGetText(L"UIIT_KMT_TWOHAND_STAFF"));
            }
            if(!this->mySlot->m_pMySlot->m_pSlot->ItemInfo->GetItemData()->IsEuCrossbow())
            {
                TargetItemNames.push_back(KmtGetText(L"UIIT_KMT_CROSSBOW"));
            }
            if(!this->mySlot->m_pMySlot->m_pSlot->ItemInfo->GetItemData()->IsEuDagger())
            {
                TargetItemNames.push_back(KmtGetText(L"UIIT_KMT_DAGGER"));
            }
            if(!this->mySlot->m_pMySlot->m_pSlot->ItemInfo->GetItemData()->IsEuHarp())
            {
                TargetItemNames.push_back(KmtGetText(L"UIIT_KMT_HARP"));
            }
            if(!this->mySlot->m_pMySlot->m_pSlot->ItemInfo->GetItemData()->IsEuStaff())
            {
                TargetItemNames.push_back(KmtGetText(L"UIIT_KMT_CLERIC_ROD"));
            }

            int i = 0;
            for(std::vector<std::n_wstring>::iterator it = TargetItemNames.begin(); it != TargetItemNames.end(); it++)
            {
                i++;
                m_TextBox->sub_64F8A0((*it), i, 0xffffff, 0xffffff, -1, 0, 0);
            }
            m_Scroll->sub_65A5A0(9999);
        }
    }

}