#include "AsyncGSCommands.h"
// Console stuffs
#pragma warning(disable:4996) // _CRT_SECURE_NO_WARNINGS
#include <Windows.h>
#include <iostream>
// Utils
#include "Utils/IO/SimpleIni.h"
#include "Utils/Memory/Process.h"
#include "Utils/Memory/hook.h"
#pragma warning(disable:4244) // Bitwise operations warnings
// ASM injection
#include "AsmEdition.h"
#include "SettingManagers/Settings.h"
#include <cstring>
#include "Network/ShardNetManager.h"
#include "Console/ShardManagerConsole.h"
#include "Runtime/CommandDispatchGuard.h"
#include <GameServerCommandContract.h>

namespace
{
	const DWORD COMMAND_IDLE_POLL_DELAY_MS = 250;
	const DWORD COMMAND_DB_RETRY_DELAY_MS = 2000;
	const DWORD COMMAND_MONITOR_INTERVAL_MS = 30000;
	const LONG MAX_COMMANDS_PER_SECOND = 500;

	HANDLE s_commandStopEvent = NULL;
	HANDLE s_commandWorkerThread = NULL;
	HANDLE s_commandMonitorThread = NULL;
	DWORD s_commandWorkerThreadId = 0;

	volatile LONG s_commandsDispatched = 0;
	volatile LONG s_commandsRejected = 0;
	volatile LONG s_commandsFailed = 0;
	volatile LONG s_commandDatabaseFailures = 0;
	volatile LONG s_lastCommandAction = 0;
	volatile LONG s_lastCommandResult = 0;
	volatile LONG s_lastCommandDurationMs = 0;
	volatile LONG s_lastCommandPollTick = 0;
	volatile LONG s_lastCommandProgressTick = 0;
	__declspec(align(8)) volatile LONGLONG s_lastCommandId = 0;
	volatile LONG s_lastWorkerExceptionCode = 0;
	volatile ULONG_PTR s_lastWorkerExceptionAddress = 0;

	DWORD WINAPI CommandWorkerThreadEntry(LPVOID parameter);

	bool IsCommandWorkerRunning()
	{
		return InterlockedCompareExchange(
			&AsyncGSCommands::m_IsRunningDatabaseFetch, 0, 0) != 0;
	}

	bool WaitForCommandStop(DWORD timeoutMs)
	{
		if (s_commandStopEvent == NULL)
		{
			Sleep(timeoutMs);
			return !IsCommandWorkerRunning();
		}
		return WaitForSingleObject(s_commandStopEvent, timeoutMs) == WAIT_OBJECT_0;
	}

	const char* CommandResultName(LONG result)
	{
		switch (result)
		{
		case 1: return "Dispatched";
		case 2: return "Rejected";
		case 3: return "Retry";
		case 4: return "Indeterminate";
		case 5: return "AcknowledgementFailed";
		default: return "None";
		}
	}

	void CloseDatabaseLink(DatabaseLink& link)
	{
		link.sqlCmd.Close();
		link.sqlConn.Close();
	}

	bool OpenDatabaseLink(DatabaseLink& link)
	{
		CloseDatabaseLink(link);
		if (CSettings::m_Settings == NULL)
			return false;
		if (!link.sqlConn.Open(
				(SQLWCHAR*)CSettings::m_Settings->DatabaseConnectionString.str().c_str()))
			return false;
		if (link.sqlCmd.Open(link.sqlConn))
			return true;
		link.sqlConn.Close();
		return false;
	}

	bool ReconnectDatabaseLink(DatabaseLink& link, const char* purpose)
	{
		ShardManagerConsole::WriteFormat(
			3, "GameServer command bridge reconnecting its %s database session", purpose);
		if (OpenDatabaseLink(link))
		{
			ShardManagerConsole::WriteFormat(
				2, "GameServer command bridge restored its %s database session", purpose);
			return true;
		}
		InterlockedIncrement(&s_commandDatabaseFailures);
		ShardManagerConsole::WriteFormat(
			4, "GameServer command bridge could not restore its %s database session", purpose);
		return false;
	}

	void RecordCommandDatabaseFailure(const char* operation, SQLBIGINT commandId, SQLINTEGER actionId)
	{
		InterlockedIncrement(&s_commandDatabaseFailures);
		ShardManagerConsole::WriteFormat(
			4, "GameServer command database operation failed: operation=%s command=%I64d action=%ld",
			operation, commandId, static_cast<long>(actionId));
	}

	bool IsValidDestination(int worldId, int regionId, int x, int y, int z)
	{
		return KmtGameServerCommand::IsValidDestination(worldId, regionId, x, y, z);
	}

	bool ExecuteClaimCompletion(DatabaseLink& link, SQLBIGINT commandId, const char* claimToken,
		const char* status, int resultCode, const char* reason)
	{
		if (claimToken == NULL || claimToken[0] == '\0')
			return false;
		std::wstringstream query;
		query << L"EXEC [KMTGuard].[dbo].[Command_CompleteGameServer] @CommandID="
			<< commandId << L",@ClaimToken='" << claimToken << L"',@Status='" << status
			<< L"',@ResultCode=" << resultCode << L",@Reason=N'" << reason << L"'";
		const bool success = link.sqlCmd.ExecuteQuery((SQLWCHAR*)query.str().c_str());
		link.sqlCmd.Clear();
		return success;
	}

	bool ExecuteClaimRetry(DatabaseLink& link, SQLBIGINT commandId, const char* claimToken, const char* reason)
	{
		if (claimToken == NULL || claimToken[0] == '\0')
			return false;
		std::wstringstream query;
		query << L"EXEC [KMTGuard].[dbo].[Command_RetryGameServer] @CommandID="
			<< commandId << L",@ClaimToken='" << claimToken << L"',@Reason=N'" << reason << L"'";
		const bool success = link.sqlCmd.ExecuteQuery((SQLWCHAR*)query.str().c_str());
		link.sqlCmd.Clear();
		return success;
	}

	bool CompleteClaimWithRecovery(SQLBIGINT commandId, SQLINTEGER actionId, const char* claimToken,
		const char* status, int resultCode, const char* reason)
	{
		if (ExecuteClaimCompletion(
				AsyncGSCommands::m_dbLinkHelper, commandId, claimToken, status, resultCode, reason))
			return true;

		RecordCommandDatabaseFailure("complete", commandId, actionId);
		if (!ReconnectDatabaseLink(AsyncGSCommands::m_dbLinkHelper, "completion"))
			return false;
		if (ExecuteClaimCompletion(
				AsyncGSCommands::m_dbLinkHelper, commandId, claimToken, status, resultCode, reason))
			return true;

		RecordCommandDatabaseFailure("complete-after-reconnect", commandId, actionId);
		return false;
	}

	bool RetryClaimWithRecovery(SQLBIGINT commandId, SQLINTEGER actionId,
		const char* claimToken, const char* reason)
	{
		if (ExecuteClaimRetry(
				AsyncGSCommands::m_dbLinkHelper, commandId, claimToken, reason))
			return true;

		RecordCommandDatabaseFailure("retry", commandId, actionId);
		if (!ReconnectDatabaseLink(AsyncGSCommands::m_dbLinkHelper, "completion"))
			return false;
		if (ExecuteClaimRetry(
				AsyncGSCommands::m_dbLinkHelper, commandId, claimToken, reason))
			return true;

		RecordCommandDatabaseFailure("retry-after-reconnect", commandId, actionId);
		return false;
	}

