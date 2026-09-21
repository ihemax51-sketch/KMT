#pragma once



#include "STL/SingletonT.h"
#include "GObjPC.h" // for CGObjPC 
#define __CGOBJ_AGGRO_LIST_MAP                       std::map<DWORD, SAggroMapSecondPairItem>
#define __CGOBJ_AGGRO_LIST_MAP_IT                    __CGOBJ_AGGRO_LIST_MAP::iterator
#define __CGOBJ_AGGRO_LIST_MAP_SECOND_ELEM_PAIR      std::pair<DWORD, SAggroMapSecondPairItem>


class CDamageMeter
{
private:
    typedef void*(__thiscall* FN_CGOBJNPC_AGGRO_MAP)(IGObj*, int a2);

    static FN_CGOBJNPC_AGGRO_MAP s_pfnCGObjNpcAggroMap;

    static void* __fastcall MyCGObjNPC_HandleAggroMap(IGObj* pObj, void* /* dummy edx */, int a2);
public:

    static bool Initialize();
    static void Shutdown();
    static void ForgetMob(DWORD mobGameId);
};
