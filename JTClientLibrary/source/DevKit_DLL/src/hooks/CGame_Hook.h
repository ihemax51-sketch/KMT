#pragma once

#include <Game.h>

class CGame_Hook : public CGame {
public:
    void LoadGameOption();

    void InitGameAssets_Impl();

    static void OnActionEvent(int actionWndID);
    // VC8 rejects __declspec(naked) on member-function definitions. The
    // actual naked entrypoint is declared below as a file-level thunk.
    static void Naked_OnActionEvent();
};

void CGame_Hook_Naked_OnActionEvent();

bool InstallInitGameAssetsBootstrapGate();
