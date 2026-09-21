//
// Created by YUMBUL on 23.06.2024.
//

#include "IFMacroMenuPickFilter.h"
#include "Game.h"
#include "IFMacroMenu.h"
#include <BSLib/Debug.h>
#include <GInterface.h>
#include <IFSliderCtrl.h>
#include <ICPlayer.h>
#include <TextStringManager.h>
#include <CustomData/CustomCICPlayer.h>
#include <GlobalDataManager.h>
#include <CustomData/CustomSettingManager.h>
#include <CustomInterface/IFPopupList.h>
#include <EntityManagerClient.h>
#include <ICMonster.h>
#include <cmath>
#include <IFPlayerMiniInfo.h>
#include <CharacterDependentData.h>
#include <sstream>
#include <BSLib/multibyte.h>
#include <fstream>
#include <iostream>
#include <IItem.h>
#include "../../../../DevKit_DLL/src/Util.h"

#include "IFMacro.h"


#define VFILTER_MACRO_PICK_SELF_CB 33
#define VFILTER_MACRO_PICK_PET_CB 36
#define OnlyRareEquipts 39
#define DontPickGold 42
#define DontPickAlchemytables 45
#define DontPickAlchemyStones 48
#define DontPickElixirs 51
#define DontPickArrow 54
#define DontPickReturn 57
#define DontPickTrash 60
#define DONT_PICK_VIGOR 28
#define DONT_PICK_HP_MP 65

#define PickDg1 68
#define PickDg2 71
#define PickDg3 74
#define PickDg4 77
#define PickDg5 80
#define PickDg6 83
#define PickDg7 86
#define PickDg8 89
#define PickDg9 92
#define PickDg10 95
#define PickDg11 98
#define PickDg12 101
#define PickDg13 104
#define PickDg14 107
#define PickDg15 110
#define PickDg16 113
#define PickDg17 116
#define PickDg18 119


GFX_IMPLEMENT_DYNCREATE(CIFMacroMenuPickFilter, CIFWnd)
GFX_BEGIN_MESSAGE_MAP(CIFMacroMenuPickFilter, CIFWnd)
                    ONG_COMMAND(300, &CIFMacroMenuPickFilter::OnUnknownStuff)
                    ONG_COMMAND(301, &CIFMacroMenuPickFilter::OnUnknownStuff)
                    ONG_COMMAND(29, &CIFMacroMenuPickFilter::SaveButton)
                    ONG_COMMAND(30, &CIFMacroMenuPickFilter::CancelBtn)
                    ONG_COMMAND(VFILTER_MACRO_PICK_SELF_CB, &CIFMacroMenuPickFilter::SelectPickPetOrSelf)
                    ONG_COMMAND(VFILTER_MACRO_PICK_PET_CB, &CIFMacroMenuPickFilter::SelectPickPetOrSelf)

GFX_END_MESSAGE_MAP()

void CIFMacroMenuPickFilter::CancelBtn(){
    g_pCGInterface->m_IRM.GetResObj<CIFMacroMenu>(1355, 1)->ShowGWnd(false);
}

int CIFMacroMenuPickFilter::Func_4(int a2) {
    int v1 = 0;
    while (a2 != v1 + 100) {
        if (++v1 >= 17)
            return -1;
    }

    return 100;
}


CIFMacroMenuPickFilter::CIFMacroMenuPickFilter(void) {
    m_pTabsSecond = 0;
    PetPickTimerIsRunning = false;
    Macro_PetFilter = false;
}
CIFMacroMenuPickFilter::~CIFMacroMenuPickFilter(void) {
    if (m_pTabsSecond) {
        delete[] m_pTabsSecond;
        m_pTabsSecond = 0;
    }
    BS_DEBUG_LOW(">" __FUNCTION__);
}
void CIFMacroMenuPickFilter::SelectPickPetOrSelf()
{
    int id = GetCurrentEventMsgCtrlId();
    if(id == VFILTER_MACRO_PICK_SELF_CB)
    {
        PickViaCharCheckBox->SetCheckBoxState(true);
        PickViaPetCheckBox->SetCheckBoxState(false);
    }
    else if(id == VFILTER_MACRO_PICK_PET_CB)
    {
        PickViaCharCheckBox->SetCheckBoxState(false);
        PickViaPetCheckBox->SetCheckBoxState(true);
    }
}
bool CIFMacroMenuPickFilter::OnCreate(long ln) {

    // Populate inherited members
    CIFWnd::OnCreate(ln);
    wnd_rect sz;
    m_IRM.LoadFromFile("clientlibrary\\resinfo\\ifmacromenupickupfilter.txt");
    m_IRM.CreateInterfaceSection("Create", this);


    sz.pos.x= 45;
    sz.pos.y = 42;
    m_pTabsSecond = new CIFSelectableArea *[numberOfTabs];

    for (int i = 0; i < numberOfTabs; i++) {

        RECT selectable_area_size;
        selectable_area_size.top = 121;
        selectable_area_size.left = 24;
        // selectable_area_size.left = tabMarginLeft + tabWidth * i;
        selectable_area_size.right = tabWidth + 1;
        selectable_area_size.bottom = tabHeight;

        m_pTabsSecond[i] = (CIFSelectableArea*)CGWnd::CreateInstance(this, GFX_RUNTIME_CLASS(CIFSelectableArea),
                                                                     selectable_area_size, tabFirstId + i, 0);
        m_pTabsSecond[i]->SetFont(this->N00009C2F);


        m_pTabsSecond[i]->sub_64CE30("interface\\option\\opt_video_tab_long01_on.ddj",
                                     "interface\\option\\opt_video_tab_long01_off.ddj", "interface\\option\\opt_video_tab_long01_off.ddj");

        switch (i) {
            case 0:
            {
                m_pTabsSecond[0]->sub_64CC30(1);
                m_pTabsSecond[0]->SetText(KmtGetText(L"UIIT_KMT_DEFAULT"));
            }
                break;
            case 1:
            {
                m_pTabsSecond[1]->MoveGWnd(m_pTabsSecond[0]->GetPos().x + tabWidth + 10, m_pTabsSecond[0]->GetPos().y);
                m_pTabsSecond[1]->SetText(KmtGetText(L"UIIT_KMT_DEGREES"));
                m_pTabsSecond[1]->SetClickable(true);
            }
                break;
        }
        m_pTabsSecond[i]->sub_64CC30(0);

    }
    m_IRM.GetResObj<CIFVerticalScroll>(7, 1)->m_scrollButton->SetClickable(false);
    m_IRM.GetResObj<CIFVerticalScroll>(7, 1)->m_arrowDownButton->SetClickable(false);
    m_IRM.GetResObj<CIFVerticalScroll>(7, 1)->m_arrowUpButton->SetClickable(false);
    m_IRM.GetResObj<CIFVerticalScroll>(7, 1)->SetClickable(false);

    PickViaCharCheckBox = m_IRM.GetResObj<CIFCheckBox>(VFILTER_MACRO_PICK_SELF_CB, 1);
    PickViaCharCheckBox->SetCheckBoxState(false);

    PickViaPetCheckBox = m_IRM.GetResObj<CIFCheckBox>(VFILTER_MACRO_PICK_PET_CB, 1);
    PickViaPetCheckBox->SetCheckBoxState(true);


    OnlyRareEquiptsCB = m_IRM.GetResObj<CIFCheckBox>(OnlyRareEquipts, 1);
    OnlyRareEquiptsCB->SetCheckBoxState(false);


    DontPickGoldCB = m_IRM.GetResObj<CIFCheckBox>(DontPickGold, 1);
    DontPickGoldCB->SetCheckBoxState(false);



    m_IRM.GetResObj(118, 1)->SetText(KmtGetText(L"UIIT_KMT_18_DEGREES"));
    m_IRM.GetResObj(115, 1)->SetText(KmtGetText(L"UIIT_KMT_17_DEGREES"));
    m_IRM.GetResObj(112, 1)->SetText(KmtGetText(L"UIIT_KMT_16_DEGREES"));
    m_IRM.GetResObj(109, 1)->SetText(KmtGetText(L"UIIT_KMT_15_DEGREES"));
    m_IRM.GetResObj(106, 1)->SetText(KmtGetText(L"UIIT_KMT_14_DEGREES"));
    m_IRM.GetResObj(103, 1)->SetText(KmtGetText(L"UIIT_KMT_13_DEGREES"));
    m_IRM.GetResObj(100, 1)->SetText(KmtGetText(L"UIIT_KMT_12_DEGREES"));
    m_IRM.GetResObj(97, 1)->SetText(KmtGetText(L"UIIT_KMT_11_DEGREES"));
    m_IRM.GetResObj(94, 1)->SetText(KmtGetText(L"UIIT_KMT_10_DEGREES"));
    m_IRM.GetResObj(91, 1)->SetText(KmtGetText(L"UIIT_KMT_9_DEGREES"));
    m_IRM.GetResObj(88, 1)->SetText(KmtGetText(L"UIIT_KMT_8_DEGREES"));
    m_IRM.GetResObj(85, 1)->SetText(KmtGetText(L"UIIT_KMT_7_DEGREES"));
    m_IRM.GetResObj(82, 1)->SetText(KmtGetText(L"UIIT_KMT_6_DEGREES"));
    m_IRM.GetResObj(79, 1)->SetText(KmtGetText(L"UIIT_KMT_5_DEGREES"));
    m_IRM.GetResObj(76, 1)->SetText(KmtGetText(L"UIIT_KMT_4_DEGREES"));
    m_IRM.GetResObj(73, 1)->SetText(KmtGetText(L"UIIT_KMT_3_DEGREES"));
    m_IRM.GetResObj(70, 1)->SetText(KmtGetText(L"UIIT_KMT_2_DEGREES"));
    m_IRM.GetResObj(67, 1)->SetText(KmtGetText(L"UIIT_KMT_1_DEGREES"));

    m_IRM.GetResObj(64, 1)->SetText(KmtGetText(L"UIIT_KMT_DONT_PICK_UP_VIGOR_POTIONS"));
    m_IRM.GetResObj(62, 1)->SetText(KmtGetText(L"UIIT_KMT_DONT_PICK_UP_HP_MP_POTIONS"));
    m_IRM.GetResObj(59, 1)->SetText(KmtGetText(L"UIIT_KMT_DONT_PICK_UP_ALCHEMY_MATERIALS"));
    m_IRM.GetResObj(56, 1)->SetText(KmtGetText(L"UIIT_KMT_DONT_PICK_UP_SPECIAL_RETURN_SCROLLS"));
    m_IRM.GetResObj(53, 1)->SetText(KmtGetText(L"UIIT_KMT_DONT_PICK_UP_ARROW_BOLTS"));
    m_IRM.GetResObj(50, 1)->SetText(KmtGetText(L"UIIT_KMT_DONT_PICK_UP_ELIXIRS"));

    m_IRM.GetResObj(47, 1)->SetText(KmtGetText(L"UIIT_KMT_DONT_PICK_UP_ALCHEMY_STONES"));
    m_IRM.GetResObj(44, 1)->SetText(KmtGetText(L"UIIT_KMT_DONT_PICK_UP_ALCHEMY_TABLETS"));
    m_IRM.GetResObj(41, 1)->SetText(KmtGetText(L"UIIT_KMT_DONT_PICK_UP_GOLDS"));

    m_IRM.GetResObj(38, 1)->SetText(KmtGetText(L"UIIT_KMT_PICK_UP_ONLY_RARE_EQUIPS"));

    m_IRM.GetResObj(35, 1)->SetText(KmtGetText(L"UIIT_KMT_PICK_UP_ITEMS_WITH_GRAP_PET"));

    m_IRM.GetResObj(32, 1)->SetText(KmtGetText(L"UIIT_KMT_PICK_UP_ITEMS_WITH_CHARACTER"));

    m_IRM.GetResObj(29, 1)->SetText(KmtGetText(L"UIIT_KMT_SAVE"));
    m_IRM.GetResObj(30, 1)->SetText(KmtGetText(L"UIIT_KMT_CANCEL"));

    DontPickAlchemytablesCB = m_IRM.GetResObj<CIFCheckBox>(DontPickAlchemytables, 1);
    DontPickAlchemytablesCB->SetCheckBoxState(false);

    DontPickAlchemyStonesCB = m_IRM.GetResObj<CIFCheckBox>(DontPickAlchemyStones, 1);
    DontPickAlchemyStonesCB->SetCheckBoxState(false);

    DontPickElixirsCB = m_IRM.GetResObj<CIFCheckBox>(DontPickElixirs, 1);
    DontPickElixirsCB->SetCheckBoxState(false);

    DontPickArrowCB = m_IRM.GetResObj<CIFCheckBox>(DontPickArrow, 1);
    DontPickArrowCB->SetCheckBoxState(false);

    DontPickReturnCB = m_IRM.GetResObj<CIFCheckBox>(DontPickReturn, 1);
    DontPickReturnCB->SetCheckBoxState(false);

    DontPickTrashCB = m_IRM.GetResObj<CIFCheckBox>(DontPickTrash, 1);
    DontPickTrashCB->SetCheckBoxState(false);

    PickDg1CB = m_IRM.GetResObj<CIFCheckBox>(PickDg1, 1);
    PickDg2CB = m_IRM.GetResObj<CIFCheckBox>(PickDg2, 1);
    PickDg3CB = m_IRM.GetResObj<CIFCheckBox>(PickDg3, 1);
    PickDg4CB = m_IRM.GetResObj<CIFCheckBox>(PickDg4, 1);
    PickDg5CB = m_IRM.GetResObj<CIFCheckBox>(PickDg5, 1);
    PickDg6CB = m_IRM.GetResObj<CIFCheckBox>(PickDg6, 1);
    PickDg7CB = m_IRM.GetResObj<CIFCheckBox>(PickDg7, 1);
    PickDg8CB = m_IRM.GetResObj<CIFCheckBox>(PickDg8, 1);
    PickDg9CB = m_IRM.GetResObj<CIFCheckBox>(PickDg9, 1);
    PickDg10CB = m_IRM.GetResObj<CIFCheckBox>(PickDg10, 1);
    PickDg11CB = m_IRM.GetResObj<CIFCheckBox>(PickDg11, 1);
    PickDg12CB = m_IRM.GetResObj<CIFCheckBox>(PickDg12, 1);
    PickDg13CB = m_IRM.GetResObj<CIFCheckBox>(PickDg13, 1);
    PickDg14CB = m_IRM.GetResObj<CIFCheckBox>(PickDg14, 1);
    PickDg15CB = m_IRM.GetResObj<CIFCheckBox>(PickDg15, 1);
    PickDg16CB = m_IRM.GetResObj<CIFCheckBox>(PickDg16, 1);
    PickDg17CB = m_IRM.GetResObj<CIFCheckBox>(PickDg17, 1);
    PickDg18CB = m_IRM.GetResObj<CIFCheckBox>(PickDg18, 1);

    CIFCheckBox* degreeBoxes[] = {
        PickDg1CB, PickDg2CB, PickDg3CB, PickDg4CB, PickDg5CB, PickDg6CB,
        PickDg7CB, PickDg8CB, PickDg9CB, PickDg10CB, PickDg11CB, PickDg12CB,
        PickDg13CB, PickDg14CB, PickDg15CB, PickDg16CB, PickDg17CB, PickDg18CB
    };
    for (int i = 0; i < 18; ++i)
        degreeBoxes[i]->SetCheckBoxState(true);

    DontPickHpMp = m_IRM.GetResObj<CIFCheckBox>(DONT_PICK_HP_MP, 1);
    DontPickHpMp->SetCheckBoxState(false);
    DontPickHpMp->BringToFront();
    DontPickVigor = m_IRM.GetResObj<CIFCheckBox>(DONT_PICK_VIGOR, 1);
    DontPickVigor->SetCheckBoxState(false);
    DontPickVigor->BringToFront();


    return true;
}

