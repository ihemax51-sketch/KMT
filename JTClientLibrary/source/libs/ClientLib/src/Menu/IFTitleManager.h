#pragma once
#include <IFSelectableArea.h>
#include <IFScrollManager.h>
#include "IFMainFrame.h"
#include "IFEdit.h"


class CIFTitleManager : public CIFMainFrame
{
GFX_DECLARE_DYNCREATE(CIFTitleManager)
GFX_DECLARE_MESSAGE_MAP(CIFTitleManager)
private:
    CIFTitleManager(void);
    ~CIFTitleManager(void);
    bool OnCreate(long ln) override;
    void OnUpdate() override;
    void OnUnknownStuff();
    int Func_4(int a2) override;
    void On_BtnClick();
    void ButtonDelay(int timeoutSeconds);
    void OnTimer(int) override;
    void On_NextBtn();
    void On_PrevBtn();
public:
    struct TitleStruct
    {
        byte TitleID;
        std::wstring TitleName;
    };
    std::vector<TitleStruct> TitleList;

    struct ColorStruct
    {
        int DBID;
        std::wstring ColorName;
        unsigned __int32 ColorCode;
    };
    std::vector<ColorStruct> ColorList;

    void ActivateTabPage(BYTE page);
    void UpdateMenuSize();
    void Clear();
    void ClearDDJ();
    CIFSelectableArea **m_pTabs; //0x04A4 is a pointer, type unknown
    static const int numberOfTabs = 2;
    static const int tabWidth = 74;
    static const int tabHeight = 24;
    static const int tabMarginLeft = 18;
    static const int tabFirstId = 100;
    int m_SelectedItemId;
    byte ActiveTabNumber;
    int m_CurrentIndex;
    int m_MaxIndex;
    void UpdateText(int Number);
void LoadColorPage(int pageNumber);
void LoadTitlePage(int pageNumber);
};
