//
// Created by YUMBUL on 17.02.2024.
//

#include <CustomData/CustomSettingManager.h>
#include <MacroAlchemy/IFAlchemyMacro.h>
#include <NIFAlchemySubWndType1.h>
#include <IFNormalTile.h>
#include <GInterface.h>
#include <LockItems/IFNewMsgBox.h>
#include <GEffSoundBody.h>
#include <CustomData/CustomDataManager.h>
#include <TextStringManager.h>
#include <IFPML.h>
#include "NIFEnchantWnd.h"
GFX_MSGMAP* CNIFEnchantWnd::MessageMap(){
    static const GFX_MSGMAP_ENTRY skillBoardMessageEntries[] =
            {
                    /* {GFX_WM_COMMAND, 0, 14, 14, BSSig_u12, 0,
                             (GFX_PMSG)(static_cast< void (GFX_MSG_CALL CGWndBase::*)() >(&CIFSkillBoard::OnBtnClick))},
 */
                    {GFX_WM_COMMAND, 0, 13402, 13402, BSSig_u12, 0,
                            (GFX_PMSG)(static_cast< void (GFX_MSG_CALL CGWndBase::*)() >(&CNIFEnchantWnd::PressNewAlchemyTab))},
                    {GFX_WM_COMMAND, 0, GDR_START_BUTTON, GDR_START_BUTTON, BSSig_u12, 0,
                            (GFX_PMSG)(static_cast< void (GFX_MSG_CALL CGWndBase::*)() >(&CNIFEnchantWnd::PressNewAlchemyStart))},
                    // Diğer özel mesaj girişleri buraya eklenebilir
            };

    static GFX_MSGMAP newmap =
            {
                    reinterpret_cast<const GFX_MSGMAP *>(0x00d8ecdc), skillBoardMessageEntries,
            };
    return &newmap;
}

