#include <Game.h>
#include "IFVAvatarMall.h"
#include "IFVAvatarMallBuyItemList.h"
#include <NetProcessIn.h>
#include <BSLib/multibyte.h>
#include <GInterface.h>
#include <ICPlayer.h>
#include <GlobalDataManager.h>
#include <BSLib/Debug.h>
#include <TextStringManager.h>
#include <CustomData/CustomDataManager.h>


#define GDR_BUYALL 13
#define GDR_CLEAR 14

#define GDR_LEFT_1 15
#define GDR_RIGHT_1 16
#define GDR_LEFT_2 17
#define GDR_RIGHT_2 18
#define GDR_LEFT_3 19
#define GDR_RIGHT_3 20

GFX_IMPLEMENT_DYNCREATE(CIFVAvatarMall, CIFMainFrame)
GFX_BEGIN_MESSAGE_MAP(CIFVAvatarMall, CIFMainFrame)
                    ONG_COMMAND(100, &CIFVAvatarMall::OnUnknownStuff)
                    ONG_COMMAND(111, &CIFVAvatarMall::OnUnknownStuff)

                    ONG_COMMAND(GDR_CLEAR, &CIFVAvatarMall::CallRenderStatic)
                    ONG_COMMAND(GDR_BUYALL, &CIFVAvatarMall::Buyalls)
                    ONG_COMMAND(GDR_LEFT_1, &CIFVAvatarMall::Slot1LeftBtn)
                    ONG_COMMAND(GDR_RIGHT_1, &CIFVAvatarMall::Slot1RightBtn)

                    ONG_COMMAND(GDR_LEFT_2, &CIFVAvatarMall::Slot2LeftBtn)
                    ONG_COMMAND(GDR_RIGHT_2, &CIFVAvatarMall::Slot2RightBtn)
                    ONG_COMMAND(GDR_LEFT_3, &CIFVAvatarMall::Slot3LeftBtn)
                    ONG_COMMAND(GDR_RIGHT_3, &CIFVAvatarMall::Slot3RightBtn)

GFX_END_MESSAGE_MAP()

CIFVAvatarMall::CIFVAvatarMall(void){
    m_pTabs = 0;
    Render = 0;
    HeadID = 0;
    AvatarID = 0;
    AttachID = 0;
    Pet1 = 0;
    Pet2 = 0;
    LastPickPet = 0;
    LastAtakPet = 0;
    GiyilenHeadID = 0;
    ActivedPage = 0;

    currentPageSlot1 = 0;
    slotsPerPageSlot1 = 5;
    currentPageSlot2 = 0;
    slotsPerPageSlot2 = 5;

    currentPageSlot3 = 0;
    slotsPerPageSlot3 = 5;


    currentPageSlot4 = 0;
    slotsPerPageSlot4 = 5;

    currentPageSlot5 = 0;
    slotsPerPageSlot5 = 5;

    currentPageSlot6 = 0;
    slotsPerPageSlot6 = 5;
    BS_DEBUG_LOW(">" __FUNCTION__);
}
CIFVAvatarMall::~CIFVAvatarMall(void){
    if (m_pTabs) {
        free(m_pTabs);
        m_pTabs = 0;
    }
    BS_DEBUG_LOW(">" __FUNCTION__);
}


void CIFVAvatarMall::OnUnknownStuff() {
    int id = GetCurrentEventMsgCtrlId();
    for (int i = 0; i < numberOfTabs; ++i) {
        if (id == m_pTabs[i]->UniqueID()) {
            ActivateTabPage(i);
            return;
        }
    }
}
int CIFVAvatarMall::Func_4(int a2) {
    int v1 = 0;
    while (a2 != v1 + 100) {
        if (++v1 >= 11)
            return -1;
    }

    return 100;
}

