//
// Created by Admin on 1/8/2022.
//

#include "CircleDrawTool.h"
#include <GInterface.h>
#include <GlobalDataManager.h>
#include <imgui/imgui.h>
#include <CustomData/CustomDataManager.h>
#include <ICPlayer.h>

void CircleDrawTool::Render() {
    if(!g_pMyPlayerObj )
        return;
    if(!g_pCGInterface->GetCNIFWorldMap())
        return;

    if(!g_pCGInterface->GetCNIFWorldMap()->IsVisible())
        return;
    if(!m_CustomDataManager->CircleGreenShow)
        return;
    if (m_CustomDataManager->DimenSionalRegion.find(g_pMyPlayerObj->GetRegion().r) != m_CustomDataManager->DimenSionalRegion.end())
    {
        ImGui::SetNextWindowSize(ImVec2(640,384));
        if (!ImGui::Begin("CircleDrawToolxx",0, ImGuiWindowFlags_NoBackground| ImGuiWindowFlags_NoBringToFrontOnFocus| ImGuiWindowFlags_NoFocusOnAppearing| ImGuiWindowFlags_NoMouseInputs| ImGuiWindowFlags_NoSavedSettings|ImGuiWindowFlags_NoMove| ImGuiWindowFlags_NoTitleBar |ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse | ImGuiWindowFlags_NoResize))
        {
            // Early out if  the window is collapsed, as an optimization.
            ImGui::End();
            return;
        }

        ImGui::SetWindowPos("CircleDrawToolxx",ImVec2(g_pCGInterface->GetCNIFWorldMap()->GetPos().x + 6,g_pCGInterface->GetCNIFWorldMap()->GetPos().y + 34));
        ImGui::DrawCircle( *((float *)((int)g_pCGInterface->GetCNIFWorldMap() + 0x95cc)) + m_CustomDataManager->cercleX,*((float *)((int)g_pCGInterface->GetCNIFWorldMap() + 0x95d0)) + m_CustomDataManager->cercleY,m_CustomDataManager->cercleRadius);

    }


    ImGui::End();
}
