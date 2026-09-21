#include "CharacterData.h"

const SCharacterData &CCharacterData::GetData() const {
    return m_data;
}
bool CCharacterData::IsGrapPet() const {
    return m_data.m_typeId.Is(TypeIdRegistry::IS_GRAP_PET);
}

bool CCharacterData::IsAttackPet() const {
    return m_data.m_typeId.Is(TypeIdRegistry::IS_ATTACK_PET);
}