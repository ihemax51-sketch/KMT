#include "GlobalItemLinking.h"
#include "GInterface.h"
#include "IFWholeChat.h"
#include <ICPlayer.h>
#include <algorithm>
#include <GFXMainFrame/Controler.h>
#include <IFChatViewer.h>
#include <PSTitle.h>
#include <SecondPW/IFSecondaryPassword.h>
#include "MemoryHelper.h"

GlobalItemLinking *g_global;
CIFEdit *g_CIFEdit;
// CIFEdit::GetText(void) .text 00653350 00000017   R . . . . . .

const DWORD cdwOnCIFEditWndProcHandleAddr = 0x00637A50;
const DWORD cdwOnCIFEditWndProcHandleJmpback = 0x00637A55;

const DWORD callOrigAddr = 0x00636CD0;
__declspec(naked) void ASM_OnCIFEditWndProcHandle() {
    __asm
    {
    pushad;
    pushfd;
    PUSH EAX;
    CALL GlobalItemLinking::FUN_00636cd0;
    popfd;
    popad;

    call callOrigAddr;
    jmp cdwOnCIFEditWndProcHandleJmpback;
    }
}

void GlobalItemLinking::FUN_00636cd0(CIFEdit *pCIFEdit) {
    if (g_CIFEdit)
        g_CIFEdit = NULL;
    if (g_pCGInterface) {
        CIFWholeChat *pWholeChat = reinterpret_cast<CIFWholeChat *>(g_pCGInterface->m_IRM.GetResObj(56, 1));
        if (pWholeChat) {
            g_CIFEdit = pWholeChat->m_pEdit;
        } else {
            g_global->m_LinkedGlobalItemEnd = 0;
            g_global->m_LinkedGlobalItemBegin = 0;
            g_global->m_CurrentLinkedItemName = std::wstring();
        }
    }
    if (pCIFEdit && !g_global->m_CurrentLinkedItemName.empty()) {
        if (g_CIFEdit != NULL && g_CIFEdit == pCIFEdit)// if its equals to current global window
        {
            short backSpaceState = GetKeyState(VK_BACK);
            short deleteState = GetKeyState(VK_DELETE);
            short leftArrowKeyState = GetKeyState(VK_LEFT);
            short rightArrowKeyState = GetKeyState(VK_RIGHT);

            int CurrIndex = pCIFEdit->GetCurrentIndex();
            std::wstring CurrText = std::wstring(pCIFEdit->GetText());
            if (!CurrText.empty()) {
                int begin = CurrText.find(L"͟<");
                if (begin != std::wstring::npos)
                    g_global->m_LinkedGlobalItemBegin = begin;
                int end = CurrText.find(L"͟>");
                if (end != std::wstring::npos)
                    g_global->m_LinkedGlobalItemEnd = end + 2;


                if (pCIFEdit->GetAreaSize() == 100 && g_global->m_LinkedGlobalItemEnd != 0)
                    pCIFEdit->SetTextAreaSize(100 + g_global->m_CurrentLinkedItemName.length());
                if (pCIFEdit->GetAreaSize() != 100 && g_global->m_LinkedGlobalItemEnd == 0)
                    pCIFEdit->SetTextAreaSize(100);
            }
            //if left
            if ((leftArrowKeyState == -127 || leftArrowKeyState == -128) && CurrIndex - 1 <= g_global->m_LinkedGlobalItemEnd && CurrIndex >= g_global->m_LinkedGlobalItemBegin) {
                //printf("left called %d\n", leftArrowKeyState);
                pCIFEdit->SetCurrentIndex(g_global->m_LinkedGlobalItemBegin);
                return;
            }
            //if right
            if ((rightArrowKeyState == -127 || rightArrowKeyState == -128) && CurrIndex + 1 >= g_global->m_LinkedGlobalItemBegin && CurrIndex <= g_global->m_LinkedGlobalItemEnd) {
                //printf("right called %d\n", rightArrowKeyState);
                pCIFEdit->SetCurrentIndex(g_global->m_LinkedGlobalItemEnd);
                return;
            }

            //printf("ChatIndex = %d - Size = %d\n", pCIFEdit->GetCurrentIndex(), pCIFEdit->GetAreaSize());


            if ((backSpaceState == -127 || backSpaceState == -128) && (CurrIndex >= g_global->m_LinkedGlobalItemBegin && CurrIndex - 1 <= g_global->m_LinkedGlobalItemEnd)) {
                std::wstring new_str = CurrText.c_str();
                int findedstring = CurrText.find(g_global->m_CurrentLinkedItemName.c_str());
                if (findedstring != std::wstring::npos) {
                    CurrText.erase(g_global->m_LinkedGlobalItemBegin, g_global->m_CurrentLinkedItemName.size());

                    pCIFEdit->SetText(CurrText.c_str());
                    if (CurrText.length() > 0)
                        pCIFEdit->SetCurrentIndex(g_global->m_LinkedGlobalItemBegin);
                    else
                        pCIFEdit->SetCurrentIndex(0);
                    g_global->m_LinkedGlobalItemEnd = 0;
                    g_global->m_LinkedGlobalItemBegin = 0;
                    g_global->m_CurrentLinkedItemName = std::wstring();
                    g_global->m_LinkedGlobalSlot = 0;
                    return;
                }
            }
            if ((deleteState == -127 || deleteState == -128) && (CurrIndex + 2 >= g_global->m_LinkedGlobalItemBegin && CurrIndex <= g_global->m_LinkedGlobalItemEnd)) {
                std::wstring new_str = CurrText.c_str();
                int findedstring = CurrText.find(g_global->m_CurrentLinkedItemName.c_str());
                if (findedstring != std::wstring::npos) {
                    CurrText.erase(g_global->m_LinkedGlobalItemBegin, g_global->m_CurrentLinkedItemName.size());

                    pCIFEdit->SetText(CurrText.c_str());
                    pCIFEdit->SetCurrentIndex(CurrText.length());
                    g_global->m_LinkedGlobalItemEnd = 0;
                    g_global->m_LinkedGlobalItemBegin = 0;
                    g_global->m_CurrentLinkedItemName = std::wstring();
                    g_global->m_LinkedGlobalSlot = 0;
                    return;
                }
            }
        }
    }
    if(pCIFEdit->GetParentControl()->IsSame(GFX_RUNTIME_CLASS(CIFSecondaryPassword)))
    {
        CIFSecondaryPassword* ps = (CIFSecondaryPassword*) pCIFEdit->GetParentControl();
        if(ps->myFirstPasswordBox)
        {
            return;
        }
    }
}



void GlobalItemLinking::Initialize() {
    m_CurrentLinkedItemName = std::wstring();
    m_LinkedGlobalItemEnd = 0;
    m_LinkedGlobalItemBegin = 0;

    g_MemoryHelper->Detour(false, reinterpret_cast<void *>(cdwOnCIFEditWndProcHandleAddr), ASM_OnCIFEditWndProcHandle);
}