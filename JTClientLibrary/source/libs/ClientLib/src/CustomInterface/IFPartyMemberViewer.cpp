#include "IFPartyMemberViewer.h"
#include "Game.h"
#include "IFPartyMatch.h"
#include <GInterface.h>
#include <BSLib/Debug.h>


GFX_IMPLEMENT_DYNCREATE(CIFPartyMemberViewer, CIFMainFrame)

CIFPartyMemberViewer::CIFPartyMemberViewer(void) {
    BS_DEBUG_LOW(">" __FUNCTION__);
}
CIFPartyMemberViewer::~CIFPartyMemberViewer(void) {
    BS_DEBUG_LOW(">" __FUNCTION__);
}


bool CIFPartyMemberViewer::OnCreate(long ln) {

    // Populate inherited members
    CIFMainFrame::OnCreate(ln);
    m_IRM.LoadFromFile("clientlibrary\\resinfo\\ifpartymemberviewer.txt");
    m_IRM.CreateInterfaceSection("Create", this);


    m_pCloseBtn->ShowGWnd(false);
    m_pHandleBar->ShowGWnd(false);
    //   TB_Func_13("interface\\frame\\frame_tooltip_", 1, 0);


    wnd_rect sz;
    /*  bars = new CIFStatic * [8];
      sz.pos.x = 7;
      sz.pos.y = 7;
      sz.size.width = 141;
      sz.size.height = 28;
      for (int i = 0; i < 8; i++)
      {
          bars[i] = (CIFStatic*)CGWnd::CreateInstance(this, GFX_RUNTIME_CLASS(CIFStatic), sz, 2+i, 1);
          bars[i]->TB_Func_13("interface\\party\\pt_tooltip_listbar.ddj", 1, 0);
          sz.pos.y += 28;
      }
  */

    //
    sz.pos.x = 34;
    sz.pos.y = 13;
    sz.size.width = 60;
    sz.size.height = 16;
    for (int i = 0; i < 8; i++)
    {
        levelslot[i] = (CIFStatic*)CGWnd::CreateInstance(this, GFX_RUNTIME_CLASS(CIFStatic), sz, 12 + i, 1);
        sz.pos.y += 28;
    }

    sz.pos.x = 13;
    sz.pos.y = 15;
    sz.size.width = 16;
    sz.size.height = 16;
    for (int i = 0; i < 8; i++)
    {
        raceslot[i] = (CIFStatic*)CGWnd::CreateInstance(this, GFX_RUNTIME_CLASS(CIFStatic), sz, 24 + i, 1);
        sz.pos.y += 28;
    }


    sz.pos.x = 95;
    sz.pos.y = 9;
    sz.size.width = 24;
    sz.size.height = 24;
    for (int i = 0; i < 8; i++)
    {
        masteryslot1[i] = (CIFStatic*)CGWnd::CreateInstance(this, GFX_RUNTIME_CLASS(CIFStatic), sz, 36 + i, 1);
        sz.pos.y += 28;
    }

    sz.pos.x = 121;
    sz.pos.y = 9;
    sz.size.width = 24;
    sz.size.height = 24;
    for (int i = 0; i < 8; i++)
    {
        masteryslot2[i] = (CIFStatic*)CGWnd::CreateInstance(this, GFX_RUNTIME_CLASS(CIFStatic), sz, 48 + i, 1);
        sz.pos.y += 28;
    }


    UpdateMenuSize();
    this->ShowGWnd(false);


    return true;
}

