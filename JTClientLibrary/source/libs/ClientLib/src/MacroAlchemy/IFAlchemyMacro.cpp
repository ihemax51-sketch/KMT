//
// Created by YUMBUL on 23.06.2024.
//

#include "IFAlchemyMacro.h"
#include "Game.h"
#include "IFAlchemyMacroBlueSlot.h"
#include <BSLib/Debug.h>
#include <GInterface.h>
#include <IFSliderCtrl.h>
#include <ICPlayer.h>
#include <TextStringManager.h>
#include <CustomData/CustomCICPlayer.h>
#include <GlobalDataManager.h>
#include <CustomData/CustomSettingManager.h>

#include <EntityManagerClient.h>
#include <ICMonster.h>
#include <cmath>
#include <IFPlayerMiniInfo.h>
#include <CharacterDependentData.h>
#include <sstream>
#include <BSLib/multibyte.h>
#include <fstream>
#include <iostream>
#include <algorithm>
#include <IItem.h>
#include <SRIFLib/NIFEnchantWnd.h>


GFX_IMPLEMENT_DYNCREATE(CIFAlchemyMacro, CIFMainFrame)
GFX_BEGIN_MESSAGE_MAP(CIFAlchemyMacro, CIFMainFrame)
                    ONG_COMMAND(205, &CIFAlchemyMacro::AutoPlusPage)
                    ONG_COMMAND(204, &CIFAlchemyMacro::AutoBluePage)
                    ONG_COMMAND(203, &CIFAlchemyMacro::AutoStatPage)

                    ONG_COMMAND(208, &CIFAlchemyMacro::StartButton)
                    ONG_COMMAND(209, &CIFAlchemyMacro::StopButton)
                    ONG_COMMAND(10, &CIFAlchemyMacro::PLUS_UP_BUTTON)
                    ONG_COMMAND(11, &CIFAlchemyMacro::PLUS_DOWN_BUTTON)
                    ONG_COMMAND(14, &CIFAlchemyMacro::LUCKYPOWDER_UP_BUTTON)
                    ONG_COMMAND(15, &CIFAlchemyMacro::LUCKYPOWDER_DOWN_BUTTON)

                    ONG_COMMAND(19, &CIFAlchemyMacro::IMMORTAL_UP_BUTTON)
                    ONG_COMMAND(20, &CIFAlchemyMacro::IMMORTAL_DOWN_BUTTON)

                    ONG_COMMAND(24, &CIFAlchemyMacro::ASTRAL_UP_BUTTON)
                    ONG_COMMAND(25, &CIFAlchemyMacro::ASTRAL_DOWN_BUTTON)

                    ONG_COMMAND(29, &CIFAlchemyMacro::STEADY_UP_BUTTON)
                    ONG_COMMAND(30, &CIFAlchemyMacro::STEADY_DOWN_BUTTON)

                    ONG_COMMAND(34, &CIFAlchemyMacro::LUCKY_UP_BUTTON)
                    ONG_COMMAND(35, &CIFAlchemyMacro::LUCKY_DOWN_BUTTON)

                    ONG_COMMAND(59, &CIFAlchemyMacro::AddMattrToList)
                    ONG_COMMAND(62, &CIFAlchemyMacro::RemoveMattrToList)

GFX_END_MESSAGE_MAP()


int CIFAlchemyMacro::Func_4(int a2) {
    int v1 = 0;
    while (a2 != v1 + 100) {
        if (++v1 >= 5)
            return -1;
    }

    return 100;
}

void CIFAlchemyMacro::UpdateMenuSize()
{


    if(g_pCGInterface->GetGuiFromList<CNIFEnchantWnd>(168) != NULL)
    {
        wnd_pos pp = g_pCGInterface->GetGuiFromList<CNIFEnchantWnd>(168)->GetPos();
        this->MoveGWnd(pp.x+400, pp.y);
    }
    else
    {
        int PosX = 0, PosY = 0;
        PosY = (g_CGame->GetRes().res->height/2 - 20) - (this->GetSize().height/2);
        PosX = (g_CGame->GetRes().res->width/2 - 300) - (this->GetSize().width/2);
        this->MoveGWnd(PosX, PosY);
        BringToFront();
        CurrentPage = 0;
        AutoPlusPage();
    }

}

CIFAlchemyMacro::CIFAlchemyMacro(void) {
    CurrentPage = 0;
    TargetPlus = 1;
    LuckyPowderMinPlus = 0;
    ImmortalMinPlus = 0;
    AstralMinPlus = 0;
    SteadyMinPlus = 0;
    LuckyMinPlus = 0;
    Fusing = false;
    AutomationRunning = false;
    SelectedAttrCode = std::wstring();
    SelectedRemoveAttrCode = std::wstring();
}
CIFAlchemyMacro::~CIFAlchemyMacro(void) {

    BS_DEBUG_LOW(">" __FUNCTION__);
}
bool CIFAlchemyMacro::OnCreate(long ln) {

    // Populate inherited members
    CIFMainFrame::OnCreate(ln);
    this->SetText(KmtGetText(L"UIIT_KMT_ALCHEMY_MACRO"));
    wnd_rect sz;
    m_IRM.LoadFromFile("clientlibrary\\resinfo\\ifalchemymacro.txt");
    m_IRM.CreateInterfaceSection("Create", this);


    m_ItemSlot = m_IRM.GetResObj<CIFMacroAlchemySlot>(210, 1);
    m_ItemSlot->m_pMySlot->m_pSlot->SetSlot(500);
    m_ItemSlot->m_pMySlot->m_pSlot->SetType(0xC);


    this->m_IRM.GetResObj(62, 1)->SetText(KmtGetText(L"UIIT_KMT_REMOVE"));
    this->m_IRM.GetResObj(208, 1)->SetText(KmtGetText(L"UIIT_KMT_START"));
    this->m_IRM.GetResObj(209, 1)->SetText(KmtGetText(L"UIIT_KMT_STOP"));

    this->m_IRM.GetResObj(205, 1)->SetText(KmtGetText(L"UIIT_KMT_AUTO_PLUS"));
    this->m_IRM.GetResObj(204, 1)->SetText(KmtGetText(L"UIIT_KMT_AUTO_BLUE"));
    this->m_IRM.GetResObj(203, 1)->SetText(KmtGetText(L"UIIT_KMT_AUTO_STAT"));

    this->m_IRM.GetResObj(8, 1)->SetText(KmtGetText(L"UIIT_KMT_TARGET_PLUS"));


    TargetPlusLabel = this->m_IRM.GetResObj<CIFStatic>(9, 1);
    wchar_t Priceb[255];
    swprintf_s(Priceb, L"+%d", TargetPlus);
    TargetPlusLabel->SetText(Priceb);


    this->m_IRM.GetResObj(12, 1)->SetText(KmtGetText(L"UIIT_KMT_LUCKY_POWDER"));
    LuckyPowderMinPlusLabel = this->m_IRM.GetResObj<CIFStatic>(13, 1);
    swprintf_s(Priceb, L"+%d", LuckyPowderMinPlus);
    LuckyPowderMinPlusLabel->SetText(Priceb);
    LuckyPowderCheckBox = this->m_IRM.GetResObj<CIFCheckBox>(16, 1);
    LuckyPowderCheckBox->BringToFront();



    this->m_IRM.GetResObj(17, 1)->SetText(KmtGetText(L"UIIT_KMT_IMMORTAL_STONE"));

    ImmortalMinPlusLabel = this->m_IRM.GetResObj<CIFStatic>(18, 1);
    swprintf_s(Priceb, L"+%d", ImmortalMinPlus);
    ImmortalMinPlusLabel->SetText(Priceb);
    ImmortalCheckBox = this->m_IRM.GetResObj<CIFCheckBox>(21, 1);
    ImmortalCheckBox->BringToFront();


    this->m_IRM.GetResObj(22, 1)->SetText(KmtGetText(L"UIIT_KMT_ASTRAL_STONE"));

    AstralMinPlusLabel = this->m_IRM.GetResObj<CIFStatic>(23, 1);
    swprintf_s(Priceb, L"+%d", AstralMinPlus);
    AstralMinPlusLabel->SetText(Priceb);
    AstralCheckBox = this->m_IRM.GetResObj<CIFCheckBox>(26, 1);
    AstralCheckBox->BringToFront();


    this->m_IRM.GetResObj(27, 1)->SetText(KmtGetText(L"UIIT_KMT_STEADY_STONE"));

    SteadyMinPlusLabel = this->m_IRM.GetResObj<CIFStatic>(28, 1);
    swprintf_s(Priceb, L"+%d", SteadyMinPlus);
    SteadyMinPlusLabel->SetText(Priceb);
    SteadyCheckBox = this->m_IRM.GetResObj<CIFCheckBox>(31, 1);
    SteadyCheckBox->BringToFront();

    this->m_IRM.GetResObj(32, 1)->SetText(KmtGetText(L"UIIT_KMT_LUCKY_STONE"));

    LuckyMinPlusLabel = this->m_IRM.GetResObj<CIFStatic>(33, 1);
    swprintf_s(Priceb, L"+%d", LuckyMinPlus);
    LuckyMinPlusLabel->SetText(Priceb);
    LuckyCheckBox = this->m_IRM.GetResObj<CIFCheckBox>(36, 1);
    LuckyCheckBox->BringToFront();


    sz.pos.x = 2;
    sz.pos.y = 26;
    sz.size.width = 128;
    sz.size.height = 128;
    Slotdeco = m_IRM.GetResObj<CIFDecoratedStatic>(207, 0);
  //  Slotdeco->ShowGWnd(false);
   // Slotdeco->FUN_00634470(L"interface\\alchemy\\alcm_effect_fail_1.ddj");

// X + 47
// Y + 42

    m_ItemSlot->BringToFront();
    m_ItemSlot->m_pMySlot->m_pSlot->BringToFront();

    //ActivateTabPage(0);
    this->ShowGWnd(false);

    m_IRM.GetResObj(59, 1)->SetStyleThingy(TOOLTIP);
    m_IRM.GetResObj(59, 1)->SetTooltip(KmtGetText(L"UIIT_KMT_ADD_TO_LIST"));

    m_scroll = this->m_IRM.GetResObj<CIFScrollManager>(41, 1);
    m_scroll->sub_008124F0(0);
    m_scroll->sub_008124C0(23);
    m_scroll->sub_008123F0(7);
    m_scroll->sub_00812500(0);
    m_scroll->sub_00812420(-8, 0);

    m_scrollattr = this->m_IRM.GetResObj<CIFScrollManager>(58, 1);
    m_scrollattr->sub_008124F0(0);
    m_scrollattr->sub_008124C0(23);
    m_scrollattr->sub_008123F0(7);
    m_scrollattr->sub_00812500(0);
    m_scrollattr->sub_00812420(-8, 0);

    std::n_wstring area1 = L"%25";
    std::n_wstring area2 = L"%50";
    std::n_wstring area3 = L"%75";
    std::n_wstring area4 = L"%100";


    m_IRM.GetResObj<CIFEdit>(60, 1)->SetMaxLength(3);
    wnd_size ttx =  m_IRM.GetResObj<CIFEdit>(60, 1)->GetSize();

     m_IRM.GetResObj<CIFEdit>(60, 1)->SetText(L"0");
     m_IRM.GetResObj<CIFEdit>(60, 1)->SetTextmode(ttx.width);

    m_IRM.GetResObj<CIFEdit>(60, 1)->AddValue_404(4);
    m_IRM.GetResObj<CIFEdit>(60, 1)->SetValue_404(1);


    std::n_wstring msg = KmtGetText(L"UIIT_KMT_REACH_TO_STAT");
    std::n_wstring labelmsg = msg.substr(0, 5) + L"..";
    m_IRM.GetResObj(61, 1)->SetText(labelmsg.c_str());
    m_IRM.GetResObj(61, 1)->SetTooltip(msg);
    m_IRM.GetResObj(61, 1)->SetStyleThingy(TOOLTIP);



    sz.pos.x = 25;
    sz.pos.y = 142;
    sz.size.width = 201;
    sz.size.height = 24;
    for (int i = 0; i < 11; ++i)
    {
        m_BlueSlots[i] = (CIFAlchemyMacroBlueSlot *) CGWnd::CreateInstance(this, GFX_RUNTIME_CLASS(CIFAlchemyMacroBlueSlot), sz, 302+i, 0);
        m_BlueSlots[i]->ShowGWnd(false);
        sz.pos.y += 23;
    }

    sz.pos.x = 255;
    sz.pos.y = 142;
    sz.size.width = 201;
    sz.size.height = 24;
    for (int i = 0; i < 11; ++i)
    {
        m_AttrSlots[i] = (CIFAlchemyMacroAttrSlot *) CGWnd::CreateInstance(this, GFX_RUNTIME_CLASS(CIFAlchemyMacroAttrSlot), sz, 402+i, 0);
        m_AttrSlots[i]->ShowGWnd(false);
        sz.pos.y += 23;
    }

    ///set_N00009BD4 == 1
    ///  set_N00009BDC == 0
/// set_N00009BD4 = 0
    return true;
}

