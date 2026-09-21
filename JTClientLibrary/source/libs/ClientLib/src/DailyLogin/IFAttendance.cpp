#include "IFAttendance.h"
#include <Game.h>
#include <GlobalDataManager.h>
#include <ctime>

#define GDR_ATTEND_BUTTON 13
#define GDR_GIFT_LEFT_BUTTON 14
#define GDR_GIFT_RIGHT_BUTTON 16

GFX_IMPLEMENT_DYNCREATE(CIFAttendance, CIFMainFrame)

GFX_BEGIN_MESSAGE_MAP(CIFAttendance, CIFMainFrame)
                    ONG_COMMAND(100, &CIFAttendance::OnUnknownStuff)
                    ONG_COMMAND(GDR_GIFT_LEFT_BUTTON, &CIFAttendance::PreviousPage)
                    ONG_COMMAND(GDR_GIFT_RIGHT_BUTTON, &CIFAttendance::NextPage)
                    ONG_COMMAND(GDR_ATTEND_BUTTON, &CIFAttendance::Attend)

GFX_END_MESSAGE_MAP()

CIFAttendance::CIFAttendance(void){
    currentPage = 0;

}
CIFAttendance::~CIFAttendance(void){
    if (m_pTabs) {
        free(m_pTabs);
        m_pTabs = 0;
    }
}
void CIFAttendance::Attend() {

    CMsgStreamBuffer buf(0x169A);
    buf << BYTE(14);
    SendMsg(buf);
}
#define GDR_COUNT_1 6
#define GDR_COUNT_2 7
int CIFAttendance::Func_4(int a2) {
    int v1 = 0;
    while (a2 != v1 + 100) {
        if (++v1 >= 5)
            return -1;
    }

    return 100;
}
void CIFAttendance::OnUnknownStuff() {
    int id = GetCurrentEventMsgCtrlId();
    int i = 0;

    for (int i = 0; i < numberOfTabs; ++i) {
        if (id == m_pTabs[i]->UniqueID()) {
            ActivateTabPage(i);
            return;
        }
    }
}
void CIFAttendance::ShowPage(int page)
{
}

void CIFAttendance::ActivateTabPage(BYTE page) {
    for (int i = 0; i < numberOfTabs; i++) {
        if (i == page)
            continue;

        m_pTabs[i]->sub_64CC30(0);
        m_pTabs[i]->m_FontTexture.sub_8B4750(2);
    }


    m_pTabs[page]->sub_64CC30(1);

    if(page == 0)
    {
        for (int i = 0; i < 35; ++i)
        {
            dayslots[i]->ShowGWnd(true);
        }

        m_IRM.GetResObj(10, 1)->ShowGWnd(true);

        m_IRM.GetResObj(11, 1)->ShowGWnd(false);
        m_IRM.GetResObj(12, 1)->ShowGWnd(false);

        m_IRM.GetResObj(13, 1)->ShowGWnd(true);

        m_IRM.GetResObj(14, 1)->ShowGWnd(false);
        m_IRM.GetResObj(15, 1)->ShowGWnd(false);

        m_IRM.GetResObj(16, 1)->ShowGWnd(false);

    }
    else if(page == 1)
    {
        for (int i = 0; i < 35; ++i)
        {
            dayslots[i]->ShowGWnd(false);
        }
        m_IRM.GetResObj(13, 1)->ShowGWnd(false);


        m_IRM.GetResObj(10, 1)->ShowGWnd(false);



        m_IRM.GetResObj(11, 1)->ShowGWnd(true);
        m_IRM.GetResObj(12, 1)->ShowGWnd(true);

        m_IRM.GetResObj(14, 1)->ShowGWnd(true);
        m_IRM.GetResObj(15, 1)->ShowGWnd(true);
        m_IRM.GetResObj(16, 1)->ShowGWnd(true);

        ShowPage(currentPage);
    }
}

