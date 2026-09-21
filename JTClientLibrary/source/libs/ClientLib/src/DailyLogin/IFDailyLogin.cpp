#include "IFDailyLogin.h"
#include <Game.h>
#include <ICPlayer.h>
#include <BSLib/multibyte.h>
#include "GInterface.h"
#include <CharacterDependentData.h>
#include <TextStringManager.h>
#include <GlobalDataManager.h>
#include <IFMagicStateBoard.h>

#include <sstream>
#include <CustomData/CustomDataManager.h>

#include <ctime>

#define GDR_ATTEND_BUTTON 13
#define GDR_GIFT_LEFT_BUTTON 14
#define GDR_GIFT_RIGHT_BUTTON 16

GFX_IMPLEMENT_DYNCREATE(CIFDailyLogin, CIFMainFrame)

GFX_BEGIN_MESSAGE_MAP(CIFDailyLogin, CIFMainFrame)
                    ONG_COMMAND(100, &CIFDailyLogin::OnUnknownStuff)
                    ONG_COMMAND(GDR_GIFT_LEFT_BUTTON, &CIFDailyLogin::PreviousPage)
                    ONG_COMMAND(GDR_GIFT_RIGHT_BUTTON, &CIFDailyLogin::NextPage)
                    ONG_COMMAND(GDR_ATTEND_BUTTON, &CIFDailyLogin::Attend)

GFX_END_MESSAGE_MAP()

CIFDailyLogin::CIFDailyLogin(void){
    currentPage = 0;
    m_RefAttendanceRewards = std::map<int, RefAttendanceEventStruct>();
}
CIFDailyLogin::~CIFDailyLogin(void){
    if (m_pTabs) {
        free(m_pTabs);
        m_pTabs = 0;
    }
}
void CIFDailyLogin::Attend() {

    CMsgStreamBuffer buf(0x169A);
    buf << BYTE(14);
    SendMsg(buf);
}
#define GDR_COUNT_1 6
#define GDR_COUNT_2 7
int CIFDailyLogin::Func_4(int a2) {
    int v1 = 0;
    while (a2 != v1 + 100) {
        if (++v1 >= 5)
            return -1;
    }

    return 100;
}
void CIFDailyLogin::OnUnknownStuff() {
    int id = GetCurrentEventMsgCtrlId();
    int i = 0;

    for (int i = 0; i < numberOfTabs; ++i) {
        if (id == m_pTabs[i]->UniqueID()) {
            ActivateTabPage(i);
            return;
        }
    }
}
void CIFDailyLogin::ShowPage(int page)
{
    for (int i = 0; i < totalSlots; ++i)
    {
        if (i / slotsPerPage == page)
        {
            rewardslot[i]->ShowGWnd(true); // Slotu göster
        }
        else
        {
            rewardslot[i]->ShowGWnd(false); // Slotu gizle
        }
    }
    wchar_t pageText[16];
    swprintf(pageText, L"%d", page + 1);
    m_IRM.GetResObj(15, 1)->SetText(pageText);

}

