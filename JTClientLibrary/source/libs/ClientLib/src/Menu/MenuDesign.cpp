#include "MenuDesign.h"

#include "IFMenu.h"
#include "../IFButton.h"

#include <GFXFM/IFileManager.h>

#include <Windows.h>

#include <cctype>
#include <climits>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <map>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

const char *kDesignPath = "clientlibrary\\config\\menu_design.json";
const int kMaxDesignSize = 1024 * 1024;

struct JsonValue {
    enum Type { Null, Boolean, Number, String, Object, Array } type;
    bool boolean;
    double number;
    std::string string;
    std::map<std::string, JsonValue> object;
    std::vector<JsonValue> array;

    JsonValue() : type(Null), boolean(false), number(0.0) {}

    const JsonValue *Find(const char *key) const {
        if (type != Object)
            return NULL;
        const std::map<std::string, JsonValue>::const_iterator it = object.find(key);
        return it == object.end() ? NULL : &it->second;
    }
};

class JsonParser {
public:
    explicit JsonParser(const std::string &text) : m_text(text), m_pos(0), m_depth(0) {}

    JsonValue Parse() {
        SkipSpace();
        JsonValue value = ParseValue();
        SkipSpace();
        if (m_pos != m_text.size())
            Fail("unexpected trailing data");
        return value;
    }

private:
    JsonValue ParseValue() {
        if (++m_depth > 32)
            Fail("maximum nesting exceeded");

        JsonValue value;
        const char c = Peek();
        if (c == '{') value = ParseObject();
        else if (c == '[') value = ParseArray();
        else if (c == '"') {
            value.type = JsonValue::String;
            value.string = ParseString();
        } else if (StartsWith("true")) {
            m_pos += 4;
            value.type = JsonValue::Boolean;
            value.boolean = true;
        } else if (StartsWith("false")) {
            m_pos += 5;
            value.type = JsonValue::Boolean;
        } else if (StartsWith("null")) {
            m_pos += 4;
        } else {
            value.type = JsonValue::Number;
            value.number = ParseNumber();
        }
        --m_depth;
        return value;
    }

    JsonValue ParseObject() {
        JsonValue value;
        value.type = JsonValue::Object;
        Expect('{');
        SkipSpace();
        if (Consume('}'))
            return value;

        for (;;) {
            if (Peek() != '"')
                Fail("object key must be a string");
            const std::string key = ParseString();
            SkipSpace();
            Expect(':');
            SkipSpace();
            value.object[key] = ParseValue();
            SkipSpace();
            if (Consume('}'))
                return value;
            Expect(',');
            SkipSpace();
        }
    }

    JsonValue ParseArray() {
        JsonValue value;
        value.type = JsonValue::Array;
        Expect('[');
        SkipSpace();
        if (Consume(']'))
            return value;
        for (;;) {
            value.array.push_back(ParseValue());
            SkipSpace();
            if (Consume(']'))
                return value;
            Expect(',');
            SkipSpace();
        }
    }

    std::string ParseString() {
        Expect('"');
        std::string result;
        while (m_pos < m_text.size()) {
            char c = m_text[m_pos++];
            if (c == '"')
                return result;
            if (static_cast<unsigned char>(c) < 0x20)
                Fail("control character in string");
            if (c != '\\') {
                result.push_back(c);
                continue;
            }
            if (m_pos >= m_text.size())
                Fail("unfinished escape");
            c = m_text[m_pos++];
            switch (c) {
                case '"': result.push_back('"'); break;
                case '\\': result.push_back('\\'); break;
                case '/': result.push_back('/'); break;
                case 'b': result.push_back('\b'); break;
                case 'f': result.push_back('\f'); break;
                case 'n': result.push_back('\n'); break;
                case 'r': result.push_back('\r'); break;
                case 't': result.push_back('\t'); break;
                case 'u': AppendUtf8(ParseHex(), result); break;
                default: Fail("invalid escape");
            }
        }
        Fail("unterminated string");
        return result;
    }

    unsigned int ParseHex() {
        if (m_pos + 4 > m_text.size())
            Fail("incomplete unicode escape");
        unsigned int value = 0;
        for (int i = 0; i < 4; ++i) {
            const char c = m_text[m_pos++];
            value <<= 4;
            if (c >= '0' && c <= '9') value += c - '0';
            else if (c >= 'a' && c <= 'f') value += c - 'a' + 10;
            else if (c >= 'A' && c <= 'F') value += c - 'A' + 10;
            else Fail("invalid unicode escape");
        }
        return value;
    }

    static void AppendUtf8(unsigned int value, std::string &out) {
        if (value <= 0x7F) {
            out.push_back(static_cast<char>(value));
        } else if (value <= 0x7FF) {
            out.push_back(static_cast<char>(0xC0 | (value >> 6)));
            out.push_back(static_cast<char>(0x80 | (value & 0x3F)));
        } else {
            out.push_back(static_cast<char>(0xE0 | (value >> 12)));
            out.push_back(static_cast<char>(0x80 | ((value >> 6) & 0x3F)));
            out.push_back(static_cast<char>(0x80 | (value & 0x3F)));
        }
    }

