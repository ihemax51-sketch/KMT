#include "IFMacroMenuAutoPotion.h"
#include "Game.h"
#include <BSLib/Debug.h>
#include <GInterface.h>
#include <IFSliderCtrl.h>
#include <ICPlayer.h>
#include <TextStringManager.h>
#include <CustomData/CustomCICPlayer.h>
#include <GlobalDataManager.h>
#include <CustomData/CustomSettingManager.h>
#include "IFMacro.h"
#include "IFMacroMenu.h"

static CIFSlotWithHelp* GetMacroInventorySlotSafe(
    CIFInventory* inventory, int slotIndex, CIFSlotWithHelp* configuredSlot)
{
    if (!inventory || slotIndex < 0 || !configuredSlot ||
        !configuredSlot->ItemInfo || !configuredSlot->ItemInfo->GetItemData())
        return 0;

    if (slotIndex >= (int)inventory->pSlots.size())
        return 0;

    CIFSlotWithHelp* liveSlot = inventory->pSlots[slotIndex];
    if (!liveSlot || !liveSlot->ItemInfo || !liveSlot->ItemInfo->GetItemData())
        return 0;

    if (liveSlot->ItemInfo->GetItemData()->RefObjectId !=
        configuredSlot->ItemInfo->GetItemData()->RefObjectId)
        return 0;

    return liveSlot;
}

static bool TryCalculatePercentage(
    unsigned int currentValue, unsigned int maximumValue, int& percentage)
{
    percentage = 0;
    if (maximumValue == 0)
        return false;

    const unsigned __int64 scaledValue =
        static_cast<unsigned __int64>(currentValue) * 100;
    const unsigned __int64 calculatedPercentage = scaledValue / maximumValue;

    percentage = calculatedPercentage > 100
        ? 100
        : static_cast<int>(calculatedPercentage);
    return true;
}

static void SchedulePercentageRetry(
    bool& usingPotion, bool& timerRunning, int timerId)
{
    usingPotion = false;
    if (!timerRunning && g_pCGInterface)
    {
        g_pCGInterface->StartTimer(timerId, 500);
        timerRunning = true;
    }
}

static bool IsAutoPotionPlayerReady()
{
    if (!m_Player || !m_Player->FirstSpawn ||
        !g_pCGInterface || !g_pMyPlayerObj ||
        g_pMyPlayerObj->CHARACTER_STATUS == 0 ||
        g_pMyPlayerObj->GetMaxHp() == 0 ||
        g_pMyPlayerObj->GetMaxMp() == 0 ||
        (g_pMyPlayerObj->CHARACTER_STATUS != Dead &&
         g_pMyPlayerObj->GetCurrentHp() == 0) ||
        !g_pCGInterface->GetMainPopup() ||
        !g_pCGInterface->GetMainPopup()->GetInventory() ||
        !g_pCGInterface->GetMainPopup()->GetEquipment())
    {
        return false;
    }

    return true;
}


#define GDR_CHAR_HP_FRAME 3
#define GDR_CHAR_HP_CHECKBOX 4
#define GDR_CHAR_HP_LABEL 5
#define GDR_CHAR_HP_LABEL_VALUE 6
#define GDR_CHAR_HP_SLIDER_BG 7
#define GDR_CHAR_HP_SLIDER 8

#define GDR_CHAR_MP_FRAME 9
#define GDR_CHAR_MP_CHECKBOX 10
#define GDR_CHAR_MP_LABEL 11
#define GDR_CHAR_MP_LABEL_VALUE 12
#define GDR_CHAR_MP_SLIDER_BG 13
#define GDR_CHAR_MP_SLIDER 14


#define GDR_CHAR_PILL_FRAME 15
#define GDR_CHAR_PILL_CHECKBOX 16
#define GDR_CHAR_PILL_LABEL 17

#define GDR_CHAR_SPEED_FRAME 18
#define GDR_CHAR_SPEED_CHECKBOX 19
#define GDR_CHAR_SPEED_LABEL 20

#define GDR_CHAR_PURI_FRAME 21
#define GDR_CHAR_PURI_CHECKBOX 22
#define GDR_CHAR_PURI_LABEL 23


#define GDR_PET_HP_FRAME 24
#define GDR_PET_HP_CHECKBOX 25
#define GDR_PET_HP_LABEL 26
#define GDR_PET_HP_LABEL_VALUE 27
#define GDR_PET_HP_SLIDER_BG 28
#define GDR_PET_HP_SLIDER 29


#define GDR_PET_MP_FRAME 30
#define GDR_PET_MP_CHECKBOX 31
#define GDR_PET_MP_LABEL 32
#define GDR_PET_MP_LABEL_VALUE 33
#define GDR_PET_MP_SLIDER_BG 34
#define GDR_PET_MP_SLIDER 35

#define GDR_PET_CURE_FRAME 36
#define GDR_PET_PILL_CHECKBOX 37
#define GDR_PET_PILL_LABEL 38
#define GDR_PET_RES_FRAME 39
#define GDR_PET_RES_CHECKBOX 40

#define GDR_PET_RES_LABEL 41
#define GDR_PET_FRAME 42
#define GDR_PET_CHECKBOX 43
#define GDR_PET_LABEL 44

#define GDR_CHAR_VIGOR_FRAME 45
#define GDR_CHAR_VIGOR_CHECKBOX 46
#define GDR_CHAR_VIGOR_LABEL 47
#define GDR_CHAR_VIGOR_LABEL_VALUE 48
#define GDR_CHAR_VIGOR_SLIDER_BG 49
#define GDR_CHAR_VIGOR_MP_FRAME 50
#define GDR_CHAR_VIGOR_MP_CHECKBOX 51
#define GDR_CHAR_VIGOR_MP_LABEL 52
#define GDR_CHAR_VIGOR_MP_LABEL_VALUE 53
#define GDR_CHAR_VIGOR_MP_SLIDER_BG 54
#define GDR_VIGOR_HP_SLIDER 55
#define GDR_VIGOR_MP_SLIDER 56



#define StartOfPetPage 24
#define EndOfPetPage 44

#define StartOfCharPage 3
#define EndOfCharPage 23


#define StartOfVigorPage 45
#define EndOfVigorPage 56


#define GDR_SAVE_BUTTON 57
#define GDR_CANCEL_BUTTON 58

#define HP_MACRO_SLOT 141
#define MP_MACRO_SLOT 142
#define CHAR_PILL_MACRO_SLOT 143
#define CHAR_PURI_PILL_MACRO_SLOT 144
#define CHAR_SPEED_MACRO_SLOT 145
#define VIGOR_HP_MACRO_SLOT 146
#define VIGOR_MP_MACRO_SLOT 147
#define MACRO_PET_HP_SLOT 148
#define MACRO_PET_MP_SLOT 149
#define MACRO_PET_PILL_SLOT 150
#define MACRO_PET_RES_SLOT 151
#define MACRO_PET_SUMMON_SLOT 152

GFX_IMPLEMENT_DYNCREATE(CIFMacroMenuAutoPotion, CIFWnd)
GFX_BEGIN_MESSAGE_MAP(CIFMacroMenuAutoPotion, CIFWnd)
                    ONG_COMMAND(300, &CIFMacroMenuAutoPotion::OnUnknownStuff)
                    ONG_COMMAND(301, &CIFMacroMenuAutoPotion::OnUnknownStuff)
                    ONG_COMMAND(302, &CIFMacroMenuAutoPotion::OnUnknownStuff)
                    ONG_COMMAND(GDR_SAVE_BUTTON, &CIFMacroMenuAutoPotion::SaveButton)
                    ONG_COMMAND(GDR_CANCEL_BUTTON, &CIFMacroMenuAutoPotion::CancelBtn)
GFX_END_MESSAGE_MAP()

CIFMacroMenuAutoPotion::CIFMacroMenuAutoPotion(void) {
    BS_DEBUG_LOW(">" __FUNCTION__);
    m_pTabsSecond = 0;
    Macro_AutoPotion = false;
    CharacterHpTimerRunning = false;
    CharacterMpTimerRunning = false;
    CharacterHpUsing = false;
    CharacterMpUsing = false;
    CharacterVigorTimerRunning = false;
    CharacterVigorUsing = false;
    CharacterVigorTimerRunningMP = false;
    CharacterVigorUsingMP = false;
    CharacterPillTimerRunning = false;
    CharacterPillUsing = false;
    CharacterPillTimerRunningPuri = false;
    CharacterPillUsingPuri = false;
    CharacterSpeedTimerRunning = false;
    PetHpTimerRunning = false;
    PetHpUsing = false;
    PetHgpTimerRunning = false;
    PetHgpUsing = false;
    PetPillTimerRunning = false;
    PetPillUsing = false;
    SummonTimerRunning = false;
    PetResTimerRunning = false;
}
CIFMacroMenuAutoPotion::~CIFMacroMenuAutoPotion(void) {
    if (m_pTabsSecond) {
        delete[] m_pTabsSecond;
        m_pTabsSecond = 0;
    }
    BS_DEBUG_LOW(">" __FUNCTION__);
}

bool CIFMacroMenuAutoPotion::OnCreate(long ln) {

    // Populate inherited members
    CIFWnd::OnCreate(ln);
    wnd_rect sz;
    m_IRM.LoadFromFile("clientlibrary\\resinfo\\ifmacromenuautopotion.txt");
    m_IRM.CreateInterfaceSection("Create", this);
    this->m_IRM.GetResObj(GDR_SAVE_BUTTON, 1)->SetText(KmtGetText(L"UIIT_KMT_SAVE"));
    m_IRM.GetResObj(58, 1)->SetText(KmtGetText(L"UIIT_KMT_CANCEL"));

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
                m_pTabsSecond[0]->SetText(KmtGetText(L"UIIT_KMT_CHARACTER"));
            }
                break;
            case 1:
            {
                m_pTabsSecond[1]->MoveGWnd(m_pTabsSecond[0]->GetPos().x + tabWidth + 10, m_pTabsSecond[0]->GetPos().y);
                m_pTabsSecond[1]->SetText(KmtGetText(L"UIIT_KMT_VIGOR"));
                m_pTabsSecond[1]->SetClickable(true);
            }
                break;
        }
        m_pTabsSecond[i]->sub_64CC30(0);



    }

    m_pTabsSecond[2]->MoveGWnd(m_pTabsSecond[1]->GetPos().x + tabWidth + 10, m_pTabsSecond[1]->GetPos().y);
    m_pTabsSecond[2]->SetText(KmtGetText(L"UIIT_KMT_PET"));
    m_pTabsSecond[2]->SetClickable(true);
    m_pTabsSecond[2]->sub_64CC30(0);




