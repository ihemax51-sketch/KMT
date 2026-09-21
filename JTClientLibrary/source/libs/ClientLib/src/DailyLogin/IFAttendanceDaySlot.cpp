//
// Created by YUMBUL on 10.07.2024.
//

#include "IFAttendanceDaySlot.h"


GFX_IMPLEMENT_DYNCREATE(CIFAttendanceDaySlot, CIFWnd)
GFX_BEGIN_MESSAGE_MAP(CIFAttendanceDaySlot, CIFWnd)

GFX_END_MESSAGE_MAP()
CIFAttendanceDaySlot::CIFAttendanceDaySlot(void)
{

}
CIFAttendanceDaySlot::~CIFAttendanceDaySlot(void)
{

}


bool CIFAttendanceDaySlot::OnCreate(long ln)
{
    // Populate inherited members
    CIFWnd::OnCreate(ln);
    m_IRM.LoadFromFile("clientlibrary\\resinfo\\ifdailylogindayslot.txt");
    m_IRM.CreateInterfaceSection("Create", this);




    m_IRM.GetResObj(11, 1)->ShowGWnd(false); // Tek sayı günlerde gizle

    m_IRM.GetResObj(14, 1)->ShowGWnd(false); // Tek sayı günlerde gizle
    m_IRM.GetResObj(15, 1)->ShowGWnd(false); // Tek sayı günlerde gizle
    m_IRM.GetResObj(16, 1)->ShowGWnd(false); // Tek sayı günlerde gizle


    return true;
}
void CIFAttendanceDaySlot::OnUpdate() {

}
void CIFAttendanceDaySlot::ShowOK()
{
    m_IRM.GetResObj(16, 1)->ShowGWnd(true); // Tek sayı günlerde gizle

}
void CIFAttendanceDaySlot::ClearOK()
{
    m_IRM.GetResObj(16, 1)->ShowGWnd(false); // Tek sayı günlerde gizle

}
void CIFAttendanceDaySlot::UpdateCalenderVisual(int day, const char* month, int year, const char* dayOfWeek)
{
    if(day < 32)
    {
        if (day < 10)
        { // Single digit day
            m_IRM.GetResObj(13, 1)->ShowGWnd(false); // Hide the image with ID 7
            char imagePath[256];
            sprintf(imagePath, "interface\\event\\summer_day_%d.ddj", day);
            m_IRM.GetResObj(12, 1)->TB_Func_13(imagePath, 1, 1); // Set the image for ID 6

            m_IRM.GetResObj(11, 1)->ShowGWnd(true);

        }
        else { // Double digit day
            int tens = day / 10;
            int ones = day % 10;
            char imagePath1[256];
            char imagePath2[256];

            sprintf(imagePath1, "interface\\event\\summer_day_%d.ddj", tens);
            sprintf(imagePath2, "interface\\event\\summer_day_%d.ddj", ones);
            m_IRM.GetResObj(12, 1)->TB_Func_13(imagePath1, 1, 1); // Set the image for ID 6
            m_IRM.GetResObj(13, 1)->TB_Func_13(imagePath2, 1, 1); // Set the image for ID 6

            m_IRM.GetResObj(11, 1)->ShowGWnd(true);
        }
    }
    else
    {
        m_IRM.GetResObj(12, 1)->ShowGWnd(false);
        m_IRM.GetResObj(13, 1)->ShowGWnd(false);
    }

    if(day == 1)
    {
        m_IRM.GetResObj(14, 1)->ShowGWnd(true); // Tek sayı günlerde gizle
    }
    if(day == 30)
    {
        m_IRM.GetResObj(15, 1)->ShowGWnd(true); // Tek sayı günlerde gizle
    }
}