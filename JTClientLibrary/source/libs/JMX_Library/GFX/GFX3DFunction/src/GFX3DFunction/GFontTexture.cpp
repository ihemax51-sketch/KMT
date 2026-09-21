#include "GFontTexture.h"
#include "../../../../../ClientLib/src/CustomData/CustomDataManager.h"
#include "RStateMgr.h"
#include "../../../../../SimpleViewer/include/SimpleViewer/VBDynamic.h"
#include <BSLib/Debug.h>
#include <cassert>
#include <BSLib/multibyte.h>
#include <cmath>

// CGFontTexture::CGFontTexture(void) .text 008B4F30 00000061 0000001C 00000000 R . . . . . .
CGFontTexture::CGFontTexture(void)
{
    //assert(FALSE);
    BS_DEBUG_LOW("> " __FUNCTION__);
    reinterpret_cast<void (__thiscall*)(CGFontTexture*)>(0x008B4F30)(this);
}

// CGFontTexture::CGFontTexture(int) .text 008B4FA0 00000067 0000001C 00000004 R . . . . . .
CGFontTexture::CGFontTexture(int a2)
{
    BS_DEBUG_LOW("> " __FUNCTION__);
    assert(FALSE);
}

// CGFontTexture::~CGFontTexture(void) .text 008B3CD0 0000006B 00000018 00000000 R . . . . . .
CGFontTexture::~CGFontTexture(void)
{
    BS_DEBUG_LOW("> " __FUNCTION__);
    // reinterpret_cast<void(__thiscall*)(CGFontTexture*)>(0x008B4FA0)(this);
}

// Non-client function
int CGFontTexture::get_N000007BC()
{
    return N000007BC;
}

// CGFontTexture::SetSomething(int) .text 008B2B50 0000000C 00000000 00000004 R . . . . . .
bool CGFontTexture::SetSomething(void* a2)
{
    // assert(FALSE);
    return reinterpret_cast<bool(__thiscall*)(CGFontTexture*,void*)>(0x008B2B50)(this, a2);
}

// CGFontTexture__SetColor .text 008B2B70 00000016 00000000 00000004 R . . . . . .
void CGFontTexture::SetColor(D3DCOLOR color)
{
    reinterpret_cast<void(__thiscall*)(CGFontTexture*,D3DCOLOR)>(0x008B2B70)(this, color);
}

// CGFontTexture::sub_8B2BA0(int,int) .text 008B2BA0 00000061 00000004 00000008 R . . . . . .
void CGFontTexture::sub_8B2BA0(int a1, D3DCOLOR color)
{
    reinterpret_cast<void(__thiscall*)(CGFontTexture*, int ,D3DCOLOR)>(0x008B2BA0)(this, a1, color);
}

// CGFontTexture::sub_8B3B60(std::n_wstring const *) .text 008B3B60 00000026 00000000 00000004 R . . . . . .
void CGFontTexture::sub_8B3B60(std::n_wstring* str)
{
    // assert(FALSE);
    reinterpret_cast<void(__thiscall*)(CGFontTexture*,std::n_wstring*)>(0x8B3B60)(this, str);
}

// CGFontTexture::sub_8B37A0(void) .text 008B37A0 0000007D 00000008 00000000 R . . . . . .
void CGFontTexture::sub_8B37A0()
{
    // assert(FALSE);
}

void CGFontTexture::sub_8B4400(int a1, int a2) {
    reinterpret_cast<void (__thiscall *)(CGFontTexture *, int, int)>(0x008B4400)(this, a1, a2);
}

void CGFontTexture::sub_8B4750(int a2) {
    reinterpret_cast<void (__thiscall *)(CGFontTexture *, int)>(0x008B4750)(this, a2);
}

std::n_wstring *CGFontTexture::GetText(std::n_wstring *str) const {
    return reinterpret_cast<std::n_wstring *(__thiscall *)(const CGFontTexture *, std::n_wstring *str)>(0x008B3C10)(
            this, str);
}

void CGFontTexture::FUN_008b2d30(float*  param_1, D3DVECTOR * dataOut) {
    reinterpret_cast<void (__thiscall *)(CGFontTexture *, float*, D3DVECTOR *)>(0x008b2d30)(this, param_1, dataOut);
}

CGFontTexture::TextureDimensions CGFontTexture::GetDimensions() const{
    return m_dimensions;
}

void CGFontTexture::FUN_008b3bd0(std::n_wstring *param_1) {
    return reinterpret_cast<void (__thiscall *)(CGFontTexture *, std::n_wstring *)>(0x008b3bd0)(this, param_1);
}

