#pragma once


#include "NIFMainFrame.h"
#include "NIFAlchemyWnd.h"
class CNIFSocketWnd : public CNIFFrame
{

};
enum
{
    WEAPON = 1,
    ARMOR = 2,
    SHIELD = 3,
    ACC = 4,

    GDR_boxforspoofnothing = 5017,
    GDR_boxforenhancernothing = 5015,
    GDR_ITEM_SLOT = 5022,
    GDR_ITEM_SLOT_ENC = 5023,
    GDR_ITEM_SLOT_PROOF = 5024,
    GDR_START_BUTTON = 5026,
    GDR_DECORATED_STATIC = 5027,
    GDR_PROGRESS_BAR = 5028,
};
class CNIFEnchantWnd : public CNIFMainFrame
{
public:
    bool OnCreateIMPL(long ln);
    int Func_43();
    GFX_MSGMAP* MessageMap();
    int GetItemInfoFromSlot();
    void PressNewAlchemyTab();
    void PressAlchemyTabButton();
    void PressSocketTabButton();
    bool OnCloseWndIMPL();
    void ShowEnchancerAndSpoofSlot();
    void HideEnchancerAndSpoofSlot();
    int GetDegreeLevel(int itemClass);
public:
    CNIFAlchemyWnd *CNIFAlchemyWndPtr; //0x0798
    void *CNIFSocketWndPtr; //0x079C
    char pad_07A0[24]; //0x07A0


bool GetDegreeCondition(int DegreeValue);
bool GetDegreeConditionForProofStone(int DegreeValue);
bool GetEnhancerInfoFromSlot();
void SetStartButtonState(bool T);
void PressNewAlchemyStart();
void StartAlchemyWOProofStone();
void OnTimerIMPL(int timerId);
void AlchemyIsSucces();
    void AlchemyIsFailed();
void ClearProofSlot();
}; //Size: 0x07B8