std::n_wstring KisaYazi2(const std::n_wstring& orijinal, size_t maksUzunluk)
{
    if (orijinal.size() <= maksUzunluk) {
        return orijinal;
    }
    else {
        return orijinal.substr(0, maksUzunluk) + L"...";
    }
}
bool CIFVAvatarMall::OnCreate(long ln)
{
    //	printf("> " __FUNCTION__ "(%d)\n", ln);

    // Populate inherited members
    CIFMainFrame::OnCreate(ln);

    m_IRM.LoadFromFile("clientlibrary\\resinfo\\ifvavatarmall.txt");
    m_IRM.CreateInterfaceSection("Create", this);

    wnd_rect sz;



    Render = m_IRM.GetResObj<CIFRenderStatic>(4, 1);
    Render->TB_Func_13("interface\\mall\\mall_charac_frame.ddj", 1,0);

    this->m_IRM.GetResObj(14, 1)->SetText(KmtGetText(L"UIIT_KMT_ORIGINAL"));
    this->m_IRM.GetResObj(13, 1)->SetText(KmtGetText(L"UIIT_KMT_PURCHASE_LIST"));




    std::n_wstring msg2 = KmtGetText(L"UIIT_KMT_PERMANENCE");
    std::n_wstring labelmsg2 = KisaYazi2(msg2, 6);
    m_IRM.GetResObj<CIFButton>(22, 1)->SetText(labelmsg2.c_str());
    m_IRM.GetResObj<CIFButton>(22, 1)->SetTooltip(msg2);
    m_IRM.GetResObj<CIFButton>(22, 1)->SetStyleThingy(TOOLTIP);
    m_IRM.GetResObj<CIFButton>(22, 1)->SetClickable(false);
    m_IRM.GetResObj<CIFButton>(22, 1)->m_FontTexture.sub_8B4750(7);

    m_IRM.GetResObj<CIFButton>(21, 1)->SetClickable(false);
    m_IRM.GetResObj<CIFButton>(21, 1)->m_FontTexture.sub_8B4750(7);
    m_pTabs = new CIFSelectableArea * [numberOfTabs];
    for (int i = 0; i < numberOfTabs; i++) {
        sz.pos.x= 35;
        sz.pos.y = 89;
        sz.size.width = tabWidth + 1;
        sz.size.height = tabHeight;
        m_pTabs[i] = (CIFSelectableArea*)CGWnd::CreateInstance(this, GFX_RUNTIME_CLASS(CIFSelectableArea),
                                                               sz, tabFirstId + i, 0);
        m_pTabs[i]->SetFont(this->N00009C2F);


        m_pTabs[i]->sub_64CE30("clientlibrary\\mall\\avatarmalltabon.ddj",
                               "clientlibrary\\mall\\avatarmalltaboff.ddj", "clientlibrary\\mall\\avatarmalltabon.ddj");

        //  m_pTabs[i]->m_FontTexture.SetColor(D3DCOLOR_ARGB(255,239,218,164));

        switch (i) {
            case 0:
                m_pTabs[0]->sub_64CC30(1);
                m_pTabs[0]->SetText(KmtGetText(L"UIIT_KMT_AVATAR"));
                break;
            case 1:
                m_pTabs[1]->MoveGWnd(m_pTabs[0]->GetPos().x + tabWidth + 6, m_pTabs[0]->GetPos().y);
                m_pTabs[1]->SetText((KmtGetText(L"UIIT_KMT_PET")));
                break;
            case 2:
            {
                m_pTabs[2]->MoveGWnd(m_pTabs[1]->GetPos().x + tabWidth + 6, m_pTabs[1]->GetPos().y);
                std::n_wstring msg = KmtGetText(L"UIIT_KMT_DEVIL_S_SPIRIT");
                std::n_wstring labelmsg = KisaYazi2(msg, 7);
                m_pTabs[2]->SetText(labelmsg.c_str());
                m_pTabs[2]->SetTooltip(msg);
                m_pTabs[2]->SetStyleThingy(TOOLTIP);
            }


                break;
            case 3:
                m_pTabs[3]->MoveGWnd(m_pTabs[2]->GetPos().x + tabWidth + 6, m_pTabs[2]->GetPos().y);
                break;
        }
        m_pTabs[i]->sub_64CC30(0);
        m_pTabs[i]->m_FontTexture.sub_8B4750(7);
        m_pTabs[i]->m_FontTexture.SetColor(D3DCOLOR_ARGB(255, 199, 173,123));
        m_pTabs[i]->ShowGWnd(true);

    }


    m_IRM.GetResObj<CIFButton>(GDR_LEFT_1, 1)->SetEnabledState(false);
    m_IRM.GetResObj<CIFButton>(GDR_LEFT_2, 1)->SetEnabledState(false);
    m_IRM.GetResObj<CIFButton>(GDR_LEFT_3, 1)->SetEnabledState(false);


    m_IRM.GetResObj<CIFButton>(GDR_RIGHT_1, 1)->SetEnabledState(false);
    m_IRM.GetResObj<CIFButton>(GDR_RIGHT_2, 1)->SetEnabledState(false);
    m_IRM.GetResObj<CIFButton>(GDR_RIGHT_3, 1)->SetEnabledState(false);




    m_pCloseBtn->ShowGWnd(true);
    m_pCloseBtn->BringToFront();

    //m_pHandleBar->ShowGWnd(false);
    m_pCloseBtn->TB_Func_13("clientlibrary\\mall\\mall_web_close_button.ddj", 1, 0);
    m_pCloseBtn->SetGWndSize(28,32);
    m_pCloseBtn->MoveGWnd(this->GetPos().x+ 671, this->GetPos().y + 10);
    ActivedPage = 0;

    sz.size.width = 74;
    sz.size.height = 72;
    sz.pos.x = 36 ;
    sz.pos.y = 173;
    for (int i = 0; i < 100; i++) {
        Slot1[i] = (CIFVAvatarMallSlot*)CGWnd::CreateInstance(this, GFX_RUNTIME_CLASS(CIFVAvatarMallSlot), sz, 601 + i, 0);
        Slot1[i]->ShowGWnd(i < 5);  // İlk 5 slot gösteriliyor

        sz.pos.x += 79;

        if ((i + 1) % 5 == 0) {
            sz.pos.x = 36; // Pozisyonu sıfırla
        }
    }

    sz.size.width = 74;
    sz.size.height = 72;
    sz.pos.x = 36 ;
    sz.pos.y = 280;
    for (int i = 0; i < 100; i++) {
        Slot2[i] = (CIFVAvatarMallSlot*)CGWnd::CreateInstance(this, GFX_RUNTIME_CLASS(CIFVAvatarMallSlot), sz, 701 + i, 0);
        Slot2[i]->ShowGWnd(i < 5);  // İlk 5 slot gösteriliyor

        sz.pos.x += 79;

        if ((i + 1) % 5 == 0) {
            sz.pos.x = 36; // Pozisyonu sıfırla
        }
        Slot2[i]->ShowGWnd(true);
    }

    sz.size.width = 74;
    sz.size.height = 72;
    sz.pos.x = 36 ;
    sz.pos.y = 387;
    for (int i = 0; i < 100; i++) {
        Slot3[i] = (CIFVAvatarMallSlot*)CGWnd::CreateInstance(this, GFX_RUNTIME_CLASS(CIFVAvatarMallSlot), sz, 801 + i, 0);
        Slot3[i]->ShowGWnd(i < 5);  // İlk 5 slot gösteriliyor

        sz.pos.x += 79;

        if ((i + 1) % 5 == 0) {
            sz.pos.x = 36; // Pozisyonu sıfırla
        }
        Slot3[i]->ShowGWnd(true);
    }


    sz.size.width = 74;
    sz.size.height = 72;
    sz.pos.x = 36 ;
    sz.pos.y = 173;
    for (int i = 0; i < 50; i++) {
        Slot4Grap[i] = (CIFVAvatarMallSlot*)CGWnd::CreateInstance(this, GFX_RUNTIME_CLASS(CIFVAvatarMallSlot), sz, 1000 + i, 0);
        Slot4Grap[i]->ShowGWnd(i < 5);  // İlk 5 slot gösteriliyor

        sz.pos.x += 79;

        if ((i + 1) % 5 == 0) {
            sz.pos.x = 36; // Pozisyonu sıfırla
        }
        Slot4Grap[i]->ShowGWnd(false);
    }

    sz.size.width = 74;
    sz.size.height = 72;
    sz.pos.x = 36 ;
    sz.pos.y = 280;
    for (int i = 0; i < 50; i++) {
        Slot5[i] = (CIFVAvatarMallSlot*)CGWnd::CreateInstance(this, GFX_RUNTIME_CLASS(CIFVAvatarMallSlot), sz, 1200 + i, 0);
        Slot5[i]->ShowGWnd(i < 5);  // İlk 5 slot gösteriliyor

        sz.pos.x += 79;

        if ((i + 1) % 5 == 0) {
            sz.pos.x = 36; // Pozisyonu sıfırla
        }
        Slot5[i]->ShowGWnd(true);
    }
    sz.size.width = 74;
    sz.size.height = 72;
    sz.pos.x = 36 ;
    sz.pos.y = 173;
    for (int i = 0; i < 12; i++) {
        Slot6[i] = (CIFVAvatarMallSlot*)CGWnd::CreateInstance(this, GFX_RUNTIME_CLASS(CIFVAvatarMallSlot), sz, 1500 + i, 0);
        Slot6[i]->ShowGWnd(i < 5);  // İlk 5 slot gösteriliyor

        sz.pos.x += 79;

        if ((i + 1) % 5 == 0) {
            sz.pos.x = 36; // Pozisyonu sıfırla
        }
        this->Slot6[i]->ShowGWnd(false);
    }


    this->ShowGWnd(false);
    //   UpdateMenuSize();

    //ActivateTabPage(0);
    return true;
}

void CIFVAvatarMall::OnUpdate()
{

    wchar_t Priceb[255];

    std::wstring test = Insert(g_pCGInterface->m_silk_own);
    swprintf_s(Priceb, KmtGetText(L"UIIT_KMT_SILK_TEXT"), test.c_str());

    this->m_IRM.GetResObj(6, 1)->SetText(Priceb);


}
void CIFVAvatarMall::Buyalls()
{

    if(!g_pCGInterface->m_IRM.GetResObj<CIFVAvatarMallBuyItemList>(AvatarMallBuyListId, 1)->IsVisible())
    {
        g_pCGInterface->m_IRM.GetResObj<CIFVAvatarMallBuyItemList>(AvatarMallBuyListId, 1)->UpdateMenuSize();

        g_pCGInterface->m_IRM.GetResObj<CIFVAvatarMallBuyItemList>(AvatarMallBuyListId, 1)->ShowGWnd(true);
    }
    else
    {
        g_pCGInterface->m_IRM.GetResObj<CIFVAvatarMallBuyItemList>(AvatarMallBuyListId, 1)->BringToFront();
    }

}

