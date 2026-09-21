#pragma once
#include "IFWnd.h"

class CIFOListCtrl : public CIFWnd {
public:
    void GetResObj();
public:
    char pad_036C[56]; //0x036C
    int N00001C53;     //0x03A4
    char pad_03A8[308];//0x03A8
};                     //Size: 0x04DC