///TODO HP
    CharHP_CheckBox = m_IRM.GetResObj<CIFCheckBox>(GDR_CHAR_HP_CHECKBOX, 1);
    CharHP_CheckBox->SetCheckBoxState(false);

    CharHP_SliderValue = m_IRM.GetResObj<CIFStatic>(GDR_CHAR_HP_LABEL_VALUE, 1);
    CharHP_SliderValue->TB_Func_13("clientlibrary\\fellowpets\\pt_figure.ddj", 1, 1);
    CharHP_SliderValue->SetText(L"50");

    CharHP_Slider = m_IRM.GetResObj<CIFSliderCtrl>(GDR_CHAR_HP_SLIDER, 1);
    CharHP_Slider->SetSliderTexture("interface\\option\\opt_volume2_prev.ddj", "interface\\option\\opt_volume2_next.ddj", "interface\\option\\opt_volume2_thumb.ddj");
    CharHP_Slider->FUN_006596f0(0, 0, 524, 0, 20, 1); // 524 - 312
    CharHP_Slider->SetMaxButtonSlide(488);  // 488 - 276
    CharHP_Slider->FUN_006599e0(1, 100, 1, 1);
    CharHP_Slider->CurrenValueYuzde = 50;
    CharHP_Slider->FUN_006598a0(50, 100);
    m_IRM.GetResObj(GDR_CHAR_HP_LABEL, 1)->SetText(KmtGetText(L"UIIT_KMT_HP"));

    /// TODO MP

    CharMP_CheckBox = m_IRM.GetResObj<CIFCheckBox>(GDR_CHAR_MP_CHECKBOX, 1);
    CharMP_CheckBox->SetCheckBoxState(false);

    CharMP_SliderValue = this->m_IRM.GetResObj<CIFStatic>(GDR_CHAR_MP_LABEL_VALUE, 1);
    CharMP_SliderValue->TB_Func_13("clientlibrary\\fellowpets\\pt_figure.ddj", 1, 1);
    CharMP_SliderValue->SetText(L"50");


    CharMP_Slider = m_IRM.GetResObj<CIFSliderCtrl>(GDR_CHAR_MP_SLIDER, 1);
    CharMP_Slider->SetSliderTexture("interface\\option\\opt_volume2_prev.ddj", "interface\\option\\opt_volume2_next.ddj", "interface\\option\\opt_volume2_thumb.ddj");
    CharMP_Slider->FUN_006596f0(0, 0, 524, 0, 20, 1); // 524 - 312
    CharMP_Slider->SetMaxButtonSlide(488);  // 488 - 276
    CharMP_Slider->FUN_006599e0(1, 100, 1, 1);
    CharMP_Slider->CurrenValueYuzde = 50;
    CharMP_Slider->FUN_006598a0(50, 100);
    m_IRM.GetResObj(GDR_CHAR_MP_LABEL, 1)->SetText(KmtGetText(L"UIIT_KMT_MP"));

    CharPill_CheckBox = m_IRM.GetResObj<CIFCheckBox>(GDR_CHAR_PILL_CHECKBOX, 1);
    CharPill_CheckBox->SetCheckBoxState(false);
    m_IRM.GetResObj(GDR_CHAR_PILL_LABEL, 1)->SetText(KmtGetText(L"UIIT_KMT_UNIVERSAL_PILL"));

    CharSpeed_CheckBox = m_IRM.GetResObj<CIFCheckBox>(GDR_CHAR_SPEED_CHECKBOX, 1);
    CharSpeed_CheckBox->SetCheckBoxState(false);
    m_IRM.GetResObj(GDR_CHAR_SPEED_LABEL, 1)->SetText(KmtGetText(L"UIIT_KMT_SPEED_DRUG"));

    CharPuriPill_CheckBox = m_IRM.GetResObj<CIFCheckBox>(GDR_CHAR_PURI_CHECKBOX, 1);
    CharPuriPill_CheckBox->SetCheckBoxState(false);
    m_IRM.GetResObj(GDR_CHAR_PURI_LABEL, 1)->SetText(KmtGetText(L"UIIT_KMT_PURIFICATION_PILL"));


    //   HPSlot->SetClickable(false);
    PetHP_CheckBox = m_IRM.GetResObj<CIFCheckBox>(GDR_PET_HP_CHECKBOX, 1);
    PetHP_CheckBox->SetCheckBoxState(false);

    PetHP_SliderValue = m_IRM.GetResObj<CIFStatic>(GDR_PET_HP_LABEL_VALUE, 1);
    PetHP_SliderValue->TB_Func_13("clientlibrary\\fellowpets\\pt_figure.ddj", 1, 1);
    PetHP_SliderValue->SetText(L"50");



    PetHP_Slider = m_IRM.GetResObj<CIFSliderCtrl>(GDR_PET_HP_SLIDER, 1);
    PetHP_Slider->SetSliderTexture("interface\\option\\opt_volume2_prev.ddj", "interface\\option\\opt_volume2_next.ddj", "interface\\option\\opt_volume2_thumb.ddj");
    PetHP_Slider->FUN_006596f0(0, 0, 524, 0, 20, 1); // 524 - 312
    PetHP_Slider->SetMaxButtonSlide(488);  // 488 - 276
    PetHP_Slider->FUN_006599e0(1, 100, 1, 1);
    PetHP_Slider->CurrenValueYuzde = 50;
    PetHP_Slider->FUN_006598a0(50, 100);
    m_IRM.GetResObj(GDR_PET_HP_LABEL, 1)->SetText(KmtGetText(L"UIIT_KMT_HP"));



    //   HPSlot->SetClickable(false);
    PetMP_CheckBox = m_IRM.GetResObj<CIFCheckBox>(GDR_PET_MP_CHECKBOX, 1);
    PetMP_CheckBox->SetCheckBoxState(false);

    PetMP_SliderValue = m_IRM.GetResObj<CIFStatic>(GDR_PET_MP_LABEL_VALUE, 1);
    PetMP_SliderValue->TB_Func_13("clientlibrary\\fellowpets\\pt_figure.ddj", 1, 1);
    PetMP_SliderValue->SetText(L"50");



    PetMP_Slider = m_IRM.GetResObj<CIFSliderCtrl>(GDR_PET_MP_SLIDER, 1);
    PetMP_Slider->SetSliderTexture("interface\\option\\opt_volume2_prev.ddj", "interface\\option\\opt_volume2_next.ddj", "interface\\option\\opt_volume2_thumb.ddj");
    PetMP_Slider->FUN_006596f0(0, 0, 524, 0, 20, 1); // 524 - 312
    PetMP_Slider->SetMaxButtonSlide(488);  // 488 - 276
    PetMP_Slider->FUN_006599e0(1, 100, 1, 1);
    PetMP_Slider->CurrenValueYuzde = 50;
    PetMP_Slider->FUN_006598a0(50, 100);
    m_IRM.GetResObj(GDR_PET_MP_LABEL, 1)->SetText(KmtGetText(L"UIIT_KMT_HUNGURY"));



    PetPill_CheckBox = m_IRM.GetResObj<CIFCheckBox>(GDR_PET_PILL_CHECKBOX, 1);
    PetPill_CheckBox->SetCheckBoxState(false);

    m_IRM.GetResObj(GDR_PET_PILL_LABEL, 1)->SetText(KmtGetText(L"UIIT_KMT_CURE_STATUS"));


    PetRes_CheckBox = m_IRM.GetResObj<CIFCheckBox>(GDR_PET_RES_CHECKBOX, 1);
    PetRes_CheckBox->SetCheckBoxState(false);
    m_IRM.GetResObj(GDR_PET_RES_LABEL, 1)->SetText(KmtGetText(L"UIIT_KMT_RESURRECT"));


    PetSpawn_CheckBox = m_IRM.GetResObj<CIFCheckBox>(GDR_PET_CHECKBOX, 1);
    PetSpawn_CheckBox->SetCheckBoxState(false);
    m_IRM.GetResObj(GDR_PET_LABEL, 1)->SetText(KmtGetText(L"UIIT_KMT_SUMMON_TRIGGER"));



    CharVigorHP_CheckBox = m_IRM.GetResObj<CIFCheckBox>(GDR_CHAR_VIGOR_CHECKBOX, 1);
    CharVigorHP_CheckBox->SetCheckBoxState(false);

    CharVigorHP_SliderValue = m_IRM.GetResObj<CIFStatic>(GDR_CHAR_VIGOR_LABEL_VALUE, 1);
    CharVigorHP_SliderValue->TB_Func_13("clientlibrary\\fellowpets\\pt_figure.ddj", 1, 1);
    CharVigorHP_SliderValue->SetText(L"50");



    CharVigorHP_Slider = m_IRM.GetResObj<CIFSliderCtrl>(GDR_VIGOR_HP_SLIDER, 1);
    CharVigorHP_Slider->SetSliderTexture("interface\\option\\opt_volume2_prev.ddj", "interface\\option\\opt_volume2_next.ddj", "interface\\option\\opt_volume2_thumb.ddj");
    CharVigorHP_Slider->FUN_006596f0(0, 0, 524, 0, 20, 1); // 524 - 312
    CharVigorHP_Slider->SetMaxButtonSlide(488);  // 488 - 276
    CharVigorHP_Slider->FUN_006599e0(1, 100, 1, 1);
    CharVigorHP_Slider->CurrenValueYuzde = 50;
    CharVigorHP_Slider->FUN_006598a0(50, 100);
    m_IRM.GetResObj(GDR_CHAR_VIGOR_LABEL, 1)->SetText(KmtGetText(L"UIIT_KMT_VIGOR_HP"));


    CharVigorMP_CheckBox = m_IRM.GetResObj<CIFCheckBox>(GDR_CHAR_VIGOR_MP_CHECKBOX, 1);
    CharVigorMP_CheckBox->SetCheckBoxState(false);

    CharVigorMP_SliderValue = m_IRM.GetResObj<CIFStatic>(GDR_CHAR_VIGOR_MP_LABEL_VALUE, 1);
    CharVigorMP_SliderValue->TB_Func_13("clientlibrary\\fellowpets\\pt_figure.ddj", 1, 1);
    CharVigorMP_SliderValue->SetText(L"50");



    CharVigorMP_Slider = m_IRM.GetResObj<CIFSliderCtrl>(GDR_VIGOR_MP_SLIDER, 1);
    CharVigorMP_Slider->SetSliderTexture("interface\\option\\opt_volume2_prev.ddj", "interface\\option\\opt_volume2_next.ddj", "interface\\option\\opt_volume2_thumb.ddj");
    CharVigorMP_Slider->FUN_006596f0(0, 0, 524, 0, 20, 1); // 524 - 312
    CharVigorMP_Slider->SetMaxButtonSlide(488);  // 488 - 276
    CharVigorMP_Slider->FUN_006599e0(1, 100, 1, 1);
    CharVigorMP_Slider->CurrenValueYuzde = 50;
    CharVigorMP_Slider->FUN_006598a0(50, 100);
    m_IRM.GetResObj(GDR_CHAR_VIGOR_MP_LABEL, 1)->SetText(KmtGetText(L"UIIT_KMT_VIGOR_MP"));


    m_pTabsSecond[0]->sub_64CC30(1);


    MySlots[HP_SLOT] = m_IRM.GetResObj<CIFMacroSlot>(HP_MACRO_SLOT, 1);
    MySlots[HP_SLOT]->m_pMySlot->m_pSlot->SetType(0xC);
    MySlots[HP_SLOT]->m_pMySlot->m_pSlot->SetSlot(141);

    MySlots[MP_SLOT] = m_IRM.GetResObj<CIFMacroSlot>(MP_MACRO_SLOT, 1);
    MySlots[MP_SLOT]->m_pMySlot->m_pSlot->SetType(0xC);
    MySlots[MP_SLOT]->m_pMySlot->m_pSlot->SetSlot(142);

    MySlots[CHAR_PILL_SLOT] = m_IRM.GetResObj<CIFMacroSlot>(CHAR_PILL_MACRO_SLOT, 1);
    MySlots[CHAR_PILL_SLOT]->m_pMySlot->m_pSlot->SetType(0xC);
    MySlots[CHAR_PILL_SLOT]->m_pMySlot->m_pSlot->SetSlot(143);

    MySlots[CHAR_PURI_PILL_SLOT] = m_IRM.GetResObj<CIFMacroSlot>(CHAR_PURI_PILL_MACRO_SLOT, 1);
    MySlots[CHAR_PURI_PILL_SLOT]->m_pMySlot->m_pSlot->SetType(0xC);
    MySlots[CHAR_PURI_PILL_SLOT]->m_pMySlot->m_pSlot->SetSlot(144);


    MySlots[CHAR_SPEED_SLOT] = m_IRM.GetResObj<CIFMacroSlot>(CHAR_SPEED_MACRO_SLOT, 1);
    MySlots[CHAR_SPEED_SLOT]->m_pMySlot->m_pSlot->SetType(0xC);
    MySlots[CHAR_SPEED_SLOT]->m_pMySlot->m_pSlot->SetSlot(145);

    MySlots[CHAR_VIGOR_HP_SLOT] = m_IRM.GetResObj<CIFMacroSlot>(VIGOR_HP_MACRO_SLOT, 1);
    MySlots[CHAR_VIGOR_HP_SLOT]->m_pMySlot->m_pSlot->SetType(0xC);
    MySlots[CHAR_VIGOR_HP_SLOT]->m_pMySlot->m_pSlot->SetSlot(146);

    MySlots[CHAR_VIGOR_MP_SLOT] = m_IRM.GetResObj<CIFMacroSlot>(VIGOR_MP_MACRO_SLOT, 1);
    MySlots[CHAR_VIGOR_MP_SLOT]->m_pMySlot->m_pSlot->SetType(0xC);
    MySlots[CHAR_VIGOR_MP_SLOT]->m_pMySlot->m_pSlot->SetSlot(147);

    MySlots[PET_HP_SLOT] = m_IRM.GetResObj<CIFMacroSlot>(MACRO_PET_HP_SLOT, 1);
    MySlots[PET_HP_SLOT]->m_pMySlot->m_pSlot->SetType(0xC);
    MySlots[PET_HP_SLOT]->m_pMySlot->m_pSlot->SetSlot(148);


    MySlots[PET_MP_SLOT] = m_IRM.GetResObj<CIFMacroSlot>(MACRO_PET_MP_SLOT, 1);
    MySlots[PET_MP_SLOT]->m_pMySlot->m_pSlot->SetType(0xC);
    MySlots[PET_MP_SLOT]->m_pMySlot->m_pSlot->SetSlot(149);

    MySlots[PET_PILL_SLOT] = m_IRM.GetResObj<CIFMacroSlot>(MACRO_PET_PILL_SLOT, 1);
    MySlots[PET_PILL_SLOT]->m_pMySlot->m_pSlot->SetType(0xC);
    MySlots[PET_PILL_SLOT]->m_pMySlot->m_pSlot->SetSlot(150);

    MySlots[PET_RES_SLOT] = m_IRM.GetResObj<CIFMacroSlot>(MACRO_PET_RES_SLOT, 1);
    MySlots[PET_RES_SLOT]->m_pMySlot->m_pSlot->SetType(0xC);
    MySlots[PET_RES_SLOT]->m_pMySlot->m_pSlot->SetSlot(151);

    MySlots[PET_SUMMON_SLOT] = m_IRM.GetResObj<CIFMacroSlot>(MACRO_PET_SUMMON_SLOT, 1);
    MySlots[PET_SUMMON_SLOT]->m_pMySlot->m_pSlot->SetType(0xC);
    MySlots[PET_SUMMON_SLOT]->m_pMySlot->m_pSlot->SetSlot(152);

    for(int i = StartOfCharPage; i < EndOfCharPage + 1; i++)
    {
        m_IRM.GetResObj(i, 1)->ShowGWnd(true);
    }

    for(int i = StartOfPetPage; i < EndOfPetPage + 1; i++)
    {
        m_IRM.GetResObj(i, 1)->ShowGWnd(false);
    }

    for(int i = StartOfVigorPage; i < EndOfVigorPage + 1; i++)
    {
        m_IRM.GetResObj(i, 1)->ShowGWnd(false);
    }


    return true;
}

void CIFMacroMenuAutoPotion::OnUpdate() {
    wchar_t number[10];
    swprintf_s(number, L"%d", CharHP_Slider->CurrenValueYuzde);
    CharHP_SliderValue->SetText(number);

    swprintf_s(number, L"%d", CharMP_Slider->CurrenValueYuzde);
    CharMP_SliderValue->SetText(number);

    swprintf_s(number, L"%d", PetHP_Slider->CurrenValueYuzde);
    PetHP_SliderValue->SetText(number);

    swprintf_s(number, L"%d", PetMP_Slider->CurrenValueYuzde);
    PetMP_SliderValue->SetText(number);

    swprintf_s(number, L"%d", CharVigorHP_Slider->CurrenValueYuzde);
    CharVigorHP_SliderValue->SetText(number);

    swprintf_s(number, L"%d", CharVigorMP_Slider->CurrenValueYuzde);
    CharVigorMP_SliderValue->SetText(number);

}

void CIFMacroMenuAutoPotion::StartAutomation()
{
    if (!g_pCGInterface)
        return;

    if (!Macro_AutoPotion)
    {
        StopAutomation();
        return;
    }

    if (!IsAutoPotionPlayerReady())
    {
        StopAutomation();
        g_pCGInterface->StartTimer(START_AUTO_POTION, 500);
        return;
    }

    g_pCGInterface->KillTimer(START_AUTO_POTION);

    CheckCharacterHP();
    CheckCharacterMP();
    CheckCharacterVigorHP();
    CheckCharacterVigorMP();
    CheckCharacterPILL();
    CheckCharacterPILLPuri();
    UseCharacterSpeed();
    CheckPetHP();
    CheckPetHgp();
    CheckPetPILL();
    CheckSummonedPet();
    CheckDeadPet();
}

void CIFMacroMenuAutoPotion::StopAutomation()
{
    if (!g_pCGInterface)
        return;

    const int timers[] = {
        HP_TIMER, HP_USINGTIMER, MP_TIMER, MP_USINGTIMER,
        VIGOR_TIMER_HP, VIGOR_USINGTIMER_HP,
        VIGOR_TIMER_MP, VIGOR_USINGTIMER_MP,
        PILL_TIMER, PILL_USINGTIMER, PILL_TIMER_PURI,
        PILL_USINGTIMER_PURI, SPEED_TIMER, PET_HP_TIMER,
        PET_HP_USINGTIMER, PET_HGP_TIMER, PET_HGP_USINGTIMER,
        PET_PILL_TIMER, PET_PILL_USINGTIMER, PET_SUMMON_TIMER,
        PET_RES_TIMER, START_AUTO_POTION
    };

    for (size_t i = 0; i < sizeof(timers) / sizeof(timers[0]); ++i)
        g_pCGInterface->KillTimer(timers[i]);

    CharacterHpTimerRunning = false;
    CharacterMpTimerRunning = false;
    CharacterHpUsing = false;
    CharacterMpUsing = false;
    CharacterVigorTimerRunning = false;
    CharacterVigorUsing = false;
    CharacterVigorTimerRunningMP = false;
    CharacterVigorUsingMP = false;
    CharacterPillTimerRunning = false;
    CharacterPillUsing = false;
    CharacterPillTimerRunningPuri = false;
    CharacterPillUsingPuri = false;
    CharacterSpeedTimerRunning = false;
    PetHpTimerRunning = false;
    PetHpUsing = false;
    PetHgpTimerRunning = false;
    PetHgpUsing = false;
    PetPillTimerRunning = false;
    PetPillUsing = false;
    SummonTimerRunning = false;
    PetResTimerRunning = false;
}

void CIFMacroMenuAutoPotion::UpdateSlot()
{

}
int CIFMacroMenuAutoPotion::GetHPItemID()
{
    if(MySlots[HP_SLOT]->m_pMySlot->m_pSlot->ItemInfo != NULL)
    {
        if(MySlots[HP_SLOT]->m_pMySlot->m_pSlot->ItemInfo->GetItemData() != NULL)
        {
            return MySlots[HP_SLOT]->m_pMySlot->m_pSlot->ItemInfo->GetItemData()->RefObjectId;
        }
    }
    return 0;
}
int CIFMacroMenuAutoPotion::GetMPItemID()
{
    if(MySlots[1]->m_pMySlot->m_pSlot->ItemInfo != NULL)
    {
        if(MySlots[1]->m_pMySlot->m_pSlot->ItemInfo->GetItemData() != NULL)
        {
            return  MySlots[1]->m_pMySlot->m_pSlot->ItemInfo->GetItemData()->RefObjectId;
        }
    }
    return 0;
}

void CIFMacroMenuAutoPotion::UpdatePetSlotVisual(byte i)
{
    CIFMacroSlot* petResSlot = MySlots[PET_RES_SLOT];
    if (petResSlot && petResSlot->m_pMySlot && petResSlot->m_pMySlot->m_pSlot &&
        petResSlot->m_pMySlot->m_pSlot->ItemInfo &&
        petResSlot->m_pMySlot->m_pSlot->GetInventorySlotType() == i)
    {
        int news = petResSlot->m_pMySlot->m_pSlot->ItemInfo->ActiveItemTime;
        petResSlot->m_pMySlot->m_pSlot->SetSlotVisual(news);
    }

}

