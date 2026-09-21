#pragma once
#include <IFScrollManager.h>
#include "IFMainFrame.h"
#include "IFEdit.h"
#include "IFEventScheduleSlot.h"

class CIFEventSchedule : public CIFMainFrame
{
GFX_DECLARE_DYNCREATE(CIFEventSchedule)
GFX_DECLARE_MESSAGE_MAP(CIFEventSchedule)
private:
    CIFEventSchedule(void);
    ~CIFEventSchedule(void);
    bool OnCreate(long ln) override;
    void OnUpdate() override;
public:
    void UpdateMenuSize();
    void Clear();
    void UpdateList();
    void Hide();
    struct EventSchedule
    {
        int ID;
        std::wstring EventName;
        byte Day;
        std::wstring Time;

    };
    std::vector<EventSchedule> EventScheduleList;
    time_t ConvertToUtc(const tm& localTime);
    int m_CurrentIndex;
    int m_MaxIndex;
void LoadPage(int pageNumber);
void UpdateText(int pageNumber);
void On_NextBtn();
void On_PrevBtn();
};
