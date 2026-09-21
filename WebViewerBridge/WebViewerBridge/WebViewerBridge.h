#pragma once

#include <windows.h>

#ifdef WEBVIEWERBRIDGE_EXPORTS
#define WVB_API extern "C" __declspec(dllexport)
#else
#define WVB_API extern "C" __declspec(dllimport)
#endif

// C ABI only. Do not pass STL/MFC/C++ objects across the DLL boundary.
// All strings are UTF-16 null-terminated and owned by the caller.

WVB_API int __stdcall WVB_Initialize(HWND gameWindow);
WVB_API int __stdcall WVB_Show(HWND parentWindow, int x, int y, int width, int height, const wchar_t* title, const wchar_t* url);
WVB_API int __stdcall WVB_Navigate(const wchar_t* url);
WVB_API int __stdcall WVB_Move(int x, int y, int width, int height);
WVB_API int __stdcall WVB_Hide();
WVB_API int __stdcall WVB_Destroy();
WVB_API int __stdcall WVB_IsVisible();
WVB_API int __stdcall WVB_IsRuntimeAvailable();

