#include "IFCOSInfo.h"
#include "ICCos.h"
#include "ICPlayer.h"
#include "IFGauge.h"
#include "IFNormalTile.h"
#include "unsorted.h"
#include "IFRenderStatic.h"
#include "TextStringManager.h"
#include <GInterface.h>
#include <GlobalDataManager.h>
#include <IFCOS.h>
#include <CustomData/CustomDataManager.h>
#include <CustomData/CustomCICPlayer.h>
GFX_IMPLEMENT_DYNAMIC_EXISTING(CIFCOSInfo, 0x00EEC088)



void CIFCOSInfo::OnUpdateIMPL() {
    reinterpret_cast<void(__thiscall *)(const CIFCOSInfo *)>(0x007a55f0)(this);

    CICCos *pUser = static_cast<CICCos *>(GetCharacterObjectByID_MAYBE(this->m_petUniqueID));
    if (pUser == NULL) {
        return;
    }
    if(m_CustomDataManager->m_RefFellowPetSystem.find(pUser->GetCommonData()->NameStrID) != m_CustomDataManager->m_RefFellowPetSystem.end())
    {
        if(this->m_IRM.GetResObj<CIFGauge>(36, 1)->m_valueFg <= 0.30)
        {
            this->m_IRM.GetResObj<CIFGauge>(36, 1)->TB_Func_13("clientlibrary\\fellowpets\\pt_hgp_bad.ddj", 1, 1);
        }
        else if(this->m_IRM.GetResObj<CIFGauge>(36, 1)->m_valueFg > 0.30 && this->m_IRM.GetResObj<CIFGauge>(36, 1)->m_valueFg <= 0.80)
        {
            this->m_IRM.GetResObj<CIFGauge>(36, 1)->TB_Func_13("clientlibrary\\fellowpets\\pt_mp.ddj", 1, 1);

        }
        else if(this->m_IRM.GetResObj<CIFGauge>(36, 1)->m_valueFg > 0.80)
        {
            this->m_IRM.GetResObj<CIFGauge>(36, 1)->TB_Func_13("clientlibrary\\fellowpets\\pt_hgp_good.ddj", 1, 1);

        }

        /*CICharactor* pUser = GetCharacterObjectByID_MAYBE(this->m_petUniqueID);
        if (pUser != NULL) {
            std::n_wstring NameStrID = pUser->GetCommonData()->NameStrID;
            if(m_CustomDataManager->m_RefFellowPetSystem.find(NameStrID) != m_CustomDataManager->m_RefFellowPetSystem.end())
            {
                if(m_CustomDataManager->m_RefFellowPetSystem[NameStrID].UniqueSkillID_1 > 0)
                {
                    const CCharacterData *uqdata = NULL;
                    uqdata = g_CGlobalDataManager->GetCharacter(pUser->GetCommonData()->RefObjectId);
                    if(uqdata != NULL)
                    {
                        if(uqdata->GetData().Level >= m_CustomDataManager->m_RefFellowPetSystem[NameStrID].ActiveUniqueSkillLevel_1)
                        {
                            this->m_IRM.GetResObj<CIFFellowSlot>(GDR_SKILL_SLOT_UNIQUE_1, 1)->m_pMySlot->m_pSlot->SetType(73);
                            this->m_IRM.GetResObj<CIFFellowSlot>(GDR_SKILL_SLOT_UNIQUE_1, 1)->m_pMySlot->m_pSlot->FUN_00666680(2);
                            this->m_IRM.GetResObj<CIFFellowSlot>(GDR_SKILL_SLOT_UNIQUE_1, 1)->m_pMySlot->m_pSlot->GetIconSlot(73, m_CustomDataManager->m_RefFellowPetSystem[NameStrID].UniqueSkillID_1);
                            this->m_IRM.GetResObj<CIFFellowSlot>(GDR_SKILL_SLOT_UNIQUE_1, 1)->m_pMySlot->m_pSlot->SetClickable(true);
                        }

                        if(m_CustomDataManager->m_RefFellowPetSystem[NameStrID].UniqueSkillID_2 > 0)
                        {
                            if(uqdata->GetData().Level >= m_CustomDataManager->m_RefFellowPetSystem[NameStrID].ActiveUniqueSkillLevel_2)
                            {
                                this->m_IRM.GetResObj<CIFFellowSlot>(GDR_SKILL_SLOT_UNIQUE_2, 1)->m_pMySlot->m_pSlot->SetType(73);
                                this->m_IRM.GetResObj<CIFFellowSlot>(GDR_SKILL_SLOT_UNIQUE_2, 1)->m_pMySlot->m_pSlot->GetIconSlot(73, m_CustomDataManager->m_RefFellowPetSystem[NameStrID].UniqueSkillID_2);
                                this->m_IRM.GetResObj<CIFFellowSlot>(GDR_SKILL_SLOT_UNIQUE_2, 1)->m_pMySlot->m_pSlot->SetClickable(true);
                            }
                        }
                        if(m_CustomDataManager->m_RefFellowPetSystem[NameStrID].UniqueSkillID_3 > 0)
                        {
                            if(uqdata->GetData().Level >= m_CustomDataManager->m_RefFellowPetSystem[NameStrID].ActiveUniqueSkillLevel_3)
                            {
                                this->m_IRM.GetResObj<CIFFellowSlot>(GDR_SKILL_SLOT_UNIQUE_3, 1)->m_pMySlot->m_pSlot->SetType(73);
                                this->m_IRM.GetResObj<CIFFellowSlot>(GDR_SKILL_SLOT_UNIQUE_3, 1)->m_pMySlot->m_pSlot->GetIconSlot(73, m_CustomDataManager->m_RefFellowPetSystem[NameStrID].UniqueSkillID_3);
                                this->m_IRM.GetResObj<CIFFellowSlot>(GDR_SKILL_SLOT_UNIQUE_3, 1)->m_pMySlot->m_pSlot->SetClickable(true);
                            }
                        }
                        if(m_CustomDataManager->m_RefFellowPetSystem[NameStrID].UniqueSkillID_4 > 0)
                        {
                            if(uqdata->GetData().Level >= m_CustomDataManager->m_RefFellowPetSystem[NameStrID].ActiveUniqueSkillLevel_4)
                            {
                                this->m_IRM.GetResObj<CIFFellowSlot>(GDR_SKILL_SLOT_UNIQUE_4, 1)->m_pMySlot->m_pSlot->SetType(73);
                                this->m_IRM.GetResObj<CIFFellowSlot>(GDR_SKILL_SLOT_UNIQUE_4, 1)->m_pMySlot->m_pSlot->GetIconSlot(73, m_CustomDataManager->m_RefFellowPetSystem[NameStrID].UniqueSkillID_4);
                                this->m_IRM.GetResObj<CIFFellowSlot>(GDR_SKILL_SLOT_UNIQUE_4, 1)->m_pMySlot->m_pSlot->SetClickable(true);
                            }
                        }
                        if(m_CustomDataManager->m_RefFellowPetSystem[NameStrID].UniqueSkillID_5 > 0)
                        {
                            if(uqdata->GetData().Level >= m_CustomDataManager->m_RefFellowPetSystem[NameStrID].ActiveUniqueSkillLevel_5)
                            {
                                this->m_IRM.GetResObj<CIFFellowSlot>(GDR_SKILL_SLOT_UNIQUE_5, 1)->m_pMySlot->m_pSlot->SetType(73);
                                this->m_IRM.GetResObj<CIFFellowSlot>(GDR_SKILL_SLOT_UNIQUE_5, 1)->m_pMySlot->m_pSlot->GetIconSlot(73, m_CustomDataManager->m_RefFellowPetSystem[NameStrID].UniqueSkillID_5);
                                this->m_IRM.GetResObj<CIFFellowSlot>(GDR_SKILL_SLOT_UNIQUE_5, 1)->m_pMySlot->m_pSlot->SetClickable(true);
                            }
                        }
                    }
                }
            }
        }*/

    }
    else
    {
        this->m_IRM.GetResObj<CIFGauge>(36, 1)->TB_Func_13("interface\\pet\\pt_hgp.ddj", 1, 1);
    }
}
int CIFCOSInfo::OnMouseLeftDownIMPL(int a1, int x, int y) {
    return reinterpret_cast<int(__thiscall *)(CIFWnd *, int, int, int)>(0x0046fd60)(this, a1, x, y);
}
bool CIFCOSInfo::OnCreateIMPL(long ln) {


    bool b = reinterpret_cast<bool(__thiscall *)(CIFCOSInfo *, long)>(0x007a48d0)(this, ln);
    wnd_rect sz;
    sz.pos.x = 20;
    sz.pos.y = 164;
    sz.size.width = 292;
    sz.size.height = 168;
    this->m_IRM.GetResObj<CIFRenderStatic>(GDR_RENDER_STATIC , 1)->ShowGWnd(false);
    this->m_IRM.GetResObj<CIFStatic>(1001, 1)->ShowGWnd(false);

    this->m_IRM.GetResObj<CIFStatic>(1002, 1)->ShowGWnd(false);
    std::n_wstring str = TSM_GETTEXTPTR(L"UIIT_STT_SKILLPOINT");
    this->m_IRM.GetResObj<CIFStatic>(1002, 1)->SetTooltip(str);
    this->m_IRM.GetResObj<CIFStatic>(1002, 1)->SetStyleThingy(TOOLTIP);

    this->m_IRM.GetResObj<CIFStatic>(1003, 1)->ShowGWnd(false);

    this->m_IRM.GetResObj<CIFStatic>(GDR_INFO_FRAME, 1)->ShowGWnd(false);

    this->m_IRM.GetResObj<CIFStatic>(GDR_STATS, 1)->ShowGWnd(false);

    this->m_IRM.GetResObj<CIFStatic>(GDR_STATS, 1)->m_FontTexture.sub_8B4750(7);
    this->m_IRM.GetResObj<CIFStatic>(GDR_STR, 1)->ShowGWnd(false);
    this->m_IRM.GetResObj<CIFStatic>(GDR_STR, 1)->SetText(KmtGetText(L"UIIT_KMT_STR"));
    this->m_IRM.GetResObj<CIFStatic>(GDR_STR_VALUE, 1)->ShowGWnd(false);
    this->m_IRM.GetResObj<CIFStatic>(GDR_STR_VALUE, 1)->SetText(L"-");
    this->m_IRM.GetResObj<CIFStatic>(GDR_INT, 1)->ShowGWnd(false);
    this->m_IRM.GetResObj<CIFStatic>(GDR_INT_VALUE, 1)->ShowGWnd(false);

    this->m_IRM.GetResObj<CIFStatic>(GDR_INT, 1)->SetText(KmtGetText(L"UIIT_KMT_INT"));
    this->m_IRM.GetResObj<CIFStatic>(GDR_INT_VALUE, 1)->SetText(L"-");

    this->m_IRM.GetResObj<CIFStatic>(GDR_ATTACK, 1)->ShowGWnd(false);

    this->m_IRM.GetResObj<CIFStatic>(GDR_ATTACK, 1)->m_FontTexture.sub_8B4750(7);

    this->m_IRM.GetResObj<CIFStatic>(GDR_CRT, 1)->ShowGWnd(false);
    this->m_IRM.GetResObj<CIFStatic>(GDR_CRT_VALUE, 1)->ShowGWnd(false);

    this->m_IRM.GetResObj<CIFStatic>(GDR_DEFEND, 1)->ShowGWnd(false);
    this->m_IRM.GetResObj<CIFStatic>(GDR_DEFEND, 1)->m_FontTexture.sub_8B4750(7);

    this->m_IRM.GetResObj<CIFStatic>(GDR_BR, 1)->ShowGWnd(false);
    this->m_IRM.GetResObj<CIFStatic>(GDR_BR_VALUE, 1)->ShowGWnd(false);

    this->m_IRM.GetResObj<CIFStatic>(GDR_SLOT1, 1)->ShowGWnd(false);
    this->m_IRM.GetResObj<CIFStatic>(GDR_SLOT2, 1)->ShowGWnd(false);
    this->m_IRM.GetResObj<CIFStatic>(GDR_SLOT3, 1)->ShowGWnd(false);
    this->m_IRM.GetResObj<CIFStatic>(GDR_SLOT4, 1)->ShowGWnd(false);

    this->m_IRM.GetResObj<CIFStatic>(GDR_SLOT5, 1)->ShowGWnd(false);
    this->m_IRM.GetResObj<CIFStatic>(GDR_SLOT6, 1)->ShowGWnd(false);
    this->m_IRM.GetResObj<CIFStatic>(GDR_SLOT7, 1)->ShowGWnd(false);
    this->m_IRM.GetResObj<CIFStatic>(GDR_SLOT8, 1)->ShowGWnd(false);
    this->m_IRM.GetResObj<CIFFrame>(GDR_SKILL_FRAME, 1)->ShowGWnd(false);
    this->m_IRM.GetResObj<CIFStatic>(GDR_SKILL_HEADER, 1)->ShowGWnd(false);

    this->m_IRM.GetResObj<CIFFrame>(GDR_SKILL_FRAME2, 1)->ShowGWnd(false);
    this->m_IRM.GetResObj<CIFStatic>(GDR_SKILL_HEADER2, 1)->ShowGWnd(false);

    this->m_IRM.GetResObj<CIFFrame>(GDR_SKILL_FRAME3, 1)->ShowGWnd(false);
    this->m_IRM.GetResObj<CIFStatic>(GDR_SKILL_HEADER3, 1)->ShowGWnd(false);

    this->m_IRM.GetResObj<CIFStatic>(GDR_SKILL_FRAME_UNIQUE_1, 1)->ShowGWnd(false);
    this->m_IRM.GetResObj<CIFSlotWithHelp>(GDR_SKILL_SLOT_UNIQUE_1, 1)->ShowGWnd(false);

    this->m_IRM.GetResObj<CIFStatic>(GDR_SKILL_FRAME_UNIQUE_2, 1)->ShowGWnd(false);
    this->m_IRM.GetResObj<CIFSlotWithHelp>(GDR_SKILL_SLOT_UNIQUE_2, 1)->ShowGWnd(false);



    this->m_IRM.GetResObj<CIFStatic>(GDR_SKILL_FRAME_UNIQUE_3, 1)->ShowGWnd(false);
    this->m_IRM.GetResObj<CIFSlotWithHelp>(GDR_SKILL_SLOT_UNIQUE_3, 1)->ShowGWnd(false);


    this->m_IRM.GetResObj<CIFStatic>(GDR_SKILL_FRAME_UNIQUE_4, 1)->ShowGWnd(false);
    this->m_IRM.GetResObj<CIFSlotWithHelp>(GDR_SKILL_SLOT_UNIQUE_4, 1)->ShowGWnd(false);


    this->m_IRM.GetResObj<CIFStatic>(GDR_SKILL_FRAME_UNIQUE_5, 1)->ShowGWnd(false);
    this->m_IRM.GetResObj<CIFSlotWithHelp>(GDR_SKILL_SLOT_UNIQUE_5, 1)->ShowGWnd(false);


    this->m_IRM.GetResObj<CIFStatic>(GDR_SKILL_FRAME_READIED_1, 1)->ShowGWnd(false);
    this->m_IRM.GetResObj<CIFSlotWithHelp>(GDR_SKILL_SLOT_READIED_1, 1)->ShowGWnd(false);

    this->m_IRM.GetResObj<CIFStatic>(GDR_SKILL_FRAME_READIED_2, 1)->ShowGWnd(false);
    this->m_IRM.GetResObj<CIFSlotWithHelp>(GDR_SKILL_SLOT_READIED_2, 1)->ShowGWnd(false);



    this->m_IRM.GetResObj<CIFStatic>(GDR_SKILL_FRAME_READIED_3, 1)->ShowGWnd(false);
    this->m_IRM.GetResObj<CIFSlotWithHelp>(GDR_SKILL_SLOT_READIED_3, 1)->ShowGWnd(false);


    this->m_IRM.GetResObj<CIFStatic>(GDR_SKILL_FRAME_READIED_4, 1)->ShowGWnd(false);
    this->m_IRM.GetResObj<CIFSlotWithHelp>(GDR_SKILL_SLOT_READIED_4, 1)->ShowGWnd(false);


    this->m_IRM.GetResObj<CIFStatic>(GDR_SKILL_FRAME_READIED_5, 1)->ShowGWnd(false);
    this->m_IRM.GetResObj<CIFSlotWithHelp>(GDR_SKILL_SLOT_READIED_5, 1)->ShowGWnd(false);

    this->m_IRM.GetResObj<CIFStatic>(GDR_SKILL_FRAME_FELLOW_1, 1)->ShowGWnd(false);
    this->m_IRM.GetResObj<CIFSlotWithHelp>(GDR_SKILL_SLOT_FELLOW_1, 1)->ShowGWnd(false);

    this->m_IRM.GetResObj<CIFStatic>(GDR_SKILL_FRAME_FELLOW_2, 1)->ShowGWnd(false);
    this->m_IRM.GetResObj<CIFSlotWithHelp>(GDR_SKILL_SLOT_FELLOW_2, 1)->ShowGWnd(false);



    this->m_IRM.GetResObj<CIFStatic>(GDR_SKILL_FRAME_FELLOW_3, 1)->ShowGWnd(false);
    this->m_IRM.GetResObj<CIFSlotWithHelp>(GDR_SKILL_SLOT_FELLOW_3, 1)->ShowGWnd(false);


    this->m_IRM.GetResObj<CIFStatic>(GDR_SKILL_FRAME_FELLOW_4, 1)->ShowGWnd(false);
    this->m_IRM.GetResObj<CIFSlotWithHelp>(GDR_SKILL_SLOT_FELLOW_4, 1)->ShowGWnd(false);


    this->m_IRM.GetResObj<CIFStatic>(GDR_SKILL_FRAME_FELLOW_5, 1)->ShowGWnd(false);
    this->m_IRM.GetResObj<CIFSlotWithHelp>(GDR_SKILL_SLOT_FELLOW_5, 1)->ShowGWnd(false);

    return b;
}