	LONG RecordCommandWorkerException(EXCEPTION_POINTERS* details)
	{
		if (details != NULL && details->ExceptionRecord != NULL)
		{
			InterlockedExchange(&s_lastWorkerExceptionCode,
				static_cast<LONG>(details->ExceptionRecord->ExceptionCode));
			s_lastWorkerExceptionAddress = reinterpret_cast<ULONG_PTR>(
				details->ExceptionRecord->ExceptionAddress);
		}
		return EXCEPTION_EXECUTE_HANDLER;
	}

	DWORD WINAPI CommandWorkerThreadEntry(LPVOID parameter)
	{
		__try
		{
			return AsyncGSCommands::DatabaseFetchThread(parameter);
		}
		__except (RecordCommandWorkerException(GetExceptionInformation()))
		{
			const DWORD exceptionCode = static_cast<DWORD>(InterlockedCompareExchange(
				&s_lastWorkerExceptionCode, 0, 0));
			ShardManagerConsole::WriteFormat(
				4, "GameServer command bridge worker trapped a structured exception: code=0x%08lX address=0x%p",
				static_cast<unsigned long>(exceptionCode),
				reinterpret_cast<void*>(s_lastWorkerExceptionAddress));
			InterlockedExchange(&AsyncGSCommands::m_IsRunningDatabaseFetch, 0);
			return exceptionCode == 0 ? ERROR_UNHANDLED_EXCEPTION : exceptionCode;
		}
	}

	bool StartCommandWorker(const char* state)
	{
		InterlockedExchange(&s_lastWorkerExceptionCode, 0);
		s_lastWorkerExceptionAddress = 0;
		InterlockedExchange(&AsyncGSCommands::m_IsRunningDatabaseFetch, 1);
		s_commandWorkerThread = CreateThread(
			NULL, 0, CommandWorkerThreadEntry, NULL, 0, &s_commandWorkerThreadId);
		if (s_commandWorkerThread == NULL)
		{
			InterlockedExchange(&AsyncGSCommands::m_IsRunningDatabaseFetch, 0);
			ShardManagerConsole::WriteFormat(
				4, "GameServer command bridge worker could not be %s (win32=%lu)",
				state, static_cast<unsigned long>(GetLastError()));
			return false;
		}

		ShardManagerConsole::WriteFormat(
			2, "GameServer command bridge worker %s (thread=%lu)",
			state, static_cast<unsigned long>(s_commandWorkerThreadId));
		return true;
	}

	bool RestartCommandWorker()
	{
		while (!WaitForCommandStop(COMMAND_DB_RETRY_DELAY_MS))
		{
			const bool claimReady = ReconnectDatabaseLink(
				AsyncGSCommands::m_dbLink, "claim after worker stop");
			const bool completionReady = ReconnectDatabaseLink(
				AsyncGSCommands::m_dbLinkHelper, "completion after worker stop");
			if (claimReady && completionReady && StartCommandWorker("restarted"))
				return true;
		}
		return false;
	}

	DWORD WINAPI CommandBridgeMonitorThread(LPVOID)
	{
		for (;;)
		{
			HANDLE waitHandles[2] = { s_commandStopEvent, s_commandWorkerThread };
			const DWORD waitResult = WaitForMultipleObjects(
				2, waitHandles, FALSE, COMMAND_MONITOR_INTERVAL_MS);
			if (waitResult == WAIT_OBJECT_0)
				break;
			if (waitResult == WAIT_OBJECT_0 + 1)
			{
				DWORD exitCode = 0;
				GetExitCodeThread(s_commandWorkerThread, &exitCode);
				const DWORD exceptionCode = static_cast<DWORD>(InterlockedCompareExchange(
					&s_lastWorkerExceptionCode, 0, 0));
				ShardManagerConsole::WriteFormat(
					4, "GameServer command bridge worker stopped unexpectedly: exit=0x%08lX exception=0x%08lX address=0x%p; restart scheduled",
					static_cast<unsigned long>(exitCode),
					static_cast<unsigned long>(exceptionCode),
					reinterpret_cast<void*>(s_lastWorkerExceptionAddress));
				CloseHandle(s_commandWorkerThread);
				s_commandWorkerThread = NULL;
				s_commandWorkerThreadId = 0;
				InterlockedExchange(&AsyncGSCommands::m_IsRunningDatabaseFetch, 0);
				if (!RestartCommandWorker())
					break;
				continue;
			}
			if (waitResult == WAIT_FAILED)
			{
				ShardManagerConsole::WriteFormat(
					4, "GameServer command bridge monitor wait failed (win32=%lu)",
					static_cast<unsigned long>(GetLastError()));
				if (WaitForCommandStop(COMMAND_DB_RETRY_DELAY_MS))
					break;
				continue;
			}

		}
		return 0;
	}
}

volatile LONG AsyncGSCommands::m_IsInitialized;
DatabaseLink AsyncGSCommands::m_dbLink, AsyncGSCommands::m_dbLinkHelper, AsyncGSCommands::m_dbUniqueLog;
volatile LONG AsyncGSCommands::m_IsRunningDatabaseFetch;

bool AsyncGSCommands::Initialize()
{
	const LONG previous = InterlockedCompareExchange(&m_IsInitialized, 1, 0);
	if (previous != 0)
	{
		while (InterlockedCompareExchange(&m_IsInitialized, 0, 0) == 1)
			Sleep(1);
		return InterlockedCompareExchange(&m_IsInitialized, 0, 0) == 2;
	}

	if (!InitDatabaseFetch())
	{
		InterlockedExchange(&m_IsInitialized, 0);
		return false;
	}
	InterlockedExchange(&m_IsInitialized, 2);
	return true;
}

bool AsyncGSCommands::InitDatabaseFetch()
{
	ShardManagerConsole::WriteInfo("Preparing the GameServer command bridge");

	s_commandStopEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
	if (s_commandStopEvent == NULL)
	{
		ShardManagerConsole::WriteFailure("GameServer command bridge stop event could not be created");
		return false;
	}

	if (!OpenDatabaseLink(m_dbLink) || !OpenDatabaseLink(m_dbLinkHelper))
	{
		ShardManagerConsole::WriteFailure("GameServer command bridge database sessions could not be opened");
		CloseDatabaseLink(m_dbLinkHelper);
		CloseDatabaseLink(m_dbLink);
		CloseHandle(s_commandStopEvent);
		s_commandStopEvent = NULL;
		return false;
	}

	if (!StartCommandWorker("started"))
	{
		CloseDatabaseLink(m_dbLinkHelper);
		CloseDatabaseLink(m_dbLink);
		CloseHandle(s_commandStopEvent);
		s_commandStopEvent = NULL;
		return false;
	}

	s_commandMonitorThread = CreateThread(
		NULL, 0, CommandBridgeMonitorThread, NULL, 0, NULL);
	if (s_commandMonitorThread == NULL)
		ShardManagerConsole::WriteWarning("GameServer command bridge health monitor could not be started");
	return true;
}

