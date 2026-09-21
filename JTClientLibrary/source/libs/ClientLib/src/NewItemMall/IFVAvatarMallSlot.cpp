#include <GInterface.h>
#include <BSLib/multibyte.h>
#include "IFVAvatarMallSlot.h"
#include "IFVItemMallBuyItem.h"
#include "IFVAvatarMall.h"
#include "IFVAvatarMallBuyItemList.h"
#include <GlobalDataManager.h>
#include <ICPlayer.h>
#include <TextStringManager.h>
#include <BSLib/Debug.h>

enum {
    GDR_PURCHASE = 9,
    GDR_ADDLIST = 10
};
GFX_IMPLEMENT_DYNCREATE(CIFVAvatarMallSlot, CIFWnd)
GFX_BEGIN_MESSAGE_MAP(CIFVAvatarMallSlot, CIFWnd)
                    ONG_COMMAND(GDR_PURCHASE, &Takebtn)
                    ONG_COMMAND(GDR_ADDLIST, &AddList)
GFX_END_MESSAGE_MAP()

CIFVAvatarMallSlot::CIFVAvatarMallSlot(void)
{
    IconID = 0;
    m_itemInfo = 0;
    BS_DEBUG_LOW(">" __FUNCTION__);
}

CIFVAvatarMallSlot::~CIFVAvatarMallSlot(void)
{
    BS_DEBUG_LOW(">" __FUNCTION__);
}

bool CIFVAvatarMallSlot::OnCreate(long ln)
{
    // Populate inherited members
    CIFWnd::OnCreate(ln);
    m_IRM.LoadFromFile("clientlibrary\\resinfo\\ifvavatarmallslot.txt");
    m_IRM.CreateInterfaceSection("Create", this);



    m_itemInfo = m_IRM.GetResObj<CIFSlotWithHelp>(8, 1);
    m_itemInfo->ShowGWnd(true);
    m_itemInfo->SetClickable(false);
    // m_itemInfo->TB_Func_14("item\\etc\\icon_edge_rare.ddj", 1,0);


    //ItemSlotPic->ShowGWnd(false);
    m_itemInfo->BringToFront();


    this->ShowGWnd(false);
    return true;

}
void CIFVAvatarMallSlot::OnUpdate() {

}
void CIFVAvatarMallSlot::AddList() {

}
u_short MevcutHeadTID;
USHORT TiklananHeadTID;
USHORT GiyilenheadTID;
int GiyilenheadID = 0;


u_short MevcutAvatarTID;
USHORT TiklananAvatarTID;
USHORT GiyilenAvatarTID;
int GiyilenAvatarID = 0;