bool CNIFEnchantWnd::OnCreateIMPL(long ln) {
    bool b = reinterpret_cast<bool(__thiscall *)(CNIFEnchantWnd *, long)>(0x00479a50)(this, ln);
    if(m_Settings->OldAlchemy)
    {
        wnd_pos x = this->GetResObj<CNIFButton>(65533)->GetPos();

        this->GetResObj<CNIFButton>(65533)->NTB_Func_13("interface\\alchemy\\alcm_windowclose.ddj", 0, 0);
        this->GetResObj<CNIFButton>(65533)->MoveGWnd(x.x, x.y+ 5);


        wnd_pos y = this->GetResObj<CNIFStatic>(65535)->GetPos();
        this->GetResObj<CNIFStatic>(65535)->MoveGWnd(y.x, y.y + 6);
    }
    if(m_Settings->ENABLE_NEW_ALCHEMY)
    {
        m_CustomDataManager->m_NewAlchemyProgress = 0;
        wnd_rect sz;
        sz.pos.x = -36;
        sz.pos.y = 163;
        sz.size.width = 40;
        sz.size.height = 40;
        CIFButton* button = (CIFButton *) CIFWnd::CreateInstance(this, GFX_RUNTIME_CLASS(CIFButton), sz, 13402, 0);
        button->TB_Func_13("interface\\alchemy\\alcm_menu_button1.ddj", 1, 1);
        button->ShowGWnd(true);
        button->BringToFront();
        button->SetStyleThingy(TOOLTIP);
        button->SetTooltip(L"New Alchemy");

        sz.pos.x = 12;
        sz.pos.y = 41;
        sz.size.width = 366;
        sz.size.height = 365;
        CIFFrame* frame = (CIFFrame *) CIFWnd::CreateInstance(this, GFX_RUNTIME_CLASS(CIFFrame), sz, 5010, 0);
        frame->TB_Func_13("interface\\frame\\frameg01_wnd_", 1, 1);
        frame->ShowGWnd(false);
        frame->BringToFront();

        sz.pos.x = 24;
        sz.pos.y = 55;
        sz.size.width = 348;
        sz.size.height = 339;
        CIFNormalTile* tile = (CIFNormalTile *) CIFWnd::CreateInstance(this, GFX_RUNTIME_CLASS(CIFNormalTile), sz, 5011, 0);
        tile->TB_Func_13("interface\\ifcommon\\bg_tile\\com_bg_tile_b.ddj", 1, 1);
        tile->ShowGWnd(false);
        tile->BringToFront();


        sz.pos.x = 21;
        sz.pos.y = 78;
        sz.size.width = 349;
        sz.size.height = 162;
        CIFFrame* frame2 = (CIFFrame *) CIFWnd::CreateInstance(this, GFX_RUNTIME_CLASS(CIFFrame), sz, 5012, 0);
        frame2->TB_Func_13("interface\\frame\\frameg_wnd_", 1, 1);
        frame2->ShowGWnd(false);
        frame2->BringToFront();

        sz.pos.x = 32;
        sz.pos.y = 90;
        sz.size.width = 48;
        sz.size.height = 48;
        CIFStatic* box = (CIFStatic *) CIFWnd::CreateInstance(this, GFX_RUNTIME_CLASS(CIFStatic), sz, 5013, 0);
        box->TB_Func_13("interface\\messagebox\\msgbox_itemwindow.ddj", 1, 1);
        box->ShowGWnd(false);
        box->BringToFront();


        sz.pos.x = 54;
        sz.pos.y = 254;
        sz.size.width = 48;
        sz.size.height = 48;
        CIFStatic* boxforenhancer = (CIFStatic *) CIFWnd::CreateInstance(this, GFX_RUNTIME_CLASS(CIFStatic), sz, 5014, 0);
        boxforenhancer->TB_Func_13("interface\\alchemy\\alcm_slot_open.ddj", 1, 1);
        boxforenhancer->ShowGWnd(false);
        boxforenhancer->BringToFront();

        sz.pos.x = 60;
        sz.pos.y = 261;
        sz.size.width = 32;
        sz.size.height = 32;
        CIFStatic* boxforenhancernothing = (CIFStatic *) CIFWnd::CreateInstance(this, GFX_RUNTIME_CLASS(CIFStatic), sz, GDR_boxforenhancernothing, 0);
        boxforenhancernothing->TB_Func_13("interface\\skill\\skl_mastery_nothing.ddj", 1, 1);
        boxforenhancernothing->ShowGWnd(false);
        boxforenhancernothing->BringToFront();




        sz.pos.x = 133;
        sz.pos.y = 254;
        sz.size.width = 48;
        sz.size.height = 48;
        CIFStatic* boxforspoof = (CIFStatic *) CIFWnd::CreateInstance(this, GFX_RUNTIME_CLASS(CIFStatic), sz, 5016, 0);
        boxforspoof->TB_Func_13("interface\\alchemy\\alcm_slot_open.ddj", 1, 1);
        boxforspoof->ShowGWnd(false);
        boxforspoof->BringToFront();

        sz.pos.x = 139;
        sz.pos.y = 261;
        sz.size.width = 32;
        sz.size.height = 32;
        CIFStatic* boxforspoofnothing = (CIFStatic *) CIFWnd::CreateInstance(this, GFX_RUNTIME_CLASS(CIFStatic), sz, GDR_boxforspoofnothing, 0);
        boxforspoofnothing->TB_Func_13("interface\\skill\\skl_mastery_nothing.ddj", 1, 1);
        boxforspoofnothing->ShowGWnd(false);
        boxforspoofnothing->BringToFront();



        sz.pos.x = 212;
        sz.pos.y = 254;
        sz.size.width = 48;
        sz.size.height = 48;
        CIFStatic* boxempty3 = (CIFStatic *) CIFWnd::CreateInstance(this, GFX_RUNTIME_CLASS(CIFStatic), sz, 5018, 0);
        boxempty3->TB_Func_13("interface\\alchemy\\alcm_slot_open.ddj", 1, 1);
        boxempty3->ShowGWnd(false);
        boxempty3->BringToFront();

        sz.pos.x = 218;
        sz.pos.y = 260;
        sz.size.width = 32;
        sz.size.height = 32;
        CIFStatic* boxempty3nothig = (CIFStatic *) CIFWnd::CreateInstance(this, GFX_RUNTIME_CLASS(CIFStatic), sz, 5019, 0);
        boxempty3nothig->TB_Func_13("interface\\skill\\skl_mastery_nothing.ddj", 1, 1);
        boxempty3nothig->ShowGWnd(false);
        boxempty3nothig->BringToFront();

        sz.pos.x = 292;
        sz.pos.y = 254;
        sz.size.width = 48;
        sz.size.height = 48;
        CIFStatic* boxempty4 = (CIFStatic *) CIFWnd::CreateInstance(this, GFX_RUNTIME_CLASS(CIFStatic), sz, 5020, 0);
        boxempty4->TB_Func_13("interface\\alchemy\\alcm_slot_open.ddj", 1, 1);
        boxempty4->ShowGWnd(false);
        boxempty4->BringToFront();

        sz.pos.x = 298;
        sz.pos.y = 260;
        sz.size.width = 32;
        sz.size.height = 32;
        CIFStatic* boxempty4nothig = (CIFStatic *) CIFWnd::CreateInstance(this, GFX_RUNTIME_CLASS(CIFStatic), sz, 5021, 0);
        boxempty4nothig->TB_Func_13("interface\\skill\\skl_mastery_nothing.ddj", 1, 1);
        boxempty4nothig->ShowGWnd(false);
        boxempty4nothig->BringToFront();

        sz.pos.x = 38;
        sz.pos.y = 97;
        sz.size.width = 32;
        sz.size.height = 32;
        CIFMacroAlchemySlot * m_ItemSlot = (CIFMacroAlchemySlot *) CIFWnd::CreateInstance(this, GFX_RUNTIME_CLASS(CIFMacroAlchemySlot), sz, GDR_ITEM_SLOT, 0);
        m_ItemSlot->m_pMySlot->m_pSlot->SetSlot(501);
        m_ItemSlot->m_pMySlot->m_pSlot->SetType(0xC);
        m_ItemSlot->ShowGWnd(false);
        m_ItemSlot->BringToFront();


        sz.pos.x = 60;
        sz.pos.y = 260;
        sz.size.width = 32;
        sz.size.height = 32;
        CIFMacroAlchemySlot * m_ItemSlotenc = (CIFMacroAlchemySlot *) CIFWnd::CreateInstance(this, GFX_RUNTIME_CLASS(CIFMacroAlchemySlot), sz, GDR_ITEM_SLOT_ENC, 0);
        m_ItemSlotenc->m_pMySlot->m_pSlot->SetSlot(502);
        m_ItemSlotenc->m_pMySlot->m_pSlot->SetType(0xC);
        m_ItemSlotenc->ShowGWnd(false);
        m_ItemSlotenc->BringToFront();
        m_ItemSlotenc->m_pMySlot->m_pSlot->SetClickable(false);

        sz.pos.x = 139;
        sz.pos.y = 260;
        sz.size.width = 32;
        sz.size.height = 32;
        CIFMacroAlchemySlot * m_ItemSlotProof = (CIFMacroAlchemySlot *) CIFWnd::CreateInstance(this, GFX_RUNTIME_CLASS(CIFMacroAlchemySlot), sz, GDR_ITEM_SLOT_PROOF, 0);
        m_ItemSlotProof->m_pMySlot->m_pSlot->SetSlot(503);
        m_ItemSlotProof->m_pMySlot->m_pSlot->SetType(0xC);
        m_ItemSlotProof->ShowGWnd(false);
        m_ItemSlotProof->BringToFront();
        m_ItemSlotProof->m_pMySlot->m_pSlot->SetClickable(false);


        sz.pos.x = 24;
        sz.pos.y = 325;
        sz.size.width = 345;
        sz.size.height = 24;
        CIFStatic* progressbarbackend = (CIFStatic *) CIFWnd::CreateInstance(this, GFX_RUNTIME_CLASS(CIFStatic), sz, 5025, 0);
        progressbarbackend->TB_Func_13("interface\\party\\pt_progress_window.ddj", 1, 1);
        progressbarbackend->ShowGWnd(false);
        progressbarbackend->BringToFront();

        sz.pos.x = 136;
        sz.pos.y = 366;
        sz.size.width = 112;
        sz.size.height = 24;
        CIFButton* myStartButton = (CIFButton *) CIFWnd::CreateInstance(this, GFX_RUNTIME_CLASS(CIFButton), sz, GDR_START_BUTTON, 0);
        myStartButton->TB_Func_13("interface\\ifcommon\\com_mid_button02.ddj", 1, 1);
        myStartButton->SetText(L"Strengthen");
        myStartButton->FUN_00656640("interface\\ifcommon\\com_mid_button02_disable.ddj");
        myStartButton->ShowGWnd(false);
        myStartButton->BringToFront();
        SetStartButtonState(false);




        sz.pos.x = -10;
        sz.pos.y = 49;
        sz.size.width = 128;
        sz.size.height = 128;

        CIFDecoratedStatic * deco = (CIFDecoratedStatic *) CIFWnd::CreateInstance(this, GFX_RUNTIME_CLASS(CIFDecoratedStatic), sz, GDR_DECORATED_STATIC, 0);
        deco->ShowGWnd(false);


        sz.pos.x = 29;
        sz.pos.y = 331;
        sz.size.width = 333;
        sz.size.height = 12;
        CIFStatic* progressbar = (CIFStatic *) CIFWnd::CreateInstance(this, GFX_RUNTIME_CLASS(CIFStatic), sz, GDR_PROGRESS_BAR, 0);
        progressbar->TB_Func_13("interface\\party\\pt_progress.ddj", 1, 1);
        progressbar->ShowGWnd(false);
        progressbar->BringToFront();
        progressbar->SetGWndSize(0, 12);

        sz.pos.x = 32;
        sz.pos.y = 90;
        sz.size.width = 48;
        sz.size.height = 48;


        sz.pos.x = 90;
        sz.pos.y = 95;
        sz.size.width = 264;
        sz.size.height = 112;

        std::wstring Msg = L"<sml2><left><font color=\"255,250,250,180\">Able to reinforce with Enhancer and Proof Stones. Place Equipment and other alchemy requirement.<br>(This window created by VFilter.)</font></left></sml2>";
        CIFPML* pml = (CIFPML *) CIFWnd::CreateInstance(this, GFX_RUNTIME_CLASS(CIFPML), sz, 5029, 0);
        pml->ShowGWnd(false);
        pml->BringToFront();
        pml->SetText(Msg.c_str());
        pml->JustifyHorizontal(CTextBoard::JUSTIFY_LEFT);
        pml->JustifyVertical(CTextBoard::JUSTIFY_TOP);
    }

    return b;
}
#define EffectTimer 22
#define FuseTimer 33
#define SuccessTimer 44
#define KillSuccessTimer 55

