#include "MsgStreamBuffer.h"
#include "../Utils/Memory/detours.h"
#pragma comment(lib, "Network/detours.lib")
#define MSG_HANDLE CMsgStreamBuffer *pMsg, DWORD dwOverlappedJobID, LPVOID lpParam, CMassiveMsg *pMassiveMsg

class CMassiveMsg;

class CMainProcess {
public:
	// Impl
	void _OnProcessMessage(CMsgStreamBuffer* pMsg, DWORD dwOverlappedJobID, LPVOID lpParam, CMassiveMsg* pMassiveMsg);
	void _OnProcessMessageSafe(CMsgStreamBuffer* pMsg, DWORD dwOverlappedJobID, LPVOID lpParam, CMassiveMsg* pMassiveMsg);
	typedef void* (__thiscall* FN_MSG_HANDLER)(CMainProcess*, CMsgStreamBuffer* pMsg, DWORD dwOverlappedJobID, LPVOID lpParam, CMassiveMsg* pMassiveMsg);

	static void Initialize();
	static bool InitializeSafe();
	static void ShutdownSafe();
	static CMainProcess* GetInstance();
	static void RegisterMsgHandler(__int16 msgID, FN_MSG_HANDLER handler);

private:
	typedef CMainProcess* (__stdcall* FN_GET_INSTANCE)();

	//VFT FN 7 ?
	typedef __int16(__thiscall* FN_SET_MSG_HANDLER)(CMainProcess*, __int16 id, void* handler);

	typedef int(__thiscall* FN_HANDLE_MSG)(
		CMainProcess*, CMsgStreamBuffer* pMsg, DWORD dwOverlappedJobID, LPVOID lpParam, CMassiveMsg* pMassiveMsg
		);

	static FN_SET_MSG_HANDLER s_pfnSetMsgHandler;
	static FN_HANDLE_MSG s_pfnHandleMsg;
	static FN_GET_INSTANCE s_pfnGetInstance;

	static __int16 __fastcall MySetMsgHandler(CMainProcess* self, void* /* dummy edx */, __int16 id, void* handler);

	static int __fastcall MyHandleMsg(
		CMainProcess* self, void* /* dummy edx */, CMsgStreamBuffer* pMsg, DWORD dwOverlappedJobID, LPVOID lpParam, CMassiveMsg* pMassiveMsg
	);
private:
};
