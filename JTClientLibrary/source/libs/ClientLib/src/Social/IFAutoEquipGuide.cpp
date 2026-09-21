#include "IFAutoEquipGuide.h"
#include <Game.h>
#include <GInterface.h>
#include <GEffSoundBody.h>
#include <TextStringManager.h>
#include <CustomData/CustomSettingManager.h>
#include <ICPlayer.h>

GFX_IMPLEMENT_DYNCREATE(CIFAutoEquipGuide, CIFDecoratedStatic)


bool CIFAutoEquipGuide::OnCreate(long ln)
{
    CIFDecoratedStatic::OnCreate(ln);

    TB_Func_13("clientlibrary\\guides\\kmt_auto_equip_1.ddj", 0, 0);
    sub_634470("clientlibrary\\guides\\kmt_auto_equip_2.ddj");

    set_N00009BD4(2);
    set_N00009BD3(500);

    m_IRM.LoadFromFile("clientlibrary\\resinfo\\ifsimple.txt");
    m_IRM.CreateInterfaceSection("Create", this);

    RECT rect = { 0,0,40,40 };
    namelabel = this->m_IRM.GetResObj<CIFStatic>(1, 0);
    std::n_wstring msg(KmtGetText(L"UIIT_KMT_AUTO_EQUIP"));
    namelabel->SetTooltip(msg);
    namelabel->SetStyleThingy(TOOLTIP);


    return true;
}

int CIFAutoEquipGuide::OnMouseLeftUp(int a1, int x, int y)
{
    if (!g_pMyPlayerObj || !m_Settings->ShowGuideAutoEquip ||
        (m_Settings->AutoEquipMaxLevel > 0 &&
         g_pMyPlayerObj->GetCurrentLevel() > m_Settings->AutoEquipMaxLevel))
        return 0;

    static DWORD lastRequestTick = 0;
    const DWORD now = GetTickCount();
    if (lastRequestTick != 0 && now - lastRequestTick < 1500)
        return 0;

    lastRequestTick = now;
    CMsgStreamBuffer buf(0x400B);
    SendMsg(buf);
    CGEffSoundBody::get()->PlaySound(L"snd_quest");
    return 0;
}

void CIFAutoEquipGuide::OnCIFReady()
{
    CIFDecoratedStatic::OnCIFReady();
    sub_633990();
}

void CIFAutoEquipGuide::OnUpdate()
{
    if (!g_pCGInterface || !g_pCGInterface->GetAlarmManager())
        return;

    const bool eligible = m_Settings->ShowGuideAutoEquip &&
                          (m_Settings->AutoEquipMaxLevel <= 0 ||
                           !g_pMyPlayerObj ||
                           g_pMyPlayerObj->GetCurrentLevel() <= m_Settings->AutoEquipMaxLevel);
    if (IsVisible() != eligible)
        g_pCGInterface->GetAlarmManager()->PrepareGuides();
}
