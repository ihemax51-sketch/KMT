#include "ItemData.h"
#include "MagicPopResult.h"

const SItemData &CItemData::GetData() const {
    return mData;
}

bool SItemData::IsGlobalMessageScroll() const {
    return m_typeId.Is(TypeIdRegistry::ITEM_ETC_SCROLL_GLOBALCHATTING);
}

bool SItemData::IsItemEtc() const {
    return m_typeId.Is(TypeIdRegistry::ITEM_ETC);
}

bool SItemData::IsAmmo() const {
    return m_typeId.Is(TypeIdRegistry::ITEM_ETC_AMMO);
}

bool SItemData::IsItemCOS() const {
    return m_typeId.Is(TypeIdRegistry::ITEM_COS);
}

bool SItemData::IsReverse() const {
    return m_typeId.Is(TypeIdRegistry::ITEM_ETC_SCROLL_REVERSE);
}

bool SItemData::IsAvatar() const {
    return m_typeId.Is(TypeIdRegistry::ITEM_AVATAR);
}
bool SItemData::IsAvatarHat() const {
    return m_typeId.Is(TypeIdRegistry::ITEM_AVATAR_HAT);
}
bool SItemData::IsAvatarAttach() const {
    return m_typeId.Is(TypeIdRegistry::ITEM_AVATAR_ATTACH);
}


bool SItemData::IsNasrun() const {
    return m_typeId.Is(TypeIdRegistry::ITEM_NASRUN_AMALRUN);
}

bool SItemData::IsHPPotion() const {
    return m_typeId.Is(TypeIdRegistry::ITEM_HP_POTION);
}
bool SItemData::IsMPPotion() const {
    return m_typeId.Is(TypeIdRegistry::ITEM_MP_POTION);
}
bool SItemData::IsVIGOR() const {
    return m_typeId.Is(TypeIdRegistry::ITEM_VIGOR);
}
bool SItemData::IsPill() const {
    return m_typeId.Is(TypeIdRegistry::ITEM_PILL);
}
bool SItemData::IsPuriPill() const {
    return m_typeId.Is(TypeIdRegistry::ITEM_PURIPILL);
}
bool SItemData::IsPETHpPotion() const {
    return m_typeId.Is(TypeIdRegistry::ITEM_COS_HP_POTION);
}
bool SItemData::IsPETMpPotion() const {
    return m_typeId.Is(TypeIdRegistry::ITEM_COS_MP_POTION);
}

bool SItemData::IsCureCOS() const {
    return m_typeId.Is(TypeIdRegistry::ITEM_CURE_COS);
}
bool SItemData::IsPetRes() const {
    return m_typeId.Is(TypeIdRegistry::ITEM_PET_RES);
}

bool SItemData::IsAttackPet() const {
    return m_typeId.Is(TypeIdRegistry::ITEM_SUMMON_ATTACK_PET);
}

bool SItemData::IsResurrectScroll() const {
    return m_typeId.Is(TypeIdRegistry::ITEM_RESURRECT_SCROLL);
}

bool SItemData::IsReturnScroll() const {
    return m_typeId.Is(TypeIdRegistry::IS_RETURN_SCROLL);
}
bool SItemData::IsRepairHammer() const {
    return m_typeId.Is(TypeIdRegistry::IS_REPAIR_HAMMER);
}

bool SItemData::IsArrow() const {
    return m_typeId.Is(TypeIdRegistry::ITEM_ARROW);
}

bool SItemData::IsBolt() const {
    return m_typeId.Is(TypeIdRegistry::ITEM_BOLT);
}
bool SItemData::IsAlchemyTablet() const{
    return m_typeId.Is(TypeIdRegistry::ITEM_ALCHEMY_TABLET);
}
bool SItemData::IsMagicStone() const{
    return m_typeId.Is(TypeIdRegistry::ITEM_MAGICSTONE);
}
bool SItemData::IsMagicStone2() const{
    return m_typeId.Is(TypeIdRegistry::ITEM_MAGICSTONE_2);
}
bool SItemData::IsAttrStone() const{
    return m_typeId.Is(TypeIdRegistry::ITEM_ATTR_STONE);
}
bool SItemData::IsElixir() const {
    return m_typeId.Is(TypeIdRegistry::ITEM_ELIXIR);
}
bool SItemData::IsAlchemyMaterial() const{
    return m_typeId.Is(TypeIdRegistry::ITEM_ALCHEMY_MATERIAL);
}
bool SItemData::IsZerkScroll() const{
    return m_typeId.Is(TypeIdRegistry::ITEM_ZERK_SC);
}

bool SItemData::IsMagicPop() const{
    return m_typeId.Is(TypeIdRegistry::ITEM_GACHA_CARD);
}
bool SItemData::IsMagicPopWinCard() const{
    return IsMagicPopWinningCoupon(
            m_typeId.Is(TypeIdRegistry::ITEM_GACHA_CARD_WIN), CodeName.c_str());
}
bool SItemData::IsLuckyPowder() const{
    return m_typeId.Is(TypeIdRegistry::ITEM_LUCKY_POWDER);
}
bool SItemData::IsEnhancer() const{
    return m_typeId.Is(TypeIdRegistry::ITEM_ENHANCER);
}
bool SItemData::IsProofStone() const{
    return m_typeId.Is(TypeIdRegistry::ITEM_PROOF_STONE);
}

bool SItemData::IsChSword() const {
    return m_typeId.Is(TypeIdRegistry::ITEM_CH_SWORD);
}
bool SItemData::IsChBlade() const {
    return m_typeId.Is(TypeIdRegistry::ITEM_CH_BLADE);
}
bool SItemData::IsChSpear() const {
    return m_typeId.Is(TypeIdRegistry::ITEM_CH_SPEAR);
}
bool SItemData::IsChTBlade() const {
    return m_typeId.Is(TypeIdRegistry::ITEM_CH_TBLADE);
}
bool SItemData::IsChBow() const {
    return m_typeId.Is(TypeIdRegistry::ITEM_CH_BOW);
}


bool SItemData::IsEuSword() const {
    return m_typeId.Is(TypeIdRegistry::ITEM_EU_SWORD);
}
bool SItemData::IsEuTSword() const {
    return m_typeId.Is(TypeIdRegistry::ITEM_EU_TSWORD);
}
bool SItemData::IsEuAxe() const {
    return m_typeId.Is(TypeIdRegistry::ITEM_EU_AXE);
}
bool SItemData::IsEuDarkStaff() const {
    return m_typeId.Is(TypeIdRegistry::ITEM_EU_DARKSTAFF);
}
bool SItemData::IsEuTStaff() const {
    return m_typeId.Is(TypeIdRegistry::ITEM_EU_TSTAFF);
}
bool SItemData::IsEuCrossbow() const {
    return m_typeId.Is(TypeIdRegistry::ITEM_EU_CROSSBOW);
}
bool SItemData::IsEuDagger() const {
    return m_typeId.Is(TypeIdRegistry::ITEM_EU_DAGGER);
}
bool SItemData::IsEuHarp() const {
    return m_typeId.Is(TypeIdRegistry::ITEM_EU_HARP);
}
bool SItemData::IsEuStaff() const {
    return m_typeId.Is(TypeIdRegistry::ITEM_EU_STAFF);
}
