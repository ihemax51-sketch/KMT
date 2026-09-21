#include "IFMovePartyMember.h"
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
#include <CustomData/CustomCICPlayer.h>


#define GDR_CANCEL 4
#define GDR_GRANTNAME_EDIT 7
#define GDR_GRANTNAME_BG3 52
#define GDR_GRANTNAME_BG2 51
#define GDR_GRANTNAME_BG1 50
#define GDR_GRANTNAME_LABEL 53


GFX_IMPLEMENT_DYNCREATE(CIFMovePartyMember, CIFMainFrame)

GFX_BEGIN_MESSAGE_MAP(CIFMovePartyMember, CIFMainFrame)
                    ONG_COMMAND(GDR_CANCEL, &On_BtnClick)
GFX_END_MESSAGE_MAP()

CIFMovePartyMember::CIFMovePartyMember(void){
    replace = -1;
}
CIFMovePartyMember::~CIFMovePartyMember(void){

}

bool CIFMovePartyMember::OnCreate(long ln)
{

    // Populate inherited members
    CIFMainFrame::OnCreate(ln);
    m_IRM.LoadFromFile("clientlibrary\\resinfo\\ifmovepartymember.txt");
    m_IRM.CreateInterfaceSection("Create", this);

    this->SetText(KmtGetText(L"UIIT_KMT_MOVE_TO_PARTY_MEMBER"));
    this->m_IRM.GetResObj(7, 1)->SetText(KmtGetText(L"UIIT_KMT_MOVE"));
    this->m_IRM.GetResObj(6, 1)->SetText(KmtGetText(L"UIIT_KMT_LOCATION"));
    this->m_IRM.GetResObj(5, 1)->SetText(KmtGetText(L"UIIT_KMT_CHARACTER_NAME"));
    this->m_IRM.GetResObj(4, 1)->SetText(KmtGetText(L"UIIT_KMT_CANCEL"));


    wnd_rect sz;
    sz.pos.x = 22;
    sz.pos.y = 71;
    sz.size.width = 412;
    sz.size.height = 36;
    for(int i =0; i < 7;i++) {
        pSlot[i] = (CIFMovePartyMemberSlot*)CGWnd::CreateInstance(this, GFX_RUNTIME_CLASS(CIFMovePartyMemberSlot), sz, 301 + i, 0);
        pSlot[i]->ShowGWnd(true);
        sz.pos.y += 33;
    }
    this->ShowGWnd(false);
    return true;
}

void CIFMovePartyMember::UpdateMenuSize()
{

    int PosX = 0, PosY = 0;
    PosY = (g_CGame->GetRes().res->height/2) - (this->GetSize().height/2);
    PosX = (g_CGame->GetRes().res->width/2) - (this->GetSize().width/2);
    this->MoveGWnd(PosX, PosY);
    BringToFront();
}


void CIFMovePartyMember::OnUpdate()
{
    const SPartyData& partyData = g_CCharacterDependentData.GetPartyData();


    if (replace != partyData.NumberOfMembers)
    {

        for(int i =0; i< 7;i++) {
            pSlot[i]->Clear();
        }

        replace = partyData.NumberOfMembers;


        if (partyData.NumberOfMembers > 0) {

            int i2 = -1;
            for (int i = 0; i < partyData.NumberOfMembers; ++i) {

                const SPartyMemberData& memberData = g_CCharacterDependentData.GetPartyMemberData(i);
                std::n_wstring test1 = memberData.m_charactername.c_str();
                std::n_wstring test2 = g_pMyPlayerObj->GetCharName().c_str();
                std::wstringstream region;
                region << memberData.region.r;
                const std::n_wstring *nwRegionName = g_CTextStringManager->GetString2(region.str().c_str());

                std::n_wstring TitleNames = nwRegionName->c_str();
                if (TitleNames.empty())
                    TitleNames = L"</>";

                if (test1 != test2)
                    ++i2;
                else
                    continue;

                switch (i2)
                {
                    case 0:
                    {

                        pSlot[0]->SetName(memberData.m_charactername.c_str(), TitleNames.c_str(), memberData.test1);
                    }break;
                    case 1:
                    {
                        pSlot[1]->SetName(memberData.m_charactername.c_str(), TitleNames.c_str(), memberData.test1);
                    }break;
                    case 2:
                    {
                        pSlot[2]->SetName(memberData.m_charactername.c_str(), TitleNames.c_str(), memberData.test1);
                    }break;
                    case 3:
                    {
                        pSlot[3]->SetName(memberData.m_charactername.c_str(), TitleNames.c_str(), memberData.test1);
                    }break;
                    case 4:
                    {
                        pSlot[4]->SetName(memberData.m_charactername.c_str(), TitleNames.c_str(), memberData.test1);
                    }break;
                    case 5:
                    {
                        pSlot[5]->SetName(memberData.m_charactername.c_str(), TitleNames.c_str(), memberData.test1);
                    }break;
                    case 6:
                    {
                        pSlot[6]->SetName(memberData.m_charactername.c_str(), TitleNames.c_str(), memberData.test1);
                    }break;
                    case 7:
                    {
                        pSlot[7]->SetName(memberData.m_charactername.c_str(), TitleNames.c_str(), memberData.test1);
                    }break;

                    default:
                        break;
                }
            }
        }


    }
}
void CIFMovePartyMember::Clear()
{

}
void CIFMovePartyMember::On_BtnClick() {
    this->OnCloseWnd();
}
undefined1 CIFMovePartyMember::OnCloseWnd(){
    m_Player->ReverseSlot = 9999;
    return CIFWnd::OnCloseWnd();
}