void CIFAlchemyMacro::WeaponAddedtoSlot()
{
    SelectedAttrCode.clear();
    SelectedRemoveAttrCode.clear();
    m_BlueList.clear();
    m_AttrList.clear();
    m_TargetAttrList.clear();
    ClearList();
    ClearDDJList();
    ClearListAttr();
    ClearDDJListAttr();
    if(CurrentPage == 1)
    {
        const SItemData * data = m_ItemSlot->m_pMySlot->m_pSlot->ItemInfo->GetItemData();
        int Degree = GetDegreeLevel(data->m_itemClass);
        for(std::map<unsigned __int32, CMagicOptionData *>::iterator it = g_CGlobalDataManager->m_magicOptionDataMap.begin();
            it != g_CGlobalDataManager->m_magicOptionDataMap.end(); it++)
        {
            if((it->second->AvailItemGroup1 == L"weapon" && it->second->ReqClass1 == 1) ||
               (it->second->AvailItemGroup2 == L"weapon" && it->second->ReqClass2 == 1)
               || (it->second->AvailItemGroup3 == L"weapon" && it->second->ReqClass3 == 1)
               || (it->second->AvailItemGroup4 == L"weapon" && it->second->ReqClass4 == 1))
            {
                if(it->second->MLevel == Degree)
                {
                    if(it->second->Param1 == 7566450)
                    {
                        MagStr aa = MagStr();
                        aa.MagID = it->first;
                        aa.Param4 = it->second->Param4;
                        aa.Code = KmtGetText(L"UIIT_KMT_STR");
                        aa.Param1 = it->second->Param1;
                        m_BlueList.push_back(aa);;
                    }
                    else if(it->second->Param1 == 6909556)
                    {
                        MagStr aa = MagStr();
                        aa.MagID = it->first;
                        aa.Param4 = it->second->Param4;
                        aa.Code = KmtGetText(L"UIIT_KMT_INT");
                        aa.Param1 = it->second->Param1;
                        m_BlueList.push_back(aa);
                    }
                    else if(it->second->Param1 == 1685418613)
                    {
                        MagStr aa = MagStr();
                        aa.MagID = it->first;
                        aa.Param4 = it->second->Param4;
                        aa.Code = KmtGetText(L"UIIT_KMT_DURABILITY");
                        aa.Param1 = it->second->Param1;
                        m_BlueList.push_back(aa);
                    }
                    else if(it->second->Param1 == 26738)
                    {
                        MagStr aa = MagStr();
                        aa.MagID = it->first;
                        aa.Param4 = it->second->Param4;
                        aa.Code = KmtGetText(L"UIIT_KMT_ATTACK_RATE");
                        aa.Param1 = it->second->Param1;
                        m_BlueList.push_back(aa);
                    }
                    else if(it->second->Param1 == 1702257260)
                    {
                        MagStr aa = MagStr();
                        aa.MagID = it->first;
                        aa.Param4 = it->second->Param4;
                        aa.Code = KmtGetText(L"UIIT_KMT_BLOCK_RATE");
                        aa.Param1 = it->second->Param1;
                        m_BlueList.push_back(aa);
                    }
                }

            }
        }
            LoadBlueList();
    }
    else if(CurrentPage == 2)
    {
        /*CSOItem *myitem = this->ItemInfo; static const CItemData *data = NULL;
        if (myitem != NULL)
        { data = g_CGlobalDataManager->GetItem(myitem->GetItemData()->RefObjectId); }
        std::wstring mymsg(L"\n <<Comparison>>");
        float magAttack = CalculateMagAttack(data->GetData().m_magAttackMinMin,
                                             data->GetData().m_magAttackMinMax,
                                             data->GetData().m_magAttackMaxMin,
                                             data->GetData().m_magAttackMaxMax,
                                             myitem->m_OptLevel,
                                             data->GetData().m_phyAttackIncrease,
                                             myitem->m_MagAtkPwrMin,
                                             myitem->m_MagAtkPwrMax);
        */
        const SItemData * data = m_ItemSlot->m_pMySlot->m_pSlot->ItemInfo->GetItemData();
        if(data->m_typeId.getTypeID4() < 7)
        {
            AttrStr aa = AttrStr();
            aa.Name = KmtGetText(L"UIIT_KMT_PHYSICAL_ATTACK_POWER");
            aa.Code = L"NATTR_PA";
            m_AttrList.push_back(aa);

            AttrStr aax = AttrStr();
            aax.Name = KmtGetText(L"UIIT_KMT_PHYSICAL_REINFORCE");
            aax.Code = L"NATTR_PASTR";
            m_AttrList.push_back(aax);


            AttrStr aaxx = AttrStr();
            aaxx.Name = KmtGetText(L"UIIT_KMT_MAGICAL_ATTACK_POWER");
            aaxx.Code = L"NATTR_MA";
            m_AttrList.push_back(aaxx);

            AttrStr aaxxc = AttrStr();
            aaxxc.Name = KmtGetText(L"UIIT_KMT_MAGICAL_REINFORCE");
            aaxxc.Code = L"NATTR_MAINT";
            m_AttrList.push_back(aaxxc);

            AttrStr aaxxcc = AttrStr();
            aaxxcc.Name = KmtGetText(L"UIIT_KMT_ATTACK_RATE");
            aaxxcc.Code = L"NATTR_HR";
            m_AttrList.push_back(aaxxcc);

            AttrStr aaxxccc = AttrStr();
            aaxxccc.Name = KmtGetText(L"UIIT_KMT_CRITICAL");
            aaxxccc.Code = L"NATTR_CRITICAL";
            m_AttrList.push_back(aaxxccc);

            LoadAttrList();
        }
        else if(data->m_typeId.getTypeID4() == 7 || data->m_typeId.getTypeID4() == 8 || data->m_typeId.getTypeID4() == 9
        || data->m_typeId.getTypeID4() == 12 || data->m_typeId.getTypeID4() == 13)
        {
            AttrStr aa = AttrStr();
            aa.Name = KmtGetText(L"UIIT_KMT_PHYSICAL_ATTACK_POWER");
            aa.Code = L"NATTR_PA";
            m_AttrList.push_back(aa);

            AttrStr aax = AttrStr();
            aax.Name = KmtGetText(L"UIIT_KMT_PHYSICAL_REINFORCE");
            aax.Code = L"NATTR_PASTR";
            m_AttrList.push_back(aax);


            AttrStr aaxxcc = AttrStr();
            aaxxcc.Name = KmtGetText(L"UIIT_KMT_ATTACK_RATE");
            aaxxcc.Code = L"NATTR_HR";
            m_AttrList.push_back(aaxxcc);

            AttrStr aaxxccc = AttrStr();
            aaxxccc.Name = KmtGetText(L"UIIT_KMT_CRITICAL");
            aaxxccc.Code = L"NATTR_CRITICAL";
            m_AttrList.push_back(aaxxccc);

            LoadAttrList();
        }
        else if(data->m_typeId.getTypeID4() == 10 || data->m_typeId.getTypeID4() == 11 || data->m_typeId.getTypeID4() == 14
                || data->m_typeId.getTypeID4() == 15)
        {

            AttrStr aaxx = AttrStr();
            aaxx.Name = KmtGetText(L"UIIT_KMT_MAGICAL_ATTACK_POWER");
            aaxx.Code = L"NATTR_MA";
            m_AttrList.push_back(aaxx);

            AttrStr aaxxc = AttrStr();
            aaxxc.Name = KmtGetText(L"UIIT_KMT_MAGICAL_REINFORCE");
            aaxxc.Code = L"NATTR_MAINT";
            m_AttrList.push_back(aaxxc);

            AttrStr aaxxcc = AttrStr();
            aaxxcc.Name = KmtGetText(L"UIIT_KMT_ATTACK_RATE");
            aaxxcc.Code = L"NATTR_HR";
            m_AttrList.push_back(aaxxcc);


            LoadAttrList();
        }
    }
}

double CIFAlchemyMacro::round(double value) {
    return (value >= 0.0) ? std::floor(value + 0.5) : std::ceil(value - 0.5);
}

// MagAttack hesaplama fonksiyonu
float CIFAlchemyMacro::CalculateAttack(float minL, float minU, float maxL, float maxU, float plus, float inc, float currentMin, float currentMax) {
    // Plus * Inc işlemini currentMin ve currentMax'ten çıkarıyoruz
    currentMin -= plus * inc;
    currentMax -= plus * inc;

    float minRange = minU - minL;
    float maxRange = maxU - maxL;
    float calculatedMin = minRange == 0.0f ? (currentMin >= minU ? 100.0f : 0.0f) : round((currentMin - minL) / minRange * 100);
    float calculatedMax = maxRange == 0.0f ? (currentMax >= maxU ? 100.0f : 0.0f) : round((currentMax - maxL) / maxRange * 100);

    // Ortalama hesaplaması
    return (calculatedMin + calculatedMax) / 2;
}
float CIFAlchemyMacro::CalculateAttackSecond(float minL, float maxL, float plus, float inc, float currentValue) {
    // Plus * Inc işlemini currentValue'dan çıkarıyoruz
    currentValue -= plus * inc;

    // Yüzde hesaplaması
    float range = maxL - minL;
    float calculatedValue = range == 0.0f ? (currentValue >= maxL ? 100.0f : 0.0f) : round((currentValue - minL) / range * 100);

    return calculatedValue;
}
float CIFAlchemyMacro::CalculateAttackSecondNoPlus(float minL, float maxL, float currentValue) {
    // Plus * Inc işlemini currentValue'dan çıkarıyoruz
    // Yüzde hesaplaması
    float range = maxL - minL;
    float calculatedValue = range == 0.0f ? (currentValue >= maxL ? 100.0f : 0.0f) : round((currentValue - minL) / range * 100);

    return calculatedValue;
}

float CIFAlchemyMacro::CalculateNoPlus(float minL, float minU, float maxL, float maxU, float currentMin, float currentMax) {
    // Plus * Inc işlemini currentMin ve currentMax'ten çıkarıyoruz

    float minRange = minU - minL;
    float maxRange = maxU - maxL;
    float calculatedMin = minRange == 0.0f ? (currentMin >= minU ? 100.0f : 0.0f) : round((currentMin - minL) / minRange * 100);
    float calculatedMax = maxRange == 0.0f ? (currentMax >= maxU ? 100.0f : 0.0f) : round((currentMax - maxL) / maxRange * 100);

    // Ortalama hesaplaması
    return (calculatedMin + calculatedMax) / 2;
}
void CIFAlchemyMacro::ShieldAddedtoSlot()
{
    SelectedAttrCode.clear();
    SelectedRemoveAttrCode.clear();
    m_BlueList.clear();
    m_AttrList.clear();
    m_TargetAttrList.clear();
    ClearList();
    ClearDDJList();
    ClearListAttr();
    ClearDDJListAttr();
    if(CurrentPage == 1)
    {
        const SItemData * data = m_ItemSlot->m_pMySlot->m_pSlot->ItemInfo->GetItemData();
        int Degree = GetDegreeLevel(data->m_itemClass);
        for(std::map<unsigned __int32, CMagicOptionData *>::iterator it = g_CGlobalDataManager->m_magicOptionDataMap.begin();
            it != g_CGlobalDataManager->m_magicOptionDataMap.end(); it++)
        {
            if((it->second->AvailItemGroup1 == L"shield" && it->second->ReqClass1 == 1) ||
               (it->second->AvailItemGroup2 == L"shield" && it->second->ReqClass2 == 1)
               || (it->second->AvailItemGroup3 == L"shield" && it->second->ReqClass3 == 1)
               || (it->second->AvailItemGroup4 == L"shield" && it->second->ReqClass4 == 1))
            {
                if(it->second->MLevel == Degree)
                {
                    if(it->second->Param1 == 7566450)
                    {
                        MagStr aa = MagStr();
                        aa.MagID = it->first;
                        aa.Param4 = it->second->Param4;
                        aa.Code = KmtGetText(L"UIIT_KMT_STR");
                        aa.Param1 = it->second->Param1;
                        m_BlueList.push_back(aa);;
                    }
                    else if(it->second->Param1 == 6909556)
                    {
                        MagStr aa = MagStr();
                        aa.MagID = it->first;
                        aa.Param4 = it->second->Param4;
                        aa.Code = KmtGetText(L"UIIT_KMT_INT");
                        aa.Param1 = it->second->Param1;
                        m_BlueList.push_back(aa);
                    }
                    else if(it->second->Param1 == 1685418613)
                    {
                        MagStr aa = MagStr();
                        aa.MagID = it->first;
                        aa.Param4 = it->second->Param4;
                        aa.Code = KmtGetText(L"UIIT_KMT_DURABILITY");
                        aa.Param1 = it->second->Param1;
                        m_BlueList.push_back(aa);
                    }
                    else if(it->second->Param1 == 26738)
                    {
                        MagStr aa = MagStr();
                        aa.MagID = it->first;
                        aa.Param4 = it->second->Param4;
                        aa.Code = KmtGetText(L"UIIT_KMT_ATTACK_RATE");
                        aa.Param1 = it->second->Param1;
                        m_BlueList.push_back(aa);
                    }
                  /*  else if(it->second->Param1 == 1702257260)
                    {
                        MagStr aa = MagStr();
                        aa.Code = KmtGetText(L"UIIT_KMT_BLOCK_RATE");
                        aa.Param1 = it->second->Param5;
                        m_BlueList.push_back(aa);
                    }*/
                      else if(it->second->Param1 == 1702257522)
                       {
                           MagStr aa = MagStr();
                           aa.MagID = it->first;
                           aa.Param4 = it->second->Param4;
                           aa.Code = KmtGetText(L"UIIT_KMT_CRITICAL");
                           aa.Param1 = it->second->Param1;
                           m_BlueList.push_back(aa);
                       }
                }

            }
        }
        LoadBlueList();
    }
    else if(CurrentPage == 2)
    {
        AttrStr aaxx = AttrStr();
        aaxx.Name = KmtGetText(L"UIIT_KMT_BLOCKING_RATE");
        aaxx.Code = L"NATTR_BR";
        m_AttrList.push_back(aaxx);

        AttrStr aaxxc = AttrStr();
        aaxxc.Name = KmtGetText(L"UIIT_KMT_MAGICAL_DEFENSE");
        aaxxc.Code = L"NATTR_MD";
        m_AttrList.push_back(aaxxc);

        AttrStr aaxxcc = AttrStr();
        aaxxcc.Name = KmtGetText(L"UIIT_KMT_MAGICAL_REINFORCE");
        aaxxcc.Code = L"NATTR_MDINT";
        m_AttrList.push_back(aaxxcc);

        AttrStr aaxxccx = AttrStr();
        aaxxccx.Name = KmtGetText(L"UIIT_KMT_PHYSICAL_DEFENSE");
        aaxxccx.Code = L"NATTR_PD";
        m_AttrList.push_back(aaxxccx);

        AttrStr aaxxcccc = AttrStr();
        aaxxcccc.Name = KmtGetText(L"UIIT_KMT_PHYSICAL_REINFORCE");
        aaxxcccc.Code = L"NATTR_PDSTR";
        m_AttrList.push_back(aaxxcccc);

        LoadAttrList();
    }
}

