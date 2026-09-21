#include "IFSavedLocation.h"
#include <ctime>
#include <Windows.h>
#include <ShellAPI.h>
#include <ICPlayer.h>
#include "ClientNet/MsgStreamBuffer.h"
#include <BSLib\multibyte.h>
#include <TextStringManager.h>
#include <sstream>
#include <GInterface.h>
#include <BSLib/Debug.h>
#include <Game.h>
#include <CustomData/CustomCICPlayer.h>

#define GDR_TELEPORT_LOCATION_1 4
#define GDR_SAVE_LOCATION_1 5
#define GDR_REMOVE_LOCATION_1 6

#define GDR_TELEPORT_LOCATION_2 7
#define GDR_SAVE_LOCATION_2 8
#define GDR_REMOVE_LOCATION_2 9

#define GDR_TELEPORT_LOCATION_3 10
#define GDR_SAVE_LOCATION_3 11
#define GDR_REMOVE_LOCATION_3 12


#define GDR_TELEPORT_LOCATION_4 13
#define GDR_SAVE_LOCATION_4 14
#define GDR_REMOVE_LOCATION_4 15

#define GDR_CANCEL 16

#define GDR_TELEPORT_LOCATION_1 4
#define GDR_SAVE_LOCATION_1 5
#define GDR_REMOVE_LOCATION_1 6




GFX_IMPLEMENT_DYNCREATE(CIFSavedLocation, CIFMainFrame)
GFX_BEGIN_MESSAGE_MAP(CIFSavedLocation, CIFMainFrame)
                    ONG_COMMAND(GDR_SAVE_LOCATION_1, &CIFSavedLocation::SaveButton1Click)
                    ONG_COMMAND(GDR_SAVE_LOCATION_2, &CIFSavedLocation::SaveButton2Click)
                    ONG_COMMAND(GDR_SAVE_LOCATION_3, &CIFSavedLocation::SaveButton3Click)
                    ONG_COMMAND(GDR_SAVE_LOCATION_4, &CIFSavedLocation::SaveButton4Click)
                    ONG_COMMAND(GDR_TELEPORT_LOCATION_1, &CIFSavedLocation::Teleport1Loc)
                    ONG_COMMAND(GDR_TELEPORT_LOCATION_2, &CIFSavedLocation::Teleport2Loc)
                    ONG_COMMAND(GDR_TELEPORT_LOCATION_3, &CIFSavedLocation::Teleport3Loc)
                    ONG_COMMAND(GDR_TELEPORT_LOCATION_4, &CIFSavedLocation::Teleport4Loc)
                    ONG_COMMAND(GDR_REMOVE_LOCATION_1, &CIFSavedLocation::Remove1Loc)
                    ONG_COMMAND(GDR_REMOVE_LOCATION_2, &CIFSavedLocation::Remove2Loc)
                    ONG_COMMAND(GDR_REMOVE_LOCATION_3, &CIFSavedLocation::Remove3Loc)
                    ONG_COMMAND(GDR_REMOVE_LOCATION_4, &CIFSavedLocation::Remove4Loc)
                    ONG_COMMAND(GDR_CANCEL, &CIFSavedLocation::CloseWnd)
GFX_END_MESSAGE_MAP()


CIFSavedLocation::CIFSavedLocation(void)
{
    BS_DEBUG_LOW(">" __FUNCTION__);
    SavedLocation_1 = 0;
    SavedLocation_2 = 0;
    SavedLocation_3 = 0;
    SavedLocation_4 = 0;
    m_SavedLocation = std::map<byte, SavedLocation_Data>();
}
CIFSavedLocation::~CIFSavedLocation(void)
{
    BS_DEBUG_LOW(">" __FUNCTION__);
}

