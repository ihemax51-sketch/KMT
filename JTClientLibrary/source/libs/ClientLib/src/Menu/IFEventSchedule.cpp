#include "IFEventSchedule.h"
#include <Game.h>
#include <ICPlayer.h>
#include <BSLib/multibyte.h>
#include "GInterface.h"
#include <CharacterDependentData.h>
#include <TextStringManager.h>
#include <GlobalDataManager.h>
#include <ctime>
#include <sstream>


#define GDR_NEXT 902
#define GDR_PREV 901
#define GDR_SPIN_TEXT_PAGE 900
GFX_IMPLEMENT_DYNCREATE(CIFEventSchedule, CIFMainFrame)

GFX_BEGIN_MESSAGE_MAP(CIFEventSchedule, CIFMainFrame)
                    ONG_COMMAND(GDR_PREV, &On_PrevBtn)
                    ONG_COMMAND(GDR_NEXT, &On_NextBtn)
GFX_END_MESSAGE_MAP()

CIFEventSchedule::CIFEventSchedule(void){
    EventScheduleList = std::vector<EventSchedule>();
    m_CurrentIndex = 0;
    m_MaxIndex = 0;
}
CIFEventSchedule::~CIFEventSchedule(void){
}

bool CIFEventSchedule::OnCreate(long ln)
{

    CIFMainFrame::OnCreate(ln);

    m_IRM.LoadFromFile("clientlibrary\\resinfo\\ifeventschedule.txt");
    m_IRM.CreateInterfaceSection("Create", this);
    wnd_rect sz;

    this->SetText(KmtGetText(L"UIIT_KMT_EVENT_SCHEDULE"));

    this->m_IRM.GetResObj(22, 1)->SetText(KmtGetText(L"UIIT_KMT_REMAINING_TIME"));
    this->m_IRM.GetResObj(28, 1)->SetText(KmtGetText(L"UIIT_KMT_EVENT_NAME"));
    this->m_IRM.GetResObj(GDR_SPIN_TEXT_PAGE, 1)->SetText(L"1");

    sz.size.width = 391;
    sz.size.height = 24;
    for (int i = 0; i < 9; ++i) {
        m_IRM.GetResObj<CIFEventScheduleSlot>(500 + i, 1)->ShowGWnd(true);
    }
    this->ShowGWnd(false);
    return true;
}

void CIFEventSchedule::UpdateMenuSize()
{

    int PosX = 0, PosY = 0;
    PosY = (g_CGame->GetRes().res->height/2) - (this->GetSize().height/2);
    PosX = (g_CGame->GetRes().res->width/2) - (this->GetSize().width/2);
    this->MoveGWnd(PosX, PosY);
    BringToFront();
}


void CIFEventSchedule::OnUpdate()
{

}
void CIFEventSchedule::Hide()
{
    this->m_IRM.GetResObj<CIFStatic>(31, 1)->ShowGWnd(false);
    this->m_IRM.GetResObj<CIFStatic>(32, 1)->ShowGWnd(false);
    this->m_IRM.GetResObj<CIFStatic>(33, 1)->ShowGWnd(false);
    this->m_IRM.GetResObj<CIFStatic>(34, 1)->ShowGWnd(false);
}
void CIFEventSchedule::Clear()
{
    for (int i = 0; i < 9; ++i) {
        m_IRM.GetResObj<CIFEventScheduleSlot>(500 + i, 1)->Clear();
    }
}
struct EventInfo
{
    long eventTime;
    long remainingTime;

