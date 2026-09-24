#!/usr/bin/env python3
from pathlib import Path
root=Path(__file__).resolve().parents[1]
base=root/'source/SilkroadOnline/SR_VIETNAM_SERVICE/v188/Server/SR_GameServer'
def read(rel): return (base/rel).read_text(encoding='utf-8-sig')
checks={
'unique entitlement': ('GameServer/GameServer/src/KMTGuardCustom/UniqueSpawnGuard.cpp','s_authorizedCharacterIds.find'),
'unique world policy': ('GameServer/GameServer/src/KMTGuardCustom/UniqueSpawnGuard.cpp','s_allowedWorlds.find'),
'unique token bucket': ('GameServer/GameServer/src/KMTGuardCustom/UniqueSpawnGuard.cpp','s_tokens -= 1.0'),
'dps CPU budget': ('GameServer/GameServer/src/Objects/GObjMob.cpp','KMT_DPS_BUDGET_US'),
'dps continuation': ('GameServer/GameServer/src/Objects/GObjMob.cpp','s_uniqueDpsPendingMobs.insert(pendingMobIds[i])'),
'bounded top N': ('GameServer/GameServer/src/Objects/GObjMob.cpp','DpsRecord records[UNIQUE_DPS_MAX_RECORDS + 1]'),
'64-bit DPS authority': ('GameServer/GameServer/src/Objects/GObjMob.cpp','unsigned __int64 total'),
'legacy DPS saturation': ('GameServer/GameServer/src/Objects/GObjMob.cpp','ToLegacyDamage'),
'multi-recipient DPS': ('GameServer/GameServer/src/Objects/GObjMob.cpp','recipientIds'),
'bounded item-lock queue': ('GameServer/GameServer/src/KMTGuardCustom/ItemLockPersistence.cpp','QUEUE_CAPACITY = 256'),
'worker-owned item-lock connection': ('GameServer/GameServer/src/KMTGuardCustom/ItemLockPersistence.cpp','CDbConnection connection'),
'initialization state machine': ('GameServer/GameServer/src/KMTGuardCustom/GameServerRuntimeSafety.cpp','s_initializationState'),
'startup dispatch gate': ('OutPut/src/DllMain.cpp','bootstrap custom-packet gate'),
'telemetry terminal state': ('GameServer/GameServer/src/KMTGuardCustom/GameServerTelemetry.cpp','s_telemetryState'),
'telemetry worker': ('GameServer/GameServer/src/KMTGuardCustom/GameServerTelemetry.cpp','DWORD WINAPI TelemetryWriter'),
'telemetry bounded queue': ('GameServer/GameServer/src/KMTGuardCustom/GameServerTelemetry.cpp','TELEMETRY_QUEUE_CAPACITY'),
'auth synchronized read': ('GameServer/GameServer/src/KMTGuardCustom/InternalPacketAuth.cpp','const bool initialized = s_initialized'),
'dedicated refresh connection': ('GameServer/GameServer/src/SqlConnection/sqlCon.cpp','CDbConnection refreshConnection'),
'independent refresh statement': ('GameServer/GameServer/src/SqlConnection/sqlCon.cpp','s_activeRefreshStatement'),
'module pin': ('OutPut/src/DllMain.cpp','GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS'),
'malformed offender tracking': ('GameServer/GameServer/src/KMTGuardCustom/GameServerTelemetry.cpp','s_malformedSessions[sessionId]'),
}
for name,(path,needle) in checks.items():
    assert needle in read(path), f'{name}: missing {needle}'
custom=read('GameServer/GameServer/src/KMTGuardCustom/CGObjPCCustom.cpp')
assert '0x705D' in custom, 'item linking was unintentionally removed'
assert 'AuthorizeAndConsume' in custom
assert 'SetItemLockState(' not in custom, 'packet path still executes item-lock SQL'
reverse=custom[custom.index('void CGObjPC::HandleCustomReverseUseRequest'):custom.index('void CGObjPC::HandleItemTranslateRequest')]
assert reverse.index('SetLiveDeleteItem(SlotID, 1)') < reverse.index('this->SendMsg(effect)')
assert reverse.index('this->SendMsg(effect)') < reverse.index('MoveTo(targetWorldId')
after_move = reverse[reverse.index('MoveTo(targetWorldId'):]
assert 'SetLiveDeleteItem' not in after_move
assert 'SendMsg(effect)' not in after_move

unique=read('GameServer/GameServer/src/KMTGuardCustom/UniqueSpawnGuard.cpp')
assert 'effectiveReplayWindow = s_replayWindowMs < s_cooldownMs' in unique

dllmain=read('OutPut/src/DllMain.cpp')
assert 'GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_PIN' in dllmain
assert dllmain.index('bootstrap custom-packet gate') < dllmain.index('CreateThread(')

for source in root.rglob('*'):
    if source.is_file() and source.suffix.lower() in {'.cpp','.h','.hpp','.cmake','.txt'}:
        text=source.read_text(encoding='utf-8-sig', errors='ignore')
        assert not any(marker in text for marker in ('<<<<<<<','>>>>>>>')), f'conflict marker: {source}'
print(f'PASS: {len(checks)} GameServer stability architecture checks')
