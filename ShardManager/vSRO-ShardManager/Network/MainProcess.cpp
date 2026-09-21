#include "MainProcess.h"
#include "ShardNetManager.h"
#include "../AsyncGSCommands.h"
#include <sstream>

enum E_CHAT_TYPE : byte
{
	LOCAL_ALL = 1,
	PRIVATE = 2,
	LOCAL_GM = 3,
	PARTY = 4,
	GUILD = 5,
	GLOBAL = 6,
	NOTICE = 7,
	STALL = 9,
	UNION = 11,
	NPC = 13,
	ACADEMY = 16
};

#define READ_LOCK_INFO 0x5060
#define SEND_TO_ALL_GS_LOCK_INFO 0x5061
#define READ_UNLOCK_INFO 0x5062
#define SEND_TO_ALL_GS_UNLOCK_INFO 0x5063
void CMainProcess::_OnProcessMessage(CMsgStreamBuffer* pMsg, DWORD dwOverlappedJobID, LPVOID lpParam, CMassiveMsg* pMassiveMsg) {

	if (pMsg->GetID() == READ_LOCK_INFO) /// item locked
	{
		INT64 ID64 = pMsg->Read<INT64>();


		CMsgStreamBuffer* pMsg = CShardNetManager::AllocMsgForGS();
		pMsg->SetMsgID(SEND_TO_ALL_GS_LOCK_INFO); // item locked send
		*pMsg << ID64;
		CShardNetManager::BroadcastMsgToGameServers(pMsg);
		return;
	}
	else if (pMsg->GetID() == READ_UNLOCK_INFO) /// item unlocked
	{
		INT64 ID64 = pMsg->Read<INT64>();

		CMsgStreamBuffer* pMsg = CShardNetManager::AllocMsgForGS();
		pMsg->SetMsgID(SEND_TO_ALL_GS_UNLOCK_INFO); // item unlocked sended
		*pMsg << ID64;
		CShardNetManager::BroadcastMsgToGameServers(pMsg);
		return;
	}
	else if (pMsg->GetID() == 0x5065)
	{
		int CharID = pMsg->Read<int>();
		INT64 ID64 = pMsg->Read<INT64>();
		int OldOptLevel = pMsg->Read<int>();
		long endtime = pMsg->Read<long>();

		CMsgStreamBuffer* pMsg = CShardNetManager::AllocMsgForGS();
		pMsg->SetMsgID(0x5066);
		*pMsg << CharID;
		*pMsg << ID64;
		*pMsg << OldOptLevel;
		*pMsg << endtime;
		CShardNetManager::BroadcastMsgToGameServers(pMsg);
		return;
	}
	else if (pMsg->GetID() == 0x5067)
	{
		INT64 ID64 = pMsg->Read<INT64>();

		CMsgStreamBuffer* pMsg = CShardNetManager::AllocMsgForGS();
		pMsg->SetMsgID(0x5068);
		*pMsg << ID64;
		CShardNetManager::BroadcastMsgToGameServers(pMsg);
		return;
	}

	else if (pMsg->GetID() == 0x5068)
	{
		int SenderID = pMsg->Read<int>();
		byte eChatType = pMsg->Read<BYTE>();

		byte btChatIndex = pMsg->Read<BYTE>();

		std::string strReceiverName;
		if (eChatType == PRIVATE)
		{
			strReceiverName = pMsg->ReadStringA();
		}

		std::string strAsciiText = pMsg->ReadStringA();

		byte LinkedItemSlot = pMsg->Read<BYTE>();

		//printf("%d %d %d %s %s %d \n", SenderID, eChatType, btChatIndex, strReceiverName.c_str(), strAsciiText.c_str(), LinkedItemSlot);


		CMsgStreamBuffer* pMsg = CShardNetManager::AllocMsgForGS();
		pMsg->SetMsgID(0x5069);
		*pMsg << SenderID;
		*pMsg << eChatType;
		*pMsg << btChatIndex;
		if (eChatType == PRIVATE) {
			pMsg->WriteStringA(strReceiverName);
		}
		pMsg->WriteStringA(strAsciiText);
		*pMsg << LinkedItemSlot;
		CShardNetManager::BroadcastMsgToGameServers(pMsg);
		return;

	}
	printf("%p \n", pMsg->GetID());
	reinterpret_cast<void(__thiscall*)(CMainProcess*, CMsgStreamBuffer*, DWORD, LPVOID, CMassiveMsg*)>(0x0040dda0)(this, pMsg, dwOverlappedJobID, lpParam, pMassiveMsg);
}




void hexdump(void* ptr, int buflen) {
	unsigned char* buf = (unsigned char*)ptr;
	int i, j;
	for (i = 0; i < buflen; i += 16) {
		printf("%06x: ", i);
		for (j = 0; j < 16; j++)
			if (i + j < buflen)
				printf("%02x ", buf[i + j]);
			else
				printf("   ");
		printf(" ");
		for (j = 0; j < 16; j++)
			if (i + j < buflen)
				printf("%c", isprint(buf[i + j]) ? buf[i + j] : '.');
		printf("\n");
	}
}