void CIFCOSInfo::FUN_007a5610(int i) {

    reinterpret_cast<void(__thiscall*)(CIFCOSInfo*, int)>(0x007a5610)(this, i);
    CICharactor* pUser = GetCharacterObjectByID_MAYBE(i);
    if (pUser != NULL) {
        if(m_CustomDataManager->m_RefFellowPetSystem.find(pUser->GetCommonData()->NameStrID) != m_CustomDataManager->m_RefFellowPetSystem.end())
        {

            this->m_IRM.GetResObj<CIFStatic>(25, 1)->ShowGWnd(true);
            this->m_IRM.GetResObj<CIFStatic>(27, 1)->ShowGWnd(true);
            this->m_IRM.GetResObj<CIFButton>(26, 1)->ShowGWnd(true);



            this->m_IRM.GetResObj<CIFButton>(26, 1)->MoveGWnd(this->GetPos().x + 232, this->GetPos().y + 35);
            this->m_IRM.GetResObj<CIFStatic>(27, 1)->MoveGWnd(this->GetPos().x + 45, this->GetPos().y + 40);
            this->m_IRM.GetResObj<CIFStatic>(25, 1)->MoveGWnd(this->GetPos().x + 25, this->GetPos().y + 34);

            this->m_IRM.GetResObj<CIFStatic>(30, 1)->MoveGWnd(this->GetPos().x + 24, this->GetPos().y + 67);
            this->m_IRM.GetResObj<CIFStatic>(31, 1)->MoveGWnd(this->GetPos().x + 24, this->GetPos().y + 97);
            this->m_IRM.GetResObj<CIFStatic>(32, 1)->MoveGWnd(this->GetPos().x + 24, this->GetPos().y + 126);

            this->m_IRM.GetResObj<CIFGauge>(35, 1)->MoveGWnd(this->GetPos().x + 75, this->GetPos().y + 73);
            this->m_IRM.GetResObj<CIFGauge>(36, 1)->MoveGWnd(this->GetPos().x + 75, this->GetPos().y + 103);
            this->m_IRM.GetResObj<CIFGauge>(37, 1)->MoveGWnd(this->GetPos().x + 75, this->GetPos().y + 132);

            this->m_IRM.GetResObj<CIFStatic>(40, 1)->MoveGWnd(this->GetPos().x + 36, this->GetPos().y + 66);
            this->m_IRM.GetResObj<CIFStatic>(40, 1)->SetGWndSize(24, 24);
            this->m_IRM.GetResObj<CIFStatic>(40, 1)->SetText(L"");
            this->m_IRM.GetResObj<CIFStatic>(40, 1)->TB_Func_13("clientlibrary\\fellowpets\\pet_icon_hp.ddj", 1, 1);

            this->m_IRM.GetResObj<CIFStatic>(1031, 1)->SetText(KmtGetText(L"UIIT_KMT_UNIQUE_SKILLS"));
            this->m_IRM.GetResObj<CIFStatic>(1033, 1)->SetText(KmtGetText(L"UIIT_KMT_READIED_SKILLS"));
            this->m_IRM.GetResObj<CIFStatic>(1035, 1)->SetText(KmtGetText(L"UIIT_KMT_PET2_SKILLS"));



            if(this->m_IRM.GetResObj(200, 1) != NULL)
            {
                this->m_IRM.GetResObj(200, 1)->ShowGWnd(false);
            }

            std::n_wstring str = TSM_GETTEXTPTR(L"PARAM_HP");
            this->m_IRM.GetResObj<CIFStatic>(40, 1)->SetTooltip(str);
            this->m_IRM.GetResObj<CIFStatic>(40, 1)->SetStyleThingy(TOOLTIP);


            this->m_IRM.GetResObj<CIFStatic>(41, 1)->MoveGWnd(this->GetPos().x + 82, this->GetPos().y + 74);

            this->m_IRM.GetResObj<CIFStatic>(42, 1)->MoveGWnd(this->GetPos().x + 36, this->GetPos().y + 94);
            this->m_IRM.GetResObj<CIFStatic>(42, 1)->SetGWndSize(24, 24);
            this->m_IRM.GetResObj<CIFStatic>(42, 1)->SetText(L"");
            this->m_IRM.GetResObj<CIFStatic>(42, 1)->TB_Func_13("clientlibrary\\fellowpets\\pet_icon_hgp.ddj", 1, 1);

            std::n_wstring str2 = TSM_GETTEXTPTR(L"UIIT_STT_COSNEWUI_BASICINFO_HGP");
            this->m_IRM.GetResObj<CIFStatic>(42, 1)->SetTooltip(str2);
            this->m_IRM.GetResObj<CIFStatic>(42, 1)->SetStyleThingy(TOOLTIP);




            this->m_IRM.GetResObj<CIFStatic>(43, 1)->MoveGWnd(this->GetPos().x + 82, this->GetPos().y + 105);

            this->m_IRM.GetResObj<CIFStatic>(44, 1)->MoveGWnd(this->GetPos().x + 36, this->GetPos().y + 124);
            this->m_IRM.GetResObj<CIFStatic>(44, 1)->SetGWndSize(24, 24);
            this->m_IRM.GetResObj<CIFStatic>(44, 1)->SetText(L"");
            this->m_IRM.GetResObj<CIFStatic>(44, 1)->TB_Func_13("clientlibrary\\fellowpets\\pet_icon_exp.ddj", 1, 1);

            std::n_wstring str3 = TSM_GETTEXTPTR(L"UIO_CHARINFO_STT_EXP");
            this->m_IRM.GetResObj<CIFStatic>(44, 1)->SetTooltip(str3);
            this->m_IRM.GetResObj<CIFStatic>(44, 1)->SetStyleThingy(TOOLTIP);



            this->m_IRM.GetResObj<CIFStatic>(45, 1)->MoveGWnd(this->GetPos().x + 82, this->GetPos().y + 133);

            if(this->m_IRM.GetResObj<CIFGauge>(36, 1)->m_valueFg <= 0.30)
            {
                this->m_IRM.GetResObj<CIFGauge>(36, 1)->TB_Func_13("clientlibrary\\fellowpets\\pt_hgp_bad.ddj", 1, 1);
            }
            else if(this->m_IRM.GetResObj<CIFGauge>(36, 1)->m_valueFg > 0.30 && this->m_IRM.GetResObj<CIFGauge>(36, 1)->m_valueFg <= 0.80)
            {
                this->m_IRM.GetResObj<CIFGauge>(36, 1)->TB_Func_13("clientlibrary\\fellowpets\\pt_mp.ddj", 1, 1);

            }
            else if(this->m_IRM.GetResObj<CIFGauge>(36, 1)->m_valueFg > 0.80)
            {
                this->m_IRM.GetResObj<CIFGauge>(36, 1)->TB_Func_13("clientlibrary\\fellowpets\\pt_hgp_good.ddj", 1, 1);

            }



            this->m_IRM.GetResObj<CIFStatic>(60, 1)->ShowGWnd(false);
            this->m_IRM.GetResObj<CIFStatic>(61, 1)->ShowGWnd(false);
            this->m_IRM.GetResObj<CIFStatic>(62, 1)->ShowGWnd(false);
            this->m_IRM.GetResObj<CIFStatic>(63, 1)->ShowGWnd(false);

            this->m_IRM.GetResObj<CIFStatic>(65, 1)->ShowGWnd(false);
            this->m_IRM.GetResObj<CIFStatic>(66, 1)->ShowGWnd(false);
            this->m_IRM.GetResObj<CIFStatic>(67, 1)->ShowGWnd(false);
            this->m_IRM.GetResObj<CIFStatic>(68, 1)->ShowGWnd(false);
            this->m_IRM.GetResObj<CIFStatic>(71, 1)->ShowGWnd(false);

            this->m_IRM.GetResObj<CIFStatic>(72, 1)->ShowGWnd(false);
            this->m_IRM.GetResObj<CIFStatic>(73, 1)->ShowGWnd(false);
            this->m_IRM.GetResObj<CIFStatic>(76, 1)->ShowGWnd(false);
            this->m_IRM.GetResObj<CIFStatic>(77, 1)->ShowGWnd(false);
            this->m_IRM.GetResObj<CIFStatic>(78, 1)->ShowGWnd(false);

            this->m_IRM.GetResObj<CIFFrame>(10, 1)->SetGWndSize(315, 383);
            this->m_IRM.GetResObj<CIFRenderStatic>(GDR_RENDER_STATIC , 1)->ShowGWnd(true);

            this->m_IRM.GetResObj<CIFNormalTile>(5, 1)->SetGWndSize(267, 351);

            this->m_IRM.GetResObj<CIFRenderStatic>(GDR_RENDER_STATIC , 1)->ShowGWnd(true);
            this->m_IRM.GetResObj<CIFStatic>(1001, 1)->ShowGWnd(true);
            this->m_IRM.GetResObj<CIFStatic>(1002 , 1)->ShowGWnd(true);
            this->m_IRM.GetResObj<CIFStatic>(1003 , 1)->ShowGWnd(true);


            this->m_IRM.GetResObj<CIFRenderStatic>(GDR_RENDER_STATIC , 1)->Test1();

            this->m_IRM.GetResObj<CIFRenderStatic>(GDR_RENDER_STATIC , 1)->Clear();
            this->m_IRM.GetResObj<CIFRenderStatic>(GDR_RENDER_STATIC , 1)->FUN_00561780(pUser->GetCommonData()->RefObjectId);
            this->m_IRM.GetResObj<CIFRenderStatic>(GDR_RENDER_STATIC , 1)->Test1();

            undefined4 uStack12;
            uStack12 = 0;
            this->m_IRM.GetResObj<CIFRenderStatic>(GDR_RENDER_STATIC , 1)->Test(&uStack12);
            this->m_IRM.GetResObj<CIFRenderStatic>(GDR_RENDER_STATIC , 1)->Test2(&uStack12, 0x420d1000);

            this->m_IRM.GetResObj<CIFRenderStatic>(GDR_RENDER_STATIC , 1)->field_0x430 = 0x43fa0000;
            this->m_IRM.GetResObj<CIFRenderStatic>(GDR_RENDER_STATIC , 1)->Test3(0, 0);

            this->m_IRM.GetResObj<CIFRenderStatic>(GDR_RENDER_STATIC , 1)->yukariasagi = 13.000;
            this->m_IRM.GetResObj<CIFRenderStatic>(GDR_RENDER_STATIC , 1)->N00000609 = 1;

            this->m_IRM.GetResObj<CIFStatic>(GDR_INFO_FRAME, 1)->ShowGWnd(false);
            this->m_IRM.GetResObj<CIFStatic>(GDR_STATS, 1)->ShowGWnd(false);
            this->m_IRM.GetResObj<CIFStatic>(GDR_STR, 1)->ShowGWnd(false);
            this->m_IRM.GetResObj<CIFStatic>(GDR_STR_VALUE, 1)->ShowGWnd(false);
            this->m_IRM.GetResObj<CIFStatic>(GDR_INT, 1)->ShowGWnd(false);
            this->m_IRM.GetResObj<CIFStatic>(GDR_INT_VALUE, 1)->ShowGWnd(false);
            this->m_IRM.GetResObj<CIFStatic>(GDR_ATTACK, 1)->ShowGWnd(false);

            this->m_IRM.GetResObj<CIFStatic>(GDR_CRT, 1)->ShowGWnd(false);
            this->m_IRM.GetResObj<CIFStatic>(GDR_CRT_VALUE, 1)->ShowGWnd(false);
            this->m_IRM.GetResObj<CIFStatic>(GDR_DEFEND, 1)->ShowGWnd(false);

            this->m_IRM.GetResObj<CIFStatic>(GDR_BR, 1)->ShowGWnd(false);
            this->m_IRM.GetResObj<CIFStatic>(GDR_BR_VALUE, 1)->ShowGWnd(false);

            this->m_IRM.GetResObj<CIFStatic>(GDR_SLOT1, 1)->ShowGWnd(false);
            this->m_IRM.GetResObj<CIFStatic>(GDR_SLOT2, 1)->ShowGWnd(false);
            this->m_IRM.GetResObj<CIFStatic>(GDR_SLOT3, 1)->ShowGWnd(false);
            this->m_IRM.GetResObj<CIFStatic>(GDR_SLOT4, 1)->ShowGWnd(false);

            this->m_IRM.GetResObj<CIFStatic>(GDR_SLOT5, 1)->ShowGWnd(false);
            this->m_IRM.GetResObj<CIFStatic>(GDR_SLOT6, 1)->ShowGWnd(false);
            this->m_IRM.GetResObj<CIFStatic>(GDR_SLOT7, 1)->ShowGWnd(false);
            this->m_IRM.GetResObj<CIFStatic>(GDR_SLOT8, 1)->ShowGWnd(false);
            this->m_IRM.GetResObj<CIFFrame>(GDR_SKILL_FRAME, 1)->ShowGWnd(false);
            this->m_IRM.GetResObj<CIFStatic>(GDR_SKILL_HEADER, 1)->ShowGWnd(false);

            this->m_IRM.GetResObj<CIFFrame>(GDR_SKILL_FRAME2, 1)->ShowGWnd(false);
            this->m_IRM.GetResObj<CIFStatic>(GDR_SKILL_HEADER2, 1)->ShowGWnd(false);

            this->m_IRM.GetResObj<CIFFrame>(GDR_SKILL_FRAME3, 1)->ShowGWnd(false);
            this->m_IRM.GetResObj<CIFStatic>(GDR_SKILL_HEADER3, 1)->ShowGWnd(false);

            this->m_IRM.GetResObj<CIFStatic>(GDR_SKILL_FRAME_UNIQUE_1, 1)->ShowGWnd(false);
            this->m_IRM.GetResObj<CIFSlotWithHelp>(GDR_SKILL_SLOT_UNIQUE_1, 1)->ShowGWnd(false);

            this->m_IRM.GetResObj<CIFStatic>(GDR_SKILL_FRAME_UNIQUE_2, 1)->ShowGWnd(false);
            this->m_IRM.GetResObj<CIFSlotWithHelp>(GDR_SKILL_SLOT_UNIQUE_2, 1)->ShowGWnd(false);



            this->m_IRM.GetResObj<CIFStatic>(GDR_SKILL_FRAME_UNIQUE_3, 1)->ShowGWnd(false);
            this->m_IRM.GetResObj<CIFSlotWithHelp>(GDR_SKILL_SLOT_UNIQUE_3, 1)->ShowGWnd(false);


            this->m_IRM.GetResObj<CIFStatic>(GDR_SKILL_FRAME_UNIQUE_4, 1)->ShowGWnd(false);
            this->m_IRM.GetResObj<CIFSlotWithHelp>(GDR_SKILL_SLOT_UNIQUE_4, 1)->ShowGWnd(false);


            this->m_IRM.GetResObj<CIFStatic>(GDR_SKILL_FRAME_UNIQUE_5, 1)->ShowGWnd(false);
            this->m_IRM.GetResObj<CIFSlotWithHelp>(GDR_SKILL_SLOT_UNIQUE_5, 1)->ShowGWnd(false);


            this->m_IRM.GetResObj<CIFStatic>(GDR_SKILL_FRAME_READIED_1, 1)->ShowGWnd(false);
            this->m_IRM.GetResObj<CIFSlotWithHelp>(GDR_SKILL_SLOT_READIED_1, 1)->ShowGWnd(false);

            this->m_IRM.GetResObj<CIFStatic>(GDR_SKILL_FRAME_READIED_2, 1)->ShowGWnd(false);
            this->m_IRM.GetResObj<CIFSlotWithHelp>(GDR_SKILL_SLOT_READIED_2, 1)->ShowGWnd(false);



            this->m_IRM.GetResObj<CIFStatic>(GDR_SKILL_FRAME_READIED_3, 1)->ShowGWnd(false);
            this->m_IRM.GetResObj<CIFSlotWithHelp>(GDR_SKILL_SLOT_READIED_3, 1)->ShowGWnd(false);


            this->m_IRM.GetResObj<CIFStatic>(GDR_SKILL_FRAME_READIED_4, 1)->ShowGWnd(false);
            this->m_IRM.GetResObj<CIFSlotWithHelp>(GDR_SKILL_SLOT_READIED_4, 1)->ShowGWnd(false);


            this->m_IRM.GetResObj<CIFStatic>(GDR_SKILL_FRAME_READIED_5, 1)->ShowGWnd(false);
            this->m_IRM.GetResObj<CIFSlotWithHelp>(GDR_SKILL_SLOT_READIED_5, 1)->ShowGWnd(false);

            this->m_IRM.GetResObj<CIFStatic>(GDR_SKILL_FRAME_FELLOW_1, 1)->ShowGWnd(false);
            this->m_IRM.GetResObj<CIFSlotWithHelp>(GDR_SKILL_SLOT_FELLOW_1, 1)->ShowGWnd(false);

            this->m_IRM.GetResObj<CIFStatic>(GDR_SKILL_FRAME_FELLOW_2, 1)->ShowGWnd(false);
            this->m_IRM.GetResObj<CIFSlotWithHelp>(GDR_SKILL_SLOT_FELLOW_2, 1)->ShowGWnd(false);



            this->m_IRM.GetResObj<CIFStatic>(GDR_SKILL_FRAME_FELLOW_3, 1)->ShowGWnd(false);
            this->m_IRM.GetResObj<CIFSlotWithHelp>(GDR_SKILL_SLOT_FELLOW_3, 1)->ShowGWnd(false);


            this->m_IRM.GetResObj<CIFStatic>(GDR_SKILL_FRAME_FELLOW_4, 1)->ShowGWnd(false);
            this->m_IRM.GetResObj<CIFSlotWithHelp>(GDR_SKILL_SLOT_FELLOW_4, 1)->ShowGWnd(false);


            this->m_IRM.GetResObj<CIFStatic>(GDR_SKILL_FRAME_FELLOW_5, 1)->ShowGWnd(false);
            this->m_IRM.GetResObj<CIFSlotWithHelp>(GDR_SKILL_SLOT_FELLOW_5, 1)->ShowGWnd(false);

        }
        else
        {
            this->m_IRM.GetResObj<CIFButton>(26, 1)->MoveGWnd(this->GetPos().x + 232, this->GetPos().y + 41);
            this->m_IRM.GetResObj<CIFStatic>(27, 1)->MoveGWnd(this->GetPos().x + 45, this->GetPos().y + 47);
            this->m_IRM.GetResObj<CIFStatic>(25, 1)->MoveGWnd(this->GetPos().x + 25, this->GetPos().y + 41);
            this->m_IRM.GetResObj<CIFStatic>(30, 1)->MoveGWnd(this->GetPos().x + 24, this->GetPos().y + 83);
            this->m_IRM.GetResObj<CIFStatic>(31, 1)->MoveGWnd(this->GetPos().x + 24, this->GetPos().y + 113);
            this->m_IRM.GetResObj<CIFStatic>(32, 1)->MoveGWnd(this->GetPos().x + 24, this->GetPos().y + 142);

            this->m_IRM.GetResObj<CIFGauge>(35, 1)->MoveGWnd(this->GetPos().x + 75, this->GetPos().y + 89);
            this->m_IRM.GetResObj<CIFGauge>(36, 1)->MoveGWnd(this->GetPos().x + 75, this->GetPos().y + 119);
            this->m_IRM.GetResObj<CIFGauge>(37, 1)->MoveGWnd(this->GetPos().x + 75, this->GetPos().y + 148);
            this->m_IRM.GetResObj<CIFStatic>(40, 1)->MoveGWnd(this->GetPos().x + 36, this->GetPos().y + 91);
            this->m_IRM.GetResObj<CIFStatic>(40, 1)->SetGWndSize(28, 11);

            this->m_IRM.GetResObj<CIFStatic>(40, 1)->SetText(TSM_GETTEXTPTR(L"PARAM_HP"));
            this->m_IRM.GetResObj<CIFStatic>(40, 1)->TB_Func_13("", 0, 0);
            this->m_IRM.GetResObj<CIFStatic>(40, 1)->SetStyleThingy(NONE);

            this->m_IRM.GetResObj<CIFStatic>(41, 1)->MoveGWnd(this->GetPos().x + 82, this->GetPos().y + 90);

            this->m_IRM.GetResObj<CIFStatic>(42, 1)->MoveGWnd(this->GetPos().x + 36, this->GetPos().y + 121);
            this->m_IRM.GetResObj<CIFStatic>(42, 1)->SetGWndSize(28, 11);

            this->m_IRM.GetResObj<CIFStatic>(42, 1)->SetText(TSM_GETTEXTPTR(L"UIIT_STT_COSNEWUI_BASICINFO_HGP"));
            this->m_IRM.GetResObj<CIFStatic>(42, 1)->TB_Func_13("", 0, 0);
            this->m_IRM.GetResObj<CIFStatic>(42, 1)->SetStyleThingy(NONE);


            this->m_IRM.GetResObj<CIFStatic>(43, 1)->MoveGWnd(this->GetPos().x + 82, this->GetPos().y + 121);

            this->m_IRM.GetResObj<CIFStatic>(44, 1)->MoveGWnd(this->GetPos().x + 36, this->GetPos().y + 149);
            this->m_IRM.GetResObj<CIFStatic>(44, 1)->SetGWndSize(28, 11);

            this->m_IRM.GetResObj<CIFStatic>(44, 1)->SetText(TSM_GETTEXTPTR(L"UIO_CHARINFO_STT_EXP"));
            this->m_IRM.GetResObj<CIFStatic>(44, 1)->TB_Func_13("", 0, 0);
            this->m_IRM.GetResObj<CIFStatic>(44, 1)->SetStyleThingy(NONE);


            this->m_IRM.GetResObj<CIFStatic>(45, 1)->MoveGWnd(this->GetPos().x + 82, this->GetPos().y + 149);

            this->m_IRM.GetResObj<CIFGauge>(36, 1)->TB_Func_13("interface\\pet\\pt_hgp.ddj", 1, 1);

            this->m_IRM.GetResObj<CIFFrame>(10, 1)->SetGWndSize(315, 297);

            this->m_IRM.GetResObj<CIFNormalTile>(5, 1)->SetGWndSize(267, 265);
            this->m_IRM.GetResObj<CIFRenderStatic>(GDR_RENDER_STATIC , 1)->ShowGWnd(false);
            this->m_IRM.GetResObj<CIFStatic>(1001, 1)->ShowGWnd(false);
            this->m_IRM.GetResObj<CIFStatic>(1002, 1)->ShowGWnd(false);
            this->m_IRM.GetResObj<CIFStatic>(1003, 1)->ShowGWnd(false);
            this->m_IRM.GetResObj<CIFStatic>(1004, 1)->ShowGWnd(false);
            this->m_IRM.GetResObj<CIFStatic>(GDR_STATS, 1)->ShowGWnd(false);
            this->m_IRM.GetResObj<CIFStatic>(GDR_STR, 1)->ShowGWnd(false);
            this->m_IRM.GetResObj<CIFStatic>(GDR_STR_VALUE, 1)->ShowGWnd(false);
            this->m_IRM.GetResObj<CIFStatic>(GDR_INT, 1)->ShowGWnd(false);
            this->m_IRM.GetResObj<CIFStatic>(GDR_INT_VALUE, 1)->ShowGWnd(false);
            this->m_IRM.GetResObj<CIFStatic>(GDR_ATTACK, 1)->ShowGWnd(false);
            this->m_IRM.GetResObj<CIFStatic>(GDR_CRT, 1)->ShowGWnd(false);
            this->m_IRM.GetResObj<CIFStatic>(GDR_CRT_VALUE, 1)->ShowGWnd(false);
            this->m_IRM.GetResObj<CIFStatic>(GDR_DEFEND, 1)->ShowGWnd(false);

            this->m_IRM.GetResObj<CIFStatic>(GDR_BR, 1)->ShowGWnd(false);
            this->m_IRM.GetResObj<CIFStatic>(GDR_BR_VALUE, 1)->ShowGWnd(false);

            this->m_IRM.GetResObj<CIFStatic>(GDR_SLOT1, 1)->ShowGWnd(false);
            this->m_IRM.GetResObj<CIFStatic>(GDR_SLOT2, 1)->ShowGWnd(false);

            this->m_IRM.GetResObj<CIFStatic>(GDR_SLOT3, 1)->ShowGWnd(false);
            this->m_IRM.GetResObj<CIFStatic>(GDR_SLOT4, 1)->ShowGWnd(false);

            this->m_IRM.GetResObj<CIFStatic>(GDR_SLOT5, 1)->ShowGWnd(false);
            this->m_IRM.GetResObj<CIFStatic>(GDR_SLOT6, 1)->ShowGWnd(false);
            this->m_IRM.GetResObj<CIFStatic>(GDR_SLOT7, 1)->ShowGWnd(false);
            this->m_IRM.GetResObj<CIFStatic>(GDR_SLOT8, 1)->ShowGWnd(false);
            this->m_IRM.GetResObj<CIFFrame>(GDR_SKILL_FRAME, 1)->ShowGWnd(false);
            this->m_IRM.GetResObj<CIFStatic>(GDR_SKILL_HEADER, 1)->ShowGWnd(false);

            this->m_IRM.GetResObj<CIFFrame>(GDR_SKILL_FRAME2, 1)->ShowGWnd(false);
            this->m_IRM.GetResObj<CIFStatic>(GDR_SKILL_HEADER2, 1)->ShowGWnd(false);

            this->m_IRM.GetResObj<CIFFrame>(GDR_SKILL_FRAME3, 1)->ShowGWnd(false);
            this->m_IRM.GetResObj<CIFStatic>(GDR_SKILL_HEADER3, 1)->ShowGWnd(false);

            this->m_IRM.GetResObj<CIFStatic>(GDR_SKILL_FRAME_UNIQUE_1, 1)->ShowGWnd(false);
            this->m_IRM.GetResObj<CIFSlotWithHelp>(GDR_SKILL_SLOT_UNIQUE_1, 1)->ShowGWnd(false);

            this->m_IRM.GetResObj<CIFStatic>(GDR_SKILL_FRAME_UNIQUE_2, 1)->ShowGWnd(false);
            this->m_IRM.GetResObj<CIFSlotWithHelp>(GDR_SKILL_SLOT_UNIQUE_2, 1)->ShowGWnd(false);



            this->m_IRM.GetResObj<CIFStatic>(GDR_SKILL_FRAME_UNIQUE_3, 1)->ShowGWnd(false);
            this->m_IRM.GetResObj<CIFSlotWithHelp>(GDR_SKILL_SLOT_UNIQUE_3, 1)->ShowGWnd(false);


            this->m_IRM.GetResObj<CIFStatic>(GDR_SKILL_FRAME_UNIQUE_4, 1)->ShowGWnd(false);
            this->m_IRM.GetResObj<CIFSlotWithHelp>(GDR_SKILL_SLOT_UNIQUE_4, 1)->ShowGWnd(false);


            this->m_IRM.GetResObj<CIFStatic>(GDR_SKILL_FRAME_UNIQUE_5, 1)->ShowGWnd(false);
            this->m_IRM.GetResObj<CIFSlotWithHelp>(GDR_SKILL_SLOT_UNIQUE_5, 1)->ShowGWnd(false);


            this->m_IRM.GetResObj<CIFStatic>(GDR_SKILL_FRAME_READIED_1, 1)->ShowGWnd(false);
            this->m_IRM.GetResObj<CIFSlotWithHelp>(GDR_SKILL_SLOT_READIED_1, 1)->ShowGWnd(false);

            this->m_IRM.GetResObj<CIFStatic>(GDR_SKILL_FRAME_READIED_2, 1)->ShowGWnd(false);
            this->m_IRM.GetResObj<CIFSlotWithHelp>(GDR_SKILL_SLOT_READIED_2, 1)->ShowGWnd(false);



            this->m_IRM.GetResObj<CIFStatic>(GDR_SKILL_FRAME_READIED_3, 1)->ShowGWnd(false);
            this->m_IRM.GetResObj<CIFSlotWithHelp>(GDR_SKILL_SLOT_READIED_3, 1)->ShowGWnd(false);


            this->m_IRM.GetResObj<CIFStatic>(GDR_SKILL_FRAME_READIED_4, 1)->ShowGWnd(false);
            this->m_IRM.GetResObj<CIFSlotWithHelp>(GDR_SKILL_SLOT_READIED_4, 1)->ShowGWnd(false);


            this->m_IRM.GetResObj<CIFStatic>(GDR_SKILL_FRAME_READIED_5, 1)->ShowGWnd(false);
            this->m_IRM.GetResObj<CIFSlotWithHelp>(GDR_SKILL_SLOT_READIED_5, 1)->ShowGWnd(false);

            this->m_IRM.GetResObj<CIFStatic>(GDR_SKILL_FRAME_FELLOW_1, 1)->ShowGWnd(false);
            this->m_IRM.GetResObj<CIFSlotWithHelp>(GDR_SKILL_SLOT_FELLOW_1, 1)->ShowGWnd(false);

            this->m_IRM.GetResObj<CIFStatic>(GDR_SKILL_FRAME_FELLOW_2, 1)->ShowGWnd(false);
            this->m_IRM.GetResObj<CIFSlotWithHelp>(GDR_SKILL_SLOT_FELLOW_2, 1)->ShowGWnd(false);



            this->m_IRM.GetResObj<CIFStatic>(GDR_SKILL_FRAME_FELLOW_3, 1)->ShowGWnd(false);
            this->m_IRM.GetResObj<CIFSlotWithHelp>(GDR_SKILL_SLOT_FELLOW_3, 1)->ShowGWnd(false);


            this->m_IRM.GetResObj<CIFStatic>(GDR_SKILL_FRAME_FELLOW_4, 1)->ShowGWnd(false);
            this->m_IRM.GetResObj<CIFSlotWithHelp>(GDR_SKILL_SLOT_FELLOW_4, 1)->ShowGWnd(false);


            this->m_IRM.GetResObj<CIFStatic>(GDR_SKILL_FRAME_FELLOW_5, 1)->ShowGWnd(false);
            this->m_IRM.GetResObj<CIFSlotWithHelp>(GDR_SKILL_SLOT_FELLOW_5, 1)->ShowGWnd(false);

        }
    }
}

