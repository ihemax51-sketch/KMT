#pragma once

#include "IFWnd.h"

class CIFStatic : public CIFWnd {
    GFX_DECLARE_DYNAMIC_EXISTING(CIFStatic, 0x00EE9720)

public:
    /// \address 0064D5F0
    CIFStatic();

    /// \address 0064D690
    ~CIFStatic();

public:
    /// \address 0064D6B0
    bool OnCreate(long ln) override;

    /// \address 0064D810
    bool SetText(const wchar_t *src) override;

    /// \address 0064D850
    void OnGfxMessage(Event3D *pEvent) override;

    void RenderMyself_IMPL() {
        CIFStatic::RenderMyself();
    }

    /// \address 0064D940
    void RenderMyself() override;

    /// \address 0064D9B0
    void Func_42() override;

    void Func_42_IMPL() {
        CIFStatic::Func_42();
    }

    /// \address 0064D780
    virtual bool SetTextFormatted(const wchar_t *format, ...);

    /// \address 0064D6C0
    virtual bool Func_50(const wchar_t *format, ...);

    /// \address 0064D750
    virtual bool Func_51(const wchar_t *text);

public:
    DWORD m_dwTextFGColor;       //0x036C
    float m_fTextRenderPosStartX;//0x0370
    float m_fTextRenderPosStartY;//0x0374
    float m_fTextRenderPosEndX;  //0x0378
    float m_fTextRenderPosEndY;  //0x037c

private:
    BEGIN_FIXTURE()
    ENSURE_SIZE(0x380)
    ENSURE_OFFSET(m_fTextRenderPosStartX, 0x0370)
    ENSURE_OFFSET(m_fTextRenderPosStartY, 0x0374)
    ENSURE_OFFSET(m_fTextRenderPosEndX, 0x0378)
    ENSURE_OFFSET(m_fTextRenderPosEndY, 0x037c)
    END_FIXTURE()

    RUN_FIXTURE(CIFStatic)
};
