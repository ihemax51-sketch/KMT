//
// Created by Kurama on 2/1/2023.
//
#pragma once

#include "GFXMainFrame/GWnd.h"

#include "NIFTextBoard.h"

#include "d3d9.h"
#include "d3dx9math.h"

class CNIFWnd : public CGWnd, public CNIFTextBoard {
GFX_DECLARE_DYNAMIC_EXISTING(CNIFWnd, 0x0115dea0)

GFX_DECLARE_MESSAGE_MAP(CNIFWnd)

protected:
    enum StyleOptionShiftType : DWORD {
        // Shift
        INVISIBLE_S = 0x05,
        TOOLTIP_S = 0x07
    };

public:
    CNIFWnd();

    ~CNIFWnd();

public:
    /// \address 0046c2c0
    bool OnCreate(long ln) override;

    /// \address 0046c2d0
    bool OnRelease() override;

    /// \address 0046cd80
    void OnUpdate() override;

    /// \address 0046cda0
    void RenderMyself() override;

    /// \address 0046c8e0
    bool OnMouseEvent(Event3D *pEvent) override;

    /// \address 0046c850
    bool OnWndMessage(const Event3D *pEvent) override;

    /// \address 0046cee0
    void OnGfxMessage(Event3D *pEvent) override;

    /// \address 0046cd60
    void BringToFront() override;

    /// \address 0046cf70
    void SetGWndSize(int width, int height) override;

    /// \address 0046cf50
    void Func_22(int x, int y) override;

    /// \address 0046d760
    void ShowGWnd(bool bVisible) override;

    /// \address 0046ca00
    void OnGWndDimensionChange() override;

    /// \address 0046cfc0
    virtual void MoveGWnd(int x, int y);

    /// \address 0046cfa0
    virtual void MoveGWnd2(wnd_pos pos);

    /// \address 0046db40
    virtual void Func_40();

    /// \address 0046cff0
    virtual void UpdateTexturesCoordinates();

    /// \address 0046c2e0
    virtual void UpdateFontCoordinates();

    /// \address 0046d7c0
    /// \returns always true
    virtual bool OnCloseWnd();

    /// \address 0046cdf0
    virtual void EnableToolTipsCtrl();

    /// \address 0046c2f0
    virtual void Func_45();

    /// \address 0046caf0
    virtual bool IsInside(int x, int y);

    /// \address 0046cc80
    /// \returns
    /// true if it got set
    /// false if is empty
    virtual bool SetText(const wchar_t *src);

    /// \address 0046ccf0
    /// \returns m_wstrInnerText.c_str();
    virtual const wchar_t *GetText();

    /// \address 0046c330
    virtual void Func_49();

public:
    /// \address 0046d250
    void FontTextureRender() override;

    /// \address 0046cd10
    void OnAniFadeRelease() override;

    /// \address 0046ca10
    /// \returns m_btCurrentTextureAlpha
    BYTE GetAlphaScale() override;

    /// \address 0046d170
    void TextureRender() override;

public:
    /// \address 0046c310
    void DisableMouseActions(DWORD dwState);

    /// \address 0046c320
    void SetTooltipVerticalMode(DWORD dwMode);

    /// \address 0046c350
    void SetTooltipOwner(CNIFWnd *pOwner);

private:
    /// \address 0046c810
    int OnCreatedInstance(UINT lParam, UINT wParam);

    /// \address 0046c830
    int OnWindowPosChanged(UINT lParam, UINT wParam);

public:
    /// \address 0046ca80
    void StartAlphaAnimation(BYTE btAlphaStart, BYTE btAlphaMax, float fTime, float fStartupTime, bool bFadeIn);

    /// \brief Mostly using to check if the mouse pos on the element
    /// \address 0046cb40
    bool IsOnWindow(wnd_pos posMouse, wnd_rect rcWnd);

    /// \address 0046cc00
    void SetTextPosBounds(const RECT &rc);

    /// \address 0046cc00
    /// \returns FontBounds
    wnd_rect GetFontBounds() const;

    /// \address 0046d110
    void SetTextureHeadPos(D3DXVECTOR2 vTopLeft, D3DXVECTOR2 vTopRight,
                           D3DXVECTOR2 vBottomRight, D3DXVECTOR2 vBottomLeft);

protected:
    /// \brief Check Line:67 CIFWnd.cpp to understand how UV works
    /// \address 0046d2a0
    void TextureGroundColorRender(DWORD dwColor);

    /// \address 0046d5c0
    void OptimizeTextureHeadPos();

public:
    /// \address 0046d6f0
    /// \brief Move the right vertices
    void MoveTextureRightVertices(float fX);

    /// \adress 0046d740
    void OnGWndShow();

    /// \address 0046d7a0
    void SetTextureColorUV(D3DXVECTOR2 vUV);

    /// \brief Set the tooltip text (mouse hover)
    /// \address 0046d7d0
    void SetTooltipText(const std::n_wstring *pWStr);

    /// \address 0046d820
    /// \returns m_wstrTooltipText
    const std::n_wstring *GetTooltipText() const;

    /// \address 0046d830
    /// \returns m_wstrTooltipText size
    size_t GetTooltipTextLength() const;

    /// \address 0046d830
    /// \returns m_dwStyleOptionBit
    DWORD GetStyleOption() const;

    /// \brief Set Style for interface
    /// \See StyleOptions
    /// \address 0046d840
    void SetStyleThingy(StyleOptions eOptions);

    /// \address 0046d880
    /// \returns m_dwStyleOptionBit after got change
    DWORD AddStyleThingy(StyleOptions eOptions);

    /// \address 0046d8c0
    /// \returns m_dwStyleOptionBit after got change
    DWORD RemoveStyleThingy(StyleOptions eOptions);

