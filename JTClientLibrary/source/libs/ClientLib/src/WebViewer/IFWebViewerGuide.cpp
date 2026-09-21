#include "IFWebViewerGuide.h"
#include "IFWebViewerFrame.h"
#include "WebViewerConfig.h"
#include <GInterface.h>
#include <GEffSoundBody.h>

GFX_IMPLEMENT_DYNCREATE(CIFWebViewerGuide, CIFDecoratedStatic)

bool CIFWebViewerGuide::OnCreate(long ln) {
    CIFDecoratedStatic::OnCreate(ln);

    const SWebViewerButtonConfig* cfg = CWebViewerConfig::GetByGuideId(UniqueID());
    const char* iconPath = "clientlibrary\\guides\\kmt_web_viewer_1.ddj";
    const char* hoverIconPath = "clientlibrary\\guides\\kmt_web_viewer_2.ddj";
    if(cfg != NULL && !cfg->IconPath.empty() &&
       cfg->IconPath != "interface\\ifcommon\\com_mid_button.ddj" &&
       cfg->IconPath != "icon\\etc\\location_1.ddj") {
        iconPath = cfg->IconPath.c_str();
        hoverIconPath = iconPath;
    }

    TB_Func_13(iconPath, 0, 0);
    sub_634470(hoverIconPath);
    set_N00009BD4(2);
    set_N00009BD3(500);

    m_label = NULL;
    m_IRM.LoadFromFile("clientlibrary\\resinfo\\ifsimple.txt");
    m_IRM.CreateInterfaceSection("Create", this);
    m_label = m_IRM.GetResObj<CIFStatic>(1, 0);
    if(m_label) {
        if(cfg != NULL)
            m_label->SetTooltip(cfg->WideName);
        else
            m_label->SetTooltip(KmtGetText(L"UIIT_KMT_WEB_VIEWER"));
        m_label->SetStyleThingy(TOOLTIP);
    }

    return true;
}

int CIFWebViewerGuide::OnMouseLeftUp(int a1, int x, int y) {
    const SWebViewerButtonConfig* cfg = CWebViewerConfig::GetByGuideId(UniqueID());
    if(cfg == NULL)
        return 0;

    CIFWebViewerFrame* wnd = g_pCGInterface->m_IRM.GetResObj<CIFWebViewerFrame>(WEBVIEWER_FRAME_ID, 1);
    if(wnd == NULL) {
        wnd_rect rect;
        rect.pos.x = 0;
        rect.pos.y = 0;
        rect.size.width = cfg->FrameWidth;
        rect.size.height = cfg->FrameHeight;
        wnd = (CIFWebViewerFrame*)CGWnd::CreateInstance(
            g_pCGInterface, GFX_RUNTIME_CLASS(CIFWebViewerFrame), rect, WEBVIEWER_FRAME_ID, 0);
        if(wnd == NULL)
            return 0;
    }

    wnd->OpenPage(cfg);
    CGEffSoundBody::get()->PlaySound(L"snd_window_open");
    return 0;
}

void CIFWebViewerGuide::OnCIFReady() {
    CIFDecoratedStatic::OnCIFReady();
    sub_633990();
}
