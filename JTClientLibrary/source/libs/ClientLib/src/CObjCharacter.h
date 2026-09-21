#pragma once

#include "IFWnd.h"
#include "IFMainFrame.h"
#include "CObjAnimation.h"

class CICPlayer;

class CCObjCharacter : public CCObjAnimation{
public:

    // virtual CCObjCharacter();
    virtual ~CCObjCharacter();
    void FUN_00a5fb90(int clothId, int p1);
    void SetOptimizeCloth2(int clothId);
    void FUN_00a5cca0(undefined4 nDefaultWearNum, undefined4 nEquipSlot);

    void FUN_00a60470(int clothId);
    void FUN_00a609f0(undefined4 nDefaultWearNum, undefined4 nEquipSlot);
    void FUN_00a5faf0(undefined4 p1, undefined4 p2, undefined4 p3, undefined4 p4, undefined4 p5, undefined4 p6);
    undefined4 FUN_00a61c30(undefined4 p1, undefined4 p2, byte p3, byte p5);
    void UnWearItem(int clothId);
    void FUN_00a60ef0(int clothId);
    void FUN_00a60080(int clothId);
    void SetEffect(undefined4 p1, undefined4 p2,undefined4 p3,undefined4 p4,undefined4 p5,undefined4 p6);
    bool maybeWearItem(int clothId);
    void FUN_00a60c60(undefined4 nDefaultWearNum, undefined4 nEquipSlot);
    void test(undefined4 nDefaultWearNum, undefined4 nEquipSlot, undefined4 p3);
    void FUN_00a5bcd0(undefined4 p1, undefined4 p2);
    int f_00a5bfe0(int clothId);
    undefined4 FUN_00a5fe00(int clothId);
    undefined4 Fun_00a5fca0(undefined4 p1, undefined4 p2);
    void FUN_00a5f4f0(undefined4 p1, undefined4 p2, undefined4 p3, undefined4 p4,undefined4 p5);
    int GetWhat(int p1);
    undefined4 FUN_004e8f90(undefined4 param1);
    int FUN_004e8750(int param1);
    undefined4 tester(undefined4 p1, undefined4 p2);
    void Fun_00a54be0(undefined4 param_1, undefined4 p2, undefined4 p3);
    void FUN_00a548e0(undefined4 p1, undefined4 p2,undefined4 p3);
public:
//    char pad[372];
    int Unk;             //0x0030
    CICPlayer *CICPlayerPtr;      //0x0034
    int MyShowHeadItemID;//0x0038
    int MyHead7;         //0x003C
    int Myhead1;         //0x0040
    int MyChestID;       //0x0044
    int MyChest7;        //0x0048
    int MyChest2;        //0x004C
    int MyShoulderID;    //0x0050
    int MyShoulder7;     //0x0054
    int MyShoulder3;     //0x0058
    int MyHandsID;       //0x005C
    int MyHands7;        //0x0060
    int MyHands5;        //0x0064
    int MyLegsID;        //0x0068
    int MyLegs7;         //0x006C
    int MyLegs4;         //0x0070
    int MyFootID;        //0x0074
    int MyFoot7;         //0x0078
    int MyFoot6;         //0x007C
    int MyWeaponID;      //0x0080
    int MyWeapon7;       //0x0084
    int MyWeapon40;      //0x0088
    char pad_008C[8];        //0x008C
    int Is53Why;         //0x0094
    int MyJobSuitID;     //0x0098
    int MyJobSuit0;      //0x009C
    int MyJobSuit100;    //0x00A0
    char pad_00A4[108];      //0x00A4
    std::list<int> ukw;             //0x0118
    char pad_011C[24];       //0x011C
public:
    int SuitWeared;      //0x0134
    char pad_0138[108];      //0x0138



BEGIN_FIXTURE()
        ENSURE_SIZE(420)
        //ENSURE_OFFSET(MyWeaponID, 0x0080)
        //ENSURE_OFFSET(MyJobSuitID, 0x0098)
    END_FIXTURE()
    RUN_FIXTURE(CCObjCharacter)

//    char pad_0030[80]; //0x0030
//    int WeaponID; //0x0080
//    char pad_0084[20]; //0x0084
//    int JobItemSlot; //0x0098
//    char pad_009C[264]; //0x009C
//
//BEGIN_FIXTURE()
//        ENSURE_SIZE(420)
//        ENSURE_OFFSET(WeaponID, 0x0080)
//        ENSURE_OFFSET(JobItemSlot, 0x0098)
//    END_FIXTURE()
//    RUN_FIXTURE(CCObjCharacter)




};
