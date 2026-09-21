#include "IFWebViewerFrame.h"
#include "WebViewerBridgeClient.h"
#include <Game.h>
#include <GEffSoundBody.h>
#include <ICPlayer.h>
#include <windows.h>

#define WEBVIEWER_FRAME_X 7
#define WEBVIEWER_FRAME_TOP 37
#define WEBVIEWER_FRAME_BOTTOM 7
#define WEBVIEWER_CONTENT_X 20
#define WEBVIEWER_CONTENT_TOP 49
#define WEBVIEWER_CONTENT_RIGHT 20
#define WEBVIEWER_CONTENT_BOTTOM 20
#define WEBVIEWER_CLOSE_W 16
#define WEBVIEWER_CLOSE_H 16
#define WEBVIEWER_CLOSE_RIGHT 10
#define WEBVIEWER_CLOSE_TOP 9

static bool IsUrlUnreserved(unsigned char c) {
    return (c >= 'A' && c <= 'Z') ||
           (c >= 'a' && c <= 'z') ||
           (c >= '0' && c <= '9') ||
           c == '-' || c == '_' || c == '.' || c == '~';
}

static std::n_wstring UrlEncodeUtf8(const std::n_wstring& value) {
    if(value.empty())
        return std::n_wstring();

    int len = WideCharToMultiByte(CP_UTF8, 0, value.c_str(), -1, NULL, 0, NULL, NULL);
    if(len <= 1)
        return std::n_wstring();

    char* buffer = new char[len];
    int written = WideCharToMultiByte(CP_UTF8, 0, value.c_str(), -1, buffer, len, NULL, NULL);
    if(written <= 0) {
        delete [] buffer;
        return std::n_wstring();
    }

    static const wchar_t hex[] = L"0123456789ABCDEF";
    std::n_wstring encoded;
    for(int i = 0; i < written - 1; ++i) {
        unsigned char c = static_cast<unsigned char>(buffer[i]);
        if(IsUrlUnreserved(c)) {
            encoded += static_cast<wchar_t>(c);
        } else {
            encoded += L'%';
            encoded += hex[(c >> 4) & 0x0F];
            encoded += hex[c & 0x0F];
        }
    }

    delete [] buffer;
    return encoded;
}

static std::n_wstring BuildWebViewerUrl(const std::n_wstring& url) {
    std::n_wstring charName;
    if(g_pMyPlayerObj != NULL)
        charName = g_pMyPlayerObj->GetCharName();

    if(charName.empty())
        return url;

    size_t fragmentPos = url.find(L'#');
    std::n_wstring base = fragmentPos == std::n_wstring::npos ? url : url.substr(0, fragmentPos);
    std::n_wstring fragment = fragmentPos == std::n_wstring::npos ? std::n_wstring() : url.substr(fragmentPos);

    std::n_wstring result = base;
    if(!result.empty()) {
        wchar_t last = result[result.size() - 1];
        if(last != L'?' && last != L'&')
            result += result.find(L'?') == std::n_wstring::npos ? L'?' : L'&';
    } else {
        result += L'?';
    }

    result += L"charname=";
    result += UrlEncodeUtf8(charName);
    result += fragment;
    return result;
}

GFX_IMPLEMENT_DYNCREATE(CIFWebViewerFrame, CIFMainFrame)

CIFWebViewerFrame::CIFWebViewerFrame()
    : m_config(NULL), m_panelFrame(NULL), m_background(NULL), m_hint(NULL),
      m_lastX(-1), m_lastY(-1), m_lastW(-1), m_lastH(-1), m_lastFailureTick(0) {
}