    /// \address 0046d900
    bool IsStyleThingy(StyleOptions eStyle) const;

    /// \address 0046d910
    void UseCurrentAlphaScale();

    /// \address 0046d920
    void SetRenderViewport(wnd_rect *pRCWnd);

    /// \address 0046d9a0
    void DefaultRenderViewport();

    /// \address 0046d9f0
    void SetTextureBkColor(D3DCOLOR d3dColor);

    /// \address 0046da00
    void SetTextureFgColor(D3DCOLOR d3dColor);

private:
    /// \address 0046db50
    void SetEdgesHelperColor();

public:
    /// \address 0046dba0
    void InitEdgesHelper();

#if 0
    /// \address 0046dd00
    void UpdateEdgesHelperPos();
#endif // if 0

    /// \address 0046de20
    void EraseEdgesHelper();

#if 0
    /// \address 0046de80
    void ShowEdgesHelper();
#endif // if 0

protected:
    CNIFWnd *GetResObj(int nId) {
        return GetResObj < CNIFWnd > (nId);
    }

    /// \address 0046e060
    template<typename T>
    T *GetResObj(int nId) {
        std::n_list<CGWndBase *>::const_iterator it = N00000707.begin();
        for (; it != N00000707.end(); it++)
            if ((*it)->UniqueID() == nId)
                return (T *) (*it);
        // Not Found
        return NULL;
    }

public: // non client stuff
    inline bool IsStyleThingyShift(CNIFWnd::StyleOptionShiftType st) const {
        return ((m_dwStyleOptionBit >> st) & 1) != false;
    }

    // Sned GFX_WM_STYLE_CHANGE to the window msg proc
    inline void UpdateStyle() {
        UpdateTexturesCoordinates();
        Event3D sEventMsg;
        sEventMsg.Msg = GFX_WM_STYLE_CHANGE;
        OnGfxMessage(&sEventMsg);
    }

private:
    bool m_bMineAlphaScaleUse; //0x0188
    DWORD m_dwIsAniFadeIn; //0x018C
    DWORD m_dwStyleOptionBit; //0x0190
    RECT m_rcTextTexturePos; //0x0194 xRight, yDown, xLeft, yUp
    std::n_wstring m_wstrInnerText; //0x01A4
    DWORD m_dwEnableMouseActions; //0x01C0
public:
    DWORD m_dwTextureBGColor; //0x01C4 // BackGround
private:
    DWORD m_dwTextureFGColor; //0x01C8 // ForeGround
    float m_fRenderColorRight; //0x01CC // Using for the bg and fg color
    float m_fRenderColorDown; // 0x01D0 // Using for the bg and fg color
    DWORD m_dwTooltipValignTop; //0x01D4
    std::n_wstring m_wstrTooltipText; //0x01D8
    bool m_blWndDragMode; //0x01F4
    CNIFWnd *m_pParentOwner; //0x01F8
    std::n_string m_strParentsId; //0x01FC // Idk for wtf we will need it
    DWORD m_cdwParentsId; //0x0218 // like my parent id is 13 and mine is 12 so this member will be 25
    D3DXVECTOR2 m_vTextureUV[4]; //0x021C
    D3DVECTOR m_vTexturedRenderCords[8]; //0x023C
    D3DVECTOR m_vUnkCords_1; //0x029C Unknown maybe using for backup?
    D3DVECTOR m_vUnkCords_2; //0x02A8 Unknown maybe using for backup?
    D3DVECTOR m_vTextureColorRenderCords[8]; //0x02B4
    D3DVECTOR m_vUnkCords_3; //0x0314 Unknown maybe using for backup?
    char pad_0320[12]; //0x0320
    std::n_vector<CNIFWnd *> m_vecEdgesTextureHelper; //0x032C
    bool m_bEdgesHelperVisibleState; //0x033C
    DWORD m_dwWndType; //0x0340
    DWORD field_0344; //0x0344
private:
BEGIN_FIXTURE()
        ENSURE_SIZE(0x0348)
        ENSURE_OFFSET(m_bMineAlphaScaleUse, 0x0188)
        ENSURE_OFFSET(m_dwIsAniFadeIn, 0x018C)
        ENSURE_OFFSET(m_dwStyleOptionBit, 0x0190)
        ENSURE_OFFSET(m_rcTextTexturePos, 0x0194)
        ENSURE_OFFSET(m_wstrInnerText, 0x01A4)
        ENSURE_OFFSET(m_dwEnableMouseActions, 0x01C0)
        ENSURE_OFFSET(m_dwTextureBGColor, 0x01C4)
        ENSURE_OFFSET(m_dwTextureFGColor, 0x01C8)
        ENSURE_OFFSET(m_fRenderColorRight, 0x01CC)
        ENSURE_OFFSET(m_fRenderColorDown, 0x01D0)
        ENSURE_OFFSET(m_wstrTooltipText, 0x01D8)
        ENSURE_OFFSET(m_blWndDragMode, 0x01F4)
        ENSURE_OFFSET(m_vTextureUV, 0x021C)
        ENSURE_OFFSET(m_vTexturedRenderCords, 0x023C)
        ENSURE_OFFSET(m_vTextureColorRenderCords, 0x02B4)
        ENSURE_OFFSET(m_vecEdgesTextureHelper, 0x032C)
        ENSURE_OFFSET(m_bEdgesHelperVisibleState, 0x033C)
        ENSURE_OFFSET(m_dwWndType, 0x0340)
    END_FIXTURE()

    RUN_FIXTURE(CNIFWnd)
};

// CNIFHelperBubbleWindow
#define g_pNewInterfaceToolTips (*((CNIFWnd**)0x0111a030))
