#pragma once

#include <windows.h>

class CWebViewerBridgeClient {
public:
    static bool EnsureLoaded();
    static bool IsRuntimeAvailable();
    static bool Show(HWND parentWindow, int x, int y, int width, int height, const wchar_t* title, const wchar_t* url);
    static bool Move(int x, int y, int width, int height);
    static void Hide();
    static void Destroy();
    static bool IsVisible();
};