bool CIFWebViewerFrame::OnCreate(long ln) {
    CIFMainFrame::OnCreate(ln);

    TB_Func_13("interface\\frame\\mframe_wnd_", 0, 1);
    SetText(KmtGetText(L"UIIT_KMT_WEB_VIEWER"));
    SetGWndSize(900, 620);

    if(m_pCloseBtn) {
        m_pCloseBtn->ShowGWnd(true);
        m_pCloseBtn->TB_Func_13("interface\\ifcommon\\com_windowclose.ddj", 0, 0);
        m_pCloseBtn->SetGWndSize(WEBVIEWER_CLOSE_W, WEBVIEWER_CLOSE_H);
        m_pCloseBtn->BringToFront();
    }

    RECT frameRect = {
        WEBVIEWER_FRAME_X,
        WEBVIEWER_FRAME_TOP,
        900 - (WEBVIEWER_FRAME_X * 2),
        620 - WEBVIEWER_FRAME_TOP - WEBVIEWER_FRAME_BOTTOM
    };
    m_panelFrame = (CIFFrame*)CGWnd::CreateInstance(
        this, GFX_RUNTIME_CLASS(CIFFrame), frameRect, 1, 0);
    if(m_panelFrame) {
        m_panelFrame->SetFrameTexture(
            std::n_string("interface\\inventory\\int_window_"));
        m_panelFrame->SetClickable(false);
        m_panelFrame->ShowGWnd(true);
    }

    RECT bgRect = {
        WEBVIEWER_CONTENT_X,
        WEBVIEWER_CONTENT_TOP,
        900 - WEBVIEWER_CONTENT_X - WEBVIEWER_CONTENT_RIGHT,
        620 - WEBVIEWER_CONTENT_TOP - WEBVIEWER_CONTENT_BOTTOM
    };
    m_background = (CIFNormalTile*)CGWnd::CreateInstance(
        this, GFX_RUNTIME_CLASS(CIFNormalTile), bgRect, 2, 0);
    if(m_background) {
        m_background->TB_Func_13("interface\\ifcommon\\bg_tile\\com_bg_tile_b.ddj", 0, 1);
        m_background->SetClickable(false);
        m_background->ShowGWnd(true);
        m_background->BringToFront();
    }

    RECT hintRect = { 30, 290, 840, 24 };
    m_hint = (CIFStatic*)CGWnd::CreateInstance(this, GFX_RUNTIME_CLASS(CIFStatic), hintRect, 3, 0);
    if(m_hint) {
        m_hint->SetText(KmtGetText(L"UIIT_KMT_LOADING_WEB_PAGE"));
        m_hint->SetFont(theApp.GetFont(0));
        m_hint->m_FontTexture.SetColor(0xFFEED7A8);
        m_hint->JustifyHorizontal(JUSTIFY_CENTER);
        m_hint->JustifyVertical(JUSTIFY_MIDDLE);
        m_hint->SetClickable(false);
        m_hint->ShowGWnd(true);
        m_hint->BringToFront();
    }

    if(m_pTitleText)
        m_pTitleText->m_FontTexture.SetColor(0xFFFFFFFF);

    if(!m_panelFrame || !m_background || !m_hint)
        return false;

    LayoutChrome();
    ShowGWnd(false);
    return true;
}

void CIFWebViewerFrame::LayoutChrome() {
    wnd_rect bounds = GetBounds();
    int baseX = bounds.pos.x;
    int baseY = bounds.pos.y;
    int width = bounds.size.width;
    int height = bounds.size.height;

    if(width < 320)
        width = 320;
    if(height < 240)
        height = 240;

    if(m_pCloseBtn) {
        m_pCloseBtn->SetGWndSize(WEBVIEWER_CLOSE_W, WEBVIEWER_CLOSE_H);
        m_pCloseBtn->MoveGWnd(baseX + width - WEBVIEWER_CLOSE_W - WEBVIEWER_CLOSE_RIGHT, baseY + WEBVIEWER_CLOSE_TOP);
        m_pCloseBtn->BringToFront();
    }

    if(m_panelFrame) {
        m_panelFrame->MoveGWnd(
            baseX + WEBVIEWER_FRAME_X,
            baseY + WEBVIEWER_FRAME_TOP);
        m_panelFrame->SetGWndSize(
            width - (WEBVIEWER_FRAME_X * 2),
            height - WEBVIEWER_FRAME_TOP - WEBVIEWER_FRAME_BOTTOM);
        m_panelFrame->BringToFront();
    }

    if(m_background) {
        m_background->MoveGWnd(
            baseX + WEBVIEWER_CONTENT_X,
            baseY + WEBVIEWER_CONTENT_TOP);
        m_background->SetGWndSize(
            width - WEBVIEWER_CONTENT_X - WEBVIEWER_CONTENT_RIGHT,
            height - WEBVIEWER_CONTENT_TOP - WEBVIEWER_CONTENT_BOTTOM);
        m_background->BringToFront();
    }

    if(m_hint) {
        int contentHeight = height - WEBVIEWER_CONTENT_TOP - WEBVIEWER_CONTENT_BOTTOM;
        m_hint->MoveGWnd(
            baseX + WEBVIEWER_CONTENT_X + 12,
            baseY + WEBVIEWER_CONTENT_TOP + ((contentHeight - 24) / 2));
        m_hint->SetGWndSize(
            width - WEBVIEWER_CONTENT_X - WEBVIEWER_CONTENT_RIGHT - 24,
            24);
        m_hint->BringToFront();
    }

    if(m_pTitleText)
        m_pTitleText->BringToFront();
    if(m_pCloseBtn)
        m_pCloseBtn->BringToFront();
}

