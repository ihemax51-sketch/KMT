#include <DevNew/CustomNPCEvent.h>
#include "sqlCon.h"
#include <Game.h>
#include <SettingMgr/NewSettings.h>
#include "BSObj/BSObj.h"
#include <memory/Process.h>
#include <psapi.h>
#include <cerrno>
#include <cctype>
#include <cstring>
#include <cstdlib>
#include <new>
#include <KMTGuardCustom/GameServerTelemetry.h>
#include <KMTGuardCustom/GameServerRuntimeSafety.h>
#pragma comment(lib, "odbc32.lib")
#pragma comment(lib, "psapi.lib")

namespace
{
    CRITICAL_SECTION s_lockedItemListLock;
    CRITICAL_SECTION s_attackRestrictionLock;
    CRITICAL_SECTION s_itemRegionRestrictionLock;
    CRITICAL_SECTION s_fortressDpsLock;
    CRITICAL_SECTION s_activeSqlStatementLock;
    CAutoCriticalSection s_sqlConnectionLock;
    HANDLE s_securityRefreshStopEvent = NULL;
    HANDLE s_securityRefreshThread = NULL;
    SQLHANDLE s_activeSqlStatement = SQL_NULL_HSTMT;
    SQLHANDLE s_activeRefreshStatement = SQL_NULL_HSTMT;
    volatile LONG s_securityRefreshState = 0;
    const DWORD SECURITY_REFRESH_SHUTDOWN_TIMEOUT_MS = 35000;

    class ScopedSqlConnectionLock
    {
    public:
        explicit ScopedSqlConnectionLock(CAutoCriticalSection& lock, bool acquire = true)
            : m_lock(lock), m_acquired(acquire)
        {
            if (m_acquired) m_lock.Enter();
        }

        ~ScopedSqlConnectionLock()
        {
            if (m_acquired) m_lock.Leave();
        }

    private:
        CAutoCriticalSection& m_lock;
        bool m_acquired;
        ScopedSqlConnectionLock(const ScopedSqlConnectionLock&);
        ScopedSqlConnectionLock& operator=(const ScopedSqlConnectionLock&);
    };

    class ScopedWinCriticalSection
    {
    public:
        explicit ScopedWinCriticalSection(CRITICAL_SECTION& lock)
            : m_lock(lock)
        {
            EnterCriticalSection(&m_lock);
        }

        ~ScopedWinCriticalSection()
        {
            LeaveCriticalSection(&m_lock);
        }

    private:
        CRITICAL_SECTION& m_lock;
        ScopedWinCriticalSection(const ScopedWinCriticalSection&);
        ScopedWinCriticalSection& operator=(const ScopedWinCriticalSection&);
    };

    class ScopedSqlStatement
    {
    public:
        explicit ScopedSqlStatement(CDbConnection* connection, bool refreshStatement = false)
            : m_connection(connection), m_handle(SQL_NULL_HSTMT), m_refreshStatement(refreshStatement)
        {
        }

        ~ScopedSqlStatement()
        {
            if (m_connection != NULL && m_handle != SQL_NULL_HSTMT)
            {
                ClearActive();
                m_connection->FreeStmt(m_handle);
            }
        }

        bool Allocate()
        {
            if (m_connection == NULL || !m_connection->AllocStmt(m_handle))
                return false;
            EnterCriticalSection(&s_activeSqlStatementLock);
            (m_refreshStatement ? s_activeRefreshStatement : s_activeSqlStatement) = m_handle;
            LeaveCriticalSection(&s_activeSqlStatementLock);
            return true;
        }

        SQLHANDLE Get() const
        {
            return m_handle;
        }

        void ClearActive()
        {
            EnterCriticalSection(&s_activeSqlStatementLock);
            SQLHANDLE& active = m_refreshStatement ? s_activeRefreshStatement : s_activeSqlStatement;
            if (active == m_handle) active = SQL_NULL_HSTMT;
            LeaveCriticalSection(&s_activeSqlStatementLock);
        }

    private:
        CDbConnection* m_connection;
        SQLHANDLE m_handle;
        bool m_refreshStatement;
        ScopedSqlStatement(const ScopedSqlStatement&);
        ScopedSqlStatement& operator=(const ScopedSqlStatement&);
    };

    struct LockedItemListLockInitializer
    {
        LockedItemListLockInitializer()
        {
            InitializeCriticalSection(&s_lockedItemListLock);
            InitializeCriticalSection(&s_attackRestrictionLock);
            InitializeCriticalSection(&s_itemRegionRestrictionLock);
            InitializeCriticalSection(&s_fortressDpsLock);
            InitializeCriticalSection(&s_activeSqlStatementLock);
        }

        ~LockedItemListLockInitializer()
        {
            // A timed-out shutdown deliberately retains all synchronization
            // state. The host must terminate rather than unload this add-on
            // while the SQL worker is still alive.
            if (s_securityRefreshThread != NULL)
                return;
            DeleteCriticalSection(&s_activeSqlStatementLock);
            DeleteCriticalSection(&s_fortressDpsLock);
            DeleteCriticalSection(&s_itemRegionRestrictionLock);
            DeleteCriticalSection(&s_attackRestrictionLock);
            DeleteCriticalSection(&s_lockedItemListLock);
        }
    } s_lockedItemListLockInitializer;

    DWORD WINAPI SecuritySnapshotRefreshWorker(LPVOID)
    {
        CDbConnection refreshConnection(CNewSettings::m_Settings->DatabaseConnectionString);
        if (!refreshConnection.Connect())
        {
            GameServerTelemetry::RecordSecuritySnapshotRefresh(false);
            InterlockedExchange(&s_securityRefreshState, 0);
            return 0;
        }
        for (;;)
        {
            const DWORD waitResult = WaitForSingleObject(s_securityRefreshStopEvent, 60000);
            if (waitResult != WAIT_TIMEOUT)
                break;

            const bool lockedItemsLoaded = CSqlCon::LoadLockedItems(&refreshConnection, true);
            if (WaitForSingleObject(s_securityRefreshStopEvent, 0) == WAIT_OBJECT_0)
                break;
            const bool fortressDpsLoaded = CSqlCon::LoadFortressDPSInfo(&refreshConnection, true);
            GameServerTelemetry::RecordSecuritySnapshotRefresh(
                lockedItemsLoaded && fortressDpsLoaded);
            if (!lockedItemsLoaded || !fortressDpsLoaded)
                BS_INFO("[KMTGuard][Security] Snapshot refresh failed; retaining the last valid snapshot");
        }
        refreshConnection.Disconnect();
        InterlockedExchange(&s_securityRefreshState, 0);
        return 0;
    }

    void CancelActiveSqlStatement()
    {
        EnterCriticalSection(&s_activeSqlStatementLock);
        const SQLHANDLE statement = s_activeSqlStatement;
        if (statement != SQL_NULL_HSTMT)
            SQLCancel(static_cast<SQLHSTMT>(statement));
        const SQLHANDLE refreshStatement = s_activeRefreshStatement;
        if (refreshStatement != SQL_NULL_HSTMT)
            SQLCancel(static_cast<SQLHSTMT>(refreshStatement));
        LeaveCriticalSection(&s_activeSqlStatementLock);
    }

    bool IsCoreBooleanSetting(const char* name)
    {
        return strcmp(name, "DisableDurability") == 0 ||
               strcmp(name, "DisableGreenBook") == 0 ||
               strcmp(name, "ShowGmUniqueKillNotice") == 0 ||
               strcmp(name, "ForceGmVisibleOnSpawn") == 0 ||
               strcmp(name, "DisableOriginalTradeGold") == 0 ||
               strcmp(name, "EnablePartyMonsterSpawn") == 0 ||
               strcmp(name, "HIGH_RATES_CONFIG") == 0 ||
               strcmp(name, "FIX_EXPLOIT_INVISIBLE_INVINCIBLE") == 0 ||
               strcmp(name, "EXCHANGE_ATTACK_CANCEL") == 0 ||
               strcmp(name, "GUILD_POINTS") == 0 ||
               strcmp(name, "FIX_GRAP_PET_PAGE") == 0 ||
               strcmp(name, "ALLOW_NON_GM_MERCENARY_SPAWN") == 0 ||
               strcmp(name, "DISABLE_GRANT_NAME_CONDITIONS") == 0 ||
               strcmp(name, "DISABLE_MOB_SPAWN_WHILE_TRADE") == 0;
    }

    bool IsCoreStringSetting(const char* name)
    {
        return strcmp(name, "CTF_ITEM_WIN_REWARD") == 0 ||
               strcmp(name, "CTF_ITEM_KILL_REWARD") == 0 ||
               strcmp(name, "BA_ITEM_REWARD") == 0;
    }

    bool ValidateCoreSettingText(const char* name, const char* value)
    {
        if (name == NULL || value == NULL || name[0] == '\0' || value[0] == '\0')
            return false;

        if (IsCoreStringSetting(name))
            return strlen(value) <= 128;

        if (IsCoreBooleanSetting(name))
            return _stricmp(value, "true") == 0 || _stricmp(value, "false") == 0 ||
                   strcmp(value, "1") == 0 || strcmp(value, "0") == 0;

        errno = 0;
        char* end = NULL;
        _strtoi64(value, &end, 10);
        if (errno == ERANGE || end == value)
            return false;
        while (*end != '\0' && std::isspace(static_cast<unsigned char>(*end)))
            ++end;
        return *end == '\0';
    }

    bool IsMemoryDiagnosticsEnabled()
    {
        char value[8] = { 0 };
        const DWORD length = GetEnvironmentVariableA("KMTGUARD_MEMORY_DIAGNOSTICS", value, sizeof(value));
        return length > 0 && value[0] != '0';
    }

    std::string EscapeSqlStringLiteral(const char* value)
    {
        std::string escaped;
        if (value == NULL)
            return escaped;

        const size_t length = strlen(value);
        escaped.reserve(length);
        for (size_t i = 0; i < length; ++i)
        {
            if (value[i] == '\'')
                escaped.push_back('\'');
            escaped.push_back(value[i]);
        }
        return escaped;
    }

    SIZE_T GetCurrentWorkingSetBytes()
    {
        PROCESS_MEMORY_COUNTERS counters = { 0 };
        counters.cb = sizeof(counters);
        if (!GetProcessMemoryInfo(GetCurrentProcess(), &counters, sizeof(counters)))
            return 0;
        return counters.WorkingSetSize;
    }

    void LogSqlInitializationMemory(const char* phase, SIZE_T beforeBytes, DWORD elapsedMs)
    {
        if (!IsMemoryDiagnosticsEnabled())
            return;

        char line[320] = { 0 };
        _snprintf(line, sizeof(line) - 1,
                  "[KMTGuardMemory][GameServer] %s elapsed_ms=%lu working_set_before=%lu working_set_after=%lu locked_items=%u timed_items=%u timed_devil_items=%u\n",
                  phase,
                  static_cast<unsigned long>(elapsedMs),
                  static_cast<unsigned long>(beforeBytes),
                  static_cast<unsigned long>(GetCurrentWorkingSetBytes()),
                  static_cast<unsigned int>(CSqlCon::LockedItemList.size()),
                  static_cast<unsigned int>(CSqlCon::TimedItemList.size()),
                  static_cast<unsigned int>(CSqlCon::STimedDevillList.size()));
        OutputDebugStringA(line);
    }
}

CDbConnection* CSqlCon::m_connectionstr;
CRegionRestrictionDBSet* CSqlCon::s_pRegionRestrictionDbSet;
std::list<SServerAutoCapebyRegionID> CSqlCon::AutoCapeList;
std::list<SServerAutoCapebyWorldID> CSqlCon::AutoCapeListWorldId;
std::map<INT64, STimedItemPlusDbRecord> CSqlCon::TimedItemList;

std::map<INT64, STimedDevillPlusDbRecord> CSqlCon::STimedDevillList;


std::map<int, _RefAbilityByItemOptLevel> CSqlCon::RefAbilitybyItemOptLevel;
std::map<int, _RefSkillByItemOptLevel> CSqlCon::RefSkillByItemOptLevel;
std::map<int, _ServerFortressDpsInfo> CSqlCon::ServerFortressDpsInfo;
std::list<SCustomNpcInteractionDbRecord> CSqlCon::s_CustomNpcInteractions;


// mob restrict
std::map<int, SAttackRestrictionByMob> CSqlCon::s_AttackRestrByMob;
std::vector<SItemRegionRestriction> CSqlCon::s_ItemRegionRestrictions;

void CSqlCon::Shutdown()
{
    InterlockedExchange(&s_securityRefreshState, 2);
    if (s_securityRefreshStopEvent != NULL)
        SetEvent(s_securityRefreshStopEvent);
    CancelActiveSqlStatement();

    if (s_securityRefreshThread != NULL)
    {
        const DWORD waitResult = WaitForSingleObject(
            s_securityRefreshThread, SECURITY_REFRESH_SHUTDOWN_TIMEOUT_MS);
        if (waitResult == WAIT_TIMEOUT)
        {
            BS_INFO("[KMTGuard][Security] Refresh worker shutdown timed out; SQL resources were retained");
            return;
        }
        if (waitResult == WAIT_FAILED)
        {
            BS_INFO("[KMTGuard][Security] Refresh worker shutdown wait failed; SQL resources were retained");
            return;
        }
        if (waitResult != WAIT_OBJECT_0)
        {
            BS_INFO("[KMTGuard][Security] Refresh worker returned an unexpected wait result; SQL resources were retained");
            return;
        }
        CloseHandle(s_securityRefreshThread);
        s_securityRefreshThread = NULL;
    }

    if (s_securityRefreshStopEvent != NULL)
    {
        CloseHandle(s_securityRefreshStopEvent);
        s_securityRefreshStopEvent = NULL;
    }

    ScopedSqlConnectionLock guard(s_sqlConnectionLock);
    delete m_connectionstr;
    m_connectionstr = NULL;
    InterlockedExchange(&s_securityRefreshState, 0);
}