void CIFCOSInfo::HideTab2(){

    this->m_IRM.GetResObj<CIFGauge>(35, 1)->ShowGWnd(false);
    this->m_IRM.GetResObj<CIFGauge>(36, 1)->ShowGWnd(false);
    this->m_IRM.GetResObj<CIFGauge>(37, 1)->ShowGWnd(false);


    this->m_IRM.GetResObj<CIFStatic>(25, 1)->ShowGWnd(false);
    this->m_IRM.GetResObj<CIFStatic>(27, 1)->ShowGWnd(false);
    this->m_IRM.GetResObj<CIFButton>(26, 1)->ShowGWnd(false);


    this->m_IRM.GetResObj<CIFStatic>(30, 1)->ShowGWnd(false);
    this->m_IRM.GetResObj<CIFStatic>(31, 1)->ShowGWnd(false);
    this->m_IRM.GetResObj<CIFStatic>(32, 1)->ShowGWnd(false);


    this->m_IRM.GetResObj<CIFStatic>(40, 1)->ShowGWnd(false);
    this->m_IRM.GetResObj<CIFStatic>(41, 1)->ShowGWnd(false);
    this->m_IRM.GetResObj<CIFStatic>(42, 1)->ShowGWnd(false);
    this->m_IRM.GetResObj<CIFStatic>(43, 1)->ShowGWnd(false);
    this->m_IRM.GetResObj<CIFStatic>(44, 1)->ShowGWnd(false);
    this->m_IRM.GetResObj<CIFStatic>(45, 1)->ShowGWnd(false);


    this->m_IRM.GetResObj<CIFRenderStatic>(GDR_RENDER_STATIC , 1)->ShowGWnd(false);
    this->m_IRM.GetResObj<CIFStatic>(1001, 1)->ShowGWnd(false);
    this->m_IRM.GetResObj<CIFStatic>(1002, 1)->ShowGWnd(false);
    this->m_IRM.GetResObj<CIFStatic>(1003, 1)->ShowGWnd(false);



    this->m_IRM.GetResObj<CIFStatic>(60, 1)->ShowGWnd(false);
    this->m_IRM.GetResObj<CIFStatic>(61, 1)->ShowGWnd(false);
    this->m_IRM.GetResObj<CIFStatic>(62, 1)->ShowGWnd(false);
    this->m_IRM.GetResObj<CIFStatic>(63, 1)->ShowGWnd(false);

    this->m_IRM.GetResObj<CIFStatic>(65, 1)->ShowGWnd(false);
    this->m_IRM.GetResObj<CIFStatic>(66, 1)->ShowGWnd(false);
    this->m_IRM.GetResObj<CIFStatic>(67, 1)->ShowGWnd(false);
    this->m_IRM.GetResObj<CIFStatic>(68, 1)->ShowGWnd(false);

    this->m_IRM.GetResObj<CIFStatic>(71, 1)->ShowGWnd(false);
    this->m_IRM.GetResObj<CIFStatic>(72, 1)->ShowGWnd(false);
    this->m_IRM.GetResObj<CIFStatic>(73, 1)->ShowGWnd(false);

    this->m_IRM.GetResObj<CIFStatic>(76, 1)->ShowGWnd(false);
    this->m_IRM.GetResObj<CIFStatic>(77, 1)->ShowGWnd(false);
    this->m_IRM.GetResObj<CIFStatic>(78, 1)->ShowGWnd(false);


    this->m_IRM.GetResObj<CIFStatic>(1004, 1)->ShowGWnd(false);
    this->m_IRM.GetResObj<CIFStatic>(GDR_STATS, 1)->ShowGWnd(false);
    this->m_IRM.GetResObj<CIFStatic>(GDR_STR, 1)->ShowGWnd(false);
    this->m_IRM.GetResObj<CIFStatic>(GDR_STR_VALUE, 1)->ShowGWnd(false);
    this->m_IRM.GetResObj<CIFStatic>(GDR_INT, 1)->ShowGWnd(false);
    this->m_IRM.GetResObj<CIFStatic>(GDR_INT_VALUE, 1)->ShowGWnd(false);
    this->m_IRM.GetResObj<CIFStatic>(GDR_ATTACK, 1)->ShowGWnd(false);

    this->m_IRM.GetResObj<CIFStatic>(GDR_CRT, 1)->ShowGWnd(false);
    this->m_IRM.GetResObj<CIFStatic>(GDR_CRT_VALUE, 1)->ShowGWnd(false);
    this->m_IRM.GetResObj<CIFStatic>(GDR_DEFEND, 1)->ShowGWnd(false);


    this->m_IRM.GetResObj<CIFStatic>(GDR_BR, 1)->ShowGWnd(false);
    this->m_IRM.GetResObj<CIFStatic>(GDR_BR_VALUE, 1)->ShowGWnd(false);

    this->m_IRM.GetResObj<CIFStatic>(GDR_SLOT1, 1)->ShowGWnd(false);
    this->m_IRM.GetResObj<CIFStatic>(GDR_SLOT2, 1)->ShowGWnd(false);
    this->m_IRM.GetResObj<CIFStatic>(GDR_SLOT3, 1)->ShowGWnd(false);
    this->m_IRM.GetResObj<CIFStatic>(GDR_SLOT4, 1)->ShowGWnd(false);
    this->m_IRM.GetResObj<CIFStatic>(GDR_SLOT5, 1)->ShowGWnd(false);
    this->m_IRM.GetResObj<CIFStatic>(GDR_SLOT6, 1)->ShowGWnd(false);
    this->m_IRM.GetResObj<CIFStatic>(GDR_SLOT7, 1)->ShowGWnd(false);
    this->m_IRM.GetResObj<CIFStatic>(GDR_SLOT8, 1)->ShowGWnd(false);

    this->m_IRM.GetResObj<CIFFrame>(GDR_SKILL_FRAME, 1)->ShowGWnd(true);
    this->m_IRM.GetResObj<CIFStatic>(GDR_SKILL_HEADER, 1)->ShowGWnd(true);
    this->m_IRM.GetResObj<CIFStatic>(GDR_SKILL_HEADER, 1)->BringToFront();

    this->m_IRM.GetResObj<CIFFrame>(GDR_SKILL_FRAME2, 1)->ShowGWnd(true);
    this->m_IRM.GetResObj<CIFStatic>(GDR_SKILL_HEADER2, 1)->ShowGWnd(true);
    this->m_IRM.GetResObj<CIFStatic>(GDR_SKILL_HEADER2, 1)->BringToFront();

    this->m_IRM.GetResObj<CIFFrame>(GDR_SKILL_FRAME3, 1)->ShowGWnd(true);
    this->m_IRM.GetResObj<CIFStatic>(GDR_SKILL_HEADER3, 1)->ShowGWnd(true);
    this->m_IRM.GetResObj<CIFStatic>(GDR_SKILL_HEADER3, 1)->BringToFront();

    this->m_IRM.GetResObj<CIFStatic>(GDR_SKILL_FRAME_UNIQUE_1, 1)->ShowGWnd(true);
    this->m_IRM.GetResObj<CIFStatic>(GDR_SKILL_FRAME_UNIQUE_1, 1)->BringToFront();

    this->m_IRM.GetResObj<CIFSlotWithHelp>(GDR_SKILL_SLOT_UNIQUE_1, 1)->ShowGWnd(true);
    this->m_IRM.GetResObj<CIFSlotWithHelp>(GDR_SKILL_SLOT_UNIQUE_1, 1)->BringToFront();

    this->m_IRM.GetResObj<CIFStatic>(GDR_SKILL_FRAME_UNIQUE_2, 1)->ShowGWnd(true);
    this->m_IRM.GetResObj<CIFSlotWithHelp>(GDR_SKILL_SLOT_UNIQUE_2, 1)->ShowGWnd(true);

    this->m_IRM.GetResObj<CIFStatic>(GDR_SKILL_FRAME_UNIQUE_2, 1)->BringToFront();
    this->m_IRM.GetResObj<CIFSlotWithHelp>(GDR_SKILL_SLOT_UNIQUE_2, 1)->BringToFront();



    this->m_IRM.GetResObj<CIFStatic>(GDR_SKILL_FRAME_UNIQUE_3, 1)->ShowGWnd(true);
    this->m_IRM.GetResObj<CIFSlotWithHelp>(GDR_SKILL_SLOT_UNIQUE_3, 1)->ShowGWnd(true);


    this->m_IRM.GetResObj<CIFStatic>(GDR_SKILL_FRAME_UNIQUE_4, 1)->ShowGWnd(true);
    this->m_IRM.GetResObj<CIFSlotWithHelp>(GDR_SKILL_SLOT_UNIQUE_4, 1)->ShowGWnd(true);


    this->m_IRM.GetResObj<CIFStatic>(GDR_SKILL_FRAME_UNIQUE_5, 1)->ShowGWnd(true);
    this->m_IRM.GetResObj<CIFSlotWithHelp>(GDR_SKILL_SLOT_UNIQUE_5, 1)->ShowGWnd(true);



    this->m_IRM.GetResObj<CIFStatic>(GDR_SKILL_FRAME_UNIQUE_3, 1)->BringToFront();
    this->m_IRM.GetResObj<CIFSlotWithHelp>(GDR_SKILL_SLOT_UNIQUE_3, 1)->BringToFront();


    this->m_IRM.GetResObj<CIFStatic>(GDR_SKILL_FRAME_UNIQUE_4, 1)->BringToFront();
    this->m_IRM.GetResObj<CIFSlotWithHelp>(GDR_SKILL_SLOT_UNIQUE_4, 1)->BringToFront();


    this->m_IRM.GetResObj<CIFStatic>(GDR_SKILL_FRAME_UNIQUE_5, 1)->BringToFront();
    this->m_IRM.GetResObj<CIFSlotWithHelp>(GDR_SKILL_SLOT_UNIQUE_5, 1)->BringToFront();


    this->m_IRM.GetResObj<CIFStatic>(GDR_SKILL_FRAME_READIED_1, 1)->ShowGWnd(true);
    this->m_IRM.GetResObj<CIFSlotWithHelp>(GDR_SKILL_SLOT_READIED_1, 1)->ShowGWnd(true);

    this->m_IRM.GetResObj<CIFStatic>(GDR_SKILL_FRAME_READIED_2, 1)->ShowGWnd(true);
    this->m_IRM.GetResObj<CIFSlotWithHelp>(GDR_SKILL_SLOT_READIED_2, 1)->ShowGWnd(true);



    this->m_IRM.GetResObj<CIFStatic>(GDR_SKILL_FRAME_READIED_3, 1)->ShowGWnd(true);
    this->m_IRM.GetResObj<CIFSlotWithHelp>(GDR_SKILL_SLOT_READIED_3, 1)->ShowGWnd(true);


    this->m_IRM.GetResObj<CIFStatic>(GDR_SKILL_FRAME_READIED_4, 1)->ShowGWnd(true);
    this->m_IRM.GetResObj<CIFSlotWithHelp>(GDR_SKILL_SLOT_READIED_4, 1)->ShowGWnd(true);


    this->m_IRM.GetResObj<CIFStatic>(GDR_SKILL_FRAME_READIED_5, 1)->ShowGWnd(true);
    this->m_IRM.GetResObj<CIFSlotWithHelp>(GDR_SKILL_SLOT_READIED_5, 1)->ShowGWnd(true);


    this->m_IRM.GetResObj<CIFStatic>(GDR_SKILL_FRAME_READIED_1, 1)->BringToFront();
    this->m_IRM.GetResObj<CIFSlotWithHelp>(GDR_SKILL_SLOT_READIED_1, 1)->BringToFront();

    this->m_IRM.GetResObj<CIFStatic>(GDR_SKILL_FRAME_READIED_2, 1)->BringToFront();
    this->m_IRM.GetResObj<CIFSlotWithHelp>(GDR_SKILL_SLOT_READIED_2, 1)->BringToFront();



    this->m_IRM.GetResObj<CIFStatic>(GDR_SKILL_FRAME_READIED_3, 1)->BringToFront();
    this->m_IRM.GetResObj<CIFSlotWithHelp>(GDR_SKILL_SLOT_READIED_3, 1)->BringToFront();


    this->m_IRM.GetResObj<CIFStatic>(GDR_SKILL_FRAME_READIED_4, 1)->BringToFront();
    this->m_IRM.GetResObj<CIFSlotWithHelp>(GDR_SKILL_SLOT_READIED_4, 1)->BringToFront();


    this->m_IRM.GetResObj<CIFStatic>(GDR_SKILL_FRAME_READIED_5, 1)->BringToFront();
    this->m_IRM.GetResObj<CIFSlotWithHelp>(GDR_SKILL_SLOT_READIED_5, 1)->BringToFront();


    this->m_IRM.GetResObj<CIFStatic>(GDR_SKILL_FRAME_FELLOW_1, 1)->ShowGWnd(true);
    this->m_IRM.GetResObj<CIFSlotWithHelp>(GDR_SKILL_SLOT_FELLOW_1, 1)->ShowGWnd(true);

    this->m_IRM.GetResObj<CIFStatic>(GDR_SKILL_FRAME_FELLOW_2, 1)->ShowGWnd(true);
    this->m_IRM.GetResObj<CIFSlotWithHelp>(GDR_SKILL_SLOT_FELLOW_2, 1)->ShowGWnd(true);



    this->m_IRM.GetResObj<CIFStatic>(GDR_SKILL_FRAME_FELLOW_3, 1)->ShowGWnd(true);
    this->m_IRM.GetResObj<CIFSlotWithHelp>(GDR_SKILL_SLOT_FELLOW_3, 1)->ShowGWnd(true);


    this->m_IRM.GetResObj<CIFStatic>(GDR_SKILL_FRAME_FELLOW_4, 1)->ShowGWnd(true);
    this->m_IRM.GetResObj<CIFSlotWithHelp>(GDR_SKILL_SLOT_FELLOW_4, 1)->ShowGWnd(true);


    this->m_IRM.GetResObj<CIFStatic>(GDR_SKILL_FRAME_FELLOW_5, 1)->ShowGWnd(true);
    this->m_IRM.GetResObj<CIFSlotWithHelp>(GDR_SKILL_SLOT_FELLOW_5, 1)->ShowGWnd(true);


    this->m_IRM.GetResObj<CIFStatic>(GDR_SKILL_FRAME_FELLOW_1, 1)->BringToFront();
    this->m_IRM.GetResObj<CIFSlotWithHelp>(GDR_SKILL_SLOT_FELLOW_1, 1)->BringToFront();

    this->m_IRM.GetResObj<CIFStatic>(GDR_SKILL_FRAME_FELLOW_2, 1)->BringToFront();
    this->m_IRM.GetResObj<CIFSlotWithHelp>(GDR_SKILL_SLOT_FELLOW_2, 1)->BringToFront();



    this->m_IRM.GetResObj<CIFStatic>(GDR_SKILL_FRAME_FELLOW_3, 1)->BringToFront();
    this->m_IRM.GetResObj<CIFSlotWithHelp>(GDR_SKILL_SLOT_FELLOW_3, 1)->BringToFront();


    this->m_IRM.GetResObj<CIFStatic>(GDR_SKILL_FRAME_FELLOW_4, 1)->BringToFront();
    this->m_IRM.GetResObj<CIFSlotWithHelp>(GDR_SKILL_SLOT_FELLOW_4, 1)->BringToFront();


    this->m_IRM.GetResObj<CIFStatic>(GDR_SKILL_FRAME_FELLOW_5, 1)->BringToFront();
    this->m_IRM.GetResObj<CIFSlotWithHelp>(GDR_SKILL_SLOT_FELLOW_5, 1)->BringToFront();


    this->m_IRM.GetResObj<CIFSlotWithHelp>(GDR_SKILL_SLOT_READIED_1, 1)->TB_Func_13("clientlibrary\\fellowpets\\com_close_icon_box.ddj", 0, 0);
    this->m_IRM.GetResObj<CIFSlotWithHelp>(GDR_SKILL_SLOT_READIED_2, 1)->TB_Func_13("clientlibrary\\fellowpets\\com_close_icon_box.ddj", 1, 1);
    this->m_IRM.GetResObj<CIFSlotWithHelp>(GDR_SKILL_SLOT_READIED_3, 1)->TB_Func_13("clientlibrary\\fellowpets\\com_close_icon_box.ddj", 1, 1);
    this->m_IRM.GetResObj<CIFSlotWithHelp>(GDR_SKILL_SLOT_READIED_4, 1)->TB_Func_13("clientlibrary\\fellowpets\\com_close_icon_box.ddj", 1, 1);
    this->m_IRM.GetResObj<CIFSlotWithHelp>(GDR_SKILL_SLOT_READIED_5, 1)->TB_Func_13("clientlibrary\\fellowpets\\com_close_icon_box.ddj", 1, 1);

   // this->m_IRM.GetResObj<CIFSlotWithHelp>(GDR_SKILL_SLOT_FELLOW_1, 1)->TB_Func_13("clientlibrary\\fellowpets\\com_close_icon_box.ddj", 1, 1);
    this->m_IRM.GetResObj<CIFSlotWithHelp>(GDR_SKILL_SLOT_FELLOW_2, 1)->TB_Func_13("clientlibrary\\fellowpets\\com_close_icon_box.ddj", 1, 1);
    this->m_IRM.GetResObj<CIFSlotWithHelp>(GDR_SKILL_SLOT_FELLOW_3, 1)->TB_Func_13("clientlibrary\\fellowpets\\com_close_icon_box.ddj", 1, 1);
    this->m_IRM.GetResObj<CIFSlotWithHelp>(GDR_SKILL_SLOT_FELLOW_4, 1)->TB_Func_13("clientlibrary\\fellowpets\\com_close_icon_box.ddj", 1, 1);
    this->m_IRM.GetResObj<CIFSlotWithHelp>(GDR_SKILL_SLOT_FELLOW_5, 1)->TB_Func_13("clientlibrary\\fellowpets\\com_close_icon_box.ddj", 1, 1);

    this->m_IRM.GetResObj<CIFSlotWithHelp>(GDR_SKILL_SLOT_READIED_1, 1)->SetClickable(false);
    this->m_IRM.GetResObj<CIFSlotWithHelp>(GDR_SKILL_SLOT_READIED_2, 1)->SetClickable(false);
    this->m_IRM.GetResObj<CIFSlotWithHelp>(GDR_SKILL_SLOT_READIED_3, 1)->SetClickable(false);
    this->m_IRM.GetResObj<CIFSlotWithHelp>(GDR_SKILL_SLOT_READIED_4, 1)->SetClickable(false);
    this->m_IRM.GetResObj<CIFSlotWithHelp>(GDR_SKILL_SLOT_READIED_5, 1)->SetClickable(false);

    this->m_IRM.GetResObj<CIFSlotWithHelp>(GDR_SKILL_SLOT_FELLOW_1, 1)->SetClickable(false);
    this->m_IRM.GetResObj<CIFSlotWithHelp>(GDR_SKILL_SLOT_FELLOW_2, 1)->SetClickable(false);
    this->m_IRM.GetResObj<CIFSlotWithHelp>(GDR_SKILL_SLOT_FELLOW_3, 1)->SetClickable(false);
    this->m_IRM.GetResObj<CIFSlotWithHelp>(GDR_SKILL_SLOT_FELLOW_4, 1)->SetClickable(false);
    this->m_IRM.GetResObj<CIFSlotWithHelp>(GDR_SKILL_SLOT_FELLOW_5, 1)->SetClickable(false);

    this->m_IRM.GetResObj<CIFSlotWithHelp>(GDR_SKILL_SLOT_UNIQUE_1, 1)->SetDragable(false);
    this->m_IRM.GetResObj<CIFSlotWithHelp>(GDR_SKILL_SLOT_UNIQUE_2, 1)->SetDragable(false);
    this->m_IRM.GetResObj<CIFSlotWithHelp>(GDR_SKILL_SLOT_UNIQUE_3, 1)->SetDragable(false);
    this->m_IRM.GetResObj<CIFSlotWithHelp>(GDR_SKILL_SLOT_UNIQUE_4, 1)->SetDragable(false);
    this->m_IRM.GetResObj<CIFSlotWithHelp>(GDR_SKILL_SLOT_UNIQUE_5, 1)->SetDragable(false);

    this->m_IRM.GetResObj<CIFSlotWithHelp>(GDR_SKILL_SLOT_UNIQUE_1, 1)->SetClickable(false);
    this->m_IRM.GetResObj<CIFSlotWithHelp>(GDR_SKILL_SLOT_UNIQUE_2, 1)->SetClickable(false);
    this->m_IRM.GetResObj<CIFSlotWithHelp>(GDR_SKILL_SLOT_UNIQUE_3, 1)->SetClickable(false);
    this->m_IRM.GetResObj<CIFSlotWithHelp>(GDR_SKILL_SLOT_UNIQUE_4, 1)->SetClickable(false);
    this->m_IRM.GetResObj<CIFSlotWithHelp>(GDR_SKILL_SLOT_UNIQUE_5, 1)->SetClickable(false);

    /*this->m_IRM.GetResObj<CIFSlotWithHelp>(GDR_SKILL_SLOT_UNIQUE_1, 1)->TB_Func_13("clientlibrary\\fellowpets\\com_close_icon_box.ddj", 1, 1);
    this->m_IRM.GetResObj<CIFSlotWithHelp>(GDR_SKILL_SLOT_UNIQUE_2, 1)->TB_Func_13("clientlibrary\\fellowpets\\com_close_icon_box.ddj", 1, 1);
    this->m_IRM.GetResObj<CIFSlotWithHelp>(GDR_SKILL_SLOT_UNIQUE_3, 1)->TB_Func_13("clientlibrary\\fellowpets\\com_close_icon_box.ddj", 1, 1);
    this->m_IRM.GetResObj<CIFSlotWithHelp>(GDR_SKILL_SLOT_UNIQUE_4, 1)->TB_Func_13("clientlibrary\\fellowpets\\com_close_icon_box.ddj", 1, 1);
    this->m_IRM.GetResObj<CIFSlotWithHelp>(GDR_SKILL_SLOT_UNIQUE_5, 1)->TB_Func_13("clientlibrary\\fellowpets\\com_close_icon_box.ddj", 1, 1);
*/

    this->m_IRM.GetResObj<CIFSlotWithHelp>(GDR_SKILL_SLOT_UNIQUE_1, 1)->SlotisLocked = GDR_FELLOW_SKILL_SLOT_DEACTIVE;
    this->m_IRM.GetResObj<CIFSlotWithHelp>(GDR_SKILL_SLOT_UNIQUE_2, 1)->SlotisLocked = GDR_FELLOW_SKILL_SLOT_DEACTIVE;
    this->m_IRM.GetResObj<CIFSlotWithHelp>(GDR_SKILL_SLOT_UNIQUE_3, 1)->SlotisLocked = GDR_FELLOW_SKILL_SLOT_DEACTIVE;
    this->m_IRM.GetResObj<CIFSlotWithHelp>(GDR_SKILL_SLOT_UNIQUE_4, 1)->SlotisLocked = GDR_FELLOW_SKILL_SLOT_DEACTIVE;
    this->m_IRM.GetResObj<CIFSlotWithHelp>(GDR_SKILL_SLOT_UNIQUE_5, 1)->SlotisLocked = GDR_FELLOW_SKILL_SLOT_DEACTIVE;


    CICharactor* pUser = GetCharacterObjectByID_MAYBE(this->m_petUniqueID);
    if (pUser != NULL) {
        std::n_wstring NameStrID = pUser->GetCommonData()->NameStrID;
        if(m_CustomDataManager->m_RefFellowPetSystem.find(NameStrID) != m_CustomDataManager->m_RefFellowPetSystem.end())
        {
            if(m_CustomDataManager->m_RefFellowPetSystem[NameStrID].SkillID_1 > 0)
            {
                const CCharacterData *uqdata = NULL;
                uqdata = g_CGlobalDataManager->GetCharacter(pUser->GetCommonData()->RefObjectId);
                if(uqdata != NULL)
                {
                    if(uqdata->GetData().Level >= m_CustomDataManager->m_RefFellowPetSystem[NameStrID].Active_Level_1)
                    {
                        this->m_IRM.GetResObj<CIFSlotWithHelp>(GDR_SKILL_SLOT_UNIQUE_1, 1)->SetClickable(true);
                        this->m_IRM.GetResObj<CIFSlotWithHelp>(GDR_SKILL_SLOT_UNIQUE_1, 1)->N595 = 0;

                        this->m_IRM.GetResObj<CIFSlotWithHelp>(GDR_SKILL_SLOT_UNIQUE_1, 1)->SetType(73);
                        this->m_IRM.GetResObj<CIFSlotWithHelp>(GDR_SKILL_SLOT_UNIQUE_1, 1)->GetIconSlot(73, m_CustomDataManager->m_RefFellowPetSystem[NameStrID].SkillID_1);

                        if(m_Player->m_FellowSkillData.size() > 0)
                        {
                            if(m_Player->m_FellowSkillData[0].Enable_Skill_1 == 1)
                            {
                                this->m_IRM.GetResObj<CIFSlotWithHelp>(GDR_SKILL_SLOT_UNIQUE_1, 1)->SlotisLocked = GDR_FELLOW_SKILL_SLOT_ACTIVE;
                            }
                        }

                        if(m_CustomDataManager->m_RefFellowPetSystem[NameStrID].SkillType_1 == 0)
                        {
                            this->m_IRM.GetResObj<CIFSlotWithHelp>(GDR_SKILL_SLOT_UNIQUE_1, 1)->SetClickable(false);
                            this->m_IRM.GetResObj<CIFSlotWithHelp>(GDR_SKILL_SLOT_UNIQUE_1, 1)->SlotisLocked = GDR_FELLOW_SKILL_SLOT_ACTIVE;
                        }
                    }
                    else
                    {
                        this->m_IRM.GetResObj<CIFSlotWithHelp>(GDR_SKILL_SLOT_UNIQUE_1, 1)->SetClickable(true);
                        this->m_IRM.GetResObj<CIFSlotWithHelp>(GDR_SKILL_SLOT_UNIQUE_1, 1)->N595 = 2;

                        this->m_IRM.GetResObj<CIFSlotWithHelp>(GDR_SKILL_SLOT_UNIQUE_1, 1)->SetType(73);
                        this->m_IRM.GetResObj<CIFSlotWithHelp>(GDR_SKILL_SLOT_UNIQUE_1, 1)->GetIconSlot(73, m_CustomDataManager->m_RefFellowPetSystem[NameStrID].SkillID_1);

                        if(m_Player->m_FellowSkillData.size() > 0)
                        {
                            if(m_Player->m_FellowSkillData[0].Enable_Skill_1 == 1)
                            {
                                this->m_IRM.GetResObj<CIFSlotWithHelp>(GDR_SKILL_SLOT_UNIQUE_1, 1)->SlotisLocked = GDR_FELLOW_SKILL_SLOT_DEACTIVE;
                            }
                        }
                        if(m_CustomDataManager->m_RefFellowPetSystem[NameStrID].SkillType_1 == 0)
                        {
                            this->m_IRM.GetResObj<CIFSlotWithHelp>(GDR_SKILL_SLOT_UNIQUE_1, 1)->SetClickable(false);
                        }
                    }

                }
            }
            else
            {
                this->m_IRM.GetResObj<CIFSlotWithHelp>(GDR_SKILL_SLOT_UNIQUE_1, 1)->TB_Func_13("clientlibrary\\fellowpets\\com_close_icon_box.ddj", 1, 1);
            }
            if(m_CustomDataManager->m_RefFellowPetSystem[NameStrID].SkillID_2 > 0)
            {
                const CCharacterData *uqdata = NULL;
                uqdata = g_CGlobalDataManager->GetCharacter(pUser->GetCommonData()->RefObjectId);
                if(uqdata != NULL)
                {
                    if(uqdata->GetData().Level >= m_CustomDataManager->m_RefFellowPetSystem[NameStrID].Active_Level_2)
                    {
                        this->m_IRM.GetResObj<CIFSlotWithHelp>(GDR_SKILL_SLOT_UNIQUE_2, 1)->SetClickable(true);
                        this->m_IRM.GetResObj<CIFSlotWithHelp>(GDR_SKILL_SLOT_UNIQUE_2, 1)->N595 = 0;

                        this->m_IRM.GetResObj<CIFSlotWithHelp>(GDR_SKILL_SLOT_UNIQUE_2, 1)->SetType(73);
                        this->m_IRM.GetResObj<CIFSlotWithHelp>(GDR_SKILL_SLOT_UNIQUE_2, 1)->GetIconSlot(73, m_CustomDataManager->m_RefFellowPetSystem[NameStrID].SkillID_2);

                        if(m_Player->m_FellowSkillData.size() > 0)
                        {
                            if(m_Player->m_FellowSkillData[0].Enable_Skill_2 == 1)
                            {
                                this->m_IRM.GetResObj<CIFSlotWithHelp>(GDR_SKILL_SLOT_UNIQUE_2, 1)->SlotisLocked = GDR_FELLOW_SKILL_SLOT_ACTIVE;
                            }
                        }

                        if(m_CustomDataManager->m_RefFellowPetSystem[NameStrID].SkillType_2 == 0)
                        {
                            this->m_IRM.GetResObj<CIFSlotWithHelp>(GDR_SKILL_SLOT_UNIQUE_2, 1)->SetClickable(false);
                            this->m_IRM.GetResObj<CIFSlotWithHelp>(GDR_SKILL_SLOT_UNIQUE_2, 1)->SlotisLocked = GDR_FELLOW_SKILL_SLOT_ACTIVE;
                        }
                    }
                    else
                    {
                        this->m_IRM.GetResObj<CIFSlotWithHelp>(GDR_SKILL_SLOT_UNIQUE_2, 1)->SetClickable(true);
                        this->m_IRM.GetResObj<CIFSlotWithHelp>(GDR_SKILL_SLOT_UNIQUE_2, 1)->N595 = 2;

                        this->m_IRM.GetResObj<CIFSlotWithHelp>(GDR_SKILL_SLOT_UNIQUE_2, 1)->SetType(73);
                        this->m_IRM.GetResObj<CIFSlotWithHelp>(GDR_SKILL_SLOT_UNIQUE_2, 1)->GetIconSlot(73, m_CustomDataManager->m_RefFellowPetSystem[NameStrID].SkillID_2);

                        if(m_Player->m_FellowSkillData.size() > 0)
                        {
                            if(m_Player->m_FellowSkillData[0].Enable_Skill_2 == 1)
                            {
                                this->m_IRM.GetResObj<CIFSlotWithHelp>(GDR_SKILL_SLOT_UNIQUE_2, 1)->SlotisLocked = GDR_FELLOW_SKILL_SLOT_DEACTIVE;
                            }
                        }
                        if(m_CustomDataManager->m_RefFellowPetSystem[NameStrID].SkillType_2 == 0)
                        {
                            this->m_IRM.GetResObj<CIFSlotWithHelp>(GDR_SKILL_SLOT_UNIQUE_2, 1)->SetClickable(false);
                        }
                    }

                }
            }
            else
            {
                this->m_IRM.GetResObj<CIFSlotWithHelp>(GDR_SKILL_SLOT_UNIQUE_2, 1)->TB_Func_13("clientlibrary\\fellowpets\\com_close_icon_box.ddj", 1, 1);
            }
            if(m_CustomDataManager->m_RefFellowPetSystem[NameStrID].SkillID_3 > 0)
            {
                const CCharacterData *uqdata = NULL;
                uqdata = g_CGlobalDataManager->GetCharacter(pUser->GetCommonData()->RefObjectId);
                if(uqdata != NULL)
                {
                    if(uqdata->GetData().Level >= m_CustomDataManager->m_RefFellowPetSystem[NameStrID].Active_Level_3)
                    {
                        this->m_IRM.GetResObj<CIFSlotWithHelp>(GDR_SKILL_SLOT_UNIQUE_3, 1)->SetClickable(true);
                        this->m_IRM.GetResObj<CIFSlotWithHelp>(GDR_SKILL_SLOT_UNIQUE_3, 1)->N595 = 0;

                        this->m_IRM.GetResObj<CIFSlotWithHelp>(GDR_SKILL_SLOT_UNIQUE_3, 1)->SetType(73);
                        this->m_IRM.GetResObj<CIFSlotWithHelp>(GDR_SKILL_SLOT_UNIQUE_3, 1)->GetIconSlot(73, m_CustomDataManager->m_RefFellowPetSystem[NameStrID].SkillID_3);

                        if(m_Player->m_FellowSkillData.size() > 0)
                        {
                            if(m_Player->m_FellowSkillData[0].Enable_Skill_3 == 1)
                            {
                                this->m_IRM.GetResObj<CIFSlotWithHelp>(GDR_SKILL_SLOT_UNIQUE_3, 1)->SlotisLocked = GDR_FELLOW_SKILL_SLOT_ACTIVE;
                            }
                        }

                        if(m_CustomDataManager->m_RefFellowPetSystem[NameStrID].SkillType_3 == 0)
                        {
                            this->m_IRM.GetResObj<CIFSlotWithHelp>(GDR_SKILL_SLOT_UNIQUE_3, 1)->SetClickable(false);
                            this->m_IRM.GetResObj<CIFSlotWithHelp>(GDR_SKILL_SLOT_UNIQUE_3, 1)->SlotisLocked = GDR_FELLOW_SKILL_SLOT_ACTIVE;
                        }
                    }
                    else
                    {
                        this->m_IRM.GetResObj<CIFSlotWithHelp>(GDR_SKILL_SLOT_UNIQUE_3, 1)->SetClickable(true);
                        this->m_IRM.GetResObj<CIFSlotWithHelp>(GDR_SKILL_SLOT_UNIQUE_3, 1)->N595 = 2;

                        this->m_IRM.GetResObj<CIFSlotWithHelp>(GDR_SKILL_SLOT_UNIQUE_3, 1)->SetType(73);
                        this->m_IRM.GetResObj<CIFSlotWithHelp>(GDR_SKILL_SLOT_UNIQUE_3, 1)->GetIconSlot(73, m_CustomDataManager->m_RefFellowPetSystem[NameStrID].SkillID_3);

                        if(m_Player->m_FellowSkillData.size() > 0)
                        {
                            if(m_Player->m_FellowSkillData[0].Enable_Skill_3 == 1)
                            {
                                this->m_IRM.GetResObj<CIFSlotWithHelp>(GDR_SKILL_SLOT_UNIQUE_3, 1)->SlotisLocked = GDR_FELLOW_SKILL_SLOT_DEACTIVE;
                            }
                        }
                        if(m_CustomDataManager->m_RefFellowPetSystem[NameStrID].SkillType_3 == 0)
                        {
                            this->m_IRM.GetResObj<CIFSlotWithHelp>(GDR_SKILL_SLOT_UNIQUE_3, 1)->SetClickable(false);
                        }
                    }

                }
            }
            else
            {
                this->m_IRM.GetResObj<CIFSlotWithHelp>(GDR_SKILL_SLOT_UNIQUE_3, 1)->TB_Func_13("clientlibrary\\fellowpets\\com_close_icon_box.ddj", 1, 1);
            }
            if(m_CustomDataManager->m_RefFellowPetSystem[NameStrID].SkillID_4 > 0)
            {
                const CCharacterData *uqdata = NULL;
                uqdata = g_CGlobalDataManager->GetCharacter(pUser->GetCommonData()->RefObjectId);
                if(uqdata != NULL)
                {
                    if(uqdata->GetData().Level >= m_CustomDataManager->m_RefFellowPetSystem[NameStrID].Active_Level_4)
                    {
                        this->m_IRM.GetResObj<CIFSlotWithHelp>(GDR_SKILL_SLOT_UNIQUE_4, 1)->SetClickable(true);
                        this->m_IRM.GetResObj<CIFSlotWithHelp>(GDR_SKILL_SLOT_UNIQUE_4, 1)->N595 = 0;

                        this->m_IRM.GetResObj<CIFSlotWithHelp>(GDR_SKILL_SLOT_UNIQUE_4, 1)->SetType(73);
                        this->m_IRM.GetResObj<CIFSlotWithHelp>(GDR_SKILL_SLOT_UNIQUE_4, 1)->GetIconSlot(73, m_CustomDataManager->m_RefFellowPetSystem[NameStrID].SkillID_4);

                        if(m_Player->m_FellowSkillData.size() > 0)
                        {
                            if(m_Player->m_FellowSkillData[0].Enable_Skill_4 == 1)
                            {
                                this->m_IRM.GetResObj<CIFSlotWithHelp>(GDR_SKILL_SLOT_UNIQUE_4, 1)->SlotisLocked = GDR_FELLOW_SKILL_SLOT_ACTIVE;
                            }
                        }

                        if(m_CustomDataManager->m_RefFellowPetSystem[NameStrID].SkillType_4 == 0)
                        {
                            this->m_IRM.GetResObj<CIFSlotWithHelp>(GDR_SKILL_SLOT_UNIQUE_4, 1)->SetClickable(false);
                            this->m_IRM.GetResObj<CIFSlotWithHelp>(GDR_SKILL_SLOT_UNIQUE_4, 1)->SlotisLocked = GDR_FELLOW_SKILL_SLOT_ACTIVE;
                        }
                    }
                    else
                    {
                        this->m_IRM.GetResObj<CIFSlotWithHelp>(GDR_SKILL_SLOT_UNIQUE_4, 1)->SetClickable(true);
                        this->m_IRM.GetResObj<CIFSlotWithHelp>(GDR_SKILL_SLOT_UNIQUE_4, 1)->N595 = 2;

                        this->m_IRM.GetResObj<CIFSlotWithHelp>(GDR_SKILL_SLOT_UNIQUE_4, 1)->SetType(73);
                        this->m_IRM.GetResObj<CIFSlotWithHelp>(GDR_SKILL_SLOT_UNIQUE_4, 1)->GetIconSlot(73, m_CustomDataManager->m_RefFellowPetSystem[NameStrID].SkillID_4);

                        if(m_Player->m_FellowSkillData.size() > 0)
                        {
                            if(m_Player->m_FellowSkillData[0].Enable_Skill_4 == 1)
                            {
                                this->m_IRM.GetResObj<CIFSlotWithHelp>(GDR_SKILL_SLOT_UNIQUE_4, 1)->SlotisLocked = GDR_FELLOW_SKILL_SLOT_DEACTIVE;
                            }
                        }
                        if(m_CustomDataManager->m_RefFellowPetSystem[NameStrID].SkillType_4 == 0)
                        {
                            this->m_IRM.GetResObj<CIFSlotWithHelp>(GDR_SKILL_SLOT_UNIQUE_4, 1)->SetClickable(false);
                        }
                    }

                }
            }
            else
            {
                this->m_IRM.GetResObj<CIFSlotWithHelp>(GDR_SKILL_SLOT_UNIQUE_4, 1)->TB_Func_13("clientlibrary\\fellowpets\\com_close_icon_box.ddj", 1, 1);
            }
            if(m_CustomDataManager->m_RefFellowPetSystem[NameStrID].SkillID_5 > 0)
            {
                const CCharacterData *uqdata = NULL;
                uqdata = g_CGlobalDataManager->GetCharacter(pUser->GetCommonData()->RefObjectId);
                if(uqdata != NULL)
                {
                    if(uqdata->GetData().Level >= m_CustomDataManager->m_RefFellowPetSystem[NameStrID].Active_Level_5)
                    {
                        this->m_IRM.GetResObj<CIFSlotWithHelp>(GDR_SKILL_SLOT_UNIQUE_5, 1)->SetClickable(true);
                        this->m_IRM.GetResObj<CIFSlotWithHelp>(GDR_SKILL_SLOT_UNIQUE_5, 1)->N595 = 0;

                        this->m_IRM.GetResObj<CIFSlotWithHelp>(GDR_SKILL_SLOT_UNIQUE_5, 1)->SetType(73);
                        this->m_IRM.GetResObj<CIFSlotWithHelp>(GDR_SKILL_SLOT_UNIQUE_5, 1)->GetIconSlot(73, m_CustomDataManager->m_RefFellowPetSystem[NameStrID].SkillID_5);

                        if(m_Player->m_FellowSkillData.size() > 0)
                        {
                            if(m_Player->m_FellowSkillData[0].Enable_Skill_5 == 1)
                            {
                                this->m_IRM.GetResObj<CIFSlotWithHelp>(GDR_SKILL_SLOT_UNIQUE_5, 1)->SlotisLocked = GDR_FELLOW_SKILL_SLOT_ACTIVE;
                            }
                        }

                        if(m_CustomDataManager->m_RefFellowPetSystem[NameStrID].SkillType_5 == 0)
                        {
                            this->m_IRM.GetResObj<CIFSlotWithHelp>(GDR_SKILL_SLOT_UNIQUE_5, 1)->SetClickable(false);
                            this->m_IRM.GetResObj<CIFSlotWithHelp>(GDR_SKILL_SLOT_UNIQUE_5, 1)->SlotisLocked = GDR_FELLOW_SKILL_SLOT_ACTIVE;
                        }
                    }
                    else
                    {
                        this->m_IRM.GetResObj<CIFSlotWithHelp>(GDR_SKILL_SLOT_UNIQUE_5, 1)->SetClickable(true);
                        this->m_IRM.GetResObj<CIFSlotWithHelp>(GDR_SKILL_SLOT_UNIQUE_5, 1)->N595 = 2;

                        this->m_IRM.GetResObj<CIFSlotWithHelp>(GDR_SKILL_SLOT_UNIQUE_5, 1)->SetType(73);
                        this->m_IRM.GetResObj<CIFSlotWithHelp>(GDR_SKILL_SLOT_UNIQUE_5, 1)->GetIconSlot(73, m_CustomDataManager->m_RefFellowPetSystem[NameStrID].SkillID_5);

                        if(m_Player->m_FellowSkillData.size() > 0)
                        {
                            if(m_Player->m_FellowSkillData[0].Enable_Skill_5 == 1)
                            {
                                this->m_IRM.GetResObj<CIFSlotWithHelp>(GDR_SKILL_SLOT_UNIQUE_5, 1)->SlotisLocked = GDR_FELLOW_SKILL_SLOT_DEACTIVE;
                            }
                        }
                        if(m_CustomDataManager->m_RefFellowPetSystem[NameStrID].SkillType_5 == 0)
                        {
                            this->m_IRM.GetResObj<CIFSlotWithHelp>(GDR_SKILL_SLOT_UNIQUE_5, 1)->SetClickable(false);
                        }
                    }

                }
            }
            else
            {
                this->m_IRM.GetResObj<CIFSlotWithHelp>(GDR_SKILL_SLOT_UNIQUE_5, 1)->TB_Func_13("clientlibrary\\fellowpets\\com_close_icon_box.ddj", 1, 1);
            }

            if(m_CustomDataManager->m_RefFellowPetSystem[NameStrID].SelfSkill_1 > 0)
            {
                const CCharacterData *uqdata = NULL;
                uqdata = g_CGlobalDataManager->GetCharacter(pUser->GetCommonData()->RefObjectId);
                if(uqdata != NULL)
                {
                    if(uqdata->GetData().Level >= m_CustomDataManager->m_RefFellowPetSystem[NameStrID].SelfSkill_Active_Level_1)
                    {
                        this->m_IRM.GetResObj<CIFSlotWithHelp>(GDR_SKILL_SLOT_FELLOW_1, 1)->SetClickable(false);
                        this->m_IRM.GetResObj<CIFSlotWithHelp>(GDR_SKILL_SLOT_FELLOW_1, 1)->N595 = 0;

                        this->m_IRM.GetResObj<CIFSlotWithHelp>(GDR_SKILL_SLOT_FELLOW_1, 1)->SlotisLocked = GDR_FELLOW_SKILL_SLOT_ACTIVE;

                        this->m_IRM.GetResObj<CIFSlotWithHelp>(GDR_SKILL_SLOT_FELLOW_1, 1)->SetType(73);
                        this->m_IRM.GetResObj<CIFSlotWithHelp>(GDR_SKILL_SLOT_FELLOW_1, 1)->GetIconSlot(73, m_CustomDataManager->m_RefFellowPetSystem[NameStrID].SelfSkill_1);

                    }
                    else
                    {
                        this->m_IRM.GetResObj<CIFSlotWithHelp>(GDR_SKILL_SLOT_FELLOW_1, 1)->SetClickable(false);
                        this->m_IRM.GetResObj<CIFSlotWithHelp>(GDR_SKILL_SLOT_FELLOW_1, 1)->N595 = 2;
                        this->m_IRM.GetResObj<CIFSlotWithHelp>(GDR_SKILL_SLOT_FELLOW_1, 1)->SlotisLocked = GDR_FELLOW_SKILL_SLOT_DEACTIVE;
                        this->m_IRM.GetResObj<CIFSlotWithHelp>(GDR_SKILL_SLOT_FELLOW_1, 1)->SetType(73);
                        this->m_IRM.GetResObj<CIFSlotWithHelp>(GDR_SKILL_SLOT_FELLOW_1, 1)->GetIconSlot(73, m_CustomDataManager->m_RefFellowPetSystem[NameStrID].SelfSkill_1);
                    }

                }
            }
        }
    }

}
void CIFCOSInfo::ShowTab1(){
    this->m_IRM.GetResObj<CIFGauge>(35, 1)->ShowGWnd(true);
    this->m_IRM.GetResObj<CIFGauge>(36, 1)->ShowGWnd(true);
    this->m_IRM.GetResObj<CIFGauge>(37, 1)->ShowGWnd(true);


    this->m_IRM.GetResObj<CIFStatic>(25, 1)->ShowGWnd(true);
    this->m_IRM.GetResObj<CIFStatic>(27, 1)->ShowGWnd(true);
    this->m_IRM.GetResObj<CIFButton>(26, 1)->ShowGWnd(true);


    this->m_IRM.GetResObj<CIFStatic>(30, 1)->ShowGWnd(true);
    this->m_IRM.GetResObj<CIFStatic>(31, 1)->ShowGWnd(true);
    this->m_IRM.GetResObj<CIFStatic>(32, 1)->ShowGWnd(true);


    this->m_IRM.GetResObj<CIFStatic>(40, 1)->ShowGWnd(true);
    this->m_IRM.GetResObj<CIFStatic>(41, 1)->ShowGWnd(true);
    this->m_IRM.GetResObj<CIFStatic>(42, 1)->ShowGWnd(true);
    this->m_IRM.GetResObj<CIFStatic>(43, 1)->ShowGWnd(true);
    this->m_IRM.GetResObj<CIFStatic>(44, 1)->ShowGWnd(true);
    this->m_IRM.GetResObj<CIFStatic>(45, 1)->ShowGWnd(true);


    this->m_IRM.GetResObj<CIFRenderStatic>(GDR_RENDER_STATIC , 1)->ShowGWnd(true);
    this->m_IRM.GetResObj<CIFStatic>(1001, 1)->ShowGWnd(true);
    this->m_IRM.GetResObj<CIFStatic>(1002, 1)->ShowGWnd(true);
    this->m_IRM.GetResObj<CIFStatic>(1003, 1)->ShowGWnd(true);


    this->m_IRM.GetResObj<CIFStatic>(60, 1)->ShowGWnd(false);
    this->m_IRM.GetResObj<CIFStatic>(61, 1)->ShowGWnd(false);
    this->m_IRM.GetResObj<CIFStatic>(62, 1)->ShowGWnd(false);
    this->m_IRM.GetResObj<CIFStatic>(63, 1)->ShowGWnd(false);

    this->m_IRM.GetResObj<CIFStatic>(65, 1)->ShowGWnd(false);
    this->m_IRM.GetResObj<CIFStatic>(66, 1)->ShowGWnd(false);
    this->m_IRM.GetResObj<CIFStatic>(67, 1)->ShowGWnd(false);
    this->m_IRM.GetResObj<CIFStatic>(68, 1)->ShowGWnd(false);

    this->m_IRM.GetResObj<CIFStatic>(71, 1)->ShowGWnd(false);
    this->m_IRM.GetResObj<CIFStatic>(72, 1)->ShowGWnd(false);
    this->m_IRM.GetResObj<CIFStatic>(73, 1)->ShowGWnd(false);

    this->m_IRM.GetResObj<CIFStatic>(76, 1)->ShowGWnd(false);
    this->m_IRM.GetResObj<CIFStatic>(77, 1)->ShowGWnd(false);
    this->m_IRM.GetResObj<CIFStatic>(78, 1)->ShowGWnd(false);

    this->m_IRM.GetResObj<CIFStatic>(1004, 1)->ShowGWnd(false);
    this->m_IRM.GetResObj<CIFStatic>(GDR_STATS, 1)->ShowGWnd(false);
    this->m_IRM.GetResObj<CIFStatic>(GDR_STR, 1)->ShowGWnd(false);
    this->m_IRM.GetResObj<CIFStatic>(GDR_STR_VALUE, 1)->ShowGWnd(false);

    this->m_IRM.GetResObj<CIFStatic>(GDR_INT, 1)->ShowGWnd(false);
    this->m_IRM.GetResObj<CIFStatic>(GDR_INT_VALUE, 1)->ShowGWnd(false);
    this->m_IRM.GetResObj<CIFStatic>(GDR_ATTACK, 1)->ShowGWnd(false);
    this->m_IRM.GetResObj<CIFStatic>(GDR_CRT, 1)->ShowGWnd(false);
    this->m_IRM.GetResObj<CIFStatic>(GDR_CRT_VALUE, 1)->ShowGWnd(false);

    this->m_IRM.GetResObj<CIFStatic>(GDR_DEFEND, 1)->ShowGWnd(false);

    this->m_IRM.GetResObj<CIFStatic>(GDR_BR, 1)->ShowGWnd(false);
    this->m_IRM.GetResObj<CIFStatic>(GDR_BR_VALUE, 1)->ShowGWnd(false);

    this->m_IRM.GetResObj<CIFStatic>(GDR_SLOT1, 1)->ShowGWnd(false);
    this->m_IRM.GetResObj<CIFStatic>(GDR_SLOT2, 1)->ShowGWnd(false);
    this->m_IRM.GetResObj<CIFStatic>(GDR_SLOT3, 1)->ShowGWnd(false);
    this->m_IRM.GetResObj<CIFStatic>(GDR_SLOT4, 1)->ShowGWnd(false);
    this->m_IRM.GetResObj<CIFStatic>(GDR_SLOT5, 1)->ShowGWnd(false);
    this->m_IRM.GetResObj<CIFStatic>(GDR_SLOT6, 1)->ShowGWnd(false);
    this->m_IRM.GetResObj<CIFStatic>(GDR_SLOT7, 1)->ShowGWnd(false);
    this->m_IRM.GetResObj<CIFStatic>(GDR_SLOT8, 1)->ShowGWnd(false);

    this->m_IRM.GetResObj<CIFFrame>(GDR_SKILL_FRAME, 1)->ShowGWnd(false);

    this->m_IRM.GetResObj<CIFStatic>(GDR_SKILL_HEADER, 1)->ShowGWnd(false);
    this->m_IRM.GetResObj<CIFFrame>(GDR_SKILL_FRAME2, 1)->ShowGWnd(false);
    this->m_IRM.GetResObj<CIFStatic>(GDR_SKILL_HEADER2, 1)->ShowGWnd(false);

    this->m_IRM.GetResObj<CIFFrame>(GDR_SKILL_FRAME3, 1)->ShowGWnd(false);
    this->m_IRM.GetResObj<CIFStatic>(GDR_SKILL_HEADER3, 1)->ShowGWnd(false);
    this->m_IRM.GetResObj<CIFStatic>(GDR_SKILL_FRAME_UNIQUE_1, 1)->ShowGWnd(false);
    this->m_IRM.GetResObj<CIFSlotWithHelp>(GDR_SKILL_SLOT_UNIQUE_1, 1)->ShowGWnd(false);

    this->m_IRM.GetResObj<CIFStatic>(GDR_SKILL_FRAME_UNIQUE_2, 1)->ShowGWnd(false);
    this->m_IRM.GetResObj<CIFSlotWithHelp>(GDR_SKILL_SLOT_UNIQUE_2, 1)->ShowGWnd(false);



    this->m_IRM.GetResObj<CIFStatic>(GDR_SKILL_FRAME_UNIQUE_3, 1)->ShowGWnd(false);
    this->m_IRM.GetResObj<CIFSlotWithHelp>(GDR_SKILL_SLOT_UNIQUE_3, 1)->ShowGWnd(false);


    this->m_IRM.GetResObj<CIFStatic>(GDR_SKILL_FRAME_UNIQUE_4, 1)->ShowGWnd(false);
    this->m_IRM.GetResObj<CIFSlotWithHelp>(GDR_SKILL_SLOT_UNIQUE_4, 1)->ShowGWnd(false);


    this->m_IRM.GetResObj<CIFStatic>(GDR_SKILL_FRAME_UNIQUE_5, 1)->ShowGWnd(false);
    this->m_IRM.GetResObj<CIFSlotWithHelp>(GDR_SKILL_SLOT_UNIQUE_5, 1)->ShowGWnd(false);

    this->m_IRM.GetResObj<CIFStatic>(GDR_SKILL_FRAME_READIED_1, 1)->ShowGWnd(false);
    this->m_IRM.GetResObj<CIFSlotWithHelp>(GDR_SKILL_SLOT_READIED_1, 1)->ShowGWnd(false);

    this->m_IRM.GetResObj<CIFStatic>(GDR_SKILL_FRAME_READIED_2, 1)->ShowGWnd(false);
    this->m_IRM.GetResObj<CIFSlotWithHelp>(GDR_SKILL_SLOT_READIED_2, 1)->ShowGWnd(false);



    this->m_IRM.GetResObj<CIFStatic>(GDR_SKILL_FRAME_READIED_3, 1)->ShowGWnd(false);
    this->m_IRM.GetResObj<CIFSlotWithHelp>(GDR_SKILL_SLOT_READIED_3, 1)->ShowGWnd(false);


    this->m_IRM.GetResObj<CIFStatic>(GDR_SKILL_FRAME_READIED_4, 1)->ShowGWnd(false);
    this->m_IRM.GetResObj<CIFSlotWithHelp>(GDR_SKILL_SLOT_READIED_4, 1)->ShowGWnd(false);


    this->m_IRM.GetResObj<CIFStatic>(GDR_SKILL_FRAME_READIED_5, 1)->ShowGWnd(false);
    this->m_IRM.GetResObj<CIFSlotWithHelp>(GDR_SKILL_SLOT_READIED_5, 1)->ShowGWnd(false);


    this->m_IRM.GetResObj<CIFStatic>(GDR_SKILL_FRAME_FELLOW_1, 1)->ShowGWnd(false);
    this->m_IRM.GetResObj<CIFSlotWithHelp>(GDR_SKILL_SLOT_FELLOW_1, 1)->ShowGWnd(false);

    this->m_IRM.GetResObj<CIFStatic>(GDR_SKILL_FRAME_FELLOW_2, 1)->ShowGWnd(false);
    this->m_IRM.GetResObj<CIFSlotWithHelp>(GDR_SKILL_SLOT_FELLOW_2, 1)->ShowGWnd(false);



    this->m_IRM.GetResObj<CIFStatic>(GDR_SKILL_FRAME_FELLOW_3, 1)->ShowGWnd(false);
    this->m_IRM.GetResObj<CIFSlotWithHelp>(GDR_SKILL_SLOT_FELLOW_3, 1)->ShowGWnd(false);


    this->m_IRM.GetResObj<CIFStatic>(GDR_SKILL_FRAME_FELLOW_4, 1)->ShowGWnd(false);
    this->m_IRM.GetResObj<CIFSlotWithHelp>(GDR_SKILL_SLOT_FELLOW_4, 1)->ShowGWnd(false);


    this->m_IRM.GetResObj<CIFStatic>(GDR_SKILL_FRAME_FELLOW_5, 1)->ShowGWnd(false);
    this->m_IRM.GetResObj<CIFSlotWithHelp>(GDR_SKILL_SLOT_FELLOW_5, 1)->ShowGWnd(false);



}