void CIFPartyMemberViewer::OnUpdate()
{

    CIFPartyMatch * test = g_pCGInterface->m_IRM.GetResObj<CIFPartyMatch>(130, 1);

    if (test == NULL)
    {
        this->ShowGWnd(false);
    }
}
void CIFPartyMemberViewer::WriteRace(int ObjID, int SlotID)
{
    switch (ObjID)
    {
        case 1907: {
            raceslot[SlotID]->TB_Func_13("interface\\ifcommon\\ifcommon\\com_kindred_china16.ddj", 0,0);

        }
            break;
        case 1908: {
            raceslot[SlotID]->TB_Func_13("interface\\ifcommon\\ifcommon\\com_kindred_china16.ddj", 0, 0);

        }
            break;
        case 1909: {
            raceslot[SlotID]->TB_Func_13("interface\\ifcommon\\ifcommon\\com_kindred_china16.ddj", 0, 0);

        }
            break;
        case 1910: {
            raceslot[SlotID]->TB_Func_13("interface\\ifcommon\\ifcommon\\com_kindred_china16.ddj", 0, 0);

        }
            break;
        case 1911: {
            raceslot[SlotID]->TB_Func_13("interface\\ifcommon\\ifcommon\\com_kindred_china16.ddj", 0, 0);

        }
            break;
        case 1912: {
            raceslot[SlotID]->TB_Func_13("interface\\ifcommon\\ifcommon\\com_kindred_china16.ddj", 0, 0);

        }
            break;
        case 1913: {
            raceslot[SlotID]->TB_Func_13("interface\\ifcommon\\ifcommon\\com_kindred_china16.ddj", 0, 0);

        }
            break;
        case 1914: {
            raceslot[SlotID]->TB_Func_13("interface\\ifcommon\\ifcommon\\com_kindred_china16.ddj", 0, 0);

        }
            break;
        case 1915: {
            raceslot[SlotID]->TB_Func_13("interface\\ifcommon\\ifcommon\\com_kindred_china16.ddj", 0, 0);

        }
            break;
        case 1916: {
            raceslot[SlotID]->TB_Func_13("interface\\ifcommon\\ifcommon\\com_kindred_china16.ddj", 0, 0);

        }
            break;
        case 1917: {
            raceslot[SlotID]->TB_Func_13("interface\\ifcommon\\ifcommon\\com_kindred_china16.ddj", 0, 0);

        }
            break;
        case 1918: {
            raceslot[SlotID]->TB_Func_13("interface\\ifcommon\\ifcommon\\com_kindred_china16.ddj", 0, 0);

        }
            break;
        case 1919: {
            raceslot[SlotID]->TB_Func_13("interface\\ifcommon\\ifcommon\\com_kindred_china16.ddj", 0, 0);

        }
            break;
        case 1920: {
            raceslot[SlotID]->TB_Func_13("interface\\ifcommon\\ifcommon\\com_kindred_china16.ddj", 0, 0);

        }
            break;
        case 1921: {
            raceslot[SlotID]->TB_Func_13("interface\\ifcommon\\ifcommon\\com_kindred_china16.ddj", 0, 0);

        }
            break;
        case 1922: {
            raceslot[SlotID]->TB_Func_13("interface\\ifcommon\\ifcommon\\com_kindred_china16.ddj", 0, 0);

        }
            break;
        case 1923: {
            raceslot[SlotID]->TB_Func_13("interface\\ifcommon\\ifcommon\\com_kindred_china16.ddj", 0, 0);

        }
            break;
        case 1924: {
            raceslot[SlotID]->TB_Func_13("interface\\ifcommon\\ifcommon\\com_kindred_china16.ddj", 0, 0);

        }
            break;
        case 1925: {
            raceslot[SlotID]->TB_Func_13("interface\\ifcommon\\ifcommon\\com_kindred_china16.ddj", 0, 0);

        }
            break;
        case 1926: {
            raceslot[SlotID]->TB_Func_13("interface\\ifcommon\\ifcommon\\com_kindred_china16.ddj", 0, 0);

        }
            break;
        case 1927: {
            raceslot[SlotID]->TB_Func_13("interface\\ifcommon\\ifcommon\\com_kindred_china16.ddj", 0, 0);

        }
            break;
        case 1928: {
            raceslot[SlotID]->TB_Func_13("interface\\ifcommon\\ifcommon\\com_kindred_china16.ddj", 0, 0);

        }
            break;
        case 1929: {
            raceslot[SlotID]->TB_Func_13("interface\\ifcommon\\ifcommon\\com_kindred_china16.ddj", 0, 0);

        }
            break;
        case 1930: {
            raceslot[SlotID]->TB_Func_13("interface\\ifcommon\\ifcommon\\com_kindred_china16.ddj", 0, 0);

        }
            break;
        case 1931: {
            raceslot[SlotID]->TB_Func_13("interface\\ifcommon\\ifcommon\\com_kindred_china16.ddj", 0, 0);

        }
            break;
        case 1932: {
            raceslot[SlotID]->TB_Func_13("interface\\ifcommon\\ifcommon\\com_kindred_china16.ddj", 0, 0);


        }
            break;

        case 14875: {
            raceslot[SlotID]->TB_Func_13("interface\\ifcommon\\ifcommon\\com_kindred_europe16.ddj", 0, 0);

        }
            break;
        case 14876: {
            raceslot[SlotID]->TB_Func_13("interface\\ifcommon\\ifcommon\\com_kindred_europe16.ddj", 0, 0);
        }
            break;
        case 14877: {
            raceslot[SlotID]->TB_Func_13("interface\\ifcommon\\ifcommon\\com_kindred_europe16.ddj", 0, 0);
        }
            break;
        case 14878: {
            raceslot[SlotID]->TB_Func_13("interface\\ifcommon\\ifcommon\\com_kindred_europe16.ddj", 0, 0);

        }
            break;
        case 14879: {
            raceslot[SlotID]->TB_Func_13("interface\\ifcommon\\ifcommon\\com_kindred_europe16.ddj", 0, 0);
        }
            break;
        case 14880: {
            raceslot[SlotID]->TB_Func_13("interface\\ifcommon\\ifcommon\\com_kindred_europe16.ddj", 0, 0);

        }
            break;
        case 14881: {
            raceslot[SlotID]->TB_Func_13("interface\\ifcommon\\ifcommon\\com_kindred_europe16.ddj", 0, 0);
        }
            break;
        case 14882: {
            raceslot[SlotID]->TB_Func_13("interface\\ifcommon\\ifcommon\\com_kindred_europe16.ddj", 0, 0);
        }
            break;
        case 14883: {
            raceslot[SlotID]->TB_Func_13("interface\\ifcommon\\ifcommon\\com_kindred_europe16.ddj", 0, 0);
        }
            break;
        case 14884: {
            raceslot[SlotID]->TB_Func_13("interface\\ifcommon\\ifcommon\\com_kindred_europe16.ddj", 0, 0);

        }
            break;
        case 14885: {
            raceslot[SlotID]->TB_Func_13("interface\\ifcommon\\ifcommon\\com_kindred_europe16.ddj", 0, 0);
        }
            break;
        case 14886: {
            raceslot[SlotID]->TB_Func_13("interface\\ifcommon\\ifcommon\\com_kindred_europe16.ddj", 0, 0);
        }
            break;
        case 14887: {
            raceslot[SlotID]->TB_Func_13("interface\\ifcommon\\ifcommon\\com_kindred_europe16.ddj", 0, 0);
        }
            break;

        case 14888: {
            raceslot[SlotID]->TB_Func_13("interface\\ifcommon\\ifcommon\\com_kindred_europe16.ddj", 0, 0);
        }
            break;
        case 14889: {
            raceslot[SlotID]->TB_Func_13("interface\\ifcommon\\ifcommon\\com_kindred_europe16.ddj", 0, 0);
        }
            break;
        case 14890: {
            raceslot[SlotID]->TB_Func_13("interface\\ifcommon\\ifcommon\\com_kindred_europe16.ddj", 0, 0);
        }
            break;
        case 14891: {
            raceslot[SlotID]->TB_Func_13("interface\\ifcommon\\ifcommon\\com_kindred_europe16.ddj", 0, 0);
        }
            break;
        case 14892: {
            raceslot[SlotID]->TB_Func_13("interface\\ifcommon\\ifcommon\\com_kindred_europe16.ddj", 0, 0);
        }
            break;
        case 14893: {
            raceslot[SlotID]->TB_Func_13("interface\\ifcommon\\ifcommon\\com_kindred_europe16.ddj", 0, 0);
        }
            break;
        case 14894: {
            raceslot[SlotID]->TB_Func_13("interface\\ifcommon\\ifcommon\\com_kindred_europe16.ddj", 0, 0);
        }
            break;
        case 14895: {
            raceslot[SlotID]->TB_Func_13("interface\\ifcommon\\ifcommon\\com_kindred_europe16.ddj", 0, 0);
        }
            break;
        case 14896: {
            raceslot[SlotID]->TB_Func_13("interface\\ifcommon\\ifcommon\\com_kindred_europe16.ddj", 0, 0);
        }
            break;
        case 14897: {
            raceslot[SlotID]->TB_Func_13("interface\\ifcommon\\ifcommon\\com_kindred_europe16.ddj", 0, 0);
        }
            break;
        case 14898: {
            raceslot[SlotID]->TB_Func_13("interface\\ifcommon\\ifcommon\\com_kindred_europe16.ddj", 0, 0);
        }
            break;
        case 14899: {
            raceslot[SlotID]->TB_Func_13("interface\\ifcommon\\ifcommon\\com_kindred_europe16.ddj", 0, 0);
        }
            break;
        case 14900: {
            raceslot[SlotID]->TB_Func_13("interface\\ifcommon\\ifcommon\\com_kindred_europe16.ddj", 0, 0);
        }
            break;
    }
}
void CIFPartyMemberViewer::WriteMastery(int mastery1, int mastery2, int slot)
{
    // Mastery1 için atama
    if (mastery1 == 257)
        masteryslot1[slot]->TB_Func_13("icon\\skillmastery\\china\\mastery_sword.ddj", 0, 0);
    else if (mastery1 == 258)
        masteryslot1[slot]->TB_Func_13("icon\\skillmastery\\china\\mastery_spear.ddj", 0, 0);
    else if (mastery1 == 259)
        masteryslot1[slot]->TB_Func_13("icon\\skillmastery\\china\\mastery_bow.ddj", 0, 0);
    else if (mastery1 == 273)
        masteryslot1[slot]->TB_Func_13("icon\\skillmastery\\china\\mastery_cold.ddj", 0, 0);
    else if (mastery1 == 274)
        masteryslot1[slot]->TB_Func_13("icon\\skillmastery\\china\\mastery_lightning.ddj", 0, 0);
    else if (mastery1 == 275)
        masteryslot1[slot]->TB_Func_13("icon\\skillmastery\\china\\mastery_fire.ddj", 0, 0);
    else if (mastery1 == 276)
        masteryslot1[slot]->TB_Func_13("icon\\skillmastery\\china\\mastery_water.ddj", 0, 0);
    else if (mastery1 == 513)
        masteryslot1[slot]->TB_Func_13("icon\\skillmastery\\europe\\eu_warrior.ddj", 0, 0);
    else if (mastery1 == 514)
        masteryslot1[slot]->TB_Func_13("icon\\skillmastery\\europe\\eu_wizard.ddj", 0, 0);
    else if (mastery1 == 515)
        masteryslot1[slot]->TB_Func_13("icon\\skillmastery\\europe\\eu_rog.ddj", 0, 0);
    else if (mastery1 == 516)
        masteryslot1[slot]->TB_Func_13("icon\\skillmastery\\europe\\eu_warlock.ddj", 0, 0);
    else if (mastery1 == 517)
        masteryslot1[slot]->TB_Func_13("icon\\skillmastery\\europe\\eu_bard.ddj", 0, 0);
    else if (mastery1 == 518)
        masteryslot1[slot]->TB_Func_13("icon\\skillmastery\\europe\\eu_cleric.ddj", 0, 0);
    else if (mastery1 == 0)
        masteryslot1[slot]->TB_Func_13("", 0, 0);

    // Mastery2 için atama
    if (mastery2 == 257)
        masteryslot2[slot]->TB_Func_13("icon\\skillmastery\\china\\mastery_sword.ddj", 0, 0);
    else if (mastery2 == 258)
        masteryslot2[slot]->TB_Func_13("icon\\skillmastery\\china\\mastery_spear.ddj", 0, 0);
    else if (mastery2 == 259)
        masteryslot2[slot]->TB_Func_13("icon\\skillmastery\\china\\mastery_bow.ddj", 0, 0);
    else if (mastery2 == 273)
        masteryslot2[slot]->TB_Func_13("icon\\skillmastery\\china\\mastery_cold.ddj", 0, 0);
    else if (mastery2 == 274)
        masteryslot2[slot]->TB_Func_13("icon\\skillmastery\\china\\mastery_lightning.ddj", 0, 0);
    else if (mastery2 == 275)
        masteryslot2[slot]->TB_Func_13("icon\\skillmastery\\china\\mastery_fire.ddj", 0, 0);
    else if (mastery2 == 276)
        masteryslot2[slot]->TB_Func_13("icon\\skillmastery\\china\\mastery_water.ddj", 0, 0);
    else if (mastery2 == 513)
        masteryslot2[slot]->TB_Func_13("icon\\skillmastery\\europe\\eu_warrior.ddj", 0, 0);
    else if (mastery2 == 514)
        masteryslot2[slot]->TB_Func_13("icon\\skillmastery\\europe\\eu_wizard.ddj", 0, 0);
    else if (mastery2 == 515)
        masteryslot2[slot]->TB_Func_13("icon\\skillmastery\\europe\\eu_rog.ddj", 0, 0);
    else if (mastery2 == 516)
        masteryslot2[slot]->TB_Func_13("icon\\skillmastery\\europe\\eu_warlock.ddj", 0, 0);
    else if (mastery2 == 517)
        masteryslot2[slot]->TB_Func_13("icon\\skillmastery\\europe\\eu_bard.ddj", 0, 0);
    else if (mastery2 == 518)
        masteryslot2[slot]->TB_Func_13("icon\\skillmastery\\europe\\eu_cleric.ddj", 0, 0);
    else if (mastery2 == 0)
        masteryslot2[slot]->TB_Func_13("", 0, 0);
}

