#pragma once


class CIDecoSkill
{
public:
    void CreateEffect(int dwInst, int player, int effectId, undefined4 param_4, undefined4 param_5, int param_6);
public:
    char pad_0000[92]; //0x0000
    int BuffId; //0x005C
    char pad_0060[28]; //0x0060

};