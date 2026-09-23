#pragma once
#include <IFSelectableArea.h>
#include <IFCheckBox.h>
#include <IFScrollManager.h>
#include <IObject.h>
#include <IItem.h>
#include <Data/ItemData.h>
#include <IFEdit.h>
#include "d3dx9math.h"
#include <map>
#include <set>

class CIFMacroMenuPickFilter : public CIFWnd
{
GFX_DECLARE_DYNCREATE(CIFMacroMenuPickFilter)
GFX_DECLARE_MESSAGE_MAP(CIFMacroMenuPickFilter)
private:
    CIFMacroMenuPickFilter(void);
    ~CIFMacroMenuPickFilter(void);
    bool OnCreate(long ln) override;
    void OnUpdate() override;
    int Func_4(int a2) override;
    void OnUnknownStuff();
    CIFSelectableArea** m_pTabsSecond;
    static const int numberOfTabs = 2;
    static const int tabWidth = 200;
    static const int tabHeight = 24;
    static const int tabFirstId = 300;
    undefined1 OnCloseWnd() override;
public:
    bool PetPickTimerIsRunning;
    std::map<int, unsigned long> m_lastPickupRequestTime;
    void ActivateTabPage(BYTE page);
    void SelectPickPetOrSelf();
    bool Macro_PetFilter;
    CIFCheckBox* PickViaPetCheckBox;
    CIFCheckBox* PickViaCharCheckBox;
    CIFCheckBox* OnlyRareEquiptsCB;
    CIFCheckBox* DontPickGoldCB;
    CIFCheckBox* DontPickAlchemytablesCB;
    CIFCheckBox* DontPickAlchemyStonesCB;
    CIFCheckBox* DontPickElixirsCB;
    CIFCheckBox* DontPickArrowCB;
    CIFCheckBox* DontPickReturnCB;
    CIFCheckBox* DontPickTrashCB;

    CIFCheckBox* PickDg1CB;
    CIFCheckBox* PickDg2CB;
    CIFCheckBox* PickDg3CB;
    CIFCheckBox* PickDg4CB;
    CIFCheckBox* PickDg5CB;
    CIFCheckBox* PickDg6CB;
    CIFCheckBox* PickDg7CB;
    CIFCheckBox* PickDg8CB;
    CIFCheckBox* PickDg9CB;
    CIFCheckBox* PickDg10CB;
    CIFCheckBox* PickDg11CB;
    CIFCheckBox* PickDg12CB;
    CIFCheckBox* PickDg13CB;
    CIFCheckBox* PickDg14CB;
    CIFCheckBox* PickDg15CB;
    CIFCheckBox* PickDg16CB;
    CIFCheckBox* PickDg17CB;
    CIFCheckBox* PickDg18CB;
    CIFCheckBox* DontPickHpMp;
    CIFCheckBox* DontPickVigor;
    void PickWithPet();
    bool IsRuntimeReady() const;
    bool IsPickupCandidate(CIItem* item, int itemUniqueId, const SItemData* data,
                           bool hasEmptySlot, const std::set<int>& mergeableItems);
    bool ShouldPickItem(const SItemData* data) const;
    bool IsDegreeEnabled(int degree) const;
    void SendPickupRequest(bool viaPet, int petUniqueId, int itemUniqueId);
    void CancelBtn();
    void SaveButton();
};
