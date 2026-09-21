#include "WebViewerConfig.h"
#include <windows.h>
#include <Game.h>
#include <GFXFM/IFileManager.h>

static bool TryReadMediaFileRaw(IFileManager* fileManager, const char* path, char** outBuffer, int* outSize) {
    *outBuffer = NULL;
    *outSize = 0;

    if(fileManager == NULL || path == NULL)
        return false;

    __try {
        int handle = fileManager->Open(path, 0, 0);
        if(handle == -1)
            return false;

        int size = fileManager->GetFileSize(handle, NULL);
        if(size <= 0 || size > 1024 * 1024) {
            fileManager->Close(handle);
            return false;
        }

        char* buffer = new char[size];
        unsigned long bytesRead = 0;
        int result = fileManager->Read(handle, buffer, size, &bytesRead);
        fileManager->Close(handle);

        if(result == 0 || bytesRead != (unsigned long)size) {
            delete [] buffer;
            return false;
        }

        *outBuffer = buffer;
        *outSize = size;
        return true;
    }
    __except(EXCEPTION_EXECUTE_HANDLER) {
        *outBuffer = NULL;
        *outSize = 0;
        return false;
    }
}

static std::vector<SWebViewerButtonConfig> g_webViewerButtons;
static bool g_webViewerLoaded = false;
static bool g_webViewerRuntimeConfig = false;

static std::n_wstring NarrowToWide(const std::n_string& value) {
    if(value.empty())
        return std::n_wstring();

    int len = MultiByteToWideChar(CP_UTF8, 0, value.c_str(), -1, NULL, 0);
    if(len <= 1)
        len = MultiByteToWideChar(CP_ACP, 0, value.c_str(), -1, NULL, 0);

    if(len <= 1)
        return std::n_wstring();

    wchar_t* buffer = new wchar_t[len + 1];
    ZeroMemory(buffer, sizeof(wchar_t) * (len + 1));

    int written = MultiByteToWideChar(CP_UTF8, 0, value.c_str(), -1, buffer, len);
    if(written <= 0)
        MultiByteToWideChar(CP_ACP, 0, value.c_str(), -1, buffer, len);

    std::n_wstring result(buffer);
    delete [] buffer;
    return result;
}

static std::n_string ReadMediaFile(const char* path) {
    char* buffer = NULL;
    int size = 0;
    TryReadMediaFileRaw(g_pCFileManager, path, &buffer, &size);

    if(buffer == NULL || size <= 0)
        return std::n_string();

    std::n_string text;
    text.assign(buffer, buffer + size);
    delete [] buffer;

    if(text.size() >= 3 &&
       (unsigned char)text[0] == 0xEF &&
       (unsigned char)text[1] == 0xBB &&
       (unsigned char)text[2] == 0xBF) {
        text.erase(0, 3);
    }

    return text;
}

static void SkipSpaces(const std::n_string& text, size_t& pos) {
    while(pos < text.size()) {
        char c = text[pos];
        if(c != ' ' && c != '\t' && c != '\r' && c != '\n')
            break;
        ++pos;
    }
}

static std::n_string ExtractString(const std::n_string& obj, const char* key) {
    std::n_string needle = "\"";
    needle += key;
    needle += "\"";

    size_t p = obj.find(needle);
    if(p == std::n_string::npos)
        return std::n_string();

    p = obj.find(':', p + needle.size());
    if(p == std::n_string::npos)
        return std::n_string();

    ++p;
    SkipSpaces(obj, p);
    if(p >= obj.size() || obj[p] != '"')
        return std::n_string();

    ++p;
    std::n_string out;
    while(p < obj.size()) {
        char c = obj[p++];
        if(c == '\\' && p < obj.size()) {
            char n = obj[p++];
            if(n == 'n') out += '\n';
            else if(n == 'r') out += '\r';
            else if(n == 't') out += '\t';
            else out += n;
            continue;
        }
        if(c == '"')
            break;
        out += c;
    }
    return out;
}

static int ExtractInt(const std::n_string& obj, const char* key, int defaultValue) {
    std::n_string needle = "\"";
    needle += key;
    needle += "\"";

    size_t p = obj.find(needle);
    if(p == std::n_string::npos)
        return defaultValue;

    p = obj.find(':', p + needle.size());
    if(p == std::n_string::npos)
        return defaultValue;

    ++p;
    SkipSpaces(obj, p);
    return atoi(obj.c_str() + p);
}