void CIFPartyMemberViewer::WriteLine(BYTE Num, int Race, int CharLevel, int Mastery1, int Mastery2)
{
    wchar_t buffer1[255];
    switch (Num)
    {
        case 1:

            swprintf_s(buffer1, KmtGetText(L"UIIT_KMT_LV_VALUE"), CharLevel);
            levelslot[0]->SetText(buffer1);
            WriteRace(Race, 0);
            WriteMastery(Mastery1, Mastery2, 0);
            break;
        case 2:
            swprintf_s(buffer1, KmtGetText(L"UIIT_KMT_LV_VALUE"), CharLevel);
            levelslot[1]->SetText(buffer1);
            WriteRace(Race, 1);
            WriteMastery(Mastery1, Mastery2, 1);
            break;
        case 3:
            swprintf_s(buffer1, KmtGetText(L"UIIT_KMT_LV_VALUE"), CharLevel);
            levelslot[2]->SetText(buffer1);
            WriteRace(Race, 2);
            WriteMastery(Mastery1, Mastery2, 2);
            break;
        case 4:
            swprintf_s(buffer1, KmtGetText(L"UIIT_KMT_LV_VALUE"), CharLevel);
            levelslot[3]->SetText(buffer1);
            WriteRace(Race, 3);
            WriteMastery(Mastery1, Mastery2, 3);
            break;
        case 5:
            swprintf_s(buffer1, KmtGetText(L"UIIT_KMT_LV_VALUE"), CharLevel);
            levelslot[4]->SetText(buffer1);
            WriteRace(Race, 4);
            WriteMastery(Mastery1, Mastery2, 4);
            break;
        case 6:
            swprintf_s(buffer1, KmtGetText(L"UIIT_KMT_LV_VALUE"), CharLevel);
            levelslot[5]->SetText(buffer1);
            WriteRace(Race, 5);
            WriteMastery(Mastery1, Mastery2, 5);
            break;
        case 7:
            swprintf_s(buffer1, KmtGetText(L"UIIT_KMT_LV_VALUE"), CharLevel);
            levelslot[6]->SetText(buffer1);
            WriteRace(Race, 6);
            WriteMastery(Mastery1, Mastery2, 6);
            break;
        case 8:
            swprintf_s(buffer1, KmtGetText(L"UIIT_KMT_LV_VALUE"), CharLevel);
            levelslot[7]->SetText(buffer1);
            WriteRace(Race, 7);
            WriteMastery(Mastery1, Mastery2, 7);
            break;

        default:
            break;
    }
}

