#pragma once

#include <Data/ItemMallData.h>
union uregion {
	struct {
		char y;
		char x;
	} single;
	short r;
};

#define g_CurrentIfUnderCursor (*(CGWndBase**)0x0110F608)

#define g_Region (*(uregion*)0xEEF68C)


#define SendMsg(x) do { \
    CMsgStreamBuffer& kmtMessage = (x); \
    reinterpret_cast<void (__cdecl *)(CMsgStreamBuffer&)>(0x008418D0)(kmtMessage); \
} while (0)


#define MainMenuID 1338
#define GrantNameID 1339
#define TitleManagerID 1340
#define IconManagerID 1341
#define DynamicRankingID 1342
#define UniqueHistoryID 1343
#define EventRegisterID 1344
#define EventScheduleID 1345
#define AchievementsID 1346

#define ChestID 1350

#define MacroID 1354
#define MacroMenuID 1355
#define PartyMemberViewer 1362
#define CustomQuickSlot 1363
#define CustomQuickSlotOption 1364

#define DPSID 1369
#define CustomMessageBox 1370
#define MovePartyMember 1371
#define SavedLocation 1372

#define SocialWndID 1373
#define ChangelogID 1374

#define ItemLocker 1380
#define Itemunlocker 1381

#define DailyLoginID 1383
#define NewItemMallId 1384
#define SelectMallId 1385
#define NewItemMallBuyId 1386

#define AvatarMallId 1387
#define AvatarMallBuyListId 1388
#define TeamCounterID 1389
#define JobCounterID 1393

#define CUSTOMWEBGUI 1390
#define AlchemyMacro 1395
#define SkillListSlot 100005
#define IFMacroSlotSkillId 10007

#define FtwKillCounter 1359

#define JobIconID 9996
#define ItemTranslationWnd 1361
#define SettingsWndID 1951
#define CounterWndNew 1960
struct SReservedItemMallData
{
    int unk2006;
    CItemMallData * pItemMallData;
};