bool CIFAttendance::OnCreate(long ln)
{

    // Populate inherited members
    CIFMainFrame::OnCreate(ln);


    m_IRM.LoadFromFile("clientlibrary\\resinfo\\ifdailylogin.txt");
    m_IRM.CreateInterfaceSection("Create", this);
    TB_Func_13("interface\\frame\\mall_sub_wnd04_", 0, 0);
    this->m_pCloseBtn->TB_Func_13("interface\\event\\summer_close.ddj", 1, 1);
    this->SetText(KmtGetText(L"UIIT_KMT_ATTENDANCE_EVENT"));

    m_IRM.GetResObj(6, 1)->TB_Func_13("interface\\event\\summer_count_1.ddj", 1, 1);
    m_IRM.GetResObj(7, 1)->TB_Func_13("interface\\event\\summer_count_2.ddj", 1, 1);


    wnd_rect sz;
    sz.pos.x = 23;
    sz.pos.y = 123;
    sz.size.width = 80;
    sz.size.height = 48;
    for (int i = 0; i < 35; ++i)
    {
        dayslots[i] = (CIFAttendanceDaySlot*)CGWnd::CreateInstance(this, GFX_RUNTIME_CLASS(CIFAttendanceDaySlot), sz, 302+i, 0);
        sz.pos.x += sz.size.width; // x + 80
        if ((i + 1) % 7 == 0) { // 7 tane oluşturduktan sonra
            sz.pos.x = 23; // x'i başa sar
            sz.pos.y += sz.size.height; // y + 48
        }
    }





    m_pTabs = new CIFSelectableArea *[numberOfTabs];

    for (int i = 0; i < numberOfTabs; i++) {


        RECT mrect;
        mrect.top = 5;
        mrect.left = 6;
        mrect.right = 0;
        mrect.bottom = 4;

        RECT selectable_area_size;
        selectable_area_size.top = 49;
        selectable_area_size.left = tabMarginLeft + tabWidth * i;
        selectable_area_size.right = tabWidth - 4;
        selectable_area_size.bottom = tabHeight;

        m_pTabs[i] = (CIFSelectableArea *) CGWnd::CreateInstance(this, GFX_RUNTIME_CLASS(CIFSelectableArea),
                                                                 selectable_area_size, tabFirstId + i, 0);

        m_pTabs[i]->SetSomeRect(mrect);
        m_pTabs[i]->SetFont(this->N00009C2F);

        m_pTabs[i]->sub_64CE30("interface\\event\\summer_tab_focus.ddj",
                               "interface\\event\\summer_tab_off.ddj", "interface\\event\\summer_tab_off.ddj");

        if (i) {
            switch (i) {
                case 1:
                    m_pTabs[1]->SetText(KmtGetText(L"UIIT_KMT_REWARD"));
                    break;
            }
            m_pTabs[i]->sub_64CC30(0);
        } else {
            m_pTabs[0]->SetText(KmtGetText(L"UIIT_KMT_ATTENDANCE"));
            m_pTabs[i]->sub_64CC30(1);
        }
    }
    this->m_IRM.GetResObj(13, 1)->SetText(KmtGetText(L"UIIT_KMT_CHECK_THE_ATTENDANCE"));
    this->m_IRM.GetResObj(10, 1)->SetText(KmtGetText(L"UIIT_KMT_NUMBER_OF_ATTENDED_DAYS"));

    UpdateMenuSize();
//    ShowPage(currentPage);
    this->ShowGWnd(false);
    return true;
}
void CIFAttendance::NextPage()
{
    currentPage = (currentPage + 1) % totalPages;
    ShowPage(currentPage);
}

void CIFAttendance::PreviousPage()
{
    currentPage = (currentPage - 1 + totalPages) % totalPages;
    ShowPage(currentPage);
}
#include <ctime>

