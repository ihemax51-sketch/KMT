#pragma once
#include <IFStatic.h>
#include <IFWnd.h>
#include <IFBarWnd.h>

class CIFAlchemyMacroBlueSlot : public CIFWnd
{
GFX_DECLARE_DYNCREATE(CIFAlchemyMacroBlueSlot)
GFX_DECLARE_MESSAGE_MAP(CIFAlchemyMacroBlueSlot)
public:
    CIFAlchemyMacroBlueSlot(void);
    ~CIFAlchemyMacroBlueSlot(void);
    bool OnCreate(long ln) override;
    void OnUpdate() override;
    int OnMouseLeftUp(int a1, int x, int y) override;
public:
void ClearDDJ();
void LoadItems(int Param1, const wchar_t * Name);
void Clear();
int Param1;
std::wstring Code;
void LoadItems2(const wchar_t* Code, const wchar_t* Name);
};