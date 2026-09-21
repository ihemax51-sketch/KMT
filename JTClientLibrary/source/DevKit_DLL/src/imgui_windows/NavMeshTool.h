#pragma once

#include <NavMesh/RTNavMeshTerrain.h>

#include "BasicWindow.h"
namespace Devkit {
    class NavMeshTool : public BasicWindow {
    public:
        NavMeshTool();

        void Render();

    private:
        void RenderNavCells(const CRTNavMeshTerrain *pNavmesh) const;

        void RenderNavEdgeInternal(const CRTNavMeshTerrain *pNavmesh) const;

        void RenderNavEdgeGlobal(const CRTNavMeshTerrain *pNavmesh) const;

        void RenderObjectOrigin(const SNavMeshInst *pInst, bool bIsFirst) const;

        void RenderObjectCells(const SNavMeshInst *pInst) const;

        void RenderObjectInternalEdges(const SNavMeshInst *pInst) const;

        void RenderObjectGlobalEdges(const SNavMeshInst *pInst) const;

        void RenderObjectGrid(const SNavMeshInst *pInst) const;

    private:
        bool bFreeze;

        bool bCells;
        bool bEdgeInternal;
        bool bEdgeGlobal;

        bool bObjectOrigin;
        bool bObjectCells;
        bool bObjectInternalEdges;
        bool bObjectGlobalEdges;
        bool bObjectGrid;

        int step;

        CRTNavMeshTerrain *m_pNavmesh;
    };
}// namespace Devkit