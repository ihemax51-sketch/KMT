#include <GFX3DFunction/RStateMgr.h>
#include <CustomData/CustomDataManager.h>
#include <CustomData/CustomCICPlayer.h>
#include <CustomInterface/IFTradeCaptchaWnd.h>
#include <CustomInterface/IFLoginRegisterWnd.h>
#include <SecondPW/IFSecondaryPassword.h>
#include <GlobalItemLinking/GlobalItemLinking.h>
#include "IFEdit.h"
#include "ICPlayer.h"

GFX_IMPLEMENT_DYNAMIC_EXISTING(CIFEdit, 0x00ee9540)

bool CIFEdit::HasFocus() const {
    // if our editbox window got focus and our interface too
    return ((GetFocus() == m_hEditBoxWnd) && (g_pUnderFocusCtrl == this));
}

void CIFEdit::AddValue_404(undefined4 value) {
    reinterpret_cast<void (__thiscall *)(CIFEdit *, undefined4)>(0x00635150)(this, value);
}

void CIFEdit::SetValue_404(undefined4 value) {
    reinterpret_cast<void (__thiscall *)(CIFEdit *, undefined4)>(0x00635070)(this, value);
}

undefined4 CIFEdit::GetValue_404() const {
    return reinterpret_cast<undefined4(__thiscall *)(const CIFEdit *)>(0x00635060)(this);
}

bool CIFEdit::IsSetValue_404(undefined4 value) const {
    return reinterpret_cast<bool (__thiscall *)(const CIFEdit *, undefined4)>(0x00635170)(this, value);
}

void CIFEdit::SetMaxLength(int length) {
    reinterpret_cast<void (__thiscall *)(CIFEdit *, int)>(0x00634f80)(this, length);
}

void CIFEdit::SetTextmode(undefined4 mode) {
    reinterpret_cast<void (__thiscall *)(CIFEdit *, undefined4)>(0x006351b0)(this, mode);
}

const std::n_wstring &CIFEdit::GetCurrentText() const {
    return m_texturestr_font;
}


unsigned int CIFEdit::GetAreaSize() {
    return this->m_TextAreaSize;
}

unsigned int CIFEdit::GetCurrentIndex() {
    return this->m_CurrentIndex;
}

void CIFEdit::SetCurrentIndex(unsigned int Index) {
    this->m_CurrentIndex = Index;
}
void CIFEdit::SetTextAreaSize(unsigned int Size) {
    this->m_TextAreaSize = Size;
}

