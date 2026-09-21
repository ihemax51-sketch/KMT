#include "IFWnd.h"
#include "Game.h"
#include "TextStringManager.h"
#include <BSLib/Debug.h>

const wchar_t *KmtGetText(const wchar_t *key) {
    return key ? TSM_GETTEXTPTR(key) : L"";
}

GFX_IMPLEMENT_DYNAMIC_EXISTING(CIFWnd, 0x00EE9808)

GFX_BEGIN_MESSAGE_MAP(CIFWnd, CObjChild)
// ONG_COMMAND(2, On2)
ONG_CREATE()
ONG_MOVE()
GFX_END_MESSAGE_MAP()

// CIFWnd::CIFWnd(void) .text 00654520 00000205 00000024 00000000 R . . . . T .
CIFWnd::CIFWnd(void) {
    BS_DEBUG_LOW(">" __FUNCTION__);

    this->N00000688 = 0;
    this->N00000689 = 0;
    this->N000006A1 = 0;
    this->N00009C01 = 0;
    this->N00009C02 = 0;
    this->N00009C1B = 0;
    this->N0000061A = 0;
    this->N00000609 = 1;
    this->m_dwTextureBGColor = 0;
    this->m_dwTextureFGColor = 0;
    this->N00009BFD = 0;
    this->m_current_style = 0;

    CIFWnd::SetText(L"");

    this->m_FontTexture.SetColor(D3DCOLOR_RGBA(255, 255, 255, 255));
    this->N0000061B = 0;
    this->m_wstrTooltip = L"";
    this->N00000615 = 1.0;
    this->N00000608 = 0;
    this->N00000617 = 1.0;
    this->N000005A9 = 0;
    this->N00000618 = 0;
    this->N0000061D = 0.0;
    this->N0000061E = 0.0;
    this->N00000628 = 0.0;
    this->N0000062B = 0.0;
    this->N00000627 = 1.0;
    this->N00000629 = 1.0;
    this->N0000062A = 1.0;
    this->N0000062C = 1.0;
    this->sub_652B00();
    this->sub_653060();
    this->Set_N00000687(0);
    this->N00009C2C = 0;
    this->N00000688 = 0;
    this->N00000689 = 0;
    this->N0000068A = 0;
    this->N0000068B = 0;
    this->N0000068C = 0;
    this->N0000068D = 0;
    this->N0000068E = 0;
}

// CIFWnd::~CIFWnd(void) .text 00653720 00000120 00000024 00000000 R . . . . . .
CIFWnd::~CIFWnd(void) {
    // empty, everything should be auto generated ...
}

// CIFWnd::OnTimer(int) .text 006548E0 0000006B 00000004 00000004 R . . . . . .
void CIFWnd::OnTimer(int) {
    assert(FALSE);
}

// CIFWnd::OnCreate(void) .text 00537950 00000005 00000000 00000000 R . . . . . .
bool CIFWnd::OnCreate(long ln) {
    return true;
}

// CIFWnd::OnRelease(void) .text 004F4D30 00000003   R . . . . . .
bool CIFWnd::OnRelease() {
    return true;
}

// CIFWnd::Func_18(void) .text 006523D0 000000F9 00000008 00000004 R . . . . . .
bool CIFWnd::OnWndMessage(const Event3D *pEvent) {
    assert(FALSE);
    return false;
}

// CIFWnd::On3DEventMAYBE(void) .text 00652540 000000FE 0000000C 00000004 R . . . . . .
bool CIFWnd::OnMouseEvent(Event3D *pEvent) {
    BS_DEBUG_LOW(__FUNCTION__ " (%p)", pEvent);
    BS_DEBUG_LOW("> %d %8x %8x", pEvent->Msg, pEvent->lParam, pEvent->wParam);

    return reinterpret_cast<bool(__thiscall *)(CIFWnd *, Event3D *)>(0x00652540)(this, pEvent);
}

// CIFWnd::Func_20(void) .text 00652BC0 00000011   R . . . . . .
void CIFWnd::BringToFront() {
    reinterpret_cast<void(__thiscall *)(CIFWnd *)>(0x00652BC0)(this);
}

// CIFWnd::SetGWndSize(int,int) .text 00652970 00000024 00000004 00000008 R . . . . . .
void CIFWnd::SetGWndSize(int width, int height) {
    BS_DEBUG_LOW(__FUNCTION__ " (%d, %d)", width, height);
    reinterpret_cast<void(__thiscall *)(CIFWnd *, int, int)>(0x00652970)(this, width, height);
}

// CIFWnd::Func_22(void) .text 00652950 00000014 00000000 00000008 R . . . . . .
void CIFWnd::Func_22(int x, int y) {
    BS_DEBUG_LOW(__FUNCTION__ " (%d, %d)", x, y);
    SetGWndSize(x, y);
}