bool CSqlCon::StartSecuritySnapshotRefresh()
{
    if (s_securityRefreshThread != NULL)
        return true;

    s_securityRefreshStopEvent = CreateEventA(NULL, TRUE, FALSE, NULL);
    if (s_securityRefreshStopEvent == NULL)
        return false;

    s_securityRefreshThread = CreateThread(
        NULL, 0, SecuritySnapshotRefreshWorker, NULL, 0, NULL);
    if (s_securityRefreshThread == NULL)
    {
        CloseHandle(s_securityRefreshStopEvent);
        s_securityRefreshStopEvent = NULL;
        return false;
    }
    InterlockedExchange(&s_securityRefreshState, 1);
    return true;
}

bool CSqlCon::TryExecNonQuery(const char* szQuery)
{
    const DWORD started = GetTickCount();
    if (szQuery == NULL || szQuery[0] == '\0' || m_connectionstr == NULL)
    {
        GameServerTelemetry::RecordDatabaseQuery(GetTickCount() - started, false);
        return false;
    }

    ScopedSqlConnectionLock guard(s_sqlConnectionLock);
    SQLHANDLE hStmt = SQL_NULL_HSTMT;
    CDbConnection* m_pDbConnection = m_connectionstr;

    if (!m_pDbConnection->AllocStmt(hStmt))
    {
        GameServerTelemetry::RecordDatabaseQuery(GetTickCount() - started, false);
        BS_INFO("[KMTGuard][Database] Statement allocation failed");
        return false;
    }

    const SQLRETURN retCode = SQLExecDirectA(hStmt, (SQLCHAR*)szQuery, SQL_NTS);
    if (!SQL_SUCCEEDED(retCode))
    {
        m_pDbConnection->FreeStmt(hStmt);
        GameServerTelemetry::RecordDatabaseQuery(GetTickCount() - started, false);
        BS_INFO("[KMTGuard][Database] Non-query execution failed");
        return false;
    }

    m_pDbConnection->FreeStmt(hStmt);
    GameServerTelemetry::RecordDatabaseQuery(GetTickCount() - started, true);
    return true;
}

bool CSqlCon::Initialize()
{
    const DWORD diagnosticsStartTick = GetTickCount();
    const SIZE_T diagnosticsWorkingSetBefore = IsMemoryDiagnosticsEnabled() ? GetCurrentWorkingSetBytes() : 0;

    Shutdown();

    if (CNewSettings::m_Settings == NULL)
        return false;

    m_connectionstr = new (std::nothrow) CDbConnection(
        CNewSettings::m_Settings->DatabaseConnectionString);
    if (m_connectionstr == NULL)
        return false;

    if (!m_connectionstr->Connect())
    {
        BS_INFO("[KMTGuard][Database] Connection failed");
        m_connectionstr->Disconnect();
        delete m_connectionstr;
        m_connectionstr = NULL;
        LogSqlInitializationMemory("sql_initialize_failed", diagnosticsWorkingSetBefore,
                                   GetTickCount() - diagnosticsStartTick);
        return false;
    }
    else
    {
        BS_INFO("[KMTGuard][Database] Connected");
    }
    if (!LoadGameServerSettings())
    {
        BS_INFO("[KMTGuard][Database] Core GameServer settings unavailable");
        Shutdown();
        return false;
    }
    if (!LoadInternalPacketSharedSecret())
    {
        BS_INFO("[KMTGuard][Security] Internal packet shared secret unavailable");
        Shutdown();
        return false;
    }

    std::string validationError;
    if (!CNewSettings::Validate(validationError))
    {
        BS_INFO("[KMTGuard][Settings] Validation failed: %s", validationError.c_str());
        Shutdown();
        return false;
    }
    if (!ApplyRuntimeSettings())
    {
        BS_INFO("[KMTGuard][Settings] Runtime settings could not be applied");
        Shutdown();
        return false;
    }

    if (!LoadLockedItems())
    {
        BS_INFO("[KMTGuard][Database] Locked-item cache unavailable");
        Shutdown();
        return false;
    }
    if (!LoadAttackRestrictionsByMob())
    {
        BS_INFO("[KMTGuard][Database] Attack restrictions unavailable");
        Shutdown();
        return false;
    }
    if (!LoadItemRegionRestrictions())
    {
        BS_INFO("[KMTGuard][Database] Item-region restrictions unavailable");
        Shutdown();
        return false;
    }
    if (!LoadFortressDPSInfo())
    {
        BS_INFO("[KMTGuard][Database] Fortress DPS configuration unavailable");
        Shutdown();
        return false;
    }
    GameServerTelemetry::RecordSecuritySnapshotRefresh(true);
    if (!StartSecuritySnapshotRefresh())
    {
        BS_INFO("[KMTGuard][Security] Snapshot refresh worker failed to start");
        Shutdown();
        return false;
    }

    LogSqlInitializationMemory("sql_initialize_complete", diagnosticsWorkingSetBefore,
                               GetTickCount() - diagnosticsStartTick);
    return true;
}

bool CSqlCon::LoadInternalPacketSharedSecret()
{
    if (m_connectionstr == NULL || CNewSettings::m_Settings == NULL)
        return false;

    ScopedSqlConnectionLock databaseGuard(s_sqlConnectionLock);
    ScopedSqlStatement statement(m_connectionstr);
    if (!statement.Allocate())
        return false;

    const LPCSTR query =
        "SELECT TOP (1) Value FROM [KMTGuard].[dbo].[System_Settings] "
        "WHERE SettingName = 'Security_InternalPacketSharedSecret'";
    SQLRETURN result = SQLExecDirectA(statement.Get(), (SQLCHAR*)query, SQL_NTS);
    if (!SQL_SUCCEEDED(result) || !SQL_SUCCEEDED(result = SQLFetch(statement.Get())))
        return false;

    SQLCHAR value[65] = { 0 };
    SQLLEN length = 0;
    result = SQLGetData(statement.Get(), 1, SQL_C_CHAR, value, sizeof(value), &length);
    if (!SQL_SUCCEEDED(result) || length != 64)
        return false;

    CNewSettings::m_Settings->InternalPacketSharedSecret.assign(
        reinterpret_cast<const char*>(value), 64);
    return true;
}

std::vector<INT64> CSqlCon::LockedItemList;

bool CSqlCon::IsItemLocked(INT64 itemId)
{
    ScopedWinCriticalSection guard(s_lockedItemListLock);
    return
        std::binary_search(LockedItemList.begin(), LockedItemList.end(), itemId);
}

void CSqlCon::AddLockedItem(INT64 itemId)
{
    size_t cacheSize = 0;
    {
        ScopedWinCriticalSection guard(s_lockedItemListLock);
        std::vector<INT64>::iterator position =
            std::lower_bound(LockedItemList.begin(), LockedItemList.end(), itemId);
        if (position == LockedItemList.end() || *position != itemId)
            LockedItemList.insert(position, itemId);
        cacheSize = LockedItemList.size();
    }
    GameServerTelemetry::SetLockedItemCacheSize(cacheSize);
}

void CSqlCon::RemoveLockedItem(INT64 itemId)
{
    size_t cacheSize = 0;
    {
        ScopedWinCriticalSection guard(s_lockedItemListLock);
        std::vector<INT64>::iterator position =
            std::lower_bound(LockedItemList.begin(), LockedItemList.end(), itemId);
        if (position != LockedItemList.end() && *position == itemId)
            LockedItemList.erase(position);
        cacheSize = LockedItemList.size();
    }
    GameServerTelemetry::SetLockedItemCacheSize(cacheSize);
}

bool CSqlCon::LoadLockedItems(CDbConnection* connection, bool workerStatement)
{
    ScopedSqlConnectionLock databaseGuard(s_sqlConnectionLock, connection == NULL);
    CDbConnection* m_pDbConnection = connection != NULL ? connection : m_connectionstr;

    if (m_pDbConnection == NULL)
        return false;

    ScopedSqlStatement statement(m_pDbConnection, workerStatement);
    if (!statement.Allocate())
    {
        BS_INFO("[KMTGuard][Database] Locked-item statement allocation failed");
        return false;
    }

    SQLRETURN retCode;
    const LPCSTR szQuery = "SELECT ItemID64 FROM [dbo].[Item_Locked]";
    std::vector<INT64> loadedItems;

    retCode = SQLExecDirectA(statement.Get(), (SQLCHAR*)szQuery, SQL_NTS);
    if (!SQL_SUCCEEDED(retCode))
    {
        BS_INFO("[KMTGuard][Database] Locked-item cache query failed");
        return false;
    }

    retCode = SQLFetch(statement.Get());

    if (retCode == SQL_NO_DATA)
    {
        {
            ScopedWinCriticalSection cacheGuard(s_lockedItemListLock);
            LockedItemList.clear();
        }
        GameServerTelemetry::SetLockedItemCacheSize(0);
        return true;
    }

    if (!SQL_SUCCEEDED(retCode))
    {
        BS_INFO("[KMTGuard][Database] Locked-item cache fetch failed");
        return false;
    }

    do
    {
        INT64 itemId = 0;
        SQLLEN itemLength = 0;
        const SQLRETURN itemResult = SQLGetData(
            statement.Get(), 1, SQL_C_SBIGINT, &itemId, sizeof(itemId), &itemLength);
        if (!SQL_SUCCEEDED(itemResult) || itemLength == SQL_NULL_DATA || itemId <= 0)
        {
            BS_INFO("[KMTGuard][Database] Locked-item row is invalid");
            return false;
        }
        loadedItems.push_back(itemId);

        retCode = SQLFetch(statement.Get());
    } while (SQL_SUCCEEDED(retCode));

    if (retCode != SQL_NO_DATA)
    {
        BS_INFO("[KMTGuard][Database] Locked-item cache ended with a fetch error");
        return false;
    }

    std::sort(loadedItems.begin(), loadedItems.end());
    loadedItems.erase(
        std::unique(loadedItems.begin(), loadedItems.end()),
        loadedItems.end());

    size_t cacheSize = 0;
    {
        ScopedWinCriticalSection cacheGuard(s_lockedItemListLock);
        LockedItemList.swap(loadedItems);
        cacheSize = LockedItemList.size();
    }
    GameServerTelemetry::SetLockedItemCacheSize(cacheSize);

    return true;
}

CSqlCon::ItemLockStateResult CSqlCon::SetItemLockState(INT64 itemId, bool locked)
{
    if (itemId <= 0 || m_connectionstr == NULL)
        return ITEM_LOCK_STATE_FAILED;

    ScopedSqlConnectionLock databaseGuard(s_sqlConnectionLock);
    ScopedSqlStatement statement(m_connectionstr);
    if (!statement.Allocate())
        return ITEM_LOCK_STATE_FAILED;
    if (!SQL_SUCCEEDED(SQLSetStmtAttr(
            statement.Get(), SQL_ATTR_QUERY_TIMEOUT,
            reinterpret_cast<SQLPOINTER>(3), 0)))
        return ITEM_LOCK_STATE_FAILED;

    char query[256] = { 0 };
    _snprintf(query, sizeof(query) - 1,
              "EXEC [KMTGuard].[dbo].[_KmtSetItemLockState] @ItemID64=%I64d, @Locked=%d",
              itemId, locked ? 1 : 0);

    SQLRETURN result = SQLExecDirectA(statement.Get(), (SQLCHAR*)query, SQL_NTS);
    if (!SQL_SUCCEEDED(result) || !SQL_SUCCEEDED(result = SQLFetch(statement.Get())))
        return ITEM_LOCK_STATE_FAILED;

    int resultCode = 0;
    SQLLEN length = 0;
    result = SQLGetData(statement.Get(), 1, SQL_C_LONG, &resultCode, sizeof(resultCode), &length);
    if (!SQL_SUCCEEDED(result) || length == SQL_NULL_DATA ||
        (resultCode != ITEM_LOCK_STATE_CHANGED && resultCode != ITEM_LOCK_STATE_ALREADY_SET))
        return ITEM_LOCK_STATE_FAILED;

    return static_cast<ItemLockStateResult>(resultCode);
}