void CIFAlchemyMacro::ShoulderHandsFootAddedtoSlot()
{
    SelectedAttrCode.clear();
    SelectedRemoveAttrCode.clear();
    m_BlueList.clear();
    m_AttrList.clear();
    m_TargetAttrList.clear();
    ClearList();
    ClearDDJList();
    ClearListAttr();
    ClearDDJListAttr();
    if(CurrentPage == 1)
    {
        const SItemData * data = m_ItemSlot->m_pMySlot->m_pSlot->ItemInfo->GetItemData();
        int Degree = GetDegreeLevel(data->m_itemClass);
        for(std::map<unsigned __int32, CMagicOptionData *>::iterator it = g_CGlobalDataManager->m_magicOptionDataMap.begin();
            it != g_CGlobalDataManager->m_magicOptionDataMap.end(); it++)
        {
            if((it->second->AvailItemGroup1 == L"armor" && it->second->ReqClass1 == 1)
               || (it->second->AvailItemGroup2 == L"armor" && it->second->ReqClass2 == 1)
               || (it->second->AvailItemGroup3 == L"armor" && it->second->ReqClass3 == 1)
               || (it->second->AvailItemGroup4 == L"armor" && it->second->ReqClass4 == 1))
            {
                if(it->second->MLevel == Degree)
                {
                    if(it->second->Param1 == 7566450)
                    {
                        MagStr aa = MagStr();
                        aa.MagID = it->first;
                        aa.Param4 = it->second->Param4;
                        aa.Code = KmtGetText(L"UIIT_KMT_STR");
                        aa.Param1 = it->second->Param1;
                        m_BlueList.push_back(aa);;
                    }
                    else if(it->second->Param1 == 6909556)
                    {
                        MagStr aa = MagStr();
                        aa.MagID = it->first;
                        aa.Param4 = it->second->Param4;
                        aa.Code = KmtGetText(L"UIIT_KMT_INT");
                        aa.Param1 = it->second->Param1;
                        m_BlueList.push_back(aa);
                    }
                    else if(it->second->Param1 == 1685418613)
                    {
                        MagStr aa = MagStr();
                        aa.MagID = it->first;
                        aa.Param4 = it->second->Param4;
                        aa.Code = KmtGetText(L"UIIT_KMT_DURABILITY");
                        aa.Param1 = it->second->Param1;
                        m_BlueList.push_back(aa);
                    }
                    else if(it->second->Param1 == 25970)
                    {
                        MagStr aa = MagStr();
                        aa.MagID = it->first;
                        aa.Param4 = it->second->Param4;
                        aa.Code = KmtGetText(L"UIIT_KMT_PARRY_RATE");
                        aa.Param1 = it->second->Param1;
                        m_BlueList.push_back(aa);
                    }
                }
            }
        }
        LoadBlueList();
    }
    else if(CurrentPage == 2)
    {
        AttrStr aaxx = AttrStr();
        aaxx.Name = KmtGetText(L"UIIT_KMT_PARRY_RATE");
        aaxx.Code = L"NATTR_ER";
        m_AttrList.push_back(aaxx);

        AttrStr aaxxc = AttrStr();
        aaxxc.Name = KmtGetText(L"UIIT_KMT_MAGICAL_DEFENSE");
        aaxxc.Code = L"NATTR_MD";
        m_AttrList.push_back(aaxxc);

        AttrStr aaxxcc = AttrStr();
        aaxxcc.Name = KmtGetText(L"UIIT_KMT_MAGICAL_REINFORCE");
        aaxxcc.Code = L"NATTR_MDINT";
        m_AttrList.push_back(aaxxcc);

        AttrStr aaxxccx = AttrStr();
        aaxxccx.Name = KmtGetText(L"UIIT_KMT_PHYSICAL_DEFENSE");
        aaxxccx.Code = L"NATTR_PD";
        m_AttrList.push_back(aaxxccx);

        AttrStr aaxxcccc = AttrStr();
        aaxxcccc.Name = KmtGetText(L"UIIT_KMT_PHYSICAL_REINFORCE");
        aaxxcccc.Code = L"NATTR_PDSTR";
        m_AttrList.push_back(aaxxcccc);

        LoadAttrList();
    }
}
void CIFAlchemyMacro::HeadChestLegsAddedtoSlot()
{
    SelectedAttrCode.clear();
    SelectedRemoveAttrCode.clear();
    m_BlueList.clear();
    m_AttrList.clear();
    m_TargetAttrList.clear();
    ClearList();
    ClearDDJList();
    ClearListAttr();
    ClearDDJListAttr();
    if(CurrentPage == 1)
    {
        const SItemData * data = m_ItemSlot->m_pMySlot->m_pSlot->ItemInfo->GetItemData();
        int Degree = GetDegreeLevel(data->m_itemClass);
        for(std::map<unsigned __int32, CMagicOptionData *>::iterator it = g_CGlobalDataManager->m_magicOptionDataMap.begin();
            it != g_CGlobalDataManager->m_magicOptionDataMap.end(); it++)
        {
            if((it->second->AvailItemGroup1 == L"armor" && it->second->ReqClass1 == 1)
            || (it->second->AvailItemGroup2 == L"armor" && it->second->ReqClass2 == 1)
            || (it->second->AvailItemGroup3 == L"armor" && it->second->ReqClass3 == 1)
            || (it->second->AvailItemGroup4 == L"armor" && it->second->ReqClass4 == 1)
            ||
            (it->second->AvailItemGroup1 == L"helm" && it->second->ReqClass1 == 1) ||
            (it->second->AvailItemGroup2 == L"mail" && it->second->ReqClass2 == 1) ||
            (it->second->AvailItemGroup3 == L"pants" && it->second->ReqClass3 == 1))
            {
                if(it->second->MLevel == Degree)
                {
                    if(it->second->Param1 == 7566450)
                    {
                        MagStr aa = MagStr();
                        aa.MagID = it->first;
                        aa.Param4 = it->second->Param4;
                        aa.Code = KmtGetText(L"UIIT_KMT_STR");
                        aa.Param1 = it->second->Param1;
                        m_BlueList.push_back(aa);;
                    }
                    else if(it->second->Param1 == 6909556)
                    {
                        MagStr aa = MagStr();
                        aa.MagID = it->first;
                        aa.Param4 = it->second->Param4;
                        aa.Code = KmtGetText(L"UIIT_KMT_INT");
                        aa.Param1 = it->second->Param1;
                        m_BlueList.push_back(aa);
                    }
                    else if(it->second->Param1 == 1685418613)
                    {
                        MagStr aa = MagStr();
                        aa.MagID = it->first;
                        aa.Param4 = it->second->Param4;
                        aa.Code = KmtGetText(L"UIIT_KMT_DURABILITY");
                        aa.Param1 = it->second->Param1;
                        m_BlueList.push_back(aa);
                    }
                    else if(it->second->Param1 == 25970)
                    {
                        MagStr aa = MagStr();
                        aa.MagID = it->first;
                        aa.Param4 = it->second->Param4;
                        aa.Code = KmtGetText(L"UIIT_KMT_PARRY_RATE");
                        aa.Param1 = it->second->Param1;
                        m_BlueList.push_back(aa);
                    }
                    else if(it->second->Param1 == 26736)
                    {
                        MagStr aa = MagStr();
                        aa.MagID = it->first;
                        aa.Param4 = it->second->Param4;
                        aa.Code = KmtGetText(L"UIIT_KMT_HP");
                        aa.Param1 = it->second->Param1;
                        m_BlueList.push_back(aa);
                    }
                    else if(it->second->Param1 == 28016)
                    {
                        MagStr aa = MagStr();
                        aa.MagID = it->first;
                        aa.Param4 = it->second->Param4;
                        aa.Code = KmtGetText(L"UIIT_KMT_MP");
                        aa.Param1 = it->second->Param1;
                        m_BlueList.push_back(aa);
                    }
                }
            }
        }
        LoadBlueList();
    }
    else if(CurrentPage == 2)
    {
        AttrStr aaxx = AttrStr();
        aaxx.Name = KmtGetText(L"UIIT_KMT_PARRY_RATE");
        aaxx.Code = L"NATTR_ER";
        m_AttrList.push_back(aaxx);

        AttrStr aaxxc = AttrStr();
        aaxxc.Name = KmtGetText(L"UIIT_KMT_MAGICAL_DEFENSE");
        aaxxc.Code = L"NATTR_MD";
        m_AttrList.push_back(aaxxc);

        AttrStr aaxxcc = AttrStr();
        aaxxcc.Name = KmtGetText(L"UIIT_KMT_MAGICAL_REINFORCE");
        aaxxcc.Code = L"NATTR_MDINT";
        m_AttrList.push_back(aaxxcc);

        AttrStr aaxxccx = AttrStr();
        aaxxccx.Name = KmtGetText(L"UIIT_KMT_PHYSICAL_DEFENSE");
        aaxxccx.Code = L"NATTR_PD";
        m_AttrList.push_back(aaxxccx);

        AttrStr aaxxcccc = AttrStr();
        aaxxcccc.Name = KmtGetText(L"UIIT_KMT_PHYSICAL_REINFORCE");
        aaxxcccc.Code = L"NATTR_PDSTR";
        m_AttrList.push_back(aaxxcccc);

        LoadAttrList();
    }
}

