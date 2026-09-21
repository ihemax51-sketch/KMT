#pragma once
#include <IFStatic.h>
#include <IFWnd.h>
#include <IFBarWnd.h>

class CIFAlchemyMacroAttrSlot : public CIFWnd
{
GFX_DECLARE_DYNCREATE(CIFAlchemyMacroAttrSlot)
GFX_DECLARE_MESSAGE_MAP(CIFAlchemyMacroAttrSlot)
public:
    CIFAlchemyMacroAttrSlot(void);
    ~CIFAlchemyMacroAttrSlot(void);
    bool OnCreate(long ln) override;
    void OnUpdate() override;
    int OnMouseLeftUp(int a1, int x, int y) override;
public:
    void ClearDDJ();
    void Clear();
    std::wstring Code;
    void LoadItems2(int Targetx, const wchar_t* Code, const wchar_t* Name);
};