#include "PSTitle.h"
#include "IFWholeChat.h"
#include "GInterface.h"
void CIFEdit::Func_49(const wchar_t *text)
{
    reinterpret_cast<int(__thiscall *)(CIFEdit *, const wchar_t*)>(0x0064d810)(this, text);
}
void CIFEdit::OnCharIMPLSelf(HWND hwnd, UINT msg, LPARAM lParam, WPARAM wParam)
{
    this->SetFocus_MAYBE();
    reinterpret_cast<int(__thiscall *)(CIFEdit *, HWND, UINT, LPARAM, WPARAM)>(0x00636cd0)(this, hwnd, msg, lParam, wParam);
}
int CIFEdit::OnCharIMPL(HWND hwnd, int a3, int a4, int a5)
{
    if (this->GetParentControl()->IsSame(
            GFX_RUNTIME_CLASS(CIFLoginRegisterWnd)))
    {
        CIFLoginRegisterWnd* registerWindow =
            (CIFLoginRegisterWnd*)this->GetParentControl();
        const WPARAM key = (WPARAM)a4;

        if (a3 == WM_KEYDOWN && key == VK_TAB)
        {
            registerWindow->FocusNextField(
                this,
                (GetKeyState(VK_SHIFT) & 0x8000) != 0);
            return 0;
        }

        if (a3 == WM_CHAR && key == VK_TAB)
            return 0;

        if (registerWindow->HandlePasswordInput(
                this,
                (UINT)a3,
                key))
            return 0;

        return reinterpret_cast<int(__thiscall *)(
            CIFEdit *,
            HWND,
            int,
            int,
            int)>(0x00636cd0)(this, hwnd, a3, a4, a5);
    }

    if (this->GetParentControl()->IsSame(GFX_RUNTIME_CLASS(CIFTradeCaptchaWnd)))
    {
        CIFTradeCaptchaWnd* tradeCaptcha = (CIFTradeCaptchaWnd*)this->GetParentControl();
        const WPARAM key = (WPARAM)a4;
        if (a3 == WM_KEYDOWN && key == VK_RETURN)
        {
            tradeCaptcha->SubmitFromKeyboard();
            return 0;
        }
        if (a3 == WM_CHAR)
        {
            if (key == VK_RETURN)
                return 0;
            if (key != VK_BACK && (key < L'0' || key > L'9'))
                return 0;
        }

        return reinterpret_cast<int(__thiscall *)(CIFEdit *, HWND, int, int, int)>(0x00636cd0)(this, hwnd, a3, a4, a5);
    }

    if(this->GetParentControl()->IsSame(GFX_RUNTIME_CLASS(CIFWholeChat)))
    {
        CIFEdit* pCIFEdit = this;
        CIFWholeChat *pWholeChat = (CIFWholeChat *)g_pCGInterface->m_IRM.GetResObj(56, 1);
        if (pCIFEdit && !g_global->m_CurrentLinkedItemName.empty()) {
            if (pWholeChat->m_pEdit != NULL && pWholeChat->m_pEdit == pCIFEdit)// if its equals to current global window
            {
                short backSpaceState = GetKeyState(VK_BACK);
                short deleteState = GetKeyState(VK_DELETE);
                short leftArrowKeyState = GetKeyState(VK_LEFT);
                short rightArrowKeyState = GetKeyState(VK_RIGHT);

                int CurrIndex = pCIFEdit->GetCurrentIndex();
                std::wstring CurrText = std::wstring(pCIFEdit->GetText());
                if (!CurrText.empty()) {
                    int begin = CurrText.find(L"͟<");
                    if (begin != std::wstring::npos)
                        g_global->m_LinkedGlobalItemBegin = begin;
                    int end = CurrText.find(L"͟>");
                    if (end != std::wstring::npos)
                        g_global->m_LinkedGlobalItemEnd = end + 2;


                    if (pCIFEdit->GetAreaSize() == 100 && g_global->m_LinkedGlobalItemEnd != 0)
                        pCIFEdit->SetTextAreaSize(100 + g_global->m_CurrentLinkedItemName.length());
                    if (pCIFEdit->GetAreaSize() != 100 && g_global->m_LinkedGlobalItemEnd == 0)
                        pCIFEdit->SetTextAreaSize(100);
                }
                //if left
                if ((leftArrowKeyState == -127 || leftArrowKeyState == -128) && CurrIndex - 1 <= g_global->m_LinkedGlobalItemEnd && CurrIndex >= g_global->m_LinkedGlobalItemBegin) {
                    //printf("left called %d\n", leftArrowKeyState);
                    pCIFEdit->SetCurrentIndex(g_global->m_LinkedGlobalItemBegin);
                    return 0;
                }
                //if right
                if ((rightArrowKeyState == -127 || rightArrowKeyState == -128) && CurrIndex + 1 >= g_global->m_LinkedGlobalItemBegin && CurrIndex <= g_global->m_LinkedGlobalItemEnd) {
                    //printf("right called %d\n", rightArrowKeyState);
                    pCIFEdit->SetCurrentIndex(g_global->m_LinkedGlobalItemEnd);
                    return 0;
                }

                //printf("ChatIndex = %d - Size = %d\n", pCIFEdit->GetCurrentIndex(), pCIFEdit->GetAreaSize());


                if ((backSpaceState == -127 || backSpaceState == -128) && (CurrIndex >= g_global->m_LinkedGlobalItemBegin && CurrIndex - 1 <= g_global->m_LinkedGlobalItemEnd)) {
                    std::wstring new_str = CurrText.c_str();
                    int findedstring = CurrText.find(g_global->m_CurrentLinkedItemName.c_str());
                    if (findedstring != std::wstring::npos) {
                        CurrText.erase(g_global->m_LinkedGlobalItemBegin, g_global->m_CurrentLinkedItemName.size());

                        pCIFEdit->SetText(CurrText.c_str());
                        if (CurrText.length() > 0)
                            pCIFEdit->SetCurrentIndex(g_global->m_LinkedGlobalItemBegin);
                        else
                            pCIFEdit->SetCurrentIndex(0);
                        g_global->m_LinkedGlobalItemEnd = 0;
                        g_global->m_LinkedGlobalItemBegin = 0;
                        g_global->m_CurrentLinkedItemName = std::wstring();
                        g_global->m_LinkedGlobalSlot = 0;
                        return 0;
                    }
                }
                if ((deleteState == -127 || deleteState == -128) && (CurrIndex + 2 >= g_global->m_LinkedGlobalItemBegin && CurrIndex <= g_global->m_LinkedGlobalItemEnd)) {
                    std::wstring new_str = CurrText.c_str();
                    int findedstring = CurrText.find(g_global->m_CurrentLinkedItemName.c_str());
                    if (findedstring != std::wstring::npos) {
                        CurrText.erase(g_global->m_LinkedGlobalItemBegin, g_global->m_CurrentLinkedItemName.size());

                        pCIFEdit->SetText(CurrText.c_str());
                        pCIFEdit->SetCurrentIndex(CurrText.length());
                        g_global->m_LinkedGlobalItemEnd = 0;
                        g_global->m_LinkedGlobalItemBegin = 0;
                        g_global->m_CurrentLinkedItemName = std::wstring();
                        g_global->m_LinkedGlobalSlot = 0;
                        return 0;
                    }
                }
            }
        }
    }
    if(this->GetParentControl()->IsSame(GFX_RUNTIME_CLASS(CIFSecondaryPassword)))
    {
        return 0;
    }
   /* else
    {
        g_global->m_LinkedGlobalItemEnd = 0;
        g_global->m_LinkedGlobalItemBegin = 0;
        g_global->m_CurrentLinkedItemName = std::wstring();
    }*/

    //printf("%d %d %d \n", a3, a4, a5);
    return reinterpret_cast<int(__thiscall *)(CIFEdit *, HWND, int, int, int)>(0x00636cd0)(this, hwnd, a3, a4, a5);
}
int CIFEdit::FUN_00634cb0(int a1, int a2)
{
    int aa = reinterpret_cast<int(__thiscall *)(CIFEdit *, int, int)>(
        0x00634cb0)(this, a1, a2);
    if(this->GetParentControl()->IsSame(GFX_RUNTIME_CLASS(CIFSecondaryPassword)))
    {
        if(this->UniqueID() == 13333 && a1 == 1)
            m_Player->FocusedId = 1;
        else if(this->UniqueID() == 13334 && a1 == 1)
            m_Player->FocusedId = 2;
        else if(this->UniqueID() == 13335 && a1 == 1)
            m_Player->FocusedId = 3;
    }
    return aa;
}
void CIFEdit::Func_17_IMPL(undefined4 p1){
    //printf("%d \n", p1);
    reinterpret_cast<void(__thiscall *)(CIFEdit *, undefined4)>(0x00637290)(this, p1);
}
