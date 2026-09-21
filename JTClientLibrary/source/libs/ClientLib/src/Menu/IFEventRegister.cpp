#include "IFEventRegister.h"
#include <ClientNet/MsgStreamBuffer.h>
#include <GEffSoundBody.h>
#include <Game.h>
#include <ICPlayer.h>
#include <BSLib/multibyte.h>
#include "GInterface.h"
#include <CharacterDependentData.h>
#include <TextStringManager.h>
#include <GlobalDataManager.h>

#define GDR_CANCEL_BUTTON 333
#define GDR_REGISTER_BUTTON 332
GFX_IMPLEMENT_DYNCREATE(CIFEventRegister, CIFMainFrame)

GFX_BEGIN_MESSAGE_MAP(CIFEventRegister, CIFMainFrame)
                    ONG_COMMAND(GDR_REGISTER_BUTTON, &On_BtnClickReg)
                    ONG_COMMAND(GDR_CANCEL_BUTTON, &On_BtnClickCancel)
GFX_END_MESSAGE_MAP()

CIFEventRegister::CIFEventRegister(void){
    EventList = std::vector<Events>();
    descbox = NULL;
    m_scroll = NULL;
    vscroll = NULL;
    SelectedEventID = 0;
}
CIFEventRegister::~CIFEventRegister(void){
}

int CIFEventRegister::Func_4(int a2) {
    int v1 = 0;
    while (a2 != v1 + 100) {
        if (++v1 >= 5)
            return -1;
    }

    return 100;
}

int CIFEventRegister::Func_36(int a1, short action, int a3, int a4) {
    if (action <= 0) {
        if (action < 0) {
            vscroll->sub_65A5C0(0);
        }
    } else {
        vscroll->sub_65A5A0(0);
    }

    return 1;
}


bool CIFEventRegister::OnCreate(long ln)
{

    // Populate inherited members
    CIFMainFrame::OnCreate(ln);
    wnd_rect sz;

    m_IRM.LoadFromFile("clientlibrary\\resinfo\\ifeventregister.txt");
    m_IRM.CreateInterfaceSection("Create", this);

    this->SetText(KmtGetText(L"UIIT_KMT_EVENT_REGISTER"));

    m_scroll = this->m_IRM.GetResObj<CIFScrollManager>(8, 1);
    m_scroll->sub_008124F0(0);
    m_scroll->sub_008124C0(33);
    m_scroll->sub_008123F0(7);
    m_scroll->sub_00812500(0);
    m_scroll->sub_00812420(-8, 0);



    // m_mybar1 = (CIFBarWnd*) CGWnd::CreateInstance(this, GFX_RUNTIME_CLASS(CIFBarWnd), sz, GDR_CHEST_SLOT1, 0);
    //m_mybar1->TB_Func_13("interface\\ifcommon\\com_bar01_", 0 ,0);
    descbox = m_IRM.GetResObj<CIFTextBox>(331, 1);
    descbox->JustifyHorizontal(JUSTIFY_LEFT);
    descbox->JustifyVertical(JUSTIFY_MIDDLE);

    vscroll = this->m_IRM.GetResObj<CIFVerticalScroll>(89, 1);
    descbox->LinkScrollbar(vscroll);
    descbox->m_HeightInLines = 11;
    descbox->SetLineHeight(23);
    descbox->sub_638B50(0);
    descbox->sub_64E380(0);
    descbox->sub_638C70(1);
    descbox->sub_638D50(1);
    descbox->sub_638D40(1);

    this->m_IRM.GetResObj(28, 1)->SetText(KmtGetText(L"UIIT_KMT_EVENT_NAME"));
    this->m_IRM.GetResObj(25, 1)->SetText(KmtGetText(L"UIIT_KMT_DESCRIPTIONS"));
    this->m_IRM.GetResObj(332, 1)->SetText(KmtGetText(L"UIIT_KMT_REGISTER"));
    this->m_IRM.GetResObj(333, 1)->SetText(KmtGetText(L"UIIT_KMT_CANCEL"));

    sz.size.width = 191;
    sz.size.height = 34;
    for (int i = 0; i < 20; ++i)
    {
        m_slots[i] = (CIFEventRegisterSlot *) CGWnd::CreateInstance(this, GFX_RUNTIME_CLASS(CIFEventRegisterSlot), sz, 302+i, 0);

    }

    this->ShowGWnd(false);
    return true;
}

void CIFEventRegister::UpdateMenuSize()
{

    int PosX = 0, PosY = 0;
    PosY = (g_CGame->GetRes().res->height/2) - (this->GetSize().height/2);
    PosX = (g_CGame->GetRes().res->width/2) - (this->GetSize().width/2);
    this->MoveGWnd(PosX, PosY);
    BringToFront();
}


