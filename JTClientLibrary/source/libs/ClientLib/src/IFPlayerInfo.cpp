#include <CustomData/CustomCICPlayer.h>
#include <CustomData/CustomSettingManager.h>
#include "IFPlayerInfo.h"
#include "CharacterDependentData.h"
#include "GInterface.h"
#include "GlobalDataManager.h"
#include "ICPlayer.h"
#include "TextStringManager.h"

GFX_IMPLEMENT_DYNAMIC_EXISTING(CIFPlayerInfo, 0x00eea7e8)
GFX_IMPLEMENT_DYNCREATE_FN(CIFPlayerInfo, CIFWnd)

enum {
    GDR_PI_BTN_ADDHP = 14,
    GDR_PI_BTN_ADDMP = 15,
    GDR_BTN_SLOT_CHECKBOX_STR = 1000,
    GDR_BTN_SLOT_CHECKBOX_INT = 1001
};

GFX_BEGIN_MESSAGE_MAP(CIFPlayerInfo, CIFWnd)
                    ONG_COMMAND(GDR_PI_BTN_ADDHP, &CIFPlayerInfo::OnClick_IncrementStr)
                    ONG_COMMAND(GDR_PI_BTN_ADDMP, &CIFPlayerInfo::OnClick_IncrementInt)
                    ONG_COMMAND(GDR_BTN_SLOT_CHECKBOX_STR, &CIFPlayerInfo::OnClick_IncrementStrCheckBox)
                    ONG_COMMAND(GDR_BTN_SLOT_CHECKBOX_INT, &CIFPlayerInfo::OnClick_IncrementIntCheckBox)
GFX_END_MESSAGE_MAP()