void CIFAlchemyMacro::AccAddedtoSlot()
{
    SelectedAttrCode.clear();
    SelectedRemoveAttrCode.clear();
    m_BlueList.clear();
    m_AttrList.clear();
    m_TargetAttrList.clear();
    ClearList();
    ClearDDJList();
    ClearListAttr();
    ClearDDJListAttr();
    if(CurrentPage == 1)
    {
        const SItemData * data = m_ItemSlot->m_pMySlot->m_pSlot->ItemInfo->GetItemData();
        int Degree = GetDegreeLevel(data->m_itemClass);
        for(std::map<unsigned __int32, CMagicOptionData *>::iterator it = g_CGlobalDataManager->m_magicOptionDataMap.begin();
            it != g_CGlobalDataManager->m_magicOptionDataMap.end(); it++)
        {
            if((it->second->AvailItemGroup1 == L"accessory" && it->second->ReqClass1 == 1) ||
               (it->second->AvailItemGroup2 == L"accessory" && it->second->ReqClass2 == 1)
               || (it->second->AvailItemGroup3 == L"accessory" && it->second->ReqClass3 == 1)
               || (it->second->AvailItemGroup4 == L"accessory" && it->second->ReqClass4 == 1))
            {
                if(it->second->MLevel == Degree)
                {
                    if(it->second->Param1 == 7566450)
                    {
                        MagStr aa = MagStr();
                        aa.MagID = it->first;
                        aa.Param4 = it->second->Param4;
                        aa.Code = KmtGetText(L"UIIT_KMT_STR");
                        aa.Param1 = it->second->Param1;
                        m_BlueList.push_back(aa);;
                    }
                    else if(it->second->Param1 == 6909556)
                    {
                        MagStr aa = MagStr();
                        aa.MagID = it->first;
                        aa.Param4 = it->second->Param4;
                        aa.Code = KmtGetText(L"UIIT_KMT_INT");
                        aa.Param1 = it->second->Param1;
                        m_BlueList.push_back(aa);
                    }
                    else if(it->second->Param1 == 26234)
                    {
                        MagStr aa = MagStr();
                        aa.MagID = it->first;
                        aa.Param4 = it->second->Param4;
                        aa.Code = KmtGetText(L"UIIT_KMT_FROZEN");
                        aa.Param1 = it->second->Param1;
                        m_BlueList.push_back(aa);
                    }
                    else if(it->second->Param1 == 25971)
                    {
                        MagStr aa = MagStr();
                        aa.MagID = it->first;
                        aa.Param4 = it->second->Param4;
                        aa.Code = KmtGetText(L"UIIT_KMT_ELECTRIC");
                        aa.Param1 = it->second->Param1;
                        m_BlueList.push_back(aa);
                    }
                    else if(it->second->Param1 == 28787)
                    {
                        MagStr aa = MagStr();
                        aa.MagID = it->first;
                        aa.Param4 = it->second->Param4;
                        aa.Code = KmtGetText(L"UIIT_KMT_BURN");
                        aa.Param1 = it->second->Param1;
                        m_BlueList.push_back(aa);
                    }
                    else if(it->second->Param1 == 25205)
                    {
                        MagStr aa = MagStr();
                        aa.MagID = it->first;
                        aa.Param4 = it->second->Param4;
                        aa.Code = KmtGetText(L"UIIT_KMT_POISON");
                        aa.Param1 = it->second->Param1;
                        m_BlueList.push_back(aa);
                    }
                    else if(it->second->Param1 == 31330)
                    {
                        MagStr aa = MagStr();
                        aa.MagID = it->first;
                        aa.Param4 = it->second->Param4;
                        aa.Code = KmtGetText(L"UIIT_KMT_ZOMBIE");
                        aa.Param1 = it->second->Param1;
                        m_BlueList.push_back(aa);
                    }
                }

            }
        }
        LoadBlueList();
    }
    else if(CurrentPage == 2)
    {
        AttrStr aaxx = AttrStr();
        aaxx.Name = KmtGetText(L"UIIT_KMT_PHY_ABSORTION");
        aaxx.Code = L"NATTR_PAR";
        m_AttrList.push_back(aaxx);

        AttrStr aaxxc = AttrStr();
        aaxxc.Name = KmtGetText(L"UIIT_KMT_MAGICAL_ABSORTION");
        aaxxc.Code = L"NATTR_MAR";
        m_AttrList.push_back(aaxxc);

        LoadAttrList();
    }
}
void CIFAlchemyMacro::LoadAttrList()
{
    ClearList();
    ClearDDJList();
    int i = 0;
    for (std::vector<AttrStr>::iterator it = m_AttrList.begin(); it != m_AttrList.end(); ++it)
    {
        if(i < 11)
        {
            m_BlueSlots[i]->LoadItems2(it->Code.c_str(), it->Name.c_str());
            m_scroll->AddItem(m_BlueSlots[i], 1, 0);
            ++i;
        }
    }
}
void CIFAlchemyMacro::LoadBlueList()
{
    ClearList();
    ClearDDJList();
    int i = 0;
    for (std::vector<MagStr>::iterator it = m_BlueList.begin(); it != m_BlueList.end(); ++it)
    {
      if(i < 11)
      {
          m_BlueSlots[i]->LoadItems(it->Param1, it->Code.c_str());
          m_scroll->AddItem(m_BlueSlots[i], 1, 0);
          ++i;
      }
    }
}
void CIFAlchemyMacro::ClearDDJList() {

    for (int i = 0; i < 11; ++i)
    {
        m_BlueSlots[i]->ClearDDJ();
    }
}
void CIFAlchemyMacro::ClearDDJListAttr() {

    for (int i = 0; i < 11; ++i)
    {
        m_AttrSlots[i]->ClearDDJ();
    }
}
void CIFAlchemyMacro::ClearList()
{
    for (int i = 0; i < 11; ++i)
    {
        m_scroll->DeleteItem(m_BlueSlots[i]);
        m_BlueSlots[i]->Clear();
    }
}
void CIFAlchemyMacro::ClearListAttr()
{
    for (int i = 0; i < 11; ++i)
    {
        m_scrollattr->DeleteItem(m_AttrSlots[i]);
        m_AttrSlots[i]->Clear();
    }
}
void CIFAlchemyMacro::LUCKY_UP_BUTTON()
{
    if(LuckyMinPlus+1 > 12)
        return;
    LuckyMinPlus++;

    wchar_t Priceb[255];
    swprintf_s(Priceb, L"+%d", LuckyMinPlus);
    LuckyMinPlusLabel->SetText(Priceb);
}
void CIFAlchemyMacro::LUCKY_DOWN_BUTTON()
{
    if(LuckyMinPlus-1 < 0)
        return;
    LuckyMinPlus--;

    wchar_t Priceb[255];
    swprintf_s(Priceb, L"+%d", LuckyMinPlus);
    LuckyMinPlusLabel->SetText(Priceb);
}
void CIFAlchemyMacro::STEADY_UP_BUTTON()
{
    if(SteadyMinPlus+1 > 12)
        return;
    SteadyMinPlus++;

    wchar_t Priceb[255];
    swprintf_s(Priceb, L"+%d", SteadyMinPlus);
    SteadyMinPlusLabel->SetText(Priceb);
}
void CIFAlchemyMacro::STEADY_DOWN_BUTTON()
{
    if(SteadyMinPlus-1 < 0)
        return;
    SteadyMinPlus--;

    wchar_t Priceb[255];
    swprintf_s(Priceb, L"+%d", SteadyMinPlus);
    SteadyMinPlusLabel->SetText(Priceb);
}

void CIFAlchemyMacro::ASTRAL_UP_BUTTON()
{
    if(AstralMinPlus+1 > 12)
        return;
    AstralMinPlus++;

    wchar_t Priceb[255];
    swprintf_s(Priceb, L"+%d", AstralMinPlus);
    AstralMinPlusLabel->SetText(Priceb);
}
void CIFAlchemyMacro::ASTRAL_DOWN_BUTTON()
{
    if(AstralMinPlus-1 < 0)
        return;
    AstralMinPlus--;

    wchar_t Priceb[255];
    swprintf_s(Priceb, L"+%d", AstralMinPlus);
    AstralMinPlusLabel->SetText(Priceb);
}

void CIFAlchemyMacro::IMMORTAL_UP_BUTTON()
{
    if(ImmortalMinPlus+1 > 12)
        return;
    ImmortalMinPlus++;

    wchar_t Priceb[255];
    swprintf_s(Priceb, L"+%d", ImmortalMinPlus);
    ImmortalMinPlusLabel->SetText(Priceb);
}
void CIFAlchemyMacro::IMMORTAL_DOWN_BUTTON()
{
    if(ImmortalMinPlus-1 < 0)
        return;
    ImmortalMinPlus--;

    wchar_t Priceb[255];
    swprintf_s(Priceb, L"+%d", ImmortalMinPlus);
    ImmortalMinPlusLabel->SetText(Priceb);
}
void CIFAlchemyMacro::LUCKYPOWDER_UP_BUTTON()
{
    if(LuckyPowderMinPlus+1 > 12)
        return;
    LuckyPowderMinPlus++;

    wchar_t Priceb[255];
    swprintf_s(Priceb, L"+%d", LuckyPowderMinPlus);
    LuckyPowderMinPlusLabel->SetText(Priceb);
}
void CIFAlchemyMacro::LUCKYPOWDER_DOWN_BUTTON()
{
    if(LuckyPowderMinPlus-1 < 0)
        return;
    LuckyPowderMinPlus--;

    wchar_t Priceb[255];
    swprintf_s(Priceb, L"+%d", LuckyPowderMinPlus);
    LuckyPowderMinPlusLabel->SetText(Priceb);
}
void CIFAlchemyMacro::PLUS_UP_BUTTON()
{
    if(TargetPlus+1 > 12)
    return;
    TargetPlus++;

    wchar_t Priceb[255];
    swprintf_s(Priceb, L"+%d", TargetPlus);
    TargetPlusLabel->SetText(Priceb);
}
void CIFAlchemyMacro::PLUS_DOWN_BUTTON()
{
    if(TargetPlus-1 < 1)
        return;
    TargetPlus--;

    wchar_t Priceb[255];
    swprintf_s(Priceb, L"+%d", TargetPlus);
    TargetPlusLabel->SetText(Priceb);
}
#define EffectTimer 2
#define FusePlusTimer 3
#define ActionStartedTimer 4
#define FuseBlueTimer 5
#define FuseAttrTimer 6
#define AutomationPulseInterval 250
#define AlchemyResponseTimeout 12000

bool CIFAlchemyMacro::HasTrackedItem() const
{
    return m_ItemSlot != NULL &&
           m_ItemSlot->m_pMySlot != NULL &&
           m_ItemSlot->m_pMySlot->m_pSlot != NULL &&
           m_ItemSlot->m_pMySlot->m_pSlot->ItemInfo != NULL &&
           m_ItemSlot->m_pMySlot->m_pSlot->ItemInfo->GetItemData() != NULL;
}

void CIFAlchemyMacro::StopAutomation(const wchar_t* reason)
{
    AutomationRunning = false;
    Fusing = false;
    Slotdeco->ShowGWnd(false);
    this->KillTimer(FusePlusTimer);
    this->KillTimer(ActionStartedTimer);
    this->KillTimer(EffectTimer);
    this->KillTimer(FuseBlueTimer);
    this->KillTimer(FuseAttrTimer);

    if (reason != NULL && reason[0] != L'\0')
    {
        D3DCOLOR color = D3DCOLOR_ARGB(255, 255, 255, 0);
        g_pCGInterface->GetSystemMessageView()->WriteMessage(255, color, reason, 1, 1);
    }
}

void CIFAlchemyMacro::StartOperationTimeout()
{
    this->KillTimer(ActionStartedTimer);
    this->StartTimer(ActionStartedTimer, AlchemyResponseTimeout);
}

void CIFAlchemyMacro::OnAlchemyResponse()
{
    if (!AutomationRunning || !Fusing)
        return;

    Fusing = false;
    this->KillTimer(ActionStartedTimer);
}

void CIFAlchemyMacro::OnTrackedItemRemoved()
{
    if (AutomationRunning || Fusing)
        StopAutomation(KmtGetText(L"UIIT_KMT_THE_SELECTED_ITEM_WAS_REMOVED_AUTO_ALCHEMY_STOPPED"));
}

undefined1 CIFAlchemyMacro::OnCloseWnd(){
    StopAutomation();
    if (m_ItemSlot != NULL && m_ItemSlot->m_pMySlot != NULL &&
        m_ItemSlot->m_pMySlot->m_pSlot != NULL)
    {
        m_ItemSlot->m_pMySlot->m_pSlot->ClearSlot();
    }
    m_TargetAttrList.clear();
    g_pCGInterface->UnLockMovement();

    return CIFMainFrame::OnCloseWnd();
}
void CIFAlchemyMacro::AutoPlusPage()
{
    CurrentPage = 0;
    for(int i = 8; i < 37; i++)
    {
        m_IRM.GetResObj(i, 1)->ShowGWnd(true);
    }
    this->m_ItemSlot->m_pMySlot->m_pSlot->ClearSlot();

    for(int i = 0; i < 11; i++)
    {
        m_BlueSlots[i]->ShowGWnd(false);
    }
    for(int i = 40; i < 51; i++)
    {
        m_IRM.GetResObj(i, 1)->ShowGWnd(false);
    }
    for(int i = 51; i < 63; i++)
    {
        m_IRM.GetResObj(i, 1)->ShowGWnd(false);
    }

    m_BlueList.clear();
    m_AttrList.clear();
    m_TargetAttrList.clear();
    ClearList();
    ClearDDJList();
    ClearListAttr();
    ClearDDJListAttr();
    StopAutomation();
}
void CIFAlchemyMacro::AutoBluePage()
{
    this->m_IRM.GetResObj<CIFTextBox>(50, 1)->SetText(L"\nAuto-Blue function is only fuse to max values. Its cannot be set custom values");

    CurrentPage = 1;
    for(int i = 8; i < 37; i++)
    {
        m_IRM.GetResObj(i, 1)->ShowGWnd(false);
    }

    this->m_ItemSlot->m_pMySlot->m_pSlot->ClearSlot();

    for(int i = 0; i < 11; i++)
    {
        m_BlueSlots[i]->ShowGWnd(false);
    }
    for(int i = 40; i < 51; i++)
    {
        m_IRM.GetResObj(i, 1)->ShowGWnd(true);
    }

    for(int i = 51; i < 63; i++)
    {
        m_IRM.GetResObj(i, 1)->ShowGWnd(false);
    }

    m_BlueList.clear();
    m_AttrList.clear();
    m_TargetAttrList.clear();
    ClearList();
    ClearDDJList();
    ClearListAttr();
    ClearDDJListAttr();
    StopAutomation();
}