void AsyncGSCommands::Shutdown()
{
	if (InterlockedCompareExchange(&m_IsInitialized, 0, 0) == 0 &&
		s_commandWorkerThread == NULL && s_commandStopEvent == NULL)
		return;

	InterlockedExchange(&m_IsRunningDatabaseFetch, 0);
	if (s_commandStopEvent != NULL)
		SetEvent(s_commandStopEvent);

	if (s_commandWorkerThread != NULL && GetCurrentThreadId() != s_commandWorkerThreadId)
		WaitForSingleObject(s_commandWorkerThread, 35000);
	if (s_commandMonitorThread != NULL)
		WaitForSingleObject(s_commandMonitorThread, 5000);

	if (s_commandMonitorThread != NULL)
	{
		CloseHandle(s_commandMonitorThread);
		s_commandMonitorThread = NULL;
	}
	if (s_commandWorkerThread != NULL)
	{
		CloseHandle(s_commandWorkerThread);
		s_commandWorkerThread = NULL;
	}
	s_commandWorkerThreadId = 0;

	CloseDatabaseLink(m_dbLinkHelper);
	CloseDatabaseLink(m_dbLink);
	if (s_commandStopEvent != NULL)
	{
		CloseHandle(s_commandStopEvent);
		s_commandStopEvent = NULL;
	}
	InterlockedExchange(&m_IsInitialized, 0);
}