bool CIFSavedLocation::OnCreate(long ln)
{
    //printf("> " __FUNCTION__ "(%d)\n", ln);

    // Populate inherited members
    CIFMainFrame::OnCreate(ln);

    this->m_IRM.LoadFromFile("clientlibrary\\resinfo\\ifsavedlocation.txt");
    this->m_IRM.CreateInterfaceSection("Create", this);
this->SetText(KmtGetText(L"UIIT_KMT_MOVE_TO_THE_USER_S_SET_LOCATION"));
    this->m_IRM.GetResObj(16, 1)->SetText(KmtGetText(L"UIIT_KMT_CANCEL"));
    this->m_IRM.GetResObj(13, 1)->SetText(KmtGetText(L"UIIT_KMT_NONE_A06007EB"));
    this->m_IRM.GetResObj(10, 1)->SetText(KmtGetText(L"UIIT_KMT_NONE_A06007EB"));
    this->m_IRM.GetResObj(7, 1)->SetText(KmtGetText(L"UIIT_KMT_NONE_A06007EB"));
    this->m_IRM.GetResObj(4, 1)->SetText(KmtGetText(L"UIIT_KMT_NONE_A06007EB"));

    this->m_IRM.GetResObj(3, 1)->SetText(KmtGetText(L"UIIT_KMT_SAVE_AND_MOVE_TO_YOUR_DESIRED_LOCATION"));

    this->m_pCloseBtn->ShowGWnd(false);


    this->ShowGWnd(false);
    return true;
}

void CIFSavedLocation::OnUpdate()
{
}
void CIFSavedLocation::CloseWnd()
{
    //this->EraseWindowObj(this);
    m_Player->ReverseSlot = 9999;
    this->ShowGWnd(false);
}

void CIFSavedLocation::Remove1Loc() {
    if(m_SavedLocation.find(1) == m_SavedLocation.end())
    {
        return;
    }
    else
    {
        m_SavedLocation.erase(1);
        CMsgStreamBuffer buf(0x200D);
        buf << byte(1);
        SendMsg(buf);
        this->m_IRM.GetResObj<CIFButton>(GDR_TELEPORT_LOCATION_1, 1)->SetText(KmtGetText(L"UIIT_KMT_NONE_A06007EB"));
    }

}

void CIFSavedLocation::Remove2Loc() {
    if(m_SavedLocation.find(2) == m_SavedLocation.end())
    {
        return;
    }
    else
    {
        m_SavedLocation.erase(2);
        CMsgStreamBuffer buf(0x200D);
        buf << byte(2);
        SendMsg(buf);
        this->m_IRM.GetResObj<CIFButton>(GDR_TELEPORT_LOCATION_2, 1)->SetText(KmtGetText(L"UIIT_KMT_NONE_A06007EB"));
    }
}

void CIFSavedLocation::Remove3Loc() {
    if(m_SavedLocation.find(3) == m_SavedLocation.end())
    {
        return;
    }
    else
    {
        m_SavedLocation.erase(3);
        CMsgStreamBuffer buf(0x200D);
        buf << byte(3);
        SendMsg(buf);
        this->m_IRM.GetResObj<CIFButton>(GDR_TELEPORT_LOCATION_3, 1)->SetText(KmtGetText(L"UIIT_KMT_NONE_A06007EB"));
    }
}
void CIFSavedLocation::Remove4Loc() {
    if(m_SavedLocation.find(4) == m_SavedLocation.end())
    {
        return;
    }
    else
    {
        m_SavedLocation.erase(4);
        CMsgStreamBuffer buf(0x200D);
        buf << byte(4);
        SendMsg(buf);
        this->m_IRM.GetResObj<CIFButton>(GDR_TELEPORT_LOCATION_4, 1)->SetText(KmtGetText(L"UIIT_KMT_NONE_A06007EB"));
    }
}

