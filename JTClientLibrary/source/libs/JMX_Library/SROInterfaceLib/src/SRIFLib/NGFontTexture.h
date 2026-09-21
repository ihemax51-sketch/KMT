//
// Created by Kurama on 2/1/2023.
//
#pragma once

#include "d3d9.h"

#include "BSLib/BSLib.h"

class CNGFontTexture {
    struct TextureDimensions {
        short width;
        short height;
    };
public:
    CNGFontTexture();

    virtual ~CNGFontTexture();

public:
    void SortTextLetters(const wchar_t *pText);

    void EraseTextLetters();

    void SetColor(DWORD dwColor);

public:
    void *m_pFontData; //0x0004
private:
    char pad_0008[4]; //0x0008
public:
    D3DCOLOR m_dwBGColor; //0x000C
private:
    D3DCOLOR m_dwFGColor; //0x0010
    char pad_0014[0x30]; //0x0014
public:
    std::n_list<std::pair<wchar_t /*Character Key*/, void * /*CNGFontChar*/>> m_listTextChar; //0x0044
private:
    TextureDimensions m_BGroundDimensions; //0x0050
    char pad_0054[0x18]; //0x0054
private:
BEGIN_FIXTURE()
        ENSURE_SIZE(0x006C)
        ENSURE_OFFSET(m_pFontData, 0x0004)
        ENSURE_OFFSET(m_listTextChar, 0x0044)
    END_FIXTURE()

    RUN_FIXTURE(CNGFontTexture)
};