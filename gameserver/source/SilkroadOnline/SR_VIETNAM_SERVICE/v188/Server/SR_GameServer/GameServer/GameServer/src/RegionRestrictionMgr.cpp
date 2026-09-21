#include <Objects/GObjPC.h>
#include <memory/MemoryUtility.h>
#include <SqlConnection/sqlCon.h>
#include "RegionRestrictionMgr.h"
#include "RegionRestrictionDBSet.h"


__REGION_RESTRICTION_MGR_HIDE_NAME_REGION_LST CRegionRestrictionMgr::s_lstHideNameRegions;

LPCSTR lpszFakeCharName = "KMTGuard";

const DWORD GROUP_SPAWN_PUT_NAME_HOOK_OFFSET = 0x004E5B6E;
const DWORD GROUP_SPAWN_PUT_NAME_HOOK_JUMPBACK = (GROUP_SPAWN_PUT_NAME_HOOK_OFFSET + 5);
const DWORD GROUP_SPAWN_PUT_GUILD_NAME_HOOK_OFFSET = 0x004E5D20;
const DWORD GROUP_SPAWN_PUT_GUILD_NAME_HOOK_JUMPBACK = (GROUP_SPAWN_PUT_GUILD_NAME_HOOK_OFFSET + 6);


const DWORD TITLE_HIDE_HOOK_OFFSET = 0x004E10CD;
const DWORD TITLE_HIDE_HOOK_JUMPBACK = 0x004E10D3;

#define GROUP_SPAWN_WRITE_GUILD_INFO_FUNC_ADDR	0x004EA2F0

typedef void(__thiscall* fnWriteGuildInfo)(CGObjPC* pPC);

fnWriteGuildInfo pfnOrigWriteGuildInfo = NULL;


bool __declspec(noinline) __stdcall IsHideNameRegion(CGObjPC* pPC)
{
    if (pPC == NULL)
        return false;

    if (pPC->m_pObjDataInstance == NULL)
        return false;

    SPosInfo posInfo;
    pPC->GetPosInfo(posInfo);

    return CRegionRestrictionMgr::IsHideNameRegion(posInfo.wRegionID);
}


__declspec(naked) void MyGroupSpawnPutCharName()
{
    __asm
    {
        REMODEL_SAVE_REGISTERS_BEFORE_STDCALL;

        //pPC
        push ebp;
        call IsHideNameRegion;
        cmp eax, TRUE;

        REMODEL_RESTORE_REGISTERS_AFTER_STDCALL;

        je PUT_CUSTOM_CHAR_NAME;
        jmp JUMPBACK;

    PUT_CUSTOM_CHAR_NAME:
        mov eax, lpszFakeCharName;

    JUMPBACK:
        push eax;
        mov dword ptr ss : [esp + 0x18] , eax;
        jmp GROUP_SPAWN_PUT_NAME_HOOK_JUMPBACK;
    }
}

__declspec(naked) void MyGroupSpawnPutGuildName()
{
    __asm
    {
        REMODEL_SAVE_REGISTERS_BEFORE_STDCALL;

        //pPC
        push ebp;
        call IsHideNameRegion;
        cmp eax, TRUE;

        REMODEL_RESTORE_REGISTERS_AFTER_STDCALL;

        je PUT_EMPTY_GUILD_NAME;
        jmp JUMPBACK;

    PUT_EMPTY_GUILD_NAME:
        mov eax, 0;

    JUMPBACK:
        //Originally movzx on ax.
        mov edi, eax;
        push edi;
        mov esi, ebx;
        jmp GROUP_SPAWN_PUT_GUILD_NAME_HOOK_JUMPBACK;
    }
}


