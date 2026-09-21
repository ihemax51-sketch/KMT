#pragma once

#include "IFWnd.h"

#include "IFChatOptionBoard.h"
#include "IFTextBox.h"
#include "IFVerticalScroll.h"

enum eLogType : BYTE {
    SYSLOG_NONE,

    SYSLOG_DEBUG = 0,

    SYSLOG_GAIN,
    SYSLOG_COMBAT,
    SYSLOG_STATUS,
    SYSLOG_PARTY,

    SYSLOG_ERR = 4,

    SYSLOG_GAME,
    SYSLOG_GUIDE,

    SYSLOG_7_UNK,
};

class CIFSystemMessage : public CIFWnd {
GFX_DECLARE_DYNAMIC_EXISTING(CIFSystemMessage, 0x00eec268)
public:
    bool OnCreateIMPL(long ln);
    void OnUpdateIMPL();
    /// \address 007B89E0
    CIFTextBox *GetTextBox();
    /// \address 007B89E0
    void WriteMessage(int a1, D3DCOLOR color, const wchar_t *msg, int a2, int a3);

    /// \address 007b8b40
    void WriteErrorMessage(BYTE btType, WORD wErrorCode, DWORD dwColor, int a5, int a6);

public:
    /// \address 007b85f0
    bool IsLogAble(BYTE btLogType);

private:
    char pad_036c[20]; //0x036C
    CIFChatOptionBoard *m_pOptionBoard; //0x0380
    CIFTextBox* m_TextBox;
    CIFVerticalScroll* m_Scroll;
private:
BEGIN_FIXTURE()
        ENSURE_SIZE(0x038C)
        ENSURE_OFFSET(m_pOptionBoard, 0x0380)
    END_FIXTURE()
    
    RUN_FIXTURE(CIFSystemMessage)
};
