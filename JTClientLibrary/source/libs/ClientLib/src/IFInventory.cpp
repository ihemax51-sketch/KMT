#include <MacroAlchemy/IFAlchemyMacro.h>
#include <ExtraUI/IFItemTranslationWnd.h>
#include "IFInventory.h"
#include "Game.h"
#include "GlobalDataManager.h"
#include "NetProcessIn.h"
#include "GInterface.h"
void CIFInventory::FakePick(int nRefObjId, BYTE btSlot, int nCount) {
    const CItemData *data = g_CGlobalDataManager->GetItem(nRefObjId);
    if (data != NULL) {
        const SItemData &sItemData = data->GetData();

        CMsgStreamBuffer buf(0xB034);
        buf << BYTE(0x01) <<  BYTE(0x06) << btSlot << INT32(0) << INT32(nRefObjId);
        u_short typeID2 = sItemData.m_typeId.getTypeID2();
        u_short typeID3 = sItemData.m_typeId.getTypeID3();
        u_short typeID4 = sItemData.m_typeId.getTypeID4();

        switch (typeID2) {
            case 1:
            case 4:
            case 5:
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
                    case 3:
                        buf << USHORT(0);
                        break;
                    default:
                        if (typeID4 == 3)//Magic cube
                            buf << UINT32(0x01);
                        break;
                }
                break;
            case 3:
                buf << UINT16(nCount);
                if ((typeID3 == 11 && (typeID4 == 1 || typeID4 == 2)) ||
                    (typeID3 == 14 && typeID4 == 2)) {
                    buf << UINT8(0x00);
                }
                break;
        }

        if (p_CNetProcessIn) {
            p_CNetProcessIn->ProcessItemMovementResult(&buf);
        }

    }
}


bool CIFInventory::OnCreateIMPL(long ln) {

    /*  wnd_rect sz;
      sz.pos.x = this->GetPos().x;
      sz.pos.y = this->GetPos().y;
      sz.size.width = 60;
      sz.size.height = 20;
      shortbutton = (CIFButton *) CGWnd::CreateInstance(this, GFX_RUNTIME_CLASS(CIFButton), sz, 545454, 0);
      shortbutton->TB_Func_13("interface\\ifcommon\\ifcommon\\com_button.ddj", 0, 0);

      //  m_custom_label->m_FontTexture.sub_8B4750(7);
      shortbutton->SetFont(theApp.GetFont(3));*/

    return reinterpret_cast<bool(__thiscall*)(CIFInventory*, long)>(0x006b2440)(this, ln);
}


CSOItem *CIFInventory::GetItemBySlot(int slotId) {
    return reinterpret_cast<CSOItem *(__thiscall *)(CIFInventory *, int)>(0x6b3e50)(this, slotId);
}

CIFSlotWithHelp* CIFInventory::GetItemSlotBySlotId(int slotId) {
    return reinterpret_cast<CIFSlotWithHelp*(__thiscall*)(CIFInventory*, int)>(0x006b3ea0)(this, slotId);
}

int CIFInventory::InventorySlotCount()
{
    return reinterpret_cast<int(__thiscall*)(CIFInventory*)>(0x006af600)(this);
}


bool CIFInventory::Func_28_IMPL(CGWnd* a1, int a2, int a3) {
    if(g_pCGInterface->m_IRM.GetResObj<CIFAlchemyMacro>(AlchemyMacro, 1)->IsVisible())
    {
        return true;
    }
    else if(g_pCGInterface->m_IRM.GetResObj<CIFItemTranslationWnd>(ItemTranslationWnd, 1)->IsVisible())
    {
        return true;
    }
    else
    {
        return reinterpret_cast<bool(__thiscall*)(CIFInventory*, CGWnd*, int, int)>(0x006b5b80)(this, a1, a2, a3);
    }
}
