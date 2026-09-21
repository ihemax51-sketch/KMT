#include "WebViewerBridge.h"

#include <WebView2.h>
#include <string>

namespace {

const wchar_t* kHostClassName = L"KMTGuardWebViewHostWindow";

HWND g_gameWindow = nullptr;
HWND g_parentWindow = nullptr;
HWND g_hostWindow = nullptr;
HINSTANCE g_instance = nullptr;

ICoreWebView2Environment* g_environment = nullptr;
ICoreWebView2Controller* g_controller = nullptr;
ICoreWebView2* g_webView = nullptr;

std::wstring g_pendingUrl;
std::wstring g_pendingTitle;
RECT g_lastRect = {0, 0, 900, 600};
bool g_creating = false;
bool g_visible = false;

template <typename T>
void SafeRelease(T*& ptr) {
    if (ptr) {
        ptr->Release();
        ptr = nullptr;
    }
}

void ApplyBounds() {
    if (!g_hostWindow)
        return;

    const int width = g_lastRect.right - g_lastRect.left;
    const int height = g_lastRect.bottom - g_lastRect.top;

    MoveWindow(g_hostWindow, g_lastRect.left, g_lastRect.top, width, height, TRUE);

    if (g_controller) {
        RECT bounds = {0, 0, width, height};
        g_controller->put_Bounds(bounds);
    }
}

LRESULT CALLBACK HostWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
    case WM_SIZE:
        if (g_controller) {
            RECT bounds = {0, 0, LOWORD(lParam), HIWORD(lParam)};
            g_controller->put_Bounds(bounds);
        }
        return 0;
    case WM_SETFOCUS:
        if (g_controller)
            g_controller->MoveFocus(COREWEBVIEW2_MOVE_FOCUS_REASON_PROGRAMMATIC);
        return 0;
    default:
        return DefWindowProcW(hwnd, msg, wParam, lParam);
    }
}

bool EnsureHostWindow(HWND parentWindow) {
    if (!parentWindow)
        return false;

    g_parentWindow = parentWindow;

    WNDCLASSEXW wc = {};
    wc.cbSize = sizeof(wc);
    wc.lpfnWndProc = HostWndProc;
    wc.hInstance = g_instance;
    wc.lpszClassName = kHostClassName;
    wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
    RegisterClassExW(&wc);

    if (!g_hostWindow) {
        g_hostWindow = CreateWindowExW(
            0,
            kHostClassName,
            L"",
            WS_CHILD | WS_CLIPSIBLINGS | WS_CLIPCHILDREN,
            g_lastRect.left,
            g_lastRect.top,
            g_lastRect.right - g_lastRect.left,
            g_lastRect.bottom - g_lastRect.top,
            parentWindow,
            nullptr,
            g_instance,
            nullptr);
    } else {
        SetParent(g_hostWindow, parentWindow);
    }

    return g_hostWindow != nullptr;
}

template <typename Interface>
class ComCallbackBase : public Interface {
public:
    ComCallbackBase() : refCount_(1) {}

    ULONG STDMETHODCALLTYPE AddRef() override {
        return InterlockedIncrement(&refCount_);
    }

    ULONG STDMETHODCALLTYPE Release() override {
        const ULONG result = InterlockedDecrement(&refCount_);
        if (result == 0)
            delete this;
        return result;
    }

    HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, void** object) override {
        if (!object)
            return E_POINTER;

        if (riid == __uuidof(IUnknown) || riid == __uuidof(Interface)) {
            *object = static_cast<Interface*>(this);
            AddRef();
            return S_OK;
        }

        *object = nullptr;
        return E_NOINTERFACE;
    }

private:
    volatile LONG refCount_;
};

class ControllerCompletedHandler final
    : public ComCallbackBase<ICoreWebView2CreateCoreWebView2ControllerCompletedHandler> {
public:
    HRESULT STDMETHODCALLTYPE Invoke(HRESULT result, ICoreWebView2Controller* controller) override {
        g_creating = false;

        if (FAILED(result) || !controller)
            return result;

        SafeRelease(g_controller);
        SafeRelease(g_webView);

        g_controller = controller;
        g_controller->AddRef();
        g_controller->get_CoreWebView2(&g_webView);

        ApplyBounds();
        g_controller->put_IsVisible(g_visible ? TRUE : FALSE);

        if (g_webView && !g_pendingUrl.empty())
            g_webView->Navigate(g_pendingUrl.c_str());

        return S_OK;
    }
};