#include "Process.h"
void CSqlCon::GameServerInitialized()
{
    if(g_pCGame != NULL)
    {
        const size_t NPC_SYNC_BATCH_LIMIT = 32768;
        std::string npcSyncBatch;
        npcSyncBatch.reserve(NPC_SYNC_BATCH_LIMIT);
        unsigned int npcCount = 0;
        unsigned int batchCount = 0;
        unsigned int failedBatchCount = 0;

        std::list<std::pair<DWORD, CGObj*> >::iterator it;
        for (it = g_pCGame->m_listObjGameId.begin(); it != g_pCGame->m_listObjGameId.end(); ++it) {
            CGObj* obj = it->second;

            if (obj != NULL) {
                if(obj->IsNPCNPC())
                {
                    const char* codeName = obj->GetCodeName();
                    if (codeName == NULL || codeName[0] == '\0')
                        continue;

                    char statementPrefix[160] = { 0 };
                    _snprintf(statementPrefix, sizeof(statementPrefix) - 1,
                              "EXEC [KMTGuard].[dbo].[NPC_Sync] %u, %d, '",
                              obj->GetGameID(), obj->GetRefObjID());

                    std::string statement(statementPrefix);
                    statement += EscapeSqlStringLiteral(codeName);
                    statement += "';";

                    if (!npcSyncBatch.empty() &&
                        npcSyncBatch.size() + statement.size() > NPC_SYNC_BATCH_LIMIT)
                    {
                        if (!TryExecNonQuery(npcSyncBatch.c_str()))
                            ++failedBatchCount;
                        ++batchCount;
                        npcSyncBatch.clear();
                    }

                    npcSyncBatch += statement;
                    ++npcCount;
                }
            }
        }

        if (!npcSyncBatch.empty())
        {
            if (!TryExecNonQuery(npcSyncBatch.c_str()))
                ++failedBatchCount;
            ++batchCount;
        }

        BS_INFO("[KMTGuard][Database] NPC sync: objects=%u batches=%u failed=%u",
                npcCount, batchCount, failedBatchCount);

        const std::string path = GetProcessInstanceId();
        std::string startupQuery = "EXEC [KMTGuard].[dbo].[Hook_GameServerStart] '";
        startupQuery += EscapeSqlStringLiteral(path.c_str());
        startupQuery += "'";

        // Sorguyu çalıştır
        if (!TryExecNonQuery(startupQuery.c_str()))
            BS_INFO("[KMTGuard][Database] GameServer start notification failed");
    }
}
std::string  CSqlCon::GetProcessInstanceId()
{
    // Check unique process instances using the executable path
    std::string path = GetExecutablePath();
    StringReplaceAll(path, "\\", "/"); // Replace special symbols used on mutex

    return path;
}
long long my_atoll(const char* str)
{
    long long result = 0;
    int sign = 1;

    while (*str == ' ' || *str == '\t') ++str;
    if (*str == '-') { sign = -1; ++str; }
    else if (*str == '+') { ++str; }

    while (*str >= '0' && *str <= '9')
    {
        result = result * 10 + (*str - '0');
        ++str;
    }

    return sign * result;
}
bool CSqlCon::LoadGameServerSettings()
{
    CDbConnection* m_pDbConnection = m_connectionstr;
    ScopedSqlConnectionLock connectionGuard(s_sqlConnectionLock);
    ScopedSqlStatement statement(m_pDbConnection);
    if (!statement.Allocate())
    {
        std::cout << __FUNCTION__ << " - Failed to allocate stmt" << std::endl;
        return false;
    }
    SQLHANDLE hStmt = statement.Get();

    SQLRETURN retCode;
    const LPCSTR szQuery = "SELECT ID, SettingName, Value FROM [KMTGuard].[dbo].[System_GameServerSettings]";

    retCode = SQLExecDirectA(hStmt, (SQLCHAR*)szQuery, SQL_NTS);
    if (!SQL_SUCCEEDED(retCode))
    {
        std::cout << __FUNCTION__ << " - SQLExecDirect failed, query = " << szQuery << std::endl;
        return false;
    }

    retCode = SQLFetch(hStmt);

    if (retCode == SQL_NO_DATA)
    {
        return false;
    }

    if (!SQL_SUCCEEDED(retCode))
    {
        std::cout << __FUNCTION__ << " - SQLFetch failed, query = " << szQuery << std::endl;
        return false;
    }

    SQLCHAR szSettingName[129] = { 0 };
    SQLCHAR szValue[129] = { 0 };

    do
    {
        std::memset(szSettingName, 0, sizeof(szSettingName));
        std::memset(szValue, 0, sizeof(szValue));
        SQLLEN settingNameLength = 0;
        SQLLEN valueLength = 0;
        const SQLRETURN settingNameResult = SQLGetData(
            hStmt, 2, SQL_C_CHAR, szSettingName, sizeof(szSettingName), &settingNameLength);
        const SQLRETURN valueResult = SQLGetData(
            hStmt, 3, SQL_C_CHAR, szValue, sizeof(szValue), &valueLength);
        if (!SQL_SUCCEEDED(settingNameResult) || !SQL_SUCCEEDED(valueResult) ||
            settingNameLength == SQL_NULL_DATA || valueLength == SQL_NULL_DATA ||
            settingNameLength >= static_cast<SQLLEN>(sizeof(szSettingName)) ||
            valueLength >= static_cast<SQLLEN>(sizeof(szValue)))
        {
            BS_INFO("[KMTGuard][Settings] Invalid or truncated database row");
            return false;
        }
        if (!ValidateCoreSettingText(
                reinterpret_cast<const char*>(szSettingName),
                reinterpret_cast<const char*>(szValue)))
        {
            BS_INFO("[KMTGuard][Settings] Invalid value for %s", szSettingName);
            return false;
        }

        // Ayar ismine göre struct alanına atama
        if (strcmp((char*)szSettingName, "SERVER_MAX_LEVEL") == 0)
        {
            CNewSettings::m_Settings->SERVER_MAX_LEVEL = atoi((char*)szValue);
            //BS_INFO("SettingName: %s, Value: %s", szSettingName, szValue);
        }
        if (strcmp((char*)szSettingName, "CH_MAX_MASTERY_LEVEL") == 0)
        {
            CNewSettings::m_Settings->CH_MAX_MASTERY_LEVEL = atoi((char*)szValue);
            //BS_INFO("SettingName: %s, Value: %s", szSettingName, szValue);
        }
        if (strcmp((char*)szSettingName, "EU_MAX_MASTERY_LEVEL") == 0)
        {
            CNewSettings::m_Settings->EU_MAX_MASTERY_LEVEL = atoi((char*)szValue);
            //BS_INFO("SettingName: %s, Value: %s", szSettingName, szValue);
        }
        if (strcmp((char*)szSettingName, "MIN_PK_LEVEL") == 0)
        {
            CNewSettings::m_Settings->MIN_PK_LEVEL = atoi((char*)szValue);
            //BS_INFO("SettingName: %s, Value: %s", szSettingName, szValue);
        }
        if (strcmp((char*)szSettingName, "STALL_EXCHANGE_GOLD_LIMIT") == 0)
        {
            CNewSettings::m_Settings->STALL_EXCHANGE_GOLD_LIMIT = my_atoll((char*)szValue);
            //BS_INFO("SettingName: %s, Value: %s", szSettingName, szValue);
        }
        if (strcmp((char*)szSettingName, "HIGH_RATES_CONFIG") == 0)
        {
            bool boolValue = (strcmp((char*)szValue, "1") == 0 || _stricmp((char*)szValue, "true") == 0);
            CNewSettings::m_Settings->HIGH_RATES_CONFIG = boolValue;
            //BS_INFO("SettingName: %s, Value: %s, BoolValue: %d", szSettingName, szValue, boolValue);
        }
        if (strcmp((char*)szSettingName, "PARTY_LEVEL_MIN") == 0)
        {
            CNewSettings::m_Settings->PARTY_LEVEL_MIN = atoi((char*)szValue);
            //BS_INFO("SettingName: %s, Value: %s", szSettingName, szValue);
        }
        if (strcmp((char*)szSettingName, "DisableDurability") == 0)
        {
            CNewSettings::m_Settings->DisableDurability =
                strcmp((char*)szValue, "1") == 0 || _stricmp((char*)szValue, "true") == 0;
        }
        if (strcmp((char*)szSettingName, "DisableGreenBook") == 0)
        {
            CNewSettings::m_Settings->DisableGreenBook =
                strcmp((char*)szValue, "1") == 0 || _stricmp((char*)szValue, "true") == 0;
        }
        if (strcmp((char*)szSettingName, "ShowGmUniqueKillNotice") == 0)
        {
            CNewSettings::m_Settings->ShowGmUniqueKillNotice =
                strcmp((char*)szValue, "1") == 0 || _stricmp((char*)szValue, "true") == 0;
        }
        if (strcmp((char*)szSettingName, "ForceGmVisibleOnSpawn") == 0)
        {
            CNewSettings::m_Settings->ForceGmVisibleOnSpawn =
                strcmp((char*)szValue, "1") == 0 || _stricmp((char*)szValue, "true") == 0;
        }
        if (strcmp((char*)szSettingName, "DisableOriginalTradeGold") == 0)
        {
            CNewSettings::m_Settings->DisableOriginalTradeGold =
                strcmp((char*)szValue, "1") == 0 || _stricmp((char*)szValue, "true") == 0;
        }
        if (strcmp((char*)szSettingName, "EnablePartyMonsterSpawn") == 0)
        {
            CNewSettings::m_Settings->EnablePartyMonsterSpawn =
                strcmp((char*)szValue, "1") == 0 || _stricmp((char*)szValue, "true") == 0;
        }
        if (strcmp((char*)szSettingName, "PartyMonsterMinimumMembers") == 0)
        {
            CNewSettings::m_Settings->PartyMonsterMinimumMembers = atoi((char*)szValue);
        }
        if (strcmp((char*)szSettingName, "PartyMonsterSpawnRate") == 0)
        {
            CNewSettings::m_Settings->PartyMonsterSpawnRate = atoi((char*)szValue);
        }
        if (strcmp((char*)szSettingName, "PENALTY_DROP_PROBABILITY") == 0)
        {
            CNewSettings::m_Settings->PENALTY_DROP_PROBABILITY = atoi((char*)szValue);
            //BS_INFO("SettingName: %s, Value: %s", szSettingName, szValue);
        }
        if (strcmp((char*)szSettingName, "RESURRECT_SAME_POINT_LEVEL_MAX") == 0)
        {
            CNewSettings::m_Settings->RESURRECT_SAME_POINT_LEVEL_MAX = atoi((char*)szValue);
            //BS_INFO("SettingName: %s, Value: %s", szSettingName, szValue);
        }
        if (strcmp((char*)szSettingName, "NPC_RETURN_DEAD_LEVEL_MAX") == 0)
        {
            CNewSettings::m_Settings->NPC_RETURN_DEAD_LEVEL_MAX = atoi((char*)szValue);
            //BS_INFO("SettingName: %s, Value: %s", szSettingName, szValue);
        }
        if (strcmp((char*)szSettingName, "BEGINNER_MARK_LEVEL_MAX") == 0)
        {
            CNewSettings::m_Settings->BEGINNER_MARK_LEVEL_MAX = atoi((char*)szValue);
            //BS_INFO("SettingName: %s, Value: %s", szSettingName, szValue);
        }
        if (strcmp((char*)szSettingName, "DROP_ITEM_MAGIC_PROBABILITY") == 0)
        {
            CNewSettings::m_Settings->DROP_ITEM_MAGIC_PROBABILITY = atoi((char*)szValue);
            //BS_INFO("SettingName: %s, Value: %s", szSettingName, szValue);
        }
        if (strcmp((char*)szSettingName, "PENALTY_DROP_LEVEL_MIN") == 0)
        {
            CNewSettings::m_Settings->PENALTY_DROP_LEVEL_MIN = atoi((char*)szValue);
            //BS_INFO("SettingName: %s, Value: %s", szSettingName, szValue);
        }
        if (strcmp((char*)szSettingName, "GRAP_PET_INVENTORY_SIZE") == 0)
        {
            CNewSettings::m_Settings->GRAP_PET_INVENTORY_SIZE = atoi((char*)szValue);
            //BS_INFO("SettingName: %s, Value: %s", szSettingName, szValue);
        }

        if (strcmp((char*)szSettingName, "FIX_EXPLOIT_INVISIBLE_INVINCIBLE") == 0)
        {
            bool boolValue = (strcmp((char*)szValue, "1") == 0 || _stricmp((char*)szValue, "true") == 0);
            CNewSettings::m_Settings->FIX_EXPLOIT_INVISIBLE_INVINCIBLE = boolValue;
            //BS_INFO("SettingName: %s, Value: %s, BoolValue: %d", szSettingName, szValue, boolValue);
        }
        if (strcmp((char*)szSettingName, "FIX_AGENT_SERVER_CAPACITY") == 0)
        {
            CNewSettings::m_Settings->FIX_AGENT_SERVER_CAPACITY = atoi((char*)szValue);
            //BS_INFO("SettingName: %s, Value: %s", szSettingName, szValue);
        }
        if (strcmp((char*)szSettingName, "EXCHANGE_ATTACK_CANCEL") == 0)
        {
            bool boolValue = (strcmp((char*)szValue, "1") == 0 || _stricmp((char*)szValue, "true") == 0);
            CNewSettings::m_Settings->EXCHANGE_ATTACK_CANCEL = boolValue;
            //BS_INFO("SettingName: %s, Value: %s, BoolValue: %d", szSettingName, szValue, boolValue);
        }
        if (strcmp((char*)szSettingName, "GUILD_POINTS") == 0)
        {
            bool boolValue = (strcmp((char*)szValue, "1") == 0 || _stricmp((char*)szValue, "true") == 0);
            CNewSettings::m_Settings->GUILD_POINTS = boolValue;
            // BS_INFO("SettingName: %s, Value: %s, BoolValue: %d", szSettingName, szValue, boolValue);
        }
        if (strcmp((char*)szSettingName, "FIX_GRAP_PET_PAGE") == 0)
        {
            bool boolValue = (strcmp((char*)szValue, "1") == 0 || _stricmp((char*)szValue, "true") == 0);
            CNewSettings::m_Settings->FIX_GRAP_PET_PAGE = boolValue;
            // BS_INFO("SettingName: %s, Value: %s, BoolValue: %d", szSettingName, szValue, boolValue);
        }
        if (strcmp((char*)szSettingName, "MEMBERS_LIMIT_LEVEL1") == 0)
        {
            CNewSettings::m_Settings->MEMBERS_LIMIT_LEVEL1 = atoi((char*)szValue);
            // BS_INFO("SettingName: %s, Value: %s", szSettingName, szValue);
        }

        if (strcmp((char*)szSettingName, "MEMBERS_LIMIT_LEVEL2") == 0)
        {
            CNewSettings::m_Settings->MEMBERS_LIMIT_LEVEL2 = atoi((char*)szValue);
            //  BS_INFO("SettingName: %s, Value: %s", szSettingName, szValue);
        }

        if (strcmp((char*)szSettingName, "MEMBERS_LIMIT_LEVEL3") == 0)
        {
            CNewSettings::m_Settings->MEMBERS_LIMIT_LEVEL3 = atoi((char*)szValue);
            //   BS_INFO("SettingName: %s, Value: %s", szSettingName, szValue);
        }
        if (strcmp((char*)szSettingName, "MEMBERS_LIMIT_LEVEL4") == 0)
        {
            CNewSettings::m_Settings->MEMBERS_LIMIT_LEVEL4 = atoi((char*)szValue);
            // BS_INFO("SettingName: %s, Value: %s", szSettingName, szValue);
        }
        if (strcmp((char*)szSettingName, "MEMBERS_LIMIT_LEVEL5") == 0)
        {
            CNewSettings::m_Settings->MEMBERS_LIMIT_LEVEL5 = atoi((char*)szValue);
            // BS_INFO("SettingName: %s, Value: %s", szSettingName, szValue);
        }
        if (strcmp((char*)szSettingName, "STORAGE_SLOTS_MIN") == 0)
        {
            CNewSettings::m_Settings->STORAGE_SLOTS_MIN = atoi((char*)szValue);
            //   BS_INFO("SettingName: %s, Value: %s", szSettingName, szValue);
        }
        if (strcmp((char*)szSettingName, "STORAGE_SLOTS_INCREASE") == 0)
        {
            CNewSettings::m_Settings->STORAGE_SLOTS_INCREASE = atoi((char*)szValue);
            //BS_INFO("SettingName: %s, Value: %s", szSettingName, szValue);
        }

        if (strcmp((char*)szSettingName, "UNION_LIMIT") == 0)
        {
            CNewSettings::m_Settings->UNION_LIMIT = atoi((char*)szValue);
            //BS_INFO("SettingName: %s, Value: %s", szSettingName, szValue);
        }
        if (strcmp((char*)szSettingName, "UNION_CHAT_PARTICIPANTS") == 0)
        {
            CNewSettings::m_Settings->UNION_CHAT_PARTICIPANTS = atoi((char*)szValue);
            //BS_INFO("SettingName: %s, Value: %s", szSettingName, szValue);
        }
        if (strcmp((char*)szSettingName, "MIN_GUILD_LEVEL_FOR_MERCENARY_SPAWN") == 0)
        {
            CNewSettings::m_Settings->MIN_GUILD_LEVEL_FOR_MERCENARY_SPAWN = atoi((char*)szValue);
            //BS_INFO("SettingName: %s, Value: %s", szSettingName, szValue);
        }

        if (strcmp((char*)szSettingName, "ALLOW_NON_GM_MERCENARY_SPAWN") == 0)
        {
            bool boolValue = (strcmp((char*)szValue, "1") == 0 || _stricmp((char*)szValue, "true") == 0);
            CNewSettings::m_Settings->ALLOW_NON_GM_MERCENARY_SPAWN = boolValue;
            //BS_INFO("SettingName: %s, Value: %s, BoolValue: %d", szSettingName, szValue, boolValue);
        }

        if (strcmp((char*)szSettingName, "DISABLE_GRANT_NAME_CONDITIONS") == 0)
        {
            bool boolValue = (strcmp((char*)szValue, "1") == 0 || _stricmp((char*)szValue, "true") == 0);
            CNewSettings::m_Settings->DISABLE_GRANT_NAME_CONDITIONS = boolValue;
            //BS_INFO("SettingName: %s, Value: %s, BoolValue: %d", szSettingName, szValue, boolValue);
        }

        if (strcmp((char*)szSettingName, "ALCHEMY_FUSING_DELAY") == 0)
        {
            CNewSettings::m_Settings->ALCHEMY_FUSING_DELAY = atoi((char*)szValue);
            //BS_INFO("SettingName: %s, Value: %s", szSettingName, szValue);
        }
        if (strcmp((char*)szSettingName, "CTF_ITEM_WIN_REWARD") == 0)
        {
            CNewSettings::m_Settings->CTF_ITEM_WIN_REWARD = ((char*)szValue);
            //BS_INFO("SettingName: %s, Value: %s", szSettingName, szValue);
        }

        if (strcmp((char*)szSettingName, "CTF_ITEM_WIN_REWARD_AMOUNT") == 0)
        {
            CNewSettings::m_Settings->CTF_ITEM_WIN_REWARD_AMOUNT = atoi((char*)szValue);
            //BS_INFO("SettingName: %s, Value: %s", szSettingName, szValue);
        }
        if (strcmp((char*)szSettingName, "CTF_ITEM_KILL_REWARD") == 0)
        {
            CNewSettings::m_Settings->CTF_ITEM_KILL_REWARD = ((char*)szValue);
            // BS_INFO("SettingName: %s, Value: %s", szSettingName, szValue);
        }

        if (strcmp((char*)szSettingName, "CTF_ITEM_KILL_REWARD_AMOUNT") == 0)
        {
            CNewSettings::m_Settings->CTF_ITEM_KILL_REWARD_AMOUNT = atoi((char*)szValue);
            //BS_INFO("SettingName: %s, Value: %s", szSettingName, szValue);
        }
        if (strcmp((char*)szSettingName, "BA_ITEM_REWARD") == 0)
        {
            CNewSettings::m_Settings->BA_ITEM_REWARD = ((char*)szValue);
            // BS_INFO("SettingName: %s, Value: %s", szSettingName, szValue);
        }

        if (strcmp((char*)szSettingName, "BA_ITEM_REWARD_GJ_W_AMOUNT") == 0)
        {
            CNewSettings::m_Settings->BA_ITEM_REWARD_GJ_W_AMOUNT = atoi((char*)szValue);
            // BS_INFO("SettingName: %s, Value: %s", szSettingName, szValue);
        }
        if (strcmp((char*)szSettingName, "BA_ITEM_REWARD_GJ_L_AMOUNT") == 0)
        {
            CNewSettings::m_Settings->BA_ITEM_REWARD_GJ_L_AMOUNT = atoi((char*)szValue);
            //   BS_INFO("SettingName: %s, Value: %s", szSettingName, szValue);
        }

        if (strcmp((char*)szSettingName, "BA_ITEM_REWARD_PR_W_AMOUNT") == 0)
        {
            CNewSettings::m_Settings->BA_ITEM_REWARD_PR_W_AMOUNT = atoi((char*)szValue);
            //  BS_INFO("SettingName: %s, Value: %s", szSettingName, szValue);
        }

        if (strcmp((char*)szSettingName, "BA_ITEM_REWARD_PR_L_AMOUNT") == 0)
        {
            CNewSettings::m_Settings->BA_ITEM_REWARD_PR_L_AMOUNT = atoi((char*)szValue);
            //  BS_INFO("SettingName: %s, Value: %s", szSettingName, szValue);
        }

        if (strcmp((char*)szSettingName, "MinItemLevelForAstralToTakeEffect") == 0)
        {
            CNewSettings::m_Settings->MinItemLevelForAstralToTakeEffect = atoi((char*)szValue);
            //  BS_INFO("SettingName: %s, Value: %s", szSettingName, szValue);
        }
        if (strcmp((char*)szSettingName, "ItemLevelForAstralRecovery") == 0)
        {
            CNewSettings::m_Settings->ItemLevelForAstralRecovery = atoi((char*)szValue);
            //  BS_INFO("SettingName: %s, Value: %s", szSettingName, szValue);
        }

        if (strcmp((char*)szSettingName, "JOB_LEVEL_MAX") == 0)
        {
            CNewSettings::m_Settings->JOB_LEVEL_MAX = atoi((char*)szValue);
            // BS_INFO("SettingName: %s, Value: %s", szSettingName, szValue);
        }
        if (strcmp((char*)szSettingName, "DISABLE_MOB_SPAWN_WHILE_TRADE") == 0)
        {
            bool boolValue = (strcmp((char*)szValue, "1") == 0 || _stricmp((char*)szValue, "true") == 0);
            CNewSettings::m_Settings->DISABLE_MOB_SPAWN_WHILE_TRADE = boolValue;
            //  BS_INFO("SettingName: %s, Value: %s, BoolValue: %d", szSettingName, szValue, boolValue);
        }
        if (strcmp((char*)szSettingName, "JOB_TEMPLE_LEVEL") == 0)
        {
            CNewSettings::m_Settings->TEMPLE_LEVEL = atoi((char*)szValue);
            //  BS_INFO("SettingName: %s, Value: %s", szSettingName, szValue);
        }
        retCode = SQLFetch(hStmt);
    } while (SQL_SUCCEEDED(retCode));

    return retCode == SQL_NO_DATA;
}
typedef unsigned char uint8_t;
typedef unsigned short uint16_t;
typedef unsigned int uint32_t;