void CIFAlchemyMacro::AutoStatPage()
{
    CurrentPage = 2;
    for(int i = 8; i < 37; i++)
    {
        m_IRM.GetResObj(i, 1)->ShowGWnd(false);
    }
    this->m_ItemSlot->m_pMySlot->m_pSlot->ClearSlot();

    for(int i = 0; i < 11; i++)
    {
        m_BlueSlots[i]->ShowGWnd(false);
    }
    for(int i = 40; i < 51; i++)
    {
        m_IRM.GetResObj(i, 1)->ShowGWnd(true);
    }

    for(int i = 51; i < 63; i++)
    {
        m_IRM.GetResObj(i, 1)->ShowGWnd(true);
    }
    m_BlueList.clear();
    m_AttrList.clear();
    m_TargetAttrList.clear();
    ClearList();
    ClearDDJList();
    ClearListAttr();
    ClearDDJListAttr();
    StopAutomation();
}
void CIFAlchemyMacro::StopButton()
{
    StopAutomation(KmtGetText(L"UIIT_KMT_AUTO_ALCHEMY_STOPPED"));
}
void CIFAlchemyMacro::StartButton()
{
    if (AutomationRunning)
        return;

    if (!HasTrackedItem())
    {
        StopAutomation(KmtGetText(L"UIIT_KMT_PLEASE_PUT_AN_ITEM_IN_THE_SLOT"));
        return;
    }

    AutomationRunning = true;
    Fusing = false;
    Slotdeco->FUN_00634470N("clientlibrary\\menu\\alcm_effect_prepare_pre.ddj");
    Slotdeco->FUN_00633910(1);
    Slotdeco->FUN_00633950(100);
    Slotdeco->FUN_00633960(1);
    Slotdeco->FUN_00633b90(0x40, 0x40);
    Slotdeco->FUN_00633970(0x10);
    Slotdeco->FUN_00634330(0);
    Slotdeco->FUN_00633c70(0);
    Slotdeco->BringToFront();
    Slotdeco->ShowGWnd(true);
    this->StartTimer(EffectTimer, 50);

    if (CurrentPage == 0)
    {
        this->StartTimer(FusePlusTimer, AutomationPulseInterval);
        FusePlus();
    }
    else if (CurrentPage == 1)
    {
        this->StartTimer(FuseBlueTimer, AutomationPulseInterval);
        FuseBlueNew();
    }
    else
    {
        this->StartTimer(FuseAttrTimer, AutomationPulseInterval);
        FuseAttrStone();
    }
}
int CIFAlchemyMacro::FindAttrs(int i)
{
    int currentvalue = 0;
    const SItemData* data = m_ItemSlot->m_pMySlot->m_pSlot->ItemInfo->GetItemData();
    if (m_TargetAttrList[i].Code == L"NATTR_PA") {
        currentvalue = CalculateAttack(data->m_phyAttackMinMin, data->m_phyAttackMinMax,
                                       data->m_phyAttackMaxMin, data->m_phyAttackMaxMax,
                                       m_ItemSlot->m_pMySlot->m_pSlot->ItemInfo->m_OptLevel,
                                       data->m_phyAttackIncrease,
                                       m_ItemSlot->m_pMySlot->m_pSlot->ItemInfo->m_PhyAtkPwrMin,
                                       m_ItemSlot->m_pMySlot->m_pSlot->ItemInfo->m_PhyAtkPwrMax);
    }
    else if (m_TargetAttrList[i].Code == L"NATTR_MA") {
        currentvalue = CalculateAttack(data->m_magAttackMinMin, data->m_magAttackMinMax,
                                       data->m_magAttackMaxMin, data->m_magAttackMaxMax,
                                       m_ItemSlot->m_pMySlot->m_pSlot->ItemInfo->m_OptLevel,
                                       data->m_magAttackIncrease,
                                       m_ItemSlot->m_pMySlot->m_pSlot->ItemInfo->m_MagAtkPwrMin,
                                       m_ItemSlot->m_pMySlot->m_pSlot->ItemInfo->m_MagAtkPwrMax);
    }
    else if (m_TargetAttrList[i].Code == L"NATTR_PASTR") {
        currentvalue = CalculateNoPlus(data->m_phyAttackStrMinMin, data->m_phyAttackStrMinMax,
                                       data->m_phyAttackStrMaxMin, data->m_phyAttackStrMaxMax,
                                       m_ItemSlot->m_pMySlot->m_pSlot->ItemInfo->m_PhyReinforcementMin,
                                       m_ItemSlot->m_pMySlot->m_pSlot->ItemInfo->m_PhyReinforcementMax);
    }
    else if (m_TargetAttrList[i].Code == L"NATTR_MAINT") {
        currentvalue = CalculateNoPlus(data->m_magAttackIntMinMin, data->m_magAttackIntMinMax,
                                       data->m_magAttackIntMaxMin, data->m_magAttackIntMaxMax,
                                       m_ItemSlot->m_pMySlot->m_pSlot->ItemInfo->m_MagReinforcementMin,
                                       m_ItemSlot->m_pMySlot->m_pSlot->ItemInfo->m_MagReinforcementMax);
    }
    else if (m_TargetAttrList[i].Code == L"NATTR_HR") {
        currentvalue = CalculateAttackSecond(data->m_hitRateMin, data->m_hitRateMax,
                                             m_ItemSlot->m_pMySlot->m_pSlot->ItemInfo->m_OptLevel,
                                             data->m_hitRateIncrease,
                                             m_ItemSlot->m_pMySlot->m_pSlot->ItemInfo->m_AttackRateValue);
    }
    else if (m_TargetAttrList[i].Code == L"NATTR_CRITICAL") {
        currentvalue = CalculateAttackSecondNoPlus(data->m_criticalHitRateMin, data->m_criticalHitRateMax,
                                                   m_ItemSlot->m_pMySlot->m_pSlot->ItemInfo->m_CriticalValue);
    }
    else if (m_TargetAttrList[i].Code == L"NATTR_ER") {
        currentvalue = CalculateAttackSecond(data->m_evasionRatioMin, data->m_evasionRatioMax,
                                             m_ItemSlot->m_pMySlot->m_pSlot->ItemInfo->m_OptLevel,
                                             data->m_evasionRatioIncrease,
                                             m_ItemSlot->m_pMySlot->m_pSlot->ItemInfo->m_ParryRateValue);
    }
    else if (m_TargetAttrList[i].Code == L"NATTR_BR") {
        currentvalue = CalculateAttackSecondNoPlus(data->m_blockRatioMin, data->m_blockRatioMax,
                                                   m_ItemSlot->m_pMySlot->m_pSlot->ItemInfo->m_BlockingRateValue);
    }
    else if (m_TargetAttrList[i].Code == L"NATTR_MAR") {
        currentvalue = CalculateAttackSecond(data->m_MARMin, data->m_MARMax,
                                             m_ItemSlot->m_pMySlot->m_pSlot->ItemInfo->m_OptLevel,
                                             data->m_MARIncrease,
                                             m_ItemSlot->m_pMySlot->m_pSlot->ItemInfo->m_MagAbsorption);
    }
    else if (m_TargetAttrList[i].Code == L"NATTR_PAR") {
        currentvalue = CalculateAttackSecond(data->m_PARMin, data->m_PARMax,
                                             m_ItemSlot->m_pMySlot->m_pSlot->ItemInfo->m_OptLevel,
                                             data->m_PARIncrease,
                                             m_ItemSlot->m_pMySlot->m_pSlot->ItemInfo->m_PhyAbsorption);
    }
    else if (m_TargetAttrList[i].Code == L"NATTR_MD") {
        currentvalue = CalculateAttackSecond(data->m_magDefMin, data->m_magDefMax,
                                             m_ItemSlot->m_pMySlot->m_pSlot->ItemInfo->m_OptLevel,
                                             data->m_magDefIncrease,
                                             m_ItemSlot->m_pMySlot->m_pSlot->ItemInfo->m_MagDefPwrValue);
    }
    else if (m_TargetAttrList[i].Code == L"NATTR_PD") {
        currentvalue = CalculateAttackSecond(data->m_phyDefMin, data->m_phyDefMax,
                                             m_ItemSlot->m_pMySlot->m_pSlot->ItemInfo->m_OptLevel,
                                             data->m_phyDefIncrease,
                                             m_ItemSlot->m_pMySlot->m_pSlot->ItemInfo->m_PhyDefPwrValue);
    }
    else if (m_TargetAttrList[i].Code == L"NATTR_MDINT") {
        currentvalue = CalculateAttackSecondNoPlus(data->m_magDefIntMin, data->m_magDefIntMax,
                                                   m_ItemSlot->m_pMySlot->m_pSlot->ItemInfo->m_MagReinforcementValue);
    }
    else if (m_TargetAttrList[i].Code == L"NATTR_PDSTR") {
        currentvalue = CalculateAttackSecondNoPlus(data->m_phyDefStrMin, data->m_phyDefStrMax,
                                                   m_ItemSlot->m_pMySlot->m_pSlot->ItemInfo->m_PhyReinforcementValue);
    }
    return currentvalue;
}
void CIFAlchemyMacro::FuseAttrStone() {
    if (!AutomationRunning || Fusing) {
        return;
    }

    if (!HasTrackedItem()) {
        StopAutomation(KmtGetText(L"UIIT_KMT_THE_SELECTED_ITEM_IS_NO_LONGER_AVAILABLE_AUTO_ALCHEMY_STOPPED"));
        return;
    }

    if (m_TargetAttrList.empty()) {
        D3DCOLOR color = D3DCOLOR_ARGB(255, 255, 255, 0);
        g_pCGInterface->GetSystemMessageView()->WriteMessage(255, color, KmtGetText(L"UIIT_KMT_PLEASE_SELECT_TARGET_ATTRIBUTES"), 1, 1);
        StopAutomation(KmtGetText(L"UIIT_KMT_PLEASE_SELECT_TARGET_ATTRIBUTES"));
        return;
    }

    bool EksikStatYok = true;
    bool ValidStoneFound = false;

    // Eksik statları ve taşları kontrol et
    for (size_t i = 0; i < m_TargetAttrList.size(); ++i) {
        const SItemData* data = m_ItemSlot->m_pMySlot->m_pSlot->ItemInfo->GetItemData();
        int maxvalue = m_TargetAttrList[i].Target;
        int currentvalue = FindAttrs(i);

        if (currentvalue < maxvalue) {
            EksikStatYok = false;
            for (size_t j = 0; j < m_TargetAttrList.size(); ++j) {
                if (m_TargetAttrList[i].Code == m_TargetAttrList[j].Code) {
                    int stoneslot = FindAttrStone(m_TargetAttrList[j].Code.c_str());
                    if (stoneslot != -1) {
                        ValidStoneFound = true;
                        // Taşı bulduğumuzda işlemi başlat
                        Fusing = true;
                        StartOperationTimeout();
                        CMsgStreamBuffer buf(0x7151);
                        buf << (byte)0x2;
                        buf << (byte)0x5;
                        buf << (byte)0x2;

                        buf << static_cast<unsigned char>(m_ItemSlot->m_pMySlot->m_pSlot->GetInventorySlotIndex() + 13u);
                        buf << static_cast<unsigned char>(stoneslot + 13u);
                        SendMsg(buf);
                        return; // İşlem yapıldı, fonksiyondan çık
                    }
                }
            }
        }
    }

    // Eksik stat yoksa, işlemi tamamla
    if (EksikStatYok) {
        StopAutomation(KmtGetText(L"UIIT_KMT_JOB_IS_COMPLETED"));
        return;
    }

    // Geçerli taş bulunamadıysa
    if (!ValidStoneFound) {
        StopAutomation(KmtGetText(L"UIIT_KMT_CHECK_ATTRIBUTE_STONES"));
    }
}





