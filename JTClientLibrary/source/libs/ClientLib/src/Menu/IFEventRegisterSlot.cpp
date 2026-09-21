#include <ClientNet\MsgStreamBuffer.h>
#include "IFEventRegisterSlot.h"
#include "IFEventRegister.h"
#include <GInterface.h>
#include <IPHlpApi.h>



#define GDR_STATIC1 6
#define GDR_STATIC2 7

GFX_IMPLEMENT_DYNCREATE(CIFEventRegisterSlot, CIFWnd)
GFX_BEGIN_MESSAGE_MAP(CIFEventRegisterSlot, CIFWnd)
GFX_END_MESSAGE_MAP()

CIFEventRegisterSlot::CIFEventRegisterSlot(void){
    number = 0;
    testbox = L"";
}
CIFEventRegisterSlot::~CIFEventRegisterSlot(void){

}


bool CIFEventRegisterSlot::OnCreate(long ln)
{
    // Populate inherited members
    CIFWnd::OnCreate(ln);
    m_IRM.LoadFromFile("clientlibrary\\resinfo\\ifeventregisterslot.txt");
    m_IRM.CreateInterfaceSection("Create", this);


    this->m_IRM.GetResObj<CIFStatic>(4, 1)->BringToFront();


    number = 0;
    testbox = L"";
    this->ShowGWnd(false);
    return true;
}

void CIFEventRegisterSlot::WriteLine(byte No, const wchar_t *EventName, const wchar_t *EventDesc) {


    number = No;

    this->m_IRM.GetResObj<CIFStatic>(4, 1)->SetText(EventName);

    testbox = EventDesc;


}
int CIFEventRegisterSlot::OnMouseLeftUp(int a1, int x, int y) {

    g_pCGInterface->m_IRM.GetResObj<CIFEventRegister>(EventRegisterID, 1)->ClearDDJ();
    if (number != 0)
    {
        g_pCGInterface->m_IRM.GetResObj<CIFEventRegister>(EventRegisterID, 1)->SelectedEventID = number;
        g_pCGInterface->m_IRM.GetResObj<CIFEventRegister>(EventRegisterID, 1)->SetDescBoxText(testbox);
        this->m_IRM.GetResObj<CIFBarWnd>(3, 1)->TB_Func_13("interface\\ifcommon\\com_bar01select_", 1, 1);

    }
    else
    {
       g_pCGInterface->m_IRM.GetResObj<CIFEventRegister>(EventRegisterID, 1)->SelectedEventID = number;
       g_pCGInterface->m_IRM.GetResObj<CIFEventRegister>(EventRegisterID, 1)->SetDescBoxText(testbox);
    }

    return true;
}
void CIFEventRegisterSlot::ClearDDJ()
{
    this->m_IRM.GetResObj<CIFBarWnd>(3, 1)->TB_Func_13("interface\\ifcommon\\com_bar01_", 0, 0);
}

void CIFEventRegisterSlot::OnUpdate() {

}