bool CompareAvatarMallStruct(const CustomDataManager::AvatarMallStruct &a, const CustomDataManager::AvatarMallStruct &b) {
    return a.ItemIndex < b.ItemIndex;
}

void CIFVAvatarMall::LoadAvatarHats() {
    ClearSlot1();
    std::list<CustomDataManager::AvatarMallStruct> items;

    // Cinsiyet verilerini al
    const CCharacterData* data = g_CGlobalDataManager->GetCharacter(g_pMyPlayerObj->GetCommonData()->RefObjectId);

    // CategoryID 0 olan ve cinsiyete uygun öğeleri topluyoruz
    for (std::map<int, CustomDataManager::AvatarMallStruct>::iterator it = m_CustomDataManager->AvatarMallItemList.begin();
         it != m_CustomDataManager->AvatarMallItemList.end(); ++it) {
        if (it->second.CategoryID == 0) {
            const SItemData* itemdata = &g_CGlobalDataManager->GetItemData(it->second.ItemID);
            if (data != NULL && itemdata != NULL) {
                if(data->GetData().CharGender == itemdata->m_reqGender && itemdata->m_typeId.getTypeID1() == 3
                   && itemdata->m_typeId.getTypeID2() == 1 && itemdata->m_typeId.getTypeID3() == 13 && itemdata->m_typeId.getTypeID4() == 1)
                {
                    items.push_back(it->second);
                }

            }
        }
    }

    // ItemIndex'e göre sıralama
    items.sort(CompareAvatarMallStruct);

    // Slotlara ekleme
    int i = -1;
    for (std::list<CustomDataManager::AvatarMallStruct>::iterator it = items.begin(); it != items.end(); ++it) {
        i++;
        if (i < 100) {  // 100 slot sınırı
            Slot1[i]->AddMarketItems(it->ID, it->ItemID, it->Silk, it->PetObjID);
        }
    }

    currentPageSlot1 = 1;  // Başlangıç sayfası 1 olarak ayarlanıyor
    UpdateSlot1Visibility();  // Slot görünürlüğünü güncelle
}


void CIFVAvatarMall::UpdateButtonStatesSlot1(int maxPageSlot1) {
    if(ActivedPage == 0)
    {
        if (currentPageSlot1 == 1) {
            m_IRM.GetResObj<CIFButton>(GDR_LEFT_1, 1)->SetEnabledState(false);
        } else {
            m_IRM.GetResObj<CIFButton>(GDR_LEFT_1, 1)->SetEnabledState(true);
        }

        if (currentPageSlot1 == maxPageSlot1) {
            m_IRM.GetResObj<CIFButton>(GDR_RIGHT_1, 1)->SetEnabledState(false);
        } else {
            m_IRM.GetResObj<CIFButton>(GDR_RIGHT_1, 1)->SetEnabledState(true);
        }
    }
    else if(ActivedPage == 1)
    {
        if (currentPageSlot4 == 1) {
            m_IRM.GetResObj<CIFButton>(GDR_LEFT_1, 1)->SetEnabledState(false);
        } else {
            m_IRM.GetResObj<CIFButton>(GDR_LEFT_1, 1)->SetEnabledState(true);
        }

        if (currentPageSlot4 == maxPageSlot1) {
            m_IRM.GetResObj<CIFButton>(GDR_RIGHT_1, 1)->SetEnabledState(false);
        } else {
            m_IRM.GetResObj<CIFButton>(GDR_RIGHT_1, 1)->SetEnabledState(true);
        }
    }
    else if(ActivedPage == 2)
    {
        if (currentPageSlot6 == 1) {
            m_IRM.GetResObj<CIFButton>(GDR_LEFT_1, 1)->SetEnabledState(false);
        } else {
            m_IRM.GetResObj<CIFButton>(GDR_LEFT_1, 1)->SetEnabledState(true);
        }

        if (currentPageSlot6 == maxPageSlot1) {
            m_IRM.GetResObj<CIFButton>(GDR_RIGHT_1, 1)->SetEnabledState(false);
        } else {
            m_IRM.GetResObj<CIFButton>(GDR_RIGHT_1, 1)->SetEnabledState(true);
        }
    }

}

void CIFVAvatarMall::UpdateSlot1Visibility() {

    for (int i = 0; i < 100; i++) {
        Slot1[i]->ShowGWnd((i / slotsPerPageSlot1) == (currentPageSlot1 - 1));
    }

    // Cinsiyete uygun ve CategoryID 0 olan öğelerin sayısını hesapla
    const CCharacterData* data = g_CGlobalDataManager->GetCharacter(g_pMyPlayerObj->GetCommonData()->RefObjectId);
    int totalItems = 0;
    for (std::map<int, CustomDataManager::AvatarMallStruct>::iterator it = m_CustomDataManager->AvatarMallItemList.begin();
         it != m_CustomDataManager->AvatarMallItemList.end(); ++it) {
        if (it->second.CategoryID == 0) {
            const SItemData* itemdata = &g_CGlobalDataManager->GetItemData(it->second.ItemID);
            if (data != NULL && itemdata != NULL) {
                if(data->GetData().CharGender == itemdata->m_reqGender && itemdata->m_typeId.getTypeID1() == 3
                   && itemdata->m_typeId.getTypeID2() == 1 && itemdata->m_typeId.getTypeID3() == 13 && itemdata->m_typeId.getTypeID4() == 1)
                {
                    totalItems++;
                }
            }
        }
    }

    int maxPageSlot1 = (totalItems + slotsPerPageSlot1 - 1) / slotsPerPageSlot1;  // Sayfa sayısını hesapla
    UpdateButtonStatesSlot1(maxPageSlot1);

}