DWORD WINAPI AsyncGSCommands::DatabaseFetchThread(LPVOID)
{
	// Load file
	CSimpleIniA ini;
	ini.LoadFile("KMTGuard-Addon.ini");


	// Show a message about table to be fetch
	ShardManagerConsole::WriteInfo("Command bridge will begin polling in 15 seconds");
	if (WaitForCommandStop(15000))
	{
		ShardManagerConsole::WriteWarning("GameServer command bridge stopped before polling began");
		return 0;
	}

	//AppManager::LoadLockedItemList();


	ShardManagerConsole::WriteSuccess("GameServer command bridge is active");

	m_dbLink.sqlCmd.Clear();

	// Start fetching actions without result
	std::wstringstream qSelectActions;
	qSelectActions << L"EXEC [KMTGuard].[dbo].[Command_ClaimGameServer]";
	DWORD rateWindowStarted = GetTickCount();
	LONG commandsInRateWindow = 0;
	while (IsCommandWorkerRunning())
	{
		SQLBIGINT activeCommandId = 0;
		SQLINTEGER activeActionId = 0;
		try
		{
		InterlockedExchange(&s_lastCommandPollTick, static_cast<LONG>(GetTickCount()));

		// Try to execute query
		if (!m_dbLink.sqlCmd.ExecuteQuery((SQLWCHAR*)qSelectActions.str().c_str()))
		{
			RecordCommandDatabaseFailure("claim", 0, 0);
			ReconnectDatabaseLink(m_dbLink, "claim");
			if (WaitForCommandStop(COMMAND_DB_RETRY_DELAY_MS))
				break;
			continue;
		}

		// Fetch one by one
		bool fetchedCommand = false;
		SQL_FETCH_RESULT fetchResult = SQL_FETCH_NO_DATA;
		while ((fetchResult = m_dbLink.sqlCmd.FetchDataResult()) == SQL_FETCH_ROW)
		{
			fetchedCommand = true;
			const DWORD commandStarted = GetTickCount();
			// Set default state
			FETCH_ACTION_STATE actionResult = FETCH_ACTION_STATE::SUCCESS;

			// Read required params
			SQLBIGINT cID = 0;
			SQLINTEGER cActionID = 0;
			char claimToken[40] = { 0 };
			SQLINTEGER attemptCount = 0;
			SQLINTEGER CharID, MobID, GameWorldID, RegionId, PosX, PosY, PosZ, GenerateRadius;

			if (!m_dbLink.sqlCmd.GetData(1, SQL_C_SBIGINT, &cID, 0, NULL) ||
				!m_dbLink.sqlCmd.GetData(2, SQL_C_ULONG, &cActionID, 0, NULL))
			{
				RecordCommandDatabaseFailure("read-identity", cID, cActionID);
				continue;
			}
			activeCommandId = cID;
			activeActionId = cActionID;
			CommandDispatchGuard::Reset();
			// Try to execute the action
			try {
				switch (cActionID)
				{
				case 1: // grantname
				{
					// Read & check params
					char GrantNameData1[128];
					SQLUINTEGER cParam02;
					SQLINTEGER cParam03;
					SQLUSMALLINT cParam04;
					if (m_dbLink.sqlCmd.GetData(3, SQL_C_LONG, &CharID, 0, NULL)
						&& m_dbLink.sqlCmd.GetData(4, SQL_C_CHAR, &GrantNameData1, 128, 0))
					{
						CMsgStreamBuffer* pMsg = CShardNetManager::AllocMsgForGS();
						pMsg->SetMsgID(0x8888);

						*pMsg << BYTE(1);
						*pMsg << CharID;
						pMsg->WriteStringA(GrantNameData1);


						CShardNetManager::BroadcastMsgToGameServers(pMsg);
					}
					else
						actionResult = FETCH_ACTION_STATE::PARAMS_NOT_SUPPLIED;
				} break;
				case KmtGameServerCommand::ActionSpawnAtPosition:
				{
					// Read & check params

					if (m_dbLink.sqlCmd.GetData(3, SQL_C_LONG, &MobID, 0, NULL)
						&& m_dbLink.sqlCmd.GetData(4, SQL_C_LONG, &GameWorldID, 0, NULL)
						&& m_dbLink.sqlCmd.GetData(5, SQL_C_LONG, &RegionId, 0, NULL)
						&& m_dbLink.sqlCmd.GetData(6, SQL_C_LONG, &PosX, 0, NULL)
						&& m_dbLink.sqlCmd.GetData(7, SQL_C_LONG, &PosY, 0, NULL)
						&& m_dbLink.sqlCmd.GetData(8, SQL_C_LONG, &PosZ, 0, NULL)
						&& m_dbLink.sqlCmd.GetData(9, SQL_C_LONG, &GenerateRadius, 0, NULL)
						&& MobID > 0 && IsValidDestination(GameWorldID, RegionId, PosX, PosY, PosZ)
						&& KmtGameServerCommand::IsValidSpawnRadius(GenerateRadius))
					{
						CMsgStreamBuffer* pMsg = CShardNetManager::AllocMsgForGS();
						pMsg->SetMsgID(0x8888);

						*pMsg << BYTE(KmtGameServerCommand::ActionSpawnAtPosition);
						*pMsg << MobID << GameWorldID << RegionId << PosX << PosY << PosZ << GenerateRadius;

						CShardNetManager::BroadcastMsgToGameServers(pMsg);
					}
					else
						actionResult = FETCH_ACTION_STATE::PARAMS_NOT_SUPPLIED;
				}
				break;
				case 3: // spawn mob by nearbymob
				{
					// Read & check params
					if (m_dbLink.sqlCmd.GetData(3, SQL_C_LONG, &CharID, 0, NULL) &&
						m_dbLink.sqlCmd.GetData(4, SQL_C_LONG, &MobID, 0, NULL) && CharID > 0 && MobID > 0)
					{
						SQLINTEGER regionIndicator = 0;
						SQLINTEGER posXIndicator = 0;
						SQLINTEGER posYIndicator = 0;
						SQLINTEGER posZIndicator = 0;
						SQLINTEGER radiusIndicator = 0;
						const bool hasRegion = m_dbLink.sqlCmd.GetData(5, SQL_C_LONG, &RegionId, 0, &regionIndicator) && regionIndicator != SQL_NULL_DATA;
						const bool hasPosX = m_dbLink.sqlCmd.GetData(6, SQL_C_LONG, &PosX, 0, &posXIndicator) && posXIndicator != SQL_NULL_DATA;
						const bool hasPosY = m_dbLink.sqlCmd.GetData(7, SQL_C_LONG, &PosY, 0, &posYIndicator) && posYIndicator != SQL_NULL_DATA;
						const bool hasPosZ = m_dbLink.sqlCmd.GetData(8, SQL_C_LONG, &PosZ, 0, &posZIndicator) && posZIndicator != SQL_NULL_DATA;
						const bool hasRadius = m_dbLink.sqlCmd.GetData(9, SQL_C_LONG, &GenerateRadius, 0, &radiusIndicator) && radiusIndicator != SQL_NULL_DATA;
						const bool hasAnyPosition = hasRegion || hasPosX || hasPosY || hasPosZ || hasRadius;
						const bool hasCompletePosition = hasRegion && hasPosX && hasPosY && hasPosZ && hasRadius;

						if (hasAnyPosition && (!hasCompletePosition ||
							!KmtGameServerCommand::IsValidRegionId(RegionId) ||
							!KmtGameServerCommand::IsValidCoordinate(PosX) ||
							!KmtGameServerCommand::IsValidCoordinate(PosY) ||
							!KmtGameServerCommand::IsValidCoordinate(PosZ) ||
							!KmtGameServerCommand::IsValidSpawnRadius(GenerateRadius)))
						{
							actionResult = FETCH_ACTION_STATE::PARAMS_NOT_SUPPLIED;
							break;
						}

						CMsgStreamBuffer* pMsg = CShardNetManager::AllocMsgForGS();
						pMsg->SetMsgID(0x8888);

						if (hasCompletePosition)
						{
							*pMsg << BYTE(KmtGameServerCommand::ActionSpawnAtPositionInPlayerWorld);
							*pMsg << (int)CharID << (int)MobID << RegionId << PosX << PosY << PosZ << GenerateRadius;
						}
						else
						{
							*pMsg << BYTE(KmtGameServerCommand::ActionSpawnNearPlayer);
							*pMsg << (int)CharID << (int)MobID;
						}

						CShardNetManager::BroadcastMsgToGameServers(pMsg);
					}
					else
						actionResult = FETCH_ACTION_STATE::PARAMS_NOT_SUPPLIED;
				} break;
				case 4: // remove monster
				{
					// Read & check params
					SQLUINTEGER MobID;
					if (m_dbLink.sqlCmd.GetData(3, SQL_C_LONG, &MobID, 0, NULL))
					{
						CMsgStreamBuffer* pMsg = CShardNetManager::AllocMsgForGS();
						pMsg->SetMsgID(0x8888);

						*pMsg << BYTE(4);
						*pMsg << MobID;

						CShardNetManager::BroadcastMsgToGameServers(pMsg);
					}
					else
						actionResult = FETCH_ACTION_STATE::PARAMS_NOT_SUPPLIED;
				} break;
				case 5: // remove monster by worldid
				{
					// Read & check params
					SQLUINTEGER WorldID;
					SQLUINTEGER MobID;
					if (m_dbLink.sqlCmd.GetData(3, SQL_C_LONG, &WorldID, 0, NULL) && m_dbLink.sqlCmd.GetData(4, SQL_C_LONG, &MobID, 0, NULL))
					{
						CMsgStreamBuffer* pMsg = CShardNetManager::AllocMsgForGS();
						pMsg->SetMsgID(0x8888);

						*pMsg << BYTE(5);
						*pMsg << WorldID << MobID;

						CShardNetManager::BroadcastMsgToGameServers(pMsg);
					}
					else
						actionResult = FETCH_ACTION_STATE::PARAMS_NOT_SUPPLIED;
				} break;

				case 6: // send live skill
				{
					// Read & check params
					SQLUINTEGER SkillID;
					if (m_dbLink.sqlCmd.GetData(3, SQL_C_LONG, &CharID, 0, NULL)
						&& m_dbLink.sqlCmd.GetData(4, SQL_C_LONG, &SkillID, 0, NULL))
					{
						CMsgStreamBuffer* pMsg = CShardNetManager::AllocMsgForGS();
						pMsg->SetMsgID(0x8888);

						*pMsg << BYTE(6);
						*pMsg << CharID << SkillID;

						CShardNetManager::BroadcastMsgToGameServers(pMsg);
					}
					else
						actionResult = FETCH_ACTION_STATE::PARAMS_NOT_SUPPLIED;
				} break;
				case 7: // remove live skill
				{
					// Read & check params
					SQLUINTEGER SkillID2;
					if (m_dbLink.sqlCmd.GetData(3, SQL_C_LONG, &CharID, 0, NULL)
						&& m_dbLink.sqlCmd.GetData(4, SQL_C_LONG, &SkillID2, 0, NULL))
					{
						CMsgStreamBuffer* pMsg = CShardNetManager::AllocMsgForGS();
						pMsg->SetMsgID(0x8888);

						*pMsg << BYTE(7);
						*pMsg << CharID << SkillID2;

						CShardNetManager::BroadcastMsgToGameServers(pMsg);
					}
					else
						actionResult = FETCH_ACTION_STATE::PARAMS_NOT_SUPPLIED;
				} break;
				case 8: // add live skill by code name
				{
					// Read & check params
					char SkillCodeName[128] = { 0 };
					if (m_dbLink.sqlCmd.GetData(3, SQL_C_LONG, &CharID, 0, NULL)
						&& m_dbLink.sqlCmd.GetData(4, SQL_C_CHAR, &SkillCodeName, 128, NULL)
						&& CharID > 0 && SkillCodeName[0] != '\0')
					{
						CMsgStreamBuffer* pMsg = CShardNetManager::AllocMsgForGS();
						pMsg->SetMsgID(0x8888);

						*pMsg << BYTE(8);
						*pMsg << CharID << SkillCodeName;

						CShardNetManager::BroadcastMsgToGameServers(pMsg);
					}
					else
						actionResult = FETCH_ACTION_STATE::PARAMS_NOT_SUPPLIED;
				} break;

				case 10: // TO TOWN
				{
					// Read & check params
					if (m_dbLink.sqlCmd.GetData(3, SQL_C_LONG, &CharID, 0, NULL)
						)
					{
						CMsgStreamBuffer* pMsg = CShardNetManager::AllocMsgForGS();
						pMsg->SetMsgID(0x8888);

						*pMsg << BYTE(10);
						*pMsg << CharID;

						CShardNetManager::BroadcastMsgToGameServers(pMsg);
					}
					else
						actionResult = FETCH_ACTION_STATE::PARAMS_NOT_SUPPLIED;
				} break;
				case 11: // TO TOWN
				{
					// Read & check params
					SQLUINTEGER WorldID2;
					if (m_dbLink.sqlCmd.GetData(3, SQL_C_LONG, &WorldID2, 0, NULL)
						)
					{
						CMsgStreamBuffer* pMsg = CShardNetManager::AllocMsgForGS();
						pMsg->SetMsgID(0x8888);

						*pMsg << BYTE(11);
						*pMsg << WorldID2;

						CShardNetManager::BroadcastMsgToGameServers(pMsg);
					}
					else
						actionResult = FETCH_ACTION_STATE::PARAMS_NOT_SUPPLIED;
				} break;
				case 12: // TO TOWN
				{
					// Read & check params
					SQLUINTEGER PetUniqueID;
					SQLUINTEGER PetSkillID;
					if (m_dbLink.sqlCmd.GetData(3, SQL_C_LONG, &PetUniqueID, 0, NULL)
						&& m_dbLink.sqlCmd.GetData(4, SQL_C_LONG, &PetSkillID, 0, NULL)
						)
					{
						CMsgStreamBuffer* pMsg = CShardNetManager::AllocMsgForGS();
						pMsg->SetMsgID(0x8888);

						*pMsg << BYTE(12);
						*pMsg << PetUniqueID << PetSkillID;

						CShardNetManager::BroadcastMsgToGameServers(pMsg);
					}
					else
						actionResult = FETCH_ACTION_STATE::PARAMS_NOT_SUPPLIED;
				} break;
				case 13: // cape
				{
					// Read & check params
					SQLUINTEGER CapeID;
					if (m_dbLink.sqlCmd.GetData(3, SQL_C_LONG, &CharID, 0, NULL)
						&& m_dbLink.sqlCmd.GetData(4, SQL_C_LONG, &CapeID, 0, NULL)
						&& CharID > 0 && CapeID <= 5
						)
					{
						CMsgStreamBuffer* pMsg = CShardNetManager::AllocMsgForGS();
						pMsg->SetMsgID(0x8888);

						*pMsg << BYTE(13);
						*pMsg << CharID << CapeID;

						CShardNetManager::BroadcastMsgToGameServers(pMsg);
					}
					else
						actionResult = FETCH_ACTION_STATE::PARAMS_NOT_SUPPLIED;
				} break;
				case KmtGameServerCommand::ActionMovePlayer:
				{
					SQLINTEGER WorldID3, RegionIds, PosXx, PosYy, PosZz;
					if (m_dbLink.sqlCmd.GetData(3, SQL_C_LONG, &CharID, 0, NULL)
						&& m_dbLink.sqlCmd.GetData(4, SQL_C_LONG, &WorldID3, 0, NULL)
						&& m_dbLink.sqlCmd.GetData(5, SQL_C_LONG, &RegionIds, 0, NULL)
						&& m_dbLink.sqlCmd.GetData(6, SQL_C_LONG, &PosXx, 0, NULL)
						&& m_dbLink.sqlCmd.GetData(7, SQL_C_LONG, &PosYy, 0, NULL)
						&& m_dbLink.sqlCmd.GetData(8, SQL_C_LONG, &PosZz, 0, NULL)
						&& CharID > 0 && IsValidDestination(WorldID3, RegionIds, PosXx, PosYy, PosZz))
					{
						CMsgStreamBuffer* pMsg = CShardNetManager::AllocMsgForGS();
						pMsg->SetMsgID(0x8888);

						*pMsg << BYTE(KmtGameServerCommand::ActionMovePlayer);
						*pMsg << CharID << WorldID3 << RegionIds << PosXx << PosYy << PosZz;

						CShardNetManager::BroadcastMsgToGameServers(pMsg);
					}
					else
						actionResult = FETCH_ACTION_STATE::PARAMS_NOT_SUPPLIED;
				} break;
				case 15: // cape
				{
					// Read & check params
					SQLUINTEGER CapeID;
					if (m_dbLink.sqlCmd.GetData(3, SQL_C_LONG, &CharID, 0, NULL))
					{
						CMsgStreamBuffer* pMsg = CShardNetManager::AllocMsgForGS();
						pMsg->SetMsgID(0x8888);

						*pMsg << BYTE(15);
						*pMsg << CharID;

						CShardNetManager::BroadcastMsgToGameServers(pMsg);
					}
					else
						actionResult = FETCH_ACTION_STATE::PARAMS_NOT_SUPPLIED;
				} break;
				case 16: // exp
				{
					// Read & check params
					SQLUINTEGER ExpRate;
					if (m_dbLink.sqlCmd.GetData(3, SQL_C_LONG, &CharID, 0, NULL)
						&& m_dbLink.sqlCmd.GetData(4, SQL_C_LONG, &ExpRate, 0, NULL))
					{
						CMsgStreamBuffer* pMsg = CShardNetManager::AllocMsgForGS();
						pMsg->SetMsgID(0x8888);

						*pMsg << BYTE(16);
						*pMsg << CharID << (unsigned int)ExpRate;
						CShardNetManager::BroadcastMsgToGameServers(pMsg);
					}
					else
						actionResult = FETCH_ACTION_STATE::PARAMS_NOT_SUPPLIED;
				} break;

				case KmtGameServerCommand::ActionChangeItem:
				{
					// Read & check params
					char MutateItemName[128] = { 0 };
					SQLINTEGER Slot;
					if (m_dbLink.sqlCmd.GetData(3, SQL_C_LONG, &CharID, 0, NULL) &&
						m_dbLink.sqlCmd.GetData(4, SQL_C_LONG, &Slot, 0, NULL)
						&& m_dbLink.sqlCmd.GetData(5, SQL_C_CHAR, &MutateItemName, 128, 0)
						&& CharID > 0 && KmtGameServerCommand::IsValidInventorySlotWireValue(Slot)
						&& MutateItemName[0] != '\0')
					{
						CMsgStreamBuffer* pMsg = CShardNetManager::AllocMsgForGS();
						pMsg->SetMsgID(0x8888);

						*pMsg << BYTE(KmtGameServerCommand::ActionChangeItem);
						*pMsg << CharID;
						*pMsg << Slot;
						pMsg->WriteStringA(MutateItemName);
						CShardNetManager::BroadcastMsgToGameServers(pMsg);
					}
					else
						actionResult = FETCH_ACTION_STATE::PARAMS_NOT_SUPPLIED;
				} break;

				case KmtGameServerCommand::ActionConsumeItem:
				{
					SQLINTEGER Slot2;
					SQLINTEGER Amount;
					if (m_dbLink.sqlCmd.GetData(3, SQL_C_LONG, &CharID, 0, NULL) &&
						m_dbLink.sqlCmd.GetData(4, SQL_C_LONG, &Slot2, 0, NULL)
						&& m_dbLink.sqlCmd.GetData(5, SQL_C_LONG, &Amount, 0, NULL)
						&& CharID > 0 && KmtGameServerCommand::IsValidInventorySlotWireValue(Slot2)
						&& Amount > 0)
					{
						CMsgStreamBuffer* pMsg = CShardNetManager::AllocMsgForGS();
						pMsg->SetMsgID(0x8888);

						*pMsg << BYTE(KmtGameServerCommand::ActionConsumeItem);
						*pMsg << CharID;
						*pMsg << Slot2;
						*pMsg << Amount;
						CShardNetManager::BroadcastMsgToGameServers(pMsg);
					}
					else
						actionResult = FETCH_ACTION_STATE::PARAMS_NOT_SUPPLIED;
				} break;

				case KmtGameServerCommand::ActionConsumeAndChangeItem:
				{
					char MutateItemName[128] = { 0 };
					SQLINTEGER MutateSlot;
					SQLINTEGER ConsumeSlot;
					SQLINTEGER ConsumeAmount;
					if (m_dbLink.sqlCmd.GetData(3, SQL_C_LONG, &CharID, 0, NULL) &&
						m_dbLink.sqlCmd.GetData(4, SQL_C_LONG, &MutateSlot, 0, NULL)
						&& m_dbLink.sqlCmd.GetData(5, SQL_C_CHAR, &MutateItemName, 128, 0)
						&& m_dbLink.sqlCmd.GetData(6, SQL_C_LONG, &ConsumeSlot, 0, NULL)
						&& m_dbLink.sqlCmd.GetData(7, SQL_C_LONG, &ConsumeAmount, 0, NULL)
						&& CharID > 0 && KmtGameServerCommand::IsValidInventorySlotWireValue(MutateSlot)
						&& KmtGameServerCommand::IsValidInventorySlotWireValue(ConsumeSlot) && MutateSlot != ConsumeSlot
						&& ConsumeAmount > 0 && MutateItemName[0] != '\0')
					{
						CMsgStreamBuffer* pMsg = CShardNetManager::AllocMsgForGS();
						pMsg->SetMsgID(0x8888);

						*pMsg << BYTE(KmtGameServerCommand::ActionConsumeAndChangeItem);
						*pMsg << CharID;
						*pMsg << MutateSlot;
						pMsg->WriteStringA(MutateItemName);
						*pMsg << ConsumeSlot;
						*pMsg << ConsumeAmount;
						CShardNetManager::BroadcastMsgToGameServers(pMsg);
					}
					else
						actionResult = FETCH_ACTION_STATE::PARAMS_NOT_SUPPLIED;

				} break;

				case 20: // live silk
				{
					SQLINTEGER nSilk;
					SQLINTEGER nSilkGift;
					SQLINTEGER nSilkPoint;
					if (m_dbLink.sqlCmd.GetData(3, SQL_C_LONG, &CharID, 0, NULL) &&
						m_dbLink.sqlCmd.GetData(4, SQL_C_LONG, &nSilk, 0, NULL)
						&& m_dbLink.sqlCmd.GetData(5, SQL_C_LONG, &nSilkGift, 0, NULL)
						&& m_dbLink.sqlCmd.GetData(6, SQL_C_LONG, &nSilkPoint, 0, NULL))
					{
						CMsgStreamBuffer* pMsg = CShardNetManager::AllocMsgForGS();
						pMsg->SetMsgID(0x8888);

						*pMsg << BYTE(20);
						*pMsg << CharID;
						*pMsg << nSilk;
						*pMsg << nSilkGift;
						*pMsg << nSilkPoint;
						CShardNetManager::BroadcastMsgToGameServers(pMsg);
					}
					else
						actionResult = FETCH_ACTION_STATE::PARAMS_NOT_SUPPLIED;

				} break;
				case KmtGameServerCommand::ActionGold:
				{
					SQLBIGINT nGold;
					SQLINTEGER AddOrRemove;

					if (m_dbLink.sqlCmd.GetData(3, SQL_C_LONG, &CharID, 0, NULL) &&
						m_dbLink.sqlCmd.GetData(4, SQL_C_SBIGINT, &nGold, 0, NULL) && // SQL_C_SBIGINT kullanýldý
						m_dbLink.sqlCmd.GetData(5, SQL_C_LONG, &AddOrRemove, 0, NULL)
						&& CharID > 0 && KmtGameServerCommand::IsValidGoldRequest(nGold, AddOrRemove))
					{
						CMsgStreamBuffer* pMsg = CShardNetManager::AllocMsgForGS();
						pMsg->SetMsgID(0x8888);

						*pMsg << BYTE(KmtGameServerCommand::ActionGold);
						*pMsg << CharID;
						*pMsg << nGold;
						*pMsg << AddOrRemove;
						CShardNetManager::BroadcastMsgToGameServers(pMsg);
					}

					else
						actionResult = FETCH_ACTION_STATE::PARAMS_NOT_SUPPLIED;

				} break;
				case KmtGameServerCommand::ActionTownWorldLayer:
				{
					// Read & check params
					SQLUINTEGER WorldID3;
					SQLUINTEGER LayerID;
					if (m_dbLink.sqlCmd.GetData(3, SQL_C_LONG, &WorldID3, 0, NULL)
						&& m_dbLink.sqlCmd.GetData(4, SQL_C_LONG, &LayerID, 0, NULL)
						&& KmtGameServerCommand::IsValidWorldId(static_cast<int>(WorldID3))
						&& KmtGameServerCommand::IsValidLayerId(static_cast<int>(LayerID)))
					{
						CMsgStreamBuffer* pMsg = CShardNetManager::AllocMsgForGS();
						pMsg->SetMsgID(0x8888);

						*pMsg << BYTE(KmtGameServerCommand::ActionTownWorldLayer);
						*pMsg << WorldID3;
						*pMsg << LayerID;

						CShardNetManager::BroadcastMsgToGameServers(pMsg);
					}
					else
						actionResult = FETCH_ACTION_STATE::PARAMS_NOT_SUPPLIED;
				} break;
				case KmtGameServerCommand::ActionGetUpAtPosition:
				{
					SQLINTEGER WorldID3, RegionIds, PosXx, PosYy, PosZz;
					if (m_dbLink.sqlCmd.GetData(3, SQL_C_LONG, &CharID, 0, NULL)
						&& m_dbLink.sqlCmd.GetData(4, SQL_C_LONG, &WorldID3, 0, NULL)
						&& m_dbLink.sqlCmd.GetData(5, SQL_C_LONG, &RegionIds, 0, NULL)
						&& m_dbLink.sqlCmd.GetData(6, SQL_C_LONG, &PosXx, 0, NULL)
						&& m_dbLink.sqlCmd.GetData(7, SQL_C_LONG, &PosYy, 0, NULL)
						&& m_dbLink.sqlCmd.GetData(8, SQL_C_LONG, &PosZz, 0, NULL)
						&& CharID > 0 && IsValidDestination(WorldID3, RegionIds, PosXx, PosYy, PosZz))
					{
						CMsgStreamBuffer* pMsg = CShardNetManager::AllocMsgForGS();
						pMsg->SetMsgID(0x8888);

						*pMsg << BYTE(KmtGameServerCommand::ActionGetUpAtPosition);
						*pMsg << CharID << WorldID3 << RegionIds << PosXx << PosYy << PosZz;

						CShardNetManager::BroadcastMsgToGameServers(pMsg);
					}
					else
						actionResult = FETCH_ACTION_STATE::PARAMS_NOT_SUPPLIED;
				} break;
				case KmtGameServerCommand::ActionMovePlayerAtPosition:
				{
					SQLINTEGER WorldID3, RegionIds, PosXx, PosYy, PosZz;
					if (m_dbLink.sqlCmd.GetData(3, SQL_C_LONG, &CharID, 0, NULL)
						&& m_dbLink.sqlCmd.GetData(4, SQL_C_LONG, &WorldID3, 0, NULL)
						&& m_dbLink.sqlCmd.GetData(5, SQL_C_LONG, &RegionIds, 0, NULL)
						&& m_dbLink.sqlCmd.GetData(6, SQL_C_LONG, &PosXx, 0, NULL)
						&& m_dbLink.sqlCmd.GetData(7, SQL_C_LONG, &PosYy, 0, NULL)
						&& m_dbLink.sqlCmd.GetData(8, SQL_C_LONG, &PosZz, 0, NULL)
						&& CharID > 0 && IsValidDestination(WorldID3, RegionIds, PosXx, PosYy, PosZz))
					{
						CMsgStreamBuffer* pMsg = CShardNetManager::AllocMsgForGS();
						pMsg->SetMsgID(0x8888);

						*pMsg << BYTE(KmtGameServerCommand::ActionMovePlayerAtPosition);
						*pMsg << CharID << WorldID3 << RegionIds << PosXx << PosYy << PosZz;

						CShardNetManager::BroadcastMsgToGameServers(pMsg);
					}
					else
						actionResult = FETCH_ACTION_STATE::PARAMS_NOT_SUPPLIED;
				} break;
				case KmtGameServerCommand::ActionTowerCombat:
				{
					SQLINTEGER enabled, worldId, regionId;
					SQLINTEGER team1MobId, team1Cape, team2MobId, team2Cape;
					if (m_dbLink.sqlCmd.GetData(3, SQL_C_LONG, &enabled, 0, NULL)
						&& m_dbLink.sqlCmd.GetData(4, SQL_C_LONG, &worldId, 0, NULL)
						&& m_dbLink.sqlCmd.GetData(5, SQL_C_LONG, &regionId, 0, NULL)
						&& m_dbLink.sqlCmd.GetData(6, SQL_C_LONG, &team1MobId, 0, NULL)
						&& m_dbLink.sqlCmd.GetData(7, SQL_C_LONG, &team1Cape, 0, NULL)
						&& m_dbLink.sqlCmd.GetData(8, SQL_C_LONG, &team2MobId, 0, NULL)
						&& m_dbLink.sqlCmd.GetData(9, SQL_C_LONG, &team2Cape, 0, NULL)
						&& (enabled == 0 || (enabled == 1 && KmtGameServerCommand::IsValidWorldId(worldId)
							&& KmtGameServerCommand::IsValidRegionId(regionId) && team1MobId > 0 && team2MobId > 0
							&& team1Cape >= 0 && team1Cape <= 5 && team2Cape >= 0 && team2Cape <= 5)))
					{
						CMsgStreamBuffer* pMsg = CShardNetManager::AllocMsgForGS();
						pMsg->SetMsgID(0x8888);
						*pMsg << BYTE(KmtGameServerCommand::ActionTowerCombat);
						*pMsg << enabled << worldId << regionId
							  << team1MobId << team1Cape << team2MobId << team2Cape;
						CShardNetManager::BroadcastMsgToGameServers(pMsg);
					}
					else
							actionResult = FETCH_ACTION_STATE::PARAMS_NOT_SUPPLIED;
				} break;
				case KmtGameServerCommand::ActionFreeForAllCombat:
				{
					SQLINTEGER enabled, worldId, regionId;
					if (m_dbLink.sqlCmd.GetData(3, SQL_C_LONG, &enabled, 0, NULL)
						&& m_dbLink.sqlCmd.GetData(4, SQL_C_LONG, &worldId, 0, NULL)
						&& m_dbLink.sqlCmd.GetData(5, SQL_C_LONG, &regionId, 0, NULL)
						&& (enabled == 0 || (enabled == 1 && KmtGameServerCommand::IsValidWorldId(worldId)
							&& KmtGameServerCommand::IsValidRegionId(regionId))))
					{
						CMsgStreamBuffer* pMsg = CShardNetManager::AllocMsgForGS();
						pMsg->SetMsgID(0x8888);
						*pMsg << BYTE(KmtGameServerCommand::ActionFreeForAllCombat);
						*pMsg << enabled << worldId << regionId;
						CShardNetManager::BroadcastMsgToGameServers(pMsg);
					}
					else
						actionResult = FETCH_ACTION_STATE::PARAMS_NOT_SUPPLIED;
				} break;
#if 0 // Action 131 is retired and rejected by the durable queue migration.
				case 131:
				{
					SQLINTEGER nItemID;
					SQLINTEGER nPlus;
					SQLINTEGER nSlot;
					SQLINTEGER nAdvLevel;
					if (m_dbLink.sqlCmd.GetData(3, SQL_C_LONG, &CharID, 0, NULL) &&
						m_dbLink.sqlCmd.GetData(4, SQL_C_LONG, &nItemID, 0, NULL)
						&& m_dbLink.sqlCmd.GetData(5, SQL_C_LONG, &nPlus, 0, NULL)
						&& m_dbLink.sqlCmd.GetData(6, SQL_C_LONG, &nSlot, 0, NULL)
						&& m_dbLink.sqlCmd.GetData(7, SQL_C_LONG, &nAdvLevel, 0, NULL))
					{
						CMsgStreamBuffer* pMsg = CShardNetManager::AllocMsgForGS();
						pMsg->SetMsgID(0x8888);

						*pMsg << BYTE(131);
						*pMsg << CharID;
						*pMsg << nItemID;
						*pMsg << (byte)nPlus;
						*pMsg << (byte)nSlot;
						*pMsg << (byte)nAdvLevel;

						CShardNetManager::BroadcastMsgToGameServers(pMsg);
					}
					else
						actionResult = FETCH_ACTION_STATE::PARAMS_NOT_SUPPLIED;

				} break;
#endif
				default:
					ShardManagerConsole::WriteFormat(
						3, "Rejected unsupported GameServer action ID %ld", static_cast<long>(cActionID));
					actionResult = FETCH_ACTION_STATE::ACTION_UNDEFINED;
					break;
				}
			}
			catch (const std::exception& ex)
			{
				ShardManagerConsole::WriteFormat(
					4, "GameServer command exception: command=%I64d action=%ld detail=%s",
					cID, static_cast<long>(cActionID), ex.what());
				actionResult = FETCH_ACTION_STATE::UNNEXPECTED_EXCEPTION;
			}
			catch (...)
			{
				ShardManagerConsole::WriteFormat(
					4, "GameServer command exception: command=%I64d action=%ld detail=unknown",
					cID, static_cast<long>(cActionID));
				actionResult = FETCH_ACTION_STATE::UNNEXPECTED_EXCEPTION;
			}

			if (!m_dbLink.sqlCmd.GetData(15, SQL_C_CHAR, claimToken, sizeof(claimToken), NULL) ||
				!m_dbLink.sqlCmd.GetData(16, SQL_C_LONG, &attemptCount, 0, NULL))
			{
				RecordCommandDatabaseFailure("read-claim-metadata", cID, cActionID);
				ShardManagerConsole::WriteFormat(
					4, "Claimed GameServer command has invalid claim metadata: command=%I64d action=%ld",
					cID, static_cast<long>(cActionID));
				continue;
			}
			const bool broadcastCompleted = CommandDispatchGuard::TryConsumeBroadcast();
			bool acknowledgementSucceeded = false;
			LONG recordedResult = 0;
			if (actionResult == FETCH_ACTION_STATE::SUCCESS && broadcastCompleted)
			{
				acknowledgementSucceeded = CompleteClaimWithRecovery(
					cID, cActionID, claimToken, "Dispatched", 0,
					"Validated and broadcast by ShardManager.");
				InterlockedIncrement(&s_commandsDispatched);
				recordedResult = acknowledgementSucceeded ? 1 : 5;
			}
			else if (actionResult == FETCH_ACTION_STATE::ACTION_UNDEFINED ||
				actionResult == FETCH_ACTION_STATE::PARAMS_NOT_SUPPLIED ||
				(actionResult == FETCH_ACTION_STATE::SUCCESS && !broadcastCompleted))
			{
				acknowledgementSucceeded = CompleteClaimWithRecovery(
					cID, cActionID, claimToken, "Rejected", static_cast<int>(actionResult),
					"Unsupported or invalid GameServer command payload.");
				InterlockedIncrement(&s_commandsRejected);
				recordedResult = acknowledgementSucceeded ? 2 : 5;
			}
			else if (broadcastCompleted)
			{
				acknowledgementSucceeded = CompleteClaimWithRecovery(
					cID, cActionID, claimToken, "Indeterminate", static_cast<int>(actionResult),
					"Broadcast completed before command processing failed.");
				InterlockedIncrement(&s_commandsFailed);
				recordedResult = acknowledgementSucceeded ? 4 : 5;
			}
			else
			{
				acknowledgementSucceeded = RetryClaimWithRecovery(
					cID, cActionID, claimToken, "GameServer command dispatch failed before broadcast.");
				InterlockedIncrement(&s_commandsFailed);
				recordedResult = acknowledgementSucceeded ? 3 : 5;
			}

			if (!acknowledgementSucceeded)
			{
				InterlockedIncrement(&s_commandsFailed);
				ShardManagerConsole::WriteFormat(
					4, "GameServer command acknowledgement failed without blocking the queue: command=%I64d action=%ld attempt=%ld",
					cID, static_cast<long>(cActionID), static_cast<long>(attemptCount));
			}

			const LONG durationMs = static_cast<LONG>(GetTickCount() - commandStarted);
			InterlockedExchange64(&s_lastCommandId, cID);
			InterlockedExchange(&s_lastCommandAction, cActionID);
			InterlockedExchange(&s_lastCommandResult, recordedResult);
			InterlockedExchange(&s_lastCommandDurationMs, durationMs);
			InterlockedExchange(&s_lastCommandProgressTick, static_cast<LONG>(GetTickCount()));
			if (acknowledgementSucceeded && recordedResult != 1)
			{
				ShardManagerConsole::WriteFormat(
					3, "GameServer command completed without dispatch: command=%I64d action=%ld result=%s attempt=%ld duration_ms=%ld",
					cID, static_cast<long>(cActionID), CommandResultName(recordedResult),
					static_cast<long>(attemptCount), static_cast<long>(durationMs));
			}

			++commandsInRateWindow;
			const DWORD rateNow = GetTickCount();
			if (rateNow - rateWindowStarted >= 1000)
			{
				rateWindowStarted = rateNow;
				commandsInRateWindow = 0;
			}
			else if (commandsInRateWindow >= MAX_COMMANDS_PER_SECOND)
			{
				const DWORD remaining = 1000 - (rateNow - rateWindowStarted);
				if (WaitForCommandStop(remaining))
					break;
				rateWindowStarted = GetTickCount();
				commandsInRateWindow = 0;
			}
		}
		m_dbLink.sqlCmd.Clear();

		if (fetchResult == SQL_FETCH_ERROR)
		{
			RecordCommandDatabaseFailure("fetch", 0, 0);
			ReconnectDatabaseLink(m_dbLink, "claim");
			if (WaitForCommandStop(COMMAND_DB_RETRY_DELAY_MS))
				break;
		}
		else if (!fetchedCommand && WaitForCommandStop(COMMAND_IDLE_POLL_DELAY_MS))
			break;
		}
		catch (const std::exception& ex)
		{
			RecordCommandDatabaseFailure("worker-loop-exception", activeCommandId, activeActionId);
			ShardManagerConsole::WriteFormat(
				4, "GameServer command worker recovered from an exception: command=%I64d action=%ld detail=%s",
				activeCommandId, static_cast<long>(activeActionId), ex.what());
			m_dbLink.sqlCmd.Clear();
			ReconnectDatabaseLink(m_dbLink, "claim");
			if (WaitForCommandStop(COMMAND_DB_RETRY_DELAY_MS))
				break;
		}
		catch (...)
		{
			RecordCommandDatabaseFailure("worker-loop-exception", activeCommandId, activeActionId);
			ShardManagerConsole::WriteFormat(
				4, "GameServer command worker recovered from an unknown exception: command=%I64d action=%ld",
				activeCommandId, static_cast<long>(activeActionId));
			m_dbLink.sqlCmd.Clear();
			ReconnectDatabaseLink(m_dbLink, "claim");
			if (WaitForCommandStop(COMMAND_DB_RETRY_DELAY_MS))
				break;
		}
	}

	// Stop flag
	InterlockedExchange(&m_IsRunningDatabaseFetch, 0);
	ShardManagerConsole::WriteWarning("GameServer command bridge stopped");

	return 0;
}

