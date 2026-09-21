#include <TextStringManager.h>
#include <time.h>
#include <ctime>
#include <GlobalHelpersThatHaveNoHomeYet.h>
#include <GInterface.h>
#include "IFEventSchedule.h"


GFX_IMPLEMENT_DYNCREATE(CIFEventScheduleSlot, CIFWnd)

CIFEventScheduleSlot::CIFEventScheduleSlot(void)
{
    times = 0;
}
CIFEventScheduleSlot::~CIFEventScheduleSlot(void)
{
}
bool CIFEventScheduleSlot::OnCreate(long ln)
{
    // Populate inherited members
    CIFWnd::OnCreate(ln);
    m_IRM.LoadFromFile("clientlibrary\\resinfo\\ifeventscheduleslot.txt");
    m_IRM.CreateInterfaceSection("Create", this);

    return true;
}


void CIFEventScheduleSlot::OnUpdate() {
    if (times != 0) {
        std::time_t currentTime = std::time(NULL);


        long long remainingTime = times - currentTime;

        if (remainingTime > 0) {
            long days = remainingTime / (60 * 60 * 24);
            long hours = (remainingTime % (60 * 60 * 24)) / (60 * 60);
            long minutes = (remainingTime % (60 * 60)) / 60;
            long seconds = remainingTime % 60;

            wchar_t buffer12[255];
            std::swprintf(buffer12, KmtGetText(L"UIIT_KMT_IN_VALUE_D_VALUE_H_VALUE_M_VALUE_S"), days, hours, minutes, seconds);

            this->m_IRM.GetResObj<CIFStatic>(4,1)->SetText(buffer12);
        } else {
            g_pCGInterface->m_IRM.GetResObj<CIFEventSchedule>(EventScheduleID, 1)->Clear();
            g_pCGInterface->m_IRM.GetResObj<CIFEventSchedule>(EventScheduleID, 1)->UpdateList();
        }
    }
}



void CIFEventScheduleSlot::SetName(int Num, const wchar_t* name, long eventtime) {
    wchar_t buffer1[255];
    swprintf(buffer1, L"%d", Num);
    this->m_IRM.GetResObj<CIFStatic>(3,1)->SetText(name);
    times = eventtime;

}

void CIFEventScheduleSlot::Clear() {
    this->m_IRM.GetResObj<CIFStatic>(3,1)->SetText(L"");
    this->m_IRM.GetResObj<CIFStatic>(4,1)->SetText(L"");
    times = 0;
}