void CIFVAvatarMall::Slot1RightBtn() {
    if(ActivedPage == 0)
    {
        // Cinsiyete uygun ve CategoryID 0 olan öğelerin sayısını hesapla
        const CCharacterData* data = g_CGlobalDataManager->GetCharacter(g_pMyPlayerObj->GetCommonData()->RefObjectId);
        int totalItems = 0;
        for (std::map<int, CustomDataManager::AvatarMallStruct>::iterator it = m_CustomDataManager->AvatarMallItemList.begin();
             it != m_CustomDataManager->AvatarMallItemList.end(); ++it) {
            if (it->second.CategoryID == 0) {
                const SItemData* itemdata = &g_CGlobalDataManager->GetItemData(it->second.ItemID);
                if (data != NULL && itemdata != NULL) {
                    if(data->GetData().CharGender == itemdata->m_reqGender && itemdata->m_typeId.getTypeID1() == 3
                       && itemdata->m_typeId.getTypeID2() == 1 && itemdata->m_typeId.getTypeID3() == 13 && itemdata->m_typeId.getTypeID4() == 1)
                    {
                        totalItems++;
                    }
                }
            }
        }

        int maxPageSlot1 = (totalItems + slotsPerPageSlot1 - 1) / slotsPerPageSlot1;  // Sayfa sayısını hesapla
        if (currentPageSlot1 < maxPageSlot1) {
            currentPageSlot1++;
            UpdateSlot1Visibility();
        }
    }
    else if(ActivedPage == 1)
    {
        int totalItems = 0;
        for (std::map<int, CustomDataManager::AvatarMallStruct>::iterator it = m_CustomDataManager->AvatarMallItemList.begin();
             it != m_CustomDataManager->AvatarMallItemList.end(); ++it) {
            if (it->second.CategoryID == 1) {
                const SItemData* itemdata = &g_CGlobalDataManager->GetItemData(it->second.ItemID);
                if (itemdata != NULL) {
                    if(itemdata->m_typeId.getTypeID1() == 3
                       && itemdata->m_typeId.getTypeID2() == 2 && itemdata->m_typeId.getTypeID3() == 1 && itemdata->m_typeId.getTypeID4() == 2)
                    {
                        totalItems++;
                    }
                }
            }
        }

        int maxPageSlot1 = (totalItems + slotsPerPageSlot4 - 1) / slotsPerPageSlot4;  // Sayfa sayısını hesapla
        if (currentPageSlot4 < maxPageSlot1) {
            currentPageSlot4++;
            UpdateSlot4Visibility();
        }
    }
    else if(ActivedPage == 2)
    {
        const CCharacterData* data = g_CGlobalDataManager->GetCharacter(g_pMyPlayerObj->GetCommonData()->RefObjectId);
        int totalItems = 0;
        for (std::map<int, CustomDataManager::AvatarMallStruct>::iterator it = m_CustomDataManager->AvatarMallItemList.begin();
             it != m_CustomDataManager->AvatarMallItemList.end(); ++it) {
            if (it->second.CategoryID == 2) {
                const SItemData* itemdata = &g_CGlobalDataManager->GetItemData(it->second.ItemID);
                if (itemdata != NULL && data != NULL) {
                    if(data->GetData().CharGender == itemdata->m_reqGender && itemdata->m_typeId.getTypeID1() == 3
                       && itemdata->m_typeId.getTypeID2() == 1 && itemdata->m_typeId.getTypeID3() == 14 && itemdata->m_typeId.getTypeID4() == 1)
                    {
                        totalItems++;
                    }
                }
            }
        }

        int maxPageSlot1 = (totalItems + slotsPerPageSlot6 - 1) / slotsPerPageSlot6;  // Sayfa sayısını hesapla
        if (currentPageSlot6 < maxPageSlot1) {
            currentPageSlot6++;
            UpdateSlot6Visibility();
        }
    }
}

void CIFVAvatarMall::Slot1LeftBtn() {
    if(ActivedPage == 0)
    {
        if (currentPageSlot1 > 1) {
            currentPageSlot1--;
            UpdateSlot1Visibility();
        }
    }
    else if(ActivedPage == 1)
    {
        if (currentPageSlot4 > 1) {
            currentPageSlot4--;
            UpdateSlot4Visibility();
        }
    }
    else if(ActivedPage == 2)
    {
        if (currentPageSlot6 > 1) {
            currentPageSlot6--;
            UpdateSlot6Visibility();
        }
    }
}

void CIFVAvatarMall::ClearSlot1()
{
    for(int i = 0; i < 100; i++)
    {
        Slot1[i]->m_itemInfo->ClearSlot();
    }
}



void CIFVAvatarMall::LoadAvatars() {
    ClearSlot2();
    std::list<CustomDataManager::AvatarMallStruct> items;

    // Cinsiyet verilerini al
    const CCharacterData* data = g_CGlobalDataManager->GetCharacter(g_pMyPlayerObj->GetCommonData()->RefObjectId);

    // CategoryID 0 olan ve cinsiyete uygun öğeleri topluyoruz
    for (std::map<int, CustomDataManager::AvatarMallStruct>::iterator it = m_CustomDataManager->AvatarMallItemList.begin();
         it != m_CustomDataManager->AvatarMallItemList.end(); ++it) {
        if (it->second.CategoryID == 0) {
            const SItemData* itemdata = &g_CGlobalDataManager->GetItemData(it->second.ItemID);
            if (data != NULL && itemdata != NULL) {
                if(data->GetData().CharGender == itemdata->m_reqGender && itemdata->m_typeId.getTypeID1() == 3
                   && itemdata->m_typeId.getTypeID2() == 1 && itemdata->m_typeId.getTypeID3() == 13 && itemdata->m_typeId.getTypeID4() == 2)
                {
                    items.push_back(it->second);
                }

            }
        }
    }

    // ItemIndex'e göre sıralama
    items.sort(CompareAvatarMallStruct);

    // Slotlara ekleme
    int i = -1;
    for (std::list<CustomDataManager::AvatarMallStruct>::iterator it = items.begin(); it != items.end(); ++it) {
        i++;
        if (i < 100) {  // 100 slot sınırı
            Slot2[i]->AddMarketItems(it->ID, it->ItemID, it->Silk, it->PetObjID);
        }
    }

    currentPageSlot2 = 1;  // Başlangıç sayfası 1 olarak ayarlanıyor
    UpdateSlot2Visibility();  // Slot görünürlüğünü güncelle
}



void CIFVAvatarMall::UpdateButtonStatesSlot2(int maxPageSlot1) {
    if(ActivedPage == 0)
    {
        if (currentPageSlot2 == 1) {
            m_IRM.GetResObj<CIFButton>(GDR_LEFT_2, 1)->SetEnabledState(false);
        } else {
            m_IRM.GetResObj<CIFButton>(GDR_LEFT_2, 1)->SetEnabledState(true);
        }

        if (currentPageSlot2 == maxPageSlot1) {
            m_IRM.GetResObj<CIFButton>(GDR_RIGHT_2, 1)->SetEnabledState(false);
        } else {
            m_IRM.GetResObj<CIFButton>(GDR_RIGHT_2, 1)->SetEnabledState(true);
        }
    }
    else if(ActivedPage == 1)
    {
        if (currentPageSlot5 == 1) {
            m_IRM.GetResObj<CIFButton>(GDR_LEFT_2, 1)->SetEnabledState(false);
        } else {
            m_IRM.GetResObj<CIFButton>(GDR_LEFT_2, 1)->SetEnabledState(true);
        }

        if (currentPageSlot5 == maxPageSlot1) {
            m_IRM.GetResObj<CIFButton>(GDR_RIGHT_2, 1)->SetEnabledState(false);
        } else {
            m_IRM.GetResObj<CIFButton>(GDR_RIGHT_2, 1)->SetEnabledState(true);
        }
    }
}