void CIFAttendance::UpdateMenuSize()
{
    std::time_t t = std::time(0);   // Şu anki zaman
    std::tm* now = std::localtime(&t);
    int day = now->tm_mday;

    if (day < 10) { // Tek haneli gün
        m_IRM.GetResObj(7, 1)->ShowGWnd(false); // ID 7 olan resmi gizle
        char imagePath[256];
        sprintf(imagePath, "interface\\event\\summer_count_%d.ddj", day);
        m_IRM.GetResObj(6, 1)->TB_Func_13(imagePath, 1, 1); // ID 6 olan resmi ayarla
    } else { // Çift haneli gün
        int tens = day / 10;
        int ones = day % 10;
        char imagePath1[256];
        char imagePath2[256];

        sprintf(imagePath1, "interface\\event\\summer_count_%d.ddj", tens);
        sprintf(imagePath2, "interface\\event\\summer_count_%d.ddj", ones);
        m_IRM.GetResObj(6, 1)->TB_Func_13(imagePath1, 1, 1); // ID 6 olan resmi ayarla
        m_IRM.GetResObj(7, 1)->TB_Func_13(imagePath2, 1, 1); // ID 7 olan resmi ayarla
    }

    const wchar_t* monthNames[] = {
            L"jan", L"feb", L"mar", L"apr", L"may", L"jun",
            L"jul", L"aug", L"sep", L"oct", L"nov", L"dec"
    };

    char monthImagePath[256];
    sprintf(monthImagePath, "interface\\event\\summer_%ls.ddj", monthNames[now->tm_mon]);
    m_IRM.GetResObj(8, 1)->TB_Func_13(monthImagePath, 1, 1); // ID 8 olan resmi ayarla

    // Yıl için görselleri ayarla
    char yearImagePath1[256];
    char yearImagePath2[256];
    char yearImagePath3[256];
    char yearImagePath4[256];
    sprintf(yearImagePath1, "interface\\event\\summer_year_%d.ddj", (now->tm_year + 1900) / 1000);
    sprintf(yearImagePath2, "interface\\event\\summer_year_%d.ddj", ((now->tm_year + 1900) / 100) % 10);
    sprintf(yearImagePath3, "interface\\event\\summer_year_%d.ddj", ((now->tm_year + 1900) / 10) % 10);
    sprintf(yearImagePath4, "interface\\event\\summer_year_%d.ddj", (now->tm_year + 1900) % 10);
    m_IRM.GetResObj(2, 1)->TB_Func_13(yearImagePath1, 1, 1);
    m_IRM.GetResObj(3, 1)->TB_Func_13(yearImagePath2, 1, 1);
    m_IRM.GetResObj(4, 1)->TB_Func_13(yearImagePath3, 1, 1);
    m_IRM.GetResObj(5, 1)->TB_Func_13(yearImagePath4, 1, 1);

    UpdateCalender();
    int PosX = 0, PosY = 0;
    PosY = (g_CGame->GetRes().res->height/2) - (this->GetSize().height/2);
    PosX = (g_CGame->GetRes().res->width/2) - (this->GetSize().width/2);
    this->MoveGWnd(PosX, PosY);
    BringToFront();
}

void CIFAttendance::UpdateCalender()
{
    const char* daysOfWeek[] = {
            "Pazartesi", "Salı", "Çarşamba", "Perşembe", "Cuma", "Cumartesi", "Pazar"
    };
    const char* months[] = {
            "Ocak", "Şubat", "Mart", "Nisan", "Mayıs", "Haziran",
            "Temmuz", "Ağustos", "Eylül", "Ekim", "Kasım", "Aralık"
    };
    int daysInMonth[] = { 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 };

    // Şu anki tarih
    std::time_t t = std::time(0);
    std::tm* now = std::localtime(&t);

    // Takvimde gösterilecek ay ve yıl
    int displayMonth = now->tm_mon;
    int displayYear = now->tm_year + 1900;

    // Ayın ilk gününü bul
    std::tm firstDayOfMonth = {};
    firstDayOfMonth.tm_year = displayYear - 1900;
    firstDayOfMonth.tm_mon = displayMonth;
    firstDayOfMonth.tm_mday = 1;
    std::mktime(&firstDayOfMonth);

    int firstDayOfWeek = firstDayOfMonth.tm_wday;

    if (firstDayOfWeek == 0)
    {
        firstDayOfWeek = 6; // Pazar günü 6. gün olarak ayarlanır (Pazartesi başlangıç)
    }
    else
    {
        firstDayOfWeek -= 1; // Diğer günler için günleri ayarla
    }

    // Takvimdeki ilk günü ayarla
    int day = 1;

    // Ayı kapsamayan günler için boş slotlar
    for (int i = 0; i < firstDayOfWeek; ++i)
    {
        dayslots[i]->UpdateCalenderVisual(0, "", 0, ""); // Boş slot
    }

    // Ayı kapsayan günleri ayarla
    for (int i = firstDayOfWeek; i < 35; ++i)
    {
        if (day <= daysInMonth[displayMonth])
        {
            // "END" yazılacak gün 31. gün
            if (day == 31)
            {
                dayslots[i]->UpdateCalenderVisual(day, "END", displayYear, daysOfWeek[(firstDayOfWeek + i) % 7]);
            }
            else
            {
                dayslots[i]->UpdateCalenderVisual(day, months[displayMonth], displayYear, daysOfWeek[(firstDayOfWeek + i) % 7]);
            }

            ++day;
        }
        else
        {
            // Diğer aya ait günleri göstermemek için boş slotlar
            dayslots[i]->UpdateCalenderVisual(0, "", 0, "");
        }
    }
}