void CIFWebViewerFrame::OpenPage(const SWebViewerButtonConfig* config) {
    if(config == NULL)
        return;

    m_config = config;
    m_lastX = -1;
    m_lastY = -1;
    m_lastW = -1;
    m_lastH = -1;
    m_lastFailureTick = 0;

    SetText(config->WideName.c_str());
    int screenWidth = 1024;
    int screenHeight = 768;
    if(g_CGame && g_CGame->GetRes().res) {
        screenWidth = g_CGame->GetRes().res->width;
        screenHeight = g_CGame->GetRes().res->height;
    }

    int frameWidth = config->FrameWidth;
    int frameHeight = config->FrameHeight;
    if(frameWidth > screenWidth)
        frameWidth = screenWidth;
    if(frameHeight > screenHeight)
        frameHeight = screenHeight;

    SetGWndSize(frameWidth, frameHeight);
    LayoutChrome();

    if(m_hint) {
        m_hint->ShowGWnd(true);
        m_hint->BringToFront();
    }
    if(m_pCloseBtn)
        m_pCloseBtn->BringToFront();

    int posX = (screenWidth - frameWidth) / 2;
    int posY = ((screenHeight - frameHeight) / 2) + 10;
    if(posX < 0)
        posX = 0;
    if(posY < 0)
        posY = 0;
    if(posX + frameWidth > screenWidth)
        posX = screenWidth - frameWidth;
    if(posY + frameHeight > screenHeight)
        posY = screenHeight - frameHeight;

    MoveGWnd(posX, posY);
    ShowGWnd(true);
    LayoutChrome();
    BringToFront();
    SyncWebView();
}

void CIFWebViewerFrame::SyncWebView() {
    if(m_config == NULL || !IsVisible())
        return;

    LayoutChrome();

    wnd_rect bounds = GetBounds();
    int contentX = bounds.pos.x + WEBVIEWER_CONTENT_X;
    int contentY = bounds.pos.y + WEBVIEWER_CONTENT_TOP;
    int contentW = bounds.size.width - WEBVIEWER_CONTENT_X - WEBVIEWER_CONTENT_RIGHT;
    int contentH = bounds.size.height - WEBVIEWER_CONTENT_TOP - WEBVIEWER_CONTENT_BOTTOM;

    if(contentW < 100 || contentH < 100)
        return;

    if(m_lastX == contentX && m_lastY == contentY && m_lastW == contentW && m_lastH == contentH)
        return;

    DWORD now = GetTickCount();
    if(m_lastFailureTick != 0 && now - m_lastFailureTick < 3000)
        return;

    HWND parentWindow = GetForegroundWindow();
    std::n_wstring url = BuildWebViewerUrl(m_config->WideUrl);
    if(!CWebViewerBridgeClient::Show(parentWindow, contentX, contentY, contentW, contentH,
                                     m_config->WideName.c_str(), url.c_str())) {
        if(m_hint)
            m_hint->SetText(KmtGetText(L"UIIT_KMT_WEBVIEW2_FAILED_CHECK_WEBVIEWERBRIDGE_DLL_RUNTIME"));
        m_lastFailureTick = now;
        return;
    }

    if(m_hint)
        m_hint->ShowGWnd(false);

    m_lastX = contentX;
    m_lastY = contentY;
    m_lastW = contentW;
    m_lastH = contentH;
    m_lastFailureTick = 0;
}

void CIFWebViewerFrame::OnUpdate() {
    CIFMainFrame::OnUpdate();
    SyncWebView();
}

undefined1 CIFWebViewerFrame::OnCloseWnd() {
    CWebViewerBridgeClient::Hide();
    return CIFWnd::OnCloseWnd();
}