void CIFVAvatarMall::UpdateSlot2Visibility() {

    for (int i = 0; i < 100; i++) {
        Slot2[i]->ShowGWnd((i / slotsPerPageSlot2) == (currentPageSlot2 - 1));
    }

    // Cinsiyete uygun ve CategoryID 0 olan öğelerin sayısını hesapla
    const CCharacterData* data = g_CGlobalDataManager->GetCharacter(g_pMyPlayerObj->GetCommonData()->RefObjectId);
    int totalItems = 0;
    for (std::map<int, CustomDataManager::AvatarMallStruct>::iterator it = m_CustomDataManager->AvatarMallItemList.begin();
         it != m_CustomDataManager->AvatarMallItemList.end(); ++it) {
        if (it->second.CategoryID == 0) {
            const SItemData* itemdata = &g_CGlobalDataManager->GetItemData(it->second.ItemID);
            if (data != NULL && itemdata != NULL) {
                if(data->GetData().CharGender == itemdata->m_reqGender && itemdata->m_typeId.getTypeID1() == 3
                   && itemdata->m_typeId.getTypeID2() == 1 && itemdata->m_typeId.getTypeID3() == 13 && itemdata->m_typeId.getTypeID4() == 2)
                {
                    totalItems++;
                }
            }
        }
    }

    int maxPageSlot1 = (totalItems + slotsPerPageSlot2 - 1) / slotsPerPageSlot2;  // Sayfa sayısını hesapla
    UpdateButtonStatesSlot2(maxPageSlot1);
}



void CIFVAvatarMall::Slot2RightBtn() {
    if(ActivedPage == 0)
    {
        // Cinsiyete uygun ve CategoryID 0 olan öğelerin sayısını hesapla
        const CCharacterData* data = g_CGlobalDataManager->GetCharacter(g_pMyPlayerObj->GetCommonData()->RefObjectId);
        int totalItems = 0;
        for (std::map<int, CustomDataManager::AvatarMallStruct>::iterator it = m_CustomDataManager->AvatarMallItemList.begin();
             it != m_CustomDataManager->AvatarMallItemList.end(); ++it) {
            if (it->second.CategoryID == 0) {
                const SItemData* itemdata = &g_CGlobalDataManager->GetItemData(it->second.ItemID);
                if (data != NULL && itemdata != NULL) {
                    if(data->GetData().CharGender == itemdata->m_reqGender && itemdata->m_typeId.getTypeID1() == 3
                       && itemdata->m_typeId.getTypeID2() == 1 && itemdata->m_typeId.getTypeID3() == 13 && itemdata->m_typeId.getTypeID4() == 2)
                    {
                        totalItems++;
                    }
                }
            }
        }

        int maxPageSlot1 = (totalItems + slotsPerPageSlot2 - 1) / slotsPerPageSlot2;  // Sayfa sayısını hesapla
        if (currentPageSlot2 < maxPageSlot1) {
            currentPageSlot2++;
            UpdateSlot2Visibility();
        }
    }
    else if(ActivedPage == 1)
    {

        int totalItems = 0;
        for (std::map<int, CustomDataManager::AvatarMallStruct>::iterator it = m_CustomDataManager->AvatarMallItemList.begin();
             it != m_CustomDataManager->AvatarMallItemList.end(); ++it) {
            if (it->second.CategoryID == 1) {
                const SItemData* itemdata = &g_CGlobalDataManager->GetItemData(it->second.ItemID);
                if (itemdata != NULL) {
                    if(itemdata->m_typeId.getTypeID1() == 3 && itemdata->m_typeId.getTypeID2() == 2 && itemdata->m_typeId.getTypeID3() == 1 && itemdata->m_typeId.getTypeID4() == 1)
                    {
                        totalItems++;
                    }
                }
            }
        }

        int maxPageSlot1 = (totalItems + slotsPerPageSlot5 - 1) / slotsPerPageSlot5;  // Sayfa sayısını hesapla
        if (currentPageSlot5 < maxPageSlot1) {
            currentPageSlot5++;
            UpdateSlot5Visibility();
        }
    }
}

void CIFVAvatarMall::Slot2LeftBtn() {
    if(ActivedPage == 0)
    {
        if (currentPageSlot2 > 1) {
            currentPageSlot2--;
            UpdateSlot2Visibility();
        }
    }
    else  if(ActivedPage == 1)
    {
        if (currentPageSlot5 > 1) {
            currentPageSlot5--;
            UpdateSlot5Visibility();
        }
    }
}

void CIFVAvatarMall::ClearSlot2()
{
    for(int i = 0; i < 100; i++)
    {
        Slot2[i]->m_itemInfo->ClearSlot();
    }
}


void CIFVAvatarMall::LoadAvatarAttach() {
    ClearSlot3();
    std::list<CustomDataManager::AvatarMallStruct> items;

    // Cinsiyet verilerini al
    const CCharacterData* data = g_CGlobalDataManager->GetCharacter(g_pMyPlayerObj->GetCommonData()->RefObjectId);

    // CategoryID 0 olan ve cinsiyete uygun öğeleri topluyoruz
    for (std::map<int, CustomDataManager::AvatarMallStruct>::iterator it = m_CustomDataManager->AvatarMallItemList.begin();
         it != m_CustomDataManager->AvatarMallItemList.end(); ++it) {
        if (it->second.CategoryID == 0) {
            const SItemData* itemdata = &g_CGlobalDataManager->GetItemData(it->second.ItemID);
            if (data != NULL && itemdata != NULL) {
                if(data->GetData().CharGender == itemdata->m_reqGender && itemdata->m_typeId.getTypeID1() == 3
                   && itemdata->m_typeId.getTypeID2() == 1 && itemdata->m_typeId.getTypeID3() == 13 && itemdata->m_typeId.getTypeID4() == 3)
                {
                    items.push_back(it->second);
                }

            }
        }
    }

    // ItemIndex'e göre sıralama
    items.sort(CompareAvatarMallStruct);

    // Slotlara ekleme
    int i = -1;
    for (std::list<CustomDataManager::AvatarMallStruct>::iterator it = items.begin(); it != items.end(); ++it) {
        i++;
        if (i < 100) {  // 100 slot sınırı
            Slot3[i]->AddMarketItems(it->ID, it->ItemID, it->Silk, it->PetObjID);
        }
    }

    currentPageSlot3 = 1;  // Başlangıç sayfası 1 olarak ayarlanıyor
    UpdateSlot3Visibility();  // Slot görünürlüğünü güncelle
}



void CIFVAvatarMall::UpdateButtonStatesSlot3(int maxPageSlot1) {
    if (currentPageSlot3 == 1) {
        m_IRM.GetResObj<CIFButton>(GDR_LEFT_3, 1)->SetEnabledState(false);
    } else {
        m_IRM.GetResObj<CIFButton>(GDR_LEFT_3, 1)->SetEnabledState(true);
    }

    if (currentPageSlot2 == maxPageSlot1) {
        m_IRM.GetResObj<CIFButton>(GDR_RIGHT_3, 1)->SetEnabledState(false);
    } else {
        m_IRM.GetResObj<CIFButton>(GDR_RIGHT_3, 1)->SetEnabledState(true);
    }
}

