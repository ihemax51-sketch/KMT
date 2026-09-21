//
// Created by Kurama on 2/1/2023.
//

#include "NGFontTexture.h"

CNGFontTexture::CNGFontTexture() {
    reinterpret_cast<void (__stdcall *)(CNGFontTexture *)>(0x004759d0)(this);
}

CNGFontTexture::~CNGFontTexture() {
    EraseTextLetters();
}

void CNGFontTexture::SortTextLetters(const wchar_t *pText) {
    /// \Why
    /// the func haz __usercall, we must use a wrapper and wrapper = asm.
#if 0
    reinterpret_cast<void (__thiscall *)(CNGFontTexture *, const wchar_t *)>(0x00476350)(this, pText);
#endif // if 0
}

void CNGFontTexture::EraseTextLetters() {
    m_BGroundDimensions.width = 0;
    m_BGroundDimensions.height = 0;

    // Clear all
    m_listTextChar.clear();
}

void CNGFontTexture::SetColor(DWORD dwColor) {
    reinterpret_cast<void (__thiscall *)(CNGFontTexture *, DWORD)>(0x00475ce0)(this, dwColor);
}
