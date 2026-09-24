#!/usr/bin/env python3
"""Source-contract checks for the vSRO 188 stability repair."""

from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]


def read(relative: str) -> str:
    return (ROOT / relative).read_text(encoding="utf-8-sig")


def require(condition: bool, message: str) -> None:
    if not condition:
        raise AssertionError(message)


unique_queue = read("ShardManager/vSRO-ShardManager/Runtime/UniqueLogQueue.cpp")
sql_command_h = read("ShardManager/vSRO-ShardManager/Database/SQLCommand.h")
sql_command_cpp = read("ShardManager/vSRO-ShardManager/Database/SQLCommand.cpp")
game_sql = read(
    "gameserver/source/SilkroadOnline/SR_VIETNAM_SERVICE/v188/Server/"
    "SR_GameServer/GameServer/GameServer/src/SqlConnection/sqlCon.cpp"
)
db_connection = read(
    "gameserver/source/SilkroadOnline/SR_VIETNAM_SERVICE/v188/Server/"
    "SR_GameServer/GameServer/GameServer/src/SqlConnection/DbConnection.cpp"
)
game_message = read(
    "gameserver/source/SilkroadOnline/SR_VIETNAM_SERVICE/v188/Server/"
    "SR_GameServer/GameServer/App/src/Game.cpp"
)
log_message_h = read(
    "gameserver/source/SilkroadOnline/SR_VIETNAM_SERVICE/v188/Server/"
    "SR_GameServer/GameServer/GameServer/src/GSLog/MsgCustom.h"
)
timed_header = read(
    "gameserver/source/SilkroadOnline/SR_VIETNAM_SERVICE/v188/Server/"
    "SR_GameServer/GameServer/GameServer/src/Objects/CustomTimedJobManager.h"
)
timed_source = read(
    "gameserver/source/SilkroadOnline/SR_VIETNAM_SERVICE/v188/Server/"
    "SR_GameServer/GameServer/GameServer/src/Objects/CustomTimedJobManager.cpp"
)
database_queue = read("filter/KMTGuardnew/KMTGuard/Helpers/DatabaseJobQueue.cs")
identifier = read("filter/KMTGuardnew/KMTGuard/Database/SqlIdentifier.cs")
auto_event = read(
    "filter/KMTGuardnew/KMTGuard/Features/AutoEvents/AutoEventService.cs"
)
client_main = read("JTClientLibrary/source/DevKit_DLL/src/DllMain.cpp")
client_d3d = read("JTClientLibrary/source/DevKit_DLL/src/hooks/GFXVideo3d_Hook.cpp")

for state in ("WORKER_STOPPED", "WORKER_RUNNING", "WORKER_STOPPING"):
    require(state in unique_queue, f"UniqueLogQueue is missing {state}")
for wait_state in ("WAIT_OBJECT_0", "WAIT_TIMEOUT", "WAIT_FAILED"):
    require(wait_state in unique_queue, f"UniqueLogQueue is missing {wait_state}")
require("activeCommand->Cancel()" in unique_queue, "Unique SQL cancellation is missing")
require("Sleep(2000)" not in unique_queue and "Sleep(1000)" not in unique_queue,
        "Unique worker still has a non-interruptible retry sleep")

for result in (
    "SQL_DATA_SUCCESS_WITH_INFO",
    "SQL_DATA_TRUNCATED",
    "SQL_DATA_NO_DATA",
    "SQL_DATA_ERROR",
):
    require(result in sql_command_h, f"ODBC classification is missing {result}")
require("SQL_SUCCEEDED(result)" in sql_command_cpp, "ODBC info-success is rejected")
require("SQL_NO_TOTAL" in sql_command_cpp, "ODBC unknown-length truncation is unchecked")
require("SQLCancelHandle" in sql_command_cpp, "ShardManager statement cancellation is missing")

require("SQL_LOGIN_TIMEOUT" in db_connection, "GameServer login timeout is missing")
require("SQL_ATTR_QUERY_TIMEOUT" in db_connection, "GameServer query timeout is missing")
require("CancelActiveSqlStatement();" in game_sql, "GameServer shutdown cancellation is missing")
require("WaitForSingleObject(s_securityRefreshStopEvent, 0)" in game_sql,
        "GameServer refresh does not recheck shutdown between snapshot queries")
for wait_state in ("WAIT_OBJECT_0", "WAIT_TIMEOUT", "WAIT_FAILED"):
    require(wait_state in game_sql, f"GameServer shutdown is missing {wait_state}")
require("SQL resources were retained" in game_sql,
        "GameServer does not retain SQL state after a failed join")

require("TryReadLiveSkillCodeName(pMsg, SkillCodeName)" in game_message,
        "ActionAddSkillByCode does not use the fixed-width wire reader")
require("message->ReadBytes(rawCodeName, sizeof(rawCodeName))" in game_message,
        "ActionAddSkillByCode does not read its complete fixed-width payload")
require("terminator == rawCodeName + LIVE_SKILL_CODE_NAME_SIZE" in game_message,
        "ActionAddSkillByCode does not reject unterminated skill names")
require("ReadString(SkillCodeName" not in game_message,
        "ActionAddSkillByCode still treats its fixed-width payload as a length-prefixed string")
require("void Read(void* dest, size_t count)" in log_message_h,
        "Custom log reader still narrows the byte count")

require("KMT_ENABLE_UNSAFE_CUSTOM_TIMED_JOB_WORKER" in timed_header,
        "Timed worker compile-time guard is missing")
require("#error The archived CustomTimedJobManager worker is unsafe" in timed_header,
        "Timed worker can be enabled accidentally")
create_start = timed_source.index("void CCustomTimedJobManager::CreateConsumeThread()")
create_end = timed_source.index("DWORD WINAPI", create_start)
require("CreateThread" not in timed_source[create_start:create_end],
        "Timed worker entry point still creates a detached thread")

require("class WorkerGeneration" in database_queue,
        "Filter database queue has no generation ownership")
require("ProcessQueueAsync(WorkerGeneration generation)" in database_queue,
        "Filter workers still read replaceable static lifecycle state")
require("await Task.WhenAll(generation.Workers);" in database_queue,
        "Filter disposes a generation before workers join")
require("generation.Shutdown.Dispose();" in database_queue,
        "Filter generation token is not disposed after join")

require("QuoteAllowed" in identifier, "Central SQL identifier allow-list API is missing")
require("EventTableNames" in auto_event and "QuoteAllowed(tableName, EventTableNames)" in auto_event,
        "AutoEvent table interpolation bypasses the allow-list")

require("hook publication was blocked" in client_main,
        "Client does not fail closed when the startup gate is unavailable")
require("return FALSE;" in client_main[client_main.index("hook publication was blocked"):],
        "Client continues after startup-gate failure")
require("if (device == NULL)" in client_d3d,
        "D3D EndScene still dereferences a missing device")

print("KMTGuard vSRO 188 stability repair source checks: PASS")
