//
// Created by Kurama on 2/1/2023.
//

#include "NIFTextBoard.h"
#include "GNFontChar.h"

#include "GFX3DFunction/RStateMgr.h"
#include <GFX3DFunction/RTLoading.h>

#include <cmath>

void CNIFTextBoard::FontTextureRender() {}

void CNIFTextBoard::OnAniFadeRelease() {}

BYTE CNIFTextBoard::GetAlphaScale() { return m_btCurrentTextureAlpha; }

void CNIFTextBoard::TextureRender() {}

CNIFTextBoard::CNIFTextBoard() : m_NFontTexture() {
    m_fAniFadeCurrentTime = 0.0f;
    m_btCurrentTextureAlpha = 255; // D3DCOLOR_RGBA(0, 0, 0, 255)
    m_fAniFadeTime = 0.0f;

    m_dwTextureColor = D3DCOLOR_RGBA(0, 0, 0, 0);

    void *pFontData = g_sNFontDataVec[0];
    m_pFontTextData = pFontData;
    m_NFontTexture.m_pFontData = pFontData;
    m_dwUnknwonColor = D3DCOLOR_RGBA(255, 255, 255, 255);
    m_dwBG_FontColor = D3DCOLOR_RGBA(0, 0, 0, 0);
    InstantiateFontText(L"");

    m_nTextureHAlignType = HALIGN_CENTER;
    NTB_Func_3();
    m_nTextureVAlignType = VALIGN_CENTER;
    NTB_Func_3();

    m_pRenderTexture = NULL;
    m_dwReleaseTexture = FALSE;
    m_pHoverTexture3D = NULL;
    m_pBgTexture3D = NULL;
    m_bRenderTexture = false;
}

CNIFTextBoard::~CNIFTextBoard() {
    if ((m_dwReleaseTexture & 1) == FALSE)
        CNIFTextBoard::NTB_Func_13("", 0, 0);

    SetHoverTexture("");
}

void CNIFTextBoard::OnAniFading(float fGameAniSpeedCorrection) {
    if (m_fAniFadeTime == 0.0f)
        return;

    if ((m_fAniFadeCurrentTime += fGameAniSpeedCorrection) >= 0.0f)
        UpdateAlphaFading();
    else
        m_btCurrentTextureAlpha = m_cAniFadeAlphaStart;

    // Fade Done
    if (m_fAniFadeCurrentTime >= m_fAniFadeTime) {
        m_fAniFadeCurrentTime = 0.0f;
        m_fAniFadeTime = 0.0f;
        OnAniFadeRelease();
    }
}

void CNIFTextBoard::OnRender() {
    EnableTextureAlphaOperations(m_dwTextureColor);

    if (m_bRenderTexture) {
        if (m_dwTextureColor != D3DCOLOR_RGBA(0, 0, 0, 0)) {
            g_RStateMgr.SetStageOperator<0, D3DTSS_COLOROP>(D3DTOP_MODULATE);
            g_RStateMgr.SetStageOperator<0, D3DTSS_COLORARG1>(D3DTA_TEXTURE);
            g_RStateMgr.SetStageOperator<0, D3DTSS_COLORARG2>(D3DTA_TFACTOR);
        }
        TextureRender();
        if (m_dwTextureColor != D3DCOLOR_RGBA(0, 0, 0, 0)) {
            // Back to default
            g_RStateMgr.SetStageOperator<0, D3DTSS_COLOROP>(D3DTOP_SELECTARG1);
            g_RStateMgr.SetStageOperator<0, D3DTSS_COLORARG1>(D3DTA_TEXTURE);
        }
    }

    if (!m_NFontTexture.m_listTextChar.empty())
        FontTextureRender();

    DefaultTextureAlphaOperations();
}

void CNIFTextBoard::SetFontData(void *pData) {
    m_pFontTextData = pData;
    m_NFontTexture.m_pFontData = pData;
}

// JMX developer was high or what? the function already getting call at the constructor of CNIFWnd.
void CNIFTextBoard::InstantiateFontText(const wchar_t *pText) {
    if (pText == NULL) {
        m_wstrFontTexture.clear();
        m_NFontTexture.EraseTextLetters();
        return;
    }
    m_wstrFontTexture = (pText);
    m_NFontTexture.SortTextLetters(m_wstrFontTexture.c_str());
}

void CNIFTextBoard::SetTextureBGColor(DWORD dwColor) {
    m_dwBG_FontColor = dwColor;
    m_NFontTexture.m_dwBGColor = dwColor;
}

void CNIFTextBoard::SetAniFadeData(BYTE btAlphaStart, BYTE btAlphaMax, float fTime, float fStartupTime) {
    m_cAniFadeAlphaStart = btAlphaStart;
    m_fAniFadeTime = fTime;
    m_cAniFadeAlphaMax = btAlphaMax;
    m_btCurrentTextureAlpha = btAlphaStart;
    m_fAniFadeCurrentTime = fStartupTime;
}

void CNIFTextBoard::UpdateAlphaFading() {
    // If our time is done!
    if (m_fAniFadeCurrentTime >= m_fAniFadeTime) {
        m_btCurrentTextureAlpha = m_cAniFadeAlphaMax;
        return;
    }

    float fRemaindTime = fmod(m_fAniFadeCurrentTime, m_fAniFadeTime);
    m_btCurrentTextureAlpha = ((m_cAniFadeAlphaStart * ((BYTE) (1.0f - fRemaindTime))) +
                               (m_cAniFadeAlphaMax * ((BYTE) fRemaindTime)));
}

