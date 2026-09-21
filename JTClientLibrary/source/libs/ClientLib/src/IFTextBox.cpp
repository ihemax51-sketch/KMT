#include <GlobalItemLinking/GlobalItemLinking.h>
#include <GFXMainFrame/Controler.h>
#include "IFTextBox.h"
#include "ICPlayer.h"
#include "IFChatViewer.h"
#include "GInterface.h"
GFX_IMPLEMENT_DYNAMIC_EXISTING(CIFTextBox, 0x00ee9760)

void CIFTextBox::SetIndentAfterBreak(bool indentAfterBreak) {
    m_identAfterBreak = indentAfterBreak;
}

void CIFTextBox::sub_64F8A0(std::n_wstring &str, int a3, int a4, int a5, int a6, int a7, int a8) {
   // printf("%ls %d %d %d %d %d %d \n", str.c_str(), a3, a4, a5, a6, a7, a8);
    reinterpret_cast<void(__thiscall*)(CIFTextBox*, std::n_wstring*, int, int, int, int, int, int)>(0x64F8A0)(this, &str, a3, a4, a5, a6, a7, a8);
}

void CIFTextBox::sub_64F660(std::n_string &str, int a3, int a4, int a5, int a6, int a7, int a8) {
    reinterpret_cast<void(__thiscall*)(CIFTextBox*, std::n_string*, int, int, int, int, int, int)>(0x64F660)(this, &str, a3, a4, a5, a6, a7, a8);
}

void CIFTextBox::sub_638B50(int a2) {
    reinterpret_cast<void(__thiscall *)(CIFTextBox *, int)>(0x638B50)(this, a2);
}

void CIFTextBox::sub_64E380(int a2) {
    reinterpret_cast<void(__thiscall *)(CIFTextBox *, int)>(0x64E380)(this, a2);
}

void CIFTextBox::sub_638C70(int a2) {
    reinterpret_cast<void(__thiscall *)(CIFTextBox *, int)>(0x638C70)(this, a2);
}

void CIFTextBox::sub_638D50(int a2) {
    reinterpret_cast<void(__thiscall *)(CIFTextBox *, int)>(0x638D50)(this, a2);
}

void CIFTextBox::sub_638D40(int a2) {
    reinterpret_cast<void(__thiscall *)(CIFTextBox *, int)>(0x638D40)(this, a2);
}


void CIFTextBox::Removeline(int line) {
    reinterpret_cast<void(__thiscall *)(CIFTextBox *, int)>(0x0063AFF0)(this, line);
}

void CIFTextBox::ClearAllLines() {
    // The native text box retains two internal/sentinel entries beyond the
    // public item count. Removing only GetNumberOfItems() entries leaves an
    // old popup row behind and makes active categories appear duplicated.
    const int lastRemoval = GetNumberOfItems() + 1;
    for (int index = 0; index <= lastRemoval; ++index) {
        Removeline(0);
    }
    m_CurrentLines = 0;
}

