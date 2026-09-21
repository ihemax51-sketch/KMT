#pragma once

#include "IFWnd.h"
#include "IFMainFrame.h"
#include "CObjCharacter.h"

class CIFRenderStatic : public CIFStatic {
GFX_DECLARE_DYNAMIC_EXISTING(CIFRenderStatic, 0x00ee7a78)
public:
    CIFRenderStatic();
    ~CIFRenderStatic();

    int OnMouseLeftUpIMPL(int a1, int x, int y);

    unsigned int FUN_0055ee20(unsigned int param_1, short p2, short p3);
    unsigned int GetCharacterObj(undefined4 param_1, CCObjCharacter* param_2);
    CCObjCharacter* GetCharacterAnimationObject(unsigned int objectIndex);
    unsigned int FUN_005602c0(unsigned int param_1, unsigned int p2);
    void Test(unsigned int* p1);




    void Test1();

    void Test2(unsigned int* p1, unsigned int p2);

    void Test3(unsigned int p1, int p2);


    undefined Clear(void);

    //  undefined FUN_00560030(undefined4 param_1);
    int FUN_00561780(unsigned int param_1);

    void RenderMyselfIMPL();


    void FUN_00561170(undefined4 param_1, undefined4 param_2);
    void FUN_0055fe70(unsigned int param_1, unsigned int * param_2);

    void RemoveItem(undefined4 param_1, int param_2, int param_3);
    void WearItem(unsigned int param_1, int param_2, int param_3);

    undefined Initialize();
    undefined4 FUN_00560720(int param_1);
    void RemovePet(int p1);
public:

    char pad_0380[20]; //0x0380
    float N000016B3; //0x0394
    char pad_0398[40]; //0x0398
    int RightLeft1; //0x03C0
    int RightLeft2; //0x03C4
    float RightLeft11; //0x03C8
    float RightLeft22; //0x03CC
    int WhenClickedis1; //0x03D0
    int WhenRightClickedis1; //0x03D4
    char pad_03D8[16]; //0x03D8
    float uzakyakin1; //0x03E8
    float uzakyakin2; //0x03EC
    float solasagi; //0x03F0
    float uzakyakin4; //0x03F4
    float yukariasagi; //0x03F8
    char pad_03FC[16]; //0x03FC
    unsigned int uzakyakin6; //0x040C
    char pad_0410[20]; //0x0410
    float uzakyakin7; //0x0424
    char pad_0428[8]; //0x0428
    unsigned int field_0x430; //0x0430
    float uzaklastir; //0x0434
    float N000016DF; //0x0438
    char pad_043C[4]; //0x043C
    unsigned int uzakyakin8; //0x0440
    char pad_0444[8]; //0x0444
    unsigned int N000016E4; //0x044C
    char pad_0450[8]; //0x0450
    unsigned int uzakyakin9; //0x0458
    char pad_045C[4]; //0x045C
    unsigned int N000016E9; //0x0460
    char pad_0464[4]; //0x0464
    float uzakyakin10; //0x0468
    float uzakyakin11; //0x046C
    float uzakyakin12; //0x0470
    char pad_0474[132]; //0x0474
    unsigned int N0000170F; //0x04F8
    char pad_04FC[4]; //0x04FC
    unsigned int uzakyakin13; //0x0500
    char pad_0504[8]; //0x0504
    float uzakyakin14; //0x050C
    char pad_0510[8]; //0x0510
    unsigned int uzakyakin15; //0x0518
    char pad_051C[4]; //0x051C
    unsigned int uzakyakin16; //0x0520
    char pad_0524[4]; //0x0524
    unsigned int uzakyakin17; //0x0528
    unsigned int uzakyakin18; //0x052C
    float uzakyakin19; //0x0530
    char pad_0534[68]; //0x0534
    unsigned int uzakyakin20; //0x0578
    char pad_057C[4]; //0x057C
    unsigned int N00001731; //0x0580
    char pad_0584[32]; //0x0584

BEGIN_FIXTURE()
        ENSURE_SIZE(0x5a4)
        ENSURE_OFFSET(field_0x430, 0x0430)
    END_FIXTURE()
    RUN_FIXTURE(CIFRenderStatic)






};
