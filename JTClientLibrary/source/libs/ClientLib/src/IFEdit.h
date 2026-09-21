#pragma once

#include "IFStatic.h"

class CIFEdit : public CIFStatic {
GFX_DECLARE_DYNAMIC_EXISTING(CIFEdit, 0x00ee9540)

public:
    /// \address 00634EA0
    bool HasFocus() const;

    /// \address 00635150
    void AddValue_404(undefined4 value);

    /// \address 00635070
    void SetValue_404(undefined4 value);

    /// \address 00635060
    undefined4 GetValue_404() const;

    /// \address 00635170
    bool IsSetValue_404(undefined4 value) const;

    /// \address 00634f80
    void SetMaxLength(int length);

    /// \address 006351b0
    void SetTextmode(undefined4 mode);

    const std::n_wstring &GetCurrentText() const;
    void Func_17_IMPL(undefined4 p1);
    unsigned int GetAreaSize();

    unsigned int GetCurrentIndex();

    void SetCurrentIndex(unsigned int Index);

    void SetTextAreaSize(unsigned int Size);
    int FUN_00634cb0(int p1, int p2);
    int OnCharIMPL(HWND hwnd, int a3, int a4, int a5);
    void OnCharIMPLSelf(HWND hwnd, UINT msg, LPARAM lParam, WPARAM wParam);
    void Func_49(const wchar_t* text);
public:
    HWND m_hEditBoxWnd; //0x0380
    char pad_0384[120];        //0x0380
    unsigned int m_TextAreaSize;//0x03FC
    char pad_0400[0x48];        //0x0400
    unsigned int m_CurrentIndex;//0x0448
    char pad_044C[0xB034];      //0x044C
private:
BEGIN_FIXTURE()
        ENSURE_SIZE(0xb480) // big size, ha
        ENSURE_OFFSET(m_hEditBoxWnd, 0x0380)
    END_FIXTURE()

    RUN_FIXTURE(CIFEdit)


};