int CIFTextBox::OnHover(int a1, int a2) {
    if (g_pMyPlayerObj != 0x0) {
        //make sure that we dealing with the chat window
        {

            if(this->GetParentControl()->IsSame(GFX_RUNTIME_CLASS(CIFChatViewer)))
            {
                CIFChatViewer *pParent = (CIFChatViewer*)GetParentControl();
                if(pParent != NULL)
                {
                    if(this->UniqueID() == pParent->m_ChatLists[0]->UniqueID())
                    {
                        CIFSlotWithHelp *itemLinkSlot = g_pCGInterface->GetGuiFromList<CIFSlotWithHelp>(13402);

                        //initialize the slot
                        CIFListCtrl * testlist = pParent->m_ChatLists[0];
                        CIFListCtrl::SLineOfText *line = testlist->GetTextByIndex(a1);
                        if(line)
                        {
                            std::n_wstring str;
                            line->m_font->GetText(&str);
                            if(str.find(':') != std::n_wstring::npos)
                            {

                            }
                        }

                        if (!itemLinkSlot) {
                            RECT rect = {0, 0, 0, 0};
                            itemLinkSlot = (CIFSlotWithHelp *) g_pCGInterface->CreateInstance(g_pCGInterface, GFX_RUNTIME_CLASS(CIFSlotWithHelp), rect, 13402, 0);
                            itemLinkSlot->ShowGWnd(false);
                        }
                        if (a2 == 0 && g_pCGInterface->m_helperWindow) {
                            itemLinkSlot->ShowGWnd(false);
                            itemLinkSlot->ItemInfo = NULL;
                            g_pCGInterface->m_helperWindow->Reset();
                            g_pCGInterface->m_helperWindow->ShowGWnd(false);
                            g_pCGInterface->m_helperWindow->m_ownerWindow = NULL;
                            return reinterpret_cast<int(__thiscall *)(CIFTextBox *, int, int)>(0x63A100)(this, a1, a2);
                        }
                        if (g_pCGInterface->m_helperWindow && !g_pCGInterface->m_helperWindow->IsVisible()) {
                            if (a1 != 0) {
                                for (std::list<GlobalChat>::iterator iter = g_global->GlobalChatLog.begin(); iter != g_global->GlobalChatLog.end(); iter++) {
                                    if (iter->index == a1) {
                                        if (a2 == 1 && iter->Message.find(L">") != std::wstring::npos && iter->Message.find(L"<") != std::wstring::npos) {
                                            //checking if the item already on our list we will show the information , otherwise we will request it
                                            if (iter->listBytes.size() > 0) {
                                                CMsgStreamBuffer buf(0xB034);
                                                for (std::list<BYTE>::iterator it = iter->listBytes.begin(); it != iter->listBytes.end(); ++it) {
                                                    buf << BYTE(*it);
                                                }

                                                //reading the item information directly through a packet by sector1337 (no need for inv slot anymore)
                                                CSOItem *tempItemInfo = new CSOItem();
                                                tempItemInfo->ReadFromPacket(&buf, 1);
                                                tempItemInfo->SetEnabled(true);

                                                if (tempItemInfo) {
                                                    g_pCGInterface->m_helperWindow->ShowGWnd(true);
                                                    itemLinkSlot->ItemInfo = tempItemInfo;

                                                    itemLinkSlot->SetType(70);
                                                    itemLinkSlot->MoveGWnd(g_Controler->m_CursorPos.x + 25, g_Controler->m_CursorPos.y);
                                                    itemLinkSlot->sub_686DB0();
                                                }
                                            }
                                        }
                                        break;
                                    }
                                }
                            }
                        }
                    }
                }




            }
            if(this->GetParentControl()->IsSame(GFX_RUNTIME_CLASS(CIFSystemMessage)))
            {
                CIFSystemMessage *pParent = (CIFSystemMessage*)GetParentControl();
                if(pParent != NULL)
                {
                    if(this->UniqueID() == 19)
                    {
                        CIFSlotWithHelp *itemLinkSlot = g_pCGInterface->GetGuiFromList<CIFSlotWithHelp>(13402);

                        //initialize the slot
                        CIFListCtrl * testlist = pParent->GetGuiFromList<CIFTextBox>(19);
                        CIFListCtrl::SLineOfText *line = testlist->GetTextByIndex(a1);
                        if(line)
                        {
                            if (g_pCGInterface->m_helperWindow && !g_pCGInterface->m_helperWindow->IsVisible()) {
                                if (a1 != 0) {
                                    for (std::list<AlchemyLogStr>::iterator iter = g_global->AlchemyLog.begin(); iter != g_global->AlchemyLog.end(); iter++) {
                                        if (iter->index == a1) {
                                            if (a2 == 1 && iter->Message.find(L">") != std::wstring::npos && iter->Message.find(L"<") != std::wstring::npos) {
                                                //checking if the item already on our list we will show the information , otherwise we will request it
                                                if (iter->listBytes.size() > 0) {
                                                    CMsgStreamBuffer buf(0xB034);
                                                    for (std::list<BYTE>::iterator it = iter->listBytes.begin(); it != iter->listBytes.end(); ++it) {
                                                        buf << BYTE(*it);
                                                    }

                                                    //reading the item information directly through a packet by sector1337 (no need for inv slot anymore)
                                                    CSOItem *tempItemInfo = new CSOItem();
                                                    tempItemInfo->ReadFromPacket(&buf, 1);
                                                    tempItemInfo->SetEnabled(true);

                                                    if (tempItemInfo) {
                                                        g_pCGInterface->m_helperWindow->ShowGWnd(true);
                                                        itemLinkSlot->ItemInfo = tempItemInfo;

                                                        itemLinkSlot->SetType(70);
                                                        itemLinkSlot->MoveGWnd(g_Controler->m_CursorPos.x - 250, g_Controler->m_CursorPos.y);
                                                        itemLinkSlot->sub_686DB0();
                                                    }
                                                }
                                            }
                                            break;
                                        }
                                    }
                                }
                            }
                        }

                        if (!itemLinkSlot) {
                            RECT rect = {0, 0, 0, 0};
                            itemLinkSlot = (CIFSlotWithHelp *) g_pCGInterface->CreateInstance(g_pCGInterface, GFX_RUNTIME_CLASS(CIFSlotWithHelp), rect, 13402, 0);
                            itemLinkSlot->ShowGWnd(false);
                        }
                        if (a2 == 0 && g_pCGInterface->m_helperWindow) {
                            itemLinkSlot->ShowGWnd(false);
                            itemLinkSlot->ItemInfo = NULL;
                            g_pCGInterface->m_helperWindow->Reset();
                            g_pCGInterface->m_helperWindow->ShowGWnd(false);
                            g_pCGInterface->m_helperWindow->m_ownerWindow = NULL;
                            return reinterpret_cast<int(__thiscall *)(CIFTextBox *, int, int)>(0x63A100)(this, a1, a2);
                        }

                    }
                }
            }
        }
    }

    return reinterpret_cast<int(__thiscall *)(CIFTextBox *, int, int)>(0x63A100)(this, a1, a2);
}
