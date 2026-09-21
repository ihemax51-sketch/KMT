#pragma once
#include <BSLib/_internal/custom_stl.h>
#include <Rpc.h>
class CSkillData {
public:

public:
    char pad_0000[1908]; //0x0000
    int ID; //0x0774
    int GroupID; //0x0778
    std::n_wstring Basic_Code; //0x077C
    std::n_wstring Basic_Name; //0x0798
    std::n_wstring Basic_Group; //0x07B4
    int Basic_Original; //0x07D0
    byte Basic_Level; //0x07D4
    byte Basic_Activity; //0x07D5
    char pad_07D6[2]; //0x07D6
    int Basic_ChainCode; //0x07D8
    int Basic_RecycleCost; //0x07DC
    int Action_PreparingTime; //0x07E0
    int Action_CastingTime; //0x07E4
    int Action_ActionDuration; //0x07E8
    int Action_ReuseDelay; //0x07EC
    int Action_CoolTime; //0x07F0
    int Action_FlyingSpeed; //0x07F4
    int Action_Interruptable; //0x07F8
    int Action_Overlap; //0x07FC
    int Action_AutoAttackType; //0x0800
    int Action_InTown; //0x0804
    int Action_Range; //0x0808
    byte Target_Required; //0x080C
    byte TargetType_Animal; //0x080D
    byte TargetType_Land; //0x080E
    byte TargetType_Building; //0x080F
    byte TargetGroup_Self; //0x0810
    byte TargetGroup_Ally; //0x0811
    byte TargetGroup_Party; //0x0812
    byte TargetGroup_Enemy_M; //0x0813
    byte TargetGroup_Enemy_P; //0x0814
    byte TargetGroup_Neutral; //0x0815
    byte TargetGroup_DontCare; //0x0816
    byte TargetEtc_SelectDeadBody; //0x0817
    int ReqCommon_Mastery1; //0x0818
    int ReqCommon_Mastery2; //0x081C
    byte ReqCommon_MasteryLevel1; //0x0820
    byte ReqCommon_MasteryLevel2; //0x0821
    char pad_0822[2]; //0x0822
    int ReqCommon_Str; //0x0824 /// TODO CHECK
    int ReqCommon_Int; //0x0828
    // char pad_0822[10]; //0x0822
    int ReqLearn_Skill1; //0x082C
    int ReqLearn_Skill2; //0x0830
    int ReqLearn_Skill3; //0x0834
    byte ReqLearn_SkillLevel1; //0x0838
    byte ReqLearn_SkillLevel2; //0x0839
    byte ReqLearn_SkillLevel3; //0x083A
    byte N00000F37; //0x083B
    int ReqLearn_SP; //0x083C
    int ReqLearn_Race; //0x0840
    int Req_Restriction1; //0x0844
    int Req_Restriction2; //0x0848
    int ReqCast_Weapon1; //0x084C
    int ReqCast_Weapon2; //0x0850
    /*int Consume_HP; //0x0854
    int Consume_MP; //0x0858
    int Consume_HPRatio; //0x085C
    */char pad_0860[16]; //0x0860
    int UI_SkillTab; //0x0864
    int UI_SkillPage; //0x0868
    int UI_SkillColumn; //0x086C
    int UI_SkillRow; //0x0870
    int AI_AttackChance; //0x0874
    int AI_SkillType; //0x0878
    std::n_wstring UI_IconFile; //0x087C
    std::n_wstring UI_SkillName; //0x0898
    std::n_wstring UI_SkillToolTip; //0x08B4
    std::n_wstring UI_SkillToolTip_Desc; //0x08D0
    std::n_wstring UI_SkillStudy_Desc; //0x08EC
    int Param1; //0x0908
    int Param2; //0x090C
    int Param3; //0x0910
    int Param4; //0x0914
    int Param5; //0x0918
    int Param6; //0x091C
    int Param7; //0x0920
    int Param8; //0x0924
    int Param9; //0x0928
    int Param10; //0x092C
    int Param11; //0x0930
    int Param12; //0x0934
    int Param13; //0x0938
    int Param14; //0x093C
    int Param15; //0x0940
    int Param16; //0x0944
    int Param17; //0x0948
    int Param18; //0x094C
    int Param19; //0x0950
    int Param20; //0x0954
    int Param21; //0x0958
    int Param22; //0x095C
    int Param23; //0x0960
    int Param24; //0x0964
    int Param25; //0x0968
    int Param26; //0x096C
    int Param27; //0x0970
    int Param28; //0x0974
    int Param29; //0x0978
    int Param30; //0x097C
    int Param31; //0x0980
    int Param32; //0x0984
    int Param33; //0x0988
    int Param34; //0x098C
    int Param35; //0x0990
    int Param36; //0x0994
    int Param37; //0x0998
    int Param38; //0x099C
    int Param39; //0x09A0
    int Param40; //0x09A4
    int Param41; //0x09A8
    int Param42; //0x09AC
    int Param43; //0x09B0
    int Param44; //0x09B4
    int Param45; //0x09B8
    int Param46; //0x09BC
    int Param47; //0x09C0
    int Param48; //0x09C4
    int Param49; //0x09C8
    int Param50; //0x09CC
public:
    BEGIN_FIXTURE()
    ENSURE_SIZE(0x09D0)
    ENSURE_OFFSET(ID, 0x0774)
    ENSURE_OFFSET(GroupID, 0x0778)
    ENSURE_OFFSET(Basic_Code, 0x077C)
    ENSURE_OFFSET(Basic_Name, 0x0798)
    ENSURE_OFFSET(Basic_Group, 0x07B4)
    ENSURE_OFFSET(Basic_Original, 0x07D0)
    ENSURE_OFFSET(Basic_Level, 0x07D4)
    ENSURE_OFFSET(Basic_Activity, 0x07D5)
    ENSURE_OFFSET(Basic_ChainCode, 0x07D8)
    ENSURE_OFFSET(Basic_RecycleCost, 0x07DC)

    ENSURE_OFFSET(Action_PreparingTime, 0x07E0)
    ENSURE_OFFSET(Action_CastingTime, 0x07E4)
    ENSURE_OFFSET(Action_ActionDuration, 0x07E8)

    ENSURE_OFFSET(TargetGroup_Self, 0x810)

    //ENSURE_OFFSET(Consume_HP, 0x0854)
    //ENSURE_OFFSET(Consume_MP, 0x0858)




    END_FIXTURE()

    RUN_FIXTURE(CSkillData)

};