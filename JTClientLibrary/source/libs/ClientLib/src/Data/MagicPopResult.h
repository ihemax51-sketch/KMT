#pragma once

#include <wchar.h>

inline bool IsMagicPopWinningCoupon(bool isCouponType, const wchar_t *codeName)
{
    // Both WIN (9239) and LOSE (9240) use TypeID 3,3,14,2.
    // The shared coupon type alone does not identify a winning result.
    return isCouponType && codeName != NULL &&
           wcscmp(codeName, L"ITEM_MALL_GACHA_CARD_WIN") == 0;
}