// CIFWnd::Func_37(void) .text 00652660 0000000A   R . . . . . .
void CIFWnd::OnGWndDimensionChange() {
    OnCIFReady();
}

// CIFWnd::CTextBoard_Func_7 .text 00652850 00000045 00000008 00000000 R . . . . . .
void CIFWnd::TB_Func_7() {
    assert(FALSE);
}

// CIFWnd::CTextBoard_Func_8 .text 00652670 0000006A 00000008 00000000 R . . . . . .
char CIFWnd::TB_Func_8() {
    return reinterpret_cast<char(__thiscall *)(CTextBoard *)>(0x00652670)(this);
}

// CIFWnd::CTextBoard_Func_11 .text 00654C30 000000D9 00000038 00000000 R . . . . . .
void CIFWnd::TB_Func_11() {
    reinterpret_cast<void(__thiscall *)(CTextBoard *)>(0x00654C30)(this);
}

// CIFWnd::Update(void) .text 006528A0 0000001C 00000004 00000000 R . . . . . .)
void CIFWnd::OnUpdate() {
    reinterpret_cast<void(__thiscall *)(CIFWnd *)>(0x006528A0)(this);
}

// CIFWnd::ShowGWnd(bool) .text 00652B70 00000048 00000004 00000004 R . . . . . .
void CIFWnd::ShowGWnd(bool bVisible) {
    reinterpret_cast<void(__thiscall *)(CIFWnd *, bool)>(0x00652B70)(this, bVisible);
}

// CIFWnd::OnWndMessage(void) .text 00652FB0 0000008D 00000000 00000004 R . . . . . .
void CIFWnd::OnGfxMessage(Event3D *pEvent) {
    reinterpret_cast<void(__thiscall *)(CIFWnd *, Event3D *)>(0x00652FB0)(this, pEvent);
}

// CIFWnd::RenderMyself(void) .text 006550C0 00000043 00000004 00000000 R . . . . . .
void CIFWnd::RenderMyself() {
    reinterpret_cast<void(__thiscall *)(CIFWnd *)>(0x06550C0)(this);
}

// CIFWnd::MoveGWnd(int,int) .text 006529A0 00000024 00000004 00000008 R . . . . T .
void CIFWnd::MoveGWnd(int x, int y) {
    BS_DEBUG_LOW(__FUNCTION__ " (%d, %d)", x, y);
    reinterpret_cast<void(__thiscall *)(CIFWnd *, int, int)>(0x006529A0)(this, x, y);
#if 0
    CGWnd::SetPosition(x, y);
    this->OnCIFReady();
#endif
}

// CIFWnd::MoveGWnd2(int,int) .text 00653040 00000012 00000000 00000008 R . . . . . .
// This is actually an overload of MoveGWnd, but we will keep the name for now
// to avoid distress with the vft order.
// Proof for param being wnd_pos can be found here: 0x007AACB5
// Call to MoveGWnd2 will reuse the copy returned from GetPos for x and y.
void CIFWnd::MoveGWnd2(wnd_pos pos) {
    CIFWnd::MoveGWnd(pos.x, pos.y);
}

// CIFWnd::Func_40(void) .text 004F4D40 00000001   R . . . . . .
void CIFWnd::Func_40() {
    // Empty in original
}

void CIFWnd::OnCIFReady() {
    reinterpret_cast<void(__thiscall *)(CIFWnd *)>(0x006529D0)(this);
}

// CIFWnd::IsInside(int,int) .text 00652740 00000046 00000010 00000008 R . . . . T .
bool CIFWnd::IsInside(int x, int y) {
    assert(FALSE);
    return false;
}

// CIFWnd::SetText(void) .text 00653AC0 00000062 00000004 00000004 R . . . . T .
bool CIFWnd::SetText(const wchar_t *src) {
    return reinterpret_cast<bool(__thiscall *)(CIFWnd *, const wchar_t *)>(0x00653AC0)(this, src);
}

// CIFWnd::GetText(void) .text 00653350 00000017   R . . . . . .
const wchar_t *CIFWnd::GetText() {
    return m_innerText.c_str();
}
std::n_wstring CIFWnd::GetNText()
{
    return m_innerText;
}
// CIFWnd::Func_42(void) .text 00652AF0 00000001   R . . . . . .
void CIFWnd::Func_42() {
    // Empty in original
}

// CIFWnd::OnCloseWnd(void) .text 00652C00 000000CF 00000004 00000000 R . . . . . .
undefined1 CIFWnd::OnCloseWnd() {
    return reinterpret_cast<undefined1(__thiscall *)(CIFWnd *)>(0x00652c00)(this);
}

