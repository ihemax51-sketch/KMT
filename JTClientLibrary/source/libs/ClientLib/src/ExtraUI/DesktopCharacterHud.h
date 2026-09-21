#pragma once

#include <windows.h>

namespace DesktopCharacterHud
{
    void SetEnabled(bool enabled);
    void PublishFromClient();
    void PublishFromPlayerMiniInfo();
    bool HasPortraitForRefObjectId(int refObjectId);
    bool RefreshPortraitFromTexturePath(int refObjectId, const char* texturePath);
    bool RefreshPortraitForRefObjectId(int refObjectId);
    LRESULT CALLBACK GameWndProcHook(HWND window, UINT message, WPARAM wParam, LPARAM lParam);
}