class EnvironmentCompletedHandler final
    : public ComCallbackBase<ICoreWebView2CreateCoreWebView2EnvironmentCompletedHandler> {
public:
    HRESULT STDMETHODCALLTYPE Invoke(HRESULT result, ICoreWebView2Environment* environment) override {
        if (FAILED(result) || !environment) {
            g_creating = false;
            return result;
        }

        SafeRelease(g_environment);
        g_environment = environment;
        g_environment->AddRef();

        return g_environment->CreateCoreWebView2Controller(
            g_hostWindow,
            new ControllerCompletedHandler());
    }
};

int EnsureWebView(HWND parentWindow) {
    if (!EnsureHostWindow(parentWindow))
        return 0;

    ShowWindow(g_hostWindow, SW_SHOW);
    ApplyBounds();

    if (g_webView) {
        if (!g_pendingUrl.empty())
            g_webView->Navigate(g_pendingUrl.c_str());
        if (g_controller)
            g_controller->put_IsVisible(TRUE);
        return 1;
    }

    if (g_creating)
        return 1;

    g_creating = true;

    const HRESULT hr = CreateCoreWebView2EnvironmentWithOptions(
        nullptr,
        nullptr,
        nullptr,
        new EnvironmentCompletedHandler());

    if (FAILED(hr)) {
        g_creating = false;
        return 0;
    }

    return 1;
}

} // namespace

BOOL APIENTRY DllMain(HMODULE module, DWORD reason, LPVOID) {
    if (reason == DLL_PROCESS_ATTACH) {
        g_instance = reinterpret_cast<HINSTANCE>(module);
        DisableThreadLibraryCalls(module);
    } else if (reason == DLL_PROCESS_DETACH) {
        WVB_Destroy();
    }
    return TRUE;
}

WVB_API int __stdcall WVB_Initialize(HWND gameWindow) {
    g_gameWindow = gameWindow;
    return 1;
}

WVB_API int __stdcall WVB_Show(HWND parentWindow, int x, int y, int width, int height, const wchar_t* title, const wchar_t* url) {
    if (!parentWindow || width <= 0 || height <= 0 || !url || !url[0])
        return 0;

    g_lastRect.left = x;
    g_lastRect.top = y;
    g_lastRect.right = x + width;
    g_lastRect.bottom = y + height;
    g_pendingTitle = title ? title : L"";
    g_pendingUrl = url;
    g_visible = true;

    return EnsureWebView(parentWindow);
}

WVB_API int __stdcall WVB_Navigate(const wchar_t* url) {
    if (!url || !url[0])
        return 0;

    g_pendingUrl = url;
    if (g_webView)
        return SUCCEEDED(g_webView->Navigate(g_pendingUrl.c_str())) ? 1 : 0;

    return 1;
}

WVB_API int __stdcall WVB_Move(int x, int y, int width, int height) {
    if (width <= 0 || height <= 0)
        return 0;

    g_lastRect.left = x;
    g_lastRect.top = y;
    g_lastRect.right = x + width;
    g_lastRect.bottom = y + height;
    ApplyBounds();
    return 1;
}

WVB_API int __stdcall WVB_Hide() {
    g_visible = false;
    if (g_controller)
        g_controller->put_IsVisible(FALSE);
    if (g_hostWindow)
        ShowWindow(g_hostWindow, SW_HIDE);
    return 1;
}

WVB_API int __stdcall WVB_Destroy() {
    g_visible = false;

    if (g_controller)
        g_controller->Close();

    SafeRelease(g_webView);
    SafeRelease(g_controller);
    SafeRelease(g_environment);

    if (g_hostWindow) {
        DestroyWindow(g_hostWindow);
        g_hostWindow = nullptr;
    }

    g_creating = false;
    return 1;
}

WVB_API int __stdcall WVB_IsVisible() {
    return g_visible ? 1 : 0;
}

WVB_API int __stdcall WVB_IsRuntimeAvailable() {
    LPWSTR versionInfo = nullptr;
    const HRESULT hr = GetAvailableCoreWebView2BrowserVersionString(nullptr, &versionInfo);
    if (versionInfo)
        CoTaskMemFree(versionInfo);
    return SUCCEEDED(hr) ? 1 : 0;
}