bool CIFPlayerInfo::OnCreate(long ln) {

    if(!m_Settings->EnableAutoStrInt)
    {
        m_IRM.LoadFromFile("resinfo\\ifplayerinfo_trijob2.txt");
        m_IRM.CreateInterfaceSection("Create", this);
    }
    else
    {
        m_IRM.LoadFromFile("clientlibrary\\resinfo\\ifplayerinfo_trijob2.txt");
        m_IRM.CreateInterfaceSection("Create", this);

        this->m_IRM.GetResObj<CIFCheckBox>(GDR_BTN_SLOT_CHECKBOX_STR, 1)->SetCheckBoxState(false);
        this->m_IRM.GetResObj<CIFCheckBox>(GDR_BTN_SLOT_CHECKBOX_INT, 1)->SetCheckBoxState(false);
    }



    m_pGDR_PI_TEXT_LEVEL = m_IRM.GetResObj<CIFStatic>(2, 1);
    m_pGDR_PI_TEXT_CURXP2 = m_IRM.GetResObj<CIFStatic>(5, 1);
    m_pGDR_PI_TEXT_NEXTXP2 = m_IRM.GetResObj<CIFStatic>(7, 1);
    m_pGDR_PI_TEXT_STAT2 = m_IRM.GetResObj<CIFStatic>(9, 1);
    m_pGDR_PI_HONOR_DATA = m_IRM.GetResObj<CIFStatic>(0x33, 1);
    m_pGDR_PI_TEXT_HONOR_STATIC = m_IRM.GetResObj<CIFStatic>(0x32, 1);
    m_pGDR_PI_TEXT_HP_DAT = m_IRM.GetResObj<CIFStatic>(0xc, 1);
    m_pGDR_PI_TEXT_MP_DAT = m_IRM.GetResObj<CIFStatic>(0xd, 1);
    m_pGDR_PI_TEXT_STRENGTH_DAT = m_IRM.GetResObj<CIFStatic>(0x12, 1);
    m_pGDR_PI_TEXT_INTELLECT_DAT = m_IRM.GetResObj<CIFStatic>(0x13, 1);
    m_pGDR_PI_TEXT_PHYATT_DAT = m_IRM.GetResObj<CIFStatic>(0x22, 1);
    m_pGDR_PI_TEXT_PHYDEF_DAT = m_IRM.GetResObj<CIFStatic>(0x23, 1);
    m_pGDR_PI_TEXT_PHYBAL_DAT = m_IRM.GetResObj<CIFStatic>(0x24, 1);
    m_pGDR_PI_TEXT_HIT_DAT = m_IRM.GetResObj<CIFStatic>(0x25, 1);
    m_pGDR_PI_TEXT_MAGATT_DAT = m_IRM.GetResObj<CIFStatic>(0x2a, 1);
    m_pGDR_PI_TEXT_MAGDEF_DAT = m_IRM.GetResObj<CIFStatic>(0x2b, 1);
    m_pGDR_PI_TEXT_MAGBAL_DAT = m_IRM.GetResObj<CIFStatic>(0x2c, 1);
    m_pGDR_PI_TEXT_PARRY_DAT = m_IRM.GetResObj<CIFStatic>(0x2d, 1);
    m_pGDR_PI_JOB_ALIAS = m_IRM.GetResObj<CIFStatic>(70, 1);
    m_pGDR_PI_JOB_ICON = m_IRM.GetResObj<CIFStatic>(71, 1);
    m_pGDR_PI_JOB_TITLE = m_IRM.GetResObj<CIFStatic>(72, 1);
    m_pGDR_PI_JOB_GRADE = m_IRM.GetResObj<CIFStatic>(73, 1);
    m_pGDR_PI_JOB_EXP = m_IRM.GetResObj<CIFStatic>(75, 1);
    m_pGDR_PI_JOB_EXP_GAUGE = m_IRM.GetResObj<CIFGauge>(74, 1);
    m_pGDR_PI_BTN_ADDHP = m_IRM.GetResObj<CIFButton>(14, 1);
    m_pGDR_PI_BTN_ADDMP = m_IRM.GetResObj<CIFButton>(15, 1);
    m_pGDR_PI_GAUGE_HPGAUGE = m_IRM.GetResObj<CIFGauge>(20, 1);
    m_pGDR_PI_GAUGE_MPGAUGE = m_IRM.GetResObj<CIFGauge>(21, 1);


    m_pGDR_PI_BTN_ADDMP->FUN_00656640("interface\\ifcommon\\com_plus_button_disable.ddj");
    m_pGDR_PI_BTN_ADDHP->FUN_00656640("interface\\ifcommon\\com_plus_button_disable.ddj");

    m_pGDR_PI_GAUGE_HPGAUGE->m_valueFg = 0.0f;
    m_pGDR_PI_GAUGE_MPGAUGE->m_valueFg = 0.0f;
    m_pGDR_PI_GAUGE_HPGAUGE->m_valueBg = 0.0f;
    m_pGDR_PI_GAUGE_MPGAUGE->m_valueBg = 0.0f;
    m_pGDR_PI_GAUGE_HPGAUGE->field_0x38c = 0.1f;
    m_pGDR_PI_GAUGE_MPGAUGE->field_0x38c = 0.1f;


    m_pGDR_PI_TEXT_PHYATT_DAT->SetTextFormatted(L"%d ~ %d",
                                                g_CCharacterDependentData.FUN_00987090().field_0,
                                                g_CCharacterDependentData.FUN_00987090().field_4);

    m_pGDR_PI_TEXT_PHYDEF_DAT->SetTextFormatted(L"%d",
                                                g_CCharacterDependentData.FUN_00987090().field_10);

    m_pGDR_PI_TEXT_PHYBAL_DAT->SetText(L"");

    m_pGDR_PI_TEXT_HIT_DAT->SetTextFormatted(L"%d",
                                             g_CCharacterDependentData.FUN_00987090().field_14);

    m_pGDR_PI_TEXT_MAGATT_DAT->SetTextFormatted(L"%d ~ %d",
                                                g_CCharacterDependentData.FUN_00987090().field_8,
                                                g_CCharacterDependentData.FUN_00987090().field_C);

    m_pGDR_PI_TEXT_MAGDEF_DAT->SetTextFormatted(L"%d",
                                                g_CCharacterDependentData.FUN_00987090().field_12);

    m_pGDR_PI_TEXT_MAGBAL_DAT->SetText(L"");

    m_pGDR_PI_TEXT_PARRY_DAT->SetTextFormatted(L"%d",
                                               g_CCharacterDependentData.FUN_00987090().field_16);

    if(m_Settings->ENABLE_NEW_JOB_UI)
    {
        wnd_pos y = m_pGDR_PI_JOB_ALIAS->GetPos();
        m_pGDR_PI_JOB_ALIAS->MoveGWnd(y.x + 32, y.y);

        wnd_pos yx = m_pGDR_PI_JOB_ICON->GetPos();
        m_pGDR_PI_JOB_ICON->MoveGWnd(yx.x - 40, yx.y - 15);

    }


    return true;
}

void CIFPlayerInfo::OnUpdate() {
    if(m_Settings->ENABLE_NEW_JOB_UI)
    {
        g_pCGInterface->GetGuiFromList<CIFStatic>(JobIconID)->MoveGWnd(this->GetPos().x + 205, this->GetPos().y + 25);
        g_pCGInterface->GetGuiFromList<CIFStatic>(JobIconID)->BringToFront();
    }

    reinterpret_cast<void (__thiscall *)(CIFPlayerInfo *)>(0x006b8910)(this);
}