void CIFPartyMemberViewer::Clear() const
{
    for (int i = 0; i < 8; i++)
    {
        levelslot[i]->SetText(L"");
        masteryslot1[i]->TB_Func_13("", 0, 0);
        masteryslot2[i]->TB_Func_13("", 0, 0);
        raceslot[i]->TB_Func_13("", 0, 0);

    }
    /* levelslot[0]->SetText(L"");
     levelslot[1]->SetText(L"");
     levelslot[2]->SetText(L"");
     levelslot[3]->SetText(L"");
     levelslot[4]->SetText(L"");
     levelslot[5]->SetText(L"");
     levelslot[6]->SetText(L"");
     levelslot[7]->SetText(L"");

     masteryslot1[0]->TB_Func_13("", 0, 0);
     masteryslot1[1]->TB_Func_13("", 0, 0);
     masteryslot1[2]->TB_Func_13("", 0, 0);
     masteryslot1[3]->TB_Func_13("", 0, 0);
     masteryslot1[4]->TB_Func_13("", 0, 0);
     masteryslot1[5]->TB_Func_13("", 0, 0);
     masteryslot1[6]->TB_Func_13("", 0, 0);
     masteryslot1[7]->TB_Func_13("", 0, 0);

     masteryslot2[0]->TB_Func_13("", 0, 0);
     masteryslot2[1]->TB_Func_13("", 0, 0);
     masteryslot2[2]->TB_Func_13("", 0, 0);
     masteryslot2[3]->TB_Func_13("", 0, 0);
     masteryslot2[4]->TB_Func_13("", 0, 0);
     masteryslot2[5]->TB_Func_13("", 0, 0);
     masteryslot2[6]->TB_Func_13("", 0, 0);
     masteryslot2[7]->TB_Func_13("", 0, 0);

     raceslot[0]->TB_Func_13("", 0, 0);
     raceslot[1]->TB_Func_13("", 0, 0);
     raceslot[2]->TB_Func_13("", 0, 0);
     raceslot[3]->TB_Func_13("", 0, 0);
     raceslot[4]->TB_Func_13("", 0, 0);
     raceslot[5]->TB_Func_13("", 0, 0);
     raceslot[6]->TB_Func_13("", 0, 0);
     raceslot[7]->TB_Func_13("", 0, 0);*/
}

void CIFPartyMemberViewer::UpdateMenuSize()
{
    wnd_rect rect = GetBounds();
    const ClientResolutonData &res = CGame::GetClientDimensionStuff();

    MoveGWnd(((res.width - GetSize().width) - 130), GetPos().y);
    BringToFront();
}
#define TIMERSHOWDPS 10002
void CIFPartyMemberViewer::OnTimer(int timerId) {
    if (timerId == TIMERSHOWDPS) {


        this->KillTimer(TIMERSHOWDPS);

        this->ShowGWnd(false);
    }
}

void CIFPartyMemberViewer::OpenDPS(int timeoutSeconds) {

    UpdateMenuSize();
    this->ShowGWnd(true);
    this->StartTimer(TIMERSHOWDPS, 10000);
}