void CIFMacroMenuAutoPotion::OnUnknownStuff() {
    int id = GetCurrentEventMsgCtrlId();
    int i = 0;
    //printf("%id %d\n", id);
    for (int i = 0; i < numberOfTabs; ++i) {
        if (id == m_pTabsSecond[i]->UniqueID()) {
            ActivateTabPage(i);
            return;
        }
    }
}
void CIFMacroMenuAutoPotion::ActivateTabPage(BYTE page) {
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

            for(int i = StartOfCharPage; i < EndOfCharPage + 1; i++)
            {
                m_IRM.GetResObj(i, 1)->ShowGWnd(true);
            }
            for(int i = StartOfPetPage; i < EndOfPetPage + 1; i++)
            {
                m_IRM.GetResObj(i, 1)->ShowGWnd(false);
            }
            for(int i = StartOfVigorPage; i < EndOfVigorPage + 1; i++)
            {
                m_IRM.GetResObj(i, 1)->ShowGWnd(false);
            }
            for(int i = HP_SLOT; i < CHAR_SPEED_SLOT + 1; i++)
            {
                MySlots[i]->ShowGWnd(true);
                MySlots[i]->BringToFront();
            }
            for(int i = CHAR_VIGOR_HP_SLOT; i < CHAR_VIGOR_MP_SLOT + 1; i++)
            {
                MySlots[i]->ShowGWnd(false);
            }
            for(int i = PET_HP_SLOT; i < PET_SUMMON_SLOT + 1; i++)
            {
                MySlots[i]->ShowGWnd(false);
            }
            g_pCGInterface->m_IRM.GetResObj<CIFMacroMenu>(MacroMenuID, 1)->descbox->SetText(KmtGetText(L"UIIT_KMT_SET_THE_OF_THE_HP_MP_GAUGE_TO_THE_DESIRED_LEVEL_TO_USE_POTIONS_AUTOMATICALLY_W_3649EBE7"));
        }
            break;
        case 1://name tag
        {
            for(int i = StartOfVigorPage; i < EndOfVigorPage + 1; i++)
            {
                m_IRM.GetResObj(i, 1)->ShowGWnd(true);
            }
            for(int i = StartOfPetPage; i < EndOfPetPage + 1; i++)
            {
                m_IRM.GetResObj(i, 1)->ShowGWnd(false);
            }
            for(int i = StartOfCharPage; i < EndOfCharPage + 1; i++)
            {
                m_IRM.GetResObj(i, 1)->ShowGWnd(false);
            }
            for(int i = HP_SLOT; i < CHAR_SPEED_SLOT + 1; i++)
            {
                MySlots[i]->ShowGWnd(false);
            }
            for(int i = CHAR_VIGOR_HP_SLOT; i < CHAR_VIGOR_MP_SLOT + 1; i++)
            {
                MySlots[i]->ShowGWnd(true);
                MySlots[i]->BringToFront();
            }
            for(int i = PET_HP_SLOT; i < PET_SUMMON_SLOT + 1; i++)
            {
                MySlots[i]->ShowGWnd(false);
            }
            g_pCGInterface->m_IRM.GetResObj<CIFMacroMenu>(MacroMenuID, 1)->descbox->SetText(KmtGetText(L"UIIT_KMT_SET_THE_OF_THE_VIGOR_HP_MP_GAUGE_TO_THE_DESIRED_LEVEL_TO_USE_POTIONS_AUTOMATIC_80830571"));

        }
            break;
        case 2:
        {
            for(int i = StartOfPetPage; i < EndOfPetPage + 1; i++)
            {
                m_IRM.GetResObj(i, 1)->ShowGWnd(true);
            }
            for(int i = StartOfCharPage; i < EndOfCharPage + 1; i++)
            {
                m_IRM.GetResObj(i, 1)->ShowGWnd(false);
            }
            for(int i = StartOfVigorPage; i < EndOfVigorPage + 1; i++)
            {
                m_IRM.GetResObj(i, 1)->ShowGWnd(false);
            }
            for(int i = HP_SLOT; i < CHAR_SPEED_SLOT + 1; i++)
            {
                MySlots[i]->ShowGWnd(false);
            }
            for(int i = CHAR_VIGOR_HP_SLOT; i < CHAR_VIGOR_MP_SLOT + 1; i++)
            {
                MySlots[i]->ShowGWnd(false);
            }
            for(int i = PET_HP_SLOT; i < PET_SUMMON_SLOT + 1; i++)
            {
                MySlots[i]->ShowGWnd(true);
                MySlots[i]->BringToFront();

            }
            g_pCGInterface->m_IRM.GetResObj<CIFMacroMenu>(1355, 1)->descbox->SetText(KmtGetText(L"UIIT_KMT_SET_THE_OF_THE_HP_HUNGER_GAUGE_TO_THE_DESIRED_LEVEL_TO_USE_POTIONS_AUTOMATICAL_0B2AB398"));

        }
    }
}


void CIFMacroMenuAutoPotion::UpdateMenuSize()
{
    int PosX = 0, PosY = 0;
    PosY = (g_CGame->GetRes().res->height/2 - 50) - (this->GetSize().height/2);
    PosX = (g_CGame->GetRes().res->width/2) - (this->GetSize().width/2);
    this->MoveGWnd(PosX, PosY);
    BringToFront();
}

void CIFMacroMenuAutoPotion::AutoSave(byte SlotSeq, byte SlotType, int SlotData)
{
    CMsgStreamBuffer buf(0x188A);
    buf << SlotSeq << SlotType << SlotData;
    SendMsg(buf);
}
void CIFMacroMenuAutoPotion::LoadSettings(byte Slot, byte Active, byte Value)
{
    if(Slot == HP_SLOT)
    {
        CharHP_CheckBox->SetCheckBoxState(Active);
        CharHP_Slider->FUN_006598a0(Value, 100);
    }
    else if (Slot == MP_SLOT)
    {
        CharMP_CheckBox->SetCheckBoxState(Active);
        CharMP_Slider->FUN_006598a0(Value, 100);
    }
    else if (Slot == CHAR_PILL_SLOT)
    {
        CharPill_CheckBox->SetCheckBoxState(Active);
    }
    else if (Slot == CHAR_PURI_PILL_SLOT)
    {
        CharPuriPill_CheckBox->SetCheckBoxState(Active);
    }
    else if (Slot == CHAR_SPEED_SLOT)
    {
        CharSpeed_CheckBox->SetCheckBoxState(Active);
    }
    else if (Slot == CHAR_VIGOR_HP_SLOT)
    {
        CharVigorHP_CheckBox->SetCheckBoxState(Active);
        CharVigorHP_Slider->FUN_006598a0(Value, 100);
    }
    else if (Slot == CHAR_VIGOR_MP_SLOT)
    {
        CharVigorMP_CheckBox->SetCheckBoxState(Active);
        CharVigorMP_Slider->FUN_006598a0(Value, 100);
    }
    else if (Slot == PET_HP_SLOT)
    {
        PetHP_CheckBox->SetCheckBoxState(Active);
        PetHP_Slider->FUN_006598a0(Value, 100);
    }
    else if (Slot == PET_MP_SLOT)
    {
        PetMP_CheckBox->SetCheckBoxState(Active);
        PetMP_Slider->FUN_006598a0(Value, 100);
    }
    else if (Slot == PET_PILL_SLOT)
    {
        PetPill_CheckBox->SetCheckBoxState(Active);
    }
    else if (Slot == PET_RES_SLOT)
    {
        PetRes_CheckBox->SetCheckBoxState(Active);
    }
    else if (Slot == PET_SUMMON_SLOT)
    {
        PetSpawn_CheckBox->SetCheckBoxState(Active);
    }
}
void CIFMacroMenuAutoPotion::SaveButton(){
    std::vector<Macro_AutoPotionData> MineDatas;

    Macro_AutoPotionData Data = Macro_AutoPotionData();
    Data.Slot = HP_SLOT;
    Data.Active = CharHP_CheckBox->GetCheckedState_MAYBE();
    Data.Value = CharHP_Slider->CurrenValueYuzde;
    MineDatas.push_back(Data);


    Data.Slot = MP_SLOT;
    Data.Active = CharMP_CheckBox->GetCheckedState_MAYBE();
    Data.Value = CharMP_Slider->CurrenValueYuzde;
    MineDatas.push_back(Data);

    Data.Slot = CHAR_PILL_SLOT;
    Data.Active = CharPill_CheckBox->GetCheckedState_MAYBE();
    Data.Value = 0;
    MineDatas.push_back(Data);


    Data.Slot = CHAR_PURI_PILL_SLOT;
    Data.Active = CharPuriPill_CheckBox->GetCheckedState_MAYBE();
    Data.Value = 0;
    MineDatas.push_back(Data);

    Data.Slot = CHAR_SPEED_SLOT;
    Data.Active = CharSpeed_CheckBox->GetCheckedState_MAYBE();
    Data.Value = 0;
    MineDatas.push_back(Data);

    Data.Slot = CHAR_VIGOR_HP_SLOT;
    Data.Active = CharVigorHP_CheckBox->GetCheckedState_MAYBE();
    Data.Value = CharVigorHP_Slider->CurrenValueYuzde;
    MineDatas.push_back(Data);

    Data.Slot = CHAR_VIGOR_MP_SLOT;
    Data.Active = CharVigorMP_CheckBox->GetCheckedState_MAYBE();
    Data.Value = CharVigorMP_Slider->CurrenValueYuzde;
    MineDatas.push_back(Data);

    Data.Slot = PET_HP_SLOT;
    Data.Active = PetHP_CheckBox->GetCheckedState_MAYBE();
    Data.Value = PetHP_Slider->CurrenValueYuzde;
    MineDatas.push_back(Data);

    Data.Slot = PET_MP_SLOT;
    Data.Active = PetMP_CheckBox->GetCheckedState_MAYBE();
    Data.Value = PetMP_Slider->CurrenValueYuzde;
    MineDatas.push_back(Data);

    Data.Slot = PET_PILL_SLOT;
    Data.Active = PetPill_CheckBox->GetCheckedState_MAYBE();
    Data.Value = 0;
    MineDatas.push_back(Data);


    Data.Slot = PET_RES_SLOT;
    Data.Active = PetRes_CheckBox->GetCheckedState_MAYBE();
    Data.Value = 0;
    MineDatas.push_back(Data);

    Data.Slot = PET_SUMMON_SLOT;
    Data.Active = PetSpawn_CheckBox->GetCheckedState_MAYBE();
    Data.Value = 0;
    MineDatas.push_back(Data);

    CMsgStreamBuffer buf(0x169A);
    buf << (byte)15;

    buf << (BYTE)MineDatas.size();

    for(std::vector<Macro_AutoPotionData>::iterator it = MineDatas.begin();
        it != MineDatas.end(); ++it)
    {
        buf << it->Slot << it->Active << it->Value;
    }
    SendMsg(buf);
}
undefined1 CIFMacroMenuAutoPotion::OnCloseWnd(){
    return CIFWnd::OnCloseWnd();
}
void CIFMacroMenuAutoPotion::CancelBtn(){
    g_pCGInterface->m_IRM.GetResObj<CIFMacroMenu>(1355, 1)->ShowGWnd(false);
}

void CIFMacroMenuAutoPotion::CheckCharacterHP()
{
    if(!g_pMyPlayerObj)
    {
        if (CharacterHpUsing)
        {
            CharacterHpUsing = false;
        }
        if (CharacterHpTimerRunning)
        {
            CharacterHpTimerRunning = false;
            g_pCGInterface->KillTimer(HP_TIMER);
        }
        return;
    }
    if (g_pMyPlayerObj->CHARACTER_STATUS == Dead || g_pMyPlayerObj->CHARACTER_STATUS == 0)
    {
        if (CharacterHpUsing)
        {
            CharacterHpUsing = false;
        }
        if (CharacterHpTimerRunning)
        {
            CharacterHpTimerRunning = false;
            g_pCGInterface->KillTimer(HP_TIMER);
        }
        return;
    }
    if(!Macro_AutoPotion)
    {
        if (CharacterHpUsing)
        {
            CharacterHpUsing = false;
        }
        if (CharacterHpTimerRunning)
        {
            CharacterHpTimerRunning = false;
            g_pCGInterface->KillTimer(HP_TIMER);
        }
        return;
    }

    int HpPercentage = 0;
    if (!TryCalculatePercentage(
            g_pMyPlayerObj->GetCurrentHp(),
            g_pMyPlayerObj->GetMaxHp(),
            HpPercentage))
    {
        SchedulePercentageRetry(
            CharacterHpUsing, CharacterHpTimerRunning, HP_TIMER);
        return;
    }
    int desiredHpPercentage = CharHP_Slider->CurrenValueYuzde;

    if (HpPercentage > desiredHpPercentage)
    {
        if (CharacterHpUsing)
        {
            CharacterHpUsing = false;
        }
        if (CharacterHpTimerRunning)
        {
            CharacterHpTimerRunning = false;
            g_pCGInterface->KillTimer(HP_TIMER);
        }
        return;
    }
    if ((g_pMyPlayerObj->m_abnormalState & ABNORMAL_STATE_ZOMBIE) != 0)
    {
        if (CharacterHpUsing)
        {
            CharacterHpUsing = false;
        }
        if (CharacterHpTimerRunning)
        {
            CharacterHpTimerRunning = false;
            g_pCGInterface->KillTimer(HP_TIMER);
        }
        return;
    }
    if (MySlots[HP_SLOT]->m_pMySlot->m_pSlot->GetType() == 0 || ! CharHP_CheckBox->GetCheckedState_MAYBE())
    {
        if (CharacterHpUsing)
        {
            CharacterHpUsing = false;
        }
        if (CharacterHpTimerRunning)
        {
            CharacterHpTimerRunning = false;
            g_pCGInterface->KillTimer(HP_TIMER);
        }
        return;
    }
    CIFInventory *inventory = g_pCGInterface->GetMainPopup()->GetInventory();
    if (inventory == NULL)
    {
        if (CharacterHpUsing)
        {
            CharacterHpUsing = false;
        }
        if (CharacterHpTimerRunning)
        {
            CharacterHpTimerRunning = false;
            g_pCGInterface->KillTimer(HP_TIMER);
        }
        return;
    }

    if (MySlots[HP_SLOT]->m_pMySlot->m_pSlot == NULL)
    {
        if (CharacterHpUsing)
        {
            CharacterHpUsing = false;
        }
        if (CharacterHpTimerRunning)
        {
            CharacterHpTimerRunning = false;
            g_pCGInterface->KillTimer(HP_TIMER);
        }
        return;
    }
    if (MySlots[HP_SLOT]->m_pMySlot->m_pSlot->ItemInfo == NULL)
    {
        if (CharacterHpUsing)
        {
            CharacterHpUsing = false;
        }
        if (CharacterHpTimerRunning)
        {
            CharacterHpTimerRunning = false;
            g_pCGInterface->KillTimer(HP_TIMER);
        }
        return;
    }

    if (MySlots[HP_SLOT]->m_pMySlot->m_pSlot->ItemInfo->GetItemData() == NULL)
    {
        if (CharacterHpUsing)
        {
            CharacterHpUsing = false;
        }
        if (CharacterHpTimerRunning)
        {
            CharacterHpTimerRunning = false;
            g_pCGInterface->KillTimer(HP_TIMER);
        }
        return;
    }
    if (!MySlots[HP_SLOT]->m_pMySlot->m_pSlot->ItemInfo->GetItemData()->IsHPPotion())
    {
        if (CharacterHpUsing)
        {
            CharacterHpUsing = false;
        }
        if (CharacterHpTimerRunning)
        {
            CharacterHpTimerRunning = false;
            g_pCGInterface->KillTimer(HP_TIMER);
        }
        return;
    }
    float delay = g_pCGInterface->GetItemReuseDelayManager()->FUN_009b94d0(1);
    if (delay > 0.0)
        return;

    if(!CharacterHpUsing)
    {
        CharacterHpUsing = true;
        g_pCGInterface->StartTimer(HP_USINGTIMER, 500);
    }
}
void CIFMacroMenuAutoPotion::UseCharacterHpPotion()
{

    if(!g_pMyPlayerObj)
    {
        if (CharacterHpUsing)
        {
            CharacterHpUsing = false;
        }
        if (CharacterHpTimerRunning)
        {
            CharacterHpTimerRunning = false;
            g_pCGInterface->KillTimer(HP_TIMER);
        }
        return;
    }
    if (g_pMyPlayerObj->CHARACTER_STATUS == Dead || g_pMyPlayerObj->CHARACTER_STATUS == 0)
    {
        if (CharacterHpUsing)
        {
            CharacterHpUsing = false;
        }
        if (CharacterHpTimerRunning)
        {
            CharacterHpTimerRunning = false;
            g_pCGInterface->KillTimer(HP_TIMER);
        }
        return;
    }
    if(!Macro_AutoPotion)
    {
        if (CharacterHpUsing)
        {
            CharacterHpUsing = false;
        }
        if (CharacterHpTimerRunning)
        {
            CharacterHpTimerRunning = false;
            g_pCGInterface->KillTimer(HP_TIMER);
        }
        return;
    }

    int HpPercentage = 0;
    if (!TryCalculatePercentage(
            g_pMyPlayerObj->GetCurrentHp(),
            g_pMyPlayerObj->GetMaxHp(),
            HpPercentage))
    {
        SchedulePercentageRetry(
            CharacterHpUsing, CharacterHpTimerRunning, HP_TIMER);
        return;
    }
    int desiredHpPercentage = CharHP_Slider->CurrenValueYuzde;

    if (HpPercentage > desiredHpPercentage)
    {
        if (CharacterHpUsing)
        {
            CharacterHpUsing = false;
        }
        if (CharacterHpTimerRunning)
        {
            CharacterHpTimerRunning = false;
            g_pCGInterface->KillTimer(HP_TIMER);
        }
        return;
    }
    if ((g_pMyPlayerObj->m_abnormalState & ABNORMAL_STATE_ZOMBIE) != 0)
    {
        if (CharacterHpUsing)
        {
            CharacterHpUsing = false;
        }
        if (CharacterHpTimerRunning)
        {
            CharacterHpTimerRunning = false;
            g_pCGInterface->KillTimer(HP_TIMER);
        }
        return;
    }
    if (MySlots[HP_SLOT]->m_pMySlot->m_pSlot->GetType() == 0 || ! CharHP_CheckBox->GetCheckedState_MAYBE())
    {
        if (CharacterHpUsing)
        {
            CharacterHpUsing = false;
        }
        if (CharacterHpTimerRunning)
        {
            CharacterHpTimerRunning = false;
            g_pCGInterface->KillTimer(HP_TIMER);
        }
        return;
    }
    CIFInventory *inventory = g_pCGInterface->GetMainPopup()->GetInventory();
    if (inventory == NULL)
    {
        if (CharacterHpUsing)
        {
            CharacterHpUsing = false;
        }
        if (CharacterHpTimerRunning)
        {
            CharacterHpTimerRunning = false;
            g_pCGInterface->KillTimer(HP_TIMER);
        }
        return;
    }

    if (MySlots[HP_SLOT]->m_pMySlot->m_pSlot == NULL)
    {
        if (CharacterHpUsing)
        {
            CharacterHpUsing = false;
        }
        if (CharacterHpTimerRunning)
        {
            CharacterHpTimerRunning = false;
            g_pCGInterface->KillTimer(HP_TIMER);
        }
        return;
    }
    if (MySlots[HP_SLOT]->m_pMySlot->m_pSlot->ItemInfo == NULL)
    {
        if (CharacterHpUsing)
        {
            CharacterHpUsing = false;
        }
        if (CharacterHpTimerRunning)
        {
            CharacterHpTimerRunning = false;
            g_pCGInterface->KillTimer(HP_TIMER);
        }
        return;
    }

    if (MySlots[HP_SLOT]->m_pMySlot->m_pSlot->ItemInfo->GetItemData() == NULL)
    {
        if (CharacterHpUsing)
        {
            CharacterHpUsing = false;
        }
        if (CharacterHpTimerRunning)
        {
            CharacterHpTimerRunning = false;
            g_pCGInterface->KillTimer(HP_TIMER);
        }
        return;
    }
    if (!MySlots[HP_SLOT]->m_pMySlot->m_pSlot->ItemInfo->GetItemData()->IsHPPotion())
    {
        if (CharacterHpUsing)
        {
            CharacterHpUsing = false;
        }
        if (CharacterHpTimerRunning)
        {
            CharacterHpTimerRunning = false;
            g_pCGInterface->KillTimer(HP_TIMER);
        }
        return;
    }
    float delay = g_pCGInterface->GetItemReuseDelayManager()->FUN_009b94d0(1);
    if (delay > 0.0)
        return;
    CIFSlotWithHelp* pSlot = GetMacroInventorySlotSafe(g_pCGInterface->GetMainPopup()->GetInventory(), MySlots[HP_SLOT]->m_pMySlot->m_pSlot->GetInventorySlotType(), MySlots[HP_SLOT]->m_pMySlot->m_pSlot);
    if(pSlot)
        pSlot->UseItem();

    CharacterHpUsing = false;
    if (!CharacterHpTimerRunning)
    {
        g_pCGInterface->StartTimer(HP_TIMER, 500);
        CharacterHpTimerRunning = true;
    }
}

