#pragma once

#include "IFWnd.h"
#include "IFButton.h"


class CIFCOSInfo : public CIFWnd {
GFX_DECLARE_DYNAMIC_EXISTING(CIFCOSInfo, 0x00EEC088)
public:
    enum
    {
        GDR_LEVEL_STATIC = 60,
        GDR_LEVEL_VALUE = 65,
        GDR_HIT_RATE_STATIC = 61,
        GDR_HIT_RATE_VALUE = 66,
        GDR_PHY_ATK_STATIC = 62,
        GDR_PHY_ATK_VALUE = 67,
        GDR_PHY_DEF_STATIC = 63,
        GDR_PHY_DEF_VALUE = 68,
        GDR_PARRY_RATE_STATIC = 71,
        GDR_PARRY_RATE_VALUE = 76,
        GDR_MAG_ATK_STATIC = 72,
        GDR_MAG_ATK_VALUE = 77,
        GDR_MAG_DEF_STATIC = 73,
        GDR_MAG_DEF_VALUE = 78,
        GDR_RENDER_STATIC = 1000,
        GDR_INFO_FRAME= 1004,
        GDR_STATS = 1005,
        GDR_STR = 1006,
        GDR_STR_VALUE = 1007,

        GDR_INT = 1008,
        GDR_INT_VALUE = 1009,
        GDR_ATTACK = 1010,
        GDR_CRT = 1011,
        GDR_CRT_VALUE = 1012,
        GDR_DEFEND = 1013,

        GDR_BR = 1020,
        GDR_BR_VALUE = 1021,
        GDR_SLOT1 = 1022,
        GDR_SLOT2 = 1023,
        GDR_SLOT3 = 1024,
        GDR_SLOT4 = 1025,
        GDR_SLOT5 = 1026,
        GDR_SLOT6 = 1027,
        GDR_SLOT7 = 1028,
        GDR_SLOT8 = 1029,
        GDR_SKILL_FRAME = 1030,
        GDR_SKILL_HEADER = 1031,
        GDR_SKILL_FRAME2 = 1032,
        GDR_SKILL_HEADER2 = 1033,
        GDR_SKILL_FRAME3 = 1034,
        GDR_SKILL_HEADER3 = 1035,

        GDR_SKILL_FRAME_UNIQUE_1 = 1036,
        GDR_SKILL_SLOT_UNIQUE_1 = 103777,
        GDR_SKILL_FRAME_UNIQUE_2 = 1038,
        GDR_SKILL_SLOT_UNIQUE_2 = 103977,
        GDR_SKILL_FRAME_UNIQUE_3 = 1040,
        GDR_SKILL_SLOT_UNIQUE_3 = 104177,
        GDR_SKILL_FRAME_UNIQUE_4 = 1042,
        GDR_SKILL_SLOT_UNIQUE_4 = 104377,
        GDR_SKILL_FRAME_UNIQUE_5 = 1044,
        GDR_SKILL_SLOT_UNIQUE_5 = 104577,


        GDR_SKILL_FRAME_READIED_1 = 1046,
        GDR_SKILL_SLOT_READIED_1 = 1047,
        GDR_SKILL_FRAME_READIED_2 = 1048,
        GDR_SKILL_SLOT_READIED_2 = 1049,
        GDR_SKILL_FRAME_READIED_3 = 1050,
        GDR_SKILL_SLOT_READIED_3 = 1051,
        GDR_SKILL_FRAME_READIED_4 = 1052,
        GDR_SKILL_SLOT_READIED_4 = 1053,
        GDR_SKILL_FRAME_READIED_5 = 1054,
        GDR_SKILL_SLOT_READIED_5 = 1055,

        GDR_SKILL_FRAME_FELLOW_1 = 1056,
        GDR_SKILL_SLOT_FELLOW_1 = 1057,
        GDR_SKILL_FRAME_FELLOW_2 = 1058,
        GDR_SKILL_SLOT_FELLOW_2 = 1059,
        GDR_SKILL_FRAME_FELLOW_3 = 1060,
        GDR_SKILL_SLOT_FELLOW_3 = 1061,
        GDR_SKILL_FRAME_FELLOW_4 = 1062,
        GDR_SKILL_SLOT_FELLOW_4 = 1063,
        GDR_SKILL_FRAME_FELLOW_5 = 1064,
        GDR_SKILL_SLOT_FELLOW_5 = 1065,


    };
    bool OnCreateIMPL(long ln);
    void OnUpdateIMPL();
    int OnMouseLeftDownIMPL(int a1, int x, int y);
    void FUN_007a5610(int i);
    void HideTab1();
    void HideTab2();
    void ShowTab1();
private:

    int m_petUniqueID; //0x036C
    char pad_0370[68]; //0x0370
BEGIN_FIXTURE()
        ENSURE_SIZE(948)
    END_FIXTURE()
    RUN_FIXTURE(CIFCOSInfo)
}; //size 0x3b4

