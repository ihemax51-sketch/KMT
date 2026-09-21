#include "IFHelperBubbleWindow.h"
#include "TextStringManager.h"
#include <BSLib/multibyte.h>
#include <iostream>
#include <Windows.h>
#include <sstream>
#include <CustomData/CustomSettingManager.h>


void CIFHelperBubbleWindow::Reset() {
    //??????????
    reinterpret_cast<void(__thiscall*)(CIFHelperBubbleWindow*)>(0x776A00)(this);
}
void CIFHelperBubbleWindow::sub_777020() {
    reinterpret_cast<void(__thiscall*)(CIFHelperBubbleWindow*)>(0x777020)(this);
}
void CIFHelperBubbleWindow::Write2(std::wstring &text, int a2, UINT ForegroundColor, int a4, BYTE FontIndex) {
    // Redirection to original client code
    //reinterpret_cast<void(__thiscall *)(CIFHelperBubbleWindow *, std::wstring *, int, UINT, int, BYTE)>(0x00776D20)(this, &text, a2, ForegroundColor, a4, FontIndex);
}
void CIFHelperBubbleWindow::Write(std::wstring& text, int a2, UINT ForegroundColor, int a4, BYTE FontIndex)
{
    // Redirection to original client code
    reinterpret_cast<void(__thiscall*)(CIFHelperBubbleWindow*, std::wstring*, int, UINT, int, BYTE)>(0x00776D20)(this, &text, a2, ForegroundColor, a4, FontIndex);
}
void CIFHelperBubbleWindow::WriteLine(std::wstring& text, int a2, UINT ForegroundColor, int a4, BYTE FontIndex) {

    if(m_Settings->InsertCommaToPrices)
    {
        if (text.size() >= 4) {
            std::wstring unit01 = text.substr(text.size() - 4);

            if (unit01 == TSM_GETTEXTPTR(L"UIIT_STT_GOLD") || unit01 == TSM_GETTEXTPTR(L"UIIT_STT_UNIT")) {
                std::wstring amountNumberText = text.substr(0, text.find(unit01)).substr(text.find(L": ") + 2);
                std::wstring unitName = text.substr(0, text.find(amountNumberText));

                std::stringstream buffer(acp_encode(amountNumberText));
                long unitAmount;
                buffer >> unitAmount;

                std::wstring commafiedAmount;
                int cnt = 0;
                do {
                    commafiedAmount.insert(0, 1, char('0' + unitAmount % 10));
                    unitAmount /= 10;
                    if (++cnt == 3 && unitAmount) {
                        commafiedAmount.insert(0, 1, ',');
                        cnt = 0;
                    }
                } while (unitAmount);

                if (unitName == L"Price : " || unitName == L"Price") {
                    ForegroundColor = 0xFFFFFF;
                    text = unitName + L" " + commafiedAmount + L" " + unit01;
                } else if (unitName == L"Copper Coin : ") {
                    ForegroundColor = 0xFFFFFF;
                    text = unitName + L" " + commafiedAmount + L" " + unit01;
                } else if (unitName == L"Iron Coin : ") {
                    ForegroundColor = 0xFFFFFF;
                    text = unitName + L" " + commafiedAmount + L" " + unit01;
                } else if (unitName == L"Silver Coin : ") {
                    ForegroundColor = 0xFFFFFF;
                    text = unitName + L" " + commafiedAmount + L" " + unit01;
                } else if (unitName == L"Gold Coin : ") {
                    ForegroundColor = 0xFFFF0099;
                    text = unitName + L" " + commafiedAmount + L" " + unit01;
                } else if (unitName == L"Arena Coin : ") {
                    ForegroundColor = 0xFFFFFF;
                    text = unitName + L" " + commafiedAmount + L" " + unit01;
                } else if (unitName == L"Silk : ") {
                    ForegroundColor = 0xFFFFFF;
                    text = unitName + L" " + commafiedAmount;
                } else if (unitName == L"Diamonds : ") {
                    ForegroundColor = 0xFFFFFF;
                    text = unitName + L" " + commafiedAmount;
                } else if (unitName == L"Job Points : ") {
                    ForegroundColor = 0xFFFFFF;
                    text = unitName + L" " + commafiedAmount;
                } else if (unitName == L"Point : ") {
                    ForegroundColor = 0xFFFFFF;
                    text = unitName + L" " + commafiedAmount;
                } else if (unitName == L"Honor Point : ") {
                    ForegroundColor = 0xFF006666;
                    text = unitName + L" " + commafiedAmount;
                }
            } else if (text.find(TSM_GETTEXTPTR(L"UIIT_STT_PRICE")) == 0) {
                std::wstring priceTag = TSM_GETTEXTPTR(L"UIIT_STT_PRICE");
                std::wstring amountNumberText = text.substr(priceTag.length() + 1); // "Price" kelimesinden sonrasını al

                std::stringstream buffer(acp_encode(amountNumberText));
                long unitAmount;
                buffer >> unitAmount;

                std::wstring commafiedAmount;
                int cnt = 0;
                do {
                    commafiedAmount.insert(0, 1, char('0' + unitAmount % 10));
                    unitAmount /= 10;
                    if (++cnt == 3 && unitAmount) {
                        commafiedAmount.insert(0, 1, ',');
                        cnt = 0;
                    }
                } while (unitAmount);

                ForegroundColor = 0xFFFFFF;
                text = priceTag + L" " + commafiedAmount;
            }
        }
    }

    reinterpret_cast<void(__thiscall*)(CIFHelperBubbleWindow*, std::wstring*, int, UINT, int, BYTE)>(0x00776D20)(this, &text, a2, ForegroundColor, a4, FontIndex);
}
int CIFHelperBubbleWindow::sub_776C00(wnd_rect* a2,int a3) {
    //char* v5; // ecx
    //int v6; // ebx
    //int v7; // esi
    //int v8; // ebx
    //int v9; // ebp
    //int v10; // esi
    //int v11; // kr04_4
    //float v12; // edx
    //int result; // eax
    //float v14; // [esp+Ch] [ebp-Ch] BYREF
    //float v15; // [esp+10h] [ebp-8h]
    //int v16; // [esp+14h] [ebp-4h]
    //this->TB_Func_9(0xC4);
    //if (a3 == 1) {
    //	v6 = *a2;
    //	v7 = a2[1] - 0x28;
    //}
    //else {
    //	v8= m_width;
    //	v9 = a2[2];
    //	v10 = *a2;
    //	if (v8 + v9 + v10 + 20 <= CGame::GetClientDimensionStuff().width)
    //		v6 = v10 + v9 + 8;
    //	else
    //		v6 = v10 - v8 - 8;
    //	v7 = a2[1] + 8;
    //	if (v7 + m_heigth >= CGame::GetClientDimensionStuff().height - 15)
    //		v7 = CGame::GetClientDimensionStuff().height - m_heigth - 15;
    //}
    //sub_818690(v6,v7);
    //v11 = m_heigth;
    //v14 = (float)(v6 + m_width / 2);
    //v15 = (float)(v7 + v11 / 2);
    //*(float*)&v16 = 1.0;
    //sub_8189F0(&v14);
    //v12 = v15;
    //result = v16;
    //*((float*)this + 219) = v14;
    //*((float*)this + 220) = v12;
    //*((DWORD*)this + 221) = result;
    //return result;
    return reinterpret_cast<int(__thiscall*)(CIFHelperBubbleWindow*, wnd_rect*,int)>(0x776C00)(this, a2,a3);
}
void CIFHelperBubbleWindow::sub_818690(int a2, int a3) {

    reinterpret_cast<void(__thiscall*)(CIFHelperBubbleWindow*, int, int)>(0x818690)(this, a2, a3);
}
void CIFHelperBubbleWindow::sub_8189F0(float* a2) {

    reinterpret_cast<void(__thiscall*)(CIFHelperBubbleWindow*, float*)>(0x8189F0)(this, a2);
}
