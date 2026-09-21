#pragma once
#include <IFSelectableArea.h>
#include "IFMainFrame.h"
#include "IFEdit.h"
#include "IFDailyLoginDaySlot.h"
#include "IFDailyLoginRewardSlot.h"

class CIFDailyLogin : public CIFMainFrame
{
GFX_DECLARE_DYNCREATE(CIFDailyLogin)
GFX_DECLARE_MESSAGE_MAP(CIFDailyLogin)
private:
    CIFDailyLogin(void);
    ~CIFDailyLogin(void);
    bool OnCreate(long ln) override;
    void OnUpdate() override;
    void On_BtnClick();
public:
    void UpdateMenuSize();
    void Clear();
    CIFDailyLoginDaySlot* dayslots[35];
    CIFDailyLoginRewardSlot* rewardslot[50];
    void OnUnknownStuff();
    void ActivateTabPage(BYTE page);
    int Func_4(int a2) override;
    static const int numberOfTabs = 2;
    static const int tabWidth = 100;
    static const int tabHeight = 24;
    static const int tabMarginLeft = 13;
    static const int tabFirstId = 100;
    CIFSelectableArea **m_pTabs; //0x04A4 is a pointer, type unknown
    void UpdateCalender();
    int currentPage;
    static const int slotsPerPage = 10;
    static const int totalSlots = 50;
    static const int totalPages = (totalSlots + slotsPerPage - 1) / slotsPerPage; // 5 sayfa
    void ShowPage(int page);
    void PreviousPage();
    void NextPage();
    void Attend();
    struct RefAttendanceEventStruct
    {
        int ID;
        int ItemID;
        int ItemCount;
        int DayCount;
    };
    std::map<int, RefAttendanceEventStruct> m_RefAttendanceRewards;


    struct MyAttendStruct
    {
        int DayCount;
        int ActiveButtons;
    };
};
