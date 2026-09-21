//
// Created by YUMBUL on 7.01.2023.
//

#include "CObjCharacter.h"
#include <GInterface.h>
#include <ICPlayer.h>
#include <CustomData/CustomSettingManager.h>
#include <GFX3DFunction/RStateMgr.h>

undefined4 CCObjCharacter::FUN_00a5fe00(int clothId) {
    if(m_Settings->FixNewJobSuit)
    {
        if (this->MyJobSuitID != 0)
        {
            this->FUN_00a61c30(0, 0, 0, 0);
            this->FUN_00a61c30(1, 0, 0, 0);
            this->FUN_00a61c30(2, 0, 0, 0);
            this->FUN_00a61c30(3, 0, 0, 0);
            this->FUN_00a61c30(4, 0, 0, 0);
            this->FUN_00a61c30(5, 0, 0, 0);
        }
    }


    undefined4 a = reinterpret_cast<undefined4(__thiscall *)(CCObjCharacter *, int)>(0x00a5fe00)(this, clothId);
    //printf("fe00 %d %d\n", clothId, a);
    return a;
}

void CCObjCharacter::FUN_00a5cca0(undefined4 nDefaultWearNum, undefined4 nEquipSlot)
{
    if(m_Settings->FixNewJobSuit)
    {
        //printf("%p %d %d\n", this, this->MyJobSuitID);
        if (this->MyJobSuitID != 0)
        {
            return;
        }
        if (g_pMyPlayerObj != 0x0)
        {
            if (g_pMyPlayerObj->GetJobType() != 4)
            {
                return;
            }
        }
    }

    //printf("def %d, %d\n", nDefaultWearNum, nEquipSlot);
    reinterpret_cast<void(__thiscall *)(CCObjCharacter *, undefined4, undefined4)>(0x00a5cca0)(this, nDefaultWearNum,nEquipSlot);
}


int CCObjCharacter::f_00a5bfe0(int clothId) {
    int a = reinterpret_cast<int(__thiscall *)(CCObjCharacter *, int)>(0x004e8830)(this, clothId);
    //printf("%d %d\n", clothId, a);
    return a;
}

undefined4 CCObjCharacter::Fun_00a5fca0(undefined4 p1, undefined4 p2) {
    undefined4 test = reinterpret_cast<undefined4(__thiscall *)(CCObjCharacter *, undefined4, undefined4)>(0x00a5fca0)(this, p1, p2);
    //printf("fca0 %d %d %d\n", p1, p2, test);
    return test;
}
bool CCObjCharacter::maybeWearItem(int clothId) {

    bool a = reinterpret_cast<bool(__thiscall *)(CCObjCharacter *, int)>(0x00a60220)(this, clothId);
    //printf("%d %d\n", clothId, a);
    return a;
}
void CCObjCharacter::SetEffect(undefined4 p1, undefined4 p2,undefined4 p3,undefined4 p4,undefined4 p5,undefined4 p6)
{
    reinterpret_cast<void(__thiscall *)(CCObjCharacter *, undefined4, undefined4,undefined4,undefined4,undefined4,undefined4)>(0x00a54020)(this, p1, p2, p3, p4, p5, p6);
}

void CCObjCharacter::FUN_00a60080(int clothId) {// bunu mu hoklad�n he
    //printf("%d \n", clothId);
    if (this->MyJobSuitID == 48100) {
        return;
    }
    reinterpret_cast<void(__thiscall *)(CCObjCharacter *, int)>(0x00a5ba90)(this, clothId);
}
void CCObjCharacter::FUN_00a5fb90(int clothId, int p1) {
    //printf("FUN_00a5fb90 == %d %d\n", clothId, p1);
    reinterpret_cast<void(__thiscall *)(CCObjCharacter *, int, int)>(0x00a5fb90)(this, clothId, p1);

}
void CCObjCharacter::UnWearItem(int clothId) {
    //printf("%d \n", clothId);
    reinterpret_cast<void(__thiscall *)(CCObjCharacter *, int)>(0x00a60160)(this, clothId);
}
void CCObjCharacter::FUN_00a60ef0(int clothId) {
    if (this->MyJobSuitID != 0) {
        return;
    }
    //printf("%d \n", clothId);
    reinterpret_cast<void(__thiscall *)(CCObjCharacter *, int)>(0x00a60ef0)(this, clothId);
}
void CCObjCharacter::FUN_00a60470(int clothId) {
    //printf("%d \n", clothId);
    reinterpret_cast<void(__thiscall *)(CCObjCharacter *, int)>(0x00a60470)(this, clothId);
}
undefined4 CCObjCharacter::tester(undefined4 p1, undefined4 p2) {
    undefined4 test = reinterpret_cast<undefined4(__thiscall *)(CCObjCharacter *, undefined4, undefined4)>(0x00a53610)(this, p1, p2);
    //printf("%p %p %p\n", p1, p2, test);
    return test;
}
int CCObjCharacter::GetWhat(int p1)
{
    int a = reinterpret_cast<undefined4(__thiscall *)(CCObjCharacter *, int)>(0x00a5fd30)(this, p1);
    //printf("%d %d \n", a, p1);
    return a;
}
void CCObjCharacter::SetOptimizeCloth2(int clothId) {
    /*  undefined4 uVar2;

      int iVar1 = this->SuitWeared;
      if (iVar1 != clothId) {
          if (clothId == -1) {
              iVar1 = g_CharacterVisualChangeDataManager.FUN_004ec340(iVar1);
              FUN_00a5fb90(iVar1,0);
          }
          else {
              if (iVar1 != -1) {
                  uVar2 = g_CharacterVisualChangeDataManager.FUN_004ec340(iVar1);
                  FUN_00a5fb90(uVar2,0);
              }
              iVar1 = g_CharacterVisualChangeDataManager.FUN_004ec340(clothId);
              FUN_00a609f0(iVar1,0);
          }
          if (iVar1 != 0) {
              this->SuitWeared = clothId;
          }
      }
  */
    //*(int *) ((int) this + 0xb0) = 1;
    //return;
    //if (clothId != 0) {
    //    *(int *) ((int) this + 0x134) = 1;
    //}
    reinterpret_cast<void(__thiscall *)(CCObjCharacter *, int)>(0x00A60F90)(this, clothId);
    //if (clothId != 0) {
    //    *(int *) ((int) this + 0x134) = 1;
    //}
}