u_short MevcutAksTID;
USHORT TiklananAksTID;
USHORT GiyilenAksTID;
int GiyilenAksID = 0;
int CIFVAvatarMallSlot::OnMouseLeftUp(int a1, int x, int y) {
    //printf("a1 a2 a3 %d %d %d \n", a1, x, y);

    if (IconID != 0) {
        //selectedpic->TB_Func_13("interface\\vlib\\mall\\mall_pre_select.ddj", 1, 0);
        //  selectedpic->ShowGWnd(true);
        //selectedpic->BringToFront();
        int gender = m_itemInfo->ItemInfo->GetItemData()->m_reqGender;
        static const CCharacterData *data = NULL;
        data = g_CGlobalDataManager->GetCharacter(g_pMyPlayerObj->GetCommonData()->RefObjectId);
        int myrace = data->GetData().CharGender;

        const SItemData *newh = &g_CGlobalDataManager->GetItemData(m_itemInfo->ItemInfo->m_refObjItemId);
        if (gender == myrace) {
            if (newh->m_typeId.getTypeID3() != 14) {
                if (newh->IsAvatarHat()) {

                    if (g_pCGInterface->m_IRM.GetResObj<CIFVAvatarMall>(AvatarMallId, 1)->HeadID != 0)/// üzerindeki item type
                    {
                        const SItemData *MevcutHead = &g_CGlobalDataManager->GetItemData(g_pCGInterface->m_IRM.GetResObj<CIFVAvatarMall>(AvatarMallId, 1)->HeadID);
                        MevcutHeadTID = MevcutHead->m_typeId.getTypeID4();
                    }
                    if (m_itemInfo->ItemInfo->m_refObjItemId != 0)/// tıkladığım item type
                    {
                        const SItemData *tiklananhead = &g_CGlobalDataManager->GetItemData(m_itemInfo->ItemInfo->m_refObjItemId);
                        TiklananHeadTID = tiklananhead->m_typeId.getTypeID4();
                    }
                    if (GiyilenheadID != 0)/// bir önceki üzerime giydiğim item type
                    {
                        const SItemData *giyilenhead = &g_CGlobalDataManager->GetItemData(GiyilenheadID);
                        GiyilenheadTID = giyilenhead->m_typeId.getTypeID4();
                    }
                    if (MevcutHeadTID == TiklananHeadTID)/// üzerimdeki item type ile seçtiğim/tıkladıdğım item type eşit ise
                    {
                        g_pCGInterface->m_IRM.GetResObj<CIFVAvatarMall>(AvatarMallId, 1)->Render->RemoveItem(0, g_pCGInterface->m_IRM.GetResObj<CIFVAvatarMall>(AvatarMallId, 1)->HeadID, 1);/// üzerimdeki itemi sil
                        g_pCGInterface->m_IRM.GetResObj<CIFVAvatarMall>(AvatarMallId, 1)->HeadID = 0;                                                                            /// head ID sıfırla
                        MevcutHeadTID = 0;                                                                                                                           /// Headtype Sıfırla

                        GiyilenheadID = m_itemInfo->ItemInfo->m_refObjItemId;                                       /// giyilen item ID
                        g_pCGInterface->m_IRM.GetResObj<CIFVAvatarMall>(AvatarMallId, 1)->Render->WearItem(0, GiyilenheadID, 1);/// itemi giy
                    }
                    if (GiyilenheadTID == TiklananHeadTID)/// tıkladığım ile son giydiğim item type eşit ise
                    {
                        g_pCGInterface->m_IRM.GetResObj<CIFVAvatarMall>(AvatarMallId, 1)->Render->RemoveItem(0, GiyilenheadID, 1);/// bir önceki giydiğimi sil


                        g_pCGInterface->m_IRM.GetResObj<CIFVAvatarMall>(AvatarMallId, 1)->Render->WearItem(0, m_itemInfo->ItemInfo->m_refObjItemId, 1);/// itemi giy
                        GiyilenheadID = m_itemInfo->ItemInfo->m_refObjItemId;                                                              /// giyilen item ID
                    }
                    if (MevcutHeadTID != TiklananHeadTID && GiyilenheadTID != TiklananHeadTID) {
                        g_pCGInterface->m_IRM.GetResObj<CIFVAvatarMall>(AvatarMallId, 1)->Render->WearItem(0, m_itemInfo->ItemInfo->m_refObjItemId, 1);///
                        GiyilenheadID = m_itemInfo->ItemInfo->m_refObjItemId;                                                              /// giyilen item ID
                    }

                    g_pCGInterface->m_IRM.GetResObj<CIFVAvatarMall>(AvatarMallId, 1)->ClearSlotVisual1();
                    this->m_itemInfo->SlotisLocked = 10;

                }

                if (newh->IsAvatar()) {
                    if (g_pCGInterface->m_IRM.GetResObj<CIFVAvatarMall>(AvatarMallId, 1)->AvatarID != 0)/// üzerindeki item type
                    {
                        const SItemData *MevcutAvatar = &g_CGlobalDataManager->GetItemData(g_pCGInterface->m_IRM.GetResObj<CIFVAvatarMall>(AvatarMallId, 1)->AvatarID);
                        MevcutAvatarTID = MevcutAvatar->m_typeId.getTypeID4();
                    }
                    if (m_itemInfo->ItemInfo->m_refObjItemId != 0)/// tıkladığım item type
                    {
                        const SItemData *tiklananavatar = &g_CGlobalDataManager->GetItemData(m_itemInfo->ItemInfo->m_refObjItemId);
                        TiklananAvatarTID = tiklananavatar->m_typeId.getTypeID4();
                    }
                    if (GiyilenAvatarID != 0)/// bir önceki üzerime giydiğim item type
                    {
                        const SItemData *giyilenhead = &g_CGlobalDataManager->GetItemData(GiyilenAvatarID);
                        GiyilenAvatarTID = giyilenhead->m_typeId.getTypeID4();
                    }
                    if (MevcutAvatarTID == TiklananAvatarTID)/// üzerimdeki item type ile seçtiğim/tıkladıdğım item type eşit ise
                    {
                        g_pCGInterface->m_IRM.GetResObj<CIFVAvatarMall>(AvatarMallId, 1)->Render->RemoveItem(0, g_pCGInterface->m_IRM.GetResObj<CIFVAvatarMall>(AvatarMallId, 1)->AvatarID, 1);/// üzerimdeki itemi sil
                        g_pCGInterface->m_IRM.GetResObj<CIFVAvatarMall>(AvatarMallId, 1)->AvatarID = 0;                                                                            /// head ID sıfırla
                        MevcutAvatarTID = 0;                                                                                                                           /// Headtype Sıfırla

                        GiyilenAvatarID = m_itemInfo->ItemInfo->m_refObjItemId;                                       /// giyilen item ID
                        g_pCGInterface->m_IRM.GetResObj<CIFVAvatarMall>(AvatarMallId, 1)->Render->WearItem(0, GiyilenAvatarID, 1);/// itemi giy
                    }
                    if (GiyilenAvatarTID == TiklananAvatarTID)/// tıkladığım ile son giydiğim item type eşit ise
                    {
                        g_pCGInterface->m_IRM.GetResObj<CIFVAvatarMall>(AvatarMallId, 1)->Render->RemoveItem(0, GiyilenAvatarID, 1);/// bir önceki giydiğimi sil


                        g_pCGInterface->m_IRM.GetResObj<CIFVAvatarMall>(AvatarMallId, 1)->Render->WearItem(0, m_itemInfo->ItemInfo->m_refObjItemId, 1);/// itemi giy
                        GiyilenAvatarID = m_itemInfo->ItemInfo->m_refObjItemId;                                                            /// giyilen item ID
                    }
                    if (MevcutAvatarTID != TiklananAvatarTID && GiyilenAvatarTID != TiklananAvatarTID) {
                        g_pCGInterface->m_IRM.GetResObj<CIFVAvatarMall>(AvatarMallId, 1)->Render->WearItem(0, m_itemInfo->ItemInfo->m_refObjItemId, 1);///
                        GiyilenAvatarID = m_itemInfo->ItemInfo->m_refObjItemId;                                                            /// giyilen item ID
                    }

                    g_pCGInterface->m_IRM.GetResObj<CIFVAvatarMall>(AvatarMallId, 1)->ClearSlotVisual2();
                    this->m_itemInfo->SlotisLocked = 10;

                }

                if (newh->IsAvatarAttach()) {

                    if (g_pCGInterface->m_IRM.GetResObj<CIFVAvatarMall>(AvatarMallId, 1)->AttachID != 0)/// üzerindeki item type
                    {
                        const SItemData *MevcutAks = &g_CGlobalDataManager->GetItemData(g_pCGInterface->m_IRM.GetResObj<CIFVAvatarMall>(AvatarMallId, 1)->AttachID);
                        MevcutAksTID = MevcutAks->m_typeId.getTypeID4();
                    }
                    if (m_itemInfo->ItemInfo->m_refObjItemId != 0)/// tıkladığım item type
                    {
                        const SItemData *tiklananAks = &g_CGlobalDataManager->GetItemData(m_itemInfo->ItemInfo->m_refObjItemId);
                        TiklananAksTID = tiklananAks->m_typeId.getTypeID4();
                    }
                    if (GiyilenAksID != 0)/// bir önceki üzerime giydiğim item type
                    {
                        const SItemData *giyilenhead = &g_CGlobalDataManager->GetItemData(GiyilenAksID);
                        GiyilenAksTID = giyilenhead->m_typeId.getTypeID4();
                    }
                    if (MevcutAksTID == TiklananAksTID)/// üzerimdeki item type ile seçtiğim/tıkladıdğım item type eşit ise
                    {
                        g_pCGInterface->m_IRM.GetResObj<CIFVAvatarMall>(AvatarMallId, 1)->Render->RemoveItem(0, g_pCGInterface->m_IRM.GetResObj<CIFVAvatarMall>(AvatarMallId, 1)->AttachID, 1);/// üzerimdeki itemi sil
                        g_pCGInterface->m_IRM.GetResObj<CIFVAvatarMall>(AvatarMallId, 1)->AttachID = 0;                                                                            /// head ID sıfırla
                        MevcutAksTID = 0;                                                                                                                              /// Headtype Sıfırla

                        GiyilenAksID = m_itemInfo->ItemInfo->m_refObjItemId;                                       /// giyilen item ID
                        g_pCGInterface->m_IRM.GetResObj<CIFVAvatarMall>(AvatarMallId, 1)->Render->WearItem(0, GiyilenAksID, 1);/// itemi giy
                    }
                    if (GiyilenAksTID == TiklananAksTID)/// tıkladığım ile son giydiğim item type eşit ise
                    {
                        g_pCGInterface->m_IRM.GetResObj<CIFVAvatarMall>(AvatarMallId, 1)->Render->RemoveItem(0, GiyilenAksID, 1);/// bir önceki giydiğimi sil


                        g_pCGInterface->m_IRM.GetResObj<CIFVAvatarMall>(AvatarMallId, 1)->Render->WearItem(0, m_itemInfo->ItemInfo->m_refObjItemId, 1);/// itemi giy
                        GiyilenAksID = m_itemInfo->ItemInfo->m_refObjItemId;                                                               /// giyilen item ID
                    }
                    if (MevcutAksTID != TiklananAksTID && GiyilenAksTID != TiklananAksTID) {
                        g_pCGInterface->m_IRM.GetResObj<CIFVAvatarMall>(AvatarMallId, 1)->Render->WearItem(0, m_itemInfo->ItemInfo->m_refObjItemId, 1);///
                        GiyilenAksID = m_itemInfo->ItemInfo->m_refObjItemId;                                                               /// giyilen item ID
                    }
                    g_pCGInterface->m_IRM.GetResObj<CIFVAvatarMall>(AvatarMallId, 1)->ClearSlotVisual3();
                    this->m_itemInfo->SlotisLocked = 10;
                }
            }
        }

        if (newh->m_typeId.getTypeID1() == 3 && newh->m_typeId.getTypeID2() == 2 && newh->m_typeId.getTypeID3() == 1 && newh->m_typeId.getTypeID4() == 2) {
            unsigned int local_c = 0x40900000;/// toplama peti.


            if (g_pCGInterface->m_IRM.GetResObj<CIFVAvatarMall>(AvatarMallId, 1)->LastAtakPet == 0 && g_pCGInterface->m_IRM.GetResObj<CIFVAvatarMall>(AvatarMallId, 1)->LastPickPet == 0) {
                g_pCGInterface->m_IRM.GetResObj<CIFVAvatarMall>(AvatarMallId, 1)->Render->FUN_00561780(ObjCharID);
                g_pCGInterface->m_IRM.GetResObj<CIFVAvatarMall>(AvatarMallId, 1)->LastPickPet = ObjCharID;
                g_pCGInterface->m_IRM.GetResObj<CIFVAvatarMall>(AvatarMallId, 1)->Render->FUN_0055fe70(1, &local_c);
                g_pCGInterface->m_IRM.GetResObj<CIFVAvatarMall>(AvatarMallId, 1)->Render->FUN_005602c0(1, 0);
            } else if (g_pCGInterface->m_IRM.GetResObj<CIFVAvatarMall>(AvatarMallId, 1)->LastAtakPet == 0 && g_pCGInterface->m_IRM.GetResObj<CIFVAvatarMall>(AvatarMallId, 1)->LastPickPet != 0) {
                g_pCGInterface->m_IRM.GetResObj<CIFVAvatarMall>(AvatarMallId, 1)->Render->RemovePet(g_pCGInterface->m_IRM.GetResObj<CIFVAvatarMall>(AvatarMallId, 1)->LastPickPet);
                g_pCGInterface->m_IRM.GetResObj<CIFVAvatarMall>(AvatarMallId, 1)->Render->FUN_00561780(ObjCharID);
                g_pCGInterface->m_IRM.GetResObj<CIFVAvatarMall>(AvatarMallId, 1)->LastPickPet = ObjCharID;
                g_pCGInterface->m_IRM.GetResObj<CIFVAvatarMall>(AvatarMallId, 1)->Render->FUN_0055fe70(1, &local_c);
                g_pCGInterface->m_IRM.GetResObj<CIFVAvatarMall>(AvatarMallId, 1)->Render->FUN_005602c0(1, 0);
            } else {
                g_pCGInterface->m_IRM.GetResObj<CIFVAvatarMall>(AvatarMallId, 1)->Render->RemovePet(g_pCGInterface->m_IRM.GetResObj<CIFVAvatarMall>(AvatarMallId, 1)->LastPickPet);
                g_pCGInterface->m_IRM.GetResObj<CIFVAvatarMall>(AvatarMallId, 1)->Render->FUN_00561780(ObjCharID);
                g_pCGInterface->m_IRM.GetResObj<CIFVAvatarMall>(AvatarMallId, 1)->LastPickPet = ObjCharID;
                g_pCGInterface->m_IRM.GetResObj<CIFVAvatarMall>(AvatarMallId, 1)->Render->FUN_0055fe70(2, &local_c);
                g_pCGInterface->m_IRM.GetResObj<CIFVAvatarMall>(AvatarMallId, 1)->Render->FUN_005602c0(2, 0);
            }
            g_pCGInterface->m_IRM.GetResObj<CIFVAvatarMall>(AvatarMallId, 1)->ClearSlotVisual4();
            this->m_itemInfo->SlotisLocked = 10;
        }

        if (newh->m_typeId.getTypeID1() == 3 && newh->m_typeId.getTypeID2() == 2 && newh->m_typeId.getTypeID3() == 1 && newh->m_typeId.getTypeID4() == 1) {
            undefined4 local_c1 = 0xc0b00000;/// atak pet.

            if (g_pCGInterface->m_IRM.GetResObj<CIFVAvatarMall>(AvatarMallId, 1)->LastAtakPet == 0 && g_pCGInterface->m_IRM.GetResObj<CIFVAvatarMall>(AvatarMallId, 1)->LastPickPet == 0) {
                g_pCGInterface->m_IRM.GetResObj<CIFVAvatarMall>(AvatarMallId, 1)->Render->FUN_00561780(ObjCharID);

                g_pCGInterface->m_IRM.GetResObj<CIFVAvatarMall>(AvatarMallId, 1)->LastAtakPet = ObjCharID;
                g_pCGInterface->m_IRM.GetResObj<CIFVAvatarMall>(AvatarMallId, 1)->Render->FUN_0055fe70(1, &local_c1);
                g_pCGInterface->m_IRM.GetResObj<CIFVAvatarMall>(AvatarMallId, 1)->Render->FUN_005602c0(1, 0);

            } else if (g_pCGInterface->m_IRM.GetResObj<CIFVAvatarMall>(AvatarMallId, 1)->LastAtakPet != 0 && g_pCGInterface->m_IRM.GetResObj<CIFVAvatarMall>(AvatarMallId, 1)->LastPickPet == 0) {
                g_pCGInterface->m_IRM.GetResObj<CIFVAvatarMall>(AvatarMallId, 1)->Render->RemovePet(g_pCGInterface->m_IRM.GetResObj<CIFVAvatarMall>(AvatarMallId, 1)->LastAtakPet);
                g_pCGInterface->m_IRM.GetResObj<CIFVAvatarMall>(AvatarMallId, 1)->Render->FUN_00561780(ObjCharID);
                g_pCGInterface->m_IRM.GetResObj<CIFVAvatarMall>(AvatarMallId, 1)->LastAtakPet = ObjCharID;
                g_pCGInterface->m_IRM.GetResObj<CIFVAvatarMall>(AvatarMallId, 1)->Render->FUN_0055fe70(1, &local_c1);
                g_pCGInterface->m_IRM.GetResObj<CIFVAvatarMall>(AvatarMallId, 1)->Render->FUN_005602c0(1, 0);
            } else {
                g_pCGInterface->m_IRM.GetResObj<CIFVAvatarMall>(AvatarMallId, 1)->Render->RemovePet(g_pCGInterface->m_IRM.GetResObj<CIFVAvatarMall>(AvatarMallId, 1)->LastAtakPet);
                g_pCGInterface->m_IRM.GetResObj<CIFVAvatarMall>(AvatarMallId, 1)->Render->FUN_00561780(ObjCharID);
                g_pCGInterface->m_IRM.GetResObj<CIFVAvatarMall>(AvatarMallId, 1)->LastAtakPet = ObjCharID;
                g_pCGInterface->m_IRM.GetResObj<CIFVAvatarMall>(AvatarMallId, 1)->Render->FUN_0055fe70(2, &local_c1);
                g_pCGInterface->m_IRM.GetResObj<CIFVAvatarMall>(AvatarMallId, 1)->Render->FUN_005602c0(2, 0);
            }
            g_pCGInterface->m_IRM.GetResObj<CIFVAvatarMall>(AvatarMallId, 1)->ClearSlotVisual5();
            this->m_itemInfo->SlotisLocked = 10;
        }



        if (newh->m_typeId.getTypeID1() == 3 && newh->m_typeId.getTypeID2() == 1 && newh->m_typeId.getTypeID3() == 14 && newh->m_typeId.getTypeID4() == 1) {


            /*    DWORD effect = g_CGlobalDataManager->GetEffectIdByName(L"SKILL_MALL_NASRUN_ECCENTRICDEMON_01");
                if(effect)
                {
                    CIFEquipment *inventory = g_pCGInterface->GetMainPopup()->GetEquipment();
                    inventory->Char.SetEffect(0, (int)g_pCGInterface->m_IRM.GetResObj<CIFVAvatarMall>(AvatarMallId, 1)->Render, effect, 1, 0,0);
                 //   g_pMyPlayerObj->MakeEffect(effect);

                }*/
            g_pCGInterface->m_IRM.GetResObj<CIFVAvatarMall>(AvatarMallId, 1)->ClearSlotVisual6();
            this->m_itemInfo->SlotisLocked = 10;
        }

        if(g_pCGInterface->m_IRM.GetResObj<CIFVAvatarMallBuyItemList>(AvatarMallBuyListId, 1)->ItemList.find(newh->m_typeId.m_type_id_value)
           != g_pCGInterface->m_IRM.GetResObj<CIFVAvatarMallBuyItemList>(AvatarMallBuyListId, 1)->ItemList.end())
        {
            // g_pCGInterface->m_IRM.GetResObj<CIFVAvatarMallBuyItemList>(AvatarMallBuyListId, 1)->ItemList[newh->m_typeId.m_type_id_value];
            g_pCGInterface->m_IRM.GetResObj<CIFVAvatarMallBuyItemList>(AvatarMallBuyListId, 1)->ItemList.erase(newh->m_typeId.m_type_id_value);

            CIFVAvatarMallBuyItemList::ItemsStruct NewItem = CIFVAvatarMallBuyItemList::ItemsStruct();
            NewItem.ItemId = m_itemInfo->ItemInfo->m_refObjItemId;
            NewItem.Price = PriceSilk;
            NewItem.DBID = IconID;
            NewItem.TypeId = newh->m_typeId.m_type_id_value;
            g_pCGInterface->m_IRM.GetResObj<CIFVAvatarMallBuyItemList>(AvatarMallBuyListId, 1)->ItemList.insert(std::make_pair(newh->m_typeId.m_type_id_value, NewItem));
            g_pCGInterface->m_IRM.GetResObj<CIFVAvatarMallBuyItemList>(AvatarMallBuyListId, 1)->UpdateList();
        }
        else
        {
            g_pCGInterface->m_IRM.GetResObj<CIFVAvatarMallBuyItemList>(AvatarMallBuyListId, 1)->ItemList.erase(newh->m_typeId.m_type_id_value);

            CIFVAvatarMallBuyItemList::ItemsStruct NewItem = CIFVAvatarMallBuyItemList::ItemsStruct();
            NewItem.ItemId = m_itemInfo->ItemInfo->m_refObjItemId;
            NewItem.Price = PriceSilk;
            NewItem.DBID = IconID;
            NewItem.TypeId = newh->m_typeId.m_type_id_value;
            g_pCGInterface->m_IRM.GetResObj<CIFVAvatarMallBuyItemList>(AvatarMallBuyListId, 1)->ItemList.insert(std::make_pair(newh->m_typeId.m_type_id_value, NewItem));
            g_pCGInterface->m_IRM.GetResObj<CIFVAvatarMallBuyItemList>(AvatarMallBuyListId, 1)->UpdateList();
        }
    }
    return false;
}
void CIFVAvatarMallSlot::Takebtn()
{

    g_pCGInterface->m_IRM.GetResObj<CIFVItemMallBuyItem>(NewItemMallBuyId, 1)->MallID = 1;
    g_pCGInterface->m_IRM.GetResObj<CIFVItemMallBuyItem>(NewItemMallBuyId, 1)->IconID = IconID;

    g_pCGInterface->m_IRM.GetResObj<CIFVItemMallBuyItem>(NewItemMallBuyId, 1)->m_itemInfo->SetSlotData(m_itemInfo->ItemInfo);

    std::wstring itemname = g_CTextStringManager->GetString2(m_itemInfo->ItemInfo->GetItemData()->NameStrID.c_str())->c_str();
    g_pCGInterface->m_IRM.GetResObj<CIFVItemMallBuyItem>(NewItemMallBuyId, 1)->IconName->SetText(itemname.c_str());


    g_pCGInterface->m_IRM.GetResObj<CIFVItemMallBuyItem>(NewItemMallBuyId, 1)->Quan = 1;
    g_pCGInterface->m_IRM.GetResObj<CIFVItemMallBuyItem>(NewItemMallBuyId, 1)->Pricex = PriceSilk;

    g_pCGInterface->m_IRM.GetResObj<CIFVItemMallBuyItem>(NewItemMallBuyId, 1)->ShowGWnd(true);
    g_pCGInterface->m_IRM.GetResObj<CIFVItemMallBuyItem>(NewItemMallBuyId, 1)->UpdateMenuSize();
    g_pCGInterface->m_IRM.GetResObj<CIFVItemMallBuyItem>(NewItemMallBuyId, 1)->BringToFront();



}