void CIFMacroMenuPickFilter::SaveButton(){
    char settingDirectory[MAX_PATH];
    sprintf(settingDirectory, "%s\\Setting", theApp.GetWorkingDir());
    CreateDirectoryA(settingDirectory, NULL);

    char buffer3[0x200];
    sprintf(buffer3, "%s\\Setting\\%ls_PickupFilter.txt", theApp.GetWorkingDir(), g_pMyPlayerObj->GetCharName().c_str());

// Dosyayı yazma modunda aç
    FILE *file3 = fopen(buffer3, "w");
    if (file3 != NULL) {
        // Veri kontrolü ve dosyaya yazma
        fprintf(file3, "Enable Pick Filter: %d\n", Macro_PetFilter);
        fprintf(file3, "Pick Via Char: %d\n", PickViaCharCheckBox->GetCheckedState_MAYBE());
        fprintf(file3, "Pick Via Pet: %d\n", PickViaPetCheckBox->GetCheckedState_MAYBE());
        fprintf(file3, "Only Rare Equipments: %d\n", OnlyRareEquiptsCB->GetCheckedState_MAYBE());
        fprintf(file3, "Don't Pick Gold: %d\n", DontPickGoldCB->GetCheckedState_MAYBE());
        fprintf(file3, "Don't Pick Alchemy Tables: %d\n", DontPickAlchemytablesCB->GetCheckedState_MAYBE());
        fprintf(file3, "Don't Pick Alchemy Stones: %d\n", DontPickAlchemyStonesCB->GetCheckedState_MAYBE());
        fprintf(file3, "Don't Pick Elixirs: %d\n", DontPickElixirsCB->GetCheckedState_MAYBE());
        fprintf(file3, "Don't Pick Arrow: %d\n", DontPickArrowCB->GetCheckedState_MAYBE());
        fprintf(file3, "Don't Pick Return: %d\n", DontPickReturnCB->GetCheckedState_MAYBE());
        fprintf(file3, "Don't Pick Trash: %d\n", DontPickTrashCB->GetCheckedState_MAYBE());

        fprintf(file3, "Pick DG1: %d\n", PickDg1CB->GetCheckedState_MAYBE());
        fprintf(file3, "Pick DG2: %d\n", PickDg2CB->GetCheckedState_MAYBE());
        fprintf(file3, "Pick DG3: %d\n", PickDg3CB->GetCheckedState_MAYBE());
        fprintf(file3, "Pick DG4: %d\n", PickDg4CB->GetCheckedState_MAYBE());
        fprintf(file3, "Pick DG5: %d\n", PickDg5CB->GetCheckedState_MAYBE());
        fprintf(file3, "Pick DG6: %d\n", PickDg6CB->GetCheckedState_MAYBE());
        fprintf(file3, "Pick DG7: %d\n", PickDg7CB->GetCheckedState_MAYBE());
        fprintf(file3, "Pick DG8: %d\n", PickDg8CB->GetCheckedState_MAYBE());
        fprintf(file3, "Pick DG9: %d\n", PickDg9CB->GetCheckedState_MAYBE());
        fprintf(file3, "Pick DG10: %d\n", PickDg10CB->GetCheckedState_MAYBE());
        fprintf(file3, "Pick DG11: %d\n", PickDg11CB->GetCheckedState_MAYBE());
        fprintf(file3, "Pick DG12: %d\n", PickDg12CB->GetCheckedState_MAYBE());
        fprintf(file3, "Pick DG13: %d\n", PickDg13CB->GetCheckedState_MAYBE());
        fprintf(file3, "Pick DG14: %d\n", PickDg14CB->GetCheckedState_MAYBE());
        fprintf(file3, "Pick DG15: %d\n", PickDg15CB->GetCheckedState_MAYBE());
        fprintf(file3, "Pick DG16: %d\n", PickDg16CB->GetCheckedState_MAYBE());
        fprintf(file3, "Pick DG17: %d\n", PickDg17CB->GetCheckedState_MAYBE());
        fprintf(file3, "Pick DG18: %d\n", PickDg18CB->GetCheckedState_MAYBE());

        fprintf(file3, "Don't Pick HP/MP: %d\n", DontPickHpMp->GetCheckedState_MAYBE());
        fprintf(file3, "Don't Pick Vigor: %d\n", DontPickVigor->GetCheckedState_MAYBE());

        // Dosyayı kapat
        fclose(file3);
    }
}

void CIFMacroMenuPickFilter::ActivateTabPage(BYTE page) {
    for (int i = 0; i < numberOfTabs; i++) {
        if (i == page)
            continue;

        m_pTabsSecond[i]->sub_64CC30(0);
        m_pTabsSecond[i]->m_FontTexture.sub_8B4750(2);
    }


    m_pTabsSecond[page]->sub_64CC30(1);
    switch (page)
    {
        case 0://title
        {
            for(int i = 31; i < 66; i++)
            {
                m_IRM.GetResObj(i,1)->ShowGWnd(true);
            }
            for(int i = 66; i < 120; i++)
            {
                m_IRM.GetResObj(i,1)->ShowGWnd(false);
            }
            DontPickVigor->ShowGWnd(true);
            DontPickHpMp->BringToFront();;
            DontPickVigor->BringToFront();
        }
            break;
        case 1:
        {
            for(int i = 31; i < 66; i++)
            {
                m_IRM.GetResObj(i,1)->ShowGWnd(false);
            }
            for(int i = 66; i < 120; i++)
            {
                m_IRM.GetResObj(i,1)->ShowGWnd(true);
            }
            DontPickVigor->ShowGWnd(false);
        }
            break;
    }
}

void CIFMacroMenuPickFilter::OnUpdate(){


}
void CIFMacroMenuPickFilter::OnUnknownStuff(){
    int id = GetCurrentEventMsgCtrlId();
    int i = 0;

    for (int i = 0; i < numberOfTabs; ++i) {
        if (id == m_pTabsSecond[i]->UniqueID()) {
            ActivateTabPage(i);
            return;
        }
    }
}
undefined1 CIFMacroMenuPickFilter::OnCloseWnd(){
    return CIFWnd::OnCloseWnd();
}
#define MEMUTIL_READ_BY_PTR_OFFSET(ptr, offset, type) \
	*(type*)(((uintptr_t)ptr) + offset)

static float MacroPickDistance(const D3DVECTOR& a, const D3DVECTOR& b)
{
    float dx = a.x - b.x;
    float dz = a.z - b.z;
    return sqrt(dx * dx + dz * dz);
}

bool CIFMacroMenuPickFilter::IsDegreeEnabled(int degree) const
{
    CIFCheckBox* boxes[] = {
        PickDg1CB, PickDg2CB, PickDg3CB, PickDg4CB, PickDg5CB, PickDg6CB,
        PickDg7CB, PickDg8CB, PickDg9CB, PickDg10CB, PickDg11CB, PickDg12CB,
        PickDg13CB, PickDg14CB, PickDg15CB, PickDg16CB, PickDg17CB, PickDg18CB
    };
    return degree >= 1 && degree <= 18 && boxes[degree - 1] &&
           boxes[degree - 1]->GetCheckedState_MAYBE();
}

