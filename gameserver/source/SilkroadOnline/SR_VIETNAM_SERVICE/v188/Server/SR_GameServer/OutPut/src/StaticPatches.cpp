#include "StaticPatches.h"

#include <Windows.h>
#include <SettingMgr/NewSettings.h>
#include <BSObj/BSObj.h>
#include <memory/MemoryUtility.h>
#include <memory/hook.h>
#include <KMTGuardCustom/GameServerRuntimeSafety.h>

#define GREEN_BOOK_NOP_OFFSET_1 0x004142E2
#define GREEN_BOOK_NOP_OFFSET_2 0x0041474D
#define QUEST_RAISE_EVENT_LOG_BLOCK_OFFSET 0x005805C8
#define QUEST_RAISE_EVENT_LOG_CALL_OFFSET 0x005805D3
#define GM_UNIQUE_KILL_NOTICE_BRANCH_OFFSET 0x004C1D23

namespace
{
    bool s_greenBookPatchApplied = false;
    bool s_questRaiseEventLogPatchApplied = false;
    bool s_gmUniqueKillNoticePatchApplied = false;
    const BYTE kGreenBookOriginal1[] =
        { 0x81, 0xE9, 0x22, 0x31, 0x00, 0x00, 0x74, 0x51 };
    const BYTE kGreenBookOriginal2[] =
        { 0xE8, 0xEE, 0x1E, 0x52, 0x00 };
    const BYTE kQuestRaiseEventLogBlockOriginal[] =
        {
            0x56,
            0x68, 0xC4, 0xD8, 0xAF, 0x00,
            0x68, 0x00, 0x00, 0x00, 0x01,
            0xE8, 0x68, 0x60, 0x3B, 0x00,
            0x83, 0xC4, 0x0C
        };
    const BYTE kQuestRaiseEventLogCallOriginal[] =
        { 0xE8, 0x68, 0x60, 0x3B, 0x00 };
    const BYTE kQuestRaiseEventLogCallSuppressed[] =
        { 0x90, 0x90, 0x90, 0x90, 0x90 };
    const BYTE kGmUniqueKillNoticeBranchOriginal[] =
        { 0x0F, 0x84, 0xB7, 0x00, 0x00, 0x00 };
    const BYTE kGmUniqueKillNoticeBranchSuppressed[] =
        { 0x90, 0x90, 0x90, 0x90, 0x90, 0x90 };
}