bool CSqlCon::ApplyRuntimeSettings()
{
    GameServerMemoryPatchTransaction transaction;

    // This build stores the European cap as an eight-byte floating-point
    // value and applies a second, independent 2*character-level check. Check
    // the exact host instructions before changing either one.
    const BYTE chineseCapOpcode[] = {0x3D};
    const BYTE europeanCapRead[] = {0xDC, 0x1D, 0x30, 0x61, 0xB4, 0x00};
    const BYTE europeanLevelCheck[] = {0x8B, 0xC6, 0xE8, 0x2E, 0x22, 0x00, 0x00};
    const BYTE europeanLevelBypass[] = {0xE9, 0x5D, 0x00, 0x00, 0x00, 0x90, 0x90};
    const bool levelCheckSupported =
        GameServerRuntimeSafety::MatchesBytes(0x0059C58B, europeanLevelCheck, sizeof(europeanLevelCheck)) ||
        GameServerRuntimeSafety::MatchesBytes(0x0059C58B, europeanLevelBypass, sizeof(europeanLevelBypass));
    if (!GameServerRuntimeSafety::MatchesBytes(0x0059C5E6, chineseCapOpcode, sizeof(chineseCapOpcode)) ||
        !GameServerRuntimeSafety::MatchesBytes(0x0059C57E, europeanCapRead, sizeof(europeanCapRead)) ||
        !levelCheckSupported)
    {
        BS_INFO("[KMTGuard][Settings] Mastery patch validation failed; runtime settings were not changed");
        return false;
    }
    double europeanMasteryValue = 0.0;
    if (!ReadMemoryValue<double>(0x00B46130, europeanMasteryValue))
    {
        BS_INFO("[KMTGuard][Settings] European mastery value could not be read; runtime settings were not changed");
        return false;
    }

#define WriteMemoryValue transaction.WriteValue

    // Keep the operator console concise: the transaction and validation
    // layers report real failures separately, while the successful old/new
    // value dump for every setting is intentionally hidden.
#pragma push_macro("BS_INFO")
#undef BS_INFO
#define BS_INFO(...) ((void)0)
    //PK
    uint8_t byteValue;
    uint32_t uintValue;

    // Server
    if (ReadMemoryValue<uint8_t>(0x004E52C7 + 2, byteValue))
    {
        uint8_t newValue = CNewSettings::m_Settings->SERVER_MAX_LEVEL;
        BS_INFO("SERVER_LEVEL_MAX WITH PET (%d) -> (%d)", byteValue, newValue);
        WriteMemoryValue<uint8_t>(0x004E52C7 + 2, newValue); // Character
        WriteMemoryValue<uint8_t>(0x004D641B + 3, newValue); // Pet
        WriteMemoryValue<uint16_t>(0x004E5471 + 4, (newValue - 1) * 4); // Exp bug fix
    }

    if (ReadMemoryValue<uint32_t>(0x0059C5E6 + 1, uintValue))
    {
        uint32_t newValue = CNewSettings::m_Settings->CH_MAX_MASTERY_LEVEL;
        BS_INFO("RACE_CH_TOTAL_MASTERIES (%u) -> (%u)", uintValue, newValue);
        WriteMemoryValue<uint32_t>(0x0059C5E6 + 1, newValue);
    }


    const double newEuropeanMasteryValue = static_cast<double>(CNewSettings::m_Settings->EU_MAX_MASTERY_LEVEL);
    BS_INFO("RACE_EU_TOTAL_MASTERIES (%g) -> (%g)", europeanMasteryValue, newEuropeanMasteryValue);
    WriteMemoryValue<double>(0x00B46130, newEuropeanMasteryValue, "European mastery total");
    transaction.WriteRaw(0x0059C58B, europeanLevelBypass,
        sizeof(europeanLevelBypass), "European total versus character level");


    if (ReadMemoryValue<uint8_t>(0x005295DA + 1, byteValue))
    {
        uint8_t newValue = CNewSettings::m_Settings->MIN_PK_LEVEL;
        BS_INFO("SERVER_PK_LEVEL_REQUIRED (%d) -> (%d)", byteValue, newValue);
        WriteMemoryValue<uint8_t>(0x005295DA + 1, newValue);
    }
    if (ReadMemoryValue<uint8_t>(0x004E6A33 + 1, byteValue))
    {
        uint8_t newValue = CNewSettings::m_Settings->PENALTY_DROP_LEVEL_MIN;
        BS_INFO("SERVER_PENALTY_DROP_LEVEL_MIN (%d) -> (%d)", byteValue, newValue);
        WriteMemoryValue<uint8_t>(0x004E6A33 + 1, newValue);
    }


    if (ReadMemoryValue<uint8_t>(0x00471B00 + 2, byteValue) && ReadMemoryValue<uint32_t>(0x00471B07 + 1, uintValue))
    {
        unsigned __int64 newValue = CNewSettings::m_Settings->STALL_EXCHANGE_GOLD_LIMIT;
        newValue = newValue & 0xFFFFFFFFFF; // Limit value to 5 bytes
        BS_INFO("SERVER_STALL_PRICE_LIMIT (%llu) -> (%llu)", ((unsigned __int64)byteValue << 32) | uintValue, newValue);
        const uint8_t stallHighByte = static_cast<uint8_t>(newValue >> 32);
        const uint32_t stallLowBytes = static_cast<uint32_t>(newValue);
        // Stall
        WriteMemoryValue<uint8_t>(0x00471B00 + 2, stallHighByte);
        WriteMemoryValue<uint32_t>(0x00471B07 + 1, stallLowBytes);
        WriteMemoryValue<uint8_t>(0x00472FF5 + 2, stallHighByte);
        WriteMemoryValue<uint32_t>(0x00473008 + 1, stallLowBytes);
        WriteMemoryValue<uint8_t>(0x0047ABD8 + 2, stallHighByte);
        WriteMemoryValue<uint32_t>(0x0047ABE3 + 1, stallLowBytes);
        // Exchange will take the highest UX value
        if (ReadMemoryValue<uint32_t>(0x00480F5E + 4, uintValue))
        {
            const uint32_t exchangeLimit = static_cast<uint32_t>(
                newValue > 4000000000ULL ? 4000000000ULL : newValue);
            BS_INFO("SERVER_EXCHANGE_GOLD_LIMIT (%u) -> (%u)", uintValue, exchangeLimit);
            WriteMemoryValue<uint32_t>(0x00480F5E + 4, exchangeLimit);
            WriteMemoryValue<uint32_t>(0x004D8F1A + 2, exchangeLimit);
            WriteMemoryValue<uint32_t>(0x004D8F22 + 2, exchangeLimit);
            WriteMemoryValue<uint32_t>(0x004F7734 + 2, exchangeLimit);
            WriteMemoryValue<uint32_t>(0x004F7746 + 4, exchangeLimit);
        }
    }



    if (CNewSettings::m_Settings->HIGH_RATES_CONFIG == true)
    {
        BS_INFO("FIX_HIGH_RATES_CONFIG -> (%s)", "True");
        WriteMemoryValue<uint8_t>(0x0042714C + 2, 0x42); // ExpRatio
        WriteMemoryValue<uint8_t>(0x004271F5 + 2, 0x42); // ExpRatioParty
        WriteMemoryValue<uint8_t>(0x004272A0 + 2, 0x42); // DropItemRatio
        WriteMemoryValue<uint8_t>(0x00427349 + 2, 0x42); // DropGoldAmountCoef
    }

    if (ReadMemoryValue<uint8_t>(0x00513FEC + 1, byteValue))
    {
        uint8_t newValue = CNewSettings::m_Settings->PARTY_LEVEL_MIN;
        BS_INFO("SERVER_PARTY_LEVEL_MIN (%d) -> (%d)", byteValue, newValue);
        WriteMemoryValue<uint8_t>(0x00513FEC + 1, newValue);
    }

    if (ReadMemoryValue<uint8_t>(0x004E696D + 1, byteValue))
    {
        uint8_t newValue = CNewSettings::m_Settings->PENALTY_DROP_PROBABILITY;
        BS_INFO("SERVER_PENALTY_DROP_PROBABILITY (%d) -> (%d)", byteValue, newValue);
        WriteMemoryValue<uint8_t>(0x004E696D + 1, newValue);
    }
    if (ReadMemoryValue<uint8_t>(0x0051017F + 1, byteValue))
    {
        uint8_t newValue = CNewSettings::m_Settings->RESURRECT_SAME_POINT_LEVEL_MAX;
        BS_INFO("SERVER_RESURRECT_SAME_POINT_LEVEL_MAX (%d) -> (%d)", byteValue, newValue);
        WriteMemoryValue<uint8_t>(0x0051017F + 1, newValue);
    }
    if (ReadMemoryValue<uint8_t>(0x004F36F3 + 1, byteValue))
    {
        uint8_t newValue = CNewSettings::m_Settings->NPC_RETURN_DEAD_LEVEL_MAX;
        BS_INFO("SERVER_NPC_RETURN_DEAD_LEVEL_MAX (%d) -> (%d)", byteValue, newValue);
        WriteMemoryValue<uint8_t>(0x004F36F3 + 1, newValue);
    }
    if (ReadMemoryValue<uint8_t>(0x004E4F0F + 4, byteValue))
    {
        uint8_t newValue = CNewSettings::m_Settings->BEGINNER_MARK_LEVEL_MAX;
        BS_INFO("SERVER_BEGINNER_MARK_LEVEL_MAX (%d) -> (%d)", byteValue, newValue);
        WriteMemoryValue<uint8_t>(0x004E4F0F + 4, newValue);
        WriteMemoryValue<uint8_t>(0x00518B99 + 3, newValue);
    }
    if (ReadMemoryValue<uint8_t>(0x00727784 + 2, byteValue))
    {
        uint8_t newValue = CNewSettings::m_Settings->DROP_ITEM_MAGIC_PROBABILITY;
        BS_INFO("SERVER_DROP_ITEM_MAGIC_PROBABILITY (%d) -> (%d)", byteValue, newValue);
        WriteMemoryValue<uint8_t>(0x00727784 + 2, newValue);
    }

    if (ReadMemoryValue<uint32_t>(0x004D6F9F, uintValue))
    {
        uint32_t newValue = CNewSettings::m_Settings->GRAP_PET_INVENTORY_SIZE;
        BS_INFO("GRAB_PET_INVENTORY_SIZE (%d) -> (%d)", uintValue, newValue);
        WriteMemoryValue<uint32_t>(0x004D6F9F, newValue);
    }

    if (CNewSettings::m_Settings->FIX_EXPLOIT_INVISIBLE_INVINCIBLE ==  true)
    {
        BS_INFO("FIX_EXPLOIT_INVISIBLE_INVINCIBLE -> (%s)", "True");
        for (int i = 0; i < 2; i++)
            WriteMemoryValue<uint8_t>(0x00515B78 + i, 0x90); // NOP jnz
    }
    if (ReadMemoryValue<uint32_t>(0x004744BC + 1, uintValue))
    {
        uint32_t newValue = CNewSettings::m_Settings->FIX_AGENT_SERVER_CAPACITY;
        BS_INFO("FIX_AGENT_SERVER_CAPACITY (%u) -> (%u)", uintValue, newValue);
        WriteMemoryValue<uint32_t>(0x004744BC + 1, newValue);
        WriteMemoryValue<uint32_t>(0x004744C7 + 1, newValue);
    }

    if (CNewSettings::m_Settings->EXCHANGE_ATTACK_CANCEL)
    {
        BS_INFO("FIX_EXCHANGE_ATTACK_CANCEL");
        for (int i = 0; i < 2; i++)
            WriteMemoryValue<uint8_t>(0x00515578 + i, 0x90); // NOP call
    }

    if (CNewSettings::m_Settings->FIX_GRAP_PET_PAGE == true)
    {
        BYTE btPayload = 0xEB;
        BS_INFO("FIX_GRAP_PET_PAGE -> (True)");
        WriteMemoryValue<uint8_t>(0x004FBD8A, btPayload);
    }






    // Fix








    // Job

    {
        uint32_t addr = 0x00ADE8DC;
        if (ReadMemoryValue<uint32_t>(addr, uintValue))
        {
            uint32_t newValue = CNewSettings::m_Settings->MEMBERS_LIMIT_LEVEL1;
            BS_INFO("GUILD_MEMBERS_LIMIT_LEVEL1 (%u) -> (%u)", uintValue, newValue);
            WriteMemoryValue<uint32_t>(addr, newValue);
        }
        if (ReadMemoryValue<uint32_t>(addr + 4, uintValue))
        {
            uint32_t newValue = CNewSettings::m_Settings->MEMBERS_LIMIT_LEVEL2;
            BS_INFO("GUILD_MEMBERS_LIMIT_LEVEL2 (%u) -> (%u)", uintValue, newValue);
            WriteMemoryValue<uint32_t>(addr + 4, newValue);
        }
        if (ReadMemoryValue<uint32_t>(addr + 8, uintValue))
        {
            uint32_t newValue = CNewSettings::m_Settings->MEMBERS_LIMIT_LEVEL3;
            BS_INFO("GUILD_MEMBERS_LIMIT_LEVEL3 (%u) -> (%u)", uintValue, newValue);
            WriteMemoryValue<uint32_t>(addr + 8, newValue);
        }
        if (ReadMemoryValue<uint32_t>(addr + 12, uintValue))
        {
            uint32_t newValue = CNewSettings::m_Settings->MEMBERS_LIMIT_LEVEL4;
            BS_INFO("GUILD_MEMBERS_LIMIT_LEVEL4 (%u) -> (%u)", uintValue, newValue);
            WriteMemoryValue<uint32_t>(addr + 12, newValue);
        }
        if (ReadMemoryValue<uint32_t>(addr + 16, uintValue))
        {
            uint32_t newValue = CNewSettings::m_Settings->MEMBERS_LIMIT_LEVEL5;
            BS_INFO("GUILD_MEMBERS_LIMIT_LEVEL5 (%u) -> (%u)", uintValue, newValue);
            WriteMemoryValue<uint32_t>(addr + 16, newValue);
        }
    }

    if (ReadMemoryValue<uint32_t>(0x00C6B5F8, uintValue))
    {
        uint32_t newValue = CNewSettings::m_Settings->STORAGE_SLOTS_MIN;
        BS_INFO("GUILD_STORAGE_SLOTS_MIN (%d) -> (%d)", uintValue, newValue);
        WriteMemoryValue<uint32_t>(0x00C6B5F8, newValue);
        // Get value increased on second level
        uint32_t increaseValue;
        if (ReadMemoryValue<uint32_t>(0x00C6B5F8 + 4, increaseValue))
        {
            uint32_t increaseNewValue = CNewSettings::m_Settings->STORAGE_SLOTS_INCREASE;
            BS_INFO("GUILD_STORAGE_SLOTS_INCREASE (%d) -> (%d)", increaseValue - uintValue, increaseNewValue);
            for (int i = 0; i < 3; i++)
                WriteMemoryValue<uint32_t>(0x00C6B5F8 + 4 + (i * 4), newValue + (i + 1) * increaseNewValue);
        }
    }
    if (ReadMemoryValue<uint8_t>(0x005B8EA1 + 1, byteValue))
    {
        uint8_t newValue = CNewSettings::m_Settings->UNION_LIMIT;
        BS_INFO("GUILD_UNION_LIMIT (%d) -> (%d)", byteValue, newValue);
        WriteMemoryValue<uint8_t>(0x005B8EA1 + 1, newValue);
    }
    if (ReadMemoryValue<uint8_t>(0x005C4B42 + 4, byteValue))
    {
        uint8_t newValue = CNewSettings::m_Settings->UNION_CHAT_PARTICIPANTS;
        BS_INFO("GUILD_UNION_CHAT_PARTICIPANTS (%d) -> (%d)", byteValue, newValue);
        WriteMemoryValue<uint8_t>(0x005C4B42 + 4, newValue);
    }
    if (ReadMemoryValue<uint8_t>(0x004FD02B, byteValue))
    {
        uint8_t newValue = CNewSettings::m_Settings->MIN_GUILD_LEVEL_FOR_MERCENARY_SPAWN;
        BS_INFO("MIN_GUILD_LEVEL_FOR_MERCENARY_SPAWN (%d) -> (%d)", byteValue, newValue);
        WriteMemoryValue<uint8_t>(0x004FD02B, newValue);
    }
    if (CNewSettings::m_Settings->ALLOW_NON_GM_MERCENARY_SPAWN)
    {
        //short jmp, skip check if summoner is guild master
        BYTE btInstruction = 0xEB;
        WriteMemoryValue(0x004FD045, btInstruction);
    }
    if (CNewSettings::m_Settings->DISABLE_GRANT_NAME_CONDITIONS)
    {
        //Short jmp, skip check if guild level is >= 4 and the player is guild master
        BYTE btInstruction = 0xEB;
        WriteMemoryValue<uint8_t>(0x00517C20, btInstruction);
        WriteMemoryValue<uint8_t>(0x005C75AB, btInstruction);

        BS_INFO(" - DISABLE_GULD_NAMES -> (%d)", btInstruction);
    }

    // Alche
    if (ReadMemoryValue<uint8_t>(0x0052ADAA + 6, byteValue))
    {
        uint8_t newValue = CNewSettings::m_Settings->ALCHEMY_FUSING_DELAY;
        BS_INFO("ALCHEMY_FUSING_DELAY (%d) -> (%d)", byteValue, newValue);
        WriteMemoryValue<uint8_t>(0x0052ADAA + 6, newValue);
    }
    if (ReadMemoryValue<uint8_t>(0x00506D92 + 2, byteValue))
    {
        uint8_t newValue = CNewSettings::m_Settings->MinItemLevelForAstralToTakeEffect;
        BS_INFO("ALCHEMY_ASTRAL_MIN_ITEM_LEVEL (%d) -> (%d)", byteValue, newValue);
        WriteMemoryValue<uint8_t>(0x00506D92 + 2, newValue);
    }
    if (ReadMemoryValue<uint8_t>(0x00506DD2 + 1, byteValue))
    {
        uint8_t newValue = CNewSettings::m_Settings->ItemLevelForAstralRecovery;
        BS_INFO("ALCHEMY_ASTRAL_RECOVERY_LEVEL (%d) -> (%d)", byteValue, newValue);
        WriteMemoryValue<uint8_t>(0x00506DD2 + 1, newValue);
    }

    LPCSTR lpszCodename = NULL;
    // Event
    {
        std::string currentValue = ReadMemoryString(0x00646D43);
        if (!currentValue.empty())
        {
            size_t newValueLen = CNewSettings::m_Settings->CTF_ITEM_WIN_REWARD.size();
            // Check value it's not empty and shorter than 128 bytes
            if (newValueLen != 0 && newValueLen <= 128)
            {
                BS_INFO("EVENT_CTF_ITEM_WIN_REWARD (%s) -> (%s)", currentValue.c_str(), CNewSettings::m_Settings->CTF_ITEM_WIN_REWARD.c_str());
                // Set char* pointer to the new value
                //WriteMemoryValue<uint32_t>(0x00646D43, (uint32_t)m_Settings->CTF_ITEM_WIN_REWARD.c_str()); // Winning Reward
                lpszCodename = static_cast<LPCSTR>(CNewSettings::m_Settings->CTF_ITEM_WIN_REWARD.c_str());
                transaction.WriteValue<LPCSTR>(0x00646D43, lpszCodename, "CTF win reward");

                //WriteMemoryValue<uint32_t>(0x00876935 + 6, (uint32_t)m_Settings->CTF_ITEM_WIN_REWARD.c_str()); // Just in case, something about Quest reward required probably
            }
        }
    }
    if (ReadMemoryValue<uint8_t>(0x00646D41, byteValue))
    {
        uint8_t newValue = CNewSettings::m_Settings->CTF_ITEM_WIN_REWARD_AMOUNT;
        BS_INFO("EVENT_CTF_ITEM_WIN_REWARD_AMOUNT (%d) -> (%d)", byteValue, newValue);
        WriteMemoryValue<uint8_t>(0x00646D40 + 1, newValue);
    }
    {
        std::string currentValue = ReadMemoryString(0x005F19AA);
        if (!currentValue.empty())
        {

            size_t newValueLen = CNewSettings::m_Settings->CTF_ITEM_KILL_REWARD.size();
            // Check value it's not empty and shorter than 128 bytes
            if (newValueLen != 0 && newValueLen <= 128)
            {
                BS_INFO("EVENT_CTF_ITEM_KILL_REWARD (%s) -> (%s)", currentValue.c_str(), CNewSettings::m_Settings->CTF_ITEM_KILL_REWARD.c_str());
                // Set char* pointer to the new value
                // WriteMemoryValue<uint32_t>(0x005F19AA, (uint32_t)m_Settings->CTF_ITEM_KILL_REWARD.c_str()); // Killing Reward
                lpszCodename = static_cast<LPCSTR>(CNewSettings::m_Settings->CTF_ITEM_KILL_REWARD.c_str());
                transaction.WriteValue<LPCSTR>(0x005F19AA, lpszCodename, "CTF kill reward");

            }
        }
    }
    if (ReadMemoryValue<uint8_t>(0x005F1998, byteValue))
    {
        uint8_t newValue = CNewSettings::m_Settings->CTF_ITEM_KILL_REWARD_AMOUNT;
        BS_INFO("EVENT_CTF_ITEM_KILL_REWARD_AMOUNT (%d) -> (%d)", byteValue, newValue);
        WriteMemoryValue<uint8_t>(0x005F1998, newValue);
    }
    {
        std::string currentValue = ReadMemoryString(0x006691C6 + 1);
        if (!currentValue.empty())
        {
            size_t newValueLen = CNewSettings::m_Settings->BA_ITEM_REWARD.size();
            // Check value it's not empty and shorter than 128 bytes
            if (newValueLen != 0 && newValueLen <= 128)
            {
                BS_INFO("EVENT_BA_ITEM_REWARD (%s) -> (%s)", currentValue.c_str(), CNewSettings::m_Settings->BA_ITEM_REWARD.c_str());
                // Set char* pointer to the new value
                WriteMemoryValue<uint32_t>(0x006691C6 + 1, (uint32_t)CNewSettings::m_Settings->BA_ITEM_REWARD.c_str());
            }
        }
    }
    if (ReadMemoryValue<uint8_t>(0x00669158 + 4, byteValue))
    {
        uint8_t newValue = CNewSettings::m_Settings->BA_ITEM_REWARD_GJ_W_AMOUNT;
        BS_INFO("EVENT_BA_ITEM_REWARD_GJ_W_AMOUNT (%d) -> (%d)", byteValue, newValue);
        WriteMemoryValue<uint8_t>(0x00669158 + 4, newValue);
    }
    if (ReadMemoryValue<uint8_t>(0x00669173 + 4, byteValue))
    {
        uint8_t newValue = CNewSettings::m_Settings->BA_ITEM_REWARD_GJ_L_AMOUNT;
        BS_INFO("EVENT_BA_ITEM_REWARD_GJ_L_AMOUNT (%d) -> (%d)", byteValue, newValue);
        WriteMemoryValue<uint8_t>(0x00669173 + 4, newValue);
    }
    if (ReadMemoryValue<uint8_t>(0x0066915F + 4, byteValue))
    {
        uint8_t newValue = CNewSettings::m_Settings->BA_ITEM_REWARD_PR_W_AMOUNT;
        BS_INFO("EVENT_BA_ITEM_REWARD_PR_W_AMOUNT (%d) -> (%d)", byteValue, newValue);
        WriteMemoryValue<uint8_t>(0x0066915F + 4, newValue);
    }
    if (ReadMemoryValue<uint8_t>(0x0066917A + 4, byteValue))
    {
        uint8_t newValue = CNewSettings::m_Settings->BA_ITEM_REWARD_PR_L_AMOUNT;
        BS_INFO("EVENT_BA_ITEM_REWARD_PR_L_AMOUNT (%d) -> (%d)", byteValue, newValue);
        WriteMemoryValue<uint8_t>(0x0066917A + 4, newValue);
    }

    if (ReadMemoryValue<uint8_t>(0x0060DE69 + 3, byteValue))
    {
        uint8_t newValue = CNewSettings::m_Settings->JOB_LEVEL_MAX;
        BS_INFO("JOB_LEVEL_MAX (%d) -> (%d)", byteValue, newValue);
        WriteMemoryValue<uint8_t>(0x0060DE69 + 3, newValue);
    }
    if (CNewSettings::m_Settings->DISABLE_MOB_SPAWN_WHILE_TRADE == true)
    {
        BS_INFO("JOB_DISABLE_MOB_SPAWN -> (%s)", "True");
        WriteMemoryValue<uint16_t>(0x0060C4AB, 0xC031); // mov eax,esi -> xor eax,eax
    }
    if (ReadMemoryValue<uint8_t>(0x0051AE71 + 1, byteValue))
    {
        uint8_t newValue = CNewSettings::m_Settings->TEMPLE_LEVEL;
        BS_INFO("JOB_TEMPLE_LEVEL (%d) -> (%d)", byteValue, newValue);
        WriteMemoryValue<uint8_t>(0x0051AE71 + 1, newValue);
        WriteMemoryValue<uint8_t>(0x0051ABE8 + 1, newValue);
    }
    //tf("---Initialized Settings----\n \n");
#pragma pop_macro("BS_INFO")
#undef WriteMemoryValue
    if (!transaction.Commit())
    {
        BS_INFO("[KMTGuard][Settings] Runtime configuration rolled back");
        return false;
    }
    BS_INFO("[KMTGuard][Settings] Runtime configuration applied atomically");
    return true;
}