void CIFMacroMenuAutoPotion::CheckCharacterMP()
{
    if(!g_pMyPlayerObj)
    {
        if (CharacterMpUsing)
        {
            CharacterMpUsing = false;
        }
        if (CharacterMpTimerRunning)
        {
            CharacterMpTimerRunning = false;
            g_pCGInterface->KillTimer(MP_TIMER);
        }
        return;
    }
    if (g_pMyPlayerObj->CHARACTER_STATUS == Dead || g_pMyPlayerObj->CHARACTER_STATUS == 0)
    {
        if (CharacterMpUsing)
        {
            CharacterMpUsing = false;
        }
        if (CharacterMpTimerRunning)
        {
            CharacterMpTimerRunning = false;
            g_pCGInterface->KillTimer(MP_TIMER);
        }
        return;
    }
    if(!Macro_AutoPotion)
    {
        if (CharacterMpUsing)
        {
            CharacterMpUsing = false;
        }
        if (CharacterMpTimerRunning)
        {
            CharacterMpTimerRunning = false;
            g_pCGInterface->KillTimer(MP_TIMER);
        }
        return;
    }

    int HpPercentage = 0;
    if (!TryCalculatePercentage(
            g_pMyPlayerObj->GetCurrentMp(),
            g_pMyPlayerObj->GetMaxMp(),
            HpPercentage))
    {
        SchedulePercentageRetry(
            CharacterMpUsing, CharacterMpTimerRunning, MP_TIMER);
        return;
    }
    int desiredHpPercentage = CharMP_Slider->CurrenValueYuzde;

    if (HpPercentage > desiredHpPercentage)
    {
        if (CharacterMpUsing)
        {
            CharacterMpUsing = false;
        }
        if (CharacterMpTimerRunning)
        {
            CharacterMpTimerRunning = false;
            g_pCGInterface->KillTimer(MP_TIMER);
        }
        return;
    }
    if (MySlots[MP_SLOT]->m_pMySlot->m_pSlot->GetType() == 0 || !CharMP_CheckBox->GetCheckedState_MAYBE())
    {
        if (CharacterMpUsing)
        {
            CharacterMpUsing = false;
        }
        if (CharacterMpTimerRunning)
        {
            CharacterMpTimerRunning = false;
            g_pCGInterface->KillTimer(MP_TIMER);
        }
        return;
    }
    CIFInventory *inventory = g_pCGInterface->GetMainPopup()->GetInventory();
    if (inventory == NULL)
    {
        if (CharacterMpUsing)
        {
            CharacterMpUsing = false;
        }
        if (CharacterMpTimerRunning)
        {
            CharacterMpTimerRunning = false;
            g_pCGInterface->KillTimer(MP_TIMER);
        }
        return;
    }
    if (MySlots[MP_SLOT]->m_pMySlot->m_pSlot == NULL)
    {
        if (CharacterMpUsing)
        {
            CharacterMpUsing = false;
        }
        if (CharacterMpTimerRunning)
        {
            CharacterMpTimerRunning = false;
            g_pCGInterface->KillTimer(MP_TIMER);
        }
        return;
    }
    if (MySlots[MP_SLOT]->m_pMySlot->m_pSlot->ItemInfo == NULL)
    {
        if (CharacterMpUsing)
        {
            CharacterMpUsing = false;
        }
        if (CharacterMpTimerRunning)
        {
            CharacterMpTimerRunning = false;
            g_pCGInterface->KillTimer(MP_TIMER);
        }
        return;
    }
    if (MySlots[MP_SLOT]->m_pMySlot->m_pSlot->ItemInfo->GetItemData() == NULL)
    {
        if (CharacterMpUsing)
        {
            CharacterMpUsing = false;
        }
        if (CharacterMpTimerRunning)
        {
            CharacterMpTimerRunning = false;
            g_pCGInterface->KillTimer(MP_TIMER);
        }
        return;
    }
    if (!MySlots[MP_SLOT]->m_pMySlot->m_pSlot->ItemInfo->GetItemData()->IsMPPotion())
    {
        if (CharacterMpUsing)
        {
            CharacterMpUsing = false;
        }
        if (CharacterMpTimerRunning)
        {
            CharacterMpTimerRunning = false;
            g_pCGInterface->KillTimer(MP_TIMER);
        }
        return;
    }
    float delay = g_pCGInterface->GetItemReuseDelayManager()->FUN_009b94d0(2);
    if (delay > 0.0)
        return;

    if(!CharacterMpUsing)
    {
        CharacterMpUsing = true;
        g_pCGInterface->StartTimer(MP_USINGTIMER, 500);
    }
}
void CIFMacroMenuAutoPotion::UseCharacterMpPotion()
{
    if(!g_pMyPlayerObj)
    {
        if (CharacterMpUsing)
        {
            CharacterMpUsing = false;
        }
        if (CharacterMpTimerRunning)
        {
            CharacterMpTimerRunning = false;
            g_pCGInterface->KillTimer(MP_TIMER);
        }
        return;
    }
    if (g_pMyPlayerObj->CHARACTER_STATUS == Dead || g_pMyPlayerObj->CHARACTER_STATUS == 0)
    {
        if (CharacterMpUsing)
        {
            CharacterMpUsing = false;
        }
        if (CharacterMpTimerRunning)
        {
            CharacterMpTimerRunning = false;
            g_pCGInterface->KillTimer(MP_TIMER);
        }
        return;
    }
    if(!Macro_AutoPotion)
    {
        if (CharacterMpUsing)
        {
            CharacterMpUsing = false;
        }
        if (CharacterMpTimerRunning)
        {
            CharacterMpTimerRunning = false;
            g_pCGInterface->KillTimer(MP_TIMER);
        }
        return;
    }

    int HpPercentage = 0;
    if (!TryCalculatePercentage(
            g_pMyPlayerObj->GetCurrentMp(),
            g_pMyPlayerObj->GetMaxMp(),
            HpPercentage))
    {
        SchedulePercentageRetry(
            CharacterMpUsing, CharacterMpTimerRunning, MP_TIMER);
        return;
    }
    int desiredHpPercentage = CharMP_Slider->CurrenValueYuzde;

    if (HpPercentage > desiredHpPercentage)
    {
        if (CharacterMpUsing)
        {
            CharacterMpUsing = false;
        }
        if (CharacterMpTimerRunning)
        {
            CharacterMpTimerRunning = false;
            g_pCGInterface->KillTimer(MP_TIMER);
        }
        return;
    }
    if (MySlots[MP_SLOT]->m_pMySlot->m_pSlot->GetType() == 0 || !CharMP_CheckBox->GetCheckedState_MAYBE())
    {
        if (CharacterMpUsing)
        {
            CharacterMpUsing = false;
        }
        if (CharacterMpTimerRunning)
        {
            CharacterMpTimerRunning = false;
            g_pCGInterface->KillTimer(MP_TIMER);
        }
        return;
    }
    CIFInventory *inventory = g_pCGInterface->GetMainPopup()->GetInventory();
    if (inventory == NULL)
    {
        if (CharacterMpUsing)
        {
            CharacterMpUsing = false;
        }
        if (CharacterMpTimerRunning)
        {
            CharacterMpTimerRunning = false;
            g_pCGInterface->KillTimer(MP_TIMER);
        }
        return;
    }
    if (MySlots[MP_SLOT]->m_pMySlot->m_pSlot == NULL)
    {
        if (CharacterMpUsing)
        {
            CharacterMpUsing = false;
        }
        if (CharacterMpTimerRunning)
        {
            CharacterMpTimerRunning = false;
            g_pCGInterface->KillTimer(MP_TIMER);
        }
        return;
    }
    if (MySlots[MP_SLOT]->m_pMySlot->m_pSlot->ItemInfo == NULL)
    {
        if (CharacterMpUsing)
        {
            CharacterMpUsing = false;
        }
        if (CharacterMpTimerRunning)
        {
            CharacterMpTimerRunning = false;
            g_pCGInterface->KillTimer(MP_TIMER);
        }
        return;
    }
    if (MySlots[MP_SLOT]->m_pMySlot->m_pSlot->ItemInfo->GetItemData() == NULL)
    {
        if (CharacterMpUsing)
        {
            CharacterMpUsing = false;
        }
        if (CharacterMpTimerRunning)
        {
            CharacterMpTimerRunning = false;
            g_pCGInterface->KillTimer(MP_TIMER);
        }
        return;
    }
    if (!MySlots[MP_SLOT]->m_pMySlot->m_pSlot->ItemInfo->GetItemData()->IsMPPotion())
    {
        if (CharacterMpUsing)
        {
            CharacterMpUsing = false;
        }
        if (CharacterMpTimerRunning)
        {
            CharacterMpTimerRunning = false;
            g_pCGInterface->KillTimer(MP_TIMER);
        }
        return;
    }
    float delay = g_pCGInterface->GetItemReuseDelayManager()->FUN_009b94d0(2);
    if (delay > 0.0)
        return;
    CIFSlotWithHelp* pSlot = GetMacroInventorySlotSafe(g_pCGInterface->GetMainPopup()->GetInventory(), MySlots[MP_SLOT]->m_pMySlot->m_pSlot->GetInventorySlotType(), MySlots[MP_SLOT]->m_pMySlot->m_pSlot);
    if(pSlot)
        pSlot->UseItem();

    CharacterMpUsing = false;
    if (!CharacterMpTimerRunning)
    {
        g_pCGInterface->StartTimer(MP_TIMER, 500);
        CharacterMpTimerRunning = true;
    }
}