/*void CIFAlchemyMacro::FuseBlueNew()
{
    if (Fusing)
        return;

    if (m_BlueList.size() == 0)
    {
        D3DCOLOR color = D3DCOLOR_ARGB(255, 255, 255, 0);
        g_pCGInterface->GetSystemMessageView()->WriteMessage(255, color, KmtGetText(L"UIIT_KMT_PLEASE_SELECT_TARGET_BLUES"), 1, 1);
        Fusing = false;
        Slotdeco->ShowGWnd(false);
        this->KillTimer(FuseBlueTimer);
        this->KillTimer(ActionStartedTimer);
        this->KillTimer(EffectTimer);
        return;
    }
    else
    {
        bool foundValidStone = false;
        bool allStonesEqual = true;

        for (std::vector<MagStr>::iterator iter = m_BlueList.begin(); iter != m_BlueList.end(); iter++)
        {
            const SItemData* data = m_ItemSlot->m_pMySlot->m_pSlot->ItemInfo->GetItemData();
            int Degree = GetDegreeLevel(data->m_itemClass);

            std::map<unsigned __int32, CMagicOptionData*>::iterator it = g_CGlobalDataManager->m_magicOptionDataMap.find(iter->MagID);

            if (it != g_CGlobalDataManager->m_magicOptionDataMap.end() && it->second->MLevel == Degree)
            {
                if (m_ItemSlot->m_pMySlot->m_pSlot->ItemInfo->bluemap.find(static_cast<const Blue>(it->first)) ==
                    m_ItemSlot->m_pMySlot->m_pSlot->ItemInfo->bluemap.end())
                {
                    int stoneslot = FindBlueStone(it->second->MOptName128.c_str());
                    if (stoneslot == -1) // stone yoksa devam et
                    {
                        continue;
                    }

                    foundValidStone = true;
                    Fusing = true;
                    this->StartTimer(ActionStartedTimer, 4000);
                    CMsgStreamBuffer buf(0x7151);
                    buf << (byte)0x2;
                    buf << (byte)0x4;
                    buf << (byte)0x2;

                    buf << static_cast<unsigned char>(m_ItemSlot->m_pMySlot->m_pSlot->GetInventorySlotIndex() + 13u);
                    buf << static_cast<unsigned char>(stoneslot + 13u);
                    SendMsg(buf);
                    return; // İşlem yapıldı, fonksiyondan çık
                }
                else
                {
                    int maxvalue = getDecimalFromLast4HexDigits(iter->Param4);
                    int currentvalue = m_ItemSlot->m_pMySlot->m_pSlot->ItemInfo->bluemap[static_cast<const Blue>(it->first)];
                    if (currentvalue < maxvalue)
                    {
                        allStonesEqual = false;
                        int stoneslot = FindBlueStone(it->second->MOptName128.c_str());
                        if (stoneslot == -1) // stone yoksa devam et
                        {
                            continue;
                        }

                        foundValidStone = true;
                        Fusing = true;
                        this->StartTimer(ActionStartedTimer, 4000);
                        CMsgStreamBuffer buf(0x7151);
                        buf << (byte)0x2;
                        buf << (byte)0x4;
                        buf << (byte)0x2;

                        buf << static_cast<unsigned char>(m_ItemSlot->m_pMySlot->m_pSlot->GetInventorySlotIndex() + 13u);
                        buf << static_cast<unsigned char>(stoneslot + 13u);
                        SendMsg(buf);
                        return; // İşlem yapıldı, fonksiyondan çık
                    }
                    else if (currentvalue == maxvalue)
                    {
                        continue; // Eşitse döngüye devam et
                    }
                }
            }
        }

        if (!foundValidStone && allStonesEqual)
        {
            // Eğer tüm stone değerleri eşit ise ve geçerli taş yoksa, timer'ı durdur
            this->KillTimer(FuseBlueTimer);
            this->KillTimer(ActionStartedTimer);
            this->KillTimer(EffectTimer);
            Fusing = false;
            Slotdeco->ShowGWnd(false);
            D3DCOLOR color = D3DCOLOR_ARGB(255, 255, 255, 0);
            g_pCGInterface->GetSystemMessageView()->WriteMessage(255, color, KmtGetText(L"UIIT_KMT_PLEASE_CHECK_YOUR_ITEM_OR_STONES"), 1, 1);
        }
    }
}
*/
void CIFAlchemyMacro::FuseBlueNew() {
    if (!AutomationRunning || Fusing) {
        return;
    }

    if (!HasTrackedItem()) {
        StopAutomation(KmtGetText(L"UIIT_KMT_THE_SELECTED_ITEM_IS_NO_LONGER_AVAILABLE_AUTO_ALCHEMY_STOPPED"));
        return;
    }

    if (m_BlueList.empty()) {
        D3DCOLOR color = D3DCOLOR_ARGB(255, 255, 255, 0);
        g_pCGInterface->GetSystemMessageView()->WriteMessage(255, color, KmtGetText(L"UIIT_KMT_PLEASE_SELECT_TARGET_BLUES"), 1, 1);
        StopAutomation(KmtGetText(L"UIIT_KMT_PLEASE_SELECT_TARGET_BLUES"));
        return;
    }

    bool allTargetsMet = true;
    bool validStoneFound = false;

    for (std::vector<MagStr>::iterator iter = m_BlueList.begin(); iter != m_BlueList.end(); ++iter) {
        const SItemData* data = m_ItemSlot->m_pMySlot->m_pSlot->ItemInfo->GetItemData();
        int Degree = GetDegreeLevel(data->m_itemClass);
        int maxvalue = getDecimalFromLast4HexDigits(iter->Param4);
        int currentvalue = 0;

        std::map<unsigned __int32, CMagicOptionData*>::iterator it = g_CGlobalDataManager->m_magicOptionDataMap.find(iter->MagID);

        if (it != g_CGlobalDataManager->m_magicOptionDataMap.end() && it->second->MLevel == Degree) {
            if (m_ItemSlot->m_pMySlot->m_pSlot->ItemInfo->bluemap.find(static_cast<const Blue>(it->first)) == m_ItemSlot->m_pMySlot->m_pSlot->ItemInfo->bluemap.end()) {
                currentvalue = 0;
            } else {
                currentvalue = m_ItemSlot->m_pMySlot->m_pSlot->ItemInfo->bluemap[static_cast<const Blue>(it->first)];
            }

            if (currentvalue < maxvalue) {
                allTargetsMet = false;
                int stoneslot = FindBlueStone(it->second->MOptName128.c_str());

                if (stoneslot != -1) {
                    validStoneFound = true;
                    Fusing = true;
                    StartOperationTimeout();
                    CMsgStreamBuffer buf(0x7151);
                    buf << (byte)0x2;
                    buf << (byte)0x4;
                    buf << (byte)0x2;

                    buf << static_cast<unsigned char>(m_ItemSlot->m_pMySlot->m_pSlot->GetInventorySlotIndex() + 13u);
                    buf << static_cast<unsigned char>(stoneslot + 13u);
                    SendMsg(buf);
                    return; // İşlem yapıldı, fonksiyondan çık
                }
            }
        }
    }

    if (allTargetsMet) {
        StopAutomation(KmtGetText(L"UIIT_KMT_JOB_IS_COMPLETED"));
        return;
    }

    if (!validStoneFound) {
        StopAutomation(KmtGetText(L"UIIT_KMT_CHECK_MAGIC_STONES"));
    }
}


int CIFAlchemyMacro::FindAttrStone(std::wstring OptName)
{

    if(m_ItemSlot->m_pMySlot->m_pSlot->ItemInfo != NULL)
    {
        if(m_ItemSlot->m_pMySlot->m_pSlot->ItemInfo->GetItemData() != NULL)
        {
            const SItemData * data = m_ItemSlot->m_pMySlot->m_pSlot->ItemInfo->GetItemData();
            int Degree = GetDegreeLevel(data->m_itemClass);
            CIFInventory *inventory = g_pCGInterface->GetMainPopup()->GetInventory();
            for (int is = 0; is < inventory->InventorySlotCount(); is++) {
                CSOItem *newitem = inventory->GetItemBySlot(is);
                if (newitem->m_blValid != 0) {
                    if (newitem->GetItemData()->IsAttrStone()) {
                        if(newitem->GetItemData()->m_param1 == Degree && newitem->GetItemData()->m_desc1_128 == OptName.c_str())
                        {
                            return is;
                        }
                    }
                }
            }
        }
    }
    return -1;
}
int CIFAlchemyMacro::FindBlueStone(std::wstring OptName)
{

    if(m_ItemSlot->m_pMySlot->m_pSlot->ItemInfo != NULL)
    {
        if(m_ItemSlot->m_pMySlot->m_pSlot->ItemInfo->GetItemData() != NULL)
        {
            const SItemData * data = m_ItemSlot->m_pMySlot->m_pSlot->ItemInfo->GetItemData();
            int Degree = GetDegreeLevel(data->m_itemClass);
            CIFInventory *inventory = g_pCGInterface->GetMainPopup()->GetInventory();
            for (int is = 0; is < inventory->InventorySlotCount(); is++) {
                CSOItem *newitem = inventory->GetItemBySlot(is);
                if (newitem->m_blValid != 0) {
                    if (newitem->GetItemData()->IsMagicStone2()) {
                        if(newitem->GetItemData()->m_param1 == Degree && newitem->GetItemData()->m_desc1_128 == OptName.c_str())
                        {
                            return is;
                        }
                    }
                }
            }
        }
    }
    return -1;
}
void CGInterface::WriteSystemMessageForHook(eLogType btLevel, LPCWSTR lpszText) {
    //printf("%d %ls \n", btLevel, lpszText);

    if(this->GetGuiFromList<CNIFEnchantWnd>(168) != NULL)
    {
        CIFSystemMessage *pSystemMsg = m_IRM.GetResObj<CIFSystemMessage>(GDR_SYSTEM_MESSAGE_VIEW, 1);
        if (!pSystemMsg->IsLogAble(btLevel))
            return;

        D3DCOLOR dwColor;
        if ((btLevel == SYSLOG_COMBAT) || (btLevel == SYSLOG_GUIDE)) {
            dwColor = D3DCOLOR_ARGB(255, 186, 207, 242);
        } else {
            dwColor = D3DCOLOR_ARGB(255, 220, 201, 155);
        }

        pSystemMsg->WriteMessage(255, dwColor, lpszText, 0, 1);
    }
    else
    {
        if(this->m_IRM.GetResObj<CIFAlchemyMacro>(AlchemyMacro, 1)->IsVisible())
        {
            CIFAlchemyMacro* Macro = this->m_IRM.GetResObj<CIFAlchemyMacro>(AlchemyMacro, 1);
            if(Macro->m_ItemSlot->m_pMySlot->m_pSlot->ItemInfo != NULL)
            {
                if(Macro->m_ItemSlot->m_pMySlot->m_pSlot->ItemInfo->GetItemData() != NULL)
                {
                    CIFSystemMessage *pSystemMsg = m_IRM.GetResObj<CIFSystemMessage>(GDR_SYSTEM_MESSAGE_VIEW, 1);
                    if (!pSystemMsg->IsLogAble(btLevel))
                        return;

                    D3DCOLOR dwColor;
                    if ((btLevel == SYSLOG_COMBAT) || (btLevel == SYSLOG_GUIDE)) {
                        dwColor = D3DCOLOR_ARGB(255, 186, 207, 242);
                    } else {
                        dwColor = D3DCOLOR_ARGB(255, 220, 201, 155);
                    }
                    dwColor = D3DCOLOR_ARGB(255, 186, 207, 242);
                    pSystemMsg->WriteMessage(255, dwColor, KmtGetText(L"UIIT_KMT_AUTO_ALCHEMY_IS_WORKING"), 7, 1);
                    return;

                }
            }
        }
    }
}
void CIFAlchemyMacro::FusePlus()
{
    if(!AutomationRunning || Fusing)
        return;
    if (!HasTrackedItem())
    {
        StopAutomation(KmtGetText(L"UIIT_KMT_THE_SELECTED_ITEM_IS_NO_LONGER_AVAILABLE_AUTO_ALCHEMY_STOPPED"));
        return;
    }
    if(m_ItemSlot->m_pMySlot->m_pSlot->ItemInfo->m_OptLevel >= TargetPlus)
    {
        StopAutomation(KmtGetText(L"UIIT_KMT_ITEM_REACHED_THE_TARGET_PLUS"));
        return;
    }
    if(CheckImmortal() == 0) /// IMMORTAL YOK
    {
        int immortalslot = FindImmortal();
        if(immortalslot != -1)
        {
            Fusing = true;/// IMMORTALI BAS !!!!!!!!
            StartOperationTimeout();
            CMsgStreamBuffer buf(0x7151);
            buf << (byte)0x2;
            buf << (byte)0x4;
            buf << (byte)0x2;

            buf << static_cast<unsigned char>(m_ItemSlot->m_pMySlot->m_pSlot->GetInventorySlotIndex() +13u);
            buf << static_cast<unsigned char>(immortalslot + 13u);
            SendMsg(buf);
        }
        else
        {
            StopAutomation(KmtGetText(L"UIIT_KMT_IMMORTAL_STONE_IS_NOT_FOUND"));
        }
    }
    else if(CheckAstral() == 0) /// I NEED ASTRAL
    {
        if(CheckImmortalForAstral() == 1) /// I NO NEED IMMORTAL CAN USE ASTRAL
        {
            int immortalslot = FindAstral();
            if(immortalslot != -1)
            {
                Fusing = true;/// IMMORTALI BAS !!!!!!!!
                StartOperationTimeout();
                CMsgStreamBuffer buf(0x7151);
                buf << (byte)0x2;
                buf << (byte)0x4;
                buf << (byte)0x2;

                buf << static_cast<unsigned char>(m_ItemSlot->m_pMySlot->m_pSlot->GetInventorySlotIndex() +13u);
                buf << static_cast<unsigned char>(immortalslot + 13u);
                SendMsg(buf);
            }
            else
            {
                StopAutomation(KmtGetText(L"UIIT_KMT_ASTRAL_STONE_IS_NOT_FOUND"));
            }
        }
        else
        {
            StopAutomation(KmtGetText(L"UIIT_KMT_ASTRAL_REQUIRES_AN_IMMORTAL_STONE_ON_THE_ITEM"));
        }
    }
    else if(CheckSteady() == 0)
    {
        int immortalslot = FindSteady();
        if(immortalslot != -1)
        {
            Fusing = true;/// IMMORTALI BAS !!!!!!!!
            StartOperationTimeout();
            CMsgStreamBuffer buf(0x7151);
            buf << (byte)0x2;
            buf << (byte)0x4;
            buf << (byte)0x2;

            buf << static_cast<unsigned char>(m_ItemSlot->m_pMySlot->m_pSlot->GetInventorySlotIndex() +13u);
            buf << static_cast<unsigned char>(immortalslot + 13u);
            SendMsg(buf);
        }
        else
        {
            StopAutomation(KmtGetText(L"UIIT_KMT_STEADY_STONE_IS_NOT_FOUND"));
        }
    }
    else if(CheckLucky() == 0)
    {
        int immortalslot = FindLuck();
        if(immortalslot != -1)
        {
            Fusing = true;/// IMMORTALI BAS !!!!!!!!
            StartOperationTimeout();
            CMsgStreamBuffer buf(0x7151);
            buf << (byte)0x2;
            buf << (byte)0x4;
            buf << (byte)0x2;

            buf << static_cast<unsigned char>(m_ItemSlot->m_pMySlot->m_pSlot->GetInventorySlotIndex() +13u);
            buf << static_cast<unsigned char>(immortalslot + 13u);
            SendMsg(buf);
        }
        else
        {
            StopAutomation(KmtGetText(L"UIIT_KMT_LUCKY_STONE_IS_NOT_FOUND"));
        }
    }
    else if(CheckLuckyPowder() == 0)
    {
        int immortalslot = FindLuckPowder();
        if(immortalslot != -1)
        {
            //// find elixir !!!!!!!!
            int ElixirSlot = FindElixir();
            if(ElixirSlot != -1)
            {
                Fusing = true;/// IMMORTALI BAS !!!!!!!!
                StartOperationTimeout();
                CMsgStreamBuffer buf(0x7150);
                buf << (byte)0x2;
                buf << (byte)0x3;
                buf << (byte)0x3;

                buf << static_cast<unsigned char>(m_ItemSlot->m_pMySlot->m_pSlot->GetInventorySlotIndex() +13u);
                buf << static_cast<unsigned char>(ElixirSlot + 13u);
                buf << static_cast<unsigned char>(immortalslot + 13u);
                SendMsg(buf);
            }
            else
            {
                StopAutomation(KmtGetText(L"UIIT_KMT_ELIXIR_IS_NOT_FOUND"));
            }

        }
        else
        {
            StopAutomation(KmtGetText(L"UIIT_KMT_LUCKY_POWDER_IS_NOT_FOUND"));
        }
    }
    else
    {
        //// find elixir !!!!!!!!
        int ElixirSlot = FindElixir();
        if(ElixirSlot != -1)
        {
            Fusing = true;/// IMMORTALI BAS !!!!!!!!
            StartOperationTimeout();
            CMsgStreamBuffer buf(0x7150);
            buf << (byte)0x2;
            buf << (byte)0x3;
            buf << (byte)0x2;

            buf << static_cast<unsigned char>(m_ItemSlot->m_pMySlot->m_pSlot->GetInventorySlotIndex() +13u);
            buf << static_cast<unsigned char>(ElixirSlot + 13u);
            SendMsg(buf);
        }
        else
        {
            StopAutomation(KmtGetText(L"UIIT_KMT_ELIXIR_IS_NOT_FOUND"));
        }
    }
}
int CIFAlchemyMacro::FindElixir()
{
    if(m_ItemSlot->m_pMySlot->m_pSlot->ItemInfo != NULL)
    {
        if(m_ItemSlot->m_pMySlot->m_pSlot->ItemInfo->GetItemData() != NULL)
        {
            CIFInventory *inventory = g_pCGInterface->GetMainPopup()->GetInventory();
            const SItemData * data = m_ItemSlot->m_pMySlot->m_pSlot->ItemInfo->GetItemData();
            if(data->m_typeId.getTypeID1() == 3 && data->m_typeId.getTypeID2() == 1 && data->m_typeId.getTypeID3() == 6) /// is weapon
            {
                for (int is = 0; is < inventory->InventorySlotCount(); is++) {
                    CSOItem *newitem = inventory->GetItemBySlot(is);
                    if (newitem->m_blValid != 0) {
                        if (newitem->GetItemData()->IsElixir()) {

                            if(newitem->GetItemData()->m_param1 == 100663296)
                            {
                                return is;
                            }
                        }
                    }
                }
            }
            else if(data->m_typeId.getTypeID1() == 3 && data->m_typeId.getTypeID2() == 1 && data->m_typeId.getTypeID3() == 4) /// is shield
            {
                for (int is = 0; is < inventory->InventorySlotCount(); is++) {
                    CSOItem *newitem = inventory->GetItemBySlot(is);
                    if (newitem->m_blValid != 0) {
                        if (newitem->GetItemData()->IsElixir()) {

                            if(newitem->GetItemData()->m_param1 == 67108864)
                            {
                                return is;
                            }
                        }
                    }
                }
            }
            else if(data->m_typeId.getTypeID1() == 3 && data->m_typeId.getTypeID2() == 1 && (data->m_typeId.getTypeID3() == 1 || data->m_typeId.getTypeID3() == 2 ||
                    data->m_typeId.getTypeID3() == 3 || data->m_typeId.getTypeID3() == 9 || data->m_typeId.getTypeID3() == 10 || data->m_typeId.getTypeID3() == 11)) /// is protector
            {
                for (int is = 0; is < inventory->InventorySlotCount(); is++) {
                    CSOItem *newitem = inventory->GetItemBySlot(is);
                    if (newitem->m_blValid != 0) {
                        if (newitem->GetItemData()->IsElixir()) {

                            if(newitem->GetItemData()->m_param1 == 16909056)
                            {
                                return is;
                            }
                        }
                    }
                }
            }
            else if(data->m_typeId.getTypeID1() == 3 && data->m_typeId.getTypeID2() == 1 && (data->m_typeId.getTypeID3() == 5 || data->m_typeId.getTypeID3() == 12)) /// is acc
            {
                for (int is = 0; is < inventory->InventorySlotCount(); is++) {
                    CSOItem *newitem = inventory->GetItemBySlot(is);
                    if (newitem->m_blValid != 0) {
                        if (newitem->GetItemData()->IsElixir()) {

                            if(newitem->GetItemData()->m_param1 == 83886080)
                            {
                                return is;
                            }
                        }
                    }
                }
            }
        }
    }
    return -1;
}