CRegionRestrictionDBSet* CSqlCon::GetRegionRestrictionDbSet()
{
    return s_pRegionRestrictionDbSet;
}
bool CSqlCon::LoadFortressDPSInfo(CDbConnection* connection, bool workerStatement)
{
    CDbConnection* database = connection != NULL ? connection : m_connectionstr;
    if (database == NULL)
        return false;

    std::map<int, _ServerFortressDpsInfo> staged;
    ScopedSqlConnectionLock sqlGuard(s_sqlConnectionLock, connection == NULL);
    ScopedSqlStatement statement(database, workerStatement);
    if (!statement.Allocate())
        return false;

    const char* query =
        "SELECT StructObjID FROM [KMTGuard].[dbo].[_ServerFortressDpsInfo] WHERE Enabled = 1";
    SQLRETURN result = SQLExecDirectA(statement.Get(), (SQLCHAR*)query, SQL_NTS);
    if (!SQL_SUCCEEDED(result))
        return false;

    while ((result = SQLFetch(statement.Get())) != SQL_NO_DATA)
    {
        if (!SQL_SUCCEEDED(result))
            return false;
        SQLINTEGER objectId = 0;
        SQLINTEGER length = 0;
        if (!SQL_SUCCEEDED(SQLGetData(statement.Get(), 1, SQL_C_LONG, &objectId, 0, &length)) ||
            length == SQL_NULL_DATA || objectId <= 0)
            return false;
        _ServerFortressDpsInfo record;
        record.ObjID = objectId;
        staged[record.ObjID] = record;
    }

    ScopedWinCriticalSection cacheGuard(s_fortressDpsLock);
    ServerFortressDpsInfo.swap(staged);
    return true;
}