void CIFMacroMenuAutoPotion::CheckCharacterVigorHP()
{
    if(!g_pMyPlayerObj)
    {
        if (CharacterVigorUsing)
        {
            CharacterVigorUsing = false;
        }
        if (CharacterVigorTimerRunning)
        {
            CharacterVigorTimerRunning = false;
            g_pCGInterface->KillTimer(VIGOR_TIMER_HP);
        }
        return;
    }
    if (g_pMyPlayerObj->CHARACTER_STATUS == Dead || g_pMyPlayerObj->CHARACTER_STATUS == 0)
    {
        if (CharacterVigorUsing)
        {
            CharacterVigorUsing = false;
        }
        if (CharacterVigorTimerRunning)
        {
            CharacterVigorTimerRunning = false;
            g_pCGInterface->KillTimer(VIGOR_TIMER_HP);
        }
        return;
    }
    if(!Macro_AutoPotion)
    {
        if (CharacterVigorUsing)
        {
            CharacterVigorUsing = false;
        }
        if (CharacterVigorTimerRunning)
        {
            CharacterVigorTimerRunning = false;
            g_pCGInterface->KillTimer(VIGOR_TIMER_HP);
        }
        return;
    }

    int HpPercentage = 0;
    if (!TryCalculatePercentage(
            g_pMyPlayerObj->GetCurrentHp(),
            g_pMyPlayerObj->GetMaxHp(),
            HpPercentage))
    {
        SchedulePercentageRetry(
            CharacterVigorUsing,
            CharacterVigorTimerRunning,
            VIGOR_TIMER_HP);
        return;
    }
    int desiredHpPercentage = CharVigorHP_Slider->CurrenValueYuzde;

    if (HpPercentage > desiredHpPercentage)
    {
        if (CharacterVigorUsing)
        {
            CharacterVigorUsing = false;
        }
        if (CharacterVigorTimerRunning)
        {
            CharacterVigorTimerRunning = false;
            g_pCGInterface->KillTimer(VIGOR_TIMER_HP);
        }
        return;
    }
    if ((g_pMyPlayerObj->m_abnormalState & ABNORMAL_STATE_ZOMBIE) != 0)
    {
        if (CharacterVigorUsing)
        {
            CharacterVigorUsing = false;
        }
        if (CharacterVigorTimerRunning)
        {
            CharacterVigorTimerRunning = false;
            g_pCGInterface->KillTimer(VIGOR_TIMER_HP);
        }
        return;
    }
    if (MySlots[CHAR_VIGOR_HP_SLOT]->m_pMySlot->m_pSlot->GetType() == 0 || !CharVigorHP_CheckBox->GetCheckedState_MAYBE())
    {
        if (CharacterVigorUsing)
        {
            CharacterVigorUsing = false;
        }
        if (CharacterVigorTimerRunning)
        {
            CharacterVigorTimerRunning = false;
            g_pCGInterface->KillTimer(VIGOR_TIMER_HP);
        }
        return;
    }
    CIFInventory *inventory = g_pCGInterface->GetMainPopup()->GetInventory();
    if (inventory == NULL)
    {
        if (CharacterVigorUsing)
        {
            CharacterVigorUsing = false;
        }
        if (CharacterVigorTimerRunning)
        {
            CharacterVigorTimerRunning = false;
            g_pCGInterface->KillTimer(VIGOR_TIMER_HP);
        }
        return;
    }

    if (MySlots[CHAR_VIGOR_HP_SLOT]->m_pMySlot->m_pSlot == NULL)
    {
        if (CharacterVigorUsing)
        {
            CharacterVigorUsing = false;
        }
        if (CharacterVigorTimerRunning)
        {
            CharacterVigorTimerRunning = false;
            g_pCGInterface->KillTimer(VIGOR_TIMER_HP);
        }
        return;
    }
    if (MySlots[CHAR_VIGOR_HP_SLOT]->m_pMySlot->m_pSlot->ItemInfo == NULL)
    {
        if (CharacterVigorUsing)
        {
            CharacterVigorUsing = false;
        }
        if (CharacterVigorTimerRunning)
        {
            CharacterVigorTimerRunning = false;
            g_pCGInterface->KillTimer(VIGOR_TIMER_HP);
        }
        return;
    }

    if (MySlots[CHAR_VIGOR_HP_SLOT]->m_pMySlot->m_pSlot->ItemInfo->GetItemData() == NULL)
    {
        if (CharacterVigorUsing)
        {
            CharacterVigorUsing = false;
        }
        if (CharacterVigorTimerRunning)
        {
            CharacterVigorTimerRunning = false;
            g_pCGInterface->KillTimer(VIGOR_TIMER_HP);
        }
        return;
    }
    if (!MySlots[CHAR_VIGOR_HP_SLOT]->m_pMySlot->m_pSlot->ItemInfo->GetItemData()->IsVIGOR())
    {
        if (CharacterVigorUsing)
        {
            CharacterVigorUsing = false;
        }
        if (CharacterVigorTimerRunning)
        {
            CharacterVigorTimerRunning = false;
            g_pCGInterface->KillTimer(VIGOR_TIMER_HP);
        }
        return;
    }
    float delay = g_pCGInterface->GetItemReuseDelayManager()->FUN_009b94d0(3);
    if (delay > 0.0)
        return;

    if(!CharacterVigorUsing)
    {
        CharacterVigorUsing = true;
        g_pCGInterface->StartTimer(VIGOR_USINGTIMER_HP, 500);
    }
}
void CIFMacroMenuAutoPotion::UseCharacterVigorHpPotion()
{
    if(!g_pMyPlayerObj)
    {
        if (CharacterVigorUsing)
        {
            CharacterVigorUsing = false;
        }
        if (CharacterVigorTimerRunning)
        {
            CharacterVigorTimerRunning = false;
            g_pCGInterface->KillTimer(VIGOR_TIMER_HP);
        }
        return;
    }
    if (g_pMyPlayerObj->CHARACTER_STATUS == Dead || g_pMyPlayerObj->CHARACTER_STATUS == 0)
    {
        if (CharacterVigorUsing)
        {
            CharacterVigorUsing = false;
        }
        if (CharacterVigorTimerRunning)
        {
            CharacterVigorTimerRunning = false;
            g_pCGInterface->KillTimer(VIGOR_TIMER_HP);
        }
        return;
    }
    if(!Macro_AutoPotion)
    {
        if (CharacterVigorUsing)
        {
            CharacterVigorUsing = false;
        }
        if (CharacterVigorTimerRunning)
        {
            CharacterVigorTimerRunning = false;
            g_pCGInterface->KillTimer(VIGOR_TIMER_HP);
        }
        return;
    }

    int HpPercentage = 0;
    if (!TryCalculatePercentage(
            g_pMyPlayerObj->GetCurrentHp(),
            g_pMyPlayerObj->GetMaxHp(),
            HpPercentage))
    {
        SchedulePercentageRetry(
            CharacterVigorUsing,
            CharacterVigorTimerRunning,
            VIGOR_TIMER_HP);
        return;
    }
    int desiredHpPercentage = CharVigorHP_Slider->CurrenValueYuzde;

    if (HpPercentage > desiredHpPercentage)
    {
        if (CharacterVigorUsing)
        {
            CharacterVigorUsing = false;
        }
        if (CharacterVigorTimerRunning)
        {
            CharacterVigorTimerRunning = false;
            g_pCGInterface->KillTimer(VIGOR_TIMER_HP);
        }
        return;
    }
    if ((g_pMyPlayerObj->m_abnormalState & ABNORMAL_STATE_ZOMBIE) != 0)
    {
        if (CharacterVigorUsing)
        {
            CharacterVigorUsing = false;
        }
        if (CharacterVigorTimerRunning)
        {
            CharacterVigorTimerRunning = false;
            g_pCGInterface->KillTimer(VIGOR_TIMER_HP);
        }
        return;
    }
    if (MySlots[CHAR_VIGOR_HP_SLOT]->m_pMySlot->m_pSlot->GetType() == 0 || !CharVigorHP_CheckBox->GetCheckedState_MAYBE())
    {
        if (CharacterVigorUsing)
        {
            CharacterVigorUsing = false;
        }
        if (CharacterVigorTimerRunning)
        {
            CharacterVigorTimerRunning = false;
            g_pCGInterface->KillTimer(VIGOR_TIMER_HP);
        }
        return;
    }
    CIFInventory *inventory = g_pCGInterface->GetMainPopup()->GetInventory();
    if (inventory == NULL)
    {
        if (CharacterVigorUsing)
        {
            CharacterVigorUsing = false;
        }
        if (CharacterVigorTimerRunning)
        {
            CharacterVigorTimerRunning = false;
            g_pCGInterface->KillTimer(VIGOR_TIMER_HP);
        }
        return;
    }

    if (MySlots[CHAR_VIGOR_HP_SLOT]->m_pMySlot->m_pSlot == NULL)
    {
        if (CharacterVigorUsing)
        {
            CharacterVigorUsing = false;
        }
        if (CharacterVigorTimerRunning)
        {
            CharacterVigorTimerRunning = false;
            g_pCGInterface->KillTimer(VIGOR_TIMER_HP);
        }
        return;
    }
    if (MySlots[CHAR_VIGOR_HP_SLOT]->m_pMySlot->m_pSlot->ItemInfo == NULL)
    {
        if (CharacterVigorUsing)
        {
            CharacterVigorUsing = false;
        }
        if (CharacterVigorTimerRunning)
        {
            CharacterVigorTimerRunning = false;
            g_pCGInterface->KillTimer(VIGOR_TIMER_HP);
        }
        return;
    }

    if (MySlots[CHAR_VIGOR_HP_SLOT]->m_pMySlot->m_pSlot->ItemInfo->GetItemData() == NULL)
    {
        if (CharacterVigorUsing)
        {
            CharacterVigorUsing = false;
        }
        if (CharacterVigorTimerRunning)
        {
            CharacterVigorTimerRunning = false;
            g_pCGInterface->KillTimer(VIGOR_TIMER_HP);
        }
        return;
    }
    if (!MySlots[CHAR_VIGOR_HP_SLOT]->m_pMySlot->m_pSlot->ItemInfo->GetItemData()->IsVIGOR())
    {
        if (CharacterVigorUsing)
        {
            CharacterVigorUsing = false;
        }
        if (CharacterVigorTimerRunning)
        {
            CharacterVigorTimerRunning = false;
            g_pCGInterface->KillTimer(VIGOR_TIMER_HP);
        }
        return;
    }
    float delay = g_pCGInterface->GetItemReuseDelayManager()->FUN_009b94d0(3);
    if (delay > 0.0)
        return;
    CIFSlotWithHelp* pSlot = GetMacroInventorySlotSafe(g_pCGInterface->GetMainPopup()->GetInventory(), MySlots[CHAR_VIGOR_HP_SLOT]->m_pMySlot->m_pSlot->GetInventorySlotType(), MySlots[CHAR_VIGOR_HP_SLOT]->m_pMySlot->m_pSlot);
    if(pSlot)
        pSlot->UseItem();

    CharacterVigorUsing = false;
    if (!CharacterVigorTimerRunning)
    {
        g_pCGInterface->StartTimer(VIGOR_TIMER_HP, 500);
        CharacterVigorTimerRunning = true;
    }
}


void CIFMacroMenuAutoPotion::CheckCharacterVigorMP()
{
    bool& CharacterVigorTimerRunning = CharacterVigorTimerRunningMP;
    bool& CharacterVigorUsing = CharacterVigorUsingMP;

    if(!g_pMyPlayerObj)
    {
        if (CharacterVigorUsing)
        {
            CharacterVigorUsing = false;
        }
        if (CharacterVigorTimerRunning)
        {
            CharacterVigorTimerRunning = false;
            g_pCGInterface->KillTimer(VIGOR_TIMER_MP);
        }
        return;
    }
    if (g_pMyPlayerObj->CHARACTER_STATUS == Dead || g_pMyPlayerObj->CHARACTER_STATUS == 0)
    {
        if (CharacterVigorUsing)
        {
            CharacterVigorUsing = false;
        }
        if (CharacterVigorTimerRunning)
        {
            CharacterVigorTimerRunning = false;
            g_pCGInterface->KillTimer(VIGOR_TIMER_MP);
        }
        return;
    }
    if(!Macro_AutoPotion)
    {
        if (CharacterVigorUsing)
        {
            CharacterVigorUsing = false;
        }
        if (CharacterVigorTimerRunning)
        {
            CharacterVigorTimerRunning = false;
            g_pCGInterface->KillTimer(VIGOR_TIMER_MP);
        }
        return;
    }

    int HpPercentage = 0;
    if (!TryCalculatePercentage(
            g_pMyPlayerObj->GetCurrentMp(),
            g_pMyPlayerObj->GetMaxMp(),
            HpPercentage))
    {
        SchedulePercentageRetry(
            CharacterVigorUsing,
            CharacterVigorTimerRunning,
            VIGOR_TIMER_MP);
        return;
    }
    int desiredHpPercentage = CharVigorMP_Slider->CurrenValueYuzde;

    if (HpPercentage > desiredHpPercentage)
    {
        if (CharacterVigorUsing)
        {
            CharacterVigorUsing = false;
        }
        if (CharacterVigorTimerRunning)
        {
            CharacterVigorTimerRunning = false;
            g_pCGInterface->KillTimer(VIGOR_TIMER_MP);
        }
        return;
    }
    if (MySlots[CHAR_VIGOR_MP_SLOT]->m_pMySlot->m_pSlot->GetSlot() == 0 || !CharVigorMP_CheckBox->GetCheckedState_MAYBE())
    {
        if (CharacterVigorUsing)
        {
            CharacterVigorUsing = false;
        }
        if (CharacterVigorTimerRunning)
        {
            CharacterVigorTimerRunning = false;
            g_pCGInterface->KillTimer(VIGOR_TIMER_MP);
        }
        return;
    }

    if (MySlots[CHAR_VIGOR_MP_SLOT]->m_pMySlot->m_pSlot == NULL)
    {
        if (CharacterVigorUsing)
        {
            CharacterVigorUsing = false;
        }
        if (CharacterVigorTimerRunning)
        {
            CharacterVigorTimerRunning = false;
            g_pCGInterface->KillTimer(VIGOR_TIMER_MP);
        }
        return;
    }
    if (MySlots[CHAR_VIGOR_MP_SLOT]->m_pMySlot->m_pSlot->ItemInfo == NULL)
    {
        if (CharacterVigorUsing)
        {
            CharacterVigorUsing = false;
        }
        if (CharacterVigorTimerRunning)
        {
            CharacterVigorTimerRunning = false;
            g_pCGInterface->KillTimer(VIGOR_TIMER_MP);
        }
        return;
    }

    if (MySlots[CHAR_VIGOR_MP_SLOT]->m_pMySlot->m_pSlot->ItemInfo->GetItemData() == NULL)
    {
        if (CharacterVigorUsing)
        {
            CharacterVigorUsing = false;
        }
        if (CharacterVigorTimerRunning)
        {
            CharacterVigorTimerRunning = false;
            g_pCGInterface->KillTimer(VIGOR_TIMER_MP);
        }
        return;
    }
    if (!MySlots[CHAR_VIGOR_MP_SLOT]->m_pMySlot->m_pSlot->ItemInfo->GetItemData()->IsVIGOR())
    {
        if (CharacterVigorUsing)
        {
            CharacterVigorUsing = false;
        }
        if (CharacterVigorTimerRunning)
        {
            CharacterVigorTimerRunning = false;
            g_pCGInterface->KillTimer(VIGOR_TIMER_MP);
        }
        return;
    }
    float delay = g_pCGInterface->GetItemReuseDelayManager()->FUN_009b94d0(3);
    if (delay > 0.0)
        return;

    if(!CharacterVigorUsing)
    {
        CharacterVigorUsing = true;
        g_pCGInterface->StartTimer(VIGOR_USINGTIMER_MP, 500);
    }
}
void CIFMacroMenuAutoPotion::UseCharacterVigorMpPotion()
{
    bool& CharacterVigorTimerRunning = CharacterVigorTimerRunningMP;
    bool& CharacterVigorUsing = CharacterVigorUsingMP;

    if(!g_pMyPlayerObj)
    {
        if (CharacterVigorUsing)
        {
            CharacterVigorUsing = false;
        }
        if (CharacterVigorTimerRunning)
        {
            CharacterVigorTimerRunning = false;
            g_pCGInterface->KillTimer(VIGOR_TIMER_MP);
        }
        return;
    }
    if (g_pMyPlayerObj->CHARACTER_STATUS == Dead || g_pMyPlayerObj->CHARACTER_STATUS == 0)
    {
        if (CharacterVigorUsing)
        {
            CharacterVigorUsing = false;
        }
        if (CharacterVigorTimerRunning)
        {
            CharacterVigorTimerRunning = false;
            g_pCGInterface->KillTimer(VIGOR_TIMER_MP);
        }
        return;
    }
    if(!Macro_AutoPotion)
    {
        if (CharacterVigorUsing)
        {
            CharacterVigorUsing = false;
        }
        if (CharacterVigorTimerRunning)
        {
            CharacterVigorTimerRunning = false;
            g_pCGInterface->KillTimer(VIGOR_TIMER_MP);
        }
        return;
    }

    int HpPercentage = 0;
    if (!TryCalculatePercentage(
            g_pMyPlayerObj->GetCurrentMp(),
            g_pMyPlayerObj->GetMaxMp(),
            HpPercentage))
    {
        SchedulePercentageRetry(
            CharacterVigorUsing,
            CharacterVigorTimerRunning,
            VIGOR_TIMER_MP);
        return;
    }
    int desiredHpPercentage = CharVigorMP_Slider->CurrenValueYuzde;

    if (HpPercentage > desiredHpPercentage)
    {
        if (CharacterVigorUsing)
        {
            CharacterVigorUsing = false;
        }
        if (CharacterVigorTimerRunning)
        {
            CharacterVigorTimerRunning = false;
            g_pCGInterface->KillTimer(VIGOR_TIMER_MP);
        }
        return;
    }
    if (MySlots[CHAR_VIGOR_MP_SLOT]->m_pMySlot->m_pSlot->GetSlot() == 0 || !CharVigorMP_CheckBox->GetCheckedState_MAYBE())
    {
        if (CharacterVigorUsing)
        {
            CharacterVigorUsing = false;
        }
        if (CharacterVigorTimerRunning)
        {
            CharacterVigorTimerRunning = false;
            g_pCGInterface->KillTimer(VIGOR_TIMER_MP);
        }
        return;
    }

    if (MySlots[CHAR_VIGOR_MP_SLOT]->m_pMySlot->m_pSlot == NULL)
    {
        if (CharacterVigorUsing)
        {
            CharacterVigorUsing = false;
        }
        if (CharacterVigorTimerRunning)
        {
            CharacterVigorTimerRunning = false;
            g_pCGInterface->KillTimer(VIGOR_TIMER_MP);
        }
        return;
    }
    if (MySlots[CHAR_VIGOR_MP_SLOT]->m_pMySlot->m_pSlot->ItemInfo == NULL)
    {
        if (CharacterVigorUsing)
        {
            CharacterVigorUsing = false;
        }
        if (CharacterVigorTimerRunning)
        {
            CharacterVigorTimerRunning = false;
            g_pCGInterface->KillTimer(VIGOR_TIMER_MP);
        }
        return;
    }

    if (MySlots[CHAR_VIGOR_MP_SLOT]->m_pMySlot->m_pSlot->ItemInfo->GetItemData() == NULL)
    {
        if (CharacterVigorUsing)
        {
            CharacterVigorUsing = false;
        }
        if (CharacterVigorTimerRunning)
        {
            CharacterVigorTimerRunning = false;
            g_pCGInterface->KillTimer(VIGOR_TIMER_MP);
        }
        return;
    }
    if (!MySlots[CHAR_VIGOR_MP_SLOT]->m_pMySlot->m_pSlot->ItemInfo->GetItemData()->IsVIGOR())
    {
        if (CharacterVigorUsing)
        {
            CharacterVigorUsing = false;
        }
        if (CharacterVigorTimerRunning)
        {
            CharacterVigorTimerRunning = false;
            g_pCGInterface->KillTimer(VIGOR_TIMER_MP);
        }
        return;
    }
    float delay = g_pCGInterface->GetItemReuseDelayManager()->FUN_009b94d0(3);
    if (delay > 0.0)
        return;

    CIFSlotWithHelp* pSlot = GetMacroInventorySlotSafe(g_pCGInterface->GetMainPopup()->GetInventory(), MySlots[CHAR_VIGOR_MP_SLOT]->m_pMySlot->m_pSlot->GetInventorySlotType(), MySlots[CHAR_VIGOR_MP_SLOT]->m_pMySlot->m_pSlot);
    if(pSlot)
        pSlot->UseItem();

    CharacterVigorUsing = false;
    if (!CharacterVigorTimerRunning)
    {
        g_pCGInterface->StartTimer(VIGOR_TIMER_MP, 500);
        CharacterVigorTimerRunning = true;
    }
}




