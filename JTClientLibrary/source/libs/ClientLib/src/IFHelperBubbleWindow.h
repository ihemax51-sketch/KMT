#pragma once

#include "IFStretchWnd.h"
#include "IFTextBox.h"
#include "IFSlotWithHelp.h"
#include "IFStatic.h"

class CIFHelperBubbleWindow : public CIFStretchWnd
{
public:
    void Reset();
    void sub_777020();
    void Write2(std::wstring &text, int a2, UINT ForegroundColor, int a4, BYTE FontIndex);
    int sub_776C00(wnd_rect* a2, int a3);
    void sub_818690(int a2, int a3);
    void sub_8189F0(float* a2);
    void WriteLine(std::wstring& text, int a2, UINT ForegroundColor, int a4, BYTE FontIndex);
    void Write(std::wstring &text, int a2, UINT ForegroundColor, int a4, BYTE FontIndex);
public:
    CIFTextBox* m_textBox; //0x07C4
    char pad_0x07C8[0x4]; //0x07C8
    __int32 m_width; //0x07CC
    __int32 m_heigth; //0x07D0
    char pad_0x07D4[0x8]; //0x07D4
    CIFSlotWithHelp* m_ownerWindow; //0x07DC
    CIFStatic* m_circleIcon; //0x07E0
    CIFStatic* N00006405; //0x07E4
    CIFStatic* m_squareIcon; //0x07E8
    char pad_0x07EC[0x4]; //0x07EC


};

//Size=0x07F0