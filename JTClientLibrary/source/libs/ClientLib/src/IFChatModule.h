#pragma once

#include "IFFrame.h"

#include "IFVerticalScroll.h"
#include "IFEdit.h"
#include "IFTextBox.h"

class CIFChatModule : public CIFFrame {
private:
    enum {
        GDR_CHAT_CHATMESSAGE = 2,
        // ....
    };

public:
    /// \address 0065b370
    void WriteChatMessage(LPCWSTR lpszMsg, D3DCOLOR colorText);

private:
    char pad_07b0[4]; //0x07B0
    CIFVerticalScroll *m_pVScroll; //0x07B4
    CIFVerticalScroll *m_pUserVScroll; //0x07B8
    CIFEdit *m_pInputBox; //0x07BC  // CIFEInputChatBox
    CIFTextBox *m_pOthersChat; //0x07C0
    DWORD m_dwShownLines; //0x07C4
    char pad_07c8[4]; //0x07C8
    DWORD m_dwHistoryLines; //0x07CC
    char pad_07d0[4]; //0x07D0
private:
BEGIN_FIXTURE()
        ENSURE_SIZE(0x07D4)
    END_FIXTURE()

    RUN_FIXTURE(CIFChatModule)
};