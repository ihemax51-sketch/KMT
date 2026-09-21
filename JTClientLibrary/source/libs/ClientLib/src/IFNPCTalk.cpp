#include <CustomData/CustomDataManager.h>
#include <BSLib/Debug.h>
#include <Menu/IFMenu.h>
#include <ExtraUI/IFItemTranslationWnd.h>
#include <CustomData/CustomSettingManager.h>
#include "IFNPCTalk.h"
#include "GInterface.h"
#include "PartyData.h"
#include "CharacterDependentData.h"


void CIF_NPCTalk::OnListChatThingIMPL(int a1, int a2) {
    BS_DEBUG_LOW("%s(%d, %d)", __FUNCTION__, a1, a2);

    int id = GetCurrentEventMsgCtrlId();
    CIFListCtrl *pList;

    if (m_textBox != NULL && id == m_textBox->UniqueID()) {
        pList = m_textBox;
    }
    else {
        // Joymax was using this here ... why ... how ... ???
        assert(FALSE);
        reinterpret_cast<void(__thiscall *)(CIF_NPCTalk *, int, int)>(0x00703f10)(this, a1, a2);
        return;
    }

    CIFListCtrl::SLineOfText *line = pList->sub_63A940();
    if (line == NULL || line->m_font == NULL) {
        reinterpret_cast<void(__thiscall *)(CIF_NPCTalk *, int, int)>(0x00703f10)(this, a1, a2);
        return;
    }

    std::n_wstring str;
    line->m_font->GetText(&str);
    CIF_NPCWindow *pParent = (CIF_NPCWindow*)GetParentControl();
    if (pParent == NULL || !pParent->IsSame(GFX_RUNTIME_CLASS(CIF_NPCWindow))) {
        reinterpret_cast<void(__thiscall *)(CIF_NPCTalk *, int, int)>(0x00703f10)(this, a1, a2);
        return;
    }

    if(pParent->GetNpcObjID() == 46400)
    {
        if(line->Index == 0)
        {
            const SPartyData& partyData = g_CCharacterDependentData.GetPartyData();
            if(partyData.bIsPartyMaster && partyData.bInParty)
            {
                CMsgStreamBuffer buf(0x169A);
                buf << byte(25);
                buf << byte(0); /// TG
                SendMsg(buf);
                pParent->OnCloseWndIMPL();
            }
            else
            {
                g_pCGInterface->ShowMessage_Warning(KmtGetText(L"UIIT_KMT_ONLY_PARTY_MASTER_CAN_BE_CALL_SHADOW_UNIQUES"));
            }
        }
        else
        {
            reinterpret_cast<void (__thiscall *)(CIF_NPCTalk *, int, int)>(0x00703f10)(this, a1, a2);
        }
    }
    else if(pParent->GetNpcObjID() == 46401)
    {
        if(line->Index == 0)
        {
            const SPartyData& partyData = g_CCharacterDependentData.GetPartyData();
            if(partyData.bIsPartyMaster && partyData.bInParty)
            {
                CMsgStreamBuffer buf(0x169A);
                buf << byte(25);
                buf << byte(1); /// CERB
                SendMsg(buf);
                pParent->OnCloseWndIMPL();
            }
            else
            {
                g_pCGInterface->ShowMessage_Warning(KmtGetText(L"UIIT_KMT_ONLY_PARTY_MASTER_CAN_BE_CALL_SHADOW_UNIQUES"));
            }
        }
        else
        {
            reinterpret_cast<void (__thiscall *)(CIF_NPCTalk *, int, int)>(0x00703f10)(this, a1, a2);
        }
    }
    else if(pParent->GetNpcObjID() == 46402)
    {
        if(line->Index == 0)
        {
            const SPartyData& partyData = g_CCharacterDependentData.GetPartyData();
            if(partyData.bIsPartyMaster && partyData.bInParty)
            {
                CMsgStreamBuffer buf(0x169A);
                buf << byte(25);
                buf << byte(2); /// IVY
                SendMsg(buf);
                pParent->OnCloseWndIMPL();
            }
            else
            {
                g_pCGInterface->ShowMessage_Warning(KmtGetText(L"UIIT_KMT_ONLY_PARTY_MASTER_CAN_BE_CALL_SHADOW_UNIQUES"));
            }
        }
        else
        {
            reinterpret_cast<void (__thiscall *)(CIF_NPCTalk *, int, int)>(0x00703f10)(this, a1, a2);
        }
    }
    else if(pParent->GetNpcObjID() == 46403)
    {
        if(line->Index == 0)
        {
            const SPartyData& partyData = g_CCharacterDependentData.GetPartyData();
            if(partyData.bIsPartyMaster && partyData.bInParty)
            {
                CMsgStreamBuffer buf(0x169A);
                buf << byte(25);
                buf << byte(3); /// uruchı
                SendMsg(buf);
                pParent->OnCloseWndIMPL();
            }
            else
            {
                g_pCGInterface->ShowMessage_Warning(KmtGetText(L"UIIT_KMT_ONLY_PARTY_MASTER_CAN_BE_CALL_SHADOW_UNIQUES"));
            }
        }
        else
        {
            reinterpret_cast<void (__thiscall *)(CIF_NPCTalk *, int, int)>(0x00703f10)(this, a1, a2);
        }
    }
    else if(pParent->GetNpcObjID() == 46404)
    {
        if(line->Index == 0)
        {
            const SPartyData& partyData = g_CCharacterDependentData.GetPartyData();
            if(partyData.bIsPartyMaster && partyData.bInParty)
            {
                CMsgStreamBuffer buf(0x169A);
                buf << byte(25);
                buf << byte(4); /// ISYU
                SendMsg(buf);
                pParent->OnCloseWndIMPL();
            }
            else
            {
                g_pCGInterface->ShowMessage_Warning(KmtGetText(L"UIIT_KMT_ONLY_PARTY_MASTER_CAN_BE_CALL_SHADOW_UNIQUES"));
            }
        }
        else
        {
            reinterpret_cast<void (__thiscall *)(CIF_NPCTalk *, int, int)>(0x00703f10)(this, a1, a2);
        }
    }
    else
    {
        reinterpret_cast<void (__thiscall *)(CIF_NPCTalk *, int, int)>(0x00703f10)(this, a1, a2);
    }
}