void CIFVAvatarMall::UpdateSlot3Visibility() {

    for (int i = 0; i < 100; i++) {
        Slot3[i]->ShowGWnd((i / slotsPerPageSlot3) == (currentPageSlot3 - 1));
    }

    // Cinsiyete uygun ve CategoryID 0 olan öğelerin sayısını hesapla
    const CCharacterData* data = g_CGlobalDataManager->GetCharacter(g_pMyPlayerObj->GetCommonData()->RefObjectId);
    int totalItems = 0;
    for (std::map<int, CustomDataManager::AvatarMallStruct>::iterator it = m_CustomDataManager->AvatarMallItemList.begin();
         it != m_CustomDataManager->AvatarMallItemList.end(); ++it) {
        if (it->second.CategoryID == 0) {
            const SItemData* itemdata = &g_CGlobalDataManager->GetItemData(it->second.ItemID);
            if (data != NULL && itemdata != NULL) {
                if(data->GetData().CharGender == itemdata->m_reqGender && itemdata->m_typeId.getTypeID1() == 3
                   && itemdata->m_typeId.getTypeID2() == 1 && itemdata->m_typeId.getTypeID3() == 13 && itemdata->m_typeId.getTypeID4() == 3)
                {
                    totalItems++;
                }
            }
        }
    }

    int maxPageSlot1 = (totalItems + slotsPerPageSlot3 - 1) / slotsPerPageSlot3;  // Sayfa sayısını hesapla
    UpdateButtonStatesSlot3(maxPageSlot1);
}



void CIFVAvatarMall::Slot3RightBtn() {
    if(ActivedPage == 0)
    {
        // Cinsiyete uygun ve CategoryID 0 olan öğelerin sayısını hesapla
        const CCharacterData* data = g_CGlobalDataManager->GetCharacter(g_pMyPlayerObj->GetCommonData()->RefObjectId);
        int totalItems = 0;
        for (std::map<int, CustomDataManager::AvatarMallStruct>::iterator it = m_CustomDataManager->AvatarMallItemList.begin();
             it != m_CustomDataManager->AvatarMallItemList.end(); ++it) {
            if (it->second.CategoryID == 0) {
                const SItemData* itemdata = &g_CGlobalDataManager->GetItemData(it->second.ItemID);
                if (data != NULL && itemdata != NULL) {
                    if(data->GetData().CharGender == itemdata->m_reqGender && itemdata->m_typeId.getTypeID1() == 3
                       && itemdata->m_typeId.getTypeID2() == 1 && itemdata->m_typeId.getTypeID3() == 13 && itemdata->m_typeId.getTypeID4() == 3)
                    {
                        totalItems++;
                    }
                }
            }
        }

        int maxPageSlot1 = (totalItems + slotsPerPageSlot3 - 1) / slotsPerPageSlot3;  // Sayfa sayısını hesapla
        if (currentPageSlot3 < maxPageSlot1) {
            currentPageSlot3++;
            UpdateSlot3Visibility();
        }
    }
}

void CIFVAvatarMall::Slot3LeftBtn() {
    if(ActivedPage == 0)
    {
        if (currentPageSlot3 > 1) {
            currentPageSlot3--;
            UpdateSlot3Visibility();
        }
    }

}

void CIFVAvatarMall::ClearSlot3()
{
    for(int i = 0; i < 100; i++)
    {
        Slot3[i]->m_itemInfo->ClearSlot();
    }
}

void CIFVAvatarMall::LoadGrapPets() {
    ClearSlot4();
    std::list<CustomDataManager::AvatarMallStruct> items;


    // CategoryID 0 olan ve cinsiyete uygun öğeleri topluyoruz
    for (std::map<int, CustomDataManager::AvatarMallStruct>::iterator it = m_CustomDataManager->AvatarMallItemList.begin();
         it != m_CustomDataManager->AvatarMallItemList.end(); ++it) {
        if (it->second.CategoryID == 1) {
            const SItemData* itemdata = &g_CGlobalDataManager->GetItemData(it->second.ItemID);
            if (itemdata != NULL) {
                if(itemdata->m_typeId.getTypeID1() == 3 && itemdata->m_typeId.getTypeID2() == 2 && itemdata->m_typeId.getTypeID3() == 1 && itemdata->m_typeId.getTypeID4() == 2)
                {
                    items.push_back(it->second);
                }
            }
        }
    }

    // ItemIndex'e göre sıralama
    items.sort(CompareAvatarMallStruct);

    // Slotlara ekleme
    int i = -1;
    for (std::list<CustomDataManager::AvatarMallStruct>::iterator it = items.begin(); it != items.end(); ++it) {
        i++;
        if (i < 50) {  // 100 slot sınırı
            Slot4Grap[i]->AddMarketItems(it->ID, it->ItemID, it->Silk, it->PetObjID);
        }
    }

    currentPageSlot4 = 1;  // Başlangıç sayfası 1 olarak ayarlanıyor
    UpdateSlot4Visibility();  // Slot görünürlüğünü güncelle
}


void CIFVAvatarMall::UpdateSlot4Visibility() {

    for (int i = 0; i < 50; i++) {
        Slot4Grap[i]->ShowGWnd((i / slotsPerPageSlot4) == (currentPageSlot4 - 1));
    }

    int totalItems = 0;
    for (std::map<int, CustomDataManager::AvatarMallStruct>::iterator it = m_CustomDataManager->AvatarMallItemList.begin();
         it != m_CustomDataManager->AvatarMallItemList.end(); ++it) {
        if (it->second.CategoryID == 1) {
            const SItemData* itemdata = &g_CGlobalDataManager->GetItemData(it->second.ItemID);
            if (itemdata != NULL) {
                if(itemdata->m_typeId.getTypeID1() == 3
                   && itemdata->m_typeId.getTypeID2() == 2 && itemdata->m_typeId.getTypeID3() == 1 && itemdata->m_typeId.getTypeID4() == 2)
                {
                    totalItems++;
                }
            }
        }
    }

    int maxPageSlot1 = (totalItems + slotsPerPageSlot4 - 1) / slotsPerPageSlot4;  // Sayfa sayısını hesapla
    UpdateButtonStatesSlot1(maxPageSlot1);
}

void CIFVAvatarMall::ClearSlot4()
{
    for(int i = 0; i < 50; i++)
    {
        Slot4Grap[i]->m_itemInfo->ClearSlot();
    }
}


void CIFVAvatarMall::LoadAttackPets() {
    ClearSlot5();
    std::list<CustomDataManager::AvatarMallStruct> items;


    // CategoryID 0 olan ve cinsiyete uygun öğeleri topluyoruz
    for (std::map<int, CustomDataManager::AvatarMallStruct>::iterator it = m_CustomDataManager->AvatarMallItemList.begin();
         it != m_CustomDataManager->AvatarMallItemList.end(); ++it) {
        if (it->second.CategoryID == 1) {
            const SItemData* itemdata = &g_CGlobalDataManager->GetItemData(it->second.ItemID);
            if (itemdata != NULL) {
                if(itemdata->m_typeId.getTypeID1() == 3 && itemdata->m_typeId.getTypeID2() == 2 && itemdata->m_typeId.getTypeID3() == 1 && itemdata->m_typeId.getTypeID4() == 1)
                {
                    items.push_back(it->second);
                }
            }
        }
    }

    // ItemIndex'e göre sıralama
    items.sort(CompareAvatarMallStruct);

    // Slotlara ekleme
    int i = -1;
    for (std::list<CustomDataManager::AvatarMallStruct>::iterator it = items.begin(); it != items.end(); ++it) {
        i++;
        if (i < 50) {  // 100 slot sınırı
            Slot5[i]->AddMarketItems(it->ID, it->ItemID, it->Silk, it->PetObjID);
        }
    }

    currentPageSlot5 = 1;  // Başlangıç sayfası 1 olarak ayarlanıyor
    UpdateSlot5Visibility();  // Slot görünürlüğünü güncelle
}