void CIFSavedLocation::LoadLocations()
{
    for(std::map<byte, SavedLocation_Data>::iterator it =  m_SavedLocation.begin();
        it != m_SavedLocation.end(); it++)
    {
        if(it->second.LocationID == 1)
        {
            std::wstringstream Reg;
            Reg << it->second.RegionID;
            std::n_wstring Name = g_CTextStringManager->GetString2(Reg.str().c_str())->c_str();
            this->m_IRM.GetResObj<CIFButton>(GDR_TELEPORT_LOCATION_1, 1)->SetText(Name.c_str());
        }
        else if(it->second.LocationID == 2)
        {
            std::wstringstream Reg;
            Reg << it->second.RegionID;
            std::n_wstring Name = g_CTextStringManager->GetString2(Reg.str().c_str())->c_str();
            this->m_IRM.GetResObj<CIFButton>(GDR_TELEPORT_LOCATION_2, 1)->SetText(Name.c_str());
        }
        else if(it->second.LocationID == 3)
        {
            std::wstringstream Reg;
            Reg << it->second.RegionID;
            std::n_wstring Name = g_CTextStringManager->GetString2(Reg.str().c_str())->c_str();
            this->m_IRM.GetResObj<CIFButton>(GDR_TELEPORT_LOCATION_3, 1)->SetText(Name.c_str());
        }
        else if(it->second.LocationID == 4)
        {
            std::wstringstream Reg;
            Reg << it->second.RegionID;
            std::n_wstring Name = g_CTextStringManager->GetString2(Reg.str().c_str())->c_str();
            this->m_IRM.GetResObj<CIFButton>(GDR_TELEPORT_LOCATION_4, 1)->SetText(Name.c_str());
        }
    }
}
void CIFSavedLocation::SetButtonText(byte LocationID, int RegionID)
{
    std::wstringstream Reg;
    Reg << RegionID;
    std::n_wstring Name = g_CTextStringManager->GetString2(Reg.str().c_str())->c_str();
    if(LocationID == 1)
    {
        this->m_IRM.GetResObj<CIFButton>(GDR_TELEPORT_LOCATION_1, 1)->SetText(Name.c_str());
    }
    else if(LocationID == 2)
    {
        this->m_IRM.GetResObj<CIFButton>(GDR_TELEPORT_LOCATION_2, 1)->SetText(Name.c_str());
    }
    else if(LocationID == 3)
    {
        this->m_IRM.GetResObj<CIFButton>(GDR_TELEPORT_LOCATION_3, 1)->SetText(Name.c_str());
    }
    else if(LocationID == 4)
    {
        this->m_IRM.GetResObj<CIFButton>(GDR_TELEPORT_LOCATION_4, 1)->SetText(Name.c_str());
    }
}
void CIFSavedLocation::Send181C(byte telslot, byte itemslot)
{
    if(g_pMyPlayerObj->ScrollStateMaybe == 1 && (g_pMyPlayerObj->CHARACTER_STATUS == Dead || g_pMyPlayerObj->CHARACTER_STATUS == Stall))
    {
        g_pCGInterface->ShowMessage_Warning(KmtGetText(L"UIIT_KMT_CANNOT_CHOOSE_ANOTHER_TELEPORT_METHOD_BECAUSE_YOU_ARE_NOW_BEING_TELEPORTED"));
        g_pCGInterface->ShowLogMessage(0xFFDBC99B, KmtGetText(L"UIIT_KMT_CANNOT_CHOOSE_ANOTHER_TELEPORT_METHOD_BECAUSE_YOU_ARE_NOW_BEING_TELEPORTED"));
        return;
    }
    if (itemslot >= 0 && itemslot <= 95)
    {

        CIFMainPopup* popup = g_pCGInterface->GetMainPopup();
        CIFInventory* inventory = popup->GetInventory();

        CSOItem* item = inventory->GetItemBySlot(itemslot);

        if (item->m_blValid == 0) {
            return;
        }

        const SItemData* pItemData = item->GetItemData();

        if (pItemData->IsReverse())
        {

            CMsgStreamBuffer buf(0x181C);
            buf << telslot;
            buf << itemslot + 13;
            SendMsg(buf);
            m_Player->ReverseSlot = 9999;
        }
        else
        {
            g_pCGInterface->WriteSystemMessage(SYSLOG_NONE, KmtGetText(L"UIIT_KMT_THE_ITEM_CANNOT_BE_USED_ON_WRONG_OBJECT"));
        }
    }
}
void CIFSavedLocation::Teleport1Loc() {
    if (m_SavedLocation.find(1) != m_SavedLocation.end()) {
        this->ShowGWnd(false);
        Send181C(1, m_Player->ReverseSlot);
    }
}