bool CWebViewerConfig::LoadFromPath(const char* path) {
    std::n_string text = ReadMediaFile(path);
    if(text.empty())
        return false;

    std::vector<SWebViewerButtonConfig> loaded;

    size_t arrayPos = text.find("\"buttons\"");
    if(arrayPos == std::n_string::npos)
        arrayPos = 0;

    size_t pos = text.find('{', arrayPos);
    while(pos != std::n_string::npos && loaded.size() < WEBVIEWER_MAX_BUTTONS) {
        size_t end = text.find('}', pos + 1);
        if(end == std::n_string::npos)
            break;

        std::n_string obj = text.substr(pos, end - pos + 1);
        SWebViewerButtonConfig cfg;
        cfg.Id = WEBVIEWER_GUIDE_BASE_ID + (int)loaded.size();
        cfg.Name = ExtractString(obj, "name");
        cfg.IconPath = ExtractString(obj, "icon");
        cfg.Url = ExtractString(obj, "url");
        cfg.FrameWidth = ExtractInt(obj, "frameWidth", 900);
        cfg.FrameHeight = ExtractInt(obj, "frameHeight", 620);

        if(cfg.FrameWidth < 320) cfg.FrameWidth = 320;
        if(cfg.FrameHeight < 240) cfg.FrameHeight = 240;
        if(cfg.FrameWidth > 1200) cfg.FrameWidth = 1200;
        if(cfg.FrameHeight > 900) cfg.FrameHeight = 900;

        if(!cfg.Name.empty() && !cfg.IconPath.empty() && !cfg.Url.empty()) {
            cfg.WideName = NarrowToWide(cfg.Name);
            cfg.WideUrl = NarrowToWide(cfg.Url);
            loaded.push_back(cfg);
        }

        pos = text.find('{', end + 1);
    }

    if(loaded.empty())
        return false;

    g_webViewerButtons = loaded;
    return true;
}

bool CWebViewerConfig::Load() {
    if(g_webViewerRuntimeConfig)
        return !g_webViewerButtons.empty();

    if(g_webViewerLoaded)
        return !g_webViewerButtons.empty();

    if(LoadFromPath("clientlibrary\\config\\webviewer.json")) {
        g_webViewerLoaded = true;
        return true;
    }

    if(LoadFromPath("clientlibrary\\webviewer\\webviewer.json")) {
        g_webViewerLoaded = true;
        return true;
    }

    if(LoadFromPath("Media\\clientlibrary\\config\\webviewer.json")) {
        g_webViewerLoaded = true;
        return true;
    }

    if(LoadFromPath("Media\\clientlibrary\\webviewer\\webviewer.json")) {
        g_webViewerLoaded = true;
        return true;
    }

    return false;
}

bool CWebViewerConfig::Reload() {
    g_webViewerRuntimeConfig = false;
    g_webViewerLoaded = false;
    g_webViewerButtons.clear();
    return Load();
}

void CWebViewerConfig::SetRuntimeButtons(const std::vector<SWebViewerButtonConfig>& buttons) {
    g_webViewerButtons = buttons;
    g_webViewerLoaded = true;
    g_webViewerRuntimeConfig = true;
}

bool CWebViewerConfig::HasRuntimeButtons() {
    return g_webViewerRuntimeConfig && !g_webViewerButtons.empty();
}

int CWebViewerConfig::GetCount() {
    Load();
    return (int)g_webViewerButtons.size();
}

const SWebViewerButtonConfig* CWebViewerConfig::GetByIndex(int index) {
    Load();
    if(index < 0 || index >= (int)g_webViewerButtons.size())
        return NULL;
    return &g_webViewerButtons[index];
}

const SWebViewerButtonConfig* CWebViewerConfig::GetByGuideId(int guideId) {
    int index = guideId - WEBVIEWER_GUIDE_BASE_ID;
    return GetByIndex(index);
}

bool CWebViewerConfig::IsWebViewerGuideId(int guideId) {
    if(guideId < WEBVIEWER_GUIDE_BASE_ID)
        return false;
    if(guideId >= WEBVIEWER_GUIDE_BASE_ID + WEBVIEWER_MAX_BUTTONS)
        return false;
    return GetByGuideId(guideId) != NULL;
}