void CIFVAvatarMall::UpdateSlot5Visibility() {

    for (int i = 0; i < 50; i++) {
        Slot5[i]->ShowGWnd((i / slotsPerPageSlot5) == (currentPageSlot5 - 1));
    }

    int totalItems = 0;
    for (std::map<int, CustomDataManager::AvatarMallStruct>::iterator it = m_CustomDataManager->AvatarMallItemList.begin();
         it != m_CustomDataManager->AvatarMallItemList.end(); ++it) {
        if (it->second.CategoryID == 1) {
            const SItemData* itemdata = &g_CGlobalDataManager->GetItemData(it->second.ItemID);
            if (itemdata != NULL) {
                if(itemdata->m_typeId.getTypeID1() == 3
                   && itemdata->m_typeId.getTypeID2() == 2 && itemdata->m_typeId.getTypeID3() == 1 && itemdata->m_typeId.getTypeID4() == 1)
                {
                    totalItems++;
                }
            }
        }
    }

    int maxPageSlot1 = (totalItems + slotsPerPageSlot5 - 1) / slotsPerPageSlot5;  // Sayfa sayısını hesapla
    UpdateButtonStatesSlot2(maxPageSlot1);
}

void CIFVAvatarMall::ClearSlot5()
{
    for(int i = 0; i < 50; i++)
    {
        Slot5[i]->m_itemInfo->ClearSlot();
    }
}



void CIFVAvatarMall::LoadAwaken() {
    ClearSlot6();
    std::list<CustomDataManager::AvatarMallStruct> items;

    const CCharacterData* data = g_CGlobalDataManager->GetCharacter(g_pMyPlayerObj->GetCommonData()->RefObjectId);

    // CategoryID 0 olan ve cinsiyete uygun öğeleri topluyoruz
    for (std::map<int, CustomDataManager::AvatarMallStruct>::iterator it = m_CustomDataManager->AvatarMallItemList.begin();
         it != m_CustomDataManager->AvatarMallItemList.end(); ++it) {
        if (it->second.CategoryID == 2) {
            const SItemData* itemdata = &g_CGlobalDataManager->GetItemData(it->second.ItemID);
            if (itemdata != NULL && data != NULL) {
                if(data->GetData().CharGender == itemdata->m_reqGender && itemdata->m_typeId.getTypeID1() == 3 && itemdata->m_typeId.getTypeID2() == 1 && itemdata->m_typeId.getTypeID3() == 14 && itemdata->m_typeId.getTypeID4() == 1)
                {
                    items.push_back(it->second);
                }
            }
        }
    }

    // ItemIndex'e göre sıralama
    items.sort(CompareAvatarMallStruct);

    // Slotlara ekleme
    int i = -1;
    for (std::list<CustomDataManager::AvatarMallStruct>::iterator it = items.begin(); it != items.end(); ++it) {
        i++;
        if (i < 12) {  // 100 slot sınırı
            Slot6[i]->AddMarketItems(it->ID, it->ItemID, it->Silk, it->PetObjID);
        }
    }

    currentPageSlot6 = 1;  // Başlangıç sayfası 1 olarak ayarlanıyor
    UpdateSlot6Visibility();  // Slot görünürlüğünü güncelle
}


void CIFVAvatarMall::UpdateSlot6Visibility() {

    for (int i = 0; i < 12; i++) {
        Slot6[i]->ShowGWnd((i / slotsPerPageSlot6) == (currentPageSlot6 - 1));
    }

    int totalItems = 0;
    for (std::map<int, CustomDataManager::AvatarMallStruct>::iterator it = m_CustomDataManager->AvatarMallItemList.begin();
         it != m_CustomDataManager->AvatarMallItemList.end(); ++it) {
        if (it->second.CategoryID == 2) {
            const SItemData* itemdata = &g_CGlobalDataManager->GetItemData(it->second.ItemID);
            if (itemdata != NULL) {
                if(itemdata->m_typeId.getTypeID1() == 3
                   && itemdata->m_typeId.getTypeID2() == 1 && itemdata->m_typeId.getTypeID3() == 14 && itemdata->m_typeId.getTypeID4() == 1)
                {
                    totalItems++;
                }
            }
        }
    }

    int maxPageSlot1 = (totalItems + slotsPerPageSlot6 - 1) / slotsPerPageSlot6;  // Sayfa sayısını hesapla
    UpdateButtonStatesSlot1(maxPageSlot1);
}

void CIFVAvatarMall::ClearSlot6()
{
    for(int i = 0; i < 12; i++)
    {
        Slot6[i]->m_itemInfo->ClearSlot();
    }
}