void CIFVAvatarMallSlot::AddMarketItems(int ID, int ItemID, int Price, int PetObjID){

    IconID = ID;

    FillTheSlot(ItemID,1);

    wchar_t Priceb[255];

    std::wstring test = Insert(Price);
    swprintf_s(Priceb, KmtGetText(L"UIIT_KMT_TEXT_SILK"), test.c_str());

    m_IRM.GetResObj(7, 1)->SetText(Priceb);

    PriceSilk = Price;

    /*if(m_itemInfo->ItemInfo->GetItemData() == NULL)
    {
        this->ShowGWnd(false);
    }*/
    ObjCharID = PetObjID;
}


void CIFVAvatarMallSlot::FillTheSlot(int itemID, int itemCount) {
    const SItemData* data = &g_CGlobalDataManager->GetItemData(itemID);
    m_itemInfo->ShowGWnd(true);
    if (data != NULL) {
        m_itemInfo->TB_Func_13(data->AssocFileIcon.c_str(), 0, 0);

        CMsgStreamBuffer buf(0xB034);
        buf << INT32(0) << INT32(itemID);
        u_short typeID2 = data->m_typeId.getTypeID2();
        u_short typeID3 = data->m_typeId.getTypeID3();
        u_short typeID4 = data->m_typeId.getTypeID4();
        switch (typeID2) {
            case 1:
                buf << UINT8(0) << UINT64(0) << UINT32(1) << UINT8(0) << UINT8(1) << UINT8(0) << UINT8(2) << UINT8(0);
                break;
            case 2:
                switch (typeID3) {
                    case 1:
                        buf << UINT8(0x01);
                        break;
                    case 2:
                        buf << UINT32(0x00);
                        break;
                    default:
                        if (typeID4 == 3)//Magic cube
                            buf << UINT32(0x01);
                        break;
                }
                break;
            case 3:
                buf << UINT16(0x01);
                if (typeID3 == 11)
                    if (typeID4 == 1 || typeID4 == 2)
                        buf << UINT8(0x00);
                    else if (typeID3 == 14 && typeID4 == 2)
                        buf << UINT8(0x00);
                break;
        }

        CSOItem* tempItemInfo = new CSOItem();
        tempItemInfo->ReadFromPacket(&buf, 1);
        tempItemInfo->SetEnabled(true);
        if (tempItemInfo) {

            m_itemInfo->ItemInfo = tempItemInfo;
            /* if(tempItemInfo->GetItemData()->IsNasrun())
             {
                 m_itemInfo->ItemInfo = tempItemInfo;
             }
             else
             {
                 m_itemInfo->SetSlotData(tempItemInfo);
             }*/
            m_itemInfo->ItemInfo->m_quantity = itemCount;
            m_itemInfo->ItemInfo->m_OptLevel = 0;
            m_itemInfo->SetType(19);
            m_itemInfo->ShowGWnd(true);
        }
    }
}
