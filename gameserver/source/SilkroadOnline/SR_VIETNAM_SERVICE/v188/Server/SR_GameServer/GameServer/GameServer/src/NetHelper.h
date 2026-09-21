#pragma once
#include "ReferenceData/ReferenceData.h"
#include "Msg.h"
class CGObjPC;

class CNetEngine;
class CTrainingCamp;
class CParty;
class CGuild;

#define NET_ENGINE_REF_OFFSET							0x00C8258C
#ifndef g_pNetEngine
#define g_pNetEngine (*(CNetEngine**)NET_ENGINE_REF_OFFSET)
#endif

#define g_CStreamBufferUnk 0xCD7DFC

#ifdef NEWMSG
#undef NEWMSG
#endif
#define NEWMSG(msgid, encrypted) \
	CNetHelper::AllocMsg(msgid, encrypted)

#define FREEMSG(msg) \
	CNetHelper::FreeMsg(msg);

#define SEND_MSG_TO_PC(pc, msg) \
	CNetHelper::SendMsgToPC(pc, msg)

class CNetHelper
{
private:
	typedef CMsg* (__stdcall* FN_ALLOC_MSG)(CNetEngine*, int);
	typedef CMsg* (__cdecl* FN_ALLOC_MSG_FOR_BROADCAST)(WORD wMsgID);
	typedef int(__thiscall* FN_EXEC_HANDLER_FOR_CGOBJPC)(CGObjPC* pPC, CMsg* pMsg);
	typedef void(__stdcall* FN_FREE_MSG)(CNetEngine*, CMsg* pMsg);
	typedef CMsg* (__thiscall* FN_CGOBJPC_ALLOCMSGFORPEER)(CGObjPC* pPC, WORD wMsgID);
	typedef CMsg* (__thiscall* FN_ALLOC_MSG_FOR_SM)(CNetEngine* pNet, int);


	static FN_ALLOC_MSG s_pfnAllocMsg;
	static FN_ALLOC_MSG_FOR_BROADCAST s_pfnAllocMsgForBroadcast;
	static FN_EXEC_HANDLER_FOR_CGOBJPC s_pfnExecHandlerForCGObjPC;
	static FN_FREE_MSG s_pfnFreeMsg;
	static FN_CGOBJPC_ALLOCMSGFORPEER s_pfnCGObjPC_AllocMsgForPeer;

	static CMsg* __fastcall MyCGObjPC_AllocMsgForPeer(CGObjPC* pPC, void* /* dummy edx */, WORD wMsgID);

public:
	static void Initialize();
	static void SendMsgToPC(CGObjPC* pPC, CMsg* pMsg);

	static CMsg* AllocMsg(WORD wMsgID, bool bEncrypted);
	static CMsg* AllocMsgForBroadcast(WORD wMsgID);

	static void FreeMsg(CMsg* pMsg);

	static void SendMsgToSM(CMsg* pMsg);

	static void SendMsgToPartyMembers(CParty* pParty, CMsg* pMsg, int nSenderGID);
	//Not sure if it's GameID or CharID here (we only need '0')
	static void SendMsgToGuildMembers(CGuild* pGuild, CMsg* pMsg, int nSenderID);
	static void SendMsgToTrainingCampMembers(CTrainingCamp* pCamp, CMsg* pMsg, int nSenderID);

	static void BindStreamBufferWithMsg(CMsg* pMsg);
	static void FlushStreamBufferMsg(CMsg* pMsg);

	static CMsg* CopyMsg(CMsg* pSrcMsg);
	static CMsg* CopyMsg(CMsg* pSrcMsg, CGObjPC* pPC);
};