void CIFMacroMenuAutoPotion::CheckCharacterPILL()
{
    if(g_pMyPlayerObj)
    {
        if (g_pMyPlayerObj->CHARACTER_STATUS != Dead && g_pMyPlayerObj->CHARACTER_STATUS != 0)
        {
            if(Macro_AutoPotion)
            {
                if (!CharacterPillTimerRunning)
                {
                    g_pCGInterface->StartTimer(PILL_TIMER, 400);
                    CharacterPillTimerRunning = true;
                }

                if (g_pMyPlayerObj->m_abnormalState & (ABNORMAL_STATE_ZOMBIE | ABNORMAL_STATE_FROZEN | ABNORMAL_STATE_FROSTBITTEN
                                                       | ABNORMAL_STATE_SHOCKED | ABNORMAL_STATE_BURN | ABNORMAL_STATE_POISONED)) {

                    if (MySlots[CHAR_PILL_SLOT]->m_pMySlot->m_pSlot->GetType() != 0 && CharPill_CheckBox->GetCheckedState_MAYBE())
                    {

                        if(MySlots[CHAR_PILL_SLOT]->m_pMySlot->m_pSlot != 0x0)
                        {
                            if(MySlots[CHAR_PILL_SLOT]->m_pMySlot->m_pSlot->ItemInfo != 0x0)
                            {
                                if(MySlots[CHAR_PILL_SLOT]->m_pMySlot->m_pSlot->ItemInfo->GetItemData() != 0x0)
                                {
                                    if(MySlots[CHAR_PILL_SLOT]->m_pMySlot->m_pSlot->ItemInfo->GetItemData()->IsPill())
                                    {
                                        float delay = g_pCGInterface->GetItemReuseDelayManager()->FUN_009b94d0(14);
                                        if (delay <= 0.0)
                                        {
                                            if(!CharacterPillUsing)
                                            {
                                                CharacterPillUsing = true;
                                                g_pCGInterface->StartTimer(PILL_USINGTIMER, 500);
                                            }
                                        }
                                    }
                                }

                            }
                        }

                    }

                }
            }
        }
        else
        {
            CharacterPillUsing = false;
            if(CharacterPillTimerRunning)
            {
                g_pCGInterface->KillTimer(PILL_TIMER);
                CharacterPillTimerRunning = false;
            }
        }
    }
    else
    {
        CharacterPillUsing = false;
        if(CharacterPillTimerRunning)
        {
            g_pCGInterface->KillTimer(PILL_TIMER);
            CharacterPillTimerRunning = false;
        }
    }
}
void CIFMacroMenuAutoPotion::UseCharacterPILL()
{
    if(g_pMyPlayerObj)
    {
        if (g_pMyPlayerObj->CHARACTER_STATUS != Dead && g_pMyPlayerObj->CHARACTER_STATUS != 0)
        {
            if(Macro_AutoPotion)
            {
                if (!CharacterPillTimerRunning)
                {
                    g_pCGInterface->StartTimer(PILL_TIMER, 400);
                    CharacterPillTimerRunning = true;
                }

                if (g_pMyPlayerObj->m_abnormalState & (ABNORMAL_STATE_ZOMBIE | ABNORMAL_STATE_FROZEN | ABNORMAL_STATE_FROSTBITTEN
                                                       | ABNORMAL_STATE_SHOCKED | ABNORMAL_STATE_BURN | ABNORMAL_STATE_POISONED)) {

                    if (MySlots[CHAR_PILL_SLOT]->m_pMySlot->m_pSlot->GetType() != 0 && CharPill_CheckBox->GetCheckedState_MAYBE())
                    {

                        if(MySlots[CHAR_PILL_SLOT]->m_pMySlot->m_pSlot != 0x0)
                        {
                            if(MySlots[CHAR_PILL_SLOT]->m_pMySlot->m_pSlot->ItemInfo != 0x0)
                            {
                                if(MySlots[CHAR_PILL_SLOT]->m_pMySlot->m_pSlot->ItemInfo->GetItemData() != 0x0)
                                {
                                    if(MySlots[CHAR_PILL_SLOT]->m_pMySlot->m_pSlot->ItemInfo->GetItemData()->IsPill())
                                    {
                                        float delay = g_pCGInterface->GetItemReuseDelayManager()->FUN_009b94d0(14);
                                        if (delay <= 0.0)
                                        {
                                            CharacterPillUsing = false;
                                            CIFSlotWithHelp* pSlot = GetMacroInventorySlotSafe(g_pCGInterface->GetMainPopup()->GetInventory(), MySlots[CHAR_PILL_SLOT]->m_pMySlot->m_pSlot->GetInventorySlotType(), MySlots[CHAR_PILL_SLOT]->m_pMySlot->m_pSlot);
                                            if(pSlot)
                                                pSlot->UseItem();


                                        }
                                    }
                                }

                            }
                        }
                    }
                }
            }
        }
        else
        {
            CharacterPillUsing = false;
            if(CharacterPillTimerRunning)
            {
                g_pCGInterface->KillTimer(PILL_TIMER);
                CharacterPillTimerRunning = false;
            }
        }
    }
    else
    {
        CharacterPillUsing = false;
        if(CharacterPillTimerRunning)
        {
            g_pCGInterface->KillTimer(PILL_TIMER);
            CharacterPillTimerRunning = false;
        }
    }
}


void CIFMacroMenuAutoPotion::CheckCharacterPILLPuri()
{
    if(g_pMyPlayerObj)
    {
        if (g_pMyPlayerObj->CHARACTER_STATUS != Dead && g_pMyPlayerObj->CHARACTER_STATUS != 0)
        {
            if(Macro_AutoPotion)
            {
                if (!CharacterPillTimerRunningPuri)
                {
                    g_pCGInterface->StartTimer(PILL_TIMER_PURI, 400);
                    CharacterPillTimerRunningPuri = true;
                }

                if (g_pMyPlayerObj->m_abnormalState & (ABNORMAL_STATE_SLEEP | ABNORMAL_STATE_BIND | ABNORMAL_STATE_DULL
                                                       | ABNORMAL_STATE_FEAR | ABNORMAL_STATE_SHORTSIGHTED | ABNORMAL_STATE_BLEED
                                                       | ABNORMAL_STATE_DARKNESS | ABNORMAL_STATE_DISEASE | ABNORMAL_STATE_DECAY
                                                       | ABNORMAL_STATE_WEAK | ABNORMAL_STATE_IMPOTENT | ABNORMAL_STATE_DIVISION
                                                       | ABNORMAL_STATE_PANIC | ABNORMAL_STATE_HIDDEN)) {

                    if (MySlots[CHAR_PURI_PILL_SLOT]->m_pMySlot->m_pSlot->GetSlotType() != 0 && CharPuriPill_CheckBox->GetCheckedState_MAYBE())
                    {
                        CIFInventory *inventory = g_pCGInterface->GetMainPopup()->GetInventory();
                        if(inventory != 0x0)
                        {

                            if(MySlots[CHAR_PURI_PILL_SLOT]->m_pMySlot->m_pSlot != 0x0)
                            {
                                if(MySlots[CHAR_PURI_PILL_SLOT]->m_pMySlot->m_pSlot->ItemInfo != 0x0)
                                {
                                    if(MySlots[CHAR_PURI_PILL_SLOT]->m_pMySlot->m_pSlot->ItemInfo->GetItemData() != 0x0)
                                    {
                                        if(MySlots[CHAR_PURI_PILL_SLOT]->m_pMySlot->m_pSlot->ItemInfo->GetItemData()->IsPuriPill())
                                        {
                                            float delay = g_pCGInterface->GetItemReuseDelayManager()->FUN_009b94d0(13);
                                            if (delay <= 0.0)
                                            {
                                                if(!CharacterPillUsingPuri)
                                                {
                                                    CharacterPillUsingPuri = true;
                                                    g_pCGInterface->StartTimer(PILL_USINGTIMER_PURI, 500);
                                                }
                                            }
                                        }
                                    }

                                }
                            }
                        }

                    }

                }
            }
        }
        else
        {
            CharacterPillUsingPuri = false;
            if(CharacterPillTimerRunningPuri)
            {
                g_pCGInterface->KillTimer(PILL_TIMER_PURI);
                CharacterPillTimerRunningPuri = false;
            }
        }
    }
    else
    {
        CharacterPillUsingPuri = false;
        if(CharacterPillTimerRunningPuri)
        {
            g_pCGInterface->KillTimer(PILL_TIMER_PURI);
            CharacterPillTimerRunningPuri = false;
        }
    }
}
void CIFMacroMenuAutoPotion::UseCharacterPILLPuri()
{
    if(g_pMyPlayerObj)
    {
        if (g_pMyPlayerObj->CHARACTER_STATUS != Dead && g_pMyPlayerObj->CHARACTER_STATUS != 0)
        {
            if(Macro_AutoPotion)
            {
                if (!CharacterPillTimerRunningPuri)
                {
                    g_pCGInterface->StartTimer(PILL_TIMER_PURI, 400);
                    CharacterPillTimerRunningPuri = true;
                }

                if (g_pMyPlayerObj->m_abnormalState & (ABNORMAL_STATE_SLEEP | ABNORMAL_STATE_BIND | ABNORMAL_STATE_DULL
                                                       | ABNORMAL_STATE_FEAR | ABNORMAL_STATE_SHORTSIGHTED | ABNORMAL_STATE_BLEED
                                                       | ABNORMAL_STATE_DARKNESS | ABNORMAL_STATE_DISEASE | ABNORMAL_STATE_DECAY
                                                       | ABNORMAL_STATE_WEAK | ABNORMAL_STATE_IMPOTENT | ABNORMAL_STATE_DIVISION
                                                       | ABNORMAL_STATE_PANIC | ABNORMAL_STATE_HIDDEN))
                {

                    if (MySlots[CHAR_PURI_PILL_SLOT]->m_pMySlot->m_pSlot->GetSlotType() != 0 && CharPuriPill_CheckBox->GetCheckedState_MAYBE())
                    {
                        CIFInventory *inventory = g_pCGInterface->GetMainPopup()->GetInventory();
                        if(inventory != 0x0)
                        {
                            if(MySlots[CHAR_PURI_PILL_SLOT]->m_pMySlot->m_pSlot != 0x0)
                            {
                                if(MySlots[CHAR_PURI_PILL_SLOT]->m_pMySlot->m_pSlot->ItemInfo != 0x0)
                                {
                                    if(MySlots[CHAR_PURI_PILL_SLOT]->m_pMySlot->m_pSlot->ItemInfo->GetItemData() != 0x0)
                                    {
                                        if(MySlots[CHAR_PURI_PILL_SLOT]->m_pMySlot->m_pSlot->ItemInfo->GetItemData()->IsPuriPill())
                                        {
                                            float delay = g_pCGInterface->GetItemReuseDelayManager()->FUN_009b94d0(13);
                                            if (delay <= 0.0)
                                            {
                                                CIFSlotWithHelp* pSlot = GetMacroInventorySlotSafe(g_pCGInterface->GetMainPopup()->GetInventory(), MySlots[CHAR_PURI_PILL_SLOT]->m_pMySlot->m_pSlot->GetInventorySlotType(), MySlots[CHAR_PURI_PILL_SLOT]->m_pMySlot->m_pSlot);
                                                if(pSlot)
                                                    pSlot->UseItem();

                                                CharacterPillUsingPuri = false;
                                            }
                                        }
                                    }

                                }
                            }
                        }

                    }

                }
            }
        }
        else
        {
            CharacterPillUsingPuri = false;
            if(CharacterPillTimerRunningPuri)
            {
                g_pCGInterface->KillTimer(PILL_TIMER_PURI);
                CharacterPillTimerRunningPuri = false;
            }
        }
    }
    else
    {
        CharacterPillUsingPuri = false;
        if(CharacterPillTimerRunningPuri)
        {
            g_pCGInterface->KillTimer(PILL_TIMER_PURI);
            CharacterPillTimerRunningPuri = false;
        }
    }
}
void CIFMacroMenuAutoPotion::UseCharacterSpeed(){
    if(!CharacterSpeedTimerRunning)
    {
        CharacterSpeedTimerRunning = true;
        g_pCGInterface->StartTimer(SPEED_TIMER, 15000);
    }
    if(g_pMyPlayerObj)
    {
        if (g_pMyPlayerObj->CHARACTER_STATUS != Dead && g_pMyPlayerObj->CHARACTER_STATUS != 0)
        {
            if(Macro_AutoPotion)
            {
                if (MySlots[CHAR_SPEED_SLOT]->m_pMySlot->m_pSlot->GetSlotType() != 0 && CharSpeed_CheckBox->GetCheckedState_MAYBE())
                {
                    if(MySlots[CHAR_SPEED_SLOT]->m_pMySlot->m_pSlot != 0x0)
                    {
                        if (MySlots[CHAR_SPEED_SLOT]->m_pMySlot->m_pSlot->ItemInfo != 0x0)
                        {
                            if(MySlots[CHAR_SPEED_SLOT]->m_pMySlot->m_pSlot->ItemInfo->GetItemData() != NULL)
                            {
                                if(MySlots[CHAR_SPEED_SLOT]->m_pMySlot->m_pSlot->ItemInfo->GetItemData()->m_desc1_128.find(L"SPEED") != std::n_wstring::npos)
                                {
                                    wchar_t *test = (wchar_t *) MySlots[CHAR_SPEED_SLOT]->m_pMySlot->m_pSlot->ItemInfo->GetItemData()->m_desc1_128.c_str();
                                    int ID = g_CGlobalDataManager->GetEffectIdByName(test);
                                    for(std::list<CIDecoSkill*>::iterator it = g_pMyPlayerObj->decolist.begin(); it != g_pMyPlayerObj->decolist.end();
                                        ++it)
                                    {
                                        if((*it)->BuffId == ID)
                                            return;
                                    }
                                    if(g_pMyPlayerObj->CheckSpeedOverlap(ID, (DWORD32)g_pMyPlayerObj))
                                    {
                                        return;
                                    }
                                    CIFSlotWithHelp* pSlot = GetMacroInventorySlotSafe(g_pCGInterface->GetMainPopup()->GetInventory(), MySlots[CHAR_SPEED_SLOT]->m_pMySlot->m_pSlot->GetInventorySlotType(), MySlots[CHAR_SPEED_SLOT]->m_pMySlot->m_pSlot);
                                    if(pSlot)
                                        pSlot->UseItem();
                                }
                            }

                        }
                    }

                }

            }

        }
    }

}




