#include "Util.h"

#include "StaticPatches.h"

#include <MainProcess.h>
#include <NetHelper.h>
#include <Objects/CGObjCOS_GoldPet.h>
#include <Objects/GObjMob.h>
#include <Objects/GObjPC.h>
#include <Objects/GObjSiegeStruct.h>
#include <SettingMgr/NewSettings.h>
#include <SqlConnection/sqlCon.h>
#include <GSLog/LogCustoms.h>
#include <KMTGuardCustom/DamageMeter.h>
#include <KMTGuardCustom/DurabilityControl.h>
#include <KMTGuardCustom/GameServerRuntimeSafety.h>
#include <KMTGuardCustom/UniqueSpawnGuard.h>
#include <KMTGuardCustom/GameServerTelemetry.h>
#include <KMTGuardCustom/GObjEvents.h>
#include <KMTGuardCustom/InternalPacketAuth.h>
#include <KMTGuardCustom/ItemRegionTravelGuard.h>
#include <KMTGuardCustom/PartyMonsterControl.h>
#include <KMTGuardCustom/TradeGoldControl.h>

#include <BSObj/BSObj.h>
#include <memory/hook.h>
#include <KmtGuardProductVersion.h>

namespace
{
    struct PointerHook
    {
        DWORD address;
        DWORD expected;
        DWORD replacement;
        const char* name;
    };

    const PointerHook kCorePointerHooks[] =
    {
        { 0x00ADF020, 0x00402C20, static_cast<DWORD>(addr_from_this(&CMainProcess::_OnProcessMessage)), "main message hook" },
        { 0x00ADF028, 0x0094CE60, static_cast<DWORD>(addr_from_this(&CMainProcess::_OnQueueTimer)), "queue timer hook" },
        { 0x00AF59FC, 0x004DE9B0, static_cast<DWORD>(addr_from_this(&CGObjPC::OnDeleteObjectCustom)), "player cleanup hook" },
        { 0x00AF5F0C, 0x004A7540, static_cast<DWORD>(addr_from_this(&CGObjPC::KillLoggerFunction)), "player kill hook" },
        { 0x00AF5FDC, 0x0050EEE0, static_cast<DWORD>(addr_from_this(&CGObjPC::ReaderPacket)), "player packet hook" },
        { 0x00AF6018, 0x004DF9E0, static_cast<DWORD>(addr_from_this(&CGObjPC::CharacterSpawn_VfTable391)), "player spawn hook" },
        { 0x00AEE3E4, 0x004C1C80, static_cast<DWORD>(addr_from_this(&CGObjMob::HandleMobKilled)), "monster death hook" },
        { 0x00AF2594, 0x004D2AD0, static_cast<DWORD>(addr_from_this(&CGObjCOS_GoldPet::FUN_004d2ad0)), "gold pet hook" },
        { 0x00AEE2A0, 0x0052A8E0, static_cast<DWORD>(addr_from_this(&CGObjMob::LivedpsMobAttackRecorder)), "live DPS hook" }
    };
    size_t s_corePointerHooksInstalled = 0;

    bool InstallCorePointerHooks()
    {
        const size_t hookCount = sizeof(kCorePointerHooks) / sizeof(kCorePointerHooks[0]);
        size_t installedCount = 0;
        for (; installedCount < hookCount; ++installedCount)
        {
            const PointerHook& hook = kCorePointerHooks[installedCount];
            if (!GameServerRuntimeSafety::ReplacePointer(
                    hook.address,
                    hook.expected,
                    hook.replacement,
                    hook.name))
                break;
        }

        if (installedCount == hookCount)
        {
            s_corePointerHooksInstalled = hookCount;
            return true;
        }

        while (installedCount > 0)
        {
            --installedCount;
            const PointerHook& hook = kCorePointerHooks[installedCount];
            GameServerRuntimeSafety::ReplacePointer(
                hook.address,
                hook.replacement,
                hook.expected,
                "pointer-hook rollback");
        }
        return false;
    }

    void RemoveCorePointerHooks()
    {
        while (s_corePointerHooksInstalled > 0)
        {
            --s_corePointerHooksInstalled;
            const PointerHook& hook = kCorePointerHooks[s_corePointerHooksInstalled];
            GameServerRuntimeSafety::ReplacePointer(
                hook.address, hook.replacement, hook.expected, "core pointer hook rollback");
        }
    }

    void RollbackInitialization()
    {
        RemoveCorePointerHooks();
        CItemRegionTravelGuard::Shutdown();
        CGObjEvents::Shutdown();
        CGObjSiegeStruct::Shutdown();
        CRegionAttackRestrictionsMgr::Shutdown();
        CDamageMeter::Shutdown();
        CLogCustoms::Shutdown();
        TradeGoldControl::Shutdown();
        PartyMonsterControl::Shutdown();
        DurabilityControl::Shutdown();
        CStaticPatches::Revert();
        InternalPacketAuth::Shutdown();
        UniqueSpawnGuard::Shutdown();
        GameServerTelemetry::Shutdown();
        CSqlCon::Shutdown();
    }
}

bool Init()
{
    try
    {
        CNewSettings::LoadIniSettings();
        if (CNewSettings::GetGameCfg() == NULL)
        {
            BS_INFO("[KMTGuard] Settings initialization failed");
            return false;
        }

        if (!CSqlCon::Initialize())
        {
            BS_INFO("[KMTGuard] Database initialization failed");
            return false;
        }

        if (!InternalPacketAuth::Initialize(
                CNewSettings::m_Settings->InternalPacketSharedSecret))
        {
            BS_INFO("[KMTGuard] Internal packet authentication initialization failed");
            RollbackInitialization();
            return false;
        }

        if (!CStaticPatches::Apply())
        {
            RollbackInitialization();
            return false;
        }

        if (!DurabilityControl::Initialize() ||
            !PartyMonsterControl::Initialize() ||
            !TradeGoldControl::Initialize())
        {
            RollbackInitialization();
            return false;
        }

        CGObjPC::Setup();
        CNetHelper::Initialize();
        GameServerTelemetry::Initialize();
        if (!UniqueSpawnGuard::Initialize())
        {
            BS_INFO("[KMTGuard] Unique-spawn policy initialization failed");
            RollbackInitialization();
            return false;
        }

        if (!CLogCustoms::Setup(KMTGUARD_VERSION_STRING) ||
            !CDamageMeter::Initialize() ||
            !CRegionAttackRestrictionsMgr::Initialize() ||
            !CGObjSiegeStruct::Initialize() ||
            !CGObjEvents::Initialize() ||
            !CItemRegionTravelGuard::Initialize() ||
            !InstallCorePointerHooks())
        {
            BS_INFO("[KMTGuard] Runtime hook installation failed");
            RollbackInitialization();
            return false;
        }

        // Runtime-setting writes are deliberately last. The transaction
        // restores every changed byte if any write fails, while all hooks
        // above remain reversible until this final commit succeeds.
        if (!CSqlCon::ApplyRuntimeSettings())
        {
            RollbackInitialization();
            return false;
        }

        return true;
    }
    catch (...)
    {
        RollbackInitialization();
        GameServerTelemetry::RecordRuntimeError();
        BS_INFO("[KMTGuard] Unexpected initialization failure");
        return false;
    }
}