void CIFCOSInfo::HideTab1(){

    this->m_IRM.GetResObj<CIFGauge>(35, 1)->ShowGWnd(false);
    this->m_IRM.GetResObj<CIFGauge>(36, 1)->ShowGWnd(false);
    this->m_IRM.GetResObj<CIFGauge>(37, 1)->ShowGWnd(false);


    this->m_IRM.GetResObj<CIFStatic>(25, 1)->ShowGWnd(false);
    this->m_IRM.GetResObj<CIFStatic>(27, 1)->ShowGWnd(false);
    this->m_IRM.GetResObj<CIFButton>(26, 1)->ShowGWnd(false);


    this->m_IRM.GetResObj<CIFStatic>(30, 1)->ShowGWnd(false);
    this->m_IRM.GetResObj<CIFStatic>(31, 1)->ShowGWnd(false);
    this->m_IRM.GetResObj<CIFStatic>(32, 1)->ShowGWnd(false);


    this->m_IRM.GetResObj<CIFStatic>(40, 1)->ShowGWnd(false);
    this->m_IRM.GetResObj<CIFStatic>(41, 1)->ShowGWnd(false);
    this->m_IRM.GetResObj<CIFStatic>(42, 1)->ShowGWnd(false);
    this->m_IRM.GetResObj<CIFStatic>(43, 1)->ShowGWnd(false);
    this->m_IRM.GetResObj<CIFStatic>(44, 1)->ShowGWnd(false);
    this->m_IRM.GetResObj<CIFStatic>(45, 1)->ShowGWnd(false);


    this->m_IRM.GetResObj<CIFRenderStatic>(GDR_RENDER_STATIC , 1)->ShowGWnd(false);
    this->m_IRM.GetResObj<CIFStatic>(1001, 1)->ShowGWnd(false);
    this->m_IRM.GetResObj<CIFStatic>(1002, 1)->ShowGWnd(false);
    this->m_IRM.GetResObj<CIFStatic>(1003, 1)->ShowGWnd(false);



    this->m_IRM.GetResObj<CIFStatic>(60, 1)->ShowGWnd(true);
    this->m_IRM.GetResObj<CIFStatic>(61, 1)->ShowGWnd(true);
    this->m_IRM.GetResObj<CIFStatic>(62, 1)->ShowGWnd(true);
    this->m_IRM.GetResObj<CIFStatic>(63, 1)->ShowGWnd(true);

    this->m_IRM.GetResObj<CIFStatic>(65, 1)->ShowGWnd(true);
    this->m_IRM.GetResObj<CIFStatic>(66, 1)->ShowGWnd(true);
    this->m_IRM.GetResObj<CIFStatic>(67, 1)->ShowGWnd(true);
    this->m_IRM.GetResObj<CIFStatic>(68, 1)->ShowGWnd(true);

    this->m_IRM.GetResObj<CIFStatic>(71, 1)->ShowGWnd(true);
    this->m_IRM.GetResObj<CIFStatic>(72, 1)->ShowGWnd(true);
    this->m_IRM.GetResObj<CIFStatic>(73, 1)->ShowGWnd(true);

    this->m_IRM.GetResObj<CIFStatic>(76, 1)->ShowGWnd(true);
    this->m_IRM.GetResObj<CIFStatic>(77, 1)->ShowGWnd(true);
    this->m_IRM.GetResObj<CIFStatic>(78, 1)->ShowGWnd(true);


    this->m_IRM.GetResObj<CIFStatic>(60, 1)->BringToFront();
    this->m_IRM.GetResObj<CIFStatic>(61, 1)->BringToFront();
    this->m_IRM.GetResObj<CIFStatic>(62, 1)->BringToFront();
    this->m_IRM.GetResObj<CIFStatic>(63, 1)->BringToFront();

    this->m_IRM.GetResObj<CIFStatic>(65, 1)->BringToFront();
    this->m_IRM.GetResObj<CIFStatic>(66, 1)->BringToFront();
    this->m_IRM.GetResObj<CIFStatic>(67, 1)->BringToFront();
    this->m_IRM.GetResObj<CIFStatic>(68, 1)->BringToFront();

    this->m_IRM.GetResObj<CIFStatic>(71, 1)->BringToFront();
    this->m_IRM.GetResObj<CIFStatic>(72, 1)->BringToFront();
    this->m_IRM.GetResObj<CIFStatic>(73, 1)->BringToFront();

    this->m_IRM.GetResObj<CIFStatic>(76, 1)->BringToFront();
    this->m_IRM.GetResObj<CIFStatic>(77, 1)->BringToFront();
    this->m_IRM.GetResObj<CIFStatic>(78, 1)->BringToFront();

    wnd_pos x = this->GetPos();
    this->m_IRM.GetResObj<CIFStatic>(GDR_LEVEL_STATIC, 1)->SetText(KmtGetText(L"UIIT_KMT_LV"));
    this->m_IRM.GetResObj<CIFStatic>(GDR_LEVEL_STATIC, 1)->MoveGWnd(x.x + 148, x.y + 32);/// level text
    this->m_IRM.GetResObj<CIFStatic>(GDR_LEVEL_VALUE, 1)->MoveGWnd(x.x + 168, x.y + 32);/// level
    this->m_IRM.GetResObj<CIFStatic>(GDR_LEVEL_STATIC, 1)->m_FontTexture.SetColor(D3DCOLOR_ARGB(255, 202, 220, 147));
    this->m_IRM.GetResObj<CIFStatic>(GDR_LEVEL_VALUE, 1)->m_FontTexture.SetColor(D3DCOLOR_ARGB(255, 202, 220, 147));


    this->m_IRM.GetResObj<CIFStatic>(1004, 1)->ShowGWnd(true);
    this->m_IRM.GetResObj<CIFStatic>(GDR_STATS, 1)->ShowGWnd(true);
    this->m_IRM.GetResObj<CIFStatic>(GDR_STR, 1)->ShowGWnd(true);
    this->m_IRM.GetResObj<CIFStatic>(GDR_STR_VALUE, 1)->ShowGWnd(true);
    this->m_IRM.GetResObj<CIFStatic>(GDR_INT, 1)->ShowGWnd(true);
    this->m_IRM.GetResObj<CIFStatic>(GDR_INT_VALUE, 1)->ShowGWnd(true);
    this->m_IRM.GetResObj<CIFStatic>(GDR_ATTACK, 1)->ShowGWnd(true);

    this->m_IRM.GetResObj<CIFStatic>(GDR_PHY_ATK_STATIC, 1)->MoveGWnd(x.x + 96, x.y + 127 + 19);


    this->m_IRM.GetResObj<CIFStatic>(GDR_PHY_ATK_VALUE, 1)->MoveGWnd(x.x + 90 + 84, x.y + 127 + 19);

    this->m_IRM.GetResObj<CIFStatic>(GDR_MAG_ATK_STATIC, 1)->MoveGWnd(x.x + 96, x.y + 127 + 38);


    this->m_IRM.GetResObj<CIFStatic>(GDR_MAG_ATK_VALUE, 1)->MoveGWnd(x.x + 90 + 84, x.y + 127 + 38);


    this->m_IRM.GetResObj<CIFStatic>(GDR_HIT_RATE_STATIC, 1)->MoveGWnd(x.x + 96, x.y + 127 + 38 + 19);


    this->m_IRM.GetResObj<CIFStatic>(GDR_HIT_RATE_VALUE, 1)->MoveGWnd(x.x + 90 + 84, x.y + 127 + 38 + 19);
    this->m_IRM.GetResObj<CIFStatic>(GDR_HIT_RATE_STATIC, 1)->SetText(TSM_GETTEXTPTR(L"PARAM_HR"));

    this->m_IRM.GetResObj<CIFStatic>(GDR_CRT, 1)->ShowGWnd(true);
    this->m_IRM.GetResObj<CIFStatic>(GDR_CRT_VALUE, 1)->ShowGWnd(true);
    this->m_IRM.GetResObj<CIFStatic>(GDR_DEFEND, 1)->ShowGWnd(true);


    this->m_IRM.GetResObj<CIFStatic>(GDR_PHY_DEF_STATIC, 1)->MoveGWnd(x.x + 96, x.y + 232 + 19);
    this->m_IRM.GetResObj<CIFStatic>(GDR_PHY_DEF_VALUE, 1)->MoveGWnd(x.x + 90 + 84, x.y + 232 + 19);

    this->m_IRM.GetResObj<CIFStatic>(GDR_MAG_DEF_STATIC, 1)->MoveGWnd(x.x + 96, x.y + 232 + 38);
    this->m_IRM.GetResObj<CIFStatic>(GDR_MAG_DEF_VALUE, 1)->MoveGWnd(x.x + 90 + 84, x.y + 232 + 38);

    this->m_IRM.GetResObj<CIFStatic>(GDR_PARRY_RATE_STATIC, 1)->MoveGWnd(x.x + 96, x.y + 232 + 57);
    this->m_IRM.GetResObj<CIFStatic>(GDR_PARRY_RATE_VALUE, 1)->MoveGWnd(x.x + 90 + 84, x.y + 232 + 57);

    this->m_IRM.GetResObj<CIFStatic>(GDR_BR, 1)->ShowGWnd(true);
    this->m_IRM.GetResObj<CIFStatic>(GDR_BR_VALUE, 1)->ShowGWnd(true);

    this->m_IRM.GetResObj<CIFStatic>(GDR_BR, 1)->MoveGWnd(x.x + 96, x.y + 232 + 76);
    this->m_IRM.GetResObj<CIFStatic>(GDR_BR_VALUE, 1)->MoveGWnd(x.x + 90 + 84, x.y + 232 + 76);
    this->m_IRM.GetResObj<CIFStatic>(GDR_CRT_VALUE, 1)->SetText(L"-");


    this->m_IRM.GetResObj<CIFStatic>(GDR_PHY_ATK_VALUE, 1)->m_FontTexture.SetColor(D3DCOLOR_ARGB(255, 255, 255, 255));
    this->m_IRM.GetResObj<CIFStatic>(GDR_MAG_ATK_VALUE, 1)->m_FontTexture.SetColor(D3DCOLOR_ARGB(255, 255, 255, 255));
    this->m_IRM.GetResObj<CIFStatic>(GDR_HIT_RATE_VALUE, 1)->m_FontTexture.SetColor(D3DCOLOR_ARGB(255, 255, 255, 255));
    this->m_IRM.GetResObj<CIFStatic>(GDR_PHY_DEF_VALUE, 1)->m_FontTexture.SetColor(D3DCOLOR_ARGB(255, 255, 255, 255));
    this->m_IRM.GetResObj<CIFStatic>(GDR_MAG_DEF_VALUE, 1)->m_FontTexture.SetColor(D3DCOLOR_ARGB(255, 255, 255, 255));
    this->m_IRM.GetResObj<CIFStatic>(GDR_PARRY_RATE_VALUE, 1)->m_FontTexture.SetColor(D3DCOLOR_ARGB(255, 255, 255, 255));



    CICharactor* pUser = GetCharacterObjectByID_MAYBE(this->m_petUniqueID);
    if (pUser != NULL) {
        const CCharacterData *uqdata = NULL;
        uqdata = g_CGlobalDataManager->GetCharacter(pUser->GetCommonData()->RefObjectId);
        if(uqdata != NULL)
        {
            wchar_t buffer12[255];
            swprintf_s(buffer12, L"%d", uqdata->GetData().BR);

            this->m_IRM.GetResObj<CIFStatic>(GDR_BR_VALUE, 1)->SetText(buffer12);
        }
    }
    this->m_IRM.GetResObj<CIFStatic>(GDR_SLOT1, 1)->ShowGWnd(true);
    this->m_IRM.GetResObj<CIFStatic>(GDR_SLOT2, 1)->ShowGWnd(true);
    this->m_IRM.GetResObj<CIFStatic>(GDR_SLOT3, 1)->ShowGWnd(true);
    this->m_IRM.GetResObj<CIFStatic>(GDR_SLOT4, 1)->ShowGWnd(true);
    this->m_IRM.GetResObj<CIFStatic>(GDR_SLOT5, 1)->ShowGWnd(true);
    this->m_IRM.GetResObj<CIFStatic>(GDR_SLOT6, 1)->ShowGWnd(true);
    this->m_IRM.GetResObj<CIFStatic>(GDR_SLOT7, 1)->ShowGWnd(true);
    this->m_IRM.GetResObj<CIFStatic>(GDR_SLOT8, 1)->ShowGWnd(true);

    this->m_IRM.GetResObj<CIFFrame>(GDR_SKILL_FRAME, 1)->ShowGWnd(false);

    this->m_IRM.GetResObj<CIFStatic>(GDR_SKILL_HEADER, 1)->ShowGWnd(false);

    this->m_IRM.GetResObj<CIFFrame>(GDR_SKILL_FRAME2, 1)->ShowGWnd(false);
    this->m_IRM.GetResObj<CIFStatic>(GDR_SKILL_HEADER2, 1)->ShowGWnd(false);

    this->m_IRM.GetResObj<CIFFrame>(GDR_SKILL_FRAME3, 1)->ShowGWnd(false);
    this->m_IRM.GetResObj<CIFStatic>(GDR_SKILL_HEADER3, 1)->ShowGWnd(false);
    this->m_IRM.GetResObj<CIFStatic>(GDR_SKILL_FRAME_UNIQUE_1, 1)->ShowGWnd(false);
    this->m_IRM.GetResObj<CIFSlotWithHelp>(GDR_SKILL_SLOT_UNIQUE_1, 1)->ShowGWnd(false);

    this->m_IRM.GetResObj<CIFStatic>(GDR_SKILL_FRAME_UNIQUE_2, 1)->ShowGWnd(false);
    this->m_IRM.GetResObj<CIFSlotWithHelp>(GDR_SKILL_SLOT_UNIQUE_2, 1)->ShowGWnd(false);


    this->m_IRM.GetResObj<CIFStatic>(GDR_SKILL_FRAME_UNIQUE_3, 1)->ShowGWnd(false);
    this->m_IRM.GetResObj<CIFSlotWithHelp>(GDR_SKILL_SLOT_UNIQUE_3, 1)->ShowGWnd(false);


    this->m_IRM.GetResObj<CIFStatic>(GDR_SKILL_FRAME_UNIQUE_4, 1)->ShowGWnd(false);
    this->m_IRM.GetResObj<CIFSlotWithHelp>(GDR_SKILL_SLOT_UNIQUE_4, 1)->ShowGWnd(false);


    this->m_IRM.GetResObj<CIFStatic>(GDR_SKILL_FRAME_UNIQUE_5, 1)->ShowGWnd(false);
    this->m_IRM.GetResObj<CIFSlotWithHelp>(GDR_SKILL_SLOT_UNIQUE_5, 1)->ShowGWnd(false);

    this->m_IRM.GetResObj<CIFStatic>(GDR_SKILL_FRAME_READIED_1, 1)->ShowGWnd(false);
    this->m_IRM.GetResObj<CIFSlotWithHelp>(GDR_SKILL_SLOT_READIED_1, 1)->ShowGWnd(false);

    this->m_IRM.GetResObj<CIFStatic>(GDR_SKILL_FRAME_READIED_2, 1)->ShowGWnd(false);
    this->m_IRM.GetResObj<CIFSlotWithHelp>(GDR_SKILL_SLOT_READIED_2, 1)->ShowGWnd(false);



    this->m_IRM.GetResObj<CIFStatic>(GDR_SKILL_FRAME_READIED_3, 1)->ShowGWnd(false);
    this->m_IRM.GetResObj<CIFSlotWithHelp>(GDR_SKILL_SLOT_READIED_3, 1)->ShowGWnd(false);


    this->m_IRM.GetResObj<CIFStatic>(GDR_SKILL_FRAME_READIED_4, 1)->ShowGWnd(false);
    this->m_IRM.GetResObj<CIFSlotWithHelp>(GDR_SKILL_SLOT_READIED_4, 1)->ShowGWnd(false);


    this->m_IRM.GetResObj<CIFStatic>(GDR_SKILL_FRAME_READIED_5, 1)->ShowGWnd(false);
    this->m_IRM.GetResObj<CIFSlotWithHelp>(GDR_SKILL_SLOT_READIED_5, 1)->ShowGWnd(false);

    this->m_IRM.GetResObj<CIFStatic>(GDR_SKILL_FRAME_FELLOW_1, 1)->ShowGWnd(false);
    this->m_IRM.GetResObj<CIFSlotWithHelp>(GDR_SKILL_SLOT_FELLOW_1, 1)->ShowGWnd(false);

    this->m_IRM.GetResObj<CIFStatic>(GDR_SKILL_FRAME_FELLOW_2, 1)->ShowGWnd(false);
    this->m_IRM.GetResObj<CIFSlotWithHelp>(GDR_SKILL_SLOT_FELLOW_2, 1)->ShowGWnd(false);



    this->m_IRM.GetResObj<CIFStatic>(GDR_SKILL_FRAME_FELLOW_3, 1)->ShowGWnd(false);
    this->m_IRM.GetResObj<CIFSlotWithHelp>(GDR_SKILL_SLOT_FELLOW_3, 1)->ShowGWnd(false);


    this->m_IRM.GetResObj<CIFStatic>(GDR_SKILL_FRAME_FELLOW_4, 1)->ShowGWnd(false);
    this->m_IRM.GetResObj<CIFSlotWithHelp>(GDR_SKILL_SLOT_FELLOW_4, 1)->ShowGWnd(false);


    this->m_IRM.GetResObj<CIFStatic>(GDR_SKILL_FRAME_FELLOW_5, 1)->ShowGWnd(false);
    this->m_IRM.GetResObj<CIFSlotWithHelp>(GDR_SKILL_SLOT_FELLOW_5, 1)->ShowGWnd(false);

}