#include "IFChatModule.h"

void CIFChatModule::WriteChatMessage(LPCWSTR lpszMsg, D3DCOLOR colorText) {
    CIFTextBox *pChatBox = m_IRM.GetResObj<CIFTextBox>(GDR_CHAT_CHATMESSAGE, true);
    std::n_wstring wstrText = lpszMsg;

    pChatBox->sub_64F8A0(wstrText, 0, colorText, colorText, colorText, 0, 1);
    m_pVScroll->FUN_00659150((m_dwHistoryLines - 5));
}