#include "../source/libs/ClientLib/src/Data/MagicPopResult.h"
#include <cassert>
#include <stdio.h>

int main()
{
    // Actual shard reference rows: winning and losing coupons share a type.
    assert(IsMagicPopWinningCoupon(true, L"ITEM_MALL_GACHA_CARD_WIN"));
    assert(!IsMagicPopWinningCoupon(true, L"ITEM_MALL_GACHA_CARD_LOSE"));
    assert(!IsMagicPopWinningCoupon(false, L"ITEM_MALL_GACHA_CARD"));
    assert(!IsMagicPopWinningCoupon(false, L"ITEM_MALL_GACHA_CARD_SILVER"));
    assert(!IsMagicPopWinningCoupon(false, L"ITEM_MALL_GACHA_CARD_WIN"));
    assert(!IsMagicPopWinningCoupon(true, L""));
    assert(!IsMagicPopWinningCoupon(true, NULL));
    puts("MagicPopResult: 7 classification checks passed.");
    return 0;
}