void CIFDailyLogin::ActivateTabPage(BYTE page) {
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

        for (int i = 0; i < 50; ++i)
        {
            rewardslot[i]->ShowGWnd(false);
        }
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

bool CIFDailyLogin::OnCreate(long ln)
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
        dayslots[i] = (CIFDailyLoginDaySlot*)CGWnd::CreateInstance(this, GFX_RUNTIME_CLASS(CIFDailyLoginDaySlot), sz, 302+i, 0);
        sz.pos.x += sz.size.width; // x + 80
        if ((i + 1) % 7 == 0) { // 7 tane oluşturduktan sonra
            sz.pos.x = 23; // x'i başa sar
            sz.pos.y += sz.size.height; // y + 48
        }
    }



    sz.pos.x = 24;
    sz.pos.y = 116;
    sz.size.width = 80;
    sz.size.height = 48;

    for (int i = 0; i < 50; ++i)
    {

        if (i % 10 < 5) // İlk 5 öğe sol tarafta alt alta
        {
            sz.pos.y = 116 + (i % 5) * 50; // Alt alta
        }
        else // Sonraki 5 öğe sağ tarafta alt alta
        {
            sz.pos.x = 307; // Sağ tarafta başlangıç x pozisyonu
            sz.pos.y = 116 + ((i % 5) * 50); // Alt alta
        }
        rewardslot[i] = (CIFDailyLoginRewardSlot*)CGWnd::CreateInstance(this, GFX_RUNTIME_CLASS(CIFDailyLoginRewardSlot), sz, 402 + i, 0);

        // Her 10 öğede bir başa dön
        if ((i + 1) % 10 == 0)
        {
            sz.pos.x = 24; // x'i başa sar
            sz.pos.y = 116; // y'yi artır
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


    ShowPage(currentPage);
    this->ShowGWnd(false);
    return true;
}
void CIFDailyLogin::NextPage()
{
    currentPage = (currentPage + 1) % totalPages;
    ShowPage(currentPage);
}

void CIFDailyLogin::PreviousPage()
{
    currentPage = (currentPage - 1 + totalPages) % totalPages;
    ShowPage(currentPage);
}

#include <map>
#include <vector>
#include <algorithm>
#include <ctime>    // For std::time_t and std::localtime
// Karşılaştırma Fonksiyonu
struct CompareDayCount { bool operator()(const std::pair<int, CIFDailyLogin::RefAttendanceEventStruct>& lhs, const std::pair<int, CIFDailyLogin::RefAttendanceEventStruct>& rhs) const { return lhs.second.DayCount < rhs.second.DayCount; }
};
void CIFDailyLogin::UpdateMenuSize()
{
    std::time_t t = std::time(0);   // Get time now
    std::tm* now = std::localtime(&t);
    int day = now->tm_mday;

    if (day < 10) { // Single digit day
        m_IRM.GetResObj(7, 1)->ShowGWnd(false); // Hide the image with ID 7
        char imagePath[256];
        sprintf(imagePath, "interface\\event\\summer_count_%d.ddj", day);
        m_IRM.GetResObj(6, 1)->TB_Func_13(imagePath, 1, 1); // Set the image for ID 6
    } else { // Double digit day
        int tens = day / 10;
        int ones = day % 10;
        char imagePath1[256];
        char imagePath2[256];

        sprintf(imagePath1, "interface\\event\\summer_count_%d.ddj", tens);
        sprintf(imagePath2, "interface\\event\\summer_count_%d.ddj", ones);
        m_IRM.GetResObj(6, 1)->TB_Func_13(imagePath1, 1, 1); // Set the image for ID 6
        m_IRM.GetResObj(7, 1)->TB_Func_13(imagePath2, 1, 1); // Set the image for ID 6
    }

    const wchar_t* monthNames[] = {
            L"jan", L"feb", L"mar", L"apr", L"may", L"jun",
            L"jul", L"aug", L"sep", L"oct", L"nov", L"dec"
    };

    char monthImagePath[256];
    sprintf(monthImagePath, "interface\\event\\summer_%ls.ddj", monthNames[now->tm_mon]);
    m_IRM.GetResObj(8, 1)->TB_Func_13(monthImagePath, 1, 1); // Set the image for ID 6

    // Verilerinizi saklamak için std::vector kullanacağız
    std::vector<std::pair<int, RefAttendanceEventStruct> > sortedVector(m_RefAttendanceRewards.begin(), m_RefAttendanceRewards.end());

    // DayCount'a göre sıralama
    std::sort(sortedVector.begin(), sortedVector.end(), CompareDayCount());

    int i = 0;
    // Sıralı vector'ü işleme
    for (std::vector<std::pair<int, RefAttendanceEventStruct> >::iterator it = sortedVector.begin();
         it != sortedVector.end() && i < 50;
         ++it)
    {
        if (rewardslot[i] != NULL)
            rewardslot[i]->FillTheSlot(it->second.ItemID, it->second.ItemCount, 0, it->second.DayCount, it->second.ID);
        i++;
    }

    UpdateCalender();
    int PosX = 0, PosY = 0;
    PosY = (g_CGame->GetRes().res->height/2) - (this->GetSize().height/2);
    PosX = (g_CGame->GetRes().res->width/2) - (this->GetSize().width/2);
    this->MoveGWnd(PosX, PosY);
    BringToFront();
}


void CIFDailyLogin::UpdateCalender()
{
    const char* daysOfWeek[] = {
            "Pazartesi", "Salı", "Çarşamba", "Perşembe", "Cuma", "Cumartesi", "Pazar"
    };
    const char* months[] = {
            "Ocak", "Şubat", "Mart", "Nisan", "Mayıs", "Haziran",
            "Temmuz", "Ağustos", "Eylül", "Ekim", "Kasım", "Aralık"
    };
    int daysInMonth[] = { 31, 31, 30, 31, 30, 31, 31, 31, 30, 31, 30, 31 };

    // Başlangıç tarihi: 1 Temmuz 2024 (Pazartesi)
    int day = 1;
    int monthIndex = 6; // Temmuz
    int year = 2025;
    int dayOfWeekIndex = 0; // Pazartesi

    for (int i = 0; i < 35; ++i) {
        dayslots[i]->UpdateCalenderVisual(i+1, months[monthIndex], year, daysOfWeek[dayOfWeekIndex]);

        // Bir sonraki günü hesapla
        ++day;
        ++dayOfWeekIndex;
        if (dayOfWeekIndex == 7) {
            dayOfWeekIndex = 0;
        }
        if (day > daysInMonth[monthIndex]) {
            day = 1;
            ++monthIndex;
            if (monthIndex == 12) {
                monthIndex = 0;
                ++year;
            }
        }
    }
}
void CIFDailyLogin::OnUpdate()
{

}
void CIFDailyLogin::Clear()
{

}

#define DAT_00eedf08 (*(void **) 0x00eedf08)        /// charID ?

void CIFDailyLogin::On_BtnClick() {

}