void CIFPlayerInfo::ShowGWnd(bool bVisible) {
    if(((bVisible != false) && g_pMyPlayerObj != NULL) && (g_pMyPlayerObj->GetStrength() != 0)) {
        m_pGDR_PI_GAUGE_HPGAUGE->m_valueBg = static_cast<float>(g_pMyPlayerObj->FUN_009c7880()) / g_pMyPlayerObj->GetStrength() * 10.0f;
        m_pGDR_PI_GAUGE_HPGAUGE->m_valueFg = m_pGDR_PI_GAUGE_HPGAUGE->m_valueBg;
        m_pGDR_PI_GAUGE_MPGAUGE->m_valueBg = static_cast<float>(g_pMyPlayerObj->FUN_009c7880()) / g_pMyPlayerObj->GetIntelligence() * 10.0f;
        m_pGDR_PI_GAUGE_MPGAUGE->m_valueFg = m_pGDR_PI_GAUGE_MPGAUGE->m_valueBg;
    }
    CIFWnd::ShowGWnd(bVisible);
    //reinterpret_cast<void(__thiscall *)(CIFPlayerInfo *, bool)>(0x006b8310)(this, bVisible);
}

void CIFPlayerInfo::OnClick_IncrementStr() {
    g_pCGInterface->RequestStatIncrement(0x4b, 1, 0, 1);
}

void CIFPlayerInfo::OnClick_IncrementInt() {
    g_pCGInterface->RequestStatIncrement(0x4b, 2, 0, 1);
}
void CIFPlayerInfo::OnClick_IncrementStrCheckBox(){
    if(m_IRM.GetResObj<CIFCheckBox>(GDR_BTN_SLOT_CHECKBOX_STR, 1)->GetCheckedState_MAYBE())
    {
        g_pCGInterface->CreateCustomMessageBox(1, 0);
        m_IRM.GetResObj<CIFCheckBox>(GDR_BTN_SLOT_CHECKBOX_STR, 1)->SetCheckBoxState(false);
    }
    else if(!m_IRM.GetResObj<CIFCheckBox>(GDR_BTN_SLOT_CHECKBOX_STR, 1)->GetCheckedState_MAYBE())
    {
        m_Player->Enabled_AutoStr = false;
        m_IRM.GetResObj<CIFCheckBox>(GDR_BTN_SLOT_CHECKBOX_STR, 1)->SetCheckBoxState(false);
    }
}
void CIFPlayerInfo::EnableAutoStr()
{
    m_Player->Enabled_AutoInt = false;
    m_IRM.GetResObj<CIFCheckBox>(GDR_BTN_SLOT_CHECKBOX_INT, 1)->SetCheckBoxState(false);

    m_Player->Enabled_AutoStr = true;
    m_IRM.GetResObj<CIFCheckBox>(GDR_BTN_SLOT_CHECKBOX_STR, 1)->SetCheckBoxState(true);

}
void CIFPlayerInfo::OnClick_IncrementIntCheckBox(){
    if(m_IRM.GetResObj<CIFCheckBox>(GDR_BTN_SLOT_CHECKBOX_INT, 1)->GetCheckedState_MAYBE())
    {
        g_pCGInterface->CreateCustomMessageBox(2, 0);
        m_IRM.GetResObj<CIFCheckBox>(GDR_BTN_SLOT_CHECKBOX_INT, 1)->SetCheckBoxState(false);
    }
    else if(!m_IRM.GetResObj<CIFCheckBox>(GDR_BTN_SLOT_CHECKBOX_INT, 1)->GetCheckedState_MAYBE())
    {
        m_Player->Enabled_AutoInt = false;
        m_IRM.GetResObj<CIFCheckBox>(GDR_BTN_SLOT_CHECKBOX_INT, 1)->SetCheckBoxState(false);
    }
}
void CIFPlayerInfo::EnableAutoInt()
{
    m_Player->Enabled_AutoStr = false;
    m_IRM.GetResObj<CIFCheckBox>(GDR_BTN_SLOT_CHECKBOX_STR, 1)->SetCheckBoxState(false);

    m_Player->Enabled_AutoInt = true;
    m_IRM.GetResObj<CIFCheckBox>(GDR_BTN_SLOT_CHECKBOX_INT, 1)->SetCheckBoxState(true);

}