// CIFWnd::Func_47(int) .text 004F4D50 00000003 00000000 00000000 R . . . . . .
void CIFWnd::Func_47(int) {
    // Empty in original
}

// CIFWnd::Func_48(void) .text 006528C0 0000008D 0000001C 00000000 R . . . . . .
void CIFWnd::EnableTooltip() {
    assert(FALSE);
}

void CIFWnd::RenderTextureColor(DWORD dwColor) {
    reinterpret_cast<void(__thiscall *)(CIFWnd *, DWORD)>(0x00654d10)(this, dwColor);
}

void CIFWnd::SetTextureBkColor(D3DCOLOR color) {
    m_dwTextureBGColor = color;
}

D3DCOLOR CIFWnd::GetTextureBkColor() const {
    return m_dwTextureBGColor;
}

void CIFWnd::SetTextureFgColor(D3DCOLOR color) {
    m_dwTextureFGColor = color;
}

D3DCOLOR CIFWnd::GetTextureFgColor() const {
    return m_dwTextureFGColor;
}

// CIFWnd::On4001(int,int) .text 00652390 00000012 00000000 00000000 R . . . . . .
int CIFWnd::OnCreatedInstance(UINT lParam, UINT wParam) {
    return reinterpret_cast<int(__thiscall *)(CIFWnd *, UINT, UINT)>(0x00652390)(this, lParam, wParam);
}

// CIFWnd::On4006(int,int) .text 006523B0 00000012 00000000 00000000 R . . . . . .
int CIFWnd::OnWindowPosChanged(UINT lParam, UINT wParam) {
    return reinterpret_cast<int(__thiscall *)(CIFWnd *, UINT, UINT)>(0x006523B0)(this, lParam, wParam);
}

// CIFWnd::sub_652B00(void) .text 00652B00 00000045   R . . . . . .
void CIFWnd::sub_652B00() {
    this->N0000066B = 1.0f;
    this->N0000066C = 0.1f;
    this->N00000672 = 0.1f;
    this->N00000678 = 0.1f;
    this->N0000067E = 0.1f;
    this->N00000684 = 0.1f;
    this->N00000671 = 1.0f;
    this->N00000677 = 1.0f;
    this->N0000067D = 1.0f;
    this->N00000683 = 1.0f;
}

// CIFWnd::sub_653060(void) .text 00653060 000000DD 00000004 00000000 R . . . . . .
void CIFWnd::sub_653060() {
    this->N0000062F = 1.0f;
    this->N00009C60 = 0.1f;
    this->N00000637 = 0.1f;
    this->N00009C70 = 0.1f;
    this->N0000065D = 0.1f;
    this->N00009C80 = 0.1f;
    this->N00000635 = 1.0f;
    this->N0000063A = 1.0f;
    this->N00009C5A = 1.0f;
    this->N00000662 = 1.0f;
    this->N00009C61 = this->N0000061D;
    this->N00000632 = this->N0000061E;
    this->N00009C69 = this->N00000627;
    this->N00009C6A = this->N00000628;
    this->N00009C71 = this->N00000629;
    this->N0000063B = this->N0000062A;
    this->N00009C79 = this->N0000062B;
    this->N00009C7A = this->N0000062C;
    this->N00009C81 = this->N0000061D;
    this->N00000668 = this->N0000061E;
}

void CIFWnd::Set_N00000687(char a2) {
    N00000687 = a2;
}

void CIFWnd::SetSomeRect(const RECT &rect) {
    m_someRECT = rect;
}

void CIFWnd::GetSomeRECT(RECT &rect) {
    reinterpret_cast<void(__thiscall *)(CIFWnd *, RECT *)>(0x00652800)(this, &rect);
}

void CIFWnd::SetTooltip(const std::n_wstring &wstrText) {
    m_wstrTooltip = wstrText;

    if (IsParentOf(g_CurrentIfUnderCursor) && IsStyleThingy(TOOLTIP))
        EnableTooltip();
}

void CIFWnd::SetStyleThingy(StyleOptions option) {
    reinterpret_cast<void(__thiscall *)(CIFWnd *, StyleOptions)>(0x00652d20)(this, option);
}

bool CIFWnd::IsStyleThingy(StyleOptions option) const {
    return ((option & m_current_style) != FALSE);
}

void CIFWnd::sub_6526E0(char a0, unsigned char opacity, float time, float a4, char a5) {
    reinterpret_cast<void(__thiscall *)(CIFWnd *, char, unsigned char, float, float, char)>(0x6526E0)(this, a0,
                                                                                                      opacity, time,
                                                                                                      a4, a5);
}
void CIFWnd::FUN_00636330(){
    reinterpret_cast<void(__thiscall *)(CIFWnd *)>(0x00636330)(this);
}
