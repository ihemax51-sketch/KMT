#include "WebViewerBridgeClient.h"

typedef int (__stdcall *PFN_WVB_INITIALIZE)(HWND);
typedef int (__stdcall *PFN_WVB_SHOW)(HWND, int, int, int, int, const wchar_t*, const wchar_t*);
typedef int (__stdcall *PFN_WVB_NAVIGATE)(const wchar_t*);
typedef int (__stdcall *PFN_WVB_MOVE)(int, int, int, int);
typedef int (__stdcall *PFN_WVB_HIDE)();
typedef int (__stdcall *PFN_WVB_DESTROY)();
typedef int (__stdcall *PFN_WVB_ISVISIBLE)();
typedef int (__stdcall *PFN_WVB_ISRUNTIMEAVAILABLE)();

static HMODULE g_wvbModule = NULL;
static PFN_WVB_INITIALIZE g_wvbInitialize = NULL;
static PFN_WVB_SHOW g_wvbShow = NULL;
static PFN_WVB_MOVE g_wvbMove = NULL;
static PFN_WVB_HIDE g_wvbHide = NULL;
static PFN_WVB_DESTROY g_wvbDestroy = NULL;
static PFN_WVB_ISVISIBLE g_wvbIsVisible = NULL;
static PFN_WVB_ISRUNTIMEAVAILABLE g_wvbIsRuntimeAvailable = NULL;
static DWORD g_wvbLastLoadFailureTick = 0;

bool CWebViewerBridgeClient::EnsureLoaded() {
    if(g_wvbModule != NULL)
        return true;

    DWORD now = GetTickCount();
    if(g_wvbLastLoadFailureTick != 0 && now - g_wvbLastLoadFailureTick < 3000)
        return false;

    g_wvbModule = LoadLibraryA("WebViewerBridge.dll");
    if(g_wvbModule == NULL) {
        g_wvbLastLoadFailureTick = now;
        return false;
    }

    g_wvbInitialize = (PFN_WVB_INITIALIZE)GetProcAddress(g_wvbModule, "WVB_Initialize");
    g_wvbShow = (PFN_WVB_SHOW)GetProcAddress(g_wvbModule, "WVB_Show");
    g_wvbMove = (PFN_WVB_MOVE)GetProcAddress(g_wvbModule, "WVB_Move");
    g_wvbHide = (PFN_WVB_HIDE)GetProcAddress(g_wvbModule, "WVB_Hide");
    g_wvbDestroy = (PFN_WVB_DESTROY)GetProcAddress(g_wvbModule, "WVB_Destroy");
    g_wvbIsVisible = (PFN_WVB_ISVISIBLE)GetProcAddress(g_wvbModule, "WVB_IsVisible");
    g_wvbIsRuntimeAvailable = (PFN_WVB_ISRUNTIMEAVAILABLE)GetProcAddress(g_wvbModule, "WVB_IsRuntimeAvailable");

    if(g_wvbInitialize == NULL || g_wvbShow == NULL || g_wvbMove == NULL || g_wvbHide == NULL ||
       g_wvbDestroy == NULL || g_wvbIsVisible == NULL || g_wvbIsRuntimeAvailable == NULL) {
        FreeLibrary(g_wvbModule);
        g_wvbModule = NULL;
        g_wvbLastLoadFailureTick = now;
        return false;
    }

    g_wvbInitialize(GetForegroundWindow());
    g_wvbLastLoadFailureTick = 0;
    return true;
}

bool CWebViewerBridgeClient::IsRuntimeAvailable() {
    if(!EnsureLoaded())
        return false;
    return g_wvbIsRuntimeAvailable() != 0;
}

bool CWebViewerBridgeClient::Show(HWND parentWindow, int x, int y, int width, int height, const wchar_t* title, const wchar_t* url) {
    if(!EnsureLoaded())
        return false;
    if(parentWindow == NULL)
        parentWindow = GetForegroundWindow();
    return g_wvbShow(parentWindow, x, y, width, height, title, url) != 0;
}

bool CWebViewerBridgeClient::Move(int x, int y, int width, int height) {
    if(!EnsureLoaded())
        return false;
    return g_wvbMove(x, y, width, height) != 0;
}

void CWebViewerBridgeClient::Hide() {
    if(EnsureLoaded())
        g_wvbHide();
}

void CWebViewerBridgeClient::Destroy() {
    if(EnsureLoaded())
        g_wvbDestroy();
}

bool CWebViewerBridgeClient::IsVisible() {
    if(!EnsureLoaded())
        return false;
    return g_wvbIsVisible() != 0;
}
