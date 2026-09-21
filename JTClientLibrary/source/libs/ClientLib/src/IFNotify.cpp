#include "IFNotify.h"
#include <windows.h>
#include <d3d9.h>
#include <string.h>

namespace {
    CIFStretchWnd* g_coloredNotice = NULL;
    D3DCOLOR g_bannerColor = 0;

    void __fastcall RenderNoticeEdges(CIFStretchWnd* window, void*, int alpha) {
        typedef void (__thiscall* RenderEdges)(CIFStretchWnd*, int);
        RenderEdges original = reinterpret_cast<RenderEdges>(0x00819D80);
        IDirect3DDevice9* device = *reinterpret_cast<IDirect3DDevice9**>(0x01112428);
        if (window != g_coloredNotice || !g_bannerColor || !device) {
            original(window, alpha);
            return;
        }

        // Verified on the supplied v188 client: native edge rendering sets
        // stage 0 alpha = texture alpha * fade, but retains the DDJ's red RGB.
        // Stage 1 replaces RGB only, keeping the original gradient and fade.
        // Restore exact device state so the client's cached states stay valid.
        const D3DTEXTURESTAGESTATETYPE states[] = {
            D3DTSS_COLOROP, D3DTSS_COLORARG1, D3DTSS_ALPHAOP,
            D3DTSS_ALPHAARG1, D3DTSS_CONSTANT
        };
        DWORD saved[5];
        for (int i = 0; i < 5; ++i) {
            if (FAILED(device->GetTextureStageState(1, states[i], &saved[i]))) {
                original(window, alpha);
                return;
            }
        }
        HRESULT result = device->SetTextureStageState(1, D3DTSS_CONSTANT, g_bannerColor);
        if (SUCCEEDED(result)) result = device->SetTextureStageState(1, D3DTSS_COLORARG1, D3DTA_CONSTANT);
        if (SUCCEEDED(result)) result = device->SetTextureStageState(1, D3DTSS_ALPHAARG1, D3DTA_CURRENT);
        if (SUCCEEDED(result)) result = device->SetTextureStageState(1, D3DTSS_ALPHAOP, D3DTOP_SELECTARG1);
        if (SUCCEEDED(result)) result = device->SetTextureStageState(1, D3DTSS_COLOROP, D3DTOP_SELECTARG1);
        if (FAILED(result)) {
            for (int i = 0; i < 5; ++i) device->SetTextureStageState(1, states[i], saved[i]);
        }
        original(window, alpha);
        for (int i = 0; i < 5; ++i) device->SetTextureStageState(1, states[i], saved[i]);
    }
}

void SetNoticeBannerTint(CIFNotify* notice, D3DCOLOR color) {
    static bool installed = false;
    if (!installed) {
        // Only redirect the verified stretch-edge call; other windows retain
        // their native rendering. The on-disk client executable is untouched.
        unsigned char* call = reinterpret_cast<unsigned char*>(0x0081A462);
        const unsigned char expected[] = {0xE8, 0x19, 0xF9, 0xFF, 0xFF};
        if (memcmp(call, expected, sizeof(expected)) != 0) return;
        DWORD protection;
        if (!VirtualProtect(call, 5, PAGE_EXECUTE_READWRITE, &protection)) return;
        const DWORD displacement = reinterpret_cast<DWORD>(&RenderNoticeEdges) -
                                   reinterpret_cast<DWORD>(call + 5);
        memcpy(call + 1, &displacement, sizeof(displacement));
        DWORD ignored;
        VirtualProtect(call, 5, protection, &ignored);
        FlushInstructionCache(GetCurrentProcess(), call, 5);
        installed = true;
    }
    g_coloredNotice = color ? notice : NULL;
    g_bannerColor = color;
}

void CIFNotify::ShowMessage(const std::n_wstring &msg) {
    reinterpret_cast<void(__thiscall*)(CIFNotify*,const std::n_wstring*)>(0x007b3eb0)(this, &msg);
}

void CIFNotify::SetYPosition(int yposition) {
    m_yposition = yposition;
}

void CIFNotify::SetColor(unsigned char red, unsigned char green, unsigned char blue) {
    m_red = red;
    m_green = green;
    m_blue = blue;
}

void CIFNotify::GetColor(unsigned char &red, unsigned char &green, unsigned char &blue) const {
    red = m_red;
    green = m_green;
    blue = m_blue;
}
