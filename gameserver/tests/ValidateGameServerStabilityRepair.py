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
'bounded top N': ('GameServer/GameServer/src/Objects/GObjMob.cpp','std::pair<DWORD, DWORD> records[UNIQUE_DPS_MAX_RECORDS]'),
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
print(f'PASS: {len(checks)} GameServer stability architecture checks')
