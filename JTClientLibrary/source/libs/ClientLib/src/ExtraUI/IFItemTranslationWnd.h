#pragma once
#include <IFTextBox.h>
#include <IFVerticalScroll.h>
#include "IFMainFrame.h"
#include "IFEdit.h"
#include "IFItemTranslationSlot.h"

class CIFItemTranslationWnd : public CIFMainFrame
{
GFX_DECLARE_DYNCREATE(CIFItemTranslationWnd)
GFX_DECLARE_MESSAGE_MAP(CIFItemTranslationWnd)
private:
    int Func_4(int a2) override;
    CIFItemTranslationWnd(void);
    ~CIFItemTranslationWnd(void);
    bool OnCreate(long ln) override;
    void OnUpdate() override;
    void On_BtnClick();
    int Func_36(int a1, short action, int a3, int a4) override;
    void OnListChatThing(int, int);

    CIFTextBox* m_TextBox;
    CIFVerticalScroll* m_Scroll;
    CIFItemTranslationSlot* mySlot;
    std::n_wstring SelectedItemType;
public:
    void UpdateMenuSize();
    void Clear();
    undefined OnCloseWnd() override;
void ChineseWeaponIsPuttedSlot();
void EuropeWeaponIsPuttedSlot();
void ClearTexts();
};