void CIFSavedLocation::Teleport2Loc()
{
    if (m_SavedLocation.find(2) != m_SavedLocation.end()) {
        this->ShowGWnd(false);
        Send181C(2, m_Player->ReverseSlot);
    }
}
void CIFSavedLocation::Teleport3Loc()
{
    if (m_SavedLocation.find(3) != m_SavedLocation.end()) {
        this->ShowGWnd(false);
        Send181C(3, m_Player->ReverseSlot);
    }
}
void CIFSavedLocation::Teleport4Loc()
{
    if (m_SavedLocation.find(4) != m_SavedLocation.end()) {
        this->ShowGWnd(false);
        Send181C(4, m_Player->ReverseSlot);
    }
}
void CIFSavedLocation::SaveButton1Click() {

    if(m_SavedLocation.find(1) != m_SavedLocation.end())
    {
        g_pCGInterface->ShowMessage_Warning(KmtGetText(L"UIIT_KMT_ALREADY_USED_THIS_SLOT"));
        g_pCGInterface->ShowLogMessage(0xFFDBC99B, KmtGetText(L"UIIT_KMT_ALREADY_USED_THIS_SLOT"));
        return;
    }
    else
    {
        D3DVECTOR CurPos = g_pMyPlayerObj->GetLocation();
        int WorldID = g_pMyPlayerObj->GetWorldID();
        if(WorldID == 1)
        {
            byte LocationID = 1;
            CMsgStreamBuffer buf(0x200C);
            buf << LocationID;
            buf << g_pMyPlayerObj->GetRegion().r << int(CurPos.x) << int(CurPos.y) << int(CurPos.z);
            buf << WorldID;
            SendMsg(buf);

            SavedLocation_Data Data = SavedLocation_Data();
            Data.LocationID = LocationID;
            Data.RegionID = g_pMyPlayerObj->GetRegion().r;
            Data.WorldID = WorldID;
            m_SavedLocation.insert((std::make_pair(LocationID, Data)));
        }
        else
        {
            g_pCGInterface->ShowMessage_Warning(KmtGetText(L"UIIT_KMT_YOU_RE_CANNOT_SAVE_SPECIAL_REGIONS"));
            g_pCGInterface->ShowLogMessage(0xFFDBC99B, KmtGetText(L"UIIT_KMT_YOU_RE_CANNOT_SAVE_SPECIAL_REGIONS"));
        }
    }

}