void CIFAlchemyMacro::OnTimer(int timerId) {
    if(timerId == EffectTimer)
    {
        Slotdeco->FUN_00634300();
    }
    if(timerId == FusePlusTimer)
    {
        FusePlus();
    }
    if(timerId == ActionStartedTimer)
    {
        StopAutomation(KmtGetText(L"UIIT_KMT_THE_ALCHEMY_SERVER_DID_NOT_RESPOND_AUTO_ALCHEMY_STOPPED"));
    }
    if(timerId == FuseBlueTimer)
    {
        FuseBlueNew();
    }
    if(timerId == FuseAttrTimer)
    {
        FuseAttrStone();
    }
}
// Hex formatına dönüştürme ve son 4 basamağı alma fonksiyonu
int CIFAlchemyMacro::getDecimalFromLast4HexDigits(int value) {
    std::stringstream ss;
    ss << std::hex << value;
    std::string hexStr = ss.str();

    // Son 4 basamağı al
    std::string last4HexDigits = hexStr.length() > 4 ? hexStr.substr(hexStr.length() - 4) : hexStr;

    // Tekrar decimal formata çevir
    int decimalValue;
    std::stringstream ss2;
    ss2 << std::hex << last4HexDigits;
    ss2 >> decimalValue;

    return decimalValue;
}

int CIFAlchemyMacro::GetDegreeLevel(int itemClass) {
    return ((itemClass - 1) / 3 + 1);
}

int CIFAlchemyMacro::CheckImmortal()
{
    if(m_ItemSlot->m_pMySlot->m_pSlot->ItemInfo != NULL)
    {
        if(m_ItemSlot->m_pMySlot->m_pSlot->ItemInfo->GetItemData() != NULL)
        {
            if(ImmortalCheckBox->GetCheckedState_MAYBE())
            {
                if(m_ItemSlot->m_pMySlot->m_pSlot->ItemInfo->m_OptLevel >= ImmortalMinPlus)  /// IMMMORTAL PLUS KONTROLU
                {
                    const SItemData * data = m_ItemSlot->m_pMySlot->m_pSlot->ItemInfo->GetItemData();
                    int Degree = GetDegreeLevel(data->m_itemClass);

                    for(std::map<unsigned __int32, CMagicOptionData *>::iterator it = g_CGlobalDataManager->m_magicOptionDataMap.begin();
                        it != g_CGlobalDataManager->m_magicOptionDataMap.end(); it++)
                    {
                        if(it->second->Param1 == 1635018849 && it->second->MLevel == Degree)
                        {
                            if(m_ItemSlot->m_pMySlot->m_pSlot->ItemInfo->bluemap.find(static_cast<const Blue>(it->first)) ==
                               m_ItemSlot->m_pMySlot->m_pSlot->ItemInfo->bluemap.end())
                            {
                                return 0; /// I DONT HAVE IMMORTAL BEYBI
                            }
                        }
                    }
                }
            }
        }
    }
    return -1;
}
int CIFAlchemyMacro::CheckImmortalForAstral()
{
    if(m_ItemSlot->m_pMySlot->m_pSlot->ItemInfo != NULL)
    {
        if(m_ItemSlot->m_pMySlot->m_pSlot->ItemInfo->GetItemData() != NULL)
        {
                    const SItemData * data = m_ItemSlot->m_pMySlot->m_pSlot->ItemInfo->GetItemData();
                    int Degree = GetDegreeLevel(data->m_itemClass);

                    for(std::map<unsigned __int32, CMagicOptionData *>::iterator it = g_CGlobalDataManager->m_magicOptionDataMap.begin();
                        it != g_CGlobalDataManager->m_magicOptionDataMap.end(); it++)
                    {
                        if(it->second->Param1 == 1635018849 && it->second->MLevel == Degree)
                        {
                            if(m_ItemSlot->m_pMySlot->m_pSlot->ItemInfo->bluemap.find(static_cast<const Blue>(it->first)) !=
                               m_ItemSlot->m_pMySlot->m_pSlot->ItemInfo->bluemap.end())
                            {
                                return 1; /// I HAVE IMMORTAL BEYBI
                            }
                        }
                    }
        }
    }
    return -1;
}
int CIFAlchemyMacro::FindImmortal()
{
    if(m_ItemSlot->m_pMySlot->m_pSlot->ItemInfo != NULL)
    {
        if(m_ItemSlot->m_pMySlot->m_pSlot->ItemInfo->GetItemData() != NULL)
        {
            const SItemData * data = m_ItemSlot->m_pMySlot->m_pSlot->ItemInfo->GetItemData();
            int Degree = GetDegreeLevel(data->m_itemClass);
            CIFInventory *inventory = g_pCGInterface->GetMainPopup()->GetInventory();
            for (int is = 0; is < inventory->InventorySlotCount(); is++) {
                CSOItem *newitem = inventory->GetItemBySlot(is);
                if (newitem->m_blValid != 0) {
                    if (newitem->GetItemData()->IsMagicStone()) {
                        if(newitem->GetItemData()->m_param1 == Degree && newitem->GetItemData()->m_desc1_128 == L"MATTR_ATHANASIA")
                        {
                            return is;
                        }
                    }
                }
            }
          }
    }
    return -1;
}

int CIFAlchemyMacro::CheckAstral()
{
    if(m_ItemSlot->m_pMySlot->m_pSlot->ItemInfo != NULL)
    {
        if(m_ItemSlot->m_pMySlot->m_pSlot->ItemInfo->GetItemData() != NULL)
        {
            if(AstralCheckBox->GetCheckedState_MAYBE())
            {
                if(m_ItemSlot->m_pMySlot->m_pSlot->ItemInfo->m_OptLevel >= AstralMinPlus)  /// IMMMORTAL PLUS KONTROLU
                {
                    const SItemData * data = m_ItemSlot->m_pMySlot->m_pSlot->ItemInfo->GetItemData();
                    int Degree = GetDegreeLevel(data->m_itemClass);
                    for(std::map<unsigned __int32, CMagicOptionData *>::iterator it = g_CGlobalDataManager->m_magicOptionDataMap.begin();
                        it != g_CGlobalDataManager->m_magicOptionDataMap.end(); it++)
                    {
                        if(it->second->Param1 == 1634956402 && it->second->MLevel == Degree)
                        {
                            if(m_ItemSlot->m_pMySlot->m_pSlot->ItemInfo->bluemap.find(static_cast<const Blue>(it->first)) ==
                               m_ItemSlot->m_pMySlot->m_pSlot->ItemInfo->bluemap.end())
                            {
                                return 0; /// I NEED ASTRAL
                            }
                        }
                    }
                }
            }
        }
    }
    return -1;
}
int CIFAlchemyMacro::FindAstral()
{
    if(m_ItemSlot->m_pMySlot->m_pSlot->ItemInfo != NULL)
    {
        if(m_ItemSlot->m_pMySlot->m_pSlot->ItemInfo->GetItemData() != NULL)
        {
            const SItemData * data = m_ItemSlot->m_pMySlot->m_pSlot->ItemInfo->GetItemData();
            int Degree = GetDegreeLevel(data->m_itemClass);
            CIFInventory *inventory = g_pCGInterface->GetMainPopup()->GetInventory();
            for (int is = 0; is < inventory->InventorySlotCount(); is++) {
                CSOItem *newitem = inventory->GetItemBySlot(is);
                if (newitem->m_blValid != 0) {
                    if (newitem->GetItemData()->IsMagicStone()) {
                        if(newitem->GetItemData()->m_param1 == Degree && newitem->GetItemData()->m_desc1_128 == L"MATTR_ASTRAL")
                        {
                            return is;
                        }
                    }
                }
            }
        }
    }
    return -1;
}

