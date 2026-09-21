#pragma once

#include <BSLib/multibyte.h>
#include <string>
#include <vector>

#define WEBVIEWER_GUIDE_BASE_ID 65000
#define WEBVIEWER_FRAME_ID 65080
#define WEBVIEWER_MAX_BUTTONS 16

struct SWebViewerButtonConfig {
    int Id;
    std::n_string Name;
    std::n_string IconPath;
    std::n_string Url;
    int FrameWidth;
    int FrameHeight;
    std::n_wstring WideName;
    std::n_wstring WideUrl;
};

class CWebViewerConfig {
public:
    static bool Load();
    static bool Reload();
    static int GetCount();
    static const SWebViewerButtonConfig* GetByIndex(int index);
    static const SWebViewerButtonConfig* GetByGuideId(int guideId);
    static bool IsWebViewerGuideId(int guideId);
    static void SetRuntimeButtons(const std::vector<SWebViewerButtonConfig>& buttons);
    static bool HasRuntimeButtons();

private:
    static bool LoadFromPath(const char* path);
};
