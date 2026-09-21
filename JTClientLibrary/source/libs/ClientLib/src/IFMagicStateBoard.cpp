#include "IFMagicStateBoard.h"

GFX_IMPLEMENT_DYNAMIC_EXISTING(CIFMagicStateBoard, 0x00eec928)

CIFMagicStateBoard::CIFMagicStateBoard(){
    reinterpret_cast<CIFMagicStateBoard *(__thiscall *)(CIFMagicStateBoard *)>(0x00806210)(this);
}
CIFMagicStateBoard::~CIFMagicStateBoard(){
    reinterpret_cast<CIFMagicStateBoard *(__thiscall *)(CIFMagicStateBoard *)>(0x00806350)(this);
}


bool CIFMagicStateBoard::OnCreateIMPL(long ln){
    bool b = reinterpret_cast<bool(__thiscall *)(CIFMagicStateBoard *, long)>(0x007feda0)(this, ln);

    return b;
}

void CIFMagicStateBoard::RenderMyselfIMPL() {
    reinterpret_cast<void (__thiscall *)(CIFMagicStateBoard *)>(0x006550c0)(this);
}

void CIFMagicStateBoard::OnUpdateIMPL()
{
    //FUN_00806d50(1, 0, 0,0, 35400);
    reinterpret_cast<void (__thiscall *)(CIFMagicStateBoard *)>(0x008056a0)(this);
}
void CIFMagicStateBoard::FUN_00804cf0(undefined4 param_1, undefined1 param_2)
{
    //printf("this = %p\n", this);
    //printf("p1 %d,,, %d\n", param_1, param_2);

    reinterpret_cast<void(__thiscall *)(CIFMagicStateBoard *, undefined4, undefined1)>(0x00803cd0)(this,param_1, param_2);
}


void CIFMagicStateBoard::FUN_00803cd0(int param_1, undefined4 param_2)
{
    reinterpret_cast<void(__thiscall *)(CIFMagicStateBoard *, int, undefined4)>(0x00803cd0)(this, param_1, param_2);
}


int CIFMagicStateBoard::FUN_00803c10()
{
    return reinterpret_cast<int(__thiscall *)(CIFMagicStateBoard *)>(0x00803c10)(this);
}


void CIFMagicStateBoard::FUN_008048c0(undefined4 param_1, undefined4 param_2, undefined4 param_3, undefined4 param_4,
                                      undefined4 param_5, undefined1 param_6, undefined4 param_7,
                                      undefined param_8, undefined param_9, undefined param_10, undefined4 param_11, undefined1 param_12, undefined4 param_13)
{
    // this->TB_Func_13("interface\\vlib\\server_max_level.ddj", 1, 0);
    //printf("wtf %d / %d / %d / %d / %d / %d / %d / %d / %d / %d / %d / %d / %d\n", param_1, param_2,
    //param_3,param_4,param_5,param_6,param_7,param_8,param_9,param_10,param_11,param_12,param_13);
    reinterpret_cast<void(__thiscall *)(CIFMagicStateBoard *, undefined4, undefined4, undefined4, undefined4, undefined4, undefined1, undefined4,
                                        undefined, undefined, undefined,
                                        undefined4, undefined1, undefined4)>(0x008048c0)(this, param_1, param_2, param_3, param_4, param_5, param_6,param_7,
                                                                                         param_8, param_9, param_10, param_11, param_12, param_13);

}


void CIFMagicStateBoard::FUN_008a59e0(undefined4 param_1)
{
    //printf("edduadala == %d\n", param_1);
    reinterpret_cast<void(__thiscall *)(CIFMagicStateBoard *, undefined4)>(0x00803c10)(this, param_1);
}