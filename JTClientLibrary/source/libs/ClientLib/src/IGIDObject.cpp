#include <CustomData/CustomDataManager.h>
#include "IGIDObject.h"
#include "ICPlayer.h"

const std::n_wstring &CIGIDObject::GetName() const {
    return m_name;
}

const SCommonData *CIGIDObject::GetCommonData() const {
    return m_commonData;
}

const int CIGIDObject::GetUniqueId() const {
    return m_uniqueId;
}


void CIGIDObject::ChangeTitleColor(D3DCOLOR Color)
{
    fonttexture_title.SetColor(Color);
}


void CIGIDObject::ChangeName(std::n_wstring Name)
{
    fonttexture_playername.sub_8B3B60(&Name);
}

void CIGIDObject::ChangeTitle(std::n_wstring Title)
{
    fonttexture_title.sub_8B3B60(&Title);
}
void CIGIDObject::  UpdateNameColor(UINT32 color) {;
    if(!(g_pMyPlayerObj->GetWorldID() >= 2 && g_pMyPlayerObj->GetWorldID() <= 9))
    {
        if (m_CustomDataManager->_ActiveNameColors.find(this->GetName()) != m_CustomDataManager->_ActiveNameColors.end())
        {
            reinterpret_cast<void(__thiscall*)(CIGIDObject*, UINT32)>(0x009C1920)(this, m_CustomDataManager->_ActiveNameColors[this->GetName()]);
        }
        else
        {
            reinterpret_cast<void(__thiscall*)(CIGIDObject*, UINT32)>(0x009C1920)(this, color);
        }
    }
}

bool CIGIDObject::IsChinese()
{
    return this->GetCommonData()->RefObjectId >= 1907 && this->GetCommonData()->RefObjectId <= 1932;
}
bool CIGIDObject::IsEurope()
{
    return this->GetCommonData()->RefObjectId >= 14875 && this->GetCommonData()->RefObjectId <= 14900;
}