void CIFEventRegister::OnUpdate()
{

}
void CIFEventRegister::SetDescBoxText(std::n_wstring string)
{
    //  vscroll->sub_65A5A0(999);
    descbox->SetText(string.c_str());
    vscroll->sub_65A5A0(999);

}
void CIFEventRegister::Clear()
{
    for (int i = 0; i < 20; ++i)
    {
        m_scroll->DeleteItem(m_slots[i]);
    }
}

void CIFEventRegister::ClearDDJ() {
    for (int i = 0; i < 20; ++i)
    {
        m_slots[i]->ClearDDJ();
    }

}

void CIFEventRegister::On_BtnClickReg(){
    if(SelectedEventID != 0)
    {
        CMsgStreamBuffer buf(0xB301);
        buf << (BYTE)16 << SelectedEventID;
        SendMsg(buf);
        OpenRegButton(5000);
    }

}
void CIFEventRegister::On_BtnClickCancel(){
    if(SelectedEventID != 0)
    {
        CMsgStreamBuffer buf(0xB301);
        buf << (BYTE) 17 << SelectedEventID;
        SendMsg(buf);
        OpenCancelButton(5000);
    }
}
#define REGBUTON 100077
#define CANCELBUTON 100088

void CIFEventRegister::OnTimer(int timerId) {
    if (timerId == REGBUTON) {
        this->KillTimer(REGBUTON);
        this->m_IRM.GetResObj<CIFButton>(332, 1)->SetEnabledState(1);
    }
    if (timerId == CANCELBUTON) {
        this->KillTimer(CANCELBUTON);
        this->m_IRM.GetResObj<CIFButton>(333, 1)->SetEnabledState(1);
    }
}
void CIFEventRegister::OpenRegButton(int timeoutSeconds) {
    this->m_IRM.GetResObj<CIFButton>(332, 1)->SetEnabledState(0);
    this->StartTimer(REGBUTON, timeoutSeconds);
}
void CIFEventRegister::OpenCancelButton(int timeoutSeconds) {
    this->m_IRM.GetResObj<CIFButton>(333, 1)->SetEnabledState(0);
    this->StartTimer(CANCELBUTON, timeoutSeconds);
}


void CIFEventRegister::UpdateEvents(){
    int i = 0;
    for (std::vector<Events>::iterator it = EventList.begin(); it != EventList.end(); ++it)
    {
        i++;
        m_slots[i]->WriteLine((*it).DBID, (*it).EventName.c_str(), (*it).EventDesc.c_str());
        m_scroll->AddItem(m_slots[i], 1, 0);
    }
}

GFX_IMPLEMENT_DYNCREATE(CIFEventRegisterGuide, CIFDecoratedStatic)

bool CIFEventRegisterGuide::OnCreate(long ln)
{
    CIFDecoratedStatic::OnCreate(ln);

    TB_Func_13("clientlibrary\\guides\\kmt_event_register_1.ddj", 0, 0);
    sub_634470("clientlibrary\\guides\\kmt_event_register_2.ddj");
    set_N00009BD4(2);
    set_N00009BD3(500);

    m_IRM.LoadFromFile("clientlibrary\\resinfo\\ifsimple.txt");
    m_IRM.CreateInterfaceSection("Create", this);
    CIFStatic* label = m_IRM.GetResObj<CIFStatic>(1, 0);
    if (label) {
        label->SetTooltip(KmtGetText(L"UIIT_KMT_EVENT_REGISTER"));
        label->SetStyleThingy(TOOLTIP);
    }

    return true;
}

int CIFEventRegisterGuide::OnMouseLeftUp(int /*a1*/, int /*x*/, int /*y*/)
{
    if (!g_pCGInterface) {
        return 0;
    }

    CIFEventRegister* window =
        g_pCGInterface->m_IRM.GetResObj<CIFEventRegister>(EventRegisterID, 1);
    if (!window) {
        return 0;
    }

    if (window->IsVisible()) {
        window->BringToFront();
        CGEffSoundBody::get()->PlaySound(L"snd_window_open");
        return 0;
    }

    window->Clear();
    window->ClearDDJ();
    CMsgStreamBuffer request(0x169A);
    request << BYTE(0x3);
    SendMsg(request);
    window->UpdateEvents();
    window->UpdateMenuSize();
    window->ShowGWnd(true);
    CGEffSoundBody::get()->PlaySound(L"snd_window_open");
    return 0;
}

void CIFEventRegisterGuide::OnCIFReady()
{
    CIFDecoratedStatic::OnCIFReady();
    sub_633990();
}
