#pragma once
#include <BSLib/_internal/custom_stl.h>
#include <ghidra/undefined.h>
#include <map>
#include <RpcNdr.h>

class SSkillData {
public:

public:
    int ID;                      //0x0000
    int GroupID;                 //0x0004
    std::n_wstring Basic_Code;             //0x0008
    std::n_wstring Basic_Name;   //0x0024
    std::n_wstring Basic_Group;        //0x0040
    int Basic_Original;              //0x005C
    byte Basic_Level;             //0x0060
    byte Basic_Activity;          //0x0061
    byte N000002BE;               //0x0062
    byte N000003D5;               //0x0063
    int Basic_ChainCode;         //0x0064
    int Basic_RecycleCost;       //0x0068
    int Action_PreparingTime;    //0x006C
    int Action_CastingTime;      //0x0070
    int Action_ActionDuration;   //0x0074
    int Action_ReuseDelay;       //0x0078
    int Action_CoolTime;         //0x007C
    int Action_FlyingSpeed;      //0x0080
    int Action_Interruptable;    //0x0084
    int Action_Overlap;          //0x0088
    int Action_AutoAttackType;   //0x008C
    int Action_InTown;           //0x0090
    int Action_Range;            //0x0094
    byte Target_Required;         //0x0098
    byte TargetType_Animal;       //0x0099
    byte TargetType_Land;         //0x009A
    byte TargetType_Building;     //0x009B
    byte TargetGroup_Self;        //0x009C
    byte TargetGroup_Ally;        //0x009D
    byte TargetGroup_Party;       //0x009E
    byte TargetGroup_Enemy_M;     //0x009F
    byte TargetGroup_Enemy_P;     //0x00A0
    byte TargetGroup_Neutral;     //0x00A1
    byte TargetGroup_DontCare;    //0x00A2
    byte TargetEtc_SelectDeadBody;//0x00A3
    int ReqCommon_Mastery1;       //0x00A4
    int ReqCommon_Mastery2;       //0x00A8
    int ReqCommon_MasteryLevel1;  //0x00AC
    int ReqCommon_MasteryLevel2;  //0x00B0
    unsigned short ReqCommon_Str;          //0x00B4
    unsigned short ReqCommon_Int;  //0x00B6
    int ReqLearn_Skill1;         //0x00B8
    int ReqLearn_Skill2;         //0x00BC
    int ReqLearn_Skill3;         //0x00C0
    byte ReqLearn_SkillLevel1;    //0x00C4
    byte ReqLearn_SkillLevel2;   //0x00C5
    byte ReqLearn_SkillLevel3;    //0x00C6
    byte N00000411;              //0x00C7
    int ReqLearn_SP;             //0x00C8
    int ReqLearn_Race;            //0x00CC
    int Req_Restriction1;        //0x00D0
    int Req_Restriction2;        //0x00D4
    int ReqCast_Weapon1;         //0x00D8
    int ReqCast_Weapon2;         //0x00DC
    int Consume_HP;              //0x00E0
    int Consume_MP;              //0x00E4
    int N000002E5;               //0x00E8
    int N000002E6;               //0x00EC
    int UI_SkillTab;             //0x00F0
    int UI_SkillPage;            //0x00F4
    int UI_SkillColumn;          //0x00F8
    int UI_SkillRow;             //0x00FC
    int AI_AttackChance;         //0x0100
    int AI_SkillType;            //0x0104
    std::n_wstring UI_IconFile;            //0x0108
    std::n_wstring UI_SkillName; //0x0124
    std::n_wstring UI_SkillToolTip;    //0x0140
    std::n_wstring UI_SkillToolTip_Desc;//0x015C
    std::n_wstring UI_SkillStudy_Desc;  //0x0178
    int Param_1;                 //0x0194
    int Param_2;                 //0x0198
    int Param_3;                 //0x019C
    int Param_4;                 //0x01A0
    int Param_5;                 //0x01A4
    int Param_6;                 //0x01A8
    int Param_7;                 //0x01AC
    int Param_8;                 //0x01B0
    int Param_9;                 //0x01B4
    int Param_10;                //0x01B8
    int Param_11;                //0x01BC
    int Param_12;                //0x01C0
    int Param_13;                //0x01C4
    int Param_14;                //0x01C8
    int Param_15;                //0x01CC
    int Param_16;                //0x01D0
    int Param_17;                //0x01D4
    int Param_18;                //0x01D8
    int Param_19;                //0x01DC
    int Param_20;                //0x01E0
    int Param_21;                //0x01E4
    int Param_22;                //0x01E8
    int Param_23;                //0x01EC
    int Param_24;                //0x01F0
    int Param_25;                //0x01F4
    int Param_26;                //0x01F8
    int Param_27;                //0x01FC
    int Param_28;                //0x0200
    int Param_29;                //0x0204
    int Param_30;                //0x0208
    int Param_31;                //0x020C
    int Param_32;                //0x0210
    int Param_33;                //0x0214
    int Param_34;                //0x0218
    int Param_35;                //0x021C
    int Param_36;                //0x0220
    int Param_37;                //0x0224
    int Param_38;                //0x0228
    int Param_39;                //0x022C
    int Param_40;                //0x0230
    int Param_41;                //0x0234
    int Param_42;                //0x0238
    int Param_43;                //0x023C
    int Param_44;                //0x0240
    int Param_45;                //0x0244
    int Param_46;                //0x0248
    int Param_47;                //0x024C
    int Param_48;                //0x0250
    int Param_49;                //0x0254
    int Param_50;                //0x0258
};