#pragma once
#include "IFWnd.h"
#include "IFButton.h"

class CIFSliderCtrl : public CIFWnd {
GFX_DECLARE_DYNAMIC_EXISTING(CIFSliderCtrl, 0x00EE9948)
public:
    /// \address 0065a0f0
    // void TB_Func_13(const std::string str, int a3, int a4);

    /// \address 00659da0
    void SetSliderTexture(const std::n_string& leftButtonTexturePath, const std::n_string& rightButtonTexturePath,
                          const std::n_string& midButtonTexturePath);

    /// \address 006596f0
    void FUN_006596f0(int param_1, int param_2, int param_3, undefined4 param_4, int param_5,
                      undefined4 param_6);

    /// \address 00659840
    void SetMaxButtonSlide(undefined4 param_1);

    /// \address 006599e0
    void FUN_006599e0(int param_2, int param_3, int param_4, int param_5);

    /// \address 006597d0
    void SetEnabledState(bool bState);
    void FUN_0065a390(int param_2, int param_3, int param_4);
    /// \address 006598a0
    void FUN_006598a0(int param_1, int p2);
public:
    char pad_036C[16]; //0x036C
    CIFButton *Button1; //0x037C
    CIFButton *Button2; //0x0380
    CIFButton *Button3; //0x0384
    int m_SliderMaxVol; //0x0388
    int m_SliderCurrentVol; //0x038C
    char pad_0390[56]; //0x0390
    int CurrenValueYuzde; //0x03C8

    //    char pad_036C[16];             //0x036C
    //CIFButton *Button1;            //0x037C
    //CIFButton *Button2;            //0x0380
    //CIFButton *Button3;            //0x0384
    //int m_SliderMaxVol;            //0x0388
    //int m_SliderCurrentVol;        //0x038C
    //char pad_0390[36];             //0x0390
    //std::map<int, int *> N00001FA6;//0x03B4
    //char pad_03B8[8];              //0x03B8
    //int CurrenValueYuzde;          //0x03C8

private:
BEGIN_FIXTURE()
        ENSURE_SIZE(0x3cc)
        ENSURE_OFFSET(m_SliderMaxVol, 0x388)
        ENSURE_OFFSET(m_SliderCurrentVol, 0x38c)
    END_FIXTURE()
    RUN_FIXTURE(CIFSliderCtrl)

};