    EventInfo(long eventTime, long remainingTime) : eventTime(eventTime), remainingTime(remainingTime) {}
};
void CIFEventSchedule::UpdateList()
{
    std::map<std::wstring, std::pair<time_t, EventSchedule>> nearestEvents; // En yakın etkinlik bilgisi

    time_t now = time(NULL);
    tm* currentTm = localtime(&now); // Yerel zaman

    // Etkinlikleri en yakın zamana göre bul ve sıralama işlemi yap
    for (std::size_t i = 0; i < EventScheduleList.size(); ++i) {
        const EventSchedule& event = EventScheduleList[i];

        // Etkinlik zamanını oluştur
        tm eventTime = *currentTm;
        std::wistringstream timeStream(event.Time);
        int hours, minutes;
        wchar_t colon; // ':' karakteri için
        timeStream >> hours >> colon >> minutes;

        // Etkinlik günü hesapla
        int daysUntilTargetDay;
        if (event.Day == 8) {
            // Her gün etkinliği için
            daysUntilTargetDay = 0;  // Aynı gün içinde
            if (hours < currentTm->tm_hour || (hours == currentTm->tm_hour && minutes <= currentTm->tm_min)) {
                // Eğer etkinlik saatini geçtiyse, yarını hedefle
                daysUntilTargetDay = 1;
            }
        } else {
            // Belirli bir gün için
            daysUntilTargetDay = (event.Day - 1 - currentTm->tm_wday + 7) % 7;
            // Eğer gün geçtiyse, bir hafta ileri hedefle
            if (daysUntilTargetDay == 0 && (hours < currentTm->tm_hour || (hours == currentTm->tm_hour && minutes <= currentTm->tm_min))) {
                daysUntilTargetDay = 7;
            }
        }

        // Etkinlik tarihi ve saati oluştur
        eventTime.tm_mday += daysUntilTargetDay;
        eventTime.tm_hour = hours;
        eventTime.tm_min = minutes;
        eventTime.tm_sec = 0;

        // Unix zaman damgasını oluştur
        time_t eventTimestamp = mktime(&eventTime);

        // Eğer etkinlik gelecekteyse ve henüz kaydedilmemişse
        if (eventTimestamp > now) {
            std::map<std::wstring, std::pair<time_t, EventSchedule>>::iterator it = nearestEvents.find(event.EventName);
            if (it == nearestEvents.end() || eventTimestamp < it->second.first) {
                nearestEvents[event.EventName] = std::make_pair(eventTimestamp, event);
            }
        }
    }

    // Yaklaşan etkinlikleri sıralamak için bir vector oluştur
    std::vector<std::pair<time_t, EventSchedule>> sortedEvents;
    for (std::map<std::wstring, std::pair<time_t, EventSchedule>>::iterator it = nearestEvents.begin(); it != nearestEvents.end(); ++it) {
        sortedEvents.push_back(it->second);
    }

    // Etkinlikleri zamana göre sırala (en yakın olan en başta olacak şekilde)
    for (size_t i = 0; i < sortedEvents.size(); ++i) {
        size_t minIndex = i;
        for (size_t j = i + 1; j < sortedEvents.size(); ++j) {
            if (sortedEvents[j].first < sortedEvents[minIndex].first) {
                minIndex = j;
            }
        }
        if (minIndex != i) {
            std::swap(sortedEvents[i], sortedEvents[minIndex]);
        }
    }

    Clear();
    m_CurrentIndex = 1;  // İlk sayfa
    m_MaxIndex = (sortedEvents.size() + 8) / 9;  // Toplam sayfa sayısı

    // İlk sayfayı yükle
    LoadPage(m_CurrentIndex);
    UpdateText(m_CurrentIndex);  // Sayfa numarasını güncelle
}