bool CSqlCon::IsFortressDpsEnabled(int structObjId)
{
    if (structObjId <= 0)
        return false;
    ScopedWinCriticalSection guard(s_fortressDpsLock);
    return ServerFortressDpsInfo.find(structObjId) != ServerFortressDpsInfo.end();
}

bool CSqlCon::ServerAutoCapebyWorldID()
{
    CAutoCriticalSection* m_pACS = &s_sqlConnectionLock;

    SQLHANDLE hStmt = SQL_NULL_HSTMT;
    CDbConnection* m_pDbConnection = m_connectionstr;

    if (!m_pDbConnection->AllocStmt(hStmt))
    {
        std::cout << __FUNCTION__ << " - Failed to allocate stmt" << std::endl;
        m_pDbConnection->FreeStmt(hStmt);
        return false;
    }

    SQLRETURN retCode;
    const LPCSTR szQuery = "SELECT ID, WorldID FROM _ServerAutoCapebyWorldID";

    m_pACS->Enter();

    retCode = SQLExecDirectA(hStmt, (SQLCHAR*)szQuery, SQL_NTS);
    if (!SQL_SUCCEEDED(retCode))
    {
        std::cout << __FUNCTION__ << " - SQLExecDirect failed, query = " << szQuery << std::endl;
        m_pDbConnection->FreeStmt(hStmt);
        m_pACS->Leave();
        return false;
    }

    retCode = SQLFetch(hStmt);

    if (retCode == SQL_NO_DATA)
    {
        m_pDbConnection->FreeStmt(hStmt);
        m_pACS->Leave();
        return true;
    }

    if (!SQL_SUCCEEDED(retCode))
    {
        std::cout << __FUNCTION__ << " - SQLFetch failed, query = " << szQuery << std::endl;
        m_pDbConnection->FreeStmt(hStmt);
        m_pACS->Leave();
        return false;
    }

    SQLINTEGER cb;

    do
    {
        SServerAutoCapebyWorldID record;
        ZeroMemory(&record, sizeof(SServerAutoCapebyWorldID));

        SQLGetData(hStmt, 1, SQL_C_LONG, (SQLPOINTER)&record.nID, 0, &cb);
        SQLGetData(hStmt, 2, SQL_C_USHORT, (SQLPOINTER)&record.wWorldID, 0, &cb);
        printf("%s - Load AutoCape regions %d\n", __FUNCTION__, record.wWorldID);
        AutoCapeListWorldId.push_back(record);
        retCode = SQLFetch(hStmt);
    } while (SQL_SUCCEEDED(retCode));

    m_pDbConnection->FreeStmt(hStmt);

    m_pACS->Leave();

    return true;
}

bool CSqlCon::ServerAutoCapebyRegionID()
{
    CAutoCriticalSection* m_pACS = &s_sqlConnectionLock;

    SQLHANDLE hStmt = SQL_NULL_HSTMT;
    CDbConnection* m_pDbConnection = m_connectionstr;

    if (!m_pDbConnection->AllocStmt(hStmt))
    {
        std::cout << __FUNCTION__ << " - Failed to allocate stmt" << std::endl;
        m_pDbConnection->FreeStmt(hStmt);
        return false;
    }

    SQLRETURN retCode;
    const LPCSTR szQuery = "SELECT ID, RegionID FROM [KMTGuard].[dbo].[AutoCapeRegions]";

    m_pACS->Enter();

    retCode = SQLExecDirectA(hStmt, (SQLCHAR*)szQuery, SQL_NTS);
    if (!SQL_SUCCEEDED(retCode))
    {
        std::cout << __FUNCTION__ << " - SQLExecDirect failed, query = " << szQuery << std::endl;
        m_pDbConnection->FreeStmt(hStmt);
        m_pACS->Leave();
        return false;
    }

    retCode = SQLFetch(hStmt);

    if (retCode == SQL_NO_DATA)
    {
        m_pDbConnection->FreeStmt(hStmt);
        m_pACS->Leave();
        return true;
    }

    if (!SQL_SUCCEEDED(retCode))
    {
        std::cout << __FUNCTION__ << " - SQLFetch failed, query = " << szQuery << std::endl;
        m_pDbConnection->FreeStmt(hStmt);
        m_pACS->Leave();
        return false;
    }

    SQLINTEGER cb;

    do
    {
        SServerAutoCapebyRegionID record;
        ZeroMemory(&record, sizeof(SServerAutoCapebyRegionID));

        SQLGetData(hStmt, 1, SQL_C_LONG, (SQLPOINTER)&record.nID, 0, &cb);
        SQLGetData(hStmt, 2, SQL_C_USHORT, (SQLPOINTER)&record.wRegionID, 0, &cb);
        printf("%s - Load AutoCape regions %d\n", __FUNCTION__, record.wRegionID);
        AutoCapeList.push_back(record);
        retCode = SQLFetch(hStmt);
    } while (SQL_SUCCEEDED(retCode));

    m_pDbConnection->FreeStmt(hStmt);

    m_pACS->Leave();

    return true;
}