bool CStaticPatches::Apply()
{
    GameCfgStruct* settings = CNewSettings::GetGameCfg();
    if (settings == NULL)
    {
        BS_INFO("STATIC_PATCHES settings are not initialized");
        return false;
    }

    // Keep quest processing intact and suppress only the hot-path diagnostic
    // call that repeatedly reports Quest:RaiseEvent failures.
    if (!GameServerRuntimeSafety::MatchesBytes(
            QUEST_RAISE_EVENT_LOG_BLOCK_OFFSET,
            kQuestRaiseEventLogBlockOriginal,
            sizeof(kQuestRaiseEventLogBlockOriginal)))
    {
        BS_INFO("QUEST_RAISE_EVENT_LOG_SUPPRESSION -> FAILED (unsupported GameServer instructions)");
        return false;
    }

    GameServerMemoryPatchTransaction questLogPatch;
    if (!questLogPatch.WriteRaw(
            QUEST_RAISE_EVENT_LOG_CALL_OFFSET,
            kQuestRaiseEventLogCallSuppressed,
            sizeof(kQuestRaiseEventLogCallSuppressed),
            "Quest RaiseEvent log suppression") ||
        !questLogPatch.Commit())
    {
        BS_INFO("QUEST_RAISE_EVENT_LOG_SUPPRESSION failed at 0x%08X", QUEST_RAISE_EVENT_LOG_CALL_OFFSET);
        return false;
    }

    s_questRaiseEventLogPatchApplied = true;
    BS_INFO("QUEST_RAISE_EVENT_LOG_SUPPRESSION -> (True)");

    if (settings->ShowGmUniqueKillNotice)
    {
        if (!GameServerRuntimeSafety::MatchesBytes(
                GM_UNIQUE_KILL_NOTICE_BRANCH_OFFSET,
                kGmUniqueKillNoticeBranchOriginal,
                sizeof(kGmUniqueKillNoticeBranchOriginal)))
        {
            BS_INFO("GM_UNIQUE_KILL_NOTICE -> FAILED (unsupported GameServer instructions)");
            Revert();
            return false;
        }

        GameServerMemoryPatchTransaction gmUniqueNoticePatch;
        if (!gmUniqueNoticePatch.WriteRaw(
                GM_UNIQUE_KILL_NOTICE_BRANCH_OFFSET,
                kGmUniqueKillNoticeBranchSuppressed,
                sizeof(kGmUniqueKillNoticeBranchSuppressed),
                "GM Unique kill notice") ||
            !gmUniqueNoticePatch.Commit())
        {
            BS_INFO("GM_UNIQUE_KILL_NOTICE failed at 0x%08X", GM_UNIQUE_KILL_NOTICE_BRANCH_OFFSET);
            Revert();
            return false;
        }

        s_gmUniqueKillNoticePatchApplied = true;
        BS_INFO("GM_UNIQUE_KILL_NOTICE -> (True)");
    }
    else
    {
        BS_INFO("GM_UNIQUE_KILL_NOTICE -> (False)");
    }

    if (!settings->DisableGreenBook)
    {
        BS_INFO("GREEN_BOOK_DISABLE -> (False)");
        return true;
    }

    if (!GameServerRuntimeSafety::MatchesBytes(
            GREEN_BOOK_NOP_OFFSET_1, kGreenBookOriginal1, sizeof(kGreenBookOriginal1)) ||
        !GameServerRuntimeSafety::MatchesBytes(
            GREEN_BOOK_NOP_OFFSET_2, kGreenBookOriginal2, sizeof(kGreenBookOriginal2)))
    {
        BS_INFO("GREEN_BOOK_DISABLE -> FAILED (unsupported GameServer instructions)");
        Revert();
        return false;
    }

    if (!MEMUTIL_NOP(GREEN_BOOK_NOP_OFFSET_1, sizeof(kGreenBookOriginal1)))
    {
        BS_INFO("GREEN_BOOK_DISABLE first patch failed at 0x%08X", GREEN_BOOK_NOP_OFFSET_1);
        Revert();
        return false;
    }

    if (!MEMUTIL_NOP(GREEN_BOOK_NOP_OFFSET_2, sizeof(kGreenBookOriginal2)))
    {
        CopyBytes(GREEN_BOOK_NOP_OFFSET_1, kGreenBookOriginal1, sizeof(kGreenBookOriginal1));
        BS_INFO("GREEN_BOOK_DISABLE second patch failed at 0x%08X", GREEN_BOOK_NOP_OFFSET_2);
        Revert();
        return false;
    }

    FlushInstructionCache(GetCurrentProcess(), NULL, 0);
    s_greenBookPatchApplied = true;
    BS_INFO("GREEN_BOOK_DISABLE -> (True)");
    return true;
}

void CStaticPatches::Revert()
{
    if (s_greenBookPatchApplied)
    {
        GameServerRuntimeSafety::RestoreBytes(
            GREEN_BOOK_NOP_OFFSET_2, NULL, kGreenBookOriginal2, sizeof(kGreenBookOriginal2), "Green Book patch 2");
        GameServerRuntimeSafety::RestoreBytes(
            GREEN_BOOK_NOP_OFFSET_1, NULL, kGreenBookOriginal1, sizeof(kGreenBookOriginal1), "Green Book patch 1");
        s_greenBookPatchApplied = false;
    }

    if (s_gmUniqueKillNoticePatchApplied)
    {
        GameServerRuntimeSafety::RestoreBytes(
            GM_UNIQUE_KILL_NOTICE_BRANCH_OFFSET,
            kGmUniqueKillNoticeBranchSuppressed,
            kGmUniqueKillNoticeBranchOriginal,
            sizeof(kGmUniqueKillNoticeBranchOriginal),
            "GM Unique kill notice");
        s_gmUniqueKillNoticePatchApplied = false;
    }

    if (s_questRaiseEventLogPatchApplied)
    {
        GameServerRuntimeSafety::RestoreBytes(
            QUEST_RAISE_EVENT_LOG_CALL_OFFSET,
            NULL,
            kQuestRaiseEventLogCallOriginal,
            sizeof(kQuestRaiseEventLogCallOriginal),
            "Quest RaiseEvent log suppression");
        s_questRaiseEventLogPatchApplied = false;
    }
}
