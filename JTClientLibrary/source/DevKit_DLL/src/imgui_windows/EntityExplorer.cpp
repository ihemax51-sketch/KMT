#include "EntityExplorer.h"

#include <imgui/imgui.h>
#include <EntityManagerClient.h>
#include <GFX3DFunction/GFXVideo3d.h>
#include <Game.h>
#include <GFX3DFunction/DrawingHelpers.h>
#include <GInterface.h>
#include <ICPlayer.h>

using namespace Devkit;

EntityExplorer::EntityExplorer() : BasicWindow("Entity Explorer", false) {}
D3DXVECTOR3 ConvertToGlobalCoordinates(const uregion& region, const D3DXVECTOR3& localCoords) {
    float globalX, globalY;

    if (region.single.x == 0) {  // Assuming dungeon
        globalX = localCoords.x / 10.0f;
        globalY = localCoords.y / 10.0f;
    } else {  // World map
        globalX = (region.single.x - 135) * 192 + localCoords.x / 10.0f;
        globalY = (region.single.y - 92) * 192 + localCoords.y / 10.0f;
    }

    return D3DXVECTOR3(globalX, globalY, localCoords.z);  // Assuming Z coordinate remains unchanged
}

float CalculateDistance(const D3DXVECTOR3& loc1, const D3DXVECTOR3& loc2) {
    D3DXVECTOR3 delta = loc1 - loc2;
    return D3DXVec3Length(&delta);
}

void EntityExplorer::Render() {

    if (!bShow) return;

    if (!ImGui::Begin(getName().c_str(), &bShow)) {
        ImGui::End();
        return;
    }

    EntityListColumnHeader headers[] = {
            {"ID",       40},
            {"Address",  70},
            {"Type",     120},
            {"Position", 500},
            {"Distance", 50}
    };

    ImGui::Columns(IM_ARRAYSIZE(headers), "TableTextureColumns", true);
    ImGui::Separator();

    float offset = 0.0f;
    for (int i = 0; i < IM_ARRAYSIZE(headers); i++) {
        EntityListColumnHeader *header = &headers[i];
        ImGui::Text(header->label);
        ImGui::SetColumnOffset(-1, offset);
        offset += header->size;
        ImGui::NextColumn();
    }

    ImGui::Separator();

    static int selected = -1;

    for (std::map<int, CIObject *>::iterator it = g_pGfxEttManager->entities.begin();
         it != g_pGfxEttManager->entities.end(); ++it) {
        char label[32];
        sprintf(label, "%d", it->first);

        if (ImGui::Selectable(label, selected == it->first, ImGuiSelectableFlags_SpanAllColumns))
            selected = it->first;

        ImGui::NextColumn();

        ImGui::Text("%p", it->second);
        ImGui::NextColumn();
        ImGui::Text("%s", it->second->GetRuntimeClass()->m_lpszClassName);
        ImGui::NextColumn();

        uregion region = it->second->GetRegion();
        D3DVECTOR location = it->second->GetLocation();

        ImGui::Text("(%d,%d) (%.2f, %.2f, %.2f) Region %d",
                    region.single.x,
                    region.single.y,
                    location.x,
                    location.y,
                    location.z, region.r
        );
        ImGui::NextColumn();

        D3DXVECTOR3 locationtarget = it->second->GetLocation();
        D3DXVECTOR3 locationmine = g_pMyPlayerObj->GetLocation();

// Convert local coordinates to global coordinates
        D3DXVECTOR3 globalTarget = ConvertToGlobalCoordinates(it->second->GetRegion(), locationtarget);
        D3DXVECTOR3 globalMine = ConvertToGlobalCoordinates(g_pMyPlayerObj->GetRegion(), locationmine);

// Calculate the distance
        float distance = CalculateDistance(globalTarget, globalMine);

        ImGui::Text("%.02f m", distance / 10);



        ImGui::NextColumn();


        ImGui::Separator();

    }

    ImGui::Columns(1);

    ImGui::Text("Number of Entites: %d", g_pGfxEttManager->entities.size());

    char label[32];
    sprintf(label, "Move to Entity #%d", selected);

    static bool b_checked = false;
    ImGui::Checkbox("Highlight selected", &b_checked);

    if (b_checked && selected != -1) {
        // ESP ON

        std::map<int, CIObject *>::iterator elem = g_pGfxEttManager->entities.find(selected);
        if (elem != g_pGfxEttManager->entities.end()) {

            D3DVECTOR d2dpos, d2dpos_up, d2dpos_own;
            D3DVECTOR d3dpos = elem->second->GetLocation();
            D3DVECTOR d3dpos_up = d3dpos;

            d3dpos_up.y += 18.0;

            if (CGFXVideo3d::get()->Project(d3dpos, d2dpos) > 0) {
                DrawRect((int) d2dpos.x - 5, (int) d2dpos.y - 5, 10, 10);

                if (CGFXVideo3d::get()->Project(theApp.camera.origin, d2dpos_own) > 0) {
                    DXDrawLine((int) d2dpos_own.x, (int) d2dpos_own.y, (int) d2dpos.x, (int) d2dpos.y,
                               D3DCOLOR_ARGB(0, 255, 255, 0), 1.0);
                }
            }

            if (CGFXVideo3d::get()->Project(d3dpos_up, d2dpos_up) > 0) {
                DXDrawLine((int) d2dpos.x, (int) d2dpos.y, (int) d2dpos_up.x, (int) d2dpos_up.y,
                           D3DCOLOR_ARGB(0, 0, 255, 0), 1.0);
            }
        }
    }

    if (ImGui::Button(label)) {
        std::map<int, CIObject *>::iterator elem = g_pGfxEttManager->entities.find(selected);
        if (elem != g_pGfxEttManager->entities.end()) {
            g_pCGInterface->m_Nav.MoveTo(elem->second->GetRegion(), elem->second->GetLocation());
        }
    }

    ImGui::End();
}

void EntityExplorer::MenuItem() {
    ImGui::MenuItem("Entity Explorer", 0, &bShow);
}
