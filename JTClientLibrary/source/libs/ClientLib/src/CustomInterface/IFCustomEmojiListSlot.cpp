
#include <Data/ItemData.h>
#include <GlobalDataManager.h>
#include <TextStringManager.h>
#include <GInterface.h>
#include <CustomData/CustomDataManager.h>
#include <CharacterDependentData.h>
#include <ICPlayer.h>
#include <CustomData/CustomCICPlayer.h>
#include <BSLib/multibyte.h>
#include <IFChatViewer.h>
#include <IFWholeChat.h>
#include "IFCustomEmojiListSlot.h"




GFX_IMPLEMENT_DYNCREATE(CIFCustomEmojiListSlot, CIFWnd)
GFX_BEGIN_MESSAGE_MAP(CIFCustomEmojiListSlot, CIFWnd)

GFX_END_MESSAGE_MAP()
CIFCustomEmojiListSlot::CIFCustomEmojiListSlot(void)
{
    FullTextx = std::wstring();
}
CIFCustomEmojiListSlot::~CIFCustomEmojiListSlot(void)
{
}
bool CIFCustomEmojiListSlot::OnCreate(long ln)
{
    // Populate inherited members
    m_IRM.LoadFromFile("clientlibrary\\resinfo\\ifcustomemojilistslot.txt");
    m_IRM.CreateInterfaceSection("Create", this);

    this->m_IRM.GetResObj<CIFStatic>(1, 1)->ShowGWnd(false);

    return true;
}
void CIFCustomEmojiListSlot::OnUpdate() {

}
void CIFCustomEmojiListSlot::SetName(std::string Path, std::wstring FullText){

    //this->m_IRM.GetResObj<CIFStatic>(1, 1)->SetStyleThingy(TOOLTIP);


  //  this->m_IRM.GetResObj<CIFStatic>(1, 1)->SetTooltip(FullText.c_str());
    this->m_IRM.GetResObj<CIFStatic>(1, 1)->TB_Func_13(Path.c_str(), 1, 1);
    this->m_IRM.GetResObj<CIFStatic>(1, 1)->ShowGWnd(true);
    FullTextx = FullText;

}
int CIFCustomEmojiListSlot::OnMouseLeftUp(int a1, int x, int y) {
    if (!FullTextx.empty()) {
        CIFChatViewer *chatview = g_pCGInterface->m_IRM.GetResObj<CIFChatViewer>(1, 1);
        CIFWholeChat *pWholeChat = (CIFWholeChat *) g_pCGInterface->m_IRM.GetResObj(56, 0);
        if(pWholeChat)
        {
            std::wstring text = pWholeChat->m_pEdit->GetText();
            text.append(FullTextx.c_str()); // FullTextx'i text'in sonuna ekliyoruz
            pWholeChat->m_pEdit->SetText(text.c_str()); // Güncellenmiş metni geri ayarlıyoruz
        }
        else if (chatview) {
            std::wstring text = chatview->m_InputBox->GetText();
            text.append(FullTextx.c_str()); // FullTextx'i text'in sonuna ekliyoruz
            chatview->m_InputBox->SetText(text.c_str()); // Güncellenmiş metni geri ayarlıyoruz
        }
    }

    return true;
}

void CIFCustomEmojiListSlot::Clear()
{

}