int __fastcall CMainProcess::MyHandleMsg(
	CMainProcess* self, void* /* dummy edx */, CMsgStreamBuffer* pMsg, DWORD dwOverlappedJobID, LPVOID lpParam, CMassiveMsg* pMassiveMsg
)
{
	int res = 0;

	try
	{
		if (pMsg != NULL)
		{
			if (pMsg->GetID() == 0x7808)
			{

				__int16 oldReadPos = pMsg->GetReadPos();
				__int16 oldWritePos = pMsg->GetWritePos();

				__int16 bytesToRead = (oldWritePos - 6);

				pMsg->SetReadPos(6);

				WORD wContainedOpcode = pMsg->Read<WORD>();

				if (wContainedOpcode == 0x300C)
				{

					WORD wOpID = pMsg->Read<WORD>();

					if (wOpID == 0xC05)
					{
						DWORD dwRefObjID = pMsg->Read<DWORD>();

						std::wstringstream qSelectActions;
						qSelectActions << L"EXEC [KMTGuard].[dbo].[Hook_UniqueSpawn] " << dwRefObjID;

						if (AsyncGSCommands::m_dbLink.sqlConn.IsOpen())
						{
							if (!(AsyncGSCommands::m_dbLink.sqlCmd.ExecuteQuery((SQLWCHAR*)qSelectActions.str().c_str())))
							{
								// Sorgu baþarýsýz oldu
							}
						}
						else
						{
							// Veritabaný baðlantýsý baþarýsýz, baðlantýyý kontrol et
						}
					}


					//Unique monster killed..
					if (wOpID == 0xC06)
					{
						DWORD dwRefObjID = pMsg->Read<DWORD>();
						std::string strKillerName = pMsg->ReadStringA();
						std::wstringstream qSelectActions;
						qSelectActions << L"EXEC [KMTGuard].[dbo].[Hook_UniqueKill] " << dwRefObjID << L", N'" << std::wstring(strKillerName.begin(), strKillerName.end()) << L"'";

						if (AsyncGSCommands::m_dbLink.sqlConn.IsOpen())
						{
							if (!(AsyncGSCommands::m_dbLink.sqlCmd.ExecuteQuery((SQLWCHAR*)qSelectActions.str().c_str())))
							{
								// Sorgu baþarýsýz oldu
							}
						}
						else
						{
							// Veritabaný baðlantýsý baþarýsýz, baðlantýyý kontrol et
						}
					}
				}

				pMsg->SetReadPos(oldReadPos);
			}
		}
		res = s_pfnHandleMsg(self, pMsg, dwOverlappedJobID, lpParam, pMassiveMsg);
	}
	catch (...)
	{
		printf("%s - Exception occoured !\n", __FUNCTION__);
		printf("!!! This could lead to shard mgr entering invalid state !!!\n");
		printf("!!! It is recommended to shut the server down until the issue is resolved !!!\n");

		if (pMsg != NULL)
		{
			printf("%s - Dumping last [non-massive] msg.\n", __FUNCTION__);

			printf("MsgID = 0x%04X\n", pMsg->GetID());


			__int16 oldReadPos = pMsg->GetReadPos();
			__int16 oldWritePos = pMsg->GetWritePos();

			__int16 bytesToRead = (oldWritePos - 6);

			pMsg->SetReadPos(6);

			char* buffer = new char[bytesToRead];

			pMsg->Read(buffer, bytesToRead);

			hexdump(buffer, bytesToRead);

			delete[] buffer;

			pMsg->SetReadPos(oldReadPos);

			printf("");
		}
	}

	return res;
}


CMainProcess* CMainProcess::GetInstance()
{
	return s_pfnGetInstance();
}

void CMainProcess::RegisterMsgHandler(__int16 id, FN_MSG_HANDLER handler)
{
	s_pfnSetMsgHandler(CMainProcess::GetInstance(), id, handler);
	printf("%s - msg handler registered 0x%x", __FUNCTION__, id);
}



CMainProcess::FN_SET_MSG_HANDLER CMainProcess::s_pfnSetMsgHandler;
CMainProcess::FN_HANDLE_MSG CMainProcess::s_pfnHandleMsg;
CMainProcess::FN_GET_INSTANCE CMainProcess::s_pfnGetInstance;

//TODO: Update get inst for shard.
#define SET_MSG_HANDLER_FUNC_OFFSET			0x006990C0
#define HANDLE_MSG_FUNC_OFFSET				0x006990D0
#define GET_INSTANCE_FUNC_OFFSET			0x00401D30



void CMainProcess::Initialize()
{
	s_pfnSetMsgHandler = reinterpret_cast<FN_SET_MSG_HANDLER>(SET_MSG_HANDLER_FUNC_OFFSET);
	s_pfnHandleMsg = reinterpret_cast<FN_HANDLE_MSG>(HANDLE_MSG_FUNC_OFFSET);
	s_pfnGetInstance = reinterpret_cast<FN_GET_INSTANCE>(GET_INSTANCE_FUNC_OFFSET);

	DetourTransactionBegin();
	DetourAttach(&(PVOID&)s_pfnHandleMsg, CMainProcess::MyHandleMsg);
	DetourTransactionCommit();

}

__int16 __fastcall CMainProcess::MySetMsgHandler(CMainProcess* self, void* /* dummy edx */, __int16 id, void* handler)
{
	printf("%s -> self = 0x%p, id = 0x%04X, handler = 0x%p\n",
		self, id, handler);

	return s_pfnSetMsgHandler(self, id, handler);
}