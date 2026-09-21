# WebViewerBridge

`WebViewerBridge.dll` is the modern x86 WebView2 host used by the VC80 `KMTGuardKit.dll`.

Design rules:

- `KMTGuardKit.dll` stays VC80/x86 and owns game icons/game frames only.
- WebView2 is isolated in this modern x86 DLL.
- Boundary is C ABI only: no STL, MFC, or game classes are passed across DLLs.
- One singleton WebView2 instance is reused; opening another page navigates/resizes the same instance.

Exports:

```cpp
int __stdcall WVB_Initialize(HWND gameWindow);
int __stdcall WVB_Show(HWND parentWindow, int x, int y, int width, int height, const wchar_t* title, const wchar_t* url);
int __stdcall WVB_Navigate(const wchar_t* url);
int __stdcall WVB_Move(int x, int y, int width, int height);
int __stdcall WVB_Hide();
int __stdcall WVB_Destroy();
int __stdcall WVB_IsVisible();
int __stdcall WVB_IsRuntimeAvailable();
```

The `.def` file exports undecorated names, so VC80 can use:

```cpp
GetProcAddress(module, "WVB_Show");
```

Build:

```powershell
& "C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\MSBuild\Current\Bin\MSBuild.exe" `
  WebViewerBridge\WebViewerBridge.sln `
  /p:Configuration=Release /p:Platform=Win32
```

The project expects the WebView2 NuGet package at:

```text
%USERPROFILE%\.nuget\packages\microsoft.web.webview2\1.0.2849.39
```

If it is missing, run once:

```powershell
& "C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\MSBuild\Current\Bin\MSBuild.exe" `
  WebViewerBridge\WebViewerBridge.sln `
  /restore /p:Configuration=Release /p:Platform=Win32
```

Deployment files expected next to `sro_client.exe`:

- `KMTGuardKit.dll`
- `WebViewerBridge.dll`
- `WebView2Loader.dll` x86 from the `Microsoft.Web.WebView2` NuGet package
- Microsoft Edge WebView2 Runtime installed on the player's PC

## Dynamic game icons

The game-side icons are created by `KMTGuardKit.dll` from inside `Media.pk2` only, using the same loading style as `clientlibrary\config\menu_design.json`:

```text
clientlibrary\config\webviewer.json
```

Example:

```json
{
  "buttons": [
    {
      "name": "Website",
      "icon": "interface\\ifcommon\\com_mid_button.ddj",
      "url": "https://example.com/battle-pass",
      "frameWidth": 900,
      "frameHeight": 620
    }
  ]
}
```

Only these values are meant to be changed per button:

- `name`: icon tooltip and frame title.
- `icon`: DDJ path loaded by the game.
- `url`: page opened by WebView2.
- `frameWidth` / `frameHeight`: size of the in-game frame.

The frame title bar, close button, WebView placement, and single-WebView reuse are handled by code.

Current ready-to-test output folder:

```text
JTClientLibrary\BinOut\RelWithDebInfo
```

Copy these next to the client executable:

- `KMTGuardKit.dll`
- `WebViewerBridge.dll`
- `WebView2Loader.dll`

Pack this file into `Media.pk2`:

- `clientlibrary\config\webviewer.json`