void CNIFTextBoard::AnimateTextureFade(float fGameAniSpeedCorrection) {
    if ((m_fAniFadeCurrentTime += fGameAniSpeedCorrection) <= 0.0f)
        m_btCurrentTextureAlpha = m_cAniFadeAlphaStart;

    UpdateAlphaFading();
}

void CNIFTextBoard::EnableTextureAlphaOperations(DWORD dwColorValue) {
    BYTE m_btAlpha = this->GetAlphaScale();

    g_RStateMgr.SetStageOperator<0, D3DTSS_ALPHAOP>(D3DTOP_MODULATE);
    g_RStateMgr.SetStageOperator<0, D3DTSS_ALPHAARG1>(D3DTA_TFACTOR);
    g_RStateMgr.SetStageOperator<0, D3DTSS_ALPHAARG2>(D3DTA_TEXTURE);

    g_RStateMgr.FUN_00471360(0x3c,
            // Replace the dwColorValue alpha with ours
                             ((int) ((((m_btAlpha) & 0xff) << 24) | dwColorValue)));
}

void CNIFTextBoard::DefaultTextureAlphaOperations() {
    g_RStateMgr.SetStageOperator<0, D3DTSS_ALPHAOP>(D3DTOP_SELECTARG1);
    g_RStateMgr.SetStageOperator<0, D3DTSS_ALPHAARG1>(D3DTA_TEXTURE);
}

void CNIFTextBoard::NTB_Func_12(const char *str, int a3, int a4) {
    NTB_Func_13(str, a3, a4);
}

void CNIFTextBoard::NTB_Func_13(std::n_string str, int a3, int a4) {
    if (m_strBGroundTexturePath.empty() && m_pRenderTexture != NULL)
        m_pRenderTexture->Release();

    if (m_pRenderTexture != NULL)
        Fun_CacheTexture_Release(&m_strBGroundTexturePath);

    if (str.empty()) {
        m_strBGroundTexturePath.clear();
        m_pRenderTexture = NULL;
        m_bRenderTexture = false;
        return;
    }

    IDirect3DBaseTexture9 *pTexture = Fun_CacheTexture_Create(str);
    m_pRenderTexture = pTexture;
    m_pBgTexture3D = pTexture;
    m_strBGroundTexturePath = str;
    m_bRenderTexture = true;
}

void CNIFTextBoard::SetHoverTexture(const std::n_string &strPath) {
    if (m_pHoverTexture3D != NULL)
        Fun_CacheTexture_Release(&m_strFocusTexturePath);

    if (!strPath.empty()) {
        m_pHoverTexture3D = Fun_CacheTexture_Create(strPath);
        m_strFocusTexturePath = strPath;
        return;
    }

    m_pHoverTexture3D = NULL;
}

void CNIFTextBoard::SetBaseTexture(std::n_string str, int a3, int a4) {
    if (m_strBGroundTexturePath.empty() && m_pRenderTexture != NULL)
        m_pRenderTexture->Release();

    if (m_pRenderTexture != NULL)
        Fun_CacheTexture_Release(&m_strBGroundTexturePath);

    if (str.empty()) {
        m_strBGroundTexturePath.clear();
        m_pRenderTexture = NULL;
        m_bRenderTexture = false;
        return;
    }

    IDirect3DBaseTexture9 *pTexture = Fun_CacheTexture_Create(str);
    m_pRenderTexture = pTexture;
    // m_pBgTexture3D = pTexture;
    m_strBGroundTexturePath = str;
    m_bRenderTexture = true;
}

void CNIFTextBoard::SetBaseTexture(IDirect3DBaseTexture9 *pNewTexture) {
    if (m_strBGroundTexturePath.empty() && m_pRenderTexture != NULL)
        m_pRenderTexture->Release();

    m_strBGroundTexturePath.clear();
    m_pRenderTexture = pNewTexture;
    m_bRenderTexture = true;
}

void CNIFTextBoard::ClearBaseTexture() {
    m_strBGroundTexturePath.clear();
    m_pRenderTexture = NULL;
    m_bRenderTexture = false;
}

void CNIFTextBoard::SetReleaseTextureValue(DWORD dwValue) {
    m_dwReleaseTexture = dwValue;
}

DWORD CNIFTextBoard::SetReleaseTextureMask(DWORD dwValue) {
    return (m_dwReleaseTexture |= dwValue);
}

void CNIFTextBoard::ClearReleaseTextureValue() {
    m_dwReleaseTexture = 0;
}

void CNIFTextBoard::SetAlphaScale(BYTE btValue) {
    m_btCurrentTextureAlpha = btValue;
}

void CNIFTextBoard::NTB_Func_3() {}

void CNIFTextBoard::SetHAlign(ENUM_HALIGN_TYPE eType) {
    m_nTextureHAlignType = eType;
    NTB_Func_3();
}

void CNIFTextBoard::SetVAlign(ENUM_VALIGN_TYPE eType) {
    m_nTextureVAlignType = eType;
    NTB_Func_3();
}

void CNIFTextBoard::NTB_Func_10() {}
