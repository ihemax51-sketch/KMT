#include <Game.h>
#include <ICPlayer.h>
#include <GInterface.h>
#include <TextStringManager.h>
#include <BSLib/multibyte.h>
#include <GlobalDataManager.h>
#include <Macro/IFMacroMenuAutoHunt.h>
#include <EntityManagerClient.h>
#include <ICMonster.h>
#include <NavMesh/LocationInfo.h>
#include <NavMesh/IRegionManager.h>
#include <Macro/IFMacroMenu.h>
#include <sstream>
#include <IFChatViewer.h>
#include <CustomData/CustomDataManager.h>
#include "IFCustomEmojiList.h"



#define GDR_GRANTNAME_BTN_APPLY 9
#define GDR_GRANTNAME_EDIT 7
#define GDR_GRANTNAME_BG3 52
#define GDR_GRANTNAME_BG2 51
#define GDR_GRANTNAME_BG1 50
#define GDR_GRANTNAME_LABEL 53


GFX_IMPLEMENT_DYNCREATE(CIFCustomEmojiList, CIFWnd)

GFX_BEGIN_MESSAGE_MAP(CIFCustomEmojiList, CIFWnd)
                    ONG_COMMAND(GDR_GRANTNAME_BTN_APPLY, &On_BtnClick)
                    ONG_COMMAND(1000, &On_BtnClick)

GFX_END_MESSAGE_MAP()

CIFCustomEmojiList::CIFCustomEmojiList(void){

}
CIFCustomEmojiList::~CIFCustomEmojiList(void){

}

bool CIFCustomEmojiList::OnCreate(long ln)
{

    // Populate inherited members
    CIFWnd::OnCreate(ln);
    m_IRM.LoadFromFile("clientlibrary\\resinfo\\ifcustomemojilist.txt");
    m_IRM.CreateInterfaceSection("Create", this);

    wnd_rect sz;
    sz.size.height = 20;
    sz.size.width = 20;
    sz.pos.x = 10;
    sz.pos.y = 10;
    for(int i = 0; i < 100; i++) {
        emojiSlots[i] = (CIFCustomEmojiListSlot *) CGWnd::CreateInstance(this, GFX_RUNTIME_CLASS(CIFCustomEmojiListSlot), sz, 700 + i, 0);
        emojiSlots[i]->ShowGWnd(false);
        // Her 8 tanede bir sağ tarafa X + 20
        if ((i + 1) % 9 == 0) {
            sz.pos.x = 10;  // X pozisyonunu sıfırla
            sz.pos.y += 30;  // Y pozisyonunu 30 artır
        } else {
            sz.pos.x += 30;  // X pozisyonunu 20 artır
        }
    }

    int i = 0;
    for(std::map<std::wstring, std::string>::iterator it = m_CustomDataManager->emojiList.begin(); it != m_CustomDataManager->emojiList.end(); it++)
    {
        if(emojiSlots[i] != NULL && i < 100)
        {
            emojiSlots[i]->SetName(it->second, it->first);
            emojiSlots[i]->ShowGWnd(true);
            i++;
        }

    }
    UpdateMenuSize();
    this->ShowGWnd(false);
    return true;
}

void CIFCustomEmojiList::UpdateMenuSize()
{

}


void CIFCustomEmojiList::OnUpdate()
{
    CIFChatViewer * view = g_pCGInterface->m_IRM.GetResObj<CIFChatViewer>(1, 1);
    if(view != NULL)
    {
        if(view->GetGuiFromList<CIFButton>(41) != NULL)
        {
            wnd_pos x = view->GetGuiFromList<CIFButton>(41)->GetPos();
            this->MoveGWnd(x.x + 390, (x.y - this->GetSize().height)- 10);
        }
    }
}
void CIFCustomEmojiList::Clear()
{

}

void CIFCustomEmojiList::On_BtnClick() {


}