void CIFSavedLocation::SaveButton2Click() {
    if(m_SavedLocation.find(2) != m_SavedLocation.end())
    {
        g_pCGInterface->ShowMessage_Warning(KmtGetText(L"UIIT_KMT_ALREADY_USED_THIS_SLOT"));
        g_pCGInterface->ShowLogMessage(0xFFDBC99B, KmtGetText(L"UIIT_KMT_ALREADY_USED_THIS_SLOT"));
        return;
    }
    else
    {
        D3DVECTOR CurPos = g_pMyPlayerObj->GetLocation();
        int WorldID = g_pMyPlayerObj->GetWorldID();
        if(WorldID == 1)
        {
            byte LocationID = 2;
            CMsgStreamBuffer buf(0x200C);
            buf << LocationID;
            buf << g_pMyPlayerObj->GetRegion().r << int(CurPos.x) << int(CurPos.y) << int(CurPos.z);
            buf << WorldID;
            SendMsg(buf);

            SavedLocation_Data Data = SavedLocation_Data();
            Data.LocationID = LocationID;
            Data.RegionID = g_pMyPlayerObj->GetRegion().r;
            Data.WorldID = WorldID;
            m_SavedLocation.insert((std::make_pair(LocationID, Data)));
        }
        else
        {
            g_pCGInterface->ShowMessage_Warning(KmtGetText(L"UIIT_KMT_YOU_RE_CANNOT_SAVE_SPECIAL_REGIONS"));
            g_pCGInterface->ShowLogMessage(0xFFDBC99B, KmtGetText(L"UIIT_KMT_YOU_RE_CANNOT_SAVE_SPECIAL_REGIONS"));
        }
    }
}
void CIFSavedLocation::SaveButton3Click() {
    if(m_SavedLocation.find(3) != m_SavedLocation.end())
    {
        g_pCGInterface->ShowMessage_Warning(KmtGetText(L"UIIT_KMT_ALREADY_USED_THIS_SLOT"));
        g_pCGInterface->ShowLogMessage(0xFFDBC99B, KmtGetText(L"UIIT_KMT_ALREADY_USED_THIS_SLOT"));
        return;
    }
    else
    {
        D3DVECTOR CurPos = g_pMyPlayerObj->GetLocation();
        int WorldID = g_pMyPlayerObj->GetWorldID();
        if(WorldID == 1)
        {
            byte LocationID = 3;
            CMsgStreamBuffer buf(0x200C);
            buf << LocationID;
            buf << g_pMyPlayerObj->GetRegion().r << int(CurPos.x) << int(CurPos.y) << int(CurPos.z);
            buf << WorldID;
            SendMsg(buf);

            SavedLocation_Data Data = SavedLocation_Data();
            Data.LocationID = LocationID;
            Data.RegionID = g_pMyPlayerObj->GetRegion().r;
            Data.WorldID = WorldID;
            m_SavedLocation.insert((std::make_pair(LocationID, Data)));
        }
        else
        {
            g_pCGInterface->ShowMessage_Warning(KmtGetText(L"UIIT_KMT_YOU_RE_CANNOT_SAVE_SPECIAL_REGIONS"));
            g_pCGInterface->ShowLogMessage(0xFFDBC99B, KmtGetText(L"UIIT_KMT_YOU_RE_CANNOT_SAVE_SPECIAL_REGIONS"));
        }
    }
}

void CIFSavedLocation::SaveButton4Click() {
    if(m_SavedLocation.find(4) != m_SavedLocation.end())
    {
        g_pCGInterface->ShowMessage_Warning(KmtGetText(L"UIIT_KMT_ALREADY_USED_THIS_SLOT"));
        g_pCGInterface->ShowLogMessage(0xFFDBC99B, KmtGetText(L"UIIT_KMT_ALREADY_USED_THIS_SLOT"));
        return;
    }
    else
    {
        D3DVECTOR CurPos = g_pMyPlayerObj->GetLocation();
        int WorldID = g_pMyPlayerObj->GetWorldID();
        if(WorldID == 1)
        {
            byte LocationID = 4;
            CMsgStreamBuffer buf(0x200C);
            buf << LocationID;
            buf << g_pMyPlayerObj->GetRegion().r << int(CurPos.x) << int(CurPos.y) << int(CurPos.z);
            buf << WorldID;
            SendMsg(buf);

            SavedLocation_Data Data = SavedLocation_Data();
            Data.LocationID = LocationID;
            Data.RegionID = g_pMyPlayerObj->GetRegion().r;
            Data.WorldID = WorldID;
            m_SavedLocation.insert((std::make_pair(LocationID, Data)));
        }
        else
        {
            g_pCGInterface->ShowMessage_Warning(KmtGetText(L"UIIT_KMT_YOU_RE_CANNOT_SAVE_SPECIAL_REGIONS"));
            g_pCGInterface->ShowLogMessage(0xFFDBC99B, KmtGetText(L"UIIT_KMT_YOU_RE_CANNOT_SAVE_SPECIAL_REGIONS"));
        }
    }
}


void CIFSavedLocation::UpdateMenuSize()
{
    USHORT PosX = 0, PosY = 0;
    PosY = (g_CGame->GetRes().res->height/2) - (this->GetSize().height/2);
    PosX = (g_CGame->GetRes().res->width/2) - (this->GetSize().width/2);
    this->MoveGWnd(PosX, PosY);
    BringToFront();
}

undefined1 CIFSavedLocation::OnCloseWnd(){
    m_Player->ReverseSlot = 9999;
    return CIFWnd::OnCloseWnd();
}