bool CIFMacroMenuPickFilter::ShouldPickItem(const SItemData* data) const
{
    if (!data)
        return false;

    const bool isGold =
        data->m_typeId.getTypeID1() == 3 &&
        data->m_typeId.getTypeID2() == 3 &&
        data->m_typeId.getTypeID3() == 5 &&
        data->m_typeId.getTypeID4() == 0;
    if (isGold)
        return !DontPickGoldCB->GetCheckedState_MAYBE();

    if (data->m_typeId.getTypeID1() == 3 && data->m_typeId.getTypeID2() == 1)
    {
        const int degree = ((data->m_itemClass - 1) / 3) + 1;
        if (!IsDegreeEnabled(degree))
            return false;
        return !OnlyRareEquiptsCB->GetCheckedState_MAYBE() ||
               data->Rarity == 6 || data->Rarity == 2;
    }

    if (data->IsAlchemyTablet())
        return IsDegreeEnabled(data->m_itemClass) &&
               !DontPickAlchemytablesCB->GetCheckedState_MAYBE();

    if (data->IsMagicStone() || data->IsMagicStone2() || data->IsAttrStone())
        return IsDegreeEnabled(data->m_itemClass) &&
               !DontPickAlchemyStonesCB->GetCheckedState_MAYBE();

    if (data->IsElixir())
        return !DontPickElixirsCB->GetCheckedState_MAYBE();
    if (data->IsArrow() || data->IsBolt())
        return !DontPickArrowCB->GetCheckedState_MAYBE();
    if (data->CodeName == L"ITEM_ETC_SCROLL_RETURN_02")
        return !DontPickReturnCB->GetCheckedState_MAYBE();
    if (data->IsAlchemyMaterial())
        return !DontPickTrashCB->GetCheckedState_MAYBE();
    if (data->IsHPPotion() || data->IsMPPotion())
        return !DontPickHpMp->GetCheckedState_MAYBE();
    if (data->IsVIGOR())
        return !DontPickVigor->GetCheckedState_MAYBE();

    return true;
}

bool CIFMacroMenuPickFilter::IsPickupCandidate(CIItem* item, int itemUniqueId, const SItemData* data)
{
    if (!g_pMyPlayerObj || !g_CGlobalDataManager || !item || itemUniqueId == 0 || data == NULL)
        return false;

    if (item->m_bPickAbbilty)
        return false;

    const bool isGold =
        data->m_typeId.getTypeID1() == 3 &&
        data->m_typeId.getTypeID2() == 3 &&
        data->m_typeId.getTypeID3() == 5 &&
        data->m_typeId.getTypeID4() == 0;
    if (!isGold && g_CGlobalDataManager->GetEmptyInventorySlots() == 0)
    {
        bool canMergeStack = false;
        if (data->m_maxStack > 1 && g_pCGInterface &&
            g_pCGInterface->GetMainPopup() &&
            g_pCGInterface->GetMainPopup()->GetInventory())
        {
            CIFInventory* inventory = g_pCGInterface->GetMainPopup()->GetInventory();
            for (std::n_vector<CIFSlotWithHelp*>::iterator slotIt = inventory->pSlots.begin();
                 slotIt != inventory->pSlots.end(); ++slotIt)
            {
                CIFSlotWithHelp* slot = *slotIt;
                if (slot && slot->ItemInfo && slot->ItemInfo->GetItemData() &&
                    slot->ItemInfo->GetItemData()->RefObjectId == data->RefObjectId &&
                    slot->ItemInfo->GetQuantity() < data->m_maxStack)
                {
                    canMergeStack = true;
                    break;
                }
            }
        }
        if (!canMergeStack)
            return false;
    }

    if (item->hasOwner)
    {
        int myOwnerName = *(int*)((DWORD32)g_pMyPlayerObj + 0x2094);
        if (item->SomeCheckForPlayerOwnerName != myOwnerName)
            return false;
    }

    D3DVECTOR myLocation = g_pMyPlayerObj->GetLocation();
    D3DVECTOR itemLocation = item->GetLocation();
    GetSilkPos(g_pMyPlayerObj->GetRegion(), myLocation);
    GetSilkPos(item->GetRegion(), itemLocation);
    if (MacroPickDistance(myLocation, itemLocation) > 800.0f)
        return false;

    unsigned long now = GetTickCount();
    if (m_lastPickupRequestTime.size() > 512)
    {
        for (std::map<int, unsigned long>::iterator old = m_lastPickupRequestTime.begin();
             old != m_lastPickupRequestTime.end();)
        {
            if (now - old->second > 10000)
                m_lastPickupRequestTime.erase(old++);
            else
                ++old;
        }
    }
    std::map<int, unsigned long>::iterator throttleIt = m_lastPickupRequestTime.find(itemUniqueId);
    if (throttleIt != m_lastPickupRequestTime.end() && now - throttleIt->second < 750)
        return false;

    m_lastPickupRequestTime[itemUniqueId] = now;
    return true;
}

void CIFMacroMenuPickFilter::SendPickupRequest(bool viaPet, int petUniqueId, int itemUniqueId)
{
    if (itemUniqueId == 0)
        return;

    if (viaPet)
    {
        if (petUniqueId == 0)
            return;

        CMsgStreamBuffer buf(0x70C5);
        buf << petUniqueId;
        buf << (byte)8;
        buf << itemUniqueId;
        SendMsg(buf);
    }
    else
    {
        CMsgStreamBuffer buf(0x7074);
        buf << (byte)1;
        buf << (byte)2;
        buf << (byte)1;
        buf << itemUniqueId;
        SendMsg(buf);
    }
}