#define FailTimer 66
#define KillFailTimer 777
void CNIFEnchantWnd::ShowEnchancerAndSpoofSlot()
{
    if(this->GetResObj(GDR_boxforspoofnothing) != NULL)
    {
        this->GetResObj(GDR_boxforspoofnothing)->ShowGWnd(false);
    }
    if(this->GetResObj(GDR_boxforenhancernothing) != NULL)
    {
        this->GetResObj(GDR_boxforenhancernothing)->ShowGWnd(false);
    }
    this->GetResObj<CIFMacroAlchemySlot>(GDR_ITEM_SLOT_ENC)->m_pMySlot->m_pSlot->ClearSlot();
    this->GetResObj<CIFMacroAlchemySlot>(GDR_ITEM_SLOT_ENC)->ShowGWnd(true);
    this->GetResObj<CIFMacroAlchemySlot>(GDR_ITEM_SLOT_ENC)->m_pMySlot->m_pSlot->SetClickable(true);
    this->GetResObj<CIFMacroAlchemySlot>(GDR_ITEM_SLOT_ENC)->BringToFront();


    this->GetResObj<CIFMacroAlchemySlot>(GDR_ITEM_SLOT_PROOF)->m_pMySlot->m_pSlot->ClearSlot();
    this->GetResObj<CIFMacroAlchemySlot>(GDR_ITEM_SLOT_PROOF)->ShowGWnd(true);
    this->GetResObj<CIFMacroAlchemySlot>(GDR_ITEM_SLOT_PROOF)->m_pMySlot->m_pSlot->SetClickable(true);
    this->GetResObj<CIFMacroAlchemySlot>(GDR_ITEM_SLOT_PROOF)->BringToFront();

    SetStartButtonState(false);
    this->KillTimer(EffectTimer);
    this->KillTimer(FuseTimer);
    m_CustomDataManager->m_NewAlchemyWorking = false;
    this->GetResObj(GDR_PROGRESS_BAR)->SetGWndSize(0, 12);

    this->GetResObj<CIFDecoratedStatic>(GDR_DECORATED_STATIC)->ShowGWnd(false);
}
void CNIFEnchantWnd::HideEnchancerAndSpoofSlot()
{
    if(this->GetResObj(GDR_boxforspoofnothing) != NULL)
    {
        this->GetResObj(GDR_boxforspoofnothing)->ShowGWnd(true);
    }
    if(this->GetResObj(GDR_boxforenhancernothing) != NULL)
    {
        this->GetResObj(GDR_boxforenhancernothing)->ShowGWnd(true);
    }


    this->GetResObj<CIFMacroAlchemySlot>(GDR_ITEM_SLOT_ENC)->m_pMySlot->m_pSlot->ClearSlot();
    this->GetResObj<CIFMacroAlchemySlot>(GDR_ITEM_SLOT_ENC)->ShowGWnd(false);
    this->GetResObj<CIFMacroAlchemySlot>(GDR_ITEM_SLOT_ENC)->m_pMySlot->m_pSlot->SetClickable(false);

    this->GetResObj<CIFMacroAlchemySlot>(GDR_ITEM_SLOT_PROOF)->m_pMySlot->m_pSlot->ClearSlot();
    this->GetResObj<CIFMacroAlchemySlot>(GDR_ITEM_SLOT_PROOF)->ShowGWnd(false);
    this->GetResObj<CIFMacroAlchemySlot>(GDR_ITEM_SLOT_PROOF)->m_pMySlot->m_pSlot->SetClickable(false);

    SetStartButtonState(false);

    this->KillTimer(EffectTimer);
    this->KillTimer(FuseTimer);
    m_CustomDataManager->m_NewAlchemyWorking = false;
    this->GetResObj(GDR_PROGRESS_BAR)->SetGWndSize(0, 12);
    this->GetResObj<CIFDecoratedStatic>(GDR_DECORATED_STATIC)->ShowGWnd(false);
}
void CNIFEnchantWnd::SetStartButtonState(bool T)
{
    this->GetResObj<CIFButton>(GDR_START_BUTTON)->SetEnabledState(T);
    this->GetResObj<CIFButton>(GDR_START_BUTTON)->SetText(L"Strengthen");
}
int CNIFEnchantWnd::GetItemInfoFromSlot()
{
    if(this->GetResObj<CIFMacroAlchemySlot>(GDR_ITEM_SLOT) != NULL)
    {
        if(this->GetResObj<CIFMacroAlchemySlot>(GDR_ITEM_SLOT)->m_pMySlot->m_pSlot->ItemInfo != NULL)
        {
            if(this->GetResObj<CIFMacroAlchemySlot>(GDR_ITEM_SLOT)->m_pMySlot->m_pSlot->ItemInfo->GetItemData() != NULL)
            {
                const SItemData * data = this->GetResObj<CIFMacroAlchemySlot>(GDR_ITEM_SLOT)->m_pMySlot->m_pSlot->ItemInfo->GetItemData();
                if(data->m_typeId.getTypeID1() == 3 && data->m_typeId.getTypeID2() == 1 && data->m_typeId.getTypeID3() == 6)
                {
                    return WEAPON;
                }
                else if(data->m_typeId.getTypeID1() == 3 && data->m_typeId.getTypeID2() == 1 && (data->m_typeId.getTypeID3() == 1 ||
                        data->m_typeId.getTypeID3() == 2 || data->m_typeId.getTypeID3() == 3 || data->m_typeId.getTypeID3() == 9 || data->m_typeId.getTypeID3() == 10
                        || data->m_typeId.getTypeID3() == 11))
                {
                    return ARMOR;
                }
                else if(data->m_typeId.getTypeID1() == 3 && data->m_typeId.getTypeID2() == 1 && data->m_typeId.getTypeID3() == 4)
                {
                    return SHIELD;
                }
                else if(data->m_typeId.getTypeID1() == 3 && data->m_typeId.getTypeID2() == 1 && (data->m_typeId.getTypeID3() == 5
                || data->m_typeId.getTypeID3() == 12))
                {
                    return ACC;
                }
            }
        }
    }
    return 0;
}
bool CNIFEnchantWnd::GetEnhancerInfoFromSlot()
{
    if(this->GetResObj<CIFMacroAlchemySlot>(GDR_ITEM_SLOT_ENC) != NULL)
    {
        if(this->GetResObj<CIFMacroAlchemySlot>(GDR_ITEM_SLOT_ENC)->m_pMySlot->m_pSlot->ItemInfo != NULL)
        {
            if(this->GetResObj<CIFMacroAlchemySlot>(GDR_ITEM_SLOT_ENC)->m_pMySlot->m_pSlot->ItemInfo->GetItemData() != NULL)
            {
                if(this->GetResObj<CIFMacroAlchemySlot>(GDR_ITEM_SLOT_ENC)->m_pMySlot->m_pSlot->ItemInfo->GetItemData()->IsEnhancer())
                {
                    return true;
                }
            }
        }
    }
 return false;
}
int CNIFEnchantWnd::GetDegreeLevel(int itemClass) {
    return ((itemClass - 1) / 3 + 1);
}
bool CNIFEnchantWnd::GetDegreeCondition(int DegreeValue)
{
    if(this->GetResObj<CIFMacroAlchemySlot>(GDR_ITEM_SLOT) != NULL)
    {
        if(this->GetResObj<CIFMacroAlchemySlot>(GDR_ITEM_SLOT)->m_pMySlot->m_pSlot->ItemInfo != NULL)
        {
            if(this->GetResObj<CIFMacroAlchemySlot>(GDR_ITEM_SLOT)->m_pMySlot->m_pSlot->ItemInfo->GetItemData() != NULL)
            {
                const SItemData * data = this->GetResObj<CIFMacroAlchemySlot>(GDR_ITEM_SLOT)->m_pMySlot->m_pSlot->ItemInfo->GetItemData();
                int Degree = GetDegreeLevel(data->m_itemClass);
                if(Degree == DegreeValue)
                {
                    return true;
                }
            }
        }
    }
    return false;
}
bool CNIFEnchantWnd::GetDegreeConditionForProofStone(int DegreeValue)
{
    if(this->GetResObj<CIFMacroAlchemySlot>(GDR_ITEM_SLOT) != NULL)
    {
        if(this->GetResObj<CIFMacroAlchemySlot>(GDR_ITEM_SLOT)->m_pMySlot->m_pSlot->ItemInfo != NULL)
        {
            if(this->GetResObj<CIFMacroAlchemySlot>(GDR_ITEM_SLOT)->m_pMySlot->m_pSlot->ItemInfo->GetItemData() != NULL)
            {
                const SItemData * data = this->GetResObj<CIFMacroAlchemySlot>(GDR_ITEM_SLOT)->m_pMySlot->m_pSlot->ItemInfo->GetItemData();
                if(data->m_itemClass == DegreeValue)
                {
                    return true;
                }
            }
        }
    }
    return false;
}


