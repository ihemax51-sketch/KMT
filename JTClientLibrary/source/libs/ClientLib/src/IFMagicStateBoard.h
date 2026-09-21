#pragma once

#include "IFWnd.h"
#include "IFMainFrame.h"
#include "IFStateSlot.h"
#include <GlobalDataManager.h>

class CIFMagicStateBoard : public CIFWnd {
GFX_DECLARE_DYNAMIC_EXISTING(CIFMagicStateBoard, 0x00eec928)
public:
    CIFMagicStateBoard();
    ~CIFMagicStateBoard();
    bool OnCreateIMPL(long ln);
    void OnUpdateIMPL();
    void RenderMyselfIMPL();
    void FUN_00804cf0(undefined4 param_1, undefined1 param_2);
    void FUN_00803cd0(int param_1, undefined4 param_2);
    int FUN_00803c10();
    void FUN_008048c0(undefined4 param_1, undefined4 param_2, undefined4 param_3, undefined4 param_4, undefined4 param_5, undefined1 param_6, undefined4 param_7, undefined param_8, undefined param_9, undefined param_10, undefined4 param_11, undefined1 param_12, undefined4 param_13);
    void FUN_008a59e0(undefined4 param_1);
public:
    //CSkillData* CSkillData; //0x036C
    //char pad_0370[8]; //0x0370
    //int SkillCount; //0x0378
    //char pad_037C[36]; //0x037C
    CSkillData* CSkillDatas; //0x036C
    std::map<int, CSkillData*> SkillList;
    std::list<void*> ZehirList;
    char pad_037C[24]; //0x037C
BEGIN_FIXTURE()
        ENSURE_SIZE(0x03A0)
        ENSURE_OFFSET(ZehirList, 0x037C)
        //   ENSURE_OFFSET(SlotList1, 0x037c)
        //  ENSURE_OFFSET(SlotList2, 0x388)
        // ENSURE_OFFSET(SlotList3, 0x394)

        //   ENSURE_OFFSET(Count, 0x0378)
    END_FIXTURE()
    RUN_FIXTURE(CIFMagicStateBoard)

};