undefined4 CCObjCharacter::FUN_00a61c30(undefined4 slot, undefined4 itemid, byte plus, byte p4) {

    undefined4 p = reinterpret_cast<undefined4(__thiscall *)(CCObjCharacter *, undefined4, undefined4, byte, byte)>(0x00a61010)(this, slot, itemid, plus, p4);

    return p;
}
void CCObjCharacter::FUN_00a609f0(undefined4 nDefaultWearNum, undefined4 nEquipSlot) /// denendi.
{
    //printf("FUN_00a609f0 = %d, %d\n", nDefaultWearNum, nEquipSlot);
    reinterpret_cast<void(__thiscall *)(CCObjCharacter *, undefined4, undefined4)>(0x00a609f0)(this, nDefaultWearNum, nEquipSlot);
}




void CCObjCharacter::FUN_00a60c60(undefined4 nDefaultWearNum, undefined4 nEquipSlot)/// denendi.
{

    //printf("def %d, %d\n", nDefaultWearNum, nEquipSlot);
    reinterpret_cast<void(__thiscall *)(CCObjCharacter *, undefined4, undefined4)>(0x00a60c60)(this, nDefaultWearNum, nEquipSlot);
}


void CCObjCharacter::test(undefined4 nDefaultWearNum, undefined4 nEquipSlot, undefined4 p3)
{

    //printf("def %d, %d %d\n", nDefaultWearNum, nEquipSlot, p3);
    reinterpret_cast<void(__thiscall *)(CCObjCharacter *, undefined4, undefined4, undefined4)>(0x00a5eac0)(this, nDefaultWearNum, nEquipSlot, p3);
}
void CCObjCharacter::FUN_00a5bcd0(undefined4 p1, undefined4 p2)
{
    //printf("%d %d \n", p1, p2);
    if(p2 == 6)
    {
        p2 = 700;
    }
    if(p2 == 68)
    {
        p2 = 800;
    }
    reinterpret_cast<void(__thiscall *)(CCObjCharacter *, undefined4, undefined4)>(0x00a5bcd0)(this, p1, p2);
}
void CCObjCharacter::Fun_00a54be0(undefined4 param_1, undefined4 p2, undefined4 p3)
{
    //printf("%d %d %d %p\n", param_1, p2, p3, this);
    reinterpret_cast<void(__thiscall *)(CCObjCharacter *, undefined4, undefined4, undefined4)>(0x00a54be0)(this, param_1, p2, p3);
}
void CCObjCharacter::FUN_00a5f4f0(undefined4 p1, undefined4 p2, undefined4 p3, undefined4 p4, undefined4 p5) {
    //printf("%d %d %d %d %d\n", p1, p2, p3, p4, p5);
    /// p2 CICPlayer ptr,
    /// p3 skill ID
    /// p4 UNK

    ///1212 p2==227231540 8296 665925256 p4==227231540=CICPlayerToo p5==663609676 CObj
    reinterpret_cast<void(__thiscall *)(CCObjCharacter *, undefined4, undefined4, undefined4, undefined4, undefined4)>(0x00a54f00)(this, p1, p2, p3, p4, p5);
}

void CCObjCharacter::FUN_00a5faf0(undefined4 p1, undefined4 p2, undefined4 p3, undefined4 p4, undefined4 p5, undefined4 p6)
{
   // printf("%d %d %d %d %d %d\n", p1, p2, p3, p4, p5, p6);
    reinterpret_cast<void(__thiscall*)(CCObjCharacter*, undefined4, undefined4, undefined4, undefined4, undefined4, undefined4)>(0x00a5faf0)(this,
                                                                                                                                             p1, p2, p3, p4, p5, p6);
}
undefined4 CCObjCharacter::FUN_004e8f90(undefined4 param1)
{

    undefined4 a = reinterpret_cast<undefined4(__thiscall *)(CCObjCharacter *, undefined4)>(0x004e8f90)(this, param1);
    //printf("%d %d %d\n", this, param1, a);
    return a;
}

int CCObjCharacter::FUN_004e8750(int param1) {


    int a = reinterpret_cast<int(__thiscall *)(CCObjCharacter *, int)>(0x004e8750)(this, param1);
    //printf("%p ---- - %d ------- %d\n", this, param1, a);
    return a;
}
void CCObjCharacter::FUN_00a548e0(undefined4 p1, undefined4 p2,undefined4 p3){
    //printf("%d %d %d \n", p1, p2, p3);
    reinterpret_cast<void(__thiscall *)(CCObjCharacter *, undefined4, undefined4, undefined4)>(0x00a54be0)(this, p1, p2, p3);
}