void CIFMacroMenuPickFilter::PickWithPet()
{
    if (!Macro_PetFilter)
    {
        if (PetPickTimerIsRunning && g_pCGInterface)
            g_pCGInterface->KillTimer(START_PICK_PET_TIMER);
        PetPickTimerIsRunning = false;
        return;
    }

    if (!g_pMyPlayerObj || !g_pCGInterface || !g_pGfxEttManager)
        return;

    if(!PetPickTimerIsRunning)
    {
        PetPickTimerIsRunning = true;
        g_pCGInterface->StartTimer(START_PICK_PET_TIMER, 500);
    }

    bool viaPet = PickViaPetCheckBox && PickViaPetCheckBox->GetCheckedState_MAYBE();
    bool viaCharacter = PickViaCharCheckBox && PickViaCharCheckBox->GetCheckedState_MAYBE();
    if (!viaPet && !viaCharacter)
    {
        viaCharacter = true;
        PickViaCharCheckBox->SetCheckBoxState(true);
    }

    int pickupPetId = 0;
    if (viaPet && g_pMyPlayerObj->CCOSDataMgr)
    {
        for(std::map<int, CCOSDataMgr::CosData*>::iterator petIt =
                g_pMyPlayerObj->CCOSDataMgr->CosList.begin();
            petIt != g_pMyPlayerObj->CCOSDataMgr->CosList.end(); ++petIt)
        {
            CICharactor* pet = GetCharacterObjectByID_MAYBE(petIt->first);
            if (!pet || pet->CHARACTER_STATUS == Dead || !pet->GetCommonData())
                continue;

            const CCharacterData* characterData =
                g_CGlobalDataManager->GetCharacter(pet->GetCommonData()->RefObjectId);
            if (characterData && characterData->IsGrapPet())
            {
                pickupPetId = petIt->first;
                break;
            }
        }
        if (pickupPetId == 0)
            return;
    }

    if (!viaPet &&
        (g_pMyPlayerObj->CHARACTER_STATUS == Dead ||
         g_pMyPlayerObj->CHARACTER_STATUS == Stall ||
         g_pMyPlayerObj->CHARACTER_STATUS == SkillCast ||
         g_pMyPlayerObj->Dead0Stay1Walking2Sit0SkillCast0emotion33Stall12817isridingpet == 17))
        return;

    for (std::map<int, CIObject*>::iterator objectIt = g_pGfxEttManager->entities.begin();
         objectIt != g_pGfxEttManager->entities.end(); ++objectIt)
    {
        CIObject* object = objectIt->second;
        if (!object || !object->IsSame(GFX_RUNTIME_CLASS(CIItem)))
            continue;

        CIItem* item = (CIItem*)object;
        const SCommonData* commonData = item->GetCommonData();
        if (!commonData || commonData->RefObjectId <= 0)
            continue;

        const int itemUniqueId = item->GetUniqueId();
        const SItemData* itemData =
            &g_CGlobalDataManager->GetItemData(commonData->RefObjectId);
        if (!ShouldPickItem(itemData) ||
            !IsPickupCandidate(item, itemUniqueId, itemData))
            continue;

        SendPickupRequest(viaPet, pickupPetId, itemUniqueId);
        return; // Global rate limit: at most one pickup request per timer tick.
    }
    return;

    if(PickViaPetCheckBox->GetCheckedState_MAYBE())
    {
        int FoundedCosUniqueID = 0;
        for(std::map<int, CCOSDataMgr::CosData*>::iterator it = g_pMyPlayerObj->CCOSDataMgr->CosList.begin();
            it != g_pMyPlayerObj->CCOSDataMgr->CosList.end(); ++it)
        {
            CICharactor *pUser = GetCharacterObjectByID_MAYBE(it->first);
            if (pUser != NULL) {
                static const CCharacterData *data = NULL;
                data = g_CGlobalDataManager->GetCharacter(pUser->GetCommonData()->RefObjectId);
                if(data)
                {
                    if(data->IsGrapPet())
                    {
                        FoundedCosUniqueID = it->first;
                    }
                }
            }
        }

        if(FoundedCosUniqueID != 0)
        {

            for (std::map<int, CIObject *>::iterator it = g_pGfxEttManager->entities.begin();
                 it != g_pGfxEttManager->entities.end(); ++it) {
                //TODO REPLACE STRING COMPARISON WITH isSame Function
                if (it->second->IsSame(GFX_RUNTIME_CLASS(CIItem))) {
                    CIItem* item = (CIItem*) it->second;
                    if(item)
                    {
                        int ItemUniqueID = *(int *) ((int) (it->second) + 0xF8); //item uniqueid
                        int refObjID = *(int *) ((int) (it->second) + 0x21C); //item refobjid
                        const SItemData *data = &g_CGlobalDataManager->GetItemData(refObjID);
                        if(!ItemUniqueID)
                            continue;
                        if(!data || data == NULL)
                            continue;

                        if(!IsPickupCandidate(item, ItemUniqueID, data))
                            continue;
                        /// PICK GOLD
                        if(data->m_typeId.getTypeID1() == 3 && data->m_typeId.getTypeID2() == 3 && data->m_typeId.getTypeID3() == 5 && data->m_typeId.getTypeID4() == 0)
                        {
                            if(!DontPickGoldCB->GetCheckedState_MAYBE())
                            {
                                CMsgStreamBuffer buf(0x70C5);
                                buf << FoundedCosUniqueID;
                                buf << (byte) 8;
                                buf << ItemUniqueID;
                                SendMsg(buf);
                            }
                        }
                        else if(data->m_typeId.getTypeID1() == 3 && data->m_typeId.getTypeID2() == 1) /// PICK NORMAL ITEM
                        {
                            if(data->m_itemClass >= 1 && data->m_itemClass <= 3) /// 1DG
                            {
                                if(PickDg1CB->GetCheckedState_MAYBE())
                                {
                                    if(OnlyRareEquiptsCB->GetCheckedState_MAYBE())
                                    {
                                        if(data->Rarity == 6 || data->Rarity == 2)
                                        {
                                            CMsgStreamBuffer buf(0x70C5);
                                            buf << FoundedCosUniqueID;
                                            buf << (byte) 8;
                                            buf << ItemUniqueID;
                                            SendMsg(buf);
                                        }
                                    }
                                    else
                                    {
                                        CMsgStreamBuffer buf(0x70C5);
                                        buf << FoundedCosUniqueID;
                                        buf << (byte) 8;
                                        buf << ItemUniqueID;
                                        SendMsg(buf);
                                    }
                                }
                            }
                            if(data->m_itemClass >= 4 && data->m_itemClass <= 6) /// 2DG
                            {
                                if(PickDg2CB->GetCheckedState_MAYBE())
                                {
                                    if(OnlyRareEquiptsCB->GetCheckedState_MAYBE())
                                    {
                                        if(data->Rarity == 6 || data->Rarity == 2)
                                        {
                                            CMsgStreamBuffer buf(0x70C5);
                                            buf << FoundedCosUniqueID;
                                            buf << (byte) 8;
                                            buf << ItemUniqueID;
                                            SendMsg(buf);
                                        }
                                    }
                                    else
                                    {
                                        CMsgStreamBuffer buf(0x70C5);
                                        buf << FoundedCosUniqueID;
                                        buf << (byte) 8;
                                        buf << ItemUniqueID;
                                        SendMsg(buf);
                                    }
                                }
                            }
                            if(data->m_itemClass >= 7 && data->m_itemClass <= 9) /// 3DG
                            {
                                if(PickDg3CB->GetCheckedState_MAYBE())
                                {
                                    if(OnlyRareEquiptsCB->GetCheckedState_MAYBE())
                                    {
                                        if(data->Rarity == 6 || data->Rarity == 2)
                                        {
                                            CMsgStreamBuffer buf(0x70C5);
                                            buf << FoundedCosUniqueID;
                                            buf << (byte) 8;
                                            buf << ItemUniqueID;
                                            SendMsg(buf);
                                        }
                                    }
                                    else
                                    {
                                        CMsgStreamBuffer buf(0x70C5);
                                        buf << FoundedCosUniqueID;
                                        buf << (byte) 8;
                                        buf << ItemUniqueID;
                                        SendMsg(buf);
                                    }
                                }
                            }
                            if(data->m_itemClass >= 10 && data->m_itemClass <= 12) /// 4DG
                            {
                                if(PickDg4CB->GetCheckedState_MAYBE())
                                {
                                    if(OnlyRareEquiptsCB->GetCheckedState_MAYBE())
                                    {
                                        if(data->Rarity == 6 || data->Rarity == 2)
                                        {
                                            CMsgStreamBuffer buf(0x70C5);
                                            buf << FoundedCosUniqueID;
                                            buf << (byte) 8;
                                            buf << ItemUniqueID;
                                            SendMsg(buf);
                                        }
                                    }
                                    else
                                    {
                                        CMsgStreamBuffer buf(0x70C5);
                                        buf << FoundedCosUniqueID;
                                        buf << (byte) 8;
                                        buf << ItemUniqueID;
                                        SendMsg(buf);
                                    }
                                }
                            }
                            if(data->m_itemClass >= 13 && data->m_itemClass <= 15) /// 5DG
                            {
                                if(PickDg5CB->GetCheckedState_MAYBE())
                                {
                                    if(OnlyRareEquiptsCB->GetCheckedState_MAYBE())
                                    {
                                        if(data->Rarity == 6 || data->Rarity == 2)
                                        {
                                            CMsgStreamBuffer buf(0x70C5);
                                            buf << FoundedCosUniqueID;
                                            buf << (byte) 8;
                                            buf << ItemUniqueID;
                                            SendMsg(buf);
                                        }
                                    }
                                    else
                                    {
                                        CMsgStreamBuffer buf(0x70C5);
                                        buf << FoundedCosUniqueID;
                                        buf << (byte) 8;
                                        buf << ItemUniqueID;
                                        SendMsg(buf);
                                    }
                                }
                            }

                            if(data->m_itemClass >= 16 && data->m_itemClass <= 18) /// 6DG
                            {
                                if(PickDg6CB->GetCheckedState_MAYBE())
                                {
                                    if(OnlyRareEquiptsCB->GetCheckedState_MAYBE())
                                    {
                                        if(data->Rarity == 6 || data->Rarity == 2)
                                        {
                                            CMsgStreamBuffer buf(0x70C5);
                                            buf << FoundedCosUniqueID;
                                            buf << (byte) 8;
                                            buf << ItemUniqueID;
                                            SendMsg(buf);
                                        }
                                    }
                                    else
                                    {
                                        CMsgStreamBuffer buf(0x70C5);
                                        buf << FoundedCosUniqueID;
                                        buf << (byte) 8;
                                        buf << ItemUniqueID;
                                        SendMsg(buf);
                                    }
                                }
                            }

                            if(data->m_itemClass >= 19 && data->m_itemClass <= 21) /// 7DG
                            {
                                if(PickDg7CB->GetCheckedState_MAYBE())
                                {
                                    if(OnlyRareEquiptsCB->GetCheckedState_MAYBE())
                                    {
                                        if(data->Rarity == 6 || data->Rarity == 2)
                                        {
                                            CMsgStreamBuffer buf(0x70C5);
                                            buf << FoundedCosUniqueID;
                                            buf << (byte) 8;
                                            buf << ItemUniqueID;
                                            SendMsg(buf);
                                        }
                                    }
                                    else
                                    {
                                        CMsgStreamBuffer buf(0x70C5);
                                        buf << FoundedCosUniqueID;
                                        buf << (byte) 8;
                                        buf << ItemUniqueID;
                                        SendMsg(buf);
                                    }
                                }
                            }
                            if(data->m_itemClass >= 22 && data->m_itemClass <= 24) /// 8DG
                            {
                                if(PickDg8CB->GetCheckedState_MAYBE())
                                {
                                    if(OnlyRareEquiptsCB->GetCheckedState_MAYBE())
                                    {
                                        if(data->Rarity == 6 || data->Rarity == 2)
                                        {
                                            CMsgStreamBuffer buf(0x70C5);
                                            buf << FoundedCosUniqueID;
                                            buf << (byte) 8;
                                            buf << ItemUniqueID;
                                            SendMsg(buf);
                                        }
                                    }
                                    else
                                    {
                                        CMsgStreamBuffer buf(0x70C5);
                                        buf << FoundedCosUniqueID;
                                        buf << (byte) 8;
                                        buf << ItemUniqueID;
                                        SendMsg(buf);
                                    }
                                }
                            }
                            if(data->m_itemClass >= 25 && data->m_itemClass <= 27) /// 9DG
                            {
                                if(PickDg9CB->GetCheckedState_MAYBE())
                                {
                                    if(OnlyRareEquiptsCB->GetCheckedState_MAYBE())
                                    {
                                        if(data->Rarity == 6 || data->Rarity == 2)
                                        {
                                            CMsgStreamBuffer buf(0x70C5);
                                            buf << FoundedCosUniqueID;
                                            buf << (byte) 8;
                                            buf << ItemUniqueID;
                                            SendMsg(buf);
                                        }
                                    }
                                    else
                                    {
                                        CMsgStreamBuffer buf(0x70C5);
                                        buf << FoundedCosUniqueID;
                                        buf << (byte) 8;
                                        buf << ItemUniqueID;
                                        SendMsg(buf);
                                    }
                                }
                            }
                            if(data->m_itemClass >= 28 && data->m_itemClass <= 30) /// 10DG
                            {
                                if(PickDg10CB->GetCheckedState_MAYBE())
                                {
                                    if(OnlyRareEquiptsCB->GetCheckedState_MAYBE())
                                    {
                                        if(data->Rarity == 6 || data->Rarity == 2)
                                        {
                                            CMsgStreamBuffer buf(0x70C5);
                                            buf << FoundedCosUniqueID;
                                            buf << (byte) 8;
                                            buf << ItemUniqueID;
                                            SendMsg(buf);
                                        }
                                    }
                                    else
                                    {
                                        CMsgStreamBuffer buf(0x70C5);
                                        buf << FoundedCosUniqueID;
                                        buf << (byte) 8;
                                        buf << ItemUniqueID;
                                        SendMsg(buf);
                                    }
                                }
                            }
                            if(data->m_itemClass >= 31 && data->m_itemClass <= 33) /// 11DG
                            {
                                if(PickDg11CB->GetCheckedState_MAYBE())
                                {
                                    if(OnlyRareEquiptsCB->GetCheckedState_MAYBE())
                                    {
                                        if(data->Rarity == 6 || data->Rarity == 2)
                                        {
                                            CMsgStreamBuffer buf(0x70C5);
                                            buf << FoundedCosUniqueID;
                                            buf << (byte) 8;
                                            buf << ItemUniqueID;
                                            SendMsg(buf);
                                        }
                                    }
                                    else
                                    {
                                        CMsgStreamBuffer buf(0x70C5);
                                        buf << FoundedCosUniqueID;
                                        buf << (byte) 8;
                                        buf << ItemUniqueID;
                                        SendMsg(buf);
                                    }
                                }
                            }
                            if(data->m_itemClass >= 34 && data->m_itemClass <= 36) /// 12DG
                            {
                                if(PickDg12CB->GetCheckedState_MAYBE())
                                {
                                    if(OnlyRareEquiptsCB->GetCheckedState_MAYBE())
                                    {
                                        if(data->Rarity == 6 || data->Rarity == 2)
                                        {
                                            CMsgStreamBuffer buf(0x70C5);
                                            buf << FoundedCosUniqueID;
                                            buf << (byte) 8;
                                            buf << ItemUniqueID;
                                            SendMsg(buf);
                                        }
                                    }
                                    else
                                    {
                                        CMsgStreamBuffer buf(0x70C5);
                                        buf << FoundedCosUniqueID;
                                        buf << (byte) 8;
                                        buf << ItemUniqueID;
                                        SendMsg(buf);
                                    }
                                }
                            }
                            if(data->m_itemClass >= 37 && data->m_itemClass <= 39) /// 13DG
                            {
                                if(PickDg13CB->GetCheckedState_MAYBE())
                                {
                                    if(OnlyRareEquiptsCB->GetCheckedState_MAYBE())
                                    {
                                        if(data->Rarity == 6 || data->Rarity == 2)
                                        {
                                            CMsgStreamBuffer buf(0x70C5);
                                            buf << FoundedCosUniqueID;
                                            buf << (byte) 8;
                                            buf << ItemUniqueID;
                                            SendMsg(buf);
                                        }
                                    }
                                    else
                                    {
                                        CMsgStreamBuffer buf(0x70C5);
                                        buf << FoundedCosUniqueID;
                                        buf << (byte) 8;
                                        buf << ItemUniqueID;
                                        SendMsg(buf);
                                    }
                                }
                            }
                            if(data->m_itemClass >= 40 && data->m_itemClass <= 42) /// 14DG
                            {
                                if(PickDg14CB->GetCheckedState_MAYBE())
                                {
                                    if(OnlyRareEquiptsCB->GetCheckedState_MAYBE())
                                    {
                                        if(data->Rarity == 6 || data->Rarity == 2)
                                        {
                                            CMsgStreamBuffer buf(0x70C5);
                                            buf << FoundedCosUniqueID;
                                            buf << (byte) 8;
                                            buf << ItemUniqueID;
                                            SendMsg(buf);
                                        }
                                    }
                                    else
                                    {
                                        CMsgStreamBuffer buf(0x70C5);
                                        buf << FoundedCosUniqueID;
                                        buf << (byte) 8;
                                        buf << ItemUniqueID;
                                        SendMsg(buf);
                                    }
                                }
                            }
                            if(data->m_itemClass >= 43 && data->m_itemClass <= 45) /// 15DG
                            {
                                if(PickDg15CB->GetCheckedState_MAYBE())
                                {
                                    if(OnlyRareEquiptsCB->GetCheckedState_MAYBE())
                                    {
                                        if(data->Rarity == 6 || data->Rarity == 2)
                                        {
                                            CMsgStreamBuffer buf(0x70C5);
                                            buf << FoundedCosUniqueID;
                                            buf << (byte) 8;
                                            buf << ItemUniqueID;
                                            SendMsg(buf);
                                        }
                                    }
                                    else
                                    {
                                        CMsgStreamBuffer buf(0x70C5);
                                        buf << FoundedCosUniqueID;
                                        buf << (byte) 8;
                                        buf << ItemUniqueID;
                                        SendMsg(buf);
                                    }
                                }
                            }

                            if(data->m_itemClass >= 46 && data->m_itemClass <= 48) /// 16DG
                            {
                                if(PickDg16CB->GetCheckedState_MAYBE())
                                {
                                    if(OnlyRareEquiptsCB->GetCheckedState_MAYBE())
                                    {
                                        if(data->Rarity == 6 || data->Rarity == 2)
                                        {
                                            CMsgStreamBuffer buf(0x70C5);
                                            buf << FoundedCosUniqueID;
                                            buf << (byte) 8;
                                            buf << ItemUniqueID;
                                            SendMsg(buf);
                                        }
                                    }
                                    else
                                    {
                                        CMsgStreamBuffer buf(0x70C5);
                                        buf << FoundedCosUniqueID;
                                        buf << (byte) 8;
                                        buf << ItemUniqueID;
                                        SendMsg(buf);
                                    }
                                }
                            }

                            if(data->m_itemClass >= 49 && data->m_itemClass <= 51) /// 17DG
                            {
                                if(PickDg17CB->GetCheckedState_MAYBE())
                                {
                                    if(OnlyRareEquiptsCB->GetCheckedState_MAYBE())
                                    {
                                        if(data->Rarity == 6 || data->Rarity == 2)
                                        {
                                            CMsgStreamBuffer buf(0x70C5);
                                            buf << FoundedCosUniqueID;
                                            buf << (byte) 8;
                                            buf << ItemUniqueID;
                                            SendMsg(buf);
                                        }
                                    }
                                    else
                                    {
                                        CMsgStreamBuffer buf(0x70C5);
                                        buf << FoundedCosUniqueID;
                                        buf << (byte) 8;
                                        buf << ItemUniqueID;
                                        SendMsg(buf);
                                    }
                                }
                            }

                            if(data->m_itemClass >= 52 && data->m_itemClass <= 54) /// 18DG
                            {
                                if(PickDg18CB->GetCheckedState_MAYBE())
                                {
                                    if(OnlyRareEquiptsCB->GetCheckedState_MAYBE())
                                    {
                                        if(data->Rarity == 6 || data->Rarity == 2)
                                        {
                                            CMsgStreamBuffer buf(0x70C5);
                                            buf << FoundedCosUniqueID;
                                            buf << (byte) 8;
                                            buf << ItemUniqueID;
                                            SendMsg(buf);
                                        }
                                    }
                                    else
                                    {
                                        CMsgStreamBuffer buf(0x70C5);
                                        buf << FoundedCosUniqueID;
                                        buf << (byte) 8;
                                        buf << ItemUniqueID;
                                        SendMsg(buf);
                                    }
                                }
                            }
                        }
                        else if(data->IsAlchemyTablet()) /// PICK POTS ?
                        {
                            if(data->m_itemClass == 1)
                            {
                                if(PickDg1CB->GetCheckedState_MAYBE())
                                {
                                    if(!DontPickAlchemytablesCB->GetCheckedState_MAYBE())
                                    {
                                        CMsgStreamBuffer buf(0x70C5);
                                        buf << FoundedCosUniqueID;
                                        buf << (byte) 8;
                                        buf << ItemUniqueID;
                                        SendMsg(buf);
                                    }
                                }
                            }
                            if(data->m_itemClass == 2)
                            {
                                if(PickDg2CB->GetCheckedState_MAYBE())
                                {
                                    if(!DontPickAlchemytablesCB->GetCheckedState_MAYBE())
                                    {
                                        CMsgStreamBuffer buf(0x70C5);
                                        buf << FoundedCosUniqueID;
                                        buf << (byte) 8;
                                        buf << ItemUniqueID;
                                        SendMsg(buf);
                                    }
                                }
                            }
                            if(data->m_itemClass == 3)
                            {
                                if(PickDg3CB->GetCheckedState_MAYBE())
                                {
                                    if(!DontPickAlchemytablesCB->GetCheckedState_MAYBE())
                                    {
                                        CMsgStreamBuffer buf(0x70C5);
                                        buf << FoundedCosUniqueID;
                                        buf << (byte) 8;
                                        buf << ItemUniqueID;
                                        SendMsg(buf);
                                    }
                                }
                            }
                            if(data->m_itemClass == 4)
                            {
                                if(PickDg4CB->GetCheckedState_MAYBE())
                                {
                                    if(!DontPickAlchemytablesCB->GetCheckedState_MAYBE())
                                    {
                                        CMsgStreamBuffer buf(0x70C5);
                                        buf << FoundedCosUniqueID;
                                        buf << (byte) 8;
                                        buf << ItemUniqueID;
                                        SendMsg(buf);
                                    }
                                }
                            }
                            if(data->m_itemClass == 5)
                            {
                                if(PickDg5CB->GetCheckedState_MAYBE())
                                {
                                    if(!DontPickAlchemytablesCB->GetCheckedState_MAYBE())
                                    {
                                        CMsgStreamBuffer buf(0x70C5);
                                        buf << FoundedCosUniqueID;
                                        buf << (byte) 8;
                                        buf << ItemUniqueID;
                                        SendMsg(buf);
                                    }
                                }
                            }
                            if(data->m_itemClass == 6)
                            {
                                if(PickDg6CB->GetCheckedState_MAYBE())
                                {
                                    if(!DontPickAlchemytablesCB->GetCheckedState_MAYBE())
                                    {
                                        CMsgStreamBuffer buf(0x70C5);
                                        buf << FoundedCosUniqueID;
                                        buf << (byte) 8;
                                        buf << ItemUniqueID;
                                        SendMsg(buf);
                                    }
                                }
                            }
                            if(data->m_itemClass == 7)
                            {
                                if(PickDg7CB->GetCheckedState_MAYBE())
                                {
                                    if(!DontPickAlchemytablesCB->GetCheckedState_MAYBE())
                                    {
                                        CMsgStreamBuffer buf(0x70C5);
                                        buf << FoundedCosUniqueID;
                                        buf << (byte) 8;
                                        buf << ItemUniqueID;
                                        SendMsg(buf);
                                    }
                                }
                            }
                            if(data->m_itemClass == 8)
                            {
                                if(PickDg8CB->GetCheckedState_MAYBE())
                                {
                                    if(!DontPickAlchemytablesCB->GetCheckedState_MAYBE())
                                    {
                                        CMsgStreamBuffer buf(0x70C5);
                                        buf << FoundedCosUniqueID;
                                        buf << (byte) 8;
                                        buf << ItemUniqueID;
                                        SendMsg(buf);
                                    }
                                }
                            }
                            if(data->m_itemClass == 9)
                            {
                                if(PickDg9CB->GetCheckedState_MAYBE())
                                {
                                    if(!DontPickAlchemytablesCB->GetCheckedState_MAYBE())
                                    {
                                        CMsgStreamBuffer buf(0x70C5);
                                        buf << FoundedCosUniqueID;
                                        buf << (byte) 8;
                                        buf << ItemUniqueID;
                                        SendMsg(buf);
                                    }
                                }
                            }
                            if(data->m_itemClass == 10)
                            {
                                if(PickDg10CB->GetCheckedState_MAYBE())
                                {
                                    if(!DontPickAlchemytablesCB->GetCheckedState_MAYBE())
                                    {
                                        CMsgStreamBuffer buf(0x70C5);
                                        buf << FoundedCosUniqueID;
                                        buf << (byte) 8;
                                        buf << ItemUniqueID;
                                        SendMsg(buf);
                                    }
                                }
                            }
                            if(data->m_itemClass == 11)
                            {
                                if(PickDg11CB->GetCheckedState_MAYBE())
                                {
                                    if(!DontPickAlchemytablesCB->GetCheckedState_MAYBE())
                                    {
                                        CMsgStreamBuffer buf(0x70C5);
                                        buf << FoundedCosUniqueID;
                                        buf << (byte) 8;
                                        buf << ItemUniqueID;
                                        SendMsg(buf);
                                    }
                                }
                            }
                            if(data->m_itemClass == 12)
                            {
                                if(PickDg12CB->GetCheckedState_MAYBE())
                                {
                                    if(!DontPickAlchemytablesCB->GetCheckedState_MAYBE())
                                    {
                                        CMsgStreamBuffer buf(0x70C5);
                                        buf << FoundedCosUniqueID;
                                        buf << (byte) 8;
                                        buf << ItemUniqueID;
                                        SendMsg(buf);
                                    }
                                }
                            }
                            if(data->m_itemClass == 13)
                            {
                                if(PickDg13CB->GetCheckedState_MAYBE())
                                {
                                    if(!DontPickAlchemytablesCB->GetCheckedState_MAYBE())
                                    {
                                        CMsgStreamBuffer buf(0x70C5);
                                        buf << FoundedCosUniqueID;
                                        buf << (byte) 8;
                                        buf << ItemUniqueID;
                                        SendMsg(buf);
                                    }
                                }
                            }
                            if(data->m_itemClass == 14)
                            {
                                if(PickDg14CB->GetCheckedState_MAYBE())
                                {
                                    if(!DontPickAlchemytablesCB->GetCheckedState_MAYBE())
                                    {
                                        CMsgStreamBuffer buf(0x70C5);
                                        buf << FoundedCosUniqueID;
                                        buf << (byte) 8;
                                        buf << ItemUniqueID;
                                        SendMsg(buf);
                                    }
                                }
                            }
                            if(data->m_itemClass == 15)
                            {
                                if(PickDg15CB->GetCheckedState_MAYBE())
                                {
                                    if(!DontPickAlchemytablesCB->GetCheckedState_MAYBE())
                                    {
                                        CMsgStreamBuffer buf(0x70C5);
                                        buf << FoundedCosUniqueID;
                                        buf << (byte) 8;
                                        buf << ItemUniqueID;
                                        SendMsg(buf);
                                    }
                                }
                            }
                            if(data->m_itemClass == 16)
                            {
                                if(PickDg16CB->GetCheckedState_MAYBE())
                                {
                                    if(!DontPickAlchemytablesCB->GetCheckedState_MAYBE())
                                    {
                                        CMsgStreamBuffer buf(0x70C5);
                                        buf << FoundedCosUniqueID;
                                        buf << (byte) 8;
                                        buf << ItemUniqueID;
                                        SendMsg(buf);
                                    }
                                }
                            }
                            if(data->m_itemClass == 17)
                            {
                                if(PickDg17CB->GetCheckedState_MAYBE())
                                {
                                    if(!DontPickAlchemytablesCB->GetCheckedState_MAYBE())
                                    {
                                        CMsgStreamBuffer buf(0x70C5);
                                        buf << FoundedCosUniqueID;
                                        buf << (byte) 8;
                                        buf << ItemUniqueID;
                                        SendMsg(buf);
                                    }
                                }
                            }
                            if(data->m_itemClass == 18)
                            {
                                if(PickDg18CB->GetCheckedState_MAYBE())
                                {
                                    if(!DontPickAlchemytablesCB->GetCheckedState_MAYBE())
                                    {
                                        CMsgStreamBuffer buf(0x70C5);
                                        buf << FoundedCosUniqueID;
                                        buf << (byte) 8;
                                        buf << ItemUniqueID;
                                        SendMsg(buf);
                                    }
                                }
                            }
                        }
                        else if(data->IsMagicStone() || data->IsMagicStone2() || data->IsAttrStone())
                        {
                            if(data->m_itemClass >= 1 && data->m_itemClass <= 18)
                            {
                                bool pickItem = false;
                                switch (data->m_itemClass)
                                {
                                    case 1: pickItem = PickDg1CB->GetCheckedState_MAYBE(); break;
                                    case 2: pickItem = PickDg2CB->GetCheckedState_MAYBE(); break;
                                    case 3: pickItem = PickDg3CB->GetCheckedState_MAYBE(); break;
                                    case 4: pickItem = PickDg4CB->GetCheckedState_MAYBE(); break;
                                    case 5: pickItem = PickDg5CB->GetCheckedState_MAYBE(); break;
                                    case 6: pickItem = PickDg6CB->GetCheckedState_MAYBE(); break;
                                    case 7: pickItem = PickDg7CB->GetCheckedState_MAYBE(); break;
                                    case 8: pickItem = PickDg8CB->GetCheckedState_MAYBE(); break;
                                    case 9: pickItem = PickDg9CB->GetCheckedState_MAYBE(); break;
                                    case 10: pickItem = PickDg10CB->GetCheckedState_MAYBE(); break;
                                    case 11: pickItem = PickDg11CB->GetCheckedState_MAYBE(); break;
                                    case 12: pickItem = PickDg12CB->GetCheckedState_MAYBE(); break;
                                    case 13: pickItem = PickDg13CB->GetCheckedState_MAYBE(); break;
                                    case 14: pickItem = PickDg14CB->GetCheckedState_MAYBE(); break;
                                    case 15: pickItem = PickDg15CB->GetCheckedState_MAYBE(); break;
                                    case 16: pickItem = PickDg16CB->GetCheckedState_MAYBE(); break;
                                    case 17: pickItem = PickDg17CB->GetCheckedState_MAYBE(); break;
                                    case 18: pickItem = PickDg18CB->GetCheckedState_MAYBE(); break;
                                    default: break;
                                }

                                if(pickItem && !DontPickAlchemyStonesCB->GetCheckedState_MAYBE())
                                {
                                    CMsgStreamBuffer buf(0x70C5);
                                    buf << FoundedCosUniqueID;
                                    buf << (byte) 8;
                                    buf << ItemUniqueID;
                                    SendMsg(buf);
                                }
                            }
                        }
                        else if(data->IsElixir())
                        {
                            if(!DontPickElixirsCB->GetCheckedState_MAYBE())
                            {
                                CMsgStreamBuffer buf(0x70C5);
                                buf << FoundedCosUniqueID;
                                buf << (byte) 8;
                                buf << ItemUniqueID;
                                SendMsg(buf);
                            }
                        }
                        else if(data->IsArrow() || data->IsBolt())
                        {
                            if(!DontPickArrowCB->GetCheckedState_MAYBE())
                            {
                                CMsgStreamBuffer buf(0x70C5);
                                buf << FoundedCosUniqueID;
                                buf << (byte) 8;
                                buf << ItemUniqueID;
                                SendMsg(buf);
                            }
                        }
                        else if(data->CodeName == L"ITEM_ETC_SCROLL_RETURN_02")
                        {
                            if(!DontPickReturnCB->GetCheckedState_MAYBE())
                            {
                                CMsgStreamBuffer buf(0x70C5);
                                buf << FoundedCosUniqueID;
                                buf << (byte) 8;
                                buf << ItemUniqueID;
                                SendMsg(buf);
                            }
                        }
                        else if(data->IsAlchemyMaterial())
                        {
                            if(!DontPickTrashCB->GetCheckedState_MAYBE())
                            {
                                CMsgStreamBuffer buf(0x70C5);
                                buf << FoundedCosUniqueID;
                                buf << (byte) 8;
                                buf << ItemUniqueID;
                                SendMsg(buf);
                            }
                        }
                        else if(data->IsHPPotion() || data->IsMPPotion())
                        {
                            if(!DontPickHpMp->GetCheckedState_MAYBE())
                            {
                                CMsgStreamBuffer buf(0x70C5);
                                buf << FoundedCosUniqueID;
                                buf << (byte) 8;
                                buf << ItemUniqueID;
                                SendMsg(buf);
                            }
                        }
                        else if(data->IsVIGOR())
                        {
                            if(!DontPickVigor->GetCheckedState_MAYBE())
                            {
                                CMsgStreamBuffer buf(0x70C5);
                                buf << FoundedCosUniqueID;
                                buf << (byte) 8;
                                buf << ItemUniqueID;
                                SendMsg(buf);
                            }
                        }
                        else
                        {
                            CMsgStreamBuffer buf(0x70C5);
                            buf << FoundedCosUniqueID;
                            buf << (byte) 8;
                            buf << ItemUniqueID;
                            SendMsg(buf);
                        }
                    }
                }
                //  if(g_pCGInterface->m_IRM.GetResObj<CIFMacroMenu>(MacroMenuID, 1)->PickupFilterSlot->
            }

        }
    }
    else if(PickViaCharCheckBox->GetCheckedState_MAYBE())
    {
        if (g_pMyPlayerObj->CHARACTER_STATUS == Dead ||g_pMyPlayerObj->CHARACTER_STATUS == Stall
            || g_pMyPlayerObj->Dead0Stay1Walking2Sit0SkillCast0emotion33Stall12817isridingpet == 17 || g_pMyPlayerObj->CHARACTER_STATUS == SkillCast)
            return;
        unsigned __int8 test = g_CGlobalDataManager->GetEmptyInventorySlots();
        if(test != 0)
        {
            for (std::map<int, CIObject *>::iterator it = g_pGfxEttManager->entities.begin();
                 it != g_pGfxEttManager->entities.end(); ++it) {
                //TODO REPLACE STRING COMPARISON WITH isSame Function
                if (it->second->IsSame(GFX_RUNTIME_CLASS(CIItem))) {
                    CIItem* item = (CIItem*) it->second;
                    if(item)
                    {
                        int ItemUniqueID = *(int *) ((int) (it->second) + 0xF8); //item uniqueid
                        int refObjID = *(int *) ((int) (it->second) + 0x21C); //item refobjid
                        const SItemData *data = &g_CGlobalDataManager->GetItemData(refObjID);
                        if(!ItemUniqueID)
                            continue;
                        if(!data || data == NULL)
                            continue;

                        if(!IsPickupCandidate(item, ItemUniqueID, data))
                            continue;
                        /// PICK GOLD
                        if(data->m_typeId.getTypeID1() == 3 && data->m_typeId.getTypeID2() == 3 && data->m_typeId.getTypeID3() == 5 && data->m_typeId.getTypeID4() == 0)
                        {
                            if(!DontPickGoldCB->GetCheckedState_MAYBE())
                            {
                                CMsgStreamBuffer buf(0x7074);
                                buf << (byte) 1;
                                buf << (byte) 2;
                                buf << (byte) 1;
                                buf << ItemUniqueID;
                                SendMsg(buf);
                            }
                        }
                        else if(data->m_typeId.getTypeID1() == 3 && data->m_typeId.getTypeID2() == 1) /// PICK NORMAL ITEM
                        {
                            if(data->m_itemClass >= 1 && data->m_itemClass <= 3) /// 1DG
                            {
                                if(PickDg1CB->GetCheckedState_MAYBE())
                                {
                                    if(OnlyRareEquiptsCB->GetCheckedState_MAYBE())
                                    {
                                        if(data->Rarity == 6 || data->Rarity == 2)
                                        {
                                            CMsgStreamBuffer buf(0x7074);
                                            buf << (byte) 1;
                                            buf << (byte) 2;
                                            buf << (byte) 1;
                                            buf << ItemUniqueID;
                                            SendMsg(buf);
                                        }
                                    }
                                    else
                                    {
                                        CMsgStreamBuffer buf(0x7074);
                                        buf << (byte) 1;
                                        buf << (byte) 2;
                                        buf << (byte) 1;
                                        buf << ItemUniqueID;
                                        SendMsg(buf);
                                    }
                                }
                            }
                            if(data->m_itemClass >= 4 && data->m_itemClass <= 6) /// 2DG
                            {
                                if(PickDg2CB->GetCheckedState_MAYBE())
                                {
                                    if(OnlyRareEquiptsCB->GetCheckedState_MAYBE())
                                    {
                                        if(data->Rarity == 6 || data->Rarity == 2)
                                        {
                                            CMsgStreamBuffer buf(0x7074);
                                            buf << (byte) 1;
                                            buf << (byte) 2;
                                            buf << (byte) 1;
                                            buf << ItemUniqueID;
                                            SendMsg(buf);
                                        }
                                    }
                                    else
                                    {
                                        CMsgStreamBuffer buf(0x7074);
                                        buf << (byte) 1;
                                        buf << (byte) 2;
                                        buf << (byte) 1;
                                        buf << ItemUniqueID;
                                        SendMsg(buf);
                                    }
                                }
                            }
                            if(data->m_itemClass >= 7 && data->m_itemClass <= 9) /// 3DG
                            {
                                if(PickDg3CB->GetCheckedState_MAYBE())
                                {
                                    if(OnlyRareEquiptsCB->GetCheckedState_MAYBE())
                                    {
                                        if(data->Rarity == 6 || data->Rarity == 2)
                                        {
                                            CMsgStreamBuffer buf(0x7074);
                                            buf << (byte) 1;
                                            buf << (byte) 2;
                                            buf << (byte) 1;
                                            buf << ItemUniqueID;
                                            SendMsg(buf);
                                        }
                                    }
                                    else
                                    {
                                        CMsgStreamBuffer buf(0x7074);
                                        buf << (byte) 1;
                                        buf << (byte) 2;
                                        buf << (byte) 1;
                                        buf << ItemUniqueID;
                                        SendMsg(buf);
                                    }
                                }
                            }
                            if(data->m_itemClass >= 10 && data->m_itemClass <= 12) /// 4DG
                            {
                                if(PickDg4CB->GetCheckedState_MAYBE())
                                {
                                    if(OnlyRareEquiptsCB->GetCheckedState_MAYBE())
                                    {
                                        if(data->Rarity == 6 || data->Rarity == 2)
                                        {
                                            CMsgStreamBuffer buf(0x7074);
                                            buf << (byte) 1;
                                            buf << (byte) 2;
                                            buf << (byte) 1;
                                            buf << ItemUniqueID;
                                            SendMsg(buf);
                                        }
                                    }
                                    else
                                    {
                                        CMsgStreamBuffer buf(0x7074);
                                        buf << (byte) 1;
                                        buf << (byte) 2;
                                        buf << (byte) 1;
                                        buf << ItemUniqueID;
                                        SendMsg(buf);
                                    }
                                }
                            }
                            if(data->m_itemClass >= 13 && data->m_itemClass <= 15) /// 5DG
                            {
                                if(PickDg5CB->GetCheckedState_MAYBE())
                                {
                                    if(OnlyRareEquiptsCB->GetCheckedState_MAYBE())
                                    {
                                        if(data->Rarity == 6 || data->Rarity == 2)
                                        {
                                            CMsgStreamBuffer buf(0x7074);
                                            buf << (byte) 1;
                                            buf << (byte) 2;
                                            buf << (byte) 1;
                                            buf << ItemUniqueID;
                                            SendMsg(buf);
                                        }
                                    }
                                    else
                                    {
                                        CMsgStreamBuffer buf(0x7074);
                                        buf << (byte) 1;
                                        buf << (byte) 2;
                                        buf << (byte) 1;
                                        buf << ItemUniqueID;
                                        SendMsg(buf);
                                    }
                                }
                            }

                            if(data->m_itemClass >= 16 && data->m_itemClass <= 18) /// 6DG
                            {
                                if(PickDg6CB->GetCheckedState_MAYBE())
                                {
                                    if(OnlyRareEquiptsCB->GetCheckedState_MAYBE())
                                    {
                                        if(data->Rarity == 6 || data->Rarity == 2)
                                        {
                                            CMsgStreamBuffer buf(0x7074);
                                            buf << (byte) 1;
                                            buf << (byte) 2;
                                            buf << (byte) 1;
                                            buf << ItemUniqueID;
                                            SendMsg(buf);
                                        }
                                    }
                                    else
                                    {
                                        CMsgStreamBuffer buf(0x7074);
                                        buf << (byte) 1;
                                        buf << (byte) 2;
                                        buf << (byte) 1;
                                        buf << ItemUniqueID;
                                        SendMsg(buf);
                                    }
                                }
                            }

                            if(data->m_itemClass >= 19 && data->m_itemClass <= 21) /// 7DG
                            {
                                if(PickDg7CB->GetCheckedState_MAYBE())
                                {
                                    if(OnlyRareEquiptsCB->GetCheckedState_MAYBE())
                                    {
                                        if(data->Rarity == 6 || data->Rarity == 2)
                                        {
                                            CMsgStreamBuffer buf(0x7074);
                                            buf << (byte) 1;
                                            buf << (byte) 2;
                                            buf << (byte) 1;
                                            buf << ItemUniqueID;
                                            SendMsg(buf);
                                        }
                                    }
                                    else
                                    {
                                        CMsgStreamBuffer buf(0x7074);
                                        buf << (byte) 1;
                                        buf << (byte) 2;
                                        buf << (byte) 1;
                                        buf << ItemUniqueID;
                                        SendMsg(buf);
                                    }
                                }
                            }
                            if(data->m_itemClass >= 22 && data->m_itemClass <= 24) /// 8DG
                            {
                                if(PickDg8CB->GetCheckedState_MAYBE())
                                {
                                    if(OnlyRareEquiptsCB->GetCheckedState_MAYBE())
                                    {
                                        if(data->Rarity == 6 || data->Rarity == 2)
                                        {
                                            CMsgStreamBuffer buf(0x7074);
                                            buf << (byte) 1;
                                            buf << (byte) 2;
                                            buf << (byte) 1;
                                            buf << ItemUniqueID;
                                            SendMsg(buf);
                                        }
                                    }
                                    else
                                    {
                                        CMsgStreamBuffer buf(0x7074);
                                        buf << (byte) 1;
                                        buf << (byte) 2;
                                        buf << (byte) 1;
                                        buf << ItemUniqueID;
                                        SendMsg(buf);
                                    }
                                }
                            }
                            if(data->m_itemClass >= 25 && data->m_itemClass <= 27) /// 9DG
                            {
                                if(PickDg9CB->GetCheckedState_MAYBE())
                                {
                                    if(OnlyRareEquiptsCB->GetCheckedState_MAYBE())
                                    {
                                        if(data->Rarity == 6 || data->Rarity == 2)
                                        {
                                            CMsgStreamBuffer buf(0x7074);
                                            buf << (byte) 1;
                                            buf << (byte) 2;
                                            buf << (byte) 1;
                                            buf << ItemUniqueID;
                                            SendMsg(buf);
                                        }
                                    }
                                    else
                                    {
                                        CMsgStreamBuffer buf(0x7074);
                                        buf << (byte) 1;
                                        buf << (byte) 2;
                                        buf << (byte) 1;
                                        buf << ItemUniqueID;
                                        SendMsg(buf);
                                    }
                                }
                            }
                            if(data->m_itemClass >= 28 && data->m_itemClass <= 30) /// 10DG
                            {
                                if(PickDg10CB->GetCheckedState_MAYBE())
                                {
                                    if(OnlyRareEquiptsCB->GetCheckedState_MAYBE())
                                    {
                                        if(data->Rarity == 6 || data->Rarity == 2)
                                        {
                                            CMsgStreamBuffer buf(0x7074);
                                            buf << (byte) 1;
                                            buf << (byte) 2;
                                            buf << (byte) 1;
                                            buf << ItemUniqueID;
                                            SendMsg(buf);
                                        }
                                    }
                                    else
                                    {
                                        CMsgStreamBuffer buf(0x7074);
                                        buf << (byte) 1;
                                        buf << (byte) 2;
                                        buf << (byte) 1;
                                        buf << ItemUniqueID;
                                        SendMsg(buf);
                                    }
                                }
                            }
                            if(data->m_itemClass >= 31 && data->m_itemClass <= 33) /// 11DG
                            {
                                if(PickDg11CB->GetCheckedState_MAYBE())
                                {
                                    if(OnlyRareEquiptsCB->GetCheckedState_MAYBE())
                                    {
                                        if(data->Rarity == 6 || data->Rarity == 2)
                                        {
                                            CMsgStreamBuffer buf(0x7074);
                                            buf << (byte) 1;
                                            buf << (byte) 2;
                                            buf << (byte) 1;
                                            buf << ItemUniqueID;
                                            SendMsg(buf);
                                        }
                                    }
                                    else
                                    {
                                        CMsgStreamBuffer buf(0x7074);
                                        buf << (byte) 1;
                                        buf << (byte) 2;
                                        buf << (byte) 1;
                                        buf << ItemUniqueID;
                                        SendMsg(buf);
                                    }
                                }
                            }
                            if(data->m_itemClass >= 34 && data->m_itemClass <= 36) /// 12DG
                            {
                                if(PickDg12CB->GetCheckedState_MAYBE())
                                {
                                    if(OnlyRareEquiptsCB->GetCheckedState_MAYBE())
                                    {
                                        if(data->Rarity == 6 || data->Rarity == 2)
                                        {
                                            CMsgStreamBuffer buf(0x7074);
                                            buf << (byte) 1;
                                            buf << (byte) 2;
                                            buf << (byte) 1;
                                            buf << ItemUniqueID;
                                            SendMsg(buf);
                                        }
                                    }
                                    else
                                    {
                                        CMsgStreamBuffer buf(0x7074);
                                        buf << (byte) 1;
                                        buf << (byte) 2;
                                        buf << (byte) 1;
                                        buf << ItemUniqueID;
                                        SendMsg(buf);
                                    }
                                }
                            }
                            if(data->m_itemClass >= 37 && data->m_itemClass <= 39) /// 13DG
                            {
                                if(PickDg13CB->GetCheckedState_MAYBE())
                                {
                                    if(OnlyRareEquiptsCB->GetCheckedState_MAYBE())
                                    {
                                        if(data->Rarity == 6 || data->Rarity == 2)
                                        {
                                            CMsgStreamBuffer buf(0x7074);
                                            buf << (byte) 1;
                                            buf << (byte) 2;
                                            buf << (byte) 1;
                                            buf << ItemUniqueID;
                                            SendMsg(buf);
                                        }
                                    }
                                    else
                                    {
                                        CMsgStreamBuffer buf(0x7074);
                                        buf << (byte) 1;
                                        buf << (byte) 2;
                                        buf << (byte) 1;
                                        buf << ItemUniqueID;
                                        SendMsg(buf);
                                    }
                                }
                            }
                            if(data->m_itemClass >= 40 && data->m_itemClass <= 42) /// 14DG
                            {
                                if(PickDg14CB->GetCheckedState_MAYBE())
                                {
                                    if(OnlyRareEquiptsCB->GetCheckedState_MAYBE())
                                    {
                                        if(data->Rarity == 6 || data->Rarity == 2)
                                        {
                                            CMsgStreamBuffer buf(0x7074);
                                            buf << (byte) 1;
                                            buf << (byte) 2;
                                            buf << (byte) 1;
                                            buf << ItemUniqueID;
                                            SendMsg(buf);
                                        }
                                    }
                                    else
                                    {
                                        CMsgStreamBuffer buf(0x7074);
                                        buf << (byte) 1;
                                        buf << (byte) 2;
                                        buf << (byte) 1;
                                        buf << ItemUniqueID;
                                        SendMsg(buf);
                                    }
                                }
                            }
                            if(data->m_itemClass >= 43 && data->m_itemClass <= 45) /// 15DG
                            {
                                if(PickDg15CB->GetCheckedState_MAYBE())
                                {
                                    if(OnlyRareEquiptsCB->GetCheckedState_MAYBE())
                                    {
                                        if(data->Rarity == 6 || data->Rarity == 2)
                                        {
                                            CMsgStreamBuffer buf(0x7074);
                                            buf << (byte) 1;
                                            buf << (byte) 2;
                                            buf << (byte) 1;
                                            buf << ItemUniqueID;
                                            SendMsg(buf);
                                        }
                                    }
                                    else
                                    {
                                        CMsgStreamBuffer buf(0x7074);
                                        buf << (byte) 1;
                                        buf << (byte) 2;
                                        buf << (byte) 1;
                                        buf << ItemUniqueID;
                                        SendMsg(buf);
                                    }
                                }
                            }

                            if(data->m_itemClass >= 46 && data->m_itemClass <= 48) /// 16DG
                            {
                                if(PickDg16CB->GetCheckedState_MAYBE())
                                {
                                    if(OnlyRareEquiptsCB->GetCheckedState_MAYBE())
                                    {
                                        if(data->Rarity == 6 || data->Rarity == 2)
                                        {
                                            CMsgStreamBuffer buf(0x7074);
                                            buf << (byte) 1;
                                            buf << (byte) 2;
                                            buf << (byte) 1;
                                            buf << ItemUniqueID;
                                            SendMsg(buf);
                                        }
                                    }
                                    else
                                    {
                                        CMsgStreamBuffer buf(0x7074);
                                        buf << (byte) 1;
                                        buf << (byte) 2;
                                        buf << (byte) 1;
                                        buf << ItemUniqueID;
                                        SendMsg(buf);
                                    }
                                }
                            }

                            if(data->m_itemClass >= 49 && data->m_itemClass <= 51) /// 17DG
                            {
                                if(PickDg17CB->GetCheckedState_MAYBE())
                                {
                                    if(OnlyRareEquiptsCB->GetCheckedState_MAYBE())
                                    {
                                        if(data->Rarity == 6 || data->Rarity == 2)
                                        {
                                            CMsgStreamBuffer buf(0x7074);
                                            buf << (byte) 1;
                                            buf << (byte) 2;
                                            buf << (byte) 1;
                                            buf << ItemUniqueID;
                                            SendMsg(buf);
                                        }
                                    }
                                    else
                                    {
                                        CMsgStreamBuffer buf(0x7074);
                                        buf << (byte) 1;
                                        buf << (byte) 2;
                                        buf << (byte) 1;
                                        buf << ItemUniqueID;
                                        SendMsg(buf);
                                    }
                                }
                            }

                            if(data->m_itemClass >= 52 && data->m_itemClass <= 54) /// 18DG
                            {
                                if(PickDg18CB->GetCheckedState_MAYBE())
                                {
                                    if(OnlyRareEquiptsCB->GetCheckedState_MAYBE())
                                    {
                                        if(data->Rarity == 6 || data->Rarity == 2)
                                        {
                                            CMsgStreamBuffer buf(0x7074);
                                            buf << (byte) 1;
                                            buf << (byte) 2;
                                            buf << (byte) 1;
                                            buf << ItemUniqueID;
                                            SendMsg(buf);
                                        }
                                    }
                                    else
                                    {
                                        CMsgStreamBuffer buf(0x7074);
                                        buf << (byte) 1;
                                        buf << (byte) 2;
                                        buf << (byte) 1;
                                        buf << ItemUniqueID;
                                        SendMsg(buf);
                                    }
                                }
                            }
                        }
                        else if(data->IsAlchemyTablet()) /// PICK POTS ?
                        {
                            if(data->m_itemClass == 1)
                            {
                                if(PickDg1CB->GetCheckedState_MAYBE())
                                {
                                    if(!DontPickAlchemytablesCB->GetCheckedState_MAYBE())
                                    {
                                        CMsgStreamBuffer buf(0x7074);
                                        buf << (byte) 1;
                                        buf << (byte) 2;
                                        buf << (byte) 1;
                                        buf << ItemUniqueID;
                                        SendMsg(buf);
                                    }
                                }
                            }
                            if(data->m_itemClass == 2)
                            {
                                if(PickDg2CB->GetCheckedState_MAYBE())
                                {
                                    if(!DontPickAlchemytablesCB->GetCheckedState_MAYBE())
                                    {
                                        CMsgStreamBuffer buf(0x7074);
                                        buf << (byte) 1;
                                        buf << (byte) 2;
                                        buf << (byte) 1;
                                        buf << ItemUniqueID;
                                        SendMsg(buf);
                                    }
                                }
                            }
                            if(data->m_itemClass == 3)
                            {
                                if(PickDg3CB->GetCheckedState_MAYBE())
                                {
                                    if(!DontPickAlchemytablesCB->GetCheckedState_MAYBE())
                                    {
                                        CMsgStreamBuffer buf(0x7074);
                                        buf << (byte) 1;
                                        buf << (byte) 2;
                                        buf << (byte) 1;
                                        buf << ItemUniqueID;
                                        SendMsg(buf);
                                    }
                                }
                            }
                            if(data->m_itemClass == 4)
                            {
                                if(PickDg4CB->GetCheckedState_MAYBE())
                                {
                                    if(!DontPickAlchemytablesCB->GetCheckedState_MAYBE())
                                    {
                                        CMsgStreamBuffer buf(0x7074);
                                        buf << (byte) 1;
                                        buf << (byte) 2;
                                        buf << (byte) 1;
                                        buf << ItemUniqueID;
                                        SendMsg(buf);
                                    }
                                }
                            }
                            if(data->m_itemClass == 5)
                            {
                                if(PickDg5CB->GetCheckedState_MAYBE())
                                {
                                    if(!DontPickAlchemytablesCB->GetCheckedState_MAYBE())
                                    {
                                        CMsgStreamBuffer buf(0x7074);
                                        buf << (byte) 1;
                                        buf << (byte) 2;
                                        buf << (byte) 1;
                                        buf << ItemUniqueID;
                                        SendMsg(buf);
                                    }
                                }
                            }
                            if(data->m_itemClass == 6)
                            {
                                if(PickDg6CB->GetCheckedState_MAYBE())
                                {
                                    if(!DontPickAlchemytablesCB->GetCheckedState_MAYBE())
                                    {
                                        CMsgStreamBuffer buf(0x7074);
                                        buf << (byte) 1;
                                        buf << (byte) 2;
                                        buf << (byte) 1;
                                        buf << ItemUniqueID;
                                        SendMsg(buf);
                                    }
                                }
                            }
                            if(data->m_itemClass == 7)
                            {
                                if(PickDg7CB->GetCheckedState_MAYBE())
                                {
                                    if(!DontPickAlchemytablesCB->GetCheckedState_MAYBE())
                                    {
                                        CMsgStreamBuffer buf(0x7074);
                                        buf << (byte) 1;
                                        buf << (byte) 2;
                                        buf << (byte) 1;
                                        buf << ItemUniqueID;
                                        SendMsg(buf);
                                    }
                                }
                            }
                            if(data->m_itemClass == 8)
                            {
                                if(PickDg8CB->GetCheckedState_MAYBE())
                                {
                                    if(!DontPickAlchemytablesCB->GetCheckedState_MAYBE())
                                    {
                                        CMsgStreamBuffer buf(0x7074);
                                        buf << (byte) 1;
                                        buf << (byte) 2;
                                        buf << (byte) 1;
                                        buf << ItemUniqueID;
                                        SendMsg(buf);
                                    }
                                }
                            }
                            if(data->m_itemClass == 9)
                            {
                                if(PickDg9CB->GetCheckedState_MAYBE())
                                {
                                    if(!DontPickAlchemytablesCB->GetCheckedState_MAYBE())
                                    {
                                        CMsgStreamBuffer buf(0x7074);
                                        buf << (byte) 1;
                                        buf << (byte) 2;
                                        buf << (byte) 1;
                                        buf << ItemUniqueID;
                                        SendMsg(buf);
                                    }
                                }
                            }
                            if(data->m_itemClass == 10)
                            {
                                if(PickDg10CB->GetCheckedState_MAYBE())
                                {
                                    if(!DontPickAlchemytablesCB->GetCheckedState_MAYBE())
                                    {
                                        CMsgStreamBuffer buf(0x7074);
                                        buf << (byte) 1;
                                        buf << (byte) 2;
                                        buf << (byte) 1;
                                        buf << ItemUniqueID;
                                        SendMsg(buf);
                                    }
                                }
                            }
                            if(data->m_itemClass == 11)
                            {
                                if(PickDg11CB->GetCheckedState_MAYBE())
                                {
                                    if(!DontPickAlchemytablesCB->GetCheckedState_MAYBE())
                                    {
                                        CMsgStreamBuffer buf(0x7074);
                                        buf << (byte) 1;
                                        buf << (byte) 2;
                                        buf << (byte) 1;
                                        buf << ItemUniqueID;
                                        SendMsg(buf);
                                    }
                                }
                            }
                            if(data->m_itemClass == 12)
                            {
                                if(PickDg12CB->GetCheckedState_MAYBE())
                                {
                                    if(!DontPickAlchemytablesCB->GetCheckedState_MAYBE())
                                    {
                                        CMsgStreamBuffer buf(0x7074);
                                        buf << (byte) 1;
                                        buf << (byte) 2;
                                        buf << (byte) 1;
                                        buf << ItemUniqueID;
                                        SendMsg(buf);
                                    }
                                }
                            }
                            if(data->m_itemClass == 13)
                            {
                                if(PickDg13CB->GetCheckedState_MAYBE())
                                {
                                    if(!DontPickAlchemytablesCB->GetCheckedState_MAYBE())
                                    {
                                        CMsgStreamBuffer buf(0x7074);
                                        buf << (byte) 1;
                                        buf << (byte) 2;
                                        buf << (byte) 1;
                                        buf << ItemUniqueID;
                                        SendMsg(buf);
                                    }
                                }
                            }
                            if(data->m_itemClass == 14)
                            {
                                if(PickDg14CB->GetCheckedState_MAYBE())
                                {
                                    if(!DontPickAlchemytablesCB->GetCheckedState_MAYBE())
                                    {
                                        CMsgStreamBuffer buf(0x7074);
                                        buf << (byte) 1;
                                        buf << (byte) 2;
                                        buf << (byte) 1;
                                        buf << ItemUniqueID;
                                        SendMsg(buf);
                                    }
                                }
                            }
                            if(data->m_itemClass == 15)
                            {
                                if(PickDg15CB->GetCheckedState_MAYBE())
                                {
                                    if(!DontPickAlchemytablesCB->GetCheckedState_MAYBE())
                                    {
                                        CMsgStreamBuffer buf(0x7074);
                                        buf << (byte) 1;
                                        buf << (byte) 2;
                                        buf << (byte) 1;
                                        buf << ItemUniqueID;
                                        SendMsg(buf);
                                    }
                                }
                            }
                            if(data->m_itemClass == 16)
                            {
                                if(PickDg16CB->GetCheckedState_MAYBE())
                                {
                                    if(!DontPickAlchemytablesCB->GetCheckedState_MAYBE())
                                    {
                                        CMsgStreamBuffer buf(0x7074);
                                        buf << (byte) 1;
                                        buf << (byte) 2;
                                        buf << (byte) 1;
                                        buf << ItemUniqueID;
                                        SendMsg(buf);
                                    }
                                }
                            }
                            if(data->m_itemClass == 17)
                            {
                                if(PickDg17CB->GetCheckedState_MAYBE())
                                {
                                    if(!DontPickAlchemytablesCB->GetCheckedState_MAYBE())
                                    {
                                        CMsgStreamBuffer buf(0x7074);
                                        buf << (byte) 1;
                                        buf << (byte) 2;
                                        buf << (byte) 1;
                                        buf << ItemUniqueID;
                                        SendMsg(buf);
                                    }
                                }
                            }
                            if(data->m_itemClass == 18)
                            {
                                if(PickDg18CB->GetCheckedState_MAYBE())
                                {
                                    if(!DontPickAlchemytablesCB->GetCheckedState_MAYBE())
                                    {
                                        CMsgStreamBuffer buf(0x7074);
                                        buf << (byte) 1;
                                        buf << (byte) 2;
                                        buf << (byte) 1;
                                        buf << ItemUniqueID;
                                        SendMsg(buf);
                                    }
                                }
                            }
                        }
                        else if(data->IsMagicStone() || data->IsMagicStone2() || data->IsAttrStone())
                        {
                            if(data->m_itemClass >= 1 && data->m_itemClass <= 18)
                            {
                                bool pickItem = false;
                                switch (data->m_itemClass)
                                {
                                    case 1: pickItem = PickDg1CB->GetCheckedState_MAYBE(); break;
                                    case 2: pickItem = PickDg2CB->GetCheckedState_MAYBE(); break;
                                    case 3: pickItem = PickDg3CB->GetCheckedState_MAYBE(); break;
                                    case 4: pickItem = PickDg4CB->GetCheckedState_MAYBE(); break;
                                    case 5: pickItem = PickDg5CB->GetCheckedState_MAYBE(); break;
                                    case 6: pickItem = PickDg6CB->GetCheckedState_MAYBE(); break;
                                    case 7: pickItem = PickDg7CB->GetCheckedState_MAYBE(); break;
                                    case 8: pickItem = PickDg8CB->GetCheckedState_MAYBE(); break;
                                    case 9: pickItem = PickDg9CB->GetCheckedState_MAYBE(); break;
                                    case 10: pickItem = PickDg10CB->GetCheckedState_MAYBE(); break;
                                    case 11: pickItem = PickDg11CB->GetCheckedState_MAYBE(); break;
                                    case 12: pickItem = PickDg12CB->GetCheckedState_MAYBE(); break;
                                    case 13: pickItem = PickDg13CB->GetCheckedState_MAYBE(); break;
                                    case 14: pickItem = PickDg14CB->GetCheckedState_MAYBE(); break;
                                    case 15: pickItem = PickDg15CB->GetCheckedState_MAYBE(); break;
                                    case 16: pickItem = PickDg16CB->GetCheckedState_MAYBE(); break;
                                    case 17: pickItem = PickDg17CB->GetCheckedState_MAYBE(); break;
                                    case 18: pickItem = PickDg18CB->GetCheckedState_MAYBE(); break;
                                    default: break;
                                }

                                if(pickItem && !DontPickAlchemyStonesCB->GetCheckedState_MAYBE())
                                {
                                    CMsgStreamBuffer buf(0x7074);
                                    buf << (byte) 1;
                                    buf << (byte) 2;
                                    buf << (byte) 1;
                                    buf << ItemUniqueID;
                                    SendMsg(buf);
                                }
                            }
                        }
                        else if(data->IsElixir())
                        {
                            if(!DontPickElixirsCB->GetCheckedState_MAYBE())
                            {
                                CMsgStreamBuffer buf(0x7074);
                                buf << (byte) 1;
                                buf << (byte) 2;
                                buf << (byte) 1;
                                buf << ItemUniqueID;
                                SendMsg(buf);
                            }
                        }
                        else if(data->IsArrow() || data->IsBolt())
                        {
                            if(!DontPickArrowCB->GetCheckedState_MAYBE())
                            {
                                CMsgStreamBuffer buf(0x7074);
                                buf << (byte) 1;
                                buf << (byte) 2;
                                buf << (byte) 1;
                                buf << ItemUniqueID;
                                SendMsg(buf);
                            }
                        }
                        else if(data->CodeName == L"ITEM_ETC_SCROLL_RETURN_02")
                        {
                            if(!DontPickReturnCB->GetCheckedState_MAYBE())
                            {
                                CMsgStreamBuffer buf(0x7074);
                                buf << (byte) 1;
                                buf << (byte) 2;
                                buf << (byte) 1;
                                buf << ItemUniqueID;
                                SendMsg(buf);
                            }
                        }
                        else if(data->IsAlchemyMaterial())
                        {
                            if(!DontPickTrashCB->GetCheckedState_MAYBE())
                            {
                                CMsgStreamBuffer buf(0x7074);
                                buf << (byte) 1;
                                buf << (byte) 2;
                                buf << (byte) 1;
                                buf << ItemUniqueID;
                                SendMsg(buf);
                            }
                        }
                        else if(data->IsHPPotion() || data->IsMPPotion())
                        {
                            if(!DontPickHpMp->GetCheckedState_MAYBE())
                            {
                                CMsgStreamBuffer buf(0x7074);
                                buf << (byte) 1;
                                buf << (byte) 2;
                                buf << (byte) 1;
                                buf << ItemUniqueID;
                                SendMsg(buf);
                            }
                        }
                        else if(data->IsVIGOR())
                        {
                            if(!DontPickVigor->GetCheckedState_MAYBE())
                            {
                                CMsgStreamBuffer buf(0x7074);
                                buf << (byte) 1;
                                buf << (byte) 2;
                                buf << (byte) 1;
                                buf << ItemUniqueID;
                                SendMsg(buf);
                            }
                        }
                        else
                        {
                            CMsgStreamBuffer buf(0x7074);
                            buf << (byte) 1;
                            buf << (byte) 2;
                            buf << (byte) 1;
                            buf << ItemUniqueID;
                            SendMsg(buf);
                        }
                    }
                }
                //  if(g_pCGInterface->m_IRM.GetResObj<CIFMacroMenu>(MacroMenuID, 1)->PickupFilterSlot->
            }
        }

    }
}