void CIF_NPCTalk::FUN_006fcd60(int p1, int p2, int p3)
{
    //printf("%d %d %d \n", p1, p2, p3);
    if(this->GetParentControl() != NULL && this->GetParentControl()->IsSame(GFX_RUNTIME_CLASS(CIF_NPCWindow)))
    {
        CIF_NPCWindow *pParent = (CIF_NPCWindow*)GetParentControl();
        if(pParent->GetNpcObjID() == 46400)
        {
            if (p1 == 1)
            {
                p1 = 2;
                std::n_wstring strmsg2 = KmtGetText(L"UIIT_KMT_1_CALL_SHADOW_TIGERWOMAN");
                m_textBox->sub_64F8A0(strmsg2, 0, -1058140, -30208, -30208, 0, 1);
                pParent->ShowGWndIMPL(true);
                this->ShowGWnd(true);
            }

           // std::n_wstring strmsg = L"2. End conversation.";
            //m_textBox->sub_64F8A0(strmsg, 1, -1058140, -30208, -30208, 0, 1);

          //  return;
        }
        else if(pParent->GetNpcObjID() == 46401)
        {
            if (p1 == 1)
            {
                p1 = 2;
                std::n_wstring strmsg2 = KmtGetText(L"UIIT_KMT_1_CALL_SHADOW_CERBERUS");
                m_textBox->sub_64F8A0(strmsg2, 0, -1058140, -30208, -30208, 0, 1);
                pParent->ShowGWndIMPL(true);
                this->ShowGWnd(true);
            }

            // std::n_wstring strmsg = L"2. End conversation.";
            //m_textBox->sub_64F8A0(strmsg, 1, -1058140, -30208, -30208, 0, 1);

            //  return;
        }
        else if(pParent->GetNpcObjID() == 46402)
        {
            if (p1 == 1)
            {
                p1 = 2;
                std::n_wstring strmsg2 = KmtGetText(L"UIIT_KMT_1_CALL_SHADOW_CAPTAIN_IVY");
                m_textBox->sub_64F8A0(strmsg2, 0, -1058140, -30208, -30208, 0, 1);
                pParent->ShowGWndIMPL(true);
                this->ShowGWnd(true);
            }

            // std::n_wstring strmsg = L"2. End conversation.";
            //m_textBox->sub_64F8A0(strmsg, 1, -1058140, -30208, -30208, 0, 1);

            //  return;
        }
        else if(pParent->GetNpcObjID() == 46403)
        {
            if (p1 == 1)
            {
                p1 = 2;
                std::n_wstring strmsg2 = KmtGetText(L"UIIT_KMT_1_CALL_SHADOW_URUCHI");
                m_textBox->sub_64F8A0(strmsg2, 0, -1058140, -30208, -30208, 0, 1);
                pParent->ShowGWndIMPL(true);
                this->ShowGWnd(true);
            }

            // std::n_wstring strmsg = L"2. End conversation.";
            //m_textBox->sub_64F8A0(strmsg, 1, -1058140, -30208, -30208, 0, 1);

            //  return;
        }
        else if(pParent->GetNpcObjID() == 46404)
        {
            if (p1 == 1)
            {
                p1 = 2;
                std::n_wstring strmsg2 = KmtGetText(L"UIIT_KMT_1_CALL_SHADOW_ISYUTARU");
                m_textBox->sub_64F8A0(strmsg2, 0, -1058140, -30208, -30208, 0, 1);
                pParent->ShowGWndIMPL(true);
                this->ShowGWnd(true);
            }

            // std::n_wstring strmsg = L"2. End conversation.";
            //m_textBox->sub_64F8A0(strmsg, 1, -1058140, -30208, -30208, 0, 1);

            //  return;
        }
    }
    reinterpret_cast<void (__thiscall *)(CIF_NPCTalk *, int, int, int)>(0x006fcd60)(this, p1, p2, p3);
}

void CIF_NPCTalk::WriteNpcInfo_Maybe()
{

    reinterpret_cast<void (__thiscall *)(CIF_NPCTalk *)>(0x00700440)(this);
}
