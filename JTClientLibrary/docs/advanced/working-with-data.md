# Working with data

The client is shipped with a bunch of text files. These files store info about characters (players, but also mobs, pets 
and NPCs), items, levels and others. This data can be accessed through `CGlobalDataManager`.

A global instance is available at `g_CGlobalDataManager`.

## Accessing level data

Level data tells the amount of experience required to reach a certain level. This can be either character level or job 
level.

```cpp
const SLevelData &data = g_CGlobalDataManager->GetLevelData(10);

printf("Required Exp: %d\n", data.m_expC);

printf("Required Trader Exp: %d\n", data.m_jobExpTrader);
printf("Required Hunter Exp: %d\n", data.m_jobExpHunter);
printf("Required Thief Exp: %d\n", data.m_jobExpRobber);
```

## Accessing item data

Item data tells you price and base-stats of an item. It does not contain additional attributes such as *blues*. The most
common use in the game is to check whenever the item is of a specific kind. But you can also access other fields known 
from the data files, such as codename, (NPC) price, maximum stack size or required gender.

You can get the item data by the ref item id.

```cpp
const SItemData *pItemData = item->GetItemData(4);

printf("Codename: %ls\n", pItemData->CodeName.c_str());
printf("Price: %ld\n". pItemData->Price);
printf("Stack: %d\n". pItemData->m_maxStack);
printf("Gender: %d\n". pItemData->m_reqGender);
```

### Getting item data from an `SOItem`

You may have an `SOItem` object instead of the ref item id. `SOItem` offers a function `GetItemData` that directly gives
you the item data for this item.

Note that this still does not include item dependent stats like blues.

```cpp
const CSOItem *item = inventory->GetItemBySlot(5);
const SItemData *pItemData = item->GetItemData();

if (pItemData->IsGlobalMessageScroll()) {
    // it's a global chat scroll
}
```
