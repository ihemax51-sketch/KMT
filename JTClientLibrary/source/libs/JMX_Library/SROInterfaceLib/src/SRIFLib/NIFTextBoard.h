//
// Created by Kurama on 2/1/2023.
//
#pragma once

#include "BSLib/BSLib.h"

#include "NGFontTexture.h"

#include <d3d9.h>

enum ENUM_HALIGN_TYPE : int {
    HALIGN_LEFT = 0,
    HALIGN_CENTER = 1,
    HALIGN_RIGHT = 2,
};

enum ENUM_VALIGN_TYPE : int {
    VALIGN_TOP = 0,
    VALIGN_CENTER = 1,
    VALIGN_DOWN = 2,
};

class CNIFTextBoard {
public:
    /// \addres 00470400
    CNIFTextBoard();

public:
    /// \addressDector 00470570
    /// \addressVirtual 00470550
    virtual ~CNIFTextBoard();

    /// \address 00470780
    virtual void OnAniFading(float fGameAniSpeedCorrection);

    /// \address 00470800
    virtual void OnRender();

    /// \address 0046c1b0
    virtual void NTB_Func_3();

    /// \address 004703c0
    virtual void FontTextureRender();

    /// \address 0046c1c0
    virtual void SetHAlign(ENUM_HALIGN_TYPE eType);

    /// \address 004703d0
    virtual void SetVAlign(ENUM_VALIGN_TYPE eType);

    /// \address 004703d0
    virtual void OnAniFadeRelease();

    ///\Alpha Alpha, is the Transparency
    /// \address 004703e0
    virtual BYTE GetAlphaScale();

    /// \address 00471080
    virtual void SetAlphaScale(BYTE btValue);

    /// \address 0046c200
    virtual void NTB_Func_10();

    /// \address 004703f0
    virtual void TextureRender();

    /// \address 00470bd0
    virtual void NTB_Func_12(const char *str, int a3, int a4);

    /// \address 00470ce0
    virtual void NTB_Func_13(std::n_string str, int a3, int a4);

    /// \address 00470ea0
    virtual void SetBaseTexture(std::n_string str, int a3, int a4);

    /// \address 00470f90
    virtual void SetBaseTexture(IDirect3DBaseTexture9 *pNewTexture);

    /// \address 00470ff0
    virtual void ClearBaseTexture();

public:
    /// \address 00470910
    void SetFontData(void *pData);

    /// \address 00470920
    void InstantiateFontText(const wchar_t *pText);

    /// \address 004709a0
    void SetTextureBGColor(DWORD dwColor);

    /// \address 004709d0
    void SetAniFadeData(BYTE btAlphaStart, BYTE btAlphaMax, float fTime, float fStartupTime);

private:
    /// \address 00470a00
    void UpdateAlphaFading();

    /// \address 00470a90
    void AnimateTextureFade(float fGameAniSpeedCorrection);

protected:
    /// \address 00470ad0
    void EnableTextureAlphaOperations(DWORD dwColorValue);

    /// \address 00470b80
    void DefaultTextureAlphaOperations();

public:
    /// \address 00470de0
    void SetHoverTexture(const std::n_string &strPath);

    /// \address 00471010
    void SetReleaseTextureValue(DWORD dwValue);

    /// \address 00471020
    DWORD SetReleaseTextureMask(DWORD dwValue);

    /// \address 00471040
    void ClearReleaseTextureValue();

private:
    ENUM_HALIGN_TYPE m_nTextureHAlignType; //0x0004
    ENUM_VALIGN_TYPE m_nTextureVAlignType; //0x0008
public:
    CNGFontTexture m_NFontTexture; //0x000C
private:
    std::n_wstring m_wstrFontTexture; //0x0078
private:
    D3DCOLOR m_dwUnknwonColor; //0x0094 im sure about thats its belong to font
    D3DCOLOR m_dwBG_FontColor; //0x0098
public:
    void *m_pFontTextData; //0x009C
private:
    D3DCOLOR m_dwTextureColor; //0x00A0 // m_pMainTexture3D color image
    char pad_00a4[8]; //0x00A4
    BYTE m_cAniFadeAlphaStart; //0x00AC // Count
protected:
    BYTE m_btCurrentTextureAlpha; //0x00AD
    BYTE m_cAniFadeAlphaMax; //0x00AE // Count
protected:
    float m_fAniFadeTime; //0x00B0
    float m_fAniFadeCurrentTime; //0x00B4
private:
    IDirect3DBaseTexture9 *m_pBgTexture3D; //0x00B8 // Background
    IDirect3DBaseTexture9 *m_pHoverTexture3D; //0x00BC
    std::n_string m_strFocusTexturePath; //0x00C0
protected:
    IDirect3DBaseTexture9 *m_pRenderTexture; //0x00DC // Current
private:
    std::n_string m_strBGroundTexturePath; //0x00E0 // Background
    DWORD m_dwReleaseTexture; //0x00FC Bit 1 mean release its from the game texture mgr
    bool m_bRenderTexture; //0x0100
private:
BEGIN_FIXTURE()
        ENSURE_SIZE(0x0104)
        ENSURE_OFFSET(m_nTextureHAlignType, 0x0004)
        ENSURE_OFFSET(m_nTextureVAlignType, 0x0008)
        ENSURE_OFFSET(m_NFontTexture, 0x000C)
        ENSURE_OFFSET(m_wstrFontTexture, 0x0078)
        ENSURE_OFFSET(m_dwUnknwonColor, 0x0094)
        ENSURE_OFFSET(m_dwBG_FontColor, 0x0098)
        ENSURE_OFFSET(m_pFontTextData, 0x009C)
        ENSURE_OFFSET(m_dwTextureColor, 0x00A0)
        ENSURE_OFFSET(m_cAniFadeAlphaStart, 0x00AC)
        ENSURE_OFFSET(m_btCurrentTextureAlpha, 0x00AD)
        ENSURE_OFFSET(m_cAniFadeAlphaMax, 0x00AE)
        ENSURE_OFFSET(m_fAniFadeTime, 0x00B0)
        ENSURE_OFFSET(m_fAniFadeCurrentTime, 0x00B4)
        ENSURE_OFFSET(m_pBgTexture3D, 0x00B8)
        ENSURE_OFFSET(m_pHoverTexture3D, 0x00BC)
        ENSURE_OFFSET(m_strFocusTexturePath, 0x00C0)
        ENSURE_OFFSET(m_pRenderTexture, 0x00DC)
        ENSURE_OFFSET(m_strBGroundTexturePath, 0x00E0)
        ENSURE_OFFSET(m_dwReleaseTexture, 0x00FC)
        ENSURE_OFFSET(m_bRenderTexture, 0x0100)
    END_FIXTURE()

    RUN_FIXTURE(CNIFTextBoard)
};