void CIFMacroMenuAutoPotion::CheckPetHP()
{
    if(g_pMyPlayerObj == NULL)
    {
        if (PetHpUsing)
        {
            PetHpUsing = false;
        }
        if (PetHpTimerRunning)
        {
            PetHpTimerRunning = false;
            g_pCGInterface->KillTimer(PET_HP_TIMER);
        }
        return;
    }
    if (g_pMyPlayerObj->CHARACTER_STATUS == Dead || g_pMyPlayerObj->CHARACTER_STATUS == 0)
    {
        if (PetHpUsing)
        {
            PetHpUsing = false;
        }
        if (PetHpTimerRunning)
        {
            PetHpTimerRunning = false;
            g_pCGInterface->KillTimer(PET_HP_TIMER);
        }
        return;
    }
    if(!Macro_AutoPotion)
    {
        if (PetHpUsing)
        {
            PetHpUsing = false;
        }
        if (PetHpTimerRunning)
        {
            PetHpTimerRunning = false;
            g_pCGInterface->KillTimer(PET_HP_TIMER);
        }
        return;
    }
    if (!g_pMyPlayerObj->CCOSDataMgr || !g_CGlobalDataManager)
    {
        SchedulePercentageRetry(PetHpUsing, PetHpTimerRunning, PET_HP_TIMER);
        return;
    }

    int FoundedCosUniqueID = 0;
    bool waitingForPetData = false;
    for(std::map<int, CCOSDataMgr::CosData*>::iterator it = g_pMyPlayerObj->CCOSDataMgr->CosList.begin();
        it != g_pMyPlayerObj->CCOSDataMgr->CosList.end(); ++it)
    {
        CICharactor *pUser = GetCharacterObjectByID_MAYBE(it->first);
        if (pUser != NULL) {
            static const CCharacterData *data = NULL;
            if (!pUser->GetCommonData())
            {
                waitingForPetData = true;
                continue;
            }

            data = g_CGlobalDataManager->GetCharacter(
                pUser->GetCommonData()->RefObjectId);
            if(data)
            {
                if(!data->IsGrapPet())
                {
                    if ((pUser->m_abnormalState & ABNORMAL_STATE_ZOMBIE) == 0)
                    {
                        int HpPercentage = 0;
                        if (!TryCalculatePercentage(
                                pUser->GetCurrentHp(),
                                data->GetData().MaxHP,
                                HpPercentage))
                        {
                            waitingForPetData = true;
                            continue;
                        }
                        int desiredHpPercentage = PetHP_Slider->CurrenValueYuzde;
                        if (HpPercentage < desiredHpPercentage)
                        {
                            FoundedCosUniqueID = it->first;
                        }
                    }
                }
            }
            else
            {
                waitingForPetData = true;
            }
        }
        else
        {
            waitingForPetData = true;
        }
    }

    if(FoundedCosUniqueID == 0)
    {
        if (PetHpUsing)
        {
            PetHpUsing = false;
        }
        if (waitingForPetData)
        {
            SchedulePercentageRetry(
                PetHpUsing, PetHpTimerRunning, PET_HP_TIMER);
        }
        else if (PetHpTimerRunning)
        {
            PetHpTimerRunning = false;
            g_pCGInterface->KillTimer(PET_HP_TIMER);
        }
        return;
    }

    if (MySlots[PET_HP_SLOT]->m_pMySlot->m_pSlot->GetSlotType() == 0 || !PetHP_CheckBox->GetCheckedState_MAYBE())
    {
        if (PetHpUsing)
        {
            PetHpUsing = false;
        }
        if (PetHpTimerRunning)
        {
            PetHpTimerRunning = false;
            g_pCGInterface->KillTimer(PET_HP_TIMER);
        }
        return;
    }
    CIFInventory *inventory = g_pCGInterface->GetMainPopup()->GetInventory();
    if (inventory == NULL)
    {
        if (PetHpUsing)
        {
            PetHpUsing = false;
        }
        if (PetHpTimerRunning)
        {
            PetHpTimerRunning = false;
            g_pCGInterface->KillTimer(PET_HP_TIMER);
        }
        return;
    }

    if (MySlots[PET_HP_SLOT]->m_pMySlot->m_pSlot == NULL)
    {
        if (PetHpUsing)
        {
            PetHpUsing = false;
        }
        if (PetHpTimerRunning)
        {
            PetHpTimerRunning = false;
            g_pCGInterface->KillTimer(PET_HP_TIMER);
        }
        return;
    }
    if (MySlots[PET_HP_SLOT]->m_pMySlot->m_pSlot->ItemInfo == NULL)
    {
        if (PetHpUsing)
        {
            PetHpUsing = false;
        }
        if (PetHpTimerRunning)
        {
            PetHpTimerRunning = false;
            g_pCGInterface->KillTimer(PET_HP_TIMER);
        }
        return;
    }

    if (MySlots[PET_HP_SLOT]->m_pMySlot->m_pSlot->ItemInfo->GetItemData() == NULL)
    {
        if (PetHpUsing)
        {
            PetHpUsing = false;
        }
        if (PetHpTimerRunning)
        {
            PetHpTimerRunning = false;
            g_pCGInterface->KillTimer(PET_HP_TIMER);
        }
        return;
    }
    if (!MySlots[PET_HP_SLOT]->m_pMySlot->m_pSlot->ItemInfo->GetItemData()->IsPETHpPotion())
    {
        if (PetHpUsing)
        {
            PetHpUsing = false;
        }
        if (PetHpTimerRunning)
        {
            PetHpTimerRunning = false;
            g_pCGInterface->KillTimer(PET_HP_TIMER);
        }
        return;
    }
    float delay = g_pCGInterface->GetItemReuseDelayManager()->FUN_009b94d0(4);
    if (delay > 0.0)
        return;

    if(!PetHpUsing)
    {
        PetHpUsing = true;
        g_pCGInterface->StartTimer(PET_HP_USINGTIMER, 500);
    }
}
void CIFMacroMenuAutoPotion::UsePetHpPotion()
{
    if(!g_pMyPlayerObj)
    {
        if (PetHpUsing)
        {
            PetHpUsing = false;
        }
        if (PetHpTimerRunning)
        {
            PetHpTimerRunning = false;
            g_pCGInterface->KillTimer(PET_HP_TIMER);
        }
        return;
    }
    if (g_pMyPlayerObj->CHARACTER_STATUS == Dead || g_pMyPlayerObj->CHARACTER_STATUS == 0)
    {
        if (PetHpUsing)
        {
            PetHpUsing = false;
        }
        if (PetHpTimerRunning)
        {
            PetHpTimerRunning = false;
            g_pCGInterface->KillTimer(PET_HP_TIMER);
        }
        return;
    }
    if(!Macro_AutoPotion)
    {
        if (PetHpUsing)
        {
            PetHpUsing = false;
        }
        if (PetHpTimerRunning)
        {
            PetHpTimerRunning = false;
            g_pCGInterface->KillTimer(PET_HP_TIMER);
        }
        return;
    }
    if (!g_pMyPlayerObj->CCOSDataMgr || !g_CGlobalDataManager)
    {
        SchedulePercentageRetry(PetHpUsing, PetHpTimerRunning, PET_HP_TIMER);
        return;
    }

    int FoundedCosUniqueID = 0;
    bool waitingForPetData = false;
    for(std::map<int, CCOSDataMgr::CosData*>::iterator it = g_pMyPlayerObj->CCOSDataMgr->CosList.begin();
        it != g_pMyPlayerObj->CCOSDataMgr->CosList.end(); ++it)
    {
        CICharactor *pUser = GetCharacterObjectByID_MAYBE(it->first);
        if (pUser != NULL) {
            static const CCharacterData *data = NULL;
            if (!pUser->GetCommonData())
            {
                waitingForPetData = true;
                continue;
            }

            data = g_CGlobalDataManager->GetCharacter(
                pUser->GetCommonData()->RefObjectId);
            if(data)
            {
                if(!data->IsGrapPet())
                {
                    if ((pUser->m_abnormalState & ABNORMAL_STATE_ZOMBIE) == 0)
                    {
                        int HpPercentage = 0;
                        if (!TryCalculatePercentage(
                                pUser->GetCurrentHp(),
                                data->GetData().MaxHP,
                                HpPercentage))
                        {
                            waitingForPetData = true;
                            continue;
                        }
                        int desiredHpPercentage = PetHP_Slider->CurrenValueYuzde;
                        if (HpPercentage < desiredHpPercentage)
                        {
                            FoundedCosUniqueID = it->first;
                        }
                    }
                }
            }
            else
            {
                waitingForPetData = true;
            }
        }
        else
        {
            waitingForPetData = true;
        }
    }

    if(FoundedCosUniqueID == 0)
    {
        if (PetHpUsing)
        {
            PetHpUsing = false;
        }
        if (waitingForPetData)
        {
            SchedulePercentageRetry(
                PetHpUsing, PetHpTimerRunning, PET_HP_TIMER);
        }
        else if (PetHpTimerRunning)
        {
            PetHpTimerRunning = false;
            g_pCGInterface->KillTimer(PET_HP_TIMER);
        }
        return;
    }

    if (MySlots[PET_HP_SLOT]->m_pMySlot->m_pSlot->GetSlotType() == 0 || !PetHP_CheckBox->GetCheckedState_MAYBE())
    {
        if (PetHpUsing)
        {
            PetHpUsing = false;
        }
        if (PetHpTimerRunning)
        {
            PetHpTimerRunning = false;
            g_pCGInterface->KillTimer(PET_HP_TIMER);
        }
        return;
    }
    CIFInventory *inventory = g_pCGInterface->GetMainPopup()->GetInventory();
    if (inventory == NULL)
    {
        if (PetHpUsing)
        {
            PetHpUsing = false;
        }
        if (PetHpTimerRunning)
        {
            PetHpTimerRunning = false;
            g_pCGInterface->KillTimer(PET_HP_TIMER);
        }
        return;
    }

    if (MySlots[PET_HP_SLOT]->m_pMySlot->m_pSlot == NULL)
    {
        if (PetHpUsing)
        {
            PetHpUsing = false;
        }
        if (PetHpTimerRunning)
        {
            PetHpTimerRunning = false;
            g_pCGInterface->KillTimer(PET_HP_TIMER);
        }
        return;
    }
    if (MySlots[PET_HP_SLOT]->m_pMySlot->m_pSlot->ItemInfo == NULL)
    {
        if (PetHpUsing)
        {
            PetHpUsing = false;
        }
        if (PetHpTimerRunning)
        {
            PetHpTimerRunning = false;
            g_pCGInterface->KillTimer(PET_HP_TIMER);
        }
        return;
    }

    if (MySlots[PET_HP_SLOT]->m_pMySlot->m_pSlot->ItemInfo->GetItemData() == NULL)
    {
        if (PetHpUsing)
        {
            PetHpUsing = false;
        }
        if (PetHpTimerRunning)
        {
            PetHpTimerRunning = false;
            g_pCGInterface->KillTimer(PET_HP_TIMER);
        }
        return;
    }
    if (!MySlots[PET_HP_SLOT]->m_pMySlot->m_pSlot->ItemInfo->GetItemData()->IsPETHpPotion())
    {
        if (PetHpUsing)
        {
            PetHpUsing = false;
        }
        if (PetHpTimerRunning)
        {
            PetHpTimerRunning = false;
            g_pCGInterface->KillTimer(PET_HP_TIMER);
        }
        return;
    }
    float delay = g_pCGInterface->GetItemReuseDelayManager()->FUN_009b94d0(4);
    if (delay > 0.0)
        return;
    CIFSlotWithHelp* pSlot = GetMacroInventorySlotSafe(inventory, MySlots[PET_HP_SLOT]->m_pMySlot->m_pSlot->GetInventorySlotType(), MySlots[PET_HP_SLOT]->m_pMySlot->m_pSlot);
    if(!pSlot || !pSlot->ItemInfo || !pSlot->ItemInfo->GetItemData())
        return;
    NEWMSG(0x704c)
        pReq << static_cast<unsigned char>(pSlot->GetSlot() + 13u) << pSlot->ItemInfo->GetItemData()->m_typeId;

        pReq << FoundedCosUniqueID;

    SENDMSG()

    PetHpUsing = false;
    if (!PetHpTimerRunning)
    {
        g_pCGInterface->StartTimer(PET_HP_TIMER, 500);
        PetHpTimerRunning = true;
    }
}

void CIFMacroMenuAutoPotion::CheckPetHgp(){
    if(!PetHgpTimerRunning)
    {
        PetHgpTimerRunning = true;
        g_pCGInterface->StartTimer(PET_HGP_TIMER, 5000);
    }
    if (!g_pMyPlayerObj) {
        if (PetHgpUsing)
        {
            PetHgpUsing = false;
        }
        return;
    }
    if (g_pMyPlayerObj->CHARACTER_STATUS == Dead || g_pMyPlayerObj->CHARACTER_STATUS == 0)
    {
        if (PetHgpUsing)
        {
            PetHgpUsing = false;
        }
        return;
    }
    if(!Macro_AutoPotion)
    {
        if (PetHgpUsing)
        {
            PetHgpUsing = false;
        }
        return;
    }

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
                if(data->IsAttackPet())
                {
                    double HpPercentage = (double(it->second->HGPValue) / 10000) * 100;
                    int desiredHpPercentage = PetMP_Slider->CurrenValueYuzde;
                    if (HpPercentage < (desiredHpPercentage-2))
                    {
                        FoundedCosUniqueID = it->first;
                    }
                }
            }
        }
    }
    if(FoundedCosUniqueID == 0)
    {
        if (PetHgpUsing)
        {
            PetHgpUsing = false;
        }
        return;
    }

    if (MySlots[PET_MP_SLOT]->m_pMySlot->m_pSlot->GetSlotType() == 0 || !PetMP_CheckBox->GetCheckedState_MAYBE())
    {
        if (PetHgpUsing)
        {
            PetHgpUsing = false;
        }
        return;
    }
    CIFInventory *inventory = g_pCGInterface->GetMainPopup()->GetInventory();
    if (inventory == 0x0)
    {
        if (PetHgpUsing)
        {
            PetHgpUsing = false;
        }
        return;
    }

    if (MySlots[PET_MP_SLOT]->m_pMySlot->m_pSlot == 0x0)
    {
        if (PetHgpUsing)
        {
            PetHgpUsing = false;
        }
        return;
    }
    if(MySlots[PET_MP_SLOT]->m_pMySlot->m_pSlot->ItemInfo == NULL)
    {
        if (PetHgpUsing)
        {
            PetHgpUsing = false;
        }
        return;
    }
    if(MySlots[PET_MP_SLOT]->m_pMySlot->m_pSlot->ItemInfo->GetItemData() == NULL)
    {
        if (PetHgpUsing)
        {
            PetHgpUsing = false;
        }
        return;
    }
    if(!MySlots[PET_MP_SLOT]->m_pMySlot->m_pSlot->ItemInfo->GetItemData()->IsPETMpPotion())
    {
        if (PetHgpUsing)
        {
            PetHgpUsing = false;
        }
        return;

    }
    float delay = g_pCGInterface->GetItemReuseDelayManager()->FUN_009b94d0(7);
    if (delay > 0.0)
        return;
    if(!PetHgpUsing)
    {
        PetHgpUsing = true;
        g_pCGInterface->StartTimer(PET_HGP_USINGTIMER, 500);
    }
}


