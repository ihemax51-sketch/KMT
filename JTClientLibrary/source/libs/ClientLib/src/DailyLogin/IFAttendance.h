#pragma once
#include <IFSelectableArea.h>
#include "IFMainFrame.h"
#include "IFEdit.h"
#include "IFAttendanceDaySlot.h"


class CIFAttendance : public CIFMainFrame
{
GFX_DECLARE_DYNCREATE(CIFAttendance)
GFX_DECLARE_MESSAGE_MAP(CIFAttendance)
private:
    CIFAttendance(void);
    ~CIFAttendance(void);
    bool OnCreate(long ln) override;


public:
    void UpdateMenuSize();

    CIFAttendanceDaySlot* dayslots[35];
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

    struct MyAttendStruct
    {
        int DayCount;
        int ActiveButtons;
    };
};
