#include "NavigationTool.h"

#include <imgui/imgui.h>
#include "ICPlayer.h"

#include "../Pathfinder.h"
#include "NavMesh/IRegionManager.h"
#include "GFX3DFunction/DrawingHelpers.h"
#include "../Node.h"


void NavigationTool::MenuItem() {
    ImGui::MenuItem("Pathfinder", 0, &bShow);
}

void NavigationTool::Render() {
    {
        if (!bShow) return;
    }
    if (!ImGui::Begin("Navigation Tool", &bShow))
    {
        ImGui::End();
        return;
    }
    if(!g_pMyPlayerObj)
        return;
    // From location
    static LocationInfo l1 = {0, 0, 0, D3DXVECTOR3(0, 0, 0)};

    ImGui::Text("From");
    ImGui::Text("Cell: %p", l1.field_0);
    ImGui::Text("Under Foot: %p", l1.field_1);
    ImGui::InputScalar("Region (From)", ImGuiDataType_U32, &l1.region, 0, 0, "%04X", ImGuiInputTextFlags_CharsHexadecimal);
    ImGui::DragFloat3("Position (From)", (float*)&l1.pos);

    l1.region.raw = g_pMyPlayerObj->GetRegion().r;
    l1.pos = g_pMyPlayerObj->GetLocation();
    l1.field_0 = g_pMyPlayerObj->GetNavCell();
    l1.field_1 = g_pMyPlayerObj->m_object_under_foot;

    ImGui::Separator();


    // To location
    static LocationInfo l2 = {0, 0, 0, D3DXVECTOR3(0, 0, 0)};

    ImGui::Text("To");
    ImGui::Text("Cell: %08X", l2.field_0);
    ImGui::Text("Under Foot: %08X", l2.field_1);
    ImGui::InputScalar("Region (To)", ImGuiDataType_U32, &l2.region, 0, 0, "%04X", ImGuiInputTextFlags_CharsHexadecimal);
    ImGui::DragFloat3("Position (To)", (float*)&l2.pos);
    if (ImGui::Button("Get \"To\" from current"))
    {
        l2.region.raw = g_pMyPlayerObj->GetRegion().r;
        l2.pos = g_pMyPlayerObj->GetLocation();
    }


    ImGui::Separator();

    // Executor
    static int return_value = 0;

    ImGui::Text("Return value was: %d", return_value);

    if (ImGui::Button("Check"))
    {
        return_value = g_CRegionManagerBody.SomethingWithMapCollision_MAYBE(1, 1, &l1, &l2, 0, g_pMyPlayerObj);
    }

    static std::list<CNode> path;
    static CPathfinder *pathfinder = 0;

    if (ImGui::Button("Navigate"))
    {
        delete pathfinder;
        pathfinder = new CPathfinder(l1, l2);
    }

    static int steps = 1;
    ImGui::InputInt("Steps", &steps);

    if (ImGui::Button("Step"))
    {
        if (pathfinder && !pathfinder->completed)
            for (int i = 0; i < steps; i++)
                if (pathfinder->step())
                    break;
    }

    if (pathfinder)
    {
        ImGui::Separator();
        ImGui::Text("Stats:");
        ImGui::Text("Open: %d nodes", pathfinder->open_set.size());
        ImGui::Text("Closed: %d nodes", pathfinder->closed_set.size());
        ImGui::Text("Rejected: %d nodes", pathfinder->rejected_set.size());
        ImGui::Text("Mismatch: %d nodes", pathfinder->mismatch_set.size());
    }

    if (pathfinder)
        pathfinder->render();

    // Draw markers
    Line3D(l1.pos, l2.pos, D3DCOLOR_RGBA(255, 0, 0, 255), 1.0);

    ImGui::End();
}
