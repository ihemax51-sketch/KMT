#pragma once

#include <IFFrame.h>
#include <IFMainFrame.h>
#include <IFNormalTile.h>
#include <IFStatic.h>
#include "WebViewerConfig.h"

class CIFWebViewerFrame : public CIFMainFrame {
    GFX_DECLARE_DYNCREATE(CIFWebViewerFrame)

public:
    CIFWebViewerFrame();
    bool OnCreate(long ln) override;
    void OnUpdate() override;
    undefined1 OnCloseWnd() override;

    void OpenPage(const SWebViewerButtonConfig* config);
    void SyncWebView();

private:
    void LayoutChrome();

    const SWebViewerButtonConfig* m_config;
    CIFFrame* m_panelFrame;
    CIFNormalTile* m_background;
    CIFStatic* m_hint;
    int m_lastX;
    int m_lastY;
    int m_lastW;
    int m_lastH;
    DWORD m_lastFailureTick;
};
