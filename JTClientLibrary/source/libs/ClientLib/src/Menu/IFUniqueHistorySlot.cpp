#include <TextStringManager.h>
#include <time.h>
#include <ctime>
#include <GInterface.h>
#include <Game.h>
#include <CustomInterface/IFDps.h>
#include "IFUniqueHistory.h"

GFX_IMPLEMENT_DYNCREATE(CIFUniqueHistorySlot, CIFWnd)
GFX_BEGIN_MESSAGE_MAP(CIFUniqueHistorySlot, CIFWnd)
GFX_END_MESSAGE_MAP()

CIFUniqueHistorySlot::CIFUniqueHistorySlot(void)
{
    times = 0;
    KilledRegID = 0;
    X = 0;
    Z = 0;
    Y = 0;
    WorldID = 0;
    MapType = 0;
    MapIndex = 0;
    UniqueID = 0;
}

CIFUniqueHistorySlot::~CIFUniqueHistorySlot(void)
{
}

bool CIFUniqueHistorySlot::OnCreate(long ln)
{
    // Populate inherited members
    CIFWnd::OnCreate(ln);
    m_IRM.LoadFromFile("clientlibrary\\resinfo\\ifuniquehistoryslot.txt");
    m_IRM.CreateInterfaceSection("Create", this);

    return true;
}

void CIFUniqueHistorySlot::OnUpdate() {
    if(times != 0)
    {
        std::time_t currentTime = std::time(NULL);
        __int64 elapsedTime = static_cast<__int64>(currentTime) - static_cast<__int64>(times);
        if (elapsedTime < 0) elapsedTime = 0;
        int days = static_cast<int>(elapsedTime / (60 * 60 * 24));
        int hours = static_cast<int>((elapsedTime % (60 * 60 * 24)) / (60 * 60));
        int minutes = static_cast<int>((elapsedTime % (60 * 60)) / 60);

        wchar_t buffer12[255];
        swprintf_s(buffer12, 255, KmtGetText(L"UIIT_KMT_VALUE_D_VALUE_H_VALUE_M_AGO"), days, hours, minutes);

        this->m_IRM.GetResObj<CIFStatic>(5, 1)->SetText(buffer12);
    }
}

void CIFUniqueHistorySlot::SetName(int Num, const wchar_t* uniquename, byte state, __int64 time,
                                  const wchar_t* killer, int RegionID, float KilledX, float KilledY,
                                  float KilledZ, int WorldIDx, byte MapTypex, int MapIndexx,
                                  int UniqueIDx) {
    UniqueID = UniqueIDx;
    wchar_t buffer1[255];
    swprintf_s(buffer1, L"%d", Num);
    UniqName = uniquename;

    this->m_IRM.GetResObj<CIFStatic>(3, 1)->SetText(uniquename);

    if(state == 0)
    {
        this->m_IRM.GetResObj<CIFStatic>(4, 1)->SetText(KmtGetText(L"UIIT_KMT_KILLED"));
        this->m_IRM.GetResObj<CIFStatic>(4, 1)->m_FontTexture.SetColor(D3DCOLOR_ARGB(255, 255, 0, 0));
    }
    else if(state == 1)
    {
        this->m_IRM.GetResObj<CIFStatic>(4, 1)->SetText(KmtGetText(L"UIIT_KMT_ALIVE"));
        this->m_IRM.GetResObj<CIFStatic>(4, 1)->m_FontTexture.SetColor(D3DCOLOR_ARGB(255, 0, 255, 0));
    }
    else
    {
        this->m_IRM.GetResObj<CIFStatic>(4, 1)->SetText(KmtGetText(L"UIIT_KMT_UNKNOWN"));
        this->m_IRM.GetResObj<CIFStatic>(4, 1)->m_FontTexture.SetColor(D3DCOLOR_ARGB(255, 255, 255, 255));
    }

    times = time;

    std::time_t currentTime = std::time(NULL);
    __int64 elapsedTime = static_cast<__int64>(currentTime) - time;
    if (elapsedTime < 0) elapsedTime = 0;

    int days = static_cast<int>(elapsedTime / (60 * 60 * 24));
    int hours = static_cast<int>((elapsedTime % (60 * 60 * 24)) / (60 * 60));
    int minutes = static_cast<int>((elapsedTime % (60 * 60)) / 60);

    wchar_t buffer12[255];
    swprintf_s(buffer12, 255, KmtGetText(L"UIIT_KMT_VALUE_D_VALUE_H_VALUE_M_AGO"), days, hours, minutes);

    this->m_IRM.GetResObj<CIFStatic>(5, 1)->SetText(buffer12);
    this->m_IRM.GetResObj<CIFStatic>(6, 1)->SetText(killer);

    KilledRegID = RegionID;
    X = KilledX;
    Z = KilledZ;
    Y = KilledY;
    WorldID = WorldIDx;
    MapType = MapTypex;
    MapIndex = MapIndexx;
}

