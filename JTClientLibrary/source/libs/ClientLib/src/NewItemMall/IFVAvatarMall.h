#pragma once
#include <IFCheckBox.h>
#include <IFTextBox.h>
#include <IFRenderStatic.h>
#include <IFSelectableArea.h>
#include <IFScrollManager.h>
#include "IFMainFrame.h"
#include "IFVAvatarMallSlot.h"



class CIFVAvatarMall : public CIFMainFrame
{
GFX_DECLARE_DYNCREATE(CIFVAvatarMall)
GFX_DECLARE_MESSAGE_MAP(CIFVAvatarMall)
private:
    CIFVAvatarMall(void);
    ~CIFVAvatarMall(void);
    bool OnCreate(long ln) override;
    void OnUpdate() override;
public:
    void UpdateMenuSize();
    void CallRenderStatic();
    void Buyalls();
    int Func_4(int a2) override;
    void ActivateTabPage(BYTE page);
    void OnUnknownStuff();
    void OnTimer(int) override;
    void OpenButton(int timeoutSeconds);


private:
    static const int numberOfTabs = 3;
    static const int tabWidth = 73;
    static const int tabHeight = 19;
    static const int tabFirstId = 100;

private:

    CIFSelectableArea** m_pTabs;
    //  CIFVAvatarMallSlotViewer** m_slotviewer;
public:
    CIFRenderStatic* Render;
    int HeadID;
    int AvatarID;
    int AttachID;
    int Pet1;
    int Pet2;
    int LastPickPet;
    int LastAtakPet;
    int GiyilenHeadID;
    int ActivedPage;
    CIFVAvatarMallSlot * Slot1[100];
    CIFVAvatarMallSlot * Slot2[100];
    CIFVAvatarMallSlot * Slot3[100];
    CIFVAvatarMallSlot * Slot4Grap[50];
    CIFVAvatarMallSlot * Slot5[50];
    CIFVAvatarMallSlot * Slot6[12];
    void LoadAvatarHats();
    void Slot1RightBtn();
    void Slot1LeftBtn();
    void UpdateSlot1Visibility();
    int currentPageSlot1;
    int slotsPerPageSlot1;
    void UpdateButtonStatesSlot1(int maxPageSlot1);
    void ClearSlot1();

    void LoadAvatars();
    void Slot2RightBtn();
    void Slot2LeftBtn();
    void UpdateSlot2Visibility();
    int currentPageSlot2;
    int slotsPerPageSlot2;
    void UpdateButtonStatesSlot2(int maxPageSlot1);
    void ClearSlot2();



    void LoadAvatarAttach();
    void Slot3RightBtn();
    void Slot3LeftBtn();
    void UpdateSlot3Visibility();
    int currentPageSlot3;
    int slotsPerPageSlot3;
    void UpdateButtonStatesSlot3(int maxPageSlot1);
    void ClearSlot3();


    void LoadGrapPets();
    void UpdateSlot4Visibility();
    int currentPageSlot4;
    int slotsPerPageSlot4;
    void ClearSlot4();

    void LoadAttackPets();
    void UpdateSlot5Visibility();
    int currentPageSlot5;
    int slotsPerPageSlot5;
    void ClearSlot5();
    void ClearSlotVisual1();
    void ClearSlotVisual2();
    void ClearSlotVisual3();
    void ClearSlotVisual4();
    void ClearSlotVisual5();


    void LoadAwaken();
    void UpdateSlot6Visibility();
    int currentPageSlot6;
    int slotsPerPageSlot6;
    void ClearSlot6();
    void ClearSlotVisual6();

    undefined OnCloseWnd() override;
};
