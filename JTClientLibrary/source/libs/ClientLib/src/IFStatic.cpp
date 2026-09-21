#include "IFStatic.h"

#include "IFListCtrl.h"

GFX_IMPLEMENT_DYNAMIC_EXISTING(CIFStatic, 0x00EE9720)

CIFStatic::CIFStatic() {
    N00000609 = 0;
    m_dwTextFGColor = 0;

    m_FontTexture.SetColor(D3DCOLOR_RGBA(255, 255, 255, 255));
    m_FontTexture.sub_8B2BA0(3, D3DCOLOR_RGBA(0xB4, 0xB4, 0xB4, 255));
}

CIFStatic::~CIFStatic() {
    // empty
}

bool CIFStatic::OnCreate(long ln) {
    return true;
}

bool CIFStatic::SetText(const wchar_t *src) {
    if (src) {
        CIFWnd::SetText(src);
        Func_42();
        return true;
    }

    return false;
}

void CIFStatic::OnGfxMessage(Event3D *pEvent) {
    if (GetParentControl()->IsKindOf(GFX_RUNTIME_CLASS(CIFListCtrl))) {
        GetParentControl()->OnGfxMessage(pEvent);
        return;
    }
    CIFWnd::OnGfxMessage(pEvent);
}

void CIFStatic::RenderMyself() {
    /*CIFWnd::RenderMyself();

    if (m_FontTexture.HasCharacters()) {
        m_FontTexture.RenderTheText(static_cast<DWORD>(m_fTextRenderPosStartX), static_cast<DWORD>(m_fTextRenderPosStartY), TB_Func_8());
        ResetTextureAlphaState();
    }

    if (m_dwTextFGColor != 0) {
        RenderTextureColor(m_dwTextFGColor);
    }*/
    reinterpret_cast<void (__thiscall *)(CIFStatic *)>(0x0064D940)(this);
}

void CIFStatic::Func_42() {
   /* if (m_FontTexture.HasCharacters()) {
        RECT local_10;
        GetSomeRECT(local_10);
        const float startX = static_cast<float>(local_10.left);
        m_fTextRenderPosStartX = startX;
        const float startY = static_cast<float>(local_10.top);
        m_fTextRenderPosStartY = startY;
        const float endX = static_cast<float>(local_10.left + local_10.right);
        m_fTextRenderPosEndX = endX;
        const float endY = static_cast<float>(local_10.top + local_10.bottom);
        m_fTextRenderPosEndY = endY;

        const float bgHeight = static_cast<float>(m_FontTexture.GetDimensions().height);
        const float bgWidth = static_cast<float>(m_FontTexture.GetDimensions().width);

        switch (m_justificationHorizontal) {
            case CTextBoard::JUSTIFY_LEFT:
                m_fTextRenderPosEndX = bgWidth + startX;
                break;
            case CTextBoard::JUSTIFY_CENTER: {
                const float newX = ((endX - startX) - bgWidth) * 0.5f + startX;
                m_fTextRenderPosStartX = newX;
                m_fTextRenderPosEndX = newX + bgWidth;
            } break;
            case CTextBoard::JUSTIFY_RIGHT:
                m_fTextRenderPosStartX = endX - bgWidth;
                break;
        }

        switch (m_justificationVertical) {
            case CTextBoard::JUSTIFY_TOP:
                m_fTextRenderPosEndY = bgHeight + startY;
                break;
            case CTextBoard::JUSTIFY_MIDDLE: {
                float newY = ((endY + startY) - bgHeight) * 0.5f;
                m_fTextRenderPosStartY = newY;
                m_fTextRenderPosEndY = newY + bgHeight;
            } break;
            case CTextBoard::JUSTIFY_BOTTOM:
                m_fTextRenderPosStartY = endY - bgHeight;
                break;
        }
    }*/
    reinterpret_cast<void (__thiscall *)(CIFStatic *)>(0x0064D9B0)(this);
}

bool CIFStatic::SetTextFormatted(const wchar_t *format, ...) {
    if (format == NULL) {
        return false;
    }

    wchar_t buffer[0x1000];

    va_list args;
    va_start(args, format);
    vswprintf_s(buffer, 0x1000, format, args);
    va_end(args);

    // Note: SetText also returns a boolean telling if setting the text was successful. However
    //       the original code does ignore that value here.
    SetText(buffer);

    Func_42();

    return true;
}

bool CIFStatic::Func_50(const wchar_t *format, ...) {
    if (format == NULL) {
        return false;
    }

    wchar_t buffer[0x1000];

    va_list args;
    va_start(args, format);
    vswprintf_s(buffer, 0x1000, format, args);
    va_end(args);

    // Note: SetText also returns a boolean telling if setting the text was successful. However
    //       the original code does ignore that value here.
    SetText(buffer);

    Func_42();

    return true;
}

bool CIFStatic::Func_51(const wchar_t *text) {
    if (text == NULL) {
        return false;
    }

    CIFWnd::SetText(text);

    Func_42();

    return true;
}
