#include "ErrorMessageTool.h"

#include <cstdio>
#include <imgui/imgui.h>
#include <Game.h>
#include <PSilkroad.h>
#include <BSLib/multibyte.h>

using namespace Devkit;

ErrorMessageTool::ErrorMessageTool() : BasicWindow("Error Message Tool", false) {}

void ErrorMessageTool::Render() {

    if (!m_bShow) return;

    if (!ImGui::Begin(getName().c_str(), &m_bShow)) {
        ImGui::End();
        return;
    }

    static char buffer[128] = {0};
    ImGui::InputText("Message", buffer, IM_ARRAYSIZE(buffer));

    if (ImGui::Button("Show")) {
        std::wstring str = acp_decode(buffer, strlen(buffer));

        CProcess *process = theApp.GetCurrentProcess();
        if (process->IsKindOf(GFX_RUNTIME_CLASS(CPSilkroad))) {
            reinterpret_cast<CPSilkroad *>(process)->ShowMessage(str.c_str(), D3DCOLOR_RGBA(255, 0, 0, 0));
        }
    }

    ImGui::End();
}