std::unordered_map<INT64, int> AsyncGSCommands::m_LockedItems;

void AsyncGSCommands::LoadLockedItemList()
{
	std::wstringstream qSelectActions;
	qSelectActions << L"SELECT ItemID64, Password FROM [KMTGuard].[dbo].[Item_Locked]";

	INT64 ItemID64 = 0;
	int Password = 0;

	if (AsyncGSCommands::m_dbLink.sqlConn.IsOpen())
	{
		SQLHSTMT hStmt = AsyncGSCommands::m_dbLink.sqlCmd.GetStmtHandle();

		// Prepare and execute the query
		if (SQLPrepare(hStmt, (SQLWCHAR*)qSelectActions.str().c_str(), SQL_NTS) == SQL_SUCCESS)
		{
			if (SQLExecute(hStmt) == SQL_SUCCESS)
			{
				SQLRETURN retcode;

				// Bind the columns
				SQLBindCol(hStmt, 1, SQL_C_SBIGINT, &ItemID64, sizeof(ItemID64), NULL);
				SQLBindCol(hStmt, 2, SQL_C_LONG, &Password, sizeof(Password), NULL);

				// Fetch and store the results
				while ((retcode = SQLFetch(hStmt)) != SQL_NO_DATA)
				{
					m_LockedItems[ItemID64] = Password;
				}

				// Cleanup
				SQLCloseCursor(hStmt);
			}
			else
			{
				// Sorgu baþarýsýz oldu
				ShardManagerConsole::WriteFailure("Locked-item query execution failed");
			}
		}
		else
		{
			// Sorgu hazýrlama baþarýsýz oldu
			ShardManagerConsole::WriteFailure("Locked-item query preparation failed");
		}

		// Release the statement handle
		AsyncGSCommands::m_dbLink.sqlCmd.ReleaseStmtHandle(hStmt);
	}
	else
	{
		// Veritabaný baðlantýsý açýk deðil
		ShardManagerConsole::WriteFailure("Locked-item database connection is unavailable");
	}
}