void CIFEventSchedule::LoadPage(int pageNumber)
{
    Clear();  // Önceki verileri temizle

    std::map<std::wstring, std::pair<time_t, EventSchedule>> nearestEvents; // En yakın etkinlik bilgisi

    time_t now = time(NULL);
    tm* currentTm = localtime(&now); // Yerel zaman

    // Etkinlikleri en yakın zamana göre bul ve sıralama işlemi yap
    for (std::size_t i = 0; i < EventScheduleList.size(); ++i) {
        const EventSchedule& event = EventScheduleList[i];

        // Etkinlik zamanını oluştur
        tm eventTime = *currentTm;
        std::wistringstream timeStream(event.Time);
        int hours, minutes;
        wchar_t colon; // ':' karakteri için
        timeStream >> hours >> colon >> minutes;

        // Etkinlik günü hesapla
        int daysUntilTargetDay;
        if (event.Day == 8) {
            // Her gün etkinliği için
            daysUntilTargetDay = 0;  // Aynı gün içinde
            if (hours < currentTm->tm_hour || (hours == currentTm->tm_hour && minutes <= currentTm->tm_min)) {
                // Eğer etkinlik saatini geçtiyse, ertesi gün için hesapla
                daysUntilTargetDay = 1;
            }
        } else {
            // Belirli bir gün için hesapla
            daysUntilTargetDay = (event.Day - 1 - currentTm->tm_wday + 7) % 7;
            // Eğer gün geçtiyse, bir hafta sonrası için ayarla
            if (daysUntilTargetDay == 0 && (hours < currentTm->tm_hour || (hours == currentTm->tm_hour && minutes <= currentTm->tm_min))) {
                daysUntilTargetDay = 7;
            }
        }

        // Etkinlik tarihi ve saati oluştur
        eventTime.tm_mday += daysUntilTargetDay;
        eventTime.tm_hour = hours;
        eventTime.tm_min = minutes;
        eventTime.tm_sec = 0;

        // Unix zaman damgasını oluştur
        time_t eventTimestamp = mktime(&eventTime);

        // Eğer etkinlik gelecekteyse ve henüz kaydedilmemişse
        if (eventTimestamp > now) {
            std::map<std::wstring, std::pair<time_t, EventSchedule>>::iterator it = nearestEvents.find(event.EventName);
            if (it == nearestEvents.end() || eventTimestamp < it->second.first) {
                nearestEvents[event.EventName] = std::make_pair(eventTimestamp, event);
            }
        }
    }

    // Yaklaşan etkinlikleri sıralamak için bir vector oluştur
    std::vector<std::pair<time_t, EventSchedule>> sortedEvents;
    for (std::map<std::wstring, std::pair<time_t, EventSchedule>>::iterator it = nearestEvents.begin(); it != nearestEvents.end(); ++it) {
        sortedEvents.push_back(it->second);
    }

    // Etkinlikleri zamana göre sırala (en yakın olan en başta olacak şekilde)
    for (size_t i = 0; i < sortedEvents.size(); ++i) {
        size_t minIndex = i;
        for (size_t j = i + 1; j < sortedEvents.size(); ++j) {
            if (sortedEvents[j].first < sortedEvents[minIndex].first) {
                minIndex = j;
            }
        }
        if (minIndex != i) {
            std::swap(sortedEvents[i], sortedEvents[minIndex]);
        }
    }

    // Sayfalama ayarları
    int startIndex = (pageNumber - 1) * 9;  // Sayfada gösterilecek ilk öğenin indeksi
    int endIndex = startIndex + 9;          // Sayfada gösterilecek son öğenin indeksi

    if (endIndex > sortedEvents.size()) {
        endIndex = sortedEvents.size();
    }

    // Yaklaşan etkinlikleri SlotID 500'den başlayarak ekle
    int slotID = 500;
    for (int i = startIndex; i < endIndex; ++i) {
        const EventSchedule& closestEvent = sortedEvents[i].second;
        long eventTime = sortedEvents[i].first;

        // Burada `SetName` çağrısı ile etkinlik bilgileri ve zamanını slotlara atıyoruz.
        m_IRM.GetResObj<CIFEventScheduleSlot>(slotID, 1)->SetName(i + 1, closestEvent.EventName.c_str(), eventTime);
        m_IRM.GetResObj<CIFEventScheduleSlot>(slotID, 1)->ShowGWnd(true);

        slotID++; // Sonraki slotID
    }
}


void CIFEventSchedule::UpdateText(int pageNumber)
{
    wchar_t buffer[255];
    swprintf_s(buffer, L"%d", pageNumber);  // Sayfa bilgisini göster
    m_IRM.GetResObj<CIFStatic>(GDR_SPIN_TEXT_PAGE, 1)->SetText(buffer);
}

void CIFEventSchedule::On_NextBtn()
{
    if (m_CurrentIndex < m_MaxIndex) {
        m_CurrentIndex++;
        LoadPage(m_CurrentIndex);  // Sonraki sayfayı yükle
        UpdateText(m_CurrentIndex);  // Sayfa numarasını güncelle
    }
}

void CIFEventSchedule::On_PrevBtn()
{
    if (m_CurrentIndex > 1) {
        m_CurrentIndex--;
        LoadPage(m_CurrentIndex);  // Önceki sayfayı yükle
        UpdateText(m_CurrentIndex);  // Sayfa numarasını güncelle
    }
}