void CIFVAvatarMall::ClearSlotVisual1()
{
    for(int i = 0; i < 100; i++)
    {
        if(Slot1[i]->m_itemInfo->SlotisLocked == 10)
        {
            Slot1[i]->m_itemInfo->SlotisLocked = 0;
        }
    }
}
void CIFVAvatarMall::ClearSlotVisual2()
{
    for(int i = 0; i < 100; i++)
    {
        if(Slot2[i]->m_itemInfo->SlotisLocked == 10)
        {
            Slot2[i]->m_itemInfo->SlotisLocked = 0;
        }
    }
}
void CIFVAvatarMall::ClearSlotVisual3()
{
    for(int i = 0; i < 100; i++)
    {
        if(Slot3[i]->m_itemInfo->SlotisLocked == 10)
        {
            Slot3[i]->m_itemInfo->SlotisLocked = 0;
        }
    }
}
void CIFVAvatarMall::ClearSlotVisual4()
{
    for(int i = 0; i < 50; i++)
    {
        if(Slot4Grap[i]->m_itemInfo->SlotisLocked == 10)
        {
            Slot4Grap[i]->m_itemInfo->SlotisLocked = 0;
        }
    }
}
void CIFVAvatarMall::ClearSlotVisual5()
{
    for(int i = 0; i < 50; i++)
    {
        if(Slot5[i]->m_itemInfo->SlotisLocked == 10)
        {
            Slot5[i]->m_itemInfo->SlotisLocked = 0;
        }
    }
}
void CIFVAvatarMall::ClearSlotVisual6()
{
    for(int i = 0; i < 12; i++)
    {
        if(Slot6[i]->m_itemInfo->SlotisLocked == 10)
        {
            Slot6[i]->m_itemInfo->SlotisLocked = 0;
        }
    }
}
void CIFVAvatarMall::CallRenderStatic()
{
    g_pCGInterface->m_IRM.GetResObj<CIFVAvatarMallBuyItemList>(AvatarMallBuyListId, 1)->ItemList.clear();
    g_pCGInterface->m_IRM.GetResObj<CIFVAvatarMallBuyItemList>(AvatarMallBuyListId, 1)->UpdateList();
    ClearSlotVisual1();
    ClearSlotVisual2();
    ClearSlotVisual3();
    ClearSlotVisual4();
    ClearSlotVisual5();
    ClearSlotVisual6();
    // CIFVItemMall::AvatarBuyWnd->ItemList.clear();
    // CIFVItemMall::AvatarBuyWnd->Clear();
    Render->Clear();
    CIFEquipment *inventory = g_pCGInterface->GetMainPopup()->GetEquipment();
    /// head 0
    /// dress 1
    /// attach 2
    HeadID = 0;
    AvatarID = 0;
    AttachID = 0;
    for (int i = 0; i < 5; i++) {
        if(inventory->Test4(i)->GetItemData() != NULL)
        {
            //printf(" TypeID = %d\n", inventory->Test4(i)->GetItemData()->m_typeId.m_type_id_value);

            if(inventory->Test4(i)->GetItemData()->IsAvatar())
            {

                AvatarID = inventory->Test4(i)->GetItemData()->RefObjectId;
                //printf(" AvatarID = %ls\n", inventory->Test4(i)->GetItemData()->CodeName.c_str());
            }
            if(inventory->Test4(i)->GetItemData()->IsAvatarHat())
            {
                HeadID = inventory->Test4(i)->GetItemData()->RefObjectId;
                //printf(" HeadID = %ls\n", inventory->Test4(i)->GetItemData()->CodeName.c_str());
            }
            if(inventory->Test4(i)->GetItemData()->IsAvatarAttach())
            {
                AttachID = inventory->Test4(i)->GetItemData()->RefObjectId;
                //printf(" AttachID = %ls\n", inventory->Test4(i)->GetItemData()->CodeName.c_str());
            }
        }
    }


    /* Render->N000016B3 = 5.000;
   Render->WhenClickedis1 = 1;
   Render->WhenRightClickedis1 = 1;
   Render->uzakyakin1 = 15.000;
   Render->solasagi = -10.000;

   Render->uzakyakin7 = 20.000; // önn arkaaa - 20.000 arkaya bakıyor

   Render->uzakyakin6 = 15;
   Render->yukariasagi = 10.000;*/


  //  printf("%u \n",Render->GetCharacterObj(g_pMyPlayerObj->GetCommonData()->RefObjectId, &inventory->Char));
    Render->FUN_005602c0(Render->GetCharacterObj(g_pMyPlayerObj->GetCommonData()->RefObjectId, &inventory->Char), 0);
    Render->Test1();

    undefined4 uStack12;
    uStack12 = 0;
    Render->Test(&uStack12);
    Render->Test2(&uStack12, 0x420d1000);

    Render->field_0x430 = 0x43fa0000;
    Render->Test3(0, 0);
    LastAtakPet = 0;
    LastPickPet = 0;
    Pet1 = 0;
    Pet2 = 0;

    Render->yukariasagi = 15.000;
    Render->N00000609 = 1;

}

#define TIMERSHOWDPS 10007
#define TIMER 10008
#define TIMER2BTN 10009

void CIFVAvatarMall::OnTimer(int timerId) {
    if (timerId == TIMERSHOWDPS) {
        this->KillTimer(TIMERSHOWDPS);

        CallRenderStatic();
    }
}

void CIFVAvatarMall::OpenButton(int timeoutSeconds) {
    this->StartTimer(TIMERSHOWDPS, timeoutSeconds);
}

void CIFVAvatarMall::UpdateMenuSize()
{
    g_pCGInterface->LockMovement(13);

    Render->Clear();
    OpenButton(100);
    // CallRenderStatic();
    LoadAvatarHats();
    LoadAvatars();
    LoadAvatarAttach();
    LoadGrapPets();
    LoadAttackPets();
    LoadAwaken();
    int PosX = 0, PosY = 0;
    PosY = (g_CGame->GetRes().res->height/2) - (this->GetSize().height/2);
    PosX = (g_CGame->GetRes().res->width/2) - (this->GetSize().width/2);
    this->MoveGWnd(PosX, PosY);
    BringToFront();
}

void CIFVAvatarMall::ActivateTabPage(BYTE page) {
    for (int i = 0; i < numberOfTabs; i++) {
        if (i == page)
            continue;
        ActivedPage = page;
        m_pTabs[i]->sub_64CC30(0);
        m_pTabs[i]->m_FontTexture.sub_8B4750(7);
        //m_pTabs[i]->m_FontTexture.sub_8B4750(2);
    }

    if(ActivedPage == 0)
    {
        UpdateSlot1Visibility();
        UpdateSlot2Visibility();
        UpdateSlot3Visibility();
        for(int i = 0; i < 50; i++)
        {
            Slot4Grap[i]->ShowGWnd(false);
            Slot5[i]->ShowGWnd(false);
        }
        for(int i = 0; i< 12; i++)
        {
            Slot6[i]->ShowGWnd(false);
        }
    }
    else if(ActivedPage == 1)
    {
        for(int i = 0; i < 100; i++)
        {
            Slot1[i]->ShowGWnd(false);
            Slot2[i]->ShowGWnd(false);
            Slot3[i]->ShowGWnd(false);
        }
        for(int i = 0; i< 12; i++)
        {
            Slot6[i]->ShowGWnd(false);
        }
        UpdateSlot4Visibility();
        UpdateSlot5Visibility();
        m_IRM.GetResObj<CIFButton>(GDR_LEFT_3, 1)->SetEnabledState(false);
        m_IRM.GetResObj<CIFButton>(GDR_RIGHT_3, 1)->SetEnabledState(false);

    }
    else if(ActivedPage == 2)
    {
        for(int i = 0; i < 100; i++)
        {
            Slot1[i]->ShowGWnd(false);
            Slot2[i]->ShowGWnd(false);
            Slot3[i]->ShowGWnd(false);
        }
        for(int i = 0; i < 50; i++)
        {
            Slot4Grap[i]->ShowGWnd(false);
            Slot5[i]->ShowGWnd(false);
        }
        UpdateSlot6Visibility();
        m_IRM.GetResObj<CIFButton>(GDR_LEFT_2, 1)->SetEnabledState(false);
        m_IRM.GetResObj<CIFButton>(GDR_RIGHT_2, 1)->SetEnabledState(false);
        m_IRM.GetResObj<CIFButton>(GDR_LEFT_3, 1)->SetEnabledState(false);
        m_IRM.GetResObj<CIFButton>(GDR_RIGHT_3, 1)->SetEnabledState(false);

    }
    m_pTabs[page]->sub_64CC30(1);

}


undefined CIFVAvatarMall::OnCloseWnd(){

    g_pCGInterface->UnLockMovement();

    if(g_pCGInterface->m_IRM.GetResObj<CIFVAvatarMallBuyItemList>(AvatarMallBuyListId, 1)->IsVisible())
    {
        g_pCGInterface->m_IRM.GetResObj<CIFVAvatarMallBuyItemList>(AvatarMallBuyListId, 1)->ShowGWnd(false);
    }
    g_pCGInterface->m_IRM.GetResObj<CIFVAvatarMallBuyItemList>(AvatarMallBuyListId, 1)->ItemList.clear();
    g_pCGInterface->m_IRM.GetResObj<CIFVAvatarMallBuyItemList>(AvatarMallBuyListId, 1)->UpdateList();
    return CIFWnd::OnCloseWnd();
}