bool CSqlCon::LoadRefSkillByItemOptLevel()
{
    CAutoCriticalSection* m_pACS = &s_sqlConnectionLock;

    SQLHANDLE hStmt = SQL_NULL_HSTMT;
    CDbConnection* m_pDbConnection = m_connectionstr;

    if (!m_pDbConnection->AllocStmt(hStmt))
    {
        std::cout << __FUNCTION__ << " - Failed to allocate stmt" << std::endl;
        m_pDbConnection->FreeStmt(hStmt);
        return false;
    }

    SQLRETURN retCode;
    const LPCSTR szQuery = "SELECT Link, RefSkillID FROM SRO_VT_SHARD.._RefSkillByItemOptLevel";

    m_pACS->Enter();

    retCode = SQLExecDirectA(hStmt, (SQLCHAR*)szQuery, SQL_NTS);
    if (!SQL_SUCCEEDED(retCode))
    {
        std::cout << __FUNCTION__ << " - SQLExecDirect failed, query = " << szQuery << std::endl;
        m_pDbConnection->FreeStmt(hStmt);
        m_pACS->Leave();
        return false;
    }

    retCode = SQLFetch(hStmt);

    if (retCode == SQL_NO_DATA)
    {
        m_pDbConnection->FreeStmt(hStmt);
        m_pACS->Leave();
        return true;
    }

    if (!SQL_SUCCEEDED(retCode))
    {
        std::cout << __FUNCTION__ << " - SQLFetch failed, query = " << szQuery << std::endl;
        m_pDbConnection->FreeStmt(hStmt);
        m_pACS->Leave();
        return false;
    }

    SQLINTEGER cb;

    do
    {
        _RefSkillByItemOptLevel record;
        ZeroMemory(&record, sizeof(_RefSkillByItemOptLevel));

        SQLGetData(hStmt, 1, SQL_C_LONG, (SQLPOINTER)&record.Link, 0, &cb);
        SQLGetData(hStmt, 2, SQL_C_LONG, (SQLPOINTER)&record.RefSkillID, 0, NULL);

        //printf("%s - pluss Items %d %d\n", __FUNCTION__, record.Link, record.RefSkillID);
        RefSkillByItemOptLevel.insert((std::make_pair(record.Link, record)));
        retCode = SQLFetch(hStmt);
    } while (SQL_SUCCEEDED(retCode));

    m_pDbConnection->FreeStmt(hStmt);

    m_pACS->Leave();

    return true;
}
bool CSqlCon::LoadRefAbilitybyItemOptLevel()
{
    CAutoCriticalSection* m_pACS = &s_sqlConnectionLock;

    SQLHANDLE hStmt = SQL_NULL_HSTMT;
    CDbConnection* m_pDbConnection = m_connectionstr;

    if (!m_pDbConnection->AllocStmt(hStmt))
    {
        std::cout << __FUNCTION__ << " - Failed to allocate stmt" << std::endl;
        m_pDbConnection->FreeStmt(hStmt);
        return false;
    }

    SQLRETURN retCode;
    const LPCSTR szQuery = "SELECT ID, RefItemID, ItemOptLevel FROM SRO_VT_SHARD.._RefAbilityByItemOptLevel";

    m_pACS->Enter();

    retCode = SQLExecDirectA(hStmt, (SQLCHAR*)szQuery, SQL_NTS);
    if (!SQL_SUCCEEDED(retCode))
    {
        std::cout << __FUNCTION__ << " - SQLExecDirect failed, query = " << szQuery << std::endl;
        m_pDbConnection->FreeStmt(hStmt);
        m_pACS->Leave();
        return false;
    }

    retCode = SQLFetch(hStmt);

    if (retCode == SQL_NO_DATA)
    {
        m_pDbConnection->FreeStmt(hStmt);
        m_pACS->Leave();
        return true;
    }

    if (!SQL_SUCCEEDED(retCode))
    {
        std::cout << __FUNCTION__ << " - SQLFetch failed, query = " << szQuery << std::endl;
        m_pDbConnection->FreeStmt(hStmt);
        m_pACS->Leave();
        return false;
    }

    SQLINTEGER cb;

    do
    {
        _RefAbilityByItemOptLevel record;
        ZeroMemory(&record, sizeof(_RefAbilityByItemOptLevel));

        SQLGetData(hStmt, 1, SQL_C_LONG, (SQLPOINTER)&record.ID, 0, &cb);
        SQLGetData(hStmt, 2, SQL_C_LONG, (SQLPOINTER)&record.RefItemID, 0, NULL);
        SQLGetData(hStmt, 3, SQL_C_SBIGINT, (SQLPOINTER)&record.OptLevel, 0, &cb);

  
        //printf("%s - pluss Items %d %d %d\n", __FUNCTION__, record.ID, record.RefItemID, record.OptLevel);
        RefAbilitybyItemOptLevel.insert((std::make_pair(record.ID, record)));
        retCode = SQLFetch(hStmt);
    } while (SQL_SUCCEEDED(retCode));

    m_pDbConnection->FreeStmt(hStmt);

    m_pACS->Leave();

    return true;
}
bool CSqlCon::TimedPlusItems()
{
    CAutoCriticalSection* m_pACS = &s_sqlConnectionLock;

    SQLHANDLE hStmt = SQL_NULL_HSTMT;
    CDbConnection* m_pDbConnection = m_connectionstr;

    if (!m_pDbConnection->AllocStmt(hStmt))
    {
        std::cout << __FUNCTION__ << " - Failed to allocate stmt" << std::endl;
        m_pDbConnection->FreeStmt(hStmt);
        return false;
    }

    SQLRETURN retCode;
    const LPCSTR szQuery = "SELECT CharID,OrjPlus,ID64,EndTime FROM [dbo].[Item_TimedPlus]";

    m_pACS->Enter();

    retCode = SQLExecDirectA(hStmt, (SQLCHAR*)szQuery, SQL_NTS);
    if (!SQL_SUCCEEDED(retCode))
    {
        std::cout << __FUNCTION__ << " - SQLExecDirect failed, query = " << szQuery << std::endl;
        m_pDbConnection->FreeStmt(hStmt);
        m_pACS->Leave();
        return false;
    }

    retCode = SQLFetch(hStmt);

    if (retCode == SQL_NO_DATA)
    {
        m_pDbConnection->FreeStmt(hStmt);
        m_pACS->Leave();
        return true;
    }

    if (!SQL_SUCCEEDED(retCode))
    {
        std::cout << __FUNCTION__ << " - SQLFetch failed, query = " << szQuery << std::endl;
        m_pDbConnection->FreeStmt(hStmt);
        m_pACS->Leave();
        return false;
    }

    SQLINTEGER cb;

    do
    {
        STimedItemPlusDbRecord record;
        ZeroMemory(&record, sizeof(STimedItemPlusDbRecord));

        SQLGetData(hStmt, 1, SQL_C_LONG, (SQLPOINTER)&record.CharID, 0, &cb);
        SQLGetData(hStmt, 2, SQL_C_LONG, (SQLPOINTER)&record.CurrentPlus, 0, NULL);
        SQLGetData(hStmt, 3, SQL_C_SBIGINT, (SQLPOINTER)&record.dwItemID64, 0, &cb);

        SQLGetData(hStmt, 4, SQL_C_UBIGINT, (SQLPOINTER)&record.endTime, 0, &cb);

        printf("%s - pluss Items %d %lld %d %ld\n", __FUNCTION__, record.CharID, record.CurrentPlus, record.dwItemID64, record.endTime);
        TimedItemList.insert((std::make_pair(record.dwItemID64, record)));
        retCode = SQLFetch(hStmt);
    } while (SQL_SUCCEEDED(retCode));

    m_pDbConnection->FreeStmt(hStmt);

    m_pACS->Leave();

    return true;
}

bool CSqlCon::TimedDevillPlusItems()
{
    CAutoCriticalSection* m_pACS = &s_sqlConnectionLock;

    SQLHANDLE hStmt = SQL_NULL_HSTMT;
    CDbConnection* m_pDbConnection = m_connectionstr;

    if (!m_pDbConnection->AllocStmt(hStmt))
    {
        std::cout << __FUNCTION__ << " - Failed to allocate stmt" << std::endl;
        m_pDbConnection->FreeStmt(hStmt);
        return false;
    }

    SQLRETURN retCode;
    const LPCSTR szQuery = "SELECT CharID,OrjPlus,ID64,EndTime FROM [dbo].[Item_TimedDevilPlus]";

    m_pACS->Enter();

    retCode = SQLExecDirectA(hStmt, (SQLCHAR*)szQuery, SQL_NTS);
    if (!SQL_SUCCEEDED(retCode))
    {
        std::cout << __FUNCTION__ << " - SQLExecDirect failed, query = " << szQuery << std::endl;
        m_pDbConnection->FreeStmt(hStmt);
        m_pACS->Leave();
        return false;
    }

    retCode = SQLFetch(hStmt);

    if (retCode == SQL_NO_DATA)
    {
        m_pDbConnection->FreeStmt(hStmt);
        m_pACS->Leave();
        return true;
    }

    if (!SQL_SUCCEEDED(retCode))
    {
        std::cout << __FUNCTION__ << " - SQLFetch failed, query = " << szQuery << std::endl;
        m_pDbConnection->FreeStmt(hStmt);
        m_pACS->Leave();
        return false;
    }

    SQLINTEGER cb;

    do
    {
        STimedDevillPlusDbRecord record;
        ZeroMemory(&record, sizeof(STimedDevillPlusDbRecord));

        SQLGetData(hStmt, 1, SQL_C_LONG, (SQLPOINTER)&record.CharID, 0, &cb);
        SQLGetData(hStmt, 2, SQL_C_LONG, (SQLPOINTER)&record.CurrentPlus, 0, NULL);
        SQLGetData(hStmt, 3, SQL_C_SBIGINT, (SQLPOINTER)&record.dwItemID64, 0, &cb);

        SQLGetData(hStmt, 4, SQL_C_UBIGINT, (SQLPOINTER)&record.endTime, 0, &cb);

        printf("%s - devil Items %d %lld %d %ld\n", __FUNCTION__, record.CharID, record.CurrentPlus, record.dwItemID64, record.endTime);
        STimedDevillList.insert((std::make_pair(record.dwItemID64, record)));
        retCode = SQLFetch(hStmt);
    } while (SQL_SUCCEEDED(retCode));

    m_pDbConnection->FreeStmt(hStmt);

    m_pACS->Leave();

    return true;
}


BYTE CSqlCon::GetItemBindingOpt(INT64 ID64)
{
    CAutoCriticalSection* m_pACS = &s_sqlConnectionLock;

    SQLHANDLE hStmt = SQL_NULL_HSTMT;
    CDbConnection* m_pDbConnection = m_connectionstr;
    BYTE OptList = 0;
    if (!m_pDbConnection->AllocStmt(hStmt))
    {
        std::cout << __FUNCTION__ << " - Failed to allocate stmt" << std::endl;
        m_pDbConnection->FreeStmt(hStmt);
        return OptList;
    }

    SQLRETURN retCode;
    char szQuery[1024] = { 0 };
    sprintf(szQuery,
            "SELECT nOptValue FROM SRO_VT_SHARD.._BindingOptionWithItem with(nolock) where nItemDBID = %lld and bOptType = 2", ID64);

    m_pACS->Enter();

    retCode = SQLExecDirectA(hStmt, (SQLCHAR*)szQuery, SQL_NTS);
    if (!SQL_SUCCEEDED(retCode))
    {
        std::cout << __FUNCTION__ << " - SQLExecDirect failed, query = " << szQuery << std::endl;
        m_pDbConnection->FreeStmt(hStmt);
        m_pACS->Leave();
        return OptList;
    }

    retCode = SQLFetch(hStmt);

    if (retCode == SQL_NO_DATA)
    {
        m_pDbConnection->FreeStmt(hStmt);
        m_pACS->Leave();
        return OptList;
    }

    if (!SQL_SUCCEEDED(retCode))
    {
        std::cout << __FUNCTION__ << " - SQLFetch failed, query = " << szQuery << std::endl;
        m_pDbConnection->FreeStmt(hStmt);
        m_pACS->Leave();
        return OptList;
    }

    SQLINTEGER cb;

    do
    {
        BYTE nOptID;

        SQLGetData(hStmt, 1, SQL_C_TINYINT, (SQLPOINTER)&nOptID, 0, NULL); // Sütun indeksi 1 olmalı
        printf("%s - aaa %d \n", __FUNCTION__, nOptID);
        OptList = nOptID;
        retCode = SQLFetch(hStmt);
    } while (SQL_SUCCEEDED(retCode));

    m_pDbConnection->FreeStmt(hStmt);

    m_pACS->Leave();

    return OptList;
}