int CIFUniqueHistorySlot::OnMouseLeftUp(int a1, int x, int y) {
    CIFUniqueHistory * HistoryWnd = g_pCGInterface->m_IRM.GetResObj<CIFUniqueHistory>(UniqueHistoryID, 1);
    if (!HistoryWnd)
        return true;

    HistoryWnd->ClearDDJ();
    HistoryWnd->SelectedUniqueID = UniqueID;
    SelectDDJ();
    if (KilledRegID != 0)
    {
        HistoryWnd->SelectedRegionID = KilledRegID;
        HistoryWnd->SelectedX = X;
        HistoryWnd->SelectedZ = Z;
        HistoryWnd->SelectedY = Y;
        HistoryWnd->SelectedWorldID = WorldID;
        HistoryWnd->SelectedMapType = MapType;
        HistoryWnd->SelectedMapIndex = MapIndex;

        wnd_pos pos = HistoryWnd->GetPos();
        CNIFWorldMap* worldMap = g_pCGInterface->GetCNIFWorldMap();
        if (worldMap != NULL)
        {
            // Match the working Unique History map sequence used by the clean
            // client source: show the existing map, normalize it to the large
            // field page, initialize that page, then navigate to the selected
            // region and its raw world position in one synchronous operation.
            worldMap->GetRegionTypeMaybe(KilledRegID, X, Y, Z);
            if (!worldMap->IsVisible())
                worldMap->ShowGWnd(true);

            if (worldMap->MapType1IsSmall0IsBig == 0)
                worldMap->SetMapMode();

            worldMap->MoveGWnd(pos.x-270, pos.y);
            worldMap->BringToFront();

            if (KilledRegID <= SHRT_MAX)
            {
                const int regionX = KilledRegID & 0xff;
                const int regionY = static_cast<unsigned int>(KilledRegID) >> 8;
                const int positionX = static_cast<int>(X / 10.0f);
                const int positionZ = static_cast<int>(Z / 10.0f);

                worldMap->CenterFieldMapAt(regionX, regionY, positionX, positionZ);
            }
        }
    }

    wnd_pos pos = HistoryWnd->GetPos();
    CIFDps* pDps = g_pCGInterface->m_IRM.GetResObj<CIFDps>(DPSID, 1);
    if (pDps)
    {
        pDps->Clear();
        pDps->SetUniqueName(UniqName.c_str());
        std::map<int, CIFUniqueHistory::UniqueHistory>::iterator history =
                HistoryWnd->UniqueHistoryList.find(UniqueID);
        if (history != HistoryWnd->UniqueHistoryList.end() && !history->second.DpsMeter.empty())
        {
            for (size_t i = 0; i < history->second.DpsMeter.size() && i < 8; ++i)
            {
                pDps->WriteLine(static_cast<BYTE>(i + 1),
                        history->second.DpsMeter[i].first.c_str(),
                        history->second.DpsMeter[i].second.c_str());
            }

            int dpsX = pos.x + 542;
            int dpsY = pos.y;
            if (g_CGame != NULL && g_CGame->GetRes().res != NULL)
            {
                if (dpsX + pDps->GetSize().width > g_CGame->GetRes().res->width)
                    dpsX = pos.x - pDps->GetSize().width;
                if (dpsX < 0) dpsX = 0;
                if (dpsY + pDps->GetSize().height > g_CGame->GetRes().res->height)
                    dpsY = g_CGame->GetRes().res->height - pDps->GetSize().height;
                if (dpsY < 0) dpsY = 0;
            }
            pDps->MoveGWnd(dpsX, dpsY);
            pDps->ShowGWnd(true);
            pDps->BringToFront();
        }
        else
        {
            pDps->ShowGWnd(false);
        }
    }

    return true;
}

void CIFUniqueHistorySlot::SelectDDJ()
{
    CIFBarWnd* pBar = this->m_IRM.GetResObj<CIFBarWnd>(7, 1);
    if (pBar)
        pBar->TB_Func_13("interface\\ifcommon\\com_bar01select_", 1, 1);
}

void CIFUniqueHistorySlot::ClearDDJ()
{
    CIFBarWnd* pBar = this->m_IRM.GetResObj<CIFBarWnd>(7, 1);
    if (pBar)
    {
        pBar->TB_Func_13("interface\\ifcommon\\com_bar01_", 0, 0);
    }
}

void CIFUniqueHistorySlot::Clear()
{
    this->m_IRM.GetResObj(3, 1)->SetText(L"");
    this->m_IRM.GetResObj(4, 1)->SetText(L"");
    this->m_IRM.GetResObj(5, 1)->SetText(L"");
    this->m_IRM.GetResObj(6, 1)->SetText(L"");

    KilledRegID = 0;
    X = 0;
    Z = 0;
    Y = 0;
    WorldID = 0;
    MapType = 0;
    MapIndex = 0;
    times = 0;
    UniqName = L"";
    UniqueID = 0;
}
