
#include <Data/ItemData.h>
#include <GlobalDataManager.h>
#include <TextStringManager.h>
#include <GInterface.h>
#include <CustomData/CustomDataManager.h>
#include <CharacterDependentData.h>
#include <ICPlayer.h>
#include <CustomData/CustomCICPlayer.h>
#include <BSLib/multibyte.h>
#include "IFMovePartyMemberSlot.h"




GFX_IMPLEMENT_DYNCREATE(CIFMovePartyMemberSlot, CIFWnd)
GFX_BEGIN_MESSAGE_MAP(CIFMovePartyMemberSlot, CIFWnd)
                    ONG_COMMAND(10, &TakeBtn)
GFX_END_MESSAGE_MAP()
CIFMovePartyMemberSlot::CIFMovePartyMemberSlot(void)
{
    CharName16 = std::n_wstring();
}
CIFMovePartyMemberSlot::~CIFMovePartyMemberSlot(void)
{
}
bool CIFMovePartyMemberSlot::OnCreate(long ln)
{
    // Populate inherited members
    m_IRM.LoadFromFile("clientlibrary\\resinfo\\ifmovepartymemberslot.txt");
    m_IRM.CreateInterfaceSection("Create", this);

    this->m_IRM.GetResObj<CIFButton>(10, 1)->SetText(KmtGetText(L"UIIT_KMT_MOVE"));
    this->m_IRM.GetResObj<CIFButton>(10, 1)->ShowGWnd(false);
    this->m_IRM.GetResObj<CIFStatic>(6, 1)->ShowGWnd(false);
    this->m_IRM.GetResObj<CIFStatic>(5, 1)->ShowGWnd(false);
    this->m_IRM.GetResObj<CIFStatic>(7, 1)->ShowGWnd(false);

    return true;
}
void CIFMovePartyMemberSlot::OnUpdate() {

}
void CIFMovePartyMemberSlot::SetName(const wchar_t * CharName, const wchar_t * Location, int ptmaster){
    this->m_IRM.GetResObj<CIFStatic>(5, 1)->SetText(CharName);
    this->m_IRM.GetResObj<CIFStatic>(6, 1)->SetText(Location);
    CharName16 = CharName;

    this->m_IRM.GetResObj<CIFButton>(10, 1)->ShowGWnd(true);
    this->m_IRM.GetResObj<CIFStatic>(6, 1)->ShowGWnd(true);
    this->m_IRM.GetResObj<CIFStatic>(5, 1)->ShowGWnd(true);
    if(ptmaster == 0)
    {
        this->m_IRM.GetResObj<CIFStatic>(7, 1)->ShowGWnd(true);
    }

}

void CIFMovePartyMemberSlot::TakeBtn()
{
    if(g_pMyPlayerObj->ScrollStateMaybe == 1 && (g_pMyPlayerObj->CHARACTER_STATUS == Dead || g_pMyPlayerObj->CHARACTER_STATUS == Stall))
    {
        g_pCGInterface->ShowMessage_Warning(KmtGetText(L"UIIT_KMT_CANNOT_CHOOSE_ANOTHER_TELEPORT_METHOD_BECAUSE_YOU_ARE_NOW_BEING_TELEPORTED"));
        g_pCGInterface->ShowLogMessage(0xFFDBC99B, KmtGetText(L"UIIT_KMT_CANNOT_CHOOSE_ANOTHER_TELEPORT_METHOD_BECAUSE_YOU_ARE_NOW_BEING_TELEPORTED"));
        return;
    }
    if(CharName16.size() > 0)
    {
        const SPartyData& partyData = g_CCharacterDependentData.GetPartyData();
        for (int i = 0; i < partyData.NumberOfMembers; ++i) {

            const SPartyMemberData& memberData = g_CCharacterDependentData.GetPartyMemberData(i);
            if (wcscmp(memberData.m_charactername.c_str(), CharName16.c_str()) == 0)
            {
                CMsgStreamBuffer buf(0x201F);
                buf << m_Player->ReverseSlot + 13;
                buf << (short)(memberData.region.r);
                buf << (int)(memberData.position.x);
                buf << (int)(memberData.position.y);
                buf << (int)(memberData.position.z);
                std::n_string Cn16 = TO_NSTRING(CharName16);
                buf << Cn16;

                SendMsg(buf);
                m_Player->ReverseSlot = 9999;
                g_pCGInterface->m_IRM.GetResObj(MovePartyMember, 1)->ShowGWnd(false);
            }
        }
    }
}
void CIFMovePartyMemberSlot::Clear()
{

}