    double ParseNumber() {
        const char *start = m_text.c_str() + m_pos;
        char *end = NULL;
        const double value = std::strtod(start, &end);
        if (end == start)
            Fail("expected value");
        m_pos += static_cast<size_t>(end - start);
        return value;
    }

    void SkipSpace() {
        while (m_pos < m_text.size() &&
               std::isspace(static_cast<unsigned char>(m_text[m_pos])))
            ++m_pos;
    }

    char Peek() const { return m_pos < m_text.size() ? m_text[m_pos] : '\0'; }

    bool Consume(char c) {
        if (Peek() != c)
            return false;
        ++m_pos;
        return true;
    }

    void Expect(char c) {
        if (!Consume(c)) {
            std::string message("expected '");
            message += c;
            message += "'";
            Fail(message.c_str());
        }
    }

    bool StartsWith(const char *text) const {
        const size_t length = std::strlen(text);
        return m_pos + length <= m_text.size() &&
               m_text.compare(m_pos, length, text) == 0;
    }

    void Fail(const char *message) const {
        char details[256];
        std::sprintf(details, "%s at byte %u", message, static_cast<unsigned int>(m_pos));
        throw std::runtime_error(details);
    }

    const std::string &m_text;
    size_t m_pos;
    int m_depth;
};

bool ReadMediaFile(std::string &content) {
    IFileManager *fileManager = g_pCFileManager;
    if (fileManager == NULL)
        return false;

    const int handle = fileManager->Open(kDesignPath, 0, 0);
    if (handle == -1)
        return false;

    const int size = fileManager->GetFileSize(handle, NULL);
    if (size <= 0 || size > kMaxDesignSize) {
        fileManager->Close(handle);
        throw std::runtime_error("menu design is empty or exceeds 1 MiB");
    }

    content.assign(static_cast<size_t>(size), '\0');
    unsigned long bytesRead = 0;
    const int result = fileManager->Read(handle, &content[0], size, &bytesRead);
    fileManager->Close(handle);
    if (result == 0 || bytesRead != static_cast<unsigned long>(size))
        throw std::runtime_error("could not read the complete menu design");

    if (content.size() >= 3 &&
        static_cast<unsigned char>(content[0]) == 0xEF &&
        static_cast<unsigned char>(content[1]) == 0xBB &&
        static_cast<unsigned char>(content[2]) == 0xBF)
        content.erase(0, 3);
    return true;
}

bool GetBool(const JsonValue &object, const char *key, bool fallback) {
    const JsonValue *value = object.Find(key);
    return value != NULL && value->type == JsonValue::Boolean ? value->boolean : fallback;
}

bool TryGetBool(const JsonValue &object, const char *key, bool &result) {
    const JsonValue *value = object.Find(key);
    if (value == NULL || value->type != JsonValue::Boolean)
        return false;
    result = value->boolean;
    return true;
}

bool GetInt(const JsonValue &object, const char *key, int &result) {
    const JsonValue *value = object.Find(key);
    if (value == NULL || value->type != JsonValue::Number)
        return false;
    if (value->number != value->number ||
        value->number > static_cast<double>(INT_MAX) ||
        value->number < static_cast<double>(INT_MIN))
        return false;
    result = static_cast<int>(value->number);
    return true;
}

bool GetString(const JsonValue &object, const char *key, std::string &result) {
    const JsonValue *value = object.Find(key);
    if (value == NULL || value->type != JsonValue::String)
        return false;
    result = value->string;
    return true;
}

bool SafeTexturePath(const std::string &path) {
    return !path.empty() && path.size() <= 512 &&
           path.find(':') == std::string::npos &&
           path.find("..") == std::string::npos &&
           path[0] != '\\' && path[0] != '/';
}

std::wstring Utf8ToWide(const std::string &text) {
    if (text.empty())
        return std::wstring();
    const int length = MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS,
                                            text.c_str(), static_cast<int>(text.size()),
                                            NULL, 0);
    if (length <= 0)
        throw std::runtime_error("text contains invalid UTF-8");
    std::wstring result(static_cast<size_t>(length), L'\0');
    MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS,
                        text.c_str(), static_cast<int>(text.size()),
                        &result[0], length);
    return result;
}

bool ParseColor(const std::string &text, DWORD &color) {
    std::string value = text;
    if (!value.empty() && value[0] == '#')
        value.erase(0, 1);
    if (value.size() != 6 && value.size() != 8)
        return false;
    char *end = NULL;
    const unsigned long parsed = std::strtoul(value.c_str(), &end, 16);
    if (end == NULL || *end != '\0')
        return false;
    color = value.size() == 6 ? 0xFF000000 | parsed : parsed;
    return true;
}

