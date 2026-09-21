#include "WndProc_Hook.h"
#include "Hooks.h"
#include <GInterface.h>

extern std::vector<WNDPROC> hooks_wndproc;
HWND g_orig_wndproc_hwnd = NULL;
static bool g_secondarySlotSpaceHeld = false;

static bool IsNativeTextEntry(HWND window)
{
    if (window == NULL || IsWindowVisible(window) == FALSE)
        return false;

    wchar_t className[64] = {0};
    if (GetClassNameW(window, className, sizeof(className) / sizeof(className[0])) == 0)
        return false;

    return _wcsicmp(className, L"Edit") == 0 ||
           _wcsnicmp(className, L"RichEdit", 8) == 0;
}

LRESULT CALLBACK WndProcHook(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    if (g_orig_wndproc_hwnd != hwnd)
    {
        g_orig_wndproc_hwnd = hwnd;
    }

    // Joymax's internal key callback is not guaranteed to receive modifier
    // combinations. Keep the physical Space state in the installed game
    // window procedure and dispatch the secondary-slot digits from here.
    if (msg == WM_KEYDOWN && wParam == VK_SPACE)
        g_secondarySlotSpaceHeld = true;
    else if (msg == WM_KEYUP && wParam == VK_SPACE)
        g_secondarySlotSpaceHeld = false;
    else if ((msg == WM_ACTIVATEAPP && wParam == FALSE) || msg == WM_KILLFOCUS)
        g_secondarySlotSpaceHeld = false;

    if (msg == WM_KEYDOWN &&
        g_secondarySlotSpaceHeld &&
        (lParam & (1UL << 30)) == 0 &&
        g_pCGInterface != NULL)
    {
        const HWND focusedWindow = GetFocus();
        if (!IsNativeTextEntry(focusedWindow) &&
            g_pCGInterface->TryUseSecondarySlotHotkey(static_cast<int>(wParam)))
        {
            return 0;
        }
    }

    // Custom windows do not all participate in Joymax's native Escape stack.
    // Close one here, then let OnKeyDown suppress only that same physical
    // key press. When nothing custom handled Escape, preserve the complete
    // native window-message and shortcut path.
    if (msg == WM_KEYDOWN && wParam == VK_ESCAPE)
    {
        if ((lParam & (1UL << 30)) == 0 && g_pCGInterface != NULL)
        {
            g_pCGInterface->SetWindowMessageEscapeHandled(false);
            if (g_pCGInterface->OnEscapePressed())
            {
                g_pCGInterface->SetWindowMessageEscapeHandled(true);
                return 0;
            }
        }
    }
    if (msg == WM_KEYUP && wParam == VK_ESCAPE)
    {
        if (g_pCGInterface != NULL)
            g_pCGInterface->SetWindowMessageEscapeHandled(false);
    }

    // Route the Pick Inventory shortcut through the installed window proc and
    // suppress it only for a real native text-entry control. The game commonly
    // focuses another visible child HWND while playing; treating every child
    // as text input prevents the shortcut from ever reaching the window.
    if (msg == WM_KEYDOWN && wParam == 'O')
    {
        const HWND focusedWindow = GetFocus();
        if (!IsNativeTextEntry(focusedWindow) &&
            (lParam & (1UL << 30)) == 0 &&
            g_pCGInterface != NULL &&
            g_pCGInterface->TogglePickInventoryWindow())
        {
            return 0;
        }
    }

	for (std::vector<WNDPROC>::iterator it = hooks_wndproc.begin();
		it != hooks_wndproc.end();
		++it)
	{
		const LRESULT hookResult = (*it)(hwnd, msg, wParam, lParam);
		if (hookResult == RESULT_DISCARD)
		{
			// Call default window proc because nothing happens otherwise ...
			return DefWindowProc(hwnd, msg, wParam, lParam);
		}
	}

	WNDPROC original = reinterpret_cast<WNDPROC>(0x008311C0);
	return original(hwnd, msg, wParam, lParam);
}
