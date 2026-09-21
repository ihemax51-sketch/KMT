#pragma once
#include <IFButton.h>
#include <IFCheckBox.h>
#include "IFFrame.h"
#include "IFWnd.h"

class CIFMessageBox : public CIFFrame {

GFX_DECLARE_DYNAMIC_EXISTING(CIFMessageBox, 0x00EE95A0)


public:
    void SetMsgBoxHandler(int Id, int a3);
    void FUN_BTNPARTY();
    void FUN_BTNLOCATION();
    void OnClickRestart();
    void OnClickAutoRestart();
    void OnChooseGoldStall();
    void OnChooseSilkStall();
    void RefreshStallPriceTypeToggle();
    void OnUpdateIMPL();
    GFX_MSGMAP* MessageMap();
    bool OnCreateIMPL(long ln);
    void SetMessageBoxParent(CIFWnd* pWnd);
    void CreateMessageBox(int Id);
    void SetMessageBoxStyle(int Id);
public:
    int OnClickConfirm(const char* a2);
    void SetEditMsgBoxHandler(char Id);
    void MsgBoxStoreMoney();
    void MsgBoxStore();
    void ShowGWndIMPL(bool bVisible);
    void Close();
    void ReverseMap();
    void DeadPoint();

    void OpenItemMall();
    void Test();
    int OnMouseLeftDownIMPL(int a1, int x, int y);
public:
    char pad_07B0[292]; //0x07B0
    int m_nMessageBoxStyleType; //0x08D4
    char pad_08D8[768]; //0x08D8

}; //Size: 0x0BD8