void ApplyCommon(CIFWnd &control, const JsonValue &definition, int originX, int originY) {
    int x = control.GetPos().x - originX;
    int y = control.GetPos().y - originY;
    int width = control.GetSize().width;
    int height = control.GetSize().height;
    const bool hasX = GetInt(definition, "x", x);
    const bool hasY = GetInt(definition, "y", y);
    const bool hasWidth = GetInt(definition, "width", width);
    const bool hasHeight = GetInt(definition, "height", height);

    if (hasX || hasY)
        control.MoveGWnd(originX + x, originY + y);
    if ((hasWidth || hasHeight) && width > 0 && height > 0)
        control.SetGWndSize(width, height);

    std::string texture;
    if (GetString(definition, "texture", texture) && SafeTexturePath(texture))
        control.TB_Func_13(texture.c_str(), 0, 0);

    std::string text;
    if (GetString(definition, "text", text)) {
        const std::wstring wide = Utf8ToWide(text);
        control.SetText(wide.c_str());
    }

    std::string tooltip;
    if (GetString(definition, "tooltip", tooltip)) {
        const std::wstring wide = Utf8ToWide(tooltip);
        control.SetStyleThingy(TOOLTIP);
        control.SetTooltip(std::n_wstring(wide.c_str()));
    }

    std::string fontColor;
    DWORD color = 0;
    if (GetString(definition, "font_color", fontColor) && ParseColor(fontColor, color))
        control.m_FontTexture.SetColor(color);

    bool visible = true;
    if (TryGetBool(definition, "visible", visible))
        control.ShowGWnd(visible);
}

bool IsMenuButton(int id) {
    return id >= 12 && id <= 21;
}

void ApplyButtonTextures(CIFWnd &control, int id, const JsonValue &definition) {
    if (!IsMenuButton(id))
        return;

    CIFButton &button = reinterpret_cast<CIFButton &>(control);
    std::string texture;
    if (GetString(definition, "pressed_texture", texture) && SafeTexturePath(texture))
        button.FUN_00656590(std::n_string(texture.c_str()));
    if (GetString(definition, "disabled_texture", texture) && SafeTexturePath(texture))
        button.FUN_00656640(std::n_string(texture.c_str()));
}

void Debug(const std::string &message) {
    OutputDebugStringA(("[MenuDesign] " + message + "\n").c_str());
}

} // namespace

namespace MenuDesign {

bool Apply(CIFMenu &menu) {
    try {
        std::string content;
        if (!ReadMediaFile(content))
            return false;

        const JsonValue root = JsonParser(content).Parse();
        if (root.type != JsonValue::Object)
            throw std::runtime_error("root must be an object");
        if (!GetBool(root, "enabled", true))
            return false;

        const JsonValue *menuDefinition = root.Find("menu");
        int requestedX = 0;
        int requestedY = 0;
        bool hasRequestedX = false;
        bool hasRequestedY = false;
        if (menuDefinition != NULL && menuDefinition->type == JsonValue::Object) {
            hasRequestedX = GetInt(*menuDefinition, "x", requestedX);
            hasRequestedY = GetInt(*menuDefinition, "y", requestedY);
            ApplyCommon(menu, *menuDefinition, 0, 0);
        }

        // Re-anchor after a possible width change. Explicit x/y override the
        // default right-side placement.
        menu.UpdateMenuSize();
        if (hasRequestedX || hasRequestedY)
            menu.MoveGWnd(hasRequestedX ? requestedX : menu.GetPos().x,
                          hasRequestedY ? requestedY : menu.GetPos().y);

        const JsonValue *elements = root.Find("elements");
        if (elements != NULL) {
            if (elements->type != JsonValue::Array)
                throw std::runtime_error("elements must be an array");

            for (std::vector<JsonValue>::const_iterator it = elements->array.begin();
                 it != elements->array.end(); ++it) {
                if (it->type != JsonValue::Object)
                    continue;
                int id = 0;
                if (!GetInt(*it, "id", id) || id <= 0)
                    continue;
                CIFWnd *control = menu.GetMenuResource(id);
                if (control != NULL) {
                    ApplyCommon(*control, *it, menu.GetPos().x, menu.GetPos().y);
                    ApplyButtonTextures(*control, id, *it);
                }
            }
        }

        Debug("menu_design.json applied successfully");
        return true;
    } catch (const std::exception &error) {
        Debug(std::string("design ignored: ") + error.what());
        return false;
    }
}

bool ApplySafely(CIFMenu &menu) {
    __try {
        return Apply(menu);
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        OutputDebugStringA("[MenuDesign] design ignored: structured exception while reading or applying menu design\n");
        return false;
    }
}

} // namespace MenuDesign