void CIFMacroMenuAutoPotion::UsePetHgpPotion(){
    if(!PetHgpTimerRunning)
    {
        PetHgpTimerRunning = true;
        g_pCGInterface->StartTimer(PET_HGP_TIMER, 5000);
    }
    if (!g_pMyPlayerObj) {
        if (PetHgpUsing)
        {
            PetHgpUsing = false;
        }
        return;
    }
    if (g_pMyPlayerObj->CHARACTER_STATUS == Dead || g_pMyPlayerObj->CHARACTER_STATUS == 0)
    {
        if (PetHgpUsing)
        {
            PetHgpUsing = false;
        }
        return;
    }
    if(!Macro_AutoPotion)
    {
        if (PetHgpUsing)
        {
            PetHgpUsing = false;
        }
        return;
    }

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
                if(data->IsAttackPet())
                {
                    double HpPercentage = (double(it->second->HGPValue) / 10000) * 100;
                    int desiredHpPercentage = PetMP_Slider->CurrenValueYuzde;
                    if (HpPercentage < (desiredHpPercentage-2))
                    {
                        FoundedCosUniqueID = it->first;
                    }
                }
            }
        }
    }
    if(FoundedCosUniqueID == 0)
    {
        if (PetHgpUsing)
        {
            PetHgpUsing = false;
        }
        return;
    }

    if (MySlots[PET_MP_SLOT]->m_pMySlot->m_pSlot->GetSlotType() == 0 || !PetMP_CheckBox->GetCheckedState_MAYBE())
    {
        if (PetHgpUsing)
        {
            PetHgpUsing = false;
        }
        return;
    }
    CIFInventory *inventory = g_pCGInterface->GetMainPopup()->GetInventory();
    if (inventory == 0x0)
    {
        if (PetHgpUsing)
        {
            PetHgpUsing = false;
        }
        return;
    }

    if (MySlots[PET_MP_SLOT]->m_pMySlot->m_pSlot == 0x0)
    {
        if (PetHgpUsing)
        {
            PetHgpUsing = false;
        }
        return;
    }
    if(MySlots[PET_MP_SLOT]->m_pMySlot->m_pSlot->ItemInfo == NULL)
    {
        if (PetHgpUsing)
        {
            PetHgpUsing = false;
        }
        return;
    }
    if(MySlots[PET_MP_SLOT]->m_pMySlot->m_pSlot->ItemInfo->GetItemData() == NULL)
    {
        if (PetHgpUsing)
        {
            PetHgpUsing = false;
        }
        return;
    }
    if(!MySlots[PET_MP_SLOT]->m_pMySlot->m_pSlot->ItemInfo->GetItemData()->IsPETMpPotion())
    {
        if (PetHgpUsing)
        {
            PetHgpUsing = false;
        }
        return;

    }
    float delay = g_pCGInterface->GetItemReuseDelayManager()->FUN_009b94d0(7);
    if (delay > 0.0)
        return;
    CIFSlotWithHelp* pSlot = GetMacroInventorySlotSafe(inventory, MySlots[PET_MP_SLOT]->m_pMySlot->m_pSlot->GetInventorySlotType(), MySlots[PET_MP_SLOT]->m_pMySlot->m_pSlot);
    if(!pSlot || !pSlot->ItemInfo || !pSlot->ItemInfo->GetItemData())
        return;
    NEWMSG(0x704c)

        pReq << static_cast<unsigned char>(pSlot->GetSlot() + 13u) << pSlot->ItemInfo->GetItemData()->m_typeId;

        pReq << FoundedCosUniqueID;

    SENDMSG()

    PetHgpUsing = false;
    if(!PetHgpTimerRunning)
    {
        PetHgpTimerRunning = true;
        g_pCGInterface->StartTimer(PET_HGP_TIMER, 5000);
    }
}



void CIFMacroMenuAutoPotion::CheckPetPILL()
{
    if(g_pMyPlayerObj)
    {
        if (g_pMyPlayerObj->CHARACTER_STATUS != Dead && g_pMyPlayerObj->CHARACTER_STATUS != 0)
        {
            if(Macro_AutoPotion)
            {
                if (!PetPillTimerRunning)
                {
                    g_pCGInterface->StartTimer(PET_PILL_TIMER, 400);
                    PetPillTimerRunning = true;
                }
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
                            if(!data->IsGrapPet())
                            {
                                if (pUser->m_abnormalState & (ABNORMAL_STATE_ZOMBIE | ABNORMAL_STATE_FROZEN | ABNORMAL_STATE_FROSTBITTEN
                                                              | ABNORMAL_STATE_SHOCKED | ABNORMAL_STATE_BURN | ABNORMAL_STATE_POISONED))
                                {
                                    FoundedCosUniqueID = it->first;
                                }
                            }
                        }
                    }
                }
                if(FoundedCosUniqueID != 0)
                {

                    if (MySlots[PET_PILL_SLOT]->m_pMySlot->m_pSlot->GetSlotType() != 0 && PetPill_CheckBox->GetCheckedState_MAYBE())
                    {
                        CIFInventory *inventory = g_pCGInterface->GetMainPopup()->GetInventory();
                        if(inventory != 0x0)
                        {

                            if(MySlots[PET_PILL_SLOT]->m_pMySlot->m_pSlot != 0x0)
                            {
                                if(MySlots[PET_PILL_SLOT]->m_pMySlot->m_pSlot->ItemInfo != 0x0)
                                {
                                    if(MySlots[PET_PILL_SLOT]->m_pMySlot->m_pSlot->ItemInfo->GetItemData() != 0x0)
                                    {
                                        if(MySlots[PET_PILL_SLOT]->m_pMySlot->m_pSlot->ItemInfo->GetItemData()->IsCureCOS())
                                        {
                                            float delay = g_pCGInterface->GetItemReuseDelayManager()->FUN_009b94d0(15);
                                            if (delay <= 0.0)
                                            {
                                                if(!PetPillUsing)
                                                {
                                                    PetPillUsing = true;
                                                    g_pCGInterface->StartTimer(PET_PILL_USINGTIMER, 500);
                                                }
                                            }
                                        }
                                    }

                                }
                            }
                        }

                    }

                }
            }
        }
        else
        {
            PetPillUsing = false;
            if(PetPillTimerRunning)
            {
                g_pCGInterface->KillTimer(PET_PILL_TIMER);
                PetPillTimerRunning = false;
            }
        }
    }
    else
    {
        PetPillUsing = false;
        if(PetPillTimerRunning)
        {
            g_pCGInterface->KillTimer(PET_PILL_TIMER);
            PetPillTimerRunning = false;
        }
    }
}
void CIFMacroMenuAutoPotion::UsePetPILL()
{
    if(g_pMyPlayerObj)
    {
        if (g_pMyPlayerObj->CHARACTER_STATUS != Dead && g_pMyPlayerObj->CHARACTER_STATUS != 0)
        {
            if(Macro_AutoPotion)
            {
                if (!PetPillTimerRunning)
                {
                    g_pCGInterface->StartTimer(PET_PILL_TIMER, 400);
                    PetPillTimerRunning = true;
                }
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
                            if(!data->IsGrapPet())
                            {
                                if (pUser->m_abnormalState & (ABNORMAL_STATE_ZOMBIE | ABNORMAL_STATE_FROZEN | ABNORMAL_STATE_FROSTBITTEN
                                                              | ABNORMAL_STATE_SHOCKED | ABNORMAL_STATE_BURN | ABNORMAL_STATE_POISONED))
                                {
                                    FoundedCosUniqueID = it->first;
                                }
                            }
                        }
                    }
                }
                if(FoundedCosUniqueID != 0)
                {

                    if (MySlots[PET_PILL_SLOT]->m_pMySlot->m_pSlot->GetSlotType() != 0 && PetPill_CheckBox->GetCheckedState_MAYBE())
                    {
                        CIFInventory *inventory = g_pCGInterface->GetMainPopup()->GetInventory();
                        if(inventory != 0x0)
                        {
                            CIFSlotWithHelp* pSlot = GetMacroInventorySlotSafe(inventory, MySlots[PET_PILL_SLOT]->m_pMySlot->m_pSlot->GetInventorySlotType(), MySlots[PET_PILL_SLOT]->m_pMySlot->m_pSlot);
                            if(pSlot != 0x0)
                            {
                                if(pSlot->ItemInfo != 0x0)
                                {
                                    if(pSlot->ItemInfo->GetItemData() != 0x0)
                                    {
                                        if(pSlot->ItemInfo->GetItemData()->IsCureCOS())
                                        {
                                            float delay = g_pCGInterface->GetItemReuseDelayManager()->FUN_009b94d0(15);
                                            if (delay <= 0.0)
                                            {
                                                NEWMSG(0x704c)

                                                    pReq << static_cast<unsigned char>(pSlot->GetSlot() + 13u) << pSlot->ItemInfo->GetItemData()->m_typeId;

                                                    pReq << FoundedCosUniqueID;

                                                SENDMSG()
                                                PetPillUsing = false;
                                            }
                                        }
                                    }
                                }
                            }
                        }
                    }

                }
            }
        }
        else
        {
            PetPillUsing = false;
            if(PetPillTimerRunning)
            {
                g_pCGInterface->KillTimer(PET_PILL_TIMER);
                PetPillTimerRunning = false;
            }
        }
    }
    else
    {
        PetPillUsing = false;
        if(PetPillTimerRunning)
        {
            g_pCGInterface->KillTimer(PET_PILL_TIMER);
            PetPillTimerRunning = false;
        }
    }
}
void CIFMacroMenuAutoPotion::CheckSummonedPet(){
    if(!SummonTimerRunning)
    {
        SummonTimerRunning = true;
        g_pCGInterface->StartTimer(PET_SUMMON_TIMER, 12000);
    }
    if(g_pMyPlayerObj)
    {
        if (g_pMyPlayerObj->CHARACTER_STATUS != Dead &&
            g_pMyPlayerObj->CHARACTER_STATUS != Stall && g_pMyPlayerObj->CHARACTER_STATUS != 0)
        {
            if(Macro_AutoPotion)
            {
                if (MySlots[PET_SUMMON_SLOT]->m_pMySlot->m_pSlot->GetSlotType() != 0 && PetSpawn_CheckBox->GetCheckedState_MAYBE())
                {
                    CIFInventory *inventory = g_pCGInterface->GetMainPopup()->GetInventory();
                    if(inventory != 0x0)
                    {

                        if(MySlots[PET_SUMMON_SLOT]->m_pMySlot->m_pSlot != 0x0)
                        {
                            if(MySlots[PET_SUMMON_SLOT]->m_pMySlot->m_pSlot->ItemInfo != 0x0)
                            {
                                if(MySlots[PET_SUMMON_SLOT]->m_pMySlot->m_pSlot->ItemInfo->GetItemData() != 0x0)
                                {
                                    if (MySlots[PET_SUMMON_SLOT]->m_pMySlot->m_pSlot->ItemInfo->GetItemData()->IsAttackPet()
                                        && MySlots[PET_SUMMON_SLOT]->m_pMySlot->m_pSlot->ItemInfo->ActiveItemTime == 3)
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
                                                    if(data->IsAttackPet())
                                                    {
                                                        FoundedCosUniqueID = pUser->GetUniqueId();
                                                        return;
                                                    }

                                                }
                                            }
                                        }
                                        if(FoundedCosUniqueID == 0)
                                        {
                                            CIFSlotWithHelp* pSlot = GetMacroInventorySlotSafe(inventory, MySlots[PET_SUMMON_SLOT]->m_pMySlot->m_pSlot->GetInventorySlotType(), MySlots[PET_SUMMON_SLOT]->m_pMySlot->m_pSlot);
                                            if(pSlot)
                                                pSlot->UseItem();

                                        }
                                    }
                                }
                            }
                        }

                    }

                }

            }

        }
    }
}


void CIFMacroMenuAutoPotion::CheckDeadPet(){
    if(!PetResTimerRunning)
    {
        PetResTimerRunning = true;
        g_pCGInterface->StartTimer(PET_RES_TIMER, 7500);
    }
    if(g_pMyPlayerObj)
    {
        if (g_pMyPlayerObj->CHARACTER_STATUS != Dead &&
            g_pMyPlayerObj->CHARACTER_STATUS != Stall && g_pMyPlayerObj->CHARACTER_STATUS != 0)
        {
            if(Macro_AutoPotion)
            {

                if (MySlots[PET_RES_SLOT]->m_pMySlot->m_pSlot->GetSlotType() != 0 && PetRes_CheckBox->GetCheckedState_MAYBE())
                {
                    CIFInventory *inventory = g_pCGInterface->GetMainPopup()->GetInventory();
                    if(inventory != 0x0)
                    {
                        CIFSlotWithHelp* pSlot = GetMacroInventorySlotSafe(inventory, MySlots[PET_RES_SLOT]->m_pMySlot->m_pSlot->GetInventorySlotType(), MySlots[PET_RES_SLOT]->m_pMySlot->m_pSlot);
                        if(pSlot != 0x0)
                        {
                            if(pSlot->ItemInfo != 0x0)
                            {
                                if(pSlot->ItemInfo->GetItemData() != 0x0)
                                {
                                    if (pSlot->ItemInfo->GetItemData()->IsPetRes())
                                    {
                                        if (MySlots[PET_SUMMON_SLOT]->m_pMySlot->m_pSlot->GetSlotType() != 0)
                                        {
                                            if(MySlots[PET_SUMMON_SLOT]->m_pMySlot->m_pSlot != 0x0)
                                            {
                                                if(MySlots[PET_SUMMON_SLOT]->m_pMySlot->m_pSlot->ItemInfo != 0x0)
                                                {
                                                    if(MySlots[PET_SUMMON_SLOT]->m_pMySlot->m_pSlot->ItemInfo->GetItemData() != 0x0)
                                                    {
                                                        CIFSlotWithHelp* TargetPetSlot = GetMacroInventorySlotSafe(inventory, MySlots[PET_SUMMON_SLOT]->m_pMySlot->m_pSlot->GetInventorySlotType(), MySlots[PET_SUMMON_SLOT]->m_pMySlot->m_pSlot);
                                                        if(TargetPetSlot != 0x0)
                                                        {
                                                            if(TargetPetSlot->ItemInfo != 0x0)
                                                            {
                                                                if(TargetPetSlot->ItemInfo->GetItemData() != NULL)
                                                                {
                                                                    if(TargetPetSlot->ItemInfo->GetItemData()->IsAttackPet() && TargetPetSlot->ItemInfo->ActiveItemTime == 4)
                                                                    {
                                                                        NEWMSG(0x704c)

                                                                            pReq << static_cast<unsigned char>(pSlot->GetSlot() + 13u) << pSlot->ItemInfo->GetItemData()->m_typeId;

                                                                            pReq << static_cast<unsigned char>(TargetPetSlot->GetSlot() + 13u);

                                                                        SENDMSG()
                                                                    }

                                                                }
                                                            }
                                                        }
                                                    }
                                                }
                                            }
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
    }
}
