#pragma once

#include "IFWnd.h"

#include "IFEdit.h"

/// \note If u are planing to change this key chackout "CIFConsole::SetVisibleMode"
#define CHEAT_CONSOLE_TOGGLE_KEY    '~' // VK_F15

enum eVisibleMode : DWORD {
    VISIBLE_MODE_INVISIBLE,
    
    VISIBLE_MODE_OPENING, 
    VISIBLE_MODE_CLOSING,

    VISIBLE_MODE_VISIBLE
};

class CIFConsole : public CIFWnd
{
public:
private:
    char pad_036C[4]; //0x036C
    CIFEdit *m_pInputBox; //0x0370
    char pad_0374[0x03B8 - 0x0374]; //0x0374
    eVisibleMode m_CurVisibleMode; //0x03B8
    char pad_03BC[0x05A4 - 0x03BC]; //0x03BC
public:
    /// \address 00537bc0
    BOOL IsConsolePermissioned();

    /// \address 00537c60
    void SetVisibleMode(DWORD dwMode);

    /// \address 00537ce0
    BOOL SetFocusOnInputBox();

private:
    BEGIN_FIXTURE()
        ENSURE_SIZE(0x05A4)
        ENSURE_OFFSET(m_pInputBox, 0x0370)
        ENSURE_OFFSET(m_CurVisibleMode, 0x03B8)
    END_FIXTURE()

    RUN_FIXTURE(CIFConsole)
};