void CGFontTexture::FUN_008b4d90(float* param_1, undefined4 param_2) {
    return reinterpret_cast<void (__thiscall *)(CGFontTexture *, float*, undefined4)>(0x008b4d90)(this, param_1, param_2);
}

int CGFontTexture::GetDis(std::n_wstring str, int index) {

    std::n_wstring tempText = str.substr(0, index);

    CGFontTexture tempTexture = CGFontTexture();
    tempTexture.SetSomething(m_CustomDataManager->font);
    tempTexture.sub_8B3B60(&tempText);

    return tempTexture.GetDimensions().width;
}

int CGFontTexture::GetTextWidth(std::n_wstring str) {
    CGFontTexture tempTexture = CGFontTexture();

    tempTexture.SetSomething(m_CustomDataManager->font);

    tempTexture.sub_8B3B60(&str);

    return tempTexture.GetDimensions().width;
}
void CGFontTexture::HandelEmoji(int x, int y, byte type) {

        std::n_wstring text;
        this->GetText(&text);

        if (text.length() > 1) {
            std::map<std::string, IDirect3DBaseTexture9 *>::const_iterator it = m_CustomDataManager->emojiListData.begin();

            // Counter for the number of emojis rendered
            int emojiCount = 0;
            const int maxEmojis = 5;// Limit to 5 emojis per message

            for (; it != m_CustomDataManager->emojiListData.end(); ++it) {
                std::n_wstring wstr_key = TO_NWSTRING(it->first);
                size_t found_pos = text.rfind(wstr_key.c_str());
                if (found_pos != std::string::npos) {
                    if (text.rfind(TO_WSTRING(it->first).c_str()) != std::string::npos) {
                        for (int j = 0; j < text.length(); j++) {
                            if (text.compare(j, wstr_key.length(), wstr_key) == 0) {
                                if (text.substr(j, TO_WSTRING(it->first).length()) == TO_WSTRING(it->first).c_str()) {
                                    if (emojiCount >= maxEmojis) {
                                        return;// Stop rendering when max emojis are reached
                                    }
                                 //   int width = getWidth(str, pList, it->first,j);

                                    int textOffset = GetDis(text, j) - 1;
                                    int textWidth = GetTextWidth(wstr_key.c_str()) + 2;
                                    float zValue = (type == 0) ? 1.0f : 0.9f;
                                    int textOffsetY = 0;

                                    if (type == 1 && textOffset > -1) {
                                        int linesToMove = static_cast<int>(ceil(float(textOffset) / 200.0)) - 1;
                                        textOffset -= linesToMove * 198;
                                        textOffsetY = 14 * linesToMove;
                                    }

                                    D3DVECTOR dataOut[8] = {
                                            {x + textOffset, y + textOffsetY, zValue},
                                            {0.1f, 0.0f, 0.0f},
                                            {x + textOffset + textWidth, y + textOffsetY, zValue},
                                            {0.1f, 1.0f, 0.0f},
                                            {x + textOffset + textWidth, y + textOffsetY + 16.0f, zValue},
                                            {0.1f, 1.0f, 1.0f},
                                            {x + textOffset, y + textOffsetY + 16.0f, zValue},
                                            {0.1f, 0.0f, 1.0f}};


                                     g_RStateMgr.SetTextureForStage(0, it->second);
                                      int local_158 = 0;
                                      if (g_pDynamicVertexBuffer->IVBDynamic_Func_6(dataOut, 0x60, &local_158) != 0) {
                                          IDirect3DVertexBuffer9 *iVar5 = g_pDynamicVertexBuffer->IVBDynamic_Func_5();
                                          g_RStateMgr.m_pDevice->SetStreamSource(0, iVar5, 0, 0x18);
                                          g_RStateMgr.FUN_00470060(6, local_158, 2);
                                      }

                                    // Increment the emoji count
                                    emojiCount++;
                                }
                            }
                        }
                    }
                }
            }
        }

}

int CGFontTexture::Render_0(int x, int y, byte alpha) {
    int result = reinterpret_cast<int(__thiscall *)(CGFontTexture *, int, int, byte)>(0x008B48B0)(this, x, y, alpha);

    // printf("Handling Function 0 On Emoji");

    HandelEmoji(x, y, 0);

    return result;
}

int CGFontTexture::Render_1(int x, int y, byte alpha) {
    int result = reinterpret_cast<int(__thiscall *)(CGFontTexture *, int, int, byte)>(0x008B48B0)(this, x, y, alpha);

    // printf("Handling Function 1 On Emoji");
    HandelEmoji(x, y, 1);

    return result;
}