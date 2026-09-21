//
// Created by YUMBUL on 24.08.2024.
//

#include <CustomData/CustomCICPlayer.h>
#include "IFGhachaSelectWnd.h"
#include "IFCheckBox.h"

#define START_GAME_BUTTON 10

#define Title 602
bool CIFGhachaSelectWnd::OnCreateIMPL(long ln)
{
    bool aa = reinterpret_cast<bool(__thiscall *)(CIFGhachaSelectWnd *, long)>(0x0074a7c0)(this, ln);
    wnd_size size = this->GetSize();
    this->SetGWndSize(size.width + 200, size.height);

    wnd_size titlesize = this->m_IRM.GetResObj(602, 1)->GetSize();

    this->m_IRM.GetResObj(602, 1)->SetGWndSize(titlesize.width + 200, titlesize.height);

    this->m_IRM.GetResObj<CIFCheckBox>(700, 1)->SetCheckBoxState(true);
    this->m_IRM.GetResObj<CIFCheckBox>(701, 1)->SetCheckBoxState(false);
    this->m_IRM.GetResObj<CIFCheckBox>(702, 1)->SetCheckBoxState(false);
    m_Player->m_MagicPopSettings = 700;
    m_Player->m_MagicPopTimerRunning = false;
    this->m_IRM.GetResObj(705, 1)->SetText(KmtGetText(L"UIIT_KMT_AUTO_PLAY_AND_STOP_WHEN_WIN"));
    this->m_IRM.GetResObj(704, 1)->SetText(KmtGetText(L"UIIT_KMT_AUTO_REFILL_MAGIC_POP_SLOT"));
    this->m_IRM.GetResObj(703, 1)->SetText(KmtGetText(L"UIIT_KMT_NONE"));



    return aa;
}

void CIFGhachaSelectWnd::StartGame()
{
    CIFCheckBox *none = this->m_IRM.GetResObj<CIFCheckBox>(700, 1);
    CIFCheckBox *autoRefill = this->m_IRM.GetResObj<CIFCheckBox>(701, 1);
    CIFCheckBox *autoPlay = this->m_IRM.GetResObj<CIFCheckBox>(702, 1);

    // Read the controls at the final Start click. This is the authoritative
    // fallback when a checkbox changes visually but its custom command entry
    // is not dispatched by this client revision.
    if (m_Player != NULL) {
        if (autoPlay != NULL && autoPlay->GetCheckedState_MAYBE())
            m_Player->m_MagicPopSettings = 702;
        else if (autoRefill != NULL && autoRefill->GetCheckedState_MAYBE())
            m_Player->m_MagicPopSettings = 701;
        else
            m_Player->m_MagicPopSettings = 700;

        m_Player->m_MagicPopTimerRunning = false;
    }

    if (none != NULL)
        none->SetCheckBoxState(m_Player == NULL || m_Player->m_MagicPopSettings == 700);

    reinterpret_cast<void(__thiscall *)(CIFGhachaSelectWnd *)>(0x0074a2b0)(this);
}

GFX_MSGMAP* CIFGhachaSelectWnd::MessageMap(){
    static const GFX_MSGMAP_ENTRY skillBoardMessageEntries[] =
            {
                    /* {GFX_WM_COMMAND, 0, 14, 14, BSSig_u12, 0,
                             (GFX_PMSG)(static_cast< void (GFX_MSG_CALL CGWndBase::*)() >(&CIFSkillBoard::OnBtnClick))},
 */
                    {GFX_WM_COMMAND, 0, 700, 700, BSSig_u12, 0,
                            (GFX_PMSG)(static_cast< void (GFX_MSG_CALL CGWndBase::*)() >(&CIFGhachaSelectWnd::ClickNone))},
                    {GFX_WM_COMMAND, 0, 701, 701, BSSig_u12, 0,
                            (GFX_PMSG)(static_cast< void (GFX_MSG_CALL CGWndBase::*)() >(&CIFGhachaSelectWnd::ClickAutoRefill))},
                    {GFX_WM_COMMAND, 0, 702, 702, BSSig_u12, 0,
                            (GFX_PMSG)(static_cast< void (GFX_MSG_CALL CGWndBase::*)() >(&CIFGhachaSelectWnd::ClickAutoPlay))},
                    {0, 0, 0, 0, 0, 0, (GFX_PMSG)0, 0, 0, 0},
            };

    static GFX_MSGMAP newmap =
            {
                    reinterpret_cast<const GFX_MSGMAP *>(0x00db4b20), skillBoardMessageEntries,
            };
    return &newmap;
}
void CIFGhachaSelectWnd::ClickNone()
{
    m_Player->m_MagicPopSettings = 700;
    this->m_IRM.GetResObj<CIFCheckBox>(700, 1)->SetCheckBoxState(true);
    this->m_IRM.GetResObj<CIFCheckBox>(701, 1)->SetCheckBoxState(false);
    this->m_IRM.GetResObj<CIFCheckBox>(702, 1)->SetCheckBoxState(false);
}
void CIFGhachaSelectWnd::ClickAutoRefill()
{
    m_Player->m_MagicPopSettings = 701;
    this->m_IRM.GetResObj<CIFCheckBox>(700, 1)->SetCheckBoxState(false);
    this->m_IRM.GetResObj<CIFCheckBox>(701, 1)->SetCheckBoxState(true);
    this->m_IRM.GetResObj<CIFCheckBox>(702, 1)->SetCheckBoxState(false);
}
void CIFGhachaSelectWnd::ClickAutoPlay()
{
    m_Player->m_MagicPopSettings = 702;
    this->m_IRM.GetResObj<CIFCheckBox>(700, 1)->SetCheckBoxState(false);
    this->m_IRM.GetResObj<CIFCheckBox>(701, 1)->SetCheckBoxState(false);
    this->m_IRM.GetResObj<CIFCheckBox>(702, 1)->SetCheckBoxState(true);
}