__declspec(naked) void MyGetSetRefObjHwanLevel()
{
    __asm
    {
    REMODEL_SAVE_REGISTERS_BEFORE_STDCALL;

    //pPC
    push ebx;
    call IsHideNameRegion;
    cmp eax, TRUE;

    REMODEL_RESTORE_REGISTERS_AFTER_STDCALL;

    je SET_ZERO_HWAN_LEVEL;
    jmp JUMPBACK;

    SET_ZERO_HWAN_LEVEL:
    mov byte ptr ds : [edx + 0xF2] , 0;
    JUMPBACK:
    mov al, byte ptr ds : [edx + 0xF2] ;
    jmp TITLE_HIDE_HOOK_JUMPBACK;
    }
    }
void __fastcall MyGroupSpawnWriteGuildInfo(CGObjPC* pPC, LPVOID /* dummy edx */)
{
    if (!IsHideNameRegion(pPC))
        pfnOrigWriteGuildInfo(pPC);
}


bool CRegionRestrictionMgr::Initialize()
{

    pfnOrigWriteGuildInfo = reinterpret_cast<fnWriteGuildInfo>(GROUP_SPAWN_WRITE_GUILD_INFO_FUNC_ADDR);


    MEMUTIL_SETUP_HOOK(
            E_MEMUTUL_HOOK_TYPE::LongJump,
            GROUP_SPAWN_PUT_NAME_HOOK_OFFSET, (uintptr_t)MyGroupSpawnPutCharName
    );

    MEMUTIL_NOP(GROUP_SPAWN_PUT_GUILD_NAME_HOOK_OFFSET, 6);

    MEMUTIL_SETUP_HOOK(
            E_MEMUTUL_HOOK_TYPE::LongJump,
            GROUP_SPAWN_PUT_GUILD_NAME_HOOK_OFFSET, (uintptr_t)MyGroupSpawnPutGuildName
    );


    MEMUTIL_NOP(TITLE_HIDE_HOOK_OFFSET, 6);
    MEMUTIL_SETUP_HOOK(
            E_MEMUTUL_HOOK_TYPE::LongJump,
            TITLE_HIDE_HOOK_OFFSET, (uintptr_t)MyGetSetRefObjHwanLevel
    );

    DetourTransactionBegin();
    DetourAttach(&(PVOID&)pfnOrigWriteGuildInfo, MyGroupSpawnWriteGuildInfo);
    DetourTransactionCommit();

    return LoadHideNameRegions();
}


bool CRegionRestrictionMgr::LoadHideNameRegions()
{
    CRegionRestrictionDBSet* pDbSet = CSqlCon::GetRegionRestrictionDbSet();

    __DBSET_HIDE_NAME_REGION_DB_RECORD_LST lstRecords;

    if (!pDbSet->GetHideNameRegionRecords(lstRecords))
        return false;

    s_lstHideNameRegions.clear();
    s_lstHideNameRegions.reserve(lstRecords.size());

    __DBSET_HIDE_NAME_REGION_DB_RECORD_LST_IT itCur = lstRecords.begin();
    __DBSET_HIDE_NAME_REGION_DB_RECORD_LST_IT itEnd = lstRecords.end();

    for (; itCur != itEnd; itCur++)
    {
        SHideNameRegionDbRecord record = (SHideNameRegionDbRecord)(*itCur);
        AddHideNameRegion(record.wRegionID);
        printf("hide name region id %d\n", record.wRegionID);
    }

    std::sort(s_lstHideNameRegions.begin(), s_lstHideNameRegions.end());
    s_lstHideNameRegions.erase(
        std::unique(s_lstHideNameRegions.begin(), s_lstHideNameRegions.end()),
        s_lstHideNameRegions.end());

    return true;
}

void CRegionRestrictionMgr::AddHideNameRegion(WORD wRegionID)
{
    s_lstHideNameRegions.push_back(wRegionID);
}

bool CRegionRestrictionMgr::IsHideNameRegion(WORD wRegionID)
{
    return std::binary_search(
        s_lstHideNameRegions.begin(),
        s_lstHideNameRegions.end(),
        wRegionID);
}
