#pragma once
#include <windows.h>
#include <iostream>
#include <vector>
#include <sstream>
#include <list>
#include <algorithm>
#include <assert.h>
#include <sstream>
#include <map>
#include <sql.h>
#include <sqlext.h>
#include <sqltypes.h>
#include <queue>
#include <deque>
#include <math.h>
#include <SqlConnection/sqlCon.h>

// This archived implementation touches live GameServer player/inventory state
// from a detached worker. It must remain disabled until it is redesigned to
// marshal every native object mutation onto the GameServer thread.
#ifndef KMT_ENABLE_UNSAFE_CUSTOM_TIMED_JOB_WORKER
#define KMT_ENABLE_UNSAFE_CUSTOM_TIMED_JOB_WORKER 0
#endif

#if KMT_ENABLE_UNSAFE_CUSTOM_TIMED_JOB_WORKER
#error The archived CustomTimedJobManager worker is unsafe and cannot be enabled.
#endif

class CGItem;
class CAutoCriticalSection;

#define __LST_TIMED_ITEM_PLUS_APPLIED		std::list<long long>
#define __LST_TIMED_ITEM_PLUS_APPLIED_IT	__LST_TIMED_ITEM_PLUS_APPLIED::iterator

class CCustomTimedJobManager
{
private:
	static void ConsumeTimedItemPlusRecords();

	static DWORD WINAPI ConsumeThreadWorker(LPVOID lpParam);
public:
	static void Setup();
	static void ConsumeTimedItemPlusRecordsDevill();
	static void CreateConsumeThread();
};
