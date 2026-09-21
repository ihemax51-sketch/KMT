#pragma once

#include "IFButton.h"
#include "IFNormalTile.h"
#include "IFStatic.h"

class CPSTitle;

void QuickLogin_CreatePanel(CPSTitle* title);
void QuickLogin_ResetPanel();
void QuickLogin_SyncPanel(CPSTitle* title);
void QuickLogin_EnsurePresentation(CPSTitle* title);
void QuickLogin_SetEnabled(CPSTitle* title, bool enabled);
void QuickLogin_HandleCreateTokenResult(CPSTitle* title, bool success, const std::n_string& username, const std::n_string& token, const wchar_t* message);
void QuickLogin_HandleQuickLoginResult(CPSTitle* title, bool success, const wchar_t* message);
void QuickLogin_HandleRevokeTokenResult(CPSTitle* title, bool success, const wchar_t* message);
bool QuickLogin_GetPendingCharacterName(std::n_string& characterName);
void QuickLogin_ClearPendingCharacterName();

class CIFQuickLoginButton : public CIFButton {
    GFX_DECLARE_DYNCREATE(CIFQuickLoginButton)

public:
    CIFQuickLoginButton();
    ~CIFQuickLoginButton();

    int OnMouseLeftUp(int a1, int x, int y) override;
};