void CNIFEnchantWnd::PressAlchemyTabButton()
{

    reinterpret_cast<void(__thiscall *)(CNIFEnchantWnd *)>(0x005942f0)(this);
    if(m_Settings->ENABLE_NEW_ALCHEMY)
    {
        m_CustomDataManager->m_NewAlchemyProgress = 0;
        this->KillTimer(EffectTimer);
        this->KillTimer(FuseTimer);
        m_CustomDataManager->m_NewAlchemyWorking = false;
        this->GetResObj(GDR_PROGRESS_BAR)->SetGWndSize(0, 12);

        this->GetResObj<CIFDecoratedStatic>(GDR_DECORATED_STATIC)->ShowGWnd(false);
        for(int i = 5010; i< 5030;i++)
        {
            if(this->GetResObj(i) != NULL)
            {
                this->GetResObj(i)->ShowGWnd(false);
                if(i == GDR_ITEM_SLOT)
                {
                    this->GetResObj<CIFMacroAlchemySlot>(i)->m_pMySlot->m_pSlot->ClearSlot();
                }
                if(i == GDR_ITEM_SLOT_ENC)
                {
                    this->GetResObj<CIFMacroAlchemySlot>(i)->m_pMySlot->m_pSlot->ClearSlot();
                }
                if(i == GDR_ITEM_SLOT_PROOF)
                {
                    this->GetResObj<CIFMacroAlchemySlot>(i)->m_pMySlot->m_pSlot->ClearSlot();
                }
            }
        }
    }

}
void CNIFEnchantWnd::PressSocketTabButton()
{
    reinterpret_cast<void(__thiscall *)(CNIFEnchantWnd *)>(0x0059e2d0)(this);
    if(m_Settings->ENABLE_NEW_ALCHEMY)
    {
        this->KillTimer(EffectTimer);
        this->KillTimer(FuseTimer);
        m_CustomDataManager->m_NewAlchemyWorking = false;
        this->GetResObj(GDR_PROGRESS_BAR)->SetGWndSize(0, 12);
        this->GetResObj<CIFDecoratedStatic>(GDR_DECORATED_STATIC)->ShowGWnd(false);
        m_CustomDataManager->m_NewAlchemyProgress = 0;
        for(int i = 5010; i< 5030;i++)
        {
            if(this->GetResObj(i) != NULL)
            {
                this->GetResObj(i)->ShowGWnd(false);
                if(i == GDR_ITEM_SLOT)
                {
                    this->GetResObj<CIFMacroAlchemySlot>(i)->m_pMySlot->m_pSlot->ClearSlot();
                }
                if(i == GDR_ITEM_SLOT_ENC)
                {
                    this->GetResObj<CIFMacroAlchemySlot>(i)->m_pMySlot->m_pSlot->ClearSlot();
                }
                if(i == GDR_ITEM_SLOT_PROOF)
                {
                    this->GetResObj<CIFMacroAlchemySlot>(i)->m_pMySlot->m_pSlot->ClearSlot();
                }
            }
        }
    }
}
void CNIFEnchantWnd::PressNewAlchemyTab()
{
    if(this->GetResObj<CNIFSocketWnd>(72) != NULL)
    {
        this->GetResObj<CNIFSocketWnd>(72)->ShowGWnd(false);
    }
    if(this->GetResObj<CNIFAlchemyWnd>(5) != NULL)
    {
        this->GetResObj<CNIFAlchemyWnd>(5)->ShowGWnd(false);
    }

    for(int i = 5010; i< 5030;i++)
    {
        if(this->GetResObj(i) != NULL)
        {
            this->GetResObj(i)->ShowGWnd(true);
            if(i == GDR_START_BUTTON)
            {
                SetStartButtonState(false);
            }
            if(i == GDR_ITEM_SLOT)
            {
                this->GetResObj<CIFMacroAlchemySlot>(i)->m_pMySlot->m_pSlot->ClearSlot();
                this->GetResObj<CIFMacroAlchemySlot>(i)->ShowGWnd(true);
                this->GetResObj<CIFMacroAlchemySlot>(i)->BringToFront();
                this->GetResObj<CIFMacroAlchemySlot>(i)->m_pMySlot->m_pSlot->ShowGWnd(true);
                this->GetResObj<CIFMacroAlchemySlot>(i)->m_pMySlot->m_pSlot->BringToFront();
            }
            if(i == GDR_ITEM_SLOT_ENC)
            {
                this->GetResObj<CIFMacroAlchemySlot>(i)->m_pMySlot->m_pSlot->ClearSlot();
                this->GetResObj<CIFMacroAlchemySlot>(i)->ShowGWnd(true);
                this->GetResObj<CIFMacroAlchemySlot>(i)->BringToFront();
                this->GetResObj<CIFMacroAlchemySlot>(i)->m_pMySlot->m_pSlot->ShowGWnd(true);
                this->GetResObj<CIFMacroAlchemySlot>(i)->m_pMySlot->m_pSlot->BringToFront();
            }
            if(i == GDR_ITEM_SLOT_PROOF)
            {
                this->GetResObj<CIFMacroAlchemySlot>(i)->m_pMySlot->m_pSlot->ClearSlot();
                this->GetResObj<CIFMacroAlchemySlot>(i)->ShowGWnd(true);
                this->GetResObj<CIFMacroAlchemySlot>(i)->BringToFront();
                this->GetResObj<CIFMacroAlchemySlot>(i)->m_pMySlot->m_pSlot->ShowGWnd(true);
                this->GetResObj<CIFMacroAlchemySlot>(i)->m_pMySlot->m_pSlot->BringToFront();
            }
        }
    }
    ///
    this->KillTimer(EffectTimer);
    this->KillTimer(FuseTimer);
    m_CustomDataManager->m_NewAlchemyWorking = false;
    this->GetResObj(GDR_PROGRESS_BAR)->SetGWndSize(0, 12);
    this->GetResObj<CIFDecoratedStatic>(GDR_DECORATED_STATIC)->ShowGWnd(false);
    m_CustomDataManager->m_NewAlchemyProgress = 0;
    this->GetResObj(GDR_PROGRESS_BAR)->SetGWndSize(0, 12);
}

