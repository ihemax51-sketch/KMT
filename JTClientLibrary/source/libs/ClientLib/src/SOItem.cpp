#include "SOItem.h"
#include "GlobalDataManager.h"

CSOItem::~CSOItem()
{
    reinterpret_cast<CSOItem *(__thiscall *)(CSOItem *)>(0x008bedd0)(this);

}

CSOItem::CSOItem() {
    reinterpret_cast<CSOItem *(__thiscall *)(CSOItem *)>(0x008bef90)(this);
    /*  ZeroMemory(&pad_0000[8], 32);
      field_28 = 0;
      ZeroMemory(&pad_0029[0], 0xB);
      m_refObjItemId = 0;
      ZeroMemory(&pad_0038[0], 0x54);
      memset(&pad_0038[60], -1, 24);
      m_OptLevel = 0;
      ZeroMemory(&pad_008D[0], 0xB);
      m_CurrDurability = 0;
      m_quantity = 0;
      ZeroMemory(&pad_00A0[0], 0x24);
      bluemap = std::n_map<Blue, int>();
      ZeroMemory(&pad_00D0[0], 0x10);

      m_PhyAtkPwrMax = 0;            //0x00E0
      m_PhyAtkPwrMin = 0;            //0x00E4
      m_MagAtkPwrMax = 0;            //0x00E8
      m_MagAtkPwrMin = 0;            //0x00EC
      m_BlockingRateValue = 0;       //0x00F0
      m_MaxDurability = 0;           //0x00F4
      m_PhyDefPwrValue = 0;          //0x00F8
      m_ParryRateValue = 0;          //0x00FC
      m_PhyAbsorption = 0;           //0x0100
      ZeroMemory(&pad_0104[0], 4);//0x0104
      m_CriticalValue = 0;           //0x0108
      m_MagDefPwrValue = 0;          //0x010C
      m_MagAbsorption = 0;           //0x0110
      m_PhyReinforcementMin = 0;     //0x0114
      m_PhyReinforcementMax = 0;     //0x0118
      m_MagReinforcementMin = 0;     //0x011C
      m_MagReinforcementMax = 0;     //0x0120
      m_PhyReinforcementValue = 0;   //0x0124
      m_MagReinforcementValue = 0;   //0x0128
      ZeroMemory(&pad_012C[0], 4);//0x012C
      m_AttackRateValue = 0;         //0x0130
      ZeroMemory(&pad_0134[0], 0x9C);//0x0134
      enableItem = 0;
      pad = 0;

      //i need this
      //giwe*/
}
const SItemData *CSOItem::GetItemData() const {
    const SItemData *data = NULL;

    if (m_refObjItemId != 0) {
        data = &g_CGlobalDataManager->GetItemData(m_refObjItemId);
    }

    return data;
}

int CSOItem::GetQuantity() const {
    return m_quantity;
}

bool CSOItem::IsSameItemType(const CSOItem *other) const {
    return reinterpret_cast<bool(__thiscall *)(const CSOItem *, const CSOItem *)>(0x008bac80)(this, other);
}
ObjectData* CSOItem::GetObjectData(void)
{
    return reinterpret_cast<ObjectData*(__thiscall*)(CSOItem*)>(0x008BA440)(this);
}
void CSOItem::ReadFromPacket(CMsgStreamBuffer* msg, BYTE param_3) {

    reinterpret_cast<CSOItem *(__thiscall *) (CSOItem *, CMsgStreamBuffer *, char)>(0x008be4f0)(this, msg, param_3);
}
void CSOItem::SetEnabled(bool enable) {
    this->enableItem = enable;
}