int CIFAlchemyMacro::CheckSteady()
{
    if(m_ItemSlot->m_pMySlot->m_pSlot->ItemInfo != NULL)
    {
        if(m_ItemSlot->m_pMySlot->m_pSlot->ItemInfo->GetItemData() != NULL)
        {
            if(SteadyCheckBox->GetCheckedState_MAYBE())
            {
                if(m_ItemSlot->m_pMySlot->m_pSlot->ItemInfo->m_OptLevel >= SteadyMinPlus)  /// IMMMORTAL PLUS KONTROLU
                {
                            if(m_ItemSlot->m_pMySlot->m_pSlot->ItemInfo->bluemap.find(BLUE_STEADY) ==
                               m_ItemSlot->m_pMySlot->m_pSlot->ItemInfo->bluemap.end())
                            {
                                return 0; /// I NEED STEADY
                            }
                }
            }
        }
    }
    return -1;
}
int CIFAlchemyMacro::FindSteady()
{
    if(m_ItemSlot->m_pMySlot->m_pSlot->ItemInfo != NULL)
    {
        if(m_ItemSlot->m_pMySlot->m_pSlot->ItemInfo->GetItemData() != NULL)
        {
            CIFInventory *inventory = g_pCGInterface->GetMainPopup()->GetInventory();
            for (int is = 0; is < inventory->InventorySlotCount(); is++) {
                CSOItem *newitem = inventory->GetItemBySlot(is);
                if (newitem->m_blValid != 0) {
                    if (newitem->GetItemData()->IsMagicStone()) {
                        int Degree = GetDegreeLevel(m_ItemSlot->m_pMySlot->m_pSlot->ItemInfo->GetItemData()->m_itemClass);
                        if(newitem->GetItemData()->m_desc1_128 == L"MATTR_SOLID" && newitem->GetItemData()->m_param1 == Degree)
                        {
                            return is;
                        }
                    }
                }
            }
        }
    }
    return -1;
}

int CIFAlchemyMacro::CheckLucky()
{
    if(m_ItemSlot->m_pMySlot->m_pSlot->ItemInfo != NULL)
    {
        if(m_ItemSlot->m_pMySlot->m_pSlot->ItemInfo->GetItemData() != NULL)
        {
            if(LuckyCheckBox->GetCheckedState_MAYBE())
            {
                if(m_ItemSlot->m_pMySlot->m_pSlot->ItemInfo->m_OptLevel >= LuckyMinPlus)  /// IMMMORTAL PLUS KONTROLU
                {
                    const SItemData * data = m_ItemSlot->m_pMySlot->m_pSlot->ItemInfo->GetItemData();
                    int Degree = GetDegreeLevel(data->m_itemClass);

                    for(std::map<unsigned __int32, CMagicOptionData *>::iterator it = g_CGlobalDataManager->m_magicOptionDataMap.begin();
                        it != g_CGlobalDataManager->m_magicOptionDataMap.end(); it++)
                    {
                        if(it->second->Param1 == 1819632491 && it->second->MLevel == Degree)
                        {
                            if(m_ItemSlot->m_pMySlot->m_pSlot->ItemInfo->bluemap.find(static_cast<const Blue>(it->first)) ==
                               m_ItemSlot->m_pMySlot->m_pSlot->ItemInfo->bluemap.end())
                            {
                                return 0; /// I DONT HAVE IMMORTAL BEYBI
                            }
                        }
                    }
                }
            }
        }
    }
    return -1;
}
int CIFAlchemyMacro::FindLuck()
{
    if(m_ItemSlot->m_pMySlot->m_pSlot->ItemInfo != NULL)
    {
        if(m_ItemSlot->m_pMySlot->m_pSlot->ItemInfo->GetItemData() != NULL)
        {
            CIFInventory *inventory = g_pCGInterface->GetMainPopup()->GetInventory();
            for (int is = 0; is < inventory->InventorySlotCount(); is++) {
                CSOItem *newitem = inventory->GetItemBySlot(is);
                if (newitem->m_blValid != 0) {
                    if (newitem->GetItemData()->IsMagicStone()) {
                        int Degree = GetDegreeLevel(m_ItemSlot->m_pMySlot->m_pSlot->ItemInfo->GetItemData()->m_itemClass);
                        if(newitem->GetItemData()->m_desc1_128 == L"MATTR_LUCK" && newitem->GetItemData()->m_param1 == Degree)
                        {
                            return is;
                        }
                    }
                }
            }
        }
    }
    return -1;
}

int CIFAlchemyMacro::CheckLuckyPowder()
{
    if(m_ItemSlot->m_pMySlot->m_pSlot->ItemInfo != NULL)
    {
        if(m_ItemSlot->m_pMySlot->m_pSlot->ItemInfo->GetItemData() != NULL)
        {
            if(LuckyPowderCheckBox->GetCheckedState_MAYBE())
            {
                if(m_ItemSlot->m_pMySlot->m_pSlot->ItemInfo->m_OptLevel >= LuckyPowderMinPlus)  /// IMMMORTAL PLUS KONTROLU
                {
                    return 0;
                }
            }
        }
    }
    return -1;
}
int CIFAlchemyMacro::FindLuckPowder()
{
    if(m_ItemSlot->m_pMySlot->m_pSlot->ItemInfo != NULL)
    {
        if(m_ItemSlot->m_pMySlot->m_pSlot->ItemInfo->GetItemData() != NULL)
        {
            CIFInventory *inventory = g_pCGInterface->GetMainPopup()->GetInventory();
            for (int is = 0; is < inventory->InventorySlotCount(); is++) {
                CSOItem *newitem = inventory->GetItemBySlot(is);
                if (newitem->m_blValid != 0) {
                    if (newitem->GetItemData()->IsLuckyPowder()) {
                        int Degree = GetDegreeLevel(m_ItemSlot->m_pMySlot->m_pSlot->ItemInfo->GetItemData()->m_itemClass);
                        if(newitem->GetItemData()->m_itemClass == Degree)
                        {
                            return is;
                        }
                    }
                }
            }
        }
    }
    return -1;
}

void CIFAlchemyMacro::OnUpdate(){
  /*  if(g_pCGInterface->GetGuiFromList<CNIFEnchantWnd>(168) != NULL)
    {
        wnd_pos pp = g_pCGInterface->GetGuiFromList<CNIFEnchantWnd>(168)->GetPos();
        this->MoveGWnd(pp.x+400, pp.y);
    }
    else
    {
        this->OnCloseWnd();
    }*/
    std::n_wstring gettext2 = m_IRM.GetResObj<CIFEdit>(60, 1)->GetNText();


    std::string str2(gettext2.begin(), gettext2.end());

    // std::string'i int'e dönüştür
    std::istringstream isss(str2);
    int result2;
    if((isss >> result2))
    {
        if(result2 > 100)
        {
            std::wstringstream ss;
            ss << 100;
            std::wstring serverMaxLevelStr = ss.str();

            m_IRM.GetResObj<CIFEdit>(60, 1)->SetText(serverMaxLevelStr.c_str());
        }
        if(result2 == 0)
        {
            m_IRM.GetResObj<CIFEdit>(60, 1)->SetText(L"0");
        }
    }
    if(m_ItemSlot->m_pMySlot->m_pSlot->ItemInfo != NULL)
    {
        if(m_ItemSlot->m_pMySlot->m_pSlot->ItemInfo->GetItemData() != NULL)
        {
            wchar_t Priceb[255];
            swprintf_s(Priceb, L"%ls (+%d)", g_CTextStringManager->GetString2(m_ItemSlot->m_pMySlot->m_pSlot->ItemInfo->GetItemData()->NameStrID.c_str())->c_str(), m_ItemSlot->m_pMySlot->m_pSlot->ItemInfo->m_OptLevel);
            m_IRM.GetResObj(206, 1)->SetText(Priceb);
        }
        else
        {
            m_IRM.GetResObj(206, 1)->SetText(L"");
        }
    }
    else
    {
        m_IRM.GetResObj(206, 1)->SetText(L"");
    }
 /*   std::n_wstring gettext2 = TargetPlusLabel->GetNText();

    std::string str2(gettext2.begin(), gettext2.end());

    // std::string'i int'e dönüştür
    std::istringstream isss(str2);
    int result2;
    if((isss >> result2))
    {
        if(result2 > 12)
        {
            std::wstringstream ss;
            ss << 12;
            std::wstring serverMaxLevelStr = ss.str();

            TargetPlusLabel->SetText(serverMaxLevelStr.c_str());
        }
    }

    if(CurrentPage == 0)
    {
        if(m_ItemSlot->m_pMySlot->m_pSlot->ItemInfo != NULL)
        {
            if(m_ItemSlot->m_pMySlot->m_pSlot->ItemInfo->GetItemData() != NULL)
            {
                wchar_t Priceb[255];
                swprintf_s(Priceb, L"+ %d", m_ItemSlot->m_pMySlot->m_pSlot->ItemInfo->m_OptLevel);
                CurrentPlusLabel->SetText(Priceb);
            }
            else
            {
                CurrentPlusLabel->SetText(L"");
            }
        }
        else
        {
            CurrentPlusLabel->SetText(L"");
        }
    }
*/

}
void CIFAlchemyMacro::AddMattrToList(){

    ClearListAttr();
    ClearDDJListAttr();
    if(CurrentPage == 2)
    {
        if(!SelectedAttrCode.empty() && !m_IRM.GetResObj<CIFEdit>(60, 1)->GetNText().empty())
        {
            std::wstring targetText = m_IRM.GetResObj<CIFEdit>(60, 1)->GetNText().c_str();
            std::wistringstream targetStream(targetText);
            int targetValue = 0;
            if (!(targetStream >> targetValue) || !targetStream.eof() || targetValue < 1 || targetValue > 100)
            {
                D3DCOLOR color = D3DCOLOR_ARGB(255, 255, 255, 0);
                g_pCGInterface->GetSystemMessageView()->WriteMessage(255, color, KmtGetText(L"UIIT_KMT_TARGET_VALUE_MUST_BE_BETWEEN_1_AND_100"), 1, 1);
                return;
            }

            bool attrcodealreadyhave = false;
            for (std::vector<TargetAttrStr>::iterator it = m_TargetAttrList.begin(); it != m_TargetAttrList.end(); ++it)
            {
                if(it->Code == SelectedAttrCode)
                {
                    attrcodealreadyhave = true;
                    break;
                }
            }
            if(!attrcodealreadyhave)
            {
                for (std::vector<AttrStr>::iterator it = m_AttrList.begin(); it != m_AttrList.end(); ++it)
                {
                    if(it->Code == SelectedAttrCode)
                    {
                        TargetAttrStr aa = TargetAttrStr();
                        aa.Code = it->Code;
                        aa.Name = it->Name;
                        aa.Target = targetValue;
                        m_TargetAttrList.push_back(aa);
                    }
                }
            }
            LoadTargetAttrList();
        }
    }
}
void CIFAlchemyMacro::LoadTargetAttrList()
{
    int i = 0;
    for (std::vector<TargetAttrStr>::iterator it = m_TargetAttrList.begin(); it != m_TargetAttrList.end(); ++it)
    {
        if(i < 11)
        {
            m_AttrSlots[i]->LoadItems2(it->Target, it->Code.c_str(), it->Name.c_str());
            m_scrollattr->AddItem(m_AttrSlots[i], 1, 0);
            ++i;
        }
    }
}
void CIFAlchemyMacro::RemoveMattrToList(){
    ClearListAttr();
    ClearDDJListAttr();
    if(!SelectedRemoveAttrCode.empty())
    {
        for (std::vector<TargetAttrStr>::iterator it = m_TargetAttrList.begin(); it != m_TargetAttrList.end();)
        {
            if(it->Code == SelectedRemoveAttrCode)
            {
                it = m_TargetAttrList.erase(it); // erase fonksiyonu geçerli iteratoru döner
            }
            else
            {
                ++it; // yalnızca öğe silinmediğinde iteratoru artırırız
            }
        }
        LoadTargetAttrList();
    }
}