void CNIFEnchantWnd::OnTimerIMPL(int timerId) {
    if(timerId == EffectTimer)
    {
        this->GetResObj<CIFDecoratedStatic>(GDR_DECORATED_STATIC)->FUN_00634300();
        CGEffSoundBody::get()->PlaySound(L"snd_elixir_use");
        int barwidht = (333 * m_CustomDataManager->m_NewAlchemyProgress++) / 60;
        if(barwidht > 333)
        {
            barwidht = 333;
        }
        this->GetResObj(GDR_PROGRESS_BAR)->SetGWndSize(barwidht, 12);
    }
    if(timerId == FuseTimer)
    {
        this->KillTimer(FuseTimer);
        //m_CustomDataManager->m_NewAlchemyProgress = 0;
        //this->GetResObj(GDR_PROGRESS_BAR)->SetGWndSize(0, 12);

        if(this->GetResObj<CIFMacroAlchemySlot>(GDR_ITEM_SLOT)->m_pMySlot->m_pSlot->ItemInfo != NULL)
        {
            if(this->GetResObj<CIFMacroAlchemySlot>(GDR_ITEM_SLOT)->m_pMySlot->m_pSlot->ItemInfo->GetItemData() != NULL)
            {
                if(this->GetResObj<CIFMacroAlchemySlot>(GDR_ITEM_SLOT_ENC)->m_pMySlot->m_pSlot->ItemInfo != NULL)
                {
                    if(this->GetResObj<CIFMacroAlchemySlot>(GDR_ITEM_SLOT_ENC)->m_pMySlot->m_pSlot->ItemInfo->GetItemData() != NULL)
                    {
                        if(this->GetResObj<CIFMacroAlchemySlot>(GDR_ITEM_SLOT_PROOF)->m_pMySlot->m_pSlot->ItemInfo != NULL)
                        {
                            if(this->GetResObj<CIFMacroAlchemySlot>(GDR_ITEM_SLOT_PROOF)->m_pMySlot->m_pSlot->ItemInfo->GetItemData() != NULL)
                            {
                                CMsgStreamBuffer buf(0xB300);
                                buf << byte(0);
//                                static_cast<unsigned char>(m_ItemSlot->m_pMySlot->m_pSlot->GetInventorySlotIndex() + 13u);
                                buf << byte(this->GetResObj<CIFMacroAlchemySlot>(GDR_ITEM_SLOT)->m_pMySlot->m_pSlot->GetInventorySlotIndex() + 13);
                                buf << byte(this->GetResObj<CIFMacroAlchemySlot>(GDR_ITEM_SLOT_ENC)->m_pMySlot->m_pSlot->GetInventorySlotIndex() + 13);
                                buf << byte(this->GetResObj<CIFMacroAlchemySlot>(GDR_ITEM_SLOT_PROOF)->m_pMySlot->m_pSlot->GetInventorySlotIndex() + 13);
                                SendMsg(buf);
                            }
                        }
                        else
                        {
                            CMsgStreamBuffer buf(0xB300);
                            buf << byte(1);
//                                static_cast<unsigned char>(m_ItemSlot->m_pMySlot->m_pSlot->GetInventorySlotIndex() + 13u);
                            buf << byte(this->GetResObj<CIFMacroAlchemySlot>(GDR_ITEM_SLOT)->m_pMySlot->m_pSlot->GetInventorySlotIndex() + 13);
                            buf << byte(this->GetResObj<CIFMacroAlchemySlot>(GDR_ITEM_SLOT_ENC)->m_pMySlot->m_pSlot->GetInventorySlotIndex() + 13);
                          //  buf << byte(this->GetResObj<CIFMacroAlchemySlot>(GDR_ITEM_SLOT_PROOF)->m_pMySlot->m_pSlot->GetInventorySlotIndex() + 13);
                            SendMsg(buf);
                        }
                    }
                }
            }
        }

    }
    if(timerId == SuccessTimer)
    {
        this->GetResObj<CIFDecoratedStatic>(GDR_DECORATED_STATIC)->FUN_00634300();
      //  this->GetResObj<CIFDecoratedStatic>(GDR_DECORATED_STATIC)->ShowGWnd(false);
    }
    if(timerId == KillSuccessTimer)
    {
        this->KillTimer(SuccessTimer);
        this->KillTimer(KillSuccessTimer);
        this->GetResObj<CIFDecoratedStatic>(GDR_DECORATED_STATIC)->ShowGWnd(false);
    }
    if(timerId == FailTimer)
    {
        this->GetResObj<CIFDecoratedStatic>(GDR_DECORATED_STATIC)->FUN_00634300();
        //  this->GetResObj<CIFDecoratedStatic>(GDR_DECORATED_STATIC)->ShowGWnd(false);
    }
    if(timerId == KillFailTimer)
    {
        this->KillTimer(FailTimer);
        this->KillTimer(KillFailTimer);
        this->GetResObj<CIFDecoratedStatic>(GDR_DECORATED_STATIC)->ShowGWnd(false);
    }

    reinterpret_cast<void(__thiscall *)(CNIFEnchantWnd *, int)>(0x0046fd90)(this, timerId);
}
void CNIFEnchantWnd::AlchemyIsSucces()
{
    this->KillTimer(FuseTimer);
    this->KillTimer(EffectTimer);
    m_CustomDataManager->m_NewAlchemyWorking = false;
    this->GetResObj<CIFButton>(GDR_START_BUTTON)->SetText(L"Strengthen");
    m_CustomDataManager->m_NewAlchemyProgress = 0;
    this->GetResObj(GDR_PROGRESS_BAR)->SetGWndSize(0, 12);

    CIFDecoratedStatic * Slotdeco = this->GetResObj<CIFDecoratedStatic>(GDR_DECORATED_STATIC);
    Slotdeco->FUN_00634470N("interface\\alchemy\\alcm_effect_success.ddj");
    Slotdeco->FUN_00633910(1);
    Slotdeco->FUN_00633950(100);
    Slotdeco->FUN_00633960(1);
    Slotdeco->FUN_00633b90(0x40, 0x40);
    Slotdeco->FUN_00633970(0x10);
    Slotdeco->FUN_00634330(0);
    Slotdeco->FUN_00633c70(0);
    Slotdeco->BringToFront();
    Slotdeco->ShowGWnd(true);
    CGEffSoundBody::get()->PlaySound(L"snd_elixir_success");
    this->StartTimer(SuccessTimer, 60);
    this->StartTimer(KillSuccessTimer, 650);
}
void CNIFEnchantWnd::AlchemyIsFailed()
{
    this->KillTimer(FuseTimer);
    this->KillTimer(EffectTimer);
    m_CustomDataManager->m_NewAlchemyWorking = false;
    this->GetResObj<CIFButton>(GDR_START_BUTTON)->SetText(L"Strengthen");
    m_CustomDataManager->m_NewAlchemyProgress = 0;
    this->GetResObj(GDR_PROGRESS_BAR)->SetGWndSize(0, 12);


    CIFDecoratedStatic * Slotdeco = this->GetResObj<CIFDecoratedStatic>(GDR_DECORATED_STATIC);

    Slotdeco->FUN_00633910(0);
    Slotdeco->FUN_00634470N("interface\\alchemy\\alcm_effect_fail_1.ddj");
    /*Slotdeco->FUN_00633950(100);
    Slotdeco->FUN_00633960(1);
    Slotdeco->FUN_00633b90(0x40, 0x40);
    Slotdeco->FUN_00633970(0x10);*/
 //   Slotdeco->FUN_00634330(0);
  //  Slotdeco->FUN_00633910(1);
//    Slotdeco->FUN_00633c70(0);
    Slotdeco->BringToFront();
    Slotdeco->ShowGWnd(true);
    CGEffSoundBody::get()->PlaySound(L"snd_elixir_fail");
    this->StartTimer(FailTimer, 50);
    this->StartTimer(KillFailTimer, 600);
}
void CNIFEnchantWnd::ClearProofSlot()
{
    this->GetResObj<CIFMacroAlchemySlot>(GDR_ITEM_SLOT_PROOF)->m_pMySlot->m_pSlot->ClearSlot();
    this->GetResObj<CIFMacroAlchemySlot>(GDR_ITEM_SLOT_PROOF)->ShowGWnd(true);
    this->GetResObj<CIFMacroAlchemySlot>(GDR_ITEM_SLOT_PROOF)->m_pMySlot->m_pSlot->SetClickable(true);
    this->GetResObj<CIFMacroAlchemySlot>(GDR_ITEM_SLOT_PROOF)->BringToFront();
    this->GetResObj<CIFMacroAlchemySlot>(GDR_ITEM_SLOT_PROOF)->m_pMySlot->m_pSlot->BringToFront();
}
void CNIFEnchantWnd::PressNewAlchemyStart()
{
    if(GetItemInfoFromSlot() != 0)
    {
        if(this->GetResObj<CIFMacroAlchemySlot>(GDR_ITEM_SLOT_ENC)->m_pMySlot->m_pSlot->ItemInfo != NULL)
        {
            if(this->GetResObj<CIFMacroAlchemySlot>(GDR_ITEM_SLOT_ENC)->m_pMySlot->m_pSlot->ItemInfo->GetItemData() != NULL)
            {
                if(this->GetResObj<CIFMacroAlchemySlot>(GDR_ITEM_SLOT_PROOF)->m_pMySlot->m_pSlot->ItemInfo != NULL)
                {
                    if(this->GetResObj<CIFMacroAlchemySlot>(GDR_ITEM_SLOT_PROOF)->m_pMySlot->m_pSlot->ItemInfo->GetItemData() != NULL)
                    {
                        /// START FULLY ENHANT
                        if(!m_CustomDataManager->m_NewAlchemyWorking)
                        {
                            CIFDecoratedStatic * Slotdeco = this->GetResObj<CIFDecoratedStatic>(GDR_DECORATED_STATIC);
                            Slotdeco->FUN_00634470N("interface\\alchemy\\alcm_effect_prepare.ddj");
                            Slotdeco->FUN_00633910(1);
                            Slotdeco->FUN_00633950(100);
                            Slotdeco->FUN_00633960(1);
                            Slotdeco->FUN_00633b90(0x40, 0x40);
                            Slotdeco->FUN_00633970(0x10);
                            Slotdeco->FUN_00634330(0);
                            Slotdeco->FUN_00633c70(0);
                            Slotdeco->BringToFront();
                            Slotdeco->ShowGWnd(true);
                            this->StartTimer(EffectTimer, 40);
                            this->StartTimer(FuseTimer, 3000);
                            m_CustomDataManager->m_NewAlchemyWorking = true;
                            CGEffSoundBody::get()->PlaySound(L"snd_elixir_use");
                            this->GetResObj<CIFButton>(GDR_START_BUTTON)->SetText(L"Cancel");

                        }
                        else
                        {
                            this->KillTimer(FuseTimer);
                            m_CustomDataManager->m_NewAlchemyWorking = false;

                            g_pCGInterface->WriteSystemMessage(SYSLOG_NONE,TSM_GETTEXTPTR(L"UIIT_MSG_ALCHEMY_CANCELED_COMPOUND"));
                            this->GetResObj<CIFButton>(GDR_START_BUTTON)->SetText(L"Strengthen");
                            m_CustomDataManager->m_NewAlchemyWorking = false;
                            this->KillTimer(EffectTimer);
                            this->GetResObj(GDR_PROGRESS_BAR)->SetGWndSize(0, 12);
                            m_CustomDataManager->m_NewAlchemyProgress = 0;
                            this->GetResObj<CIFDecoratedStatic>(GDR_DECORATED_STATIC)->ShowGWnd(false);
                        }

                    }
                    else
                    {

                        if(!m_CustomDataManager->m_NewAlchemyWorking)
                        {
                            // START W/O PROOF ENC
                            g_pCGInterface->m_IRM.GetResObj<CIFNewMsgBox>(1394, 1)->SetType(0);
                            g_pCGInterface->m_IRM.GetResObj<CIFNewMsgBox>(1394, 1)->UpdateMenuSize();
                            g_pCGInterface->GetGuiFromList<CIFNewMsgBox>(1394)->ShowGWnd(true);
                        }
                        else
                        {
                            this->KillTimer(FuseTimer);
                            m_CustomDataManager->m_NewAlchemyWorking = false;

                            g_pCGInterface->WriteSystemMessage(SYSLOG_NONE,TSM_GETTEXTPTR(L"UIIT_MSG_ALCHEMY_CANCELED_COMPOUND"));
                            this->GetResObj<CIFButton>(GDR_START_BUTTON)->SetText(L"Strengthen");
                            m_CustomDataManager->m_NewAlchemyWorking = false;
                            this->KillTimer(EffectTimer);
                            this->GetResObj(GDR_PROGRESS_BAR)->SetGWndSize(0, 12);
                            m_CustomDataManager->m_NewAlchemyProgress = 0;
                            this->GetResObj<CIFDecoratedStatic>(GDR_DECORATED_STATIC)->ShowGWnd(false);
                        }
                        // START W/O PROO ENC
                    }
                }
                else
                {
                    if(!m_CustomDataManager->m_NewAlchemyWorking)
                    {
                    // START W/O PROOF ENC
                        g_pCGInterface->m_IRM.GetResObj<CIFNewMsgBox>(1394, 1)->SetType(0);
                        g_pCGInterface->m_IRM.GetResObj<CIFNewMsgBox>(1394, 1)->UpdateMenuSize();
                        g_pCGInterface->GetGuiFromList<CIFNewMsgBox>(1394)->ShowGWnd(true);
                    }
                    else
                    {
                        this->KillTimer(FuseTimer);
                        m_CustomDataManager->m_NewAlchemyWorking = false;

                        g_pCGInterface->WriteSystemMessage(SYSLOG_NONE,TSM_GETTEXTPTR(L"UIIT_MSG_ALCHEMY_CANCELED_COMPOUND"));
                        this->GetResObj<CIFButton>(GDR_START_BUTTON)->SetText(L"Strengthen");
                        m_CustomDataManager->m_NewAlchemyWorking = false;
                        this->KillTimer(EffectTimer);
                        this->GetResObj(GDR_PROGRESS_BAR)->SetGWndSize(0, 12);
                        m_CustomDataManager->m_NewAlchemyProgress = 0;
                        this->GetResObj<CIFDecoratedStatic>(GDR_DECORATED_STATIC)->ShowGWnd(false);
                    }
                }
            }
        }

    }
}
void CNIFEnchantWnd::StartAlchemyWOProofStone()
{
    if(GetItemInfoFromSlot() != 0)
    {
        if(this->GetResObj<CIFMacroAlchemySlot>(GDR_ITEM_SLOT_ENC)->m_pMySlot->m_pSlot->ItemInfo != NULL)
        {
            if(this->GetResObj<CIFMacroAlchemySlot>(GDR_ITEM_SLOT_ENC)->m_pMySlot->m_pSlot->ItemInfo->GetItemData() != NULL)
            {
                if(!m_CustomDataManager->m_NewAlchemyWorking)
                {
                    CIFDecoratedStatic * Slotdeco = this->GetResObj<CIFDecoratedStatic>(GDR_DECORATED_STATIC);
                    Slotdeco->FUN_00634470N("interface\\alchemy\\alcm_effect_prepare.ddj");
                    Slotdeco->FUN_00633910(1);
                    Slotdeco->FUN_00633950(100);
                    Slotdeco->FUN_00633960(1);
                    Slotdeco->FUN_00633b90(0x40, 0x40);
                    Slotdeco->FUN_00633970(0x10);
                    Slotdeco->FUN_00634330(0);
                    Slotdeco->FUN_00633c70(0);
                    Slotdeco->BringToFront();
                    Slotdeco->ShowGWnd(true);
                    this->StartTimer(EffectTimer, 40);
                    this->StartTimer(FuseTimer, 3000);
                    m_CustomDataManager->m_NewAlchemyWorking = true;
                    CGEffSoundBody::get()->PlaySound(L"snd_elixir_use");
                    this->GetResObj<CIFButton>(GDR_START_BUTTON)->SetText(L"Cancel");

                }
            }
        }

    }
}
bool CNIFEnchantWnd::OnCloseWndIMPL()
{
   /* if(this->GetGuiFromList<CIFAlchemyMacro>(AlchemyMacro) != NULL)
    {
        this->EraseWindowObj(this->GetGuiFromList<CIFAlchemyMacro>(AlchemyMacro));
    }*/
    bool aa = reinterpret_cast<bool(__thiscall *)(CNIFEnchantWnd *)>(0x005942a0)(this);

    return aa;
}
int CNIFEnchantWnd::Func_43()
{
    int b = reinterpret_cast<int(__thiscall *)(CNIFEnchantWnd *)>(0x0046cdf0)(this);
    return b;
}