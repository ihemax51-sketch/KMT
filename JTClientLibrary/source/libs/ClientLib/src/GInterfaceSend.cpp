////////////////////////////////////////////////////////////////////////////////////
//
// GInterfaceSend.cpp: implementation of the CGInterface network msg send functions.
//
////////////////////////////////////////////////////////////////////////////////////

#include "GInterface.h"

#include <BSLib/multibyte.h>
#include <GlobalItemLinking/GlobalItemLinking.h>

#include "support/MemberFunctionHook.h"
void StringReplaceAll2(std::n_wstring &Value, const std::wstring &From, const std::wstring &To) {
    if (From.empty())
        return;
    size_t start_pos = 0;
    while ((start_pos = Value.find(From.c_str(), start_pos)) != std::wstring::npos) {
        Value.replace(start_pos, From.length(), To.c_str());
        start_pos += To.length();// Increase the same length in case 'To' contPains 'From', like replacing 'x' with 'yx'
    }
}
HOOK_ORIGINAL_MEMBER(0x007901c0, &CGInterface::WriteGlobalMessage)
void CGInterface::WriteGlobalMessage(unsigned char nSlot, std::n_wstring message) {
    CIFMainPopup *popup = GetMainPopup();
    CIFInventory *inventory = popup->GetInventory();

    CSOItem *item = inventory->GetItemBySlot(nSlot);

    if (item->m_blValid == 0) {
        return;
    }

    const SItemData *pItemData = item->GetItemData();
    if (pItemData->IsGlobalMessageScroll()) {
        /*    NEWMSG(0x704c)

            pReq << static_cast<unsigned char>(nSlot + 13u) << pItemData->m_typeId << message;

        SENDMSG()*/

        if (!g_global->m_CurrentLinkedItemName.empty() && g_global->m_LinkedGlobalItemEnd != 0) {

            if (message.find(g_global->m_CurrentLinkedItemName.c_str()) != std::n_wstring::npos) {
                StringReplaceAll2(message, g_global->m_CurrentLinkedItemName, L"{wpname}");

            }

            NEWMSG(0x705C)

                //pReq << testaa << testaa2 << pItemData->m_typeId << pItemData->RefObjectId << message;
                pReq << (BYTE)1 << static_cast<unsigned char>(nSlot + 13u) << pItemData->m_typeId << pItemData->RefObjectId << message << g_global->m_LinkedGlobalSlot;
                g_global->m_LinkedGlobalItemEnd = 0;
                g_global->m_CurrentLinkedItemName = L"";
                g_global->m_LinkedGlobalSlot = 0;
            SENDMSG()
        }
        else {

            NEWMSG(0x705C)
                pReq << (BYTE)0 << static_cast<unsigned char>(nSlot + 13u) << pItemData->m_typeId << pItemData->RefObjectId << message;
            SENDMSG()
        }
    }
}

void CGInterface::RequestStatIncrement(undefined4 a1, undefined4 a2, undefined4 a3, undefined4 a4) {
    if (a1 == 0x4b) {
        if (a2 == 1) {
            NEWMSG(0x7050)
            SENDMSG()
        } else if (a2 == 2) {
            NEWMSG(0x7051)
            SENDMSG()
        }
    }
}
