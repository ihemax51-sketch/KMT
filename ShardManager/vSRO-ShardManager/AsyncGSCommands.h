#pragma once
#include "Database/SQLConnection.h"
#include "Database/SQLCommand.h"
#include <unordered_map>

// All states fetching can generate
enum FETCH_ACTION_STATE {
	UNKNOWN = 0,
	SUCCESS = 1,
	ACTION_UNDEFINED = 2,
	UNNEXPECTED_EXCEPTION = 3,
	PARAMS_NOT_SUPPLIED = 4,
	CHARNAME_NOT_FOUND = 5,
	FUNCTION_ERROR = 6
};

// Handlers required to make a database link
struct DatabaseLink {
	SQLConnection sqlConn;
	SQLCommand sqlCmd;
};


// Application Manager sharing info to any place in the project
class AsyncGSCommands
{
private: // Private members
	// Check if app has been initialized
	static volatile LONG m_IsInitialized;
public: // Public Methods
	// Initialize manager
	static bool Initialize();
	static void Shutdown();


	// Handlers for SQL communication
	static DatabaseLink m_dbLink, m_dbLinkHelper, m_dbUniqueLog;
	// Flag to keep thread safe
	static volatile LONG m_IsRunningDatabaseFetch;
	// Keeps in memory the value assigned

	static DWORD WINAPI DatabaseFetchThread(LPVOID parameter);

	static void LoadLockedItemList();

	static bool InitDatabaseFetch();


	static std::unordered_map<INT64, int> m_LockedItems;
private: // Private Helpers
};