bool CSqlCon::GetCustomNpcInteractionRecords(std::list<SCustomNpcInteractionDbRecord>& result)
{
    CAutoCriticalSection* m_pACS = &s_sqlConnectionLock;

    SQLHANDLE hStmt = SQL_NULL_HSTMT;

    CDbConnection* m_pDbConnection = m_connectionstr;


    if (!m_pDbConnection->AllocStmt(hStmt))
    {
        std::cout << __FUNCTION__ << " - Failed to allocate stmt" << std::endl;
        m_pDbConnection->FreeStmt(hStmt);
        return false;
    }

    SQLRETURN retCode;
    const LPCSTR szQuery = "SELECT ID, CodeName128, InteractionID FROM ___CustomNpcInteraction";

    m_pACS->Enter();

    retCode = SQLExecDirectA(hStmt, (SQLCHAR*)szQuery, SQL_NTS);
    if (!SQL_SUCCEEDED(retCode))
    {
        std::cout << __FUNCTION__ << " - SQLExecDirect failed, query = " << szQuery << std::endl;
        m_pDbConnection->FreeStmt(hStmt);
        m_pACS->Leave();
        return false;
    }

    retCode = SQLFetch(hStmt);

    if (retCode == SQL_NO_DATA)
    {
        m_pDbConnection->FreeStmt(hStmt);
        m_pACS->Leave();
        return true;
    }

    if (!SQL_SUCCEEDED(retCode))
    {
        std::cout << __FUNCTION__ << " - SQLFetch failed, query = " << szQuery << std::endl;
        m_pDbConnection->FreeStmt(hStmt);
        m_pACS->Leave();
        return false;
    }

    SQLINTEGER cb;

    do
    {
        SCustomNpcInteractionDbRecord record;
        ZeroMemory(&record, sizeof(SCustomNpcInteractionDbRecord));

        SQLGetData(hStmt, 1, SQL_C_LONG, (SQLPOINTER)&record.nID, 0, &cb);
        SQLGetData(hStmt, 2, SQL_C_CHAR, (SQLPOINTER)&record.szCodeName128, sizeof(record.szCodeName128), &cb);
        SQLGetData(hStmt, 3, SQL_C_LONG, (SQLPOINTER)&record.nInteractionID, 0, &cb);

        result.push_back(record);
        retCode = SQLFetch(hStmt);
    } while (SQL_SUCCEEDED(retCode));

    m_pDbConnection->FreeStmt(hStmt);
    m_pACS->Leave();
    return true;
}

// mob restrict
bool CSqlCon::LoadAttackRestrictionsByMob()
{
    ScopedSqlConnectionLock databaseGuard(s_sqlConnectionLock);
    ScopedSqlStatement statement(m_connectionstr);
    if (!statement.Allocate())
    {
        BS_INFO("[KMTGuard][Database] Attack-restriction statement allocation failed");
        return false;
    }
    const SQLHANDLE hStmt = statement.Get();

    const LPCSTR szQuery =
        "SELECT MobRefObjID,"
        "       OnlyOffJob, OnlyOnJob, OnlyByThief, OnlyByTrader, OnlyStrPlayer, OnlyIntPlayer,"
        "       ISNULL(AllowedJobMask,0), ISNULL(AllowedCapeMask,0), ISNULL(AllowedRaceMask,0),"
        "       ISNULL(CAST(RequireParty AS INT),-1), ISNULL(CAST(RequireGuild AS INT),-1)"
        "  FROM [KMTGuard].[dbo].[Security_AttackRules]";

    SQLRETURN rc = SQLExecDirectA(hStmt, (SQLCHAR*)szQuery, SQL_NTS);
    if (!SQL_SUCCEEDED(rc))
    {
        BS_INFO("[KMTGuard][Database] Attack-restriction query failed");
        return false;
    }

    rc = SQLFetch(hStmt);
    if (rc == SQL_NO_DATA)
    {
        ScopedWinCriticalSection cacheGuard(s_attackRestrictionLock);
        s_AttackRestrByMob.clear();
        BS_INFO("[KMTGuard][Security] Attack restrictions loaded: 0");
        return true;
    }
    if (!SQL_SUCCEEDED(rc))
    {
        BS_INFO("[KMTGuard][Database] Attack-restriction fetch failed");
        return false;
    }

    std::map<int, SAttackRestrictionByMob> loadedRestrictions;

    int  mob = 0;
    BYTE bOnlyOff = 0, bOnlyOn = 0, bThief = 0, bTrader = 0, bStr = 0, bInt = 0;
    BYTE maskJob = 0, maskCape = 0, maskRace = 0;
    int  reqParty = -1, reqGuild = -1;
    unsigned int rows = 0;

    do
    {
        SAttackRestrictionByMob r;

        mob = 0;
        bOnlyOff = bOnlyOn = bThief = bTrader = bStr = bInt = 0;
        maskJob = maskCape = maskRace = 0;
        reqParty = reqGuild = -1;

        SQLRETURN reads[12];
        SQLLEN lengths[12] = { 0 };
        reads[0] = SQLGetData(hStmt, 1, SQL_C_LONG, &mob, 0, &lengths[0]);
        reads[1] = SQLGetData(hStmt, 2, SQL_C_BIT, &bOnlyOff, 0, &lengths[1]);
        reads[2] = SQLGetData(hStmt, 3, SQL_C_BIT, &bOnlyOn, 0, &lengths[2]);
        reads[3] = SQLGetData(hStmt, 4, SQL_C_BIT, &bThief, 0, &lengths[3]);
        reads[4] = SQLGetData(hStmt, 5, SQL_C_BIT, &bTrader, 0, &lengths[4]);
        reads[5] = SQLGetData(hStmt, 6, SQL_C_BIT, &bStr, 0, &lengths[5]);
        reads[6] = SQLGetData(hStmt, 7, SQL_C_BIT, &bInt, 0, &lengths[6]);
        reads[7] = SQLGetData(hStmt, 8, SQL_C_TINYINT, &maskJob, 0, &lengths[7]);
        reads[8] = SQLGetData(hStmt, 9, SQL_C_TINYINT, &maskCape, 0, &lengths[8]);
        reads[9] = SQLGetData(hStmt, 10, SQL_C_TINYINT, &maskRace, 0, &lengths[9]);
        reads[10] = SQLGetData(hStmt, 11, SQL_C_LONG, &reqParty, 0, &lengths[10]);
        reads[11] = SQLGetData(hStmt, 12, SQL_C_LONG, &reqGuild, 0, &lengths[11]);

        for (size_t index = 0; index < 12; ++index)
        {
            if (!SQL_SUCCEEDED(reads[index]) || lengths[index] == SQL_NULL_DATA)
            {
                BS_INFO("[KMTGuard][Database] Attack-restriction row is incomplete");
                return false;
            }
        }
        if (mob <= 0 || reqParty < -1 || reqParty > 1 || reqGuild < -1 || reqGuild > 1)
        {
            BS_INFO("[KMTGuard][Database] Attack-restriction row is invalid");
            return false;
        }

        r.MobRefObjID = mob;
        r.OnlyOffJob = (bOnlyOff != 0);
        r.OnlyOnJob = (bOnlyOn != 0);
        r.OnlyByThief = (bThief != 0);
        r.OnlyByTrader = (bTrader != 0);
        r.OnlyStrPlayer = (bStr != 0);
        r.OnlyIntPlayer = (bInt != 0);
        r.AllowedJobMask = maskJob;
        r.AllowedCapeMask = maskCape;
        r.AllowedRaceMask = maskRace;
        r.RequireParty = reqParty;
        r.RequireGuild = reqGuild;

        loadedRestrictions[r.MobRefObjID] = r;

        ++rows;
        rc = SQLFetch(hStmt);
    } while (SQL_SUCCEEDED(rc));

    if (rc != SQL_NO_DATA)
    {
        BS_INFO("[KMTGuard][Database] Attack-restriction fetch ended with an error");
        return false;
    }

    {
        ScopedWinCriticalSection cacheGuard(s_attackRestrictionLock);
        s_AttackRestrByMob.swap(loadedRestrictions);
    }
    BS_INFO("[KMTGuard][Security] Attack restrictions loaded: %u", rows);
    return true;
}

bool CSqlCon::TryGetAttackRestriction(int mobRefObjId, SAttackRestrictionByMob& result)
{
    ScopedWinCriticalSection cacheGuard(s_attackRestrictionLock);
    std::map<int, SAttackRestrictionByMob>::const_iterator entry =
        s_AttackRestrByMob.find(mobRefObjId);
    if (entry == s_AttackRestrByMob.end())
        return false;
    result = entry->second;
    return true;
}

bool CSqlCon::LoadItemRegionRestrictions()
{
    ScopedSqlConnectionLock databaseGuard(s_sqlConnectionLock);
    ScopedSqlStatement statement(m_connectionstr);
    if (!statement.Allocate())
    {
        BS_INFO("[KMTGuard][Database] Item-region statement allocation failed");
        return false;
    }

    const LPCSTR query =
        "SELECT WorldID, RegionID, ItemID "
        "FROM [KMTGuard].[dbo].[Security_ItemRegionRestrictions] "
        "WHERE Enabled = 1";
    SQLRETURN result = SQLExecDirectA(statement.Get(), (SQLCHAR*)query, SQL_NTS);
    if (!SQL_SUCCEEDED(result))
    {
        BS_INFO("[KMTGuard][Database] Item-region restriction query failed; apply the v6.0.0 SQL update first");
        return false;
    }

    std::vector<SItemRegionRestriction> loaded;
    result = SQLFetch(statement.Get());
    while (SQL_SUCCEEDED(result))
    {
        SItemRegionRestriction rule;
        SQLLEN lengths[3] = { 0 };
        SQLRETURN reads[3];
        reads[0] = SQLGetData(statement.Get(), 1, SQL_C_LONG, &rule.WorldID, 0, &lengths[0]);
        reads[1] = SQLGetData(statement.Get(), 2, SQL_C_LONG, &rule.RegionID, 0, &lengths[1]);
        reads[2] = SQLGetData(statement.Get(), 3, SQL_C_LONG, &rule.ItemID, 0, &lengths[2]);

        for (size_t index = 0; index < 3; ++index)
        {
            if (!SQL_SUCCEEDED(reads[index]) || lengths[index] == SQL_NULL_DATA)
            {
                BS_INFO("[KMTGuard][Database] Item-region restriction row is incomplete");
                return false;
            }
        }
        if (rule.WorldID < 0 || rule.WorldID > 65535 ||
            rule.RegionID < -32768 || rule.RegionID > 32767 || rule.ItemID < 0)
        {
            BS_INFO("[KMTGuard][Database] Item-region restriction row is invalid");
            return false;
        }

        loaded.push_back(rule);
        result = SQLFetch(statement.Get());
    }

    if (result != SQL_NO_DATA)
    {
        BS_INFO("[KMTGuard][Database] Item-region restriction fetch ended with an error");
        return false;
    }

    {
        ScopedWinCriticalSection cacheGuard(s_itemRegionRestrictionLock);
        s_ItemRegionRestrictions.swap(loaded);
    }
    BS_INFO("[KMTGuard][Security] Item-region restrictions loaded: %u (startup cache)",
            static_cast<unsigned int>(s_ItemRegionRestrictions.size()));
    return true;
}

bool CSqlCon::IsItemTrackedForRegion(int itemId)
{
    if (itemId <= 0)
        return false;

    ScopedWinCriticalSection cacheGuard(s_itemRegionRestrictionLock);
    for (std::vector<SItemRegionRestriction>::const_iterator entry =
             s_ItemRegionRestrictions.begin();
         entry != s_ItemRegionRestrictions.end(); ++entry)
    {
        if (entry->ItemID == 0 || entry->ItemID == itemId)
            return true;
    }
    return false;
}

bool CSqlCon::IsItemBlockedInRegion(int worldId, int regionId, int itemId)
{
    if (worldId < 0 || worldId > 65535 ||
        regionId < -32768 || regionId > 32767 || itemId <= 0)
        return false;

    ScopedWinCriticalSection cacheGuard(s_itemRegionRestrictionLock);
    for (std::vector<SItemRegionRestriction>::const_iterator entry =
             s_ItemRegionRestrictions.begin();
         entry != s_ItemRegionRestrictions.end(); ++entry)
    {
        if ((entry->WorldID == 0 || entry->WorldID == worldId) &&
            entry->RegionID == regionId &&
            (entry->ItemID == 0 || entry->ItemID == itemId))
            return true;
    }
    return false;
}

bool CSqlCon::IsCharInParty(int CharID)
{
    CAutoCriticalSection* m_pACS = &s_sqlConnectionLock;
    SQLHANDLE hStmt = SQL_NULL_HSTMT;
    CDbConnection* m_pDbConnection = m_connectionstr;

    if (!m_pDbConnection->AllocStmt(hStmt))
    {
        std::cout << __FUNCTION__ << " - Failed to allocate stmt" << std::endl;
        m_pDbConnection->FreeStmt(hStmt);
        return false;
    }

    char szQuery[256] = { 0 };
    sprintf(szQuery,
        "SELECT TOP 1 PartyID FROM [KMTGuard].[dbo].[Party_Members] WITH (NOLOCK) WHERE CharID = %d",
        CharID);

    m_pACS->Enter();

    SQLRETURN retCode = SQLExecDirectA(hStmt, (SQLCHAR*)szQuery, SQL_NTS);
    if (!SQL_SUCCEEDED(retCode))
    {
        std::cout << __FUNCTION__ << " - SQLExecDirect failed, query = " << szQuery << std::endl;
        m_pDbConnection->FreeStmt(hStmt);
        m_pACS->Leave();
        return false;
    }

    retCode = SQLFetch(hStmt);

    bool inParty = false;
    if (retCode == SQL_NO_DATA)
    {
        inParty = false;
    }
    else if (SQL_SUCCEEDED(retCode))
    {
        inParty = true;
    }
    else
    {
        std::cout << __FUNCTION__ << " - SQLFetch failed, query = " << szQuery << std::endl;
        inParty = false;
    }

    m_pDbConnection->FreeStmt(hStmt);
    m_pACS->Leave();

    return inParty;
}
