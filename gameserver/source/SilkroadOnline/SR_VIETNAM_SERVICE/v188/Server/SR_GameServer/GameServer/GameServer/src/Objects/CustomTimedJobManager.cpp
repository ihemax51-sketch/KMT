#include "CustomTimedJobManager.h"
#include "GItem.h"
#include <math.h>
#include <time.h>
#include <Game.h>
#include <NetHelper.h>


//TODO: Need to store a pair of player ptr and item id, to find out if plus applied because
//of respawns.

void CCustomTimedJobManager::Setup()
{
}

void CCustomTimedJobManager::ConsumeTimedItemPlusRecords()
{
    while (CSqlCon::TimedItemList.size() > 0)
    {
        Sleep(2000);
        bool exitOuterLoop = false;

        for (std::map<INT64, STimedItemPlusDbRecord>::iterator it = CSqlCon::TimedItemList.begin(); it != CSqlCon::TimedItemList.end(); ++it)
        {
            time_t currentTime;
            time(&currentTime);

            if (g_pCGame->m_mapPcCharId.size() > 0)
            {
                if (currentTime > it->second.endTime)
                {
                    if (g_pCGame->m_mapPcCharId.find(it->second.CharID) != g_pCGame->m_mapPcCharId.end())
                    {
                        CGObjPC* pPC = g_pCGame->m_mapPcCharId[it->second.CharID];
                        if (pPC->CanSendMessage())
                        {
                            int slotcnt = pPC->m_PCInventory.m_nSlotsCount;

                            for (int i = 0; i < slotcnt - 13; i++)
                            {
                                CGItem* pItem = (CGItem*)pPC->GetItemAtStorageSlot(i);

                                if (pItem != NULL && pItem->ID64 == it->first)
                                {
                                    pItem->InstanceItem->SetPlus(it->second.CurrentPlus);
                                    pItem->RefreshItemStats();

                                    CMsg* pMsg = pPC->AllocMsg(0x3040);
                                    *pMsg << BYTE(i);
                                    *pMsg << BYTE(2); //flag opt lvl
                                    *pMsg << (BYTE)it->second.CurrentPlus;
                                    pPC->SendMsg(pMsg);


                                    pPC->m_PCInventory.RefreshEquipParamEntryForPC(false);



                                    CMsg* pMsg2 = pPC->AllocMsg(0x5025);
                                    *pMsg2 << (byte)1;
                                    *pMsg2 << BYTE(i);
                                    pPC->SendMsg(pMsg2);


                                    CMsg* pShardMsgFirst = NEWMSG(0x5067, false);
                                    *pShardMsgFirst << it->first;
                                    CNetHelper::SendMsgToSM(pShardMsgFirst);

                                    CSqlCon::TimedItemList.erase(it->second.dwItemID64);

                                    char szQuery[256] = { 0 };
                                    sprintf(szQuery, "UPDATE SRO_VT_SHARD.._Items SET OptLevel = %d WHERE ID64 = %lld",
                                        it->second.CurrentPlus, it->second.dwItemID64);

                                    if (!CSqlCon::TryExecNonQuery(szQuery))
                                    {
                                        printf("%s - What the fuck ?! Query failed, query = %s\n", __FUNCTION__, szQuery);
                                    }
                                    else
                                    {
                                        ZeroMemory(szQuery, sizeof(szQuery));
                                    }
                                    sprintf(szQuery, "DELETE FROM [KMTGuard].[dbo].[Item_Locked] WHERE ItemID64 = %lld",
                                        it->second.dwItemID64);
                                    if (!CSqlCon::TryExecNonQuery(szQuery))
                                    {
                                        sprintf("%s - What the fuck ?! Query failed, query = %s",__FUNCTION__, szQuery);
                                    }
                                    else
                                    {
                                        ZeroMemory(szQuery, sizeof(szQuery));
                                    }
                                    exitOuterLoop = true;

                                    pPC->CharacterStats();



                                    break; // �� d�ng�den ��k
                                }
                            }
                        }

                    }

                }
            }
            //else
            //{
            //    if (currentTime > it->second.endTime)
            //    {
            //        char szQuery[256] = { 0 };

            //        ZeroMemory(szQuery, sizeof(szQuery));
            //        sprintf(szQuery, "UPDATE SRO_VT_SHARD.._Items SET OptLevel = %d WHERE ID64 = %lld",
            //            it->second.CurrentPlus, it->first);

            //        if (!CSqlCon::TryExecNonQuery(szQuery))
            //        {
            //            printf("%s - What the fuck ?! Query failed, query = %s\n",
            //                __FUNCTION__, szQuery);
            //        }
            //        else
            //        {
            //            ZeroMemory(szQuery, sizeof(szQuery));
            //        }

            //        sprintf(szQuery, "DELETE FROM [KMTGuard].[dbo].[Item_TimedPlus] where ID64 = %lld",
            //            it->second.dwItemID64);

            //        if (!CSqlCon::TryExecNonQuery(szQuery))
            //        {
            //            sprintf("%s - What the fuck ?! Query failed, query = %s",
            //                __FUNCTION__, szQuery);
            //        }
            //        else
            //        {
            //            ZeroMemory(szQuery, sizeof(szQuery));
            //        }
            //        CMsg* pShardMsgFirst = NEWMSG(0x5067, false);
            //        *pShardMsgFirst << it->first;
            //        CNetHelper::SendMsgToSM(pShardMsgFirst);
            //        CSqlCon::TimedItemList.erase(it++); // �terat�r� sil ve sonraki iterat�re ge�
            //        break; // D�ng�y� sonland�r
            //    }
            //}
            if (exitOuterLoop)
            {
                break; // D�� d�ng�den ��k
            }
        }
    }
}


void CCustomTimedJobManager::ConsumeTimedItemPlusRecordsDevill()
{
    while (CSqlCon::STimedDevillList.size() > 0)
    {
        Sleep(2000);
        bool exitOuterLoop = false;

        for (std::map<INT64, STimedDevillPlusDbRecord>::iterator it = CSqlCon::STimedDevillList.begin(); it != CSqlCon::STimedDevillList.end(); ++it)
        {
            time_t currentTime;
            time(&currentTime);

            if (g_pCGame->m_mapPcCharId.size() > 0)
            {
                if (currentTime > it->second.endTime)
                {
                    if (g_pCGame->m_mapPcCharId.find(it->second.CharID) != g_pCGame->m_mapPcCharId.end())
                    {
                        CGObjPC* pPC = g_pCGame->m_mapPcCharId[it->second.CharID];
                        if (pPC->CanSendMessage())
                        {
                            if (pPC->m_PCAvatarInventory.GetItem(4) != NULL)
                            {
                                if (pPC->m_PCAvatarInventory.GetItem(4)->ID64 == it->second.dwItemID64)
                                {
                                    for (std::map<int, _RefAbilityByItemOptLevel>::iterator opt = CSqlCon::RefAbilitybyItemOptLevel.begin(); opt != CSqlCon::RefAbilitybyItemOptLevel.end(); opt++)
                                    {
                                        int AbilityOptLevel = (*opt).second.OptLevel;
                                        if (AbilityOptLevel == 0 && pPC->m_PCAvatarInventory.GetItem(4)->InstanceItem->RefItemID == opt->second.RefItemID)
                                        {
                                            int AbilityID = (*opt).second.ID;
                                            if (CSqlCon::RefSkillByItemOptLevel.find(AbilityID) != CSqlCon::RefSkillByItemOptLevel.end())
                                            {
                                                int OptLevelSkillID = CSqlCon::RefSkillByItemOptLevel[(*opt).second.ID].RefSkillID;
                                                printf("%d\n", OptLevelSkillID);
                                                if (pPC->pData.active.find(OptLevelSkillID) != pPC->pData.active.end())
                                                {
                                                    pPC->pData.RemoveActiveSkillById(OptLevelSkillID);
                                                }

                                                CMsg* pMsg = pPC->AllocMsg(0x5024);
                                                *pMsg << OptLevelSkillID;
                                                pPC->SendMsg(pMsg);
                                            }
                                        }

                                    }
                                    //fun 383
                                    pPC->m_PCAvatarInventory.GetItem(4)->InstanceItem->SetPlus(0);
                                    pPC->m_PCAvatarInventory.GetItem(4)->RefreshItemStats();



                                    pPC->m_PCAvatarInventory.RefreshEquipParamEntryForPC(false);
                                    pPC->CharacterStats();
                                    CMsg* pMsg2 = pPC->AllocMsg(0x3078);
                                    *pMsg2 << BYTE(1);
                                    pPC->SendMsg(pMsg2);

                                    /*                          pPC->Fun_214();*/

                                                               //pPC->m_PCAvatarInventory.RefreshEquipParamEntryForPC(false);
                                                          /*      pPC->m_PCAvatarInventory.CheckRentalItemValidity();
                                                              pPC->m_PCAvatarInventory.CheckAvatarsetIntegrity();*/

                                    printf("item removed plus\n");
                                    CSqlCon::STimedDevillList.erase(it->second.dwItemID64);

                                    char szQuery[256] = { 0 };
                                    sprintf(szQuery, "UPDATE SRO_VT_SHARD.._Items SET OptLevel = %d WHERE ID64 = %lld",
                                        it->second.CurrentPlus, it->second.dwItemID64);

                                    if (!CSqlCon::TryExecNonQuery(szQuery))
                                    {
                                        printf("%s - What the fuck ?! Query failed, query = %s\n",
                                            __FUNCTION__, szQuery);
                                    }

                                    else
                                    {
                                        ZeroMemory(szQuery, sizeof(szQuery));
                                    }

                                    sprintf(szQuery, "DELETE FROM [KMTGuard].[dbo].[Item_TimedDevilPlus] where ID64 = %lld",
                                        it->second.dwItemID64);

                                    if (!CSqlCon::TryExecNonQuery(szQuery))
                                    {
                                        sprintf("%s - What the fuck ?! Query failed, query = %s",
                                            __FUNCTION__, szQuery);
                                    }
                                    else
                                    {
                                        ZeroMemory(szQuery, sizeof(szQuery));
                                    }

                                    exitOuterLoop = true;

                                    pPC->CharacterStats();
                                    break; // �� d�ng�den ��k
                                }
                            }




                            int slotcnt = pPC->m_PCInventory.m_nSlotsCount;

                            for (int i = 0; i < slotcnt - 13; i++)
                            {
                                CGItem* pItem = (CGItem*)pPC->GetItemAtStorageSlot(i);

                                if (pItem != NULL && pItem->ID64 == it->first)
                                {
                                    pItem->InstanceItem->SetPlus(it->second.CurrentPlus);
                                    pItem->RefreshItemStats();

                                    CMsg* pMsg = pPC->AllocMsg(0x3040);
                                    *pMsg << BYTE(i);
                                    *pMsg << BYTE(2); //flag opt lvl
                                    *pMsg << (BYTE)it->second.CurrentPlus;
                                    pPC->SendMsg(pMsg);


                                    pPC->m_PCInventory.RefreshEquipParamEntryForPC(false);




                                    CMsg* pMsg2 = pPC->AllocMsg(0x5025);
                                    *pMsg2 << (byte)3;
                                    *pMsg2 << BYTE(i);
                                    pPC->SendMsg(pMsg2);


                                    //CMsg* pShardMsgFirst = NEWMSG(0x5067, false);
                                    //*pShardMsgFirst << it->first;
                                    //CNetHelper::SendMsgToSM(pShardMsgFirst);

                                    CSqlCon::STimedDevillList.erase(it->second.dwItemID64);

                                    char szQuery[256] = { 0 };
                                    sprintf(szQuery, "UPDATE SRO_VT_SHARD.._Items SET OptLevel = %d WHERE ID64 = %lld",
                                        it->second.CurrentPlus, it->second.dwItemID64);

                                    if (!CSqlCon::TryExecNonQuery(szQuery))
                                    {
                                        printf("%s - What the fuck ?! Query failed, query = %s\n",
                                            __FUNCTION__, szQuery);
                                    }

                                    else
                                    {
                                        ZeroMemory(szQuery, sizeof(szQuery));
                                    }

                                    sprintf(szQuery, "DELETE FROM [KMTGuard].[dbo].[Item_TimedDevilPlus] where ID64 = %lld",
                                        it->second.dwItemID64);

                                    if (!CSqlCon::TryExecNonQuery(szQuery))
                                    {
                                        sprintf("%s - What the fuck ?! Query failed, query = %s",
                                            __FUNCTION__, szQuery);
                                    }
                                    else
                                    {
                                        ZeroMemory(szQuery, sizeof(szQuery));
                                    }

                                    exitOuterLoop = true;

                                    pPC->CharacterStats();



                                    break; // �� d�ng�den ��k
                                }
                            }
                        }
                        

                    }
                }                
            }
            if (exitOuterLoop)
            {
                break; // D�� d�ng�den ��k
            }
        }
    }
}

void CCustomTimedJobManager::CreateConsumeThread()
{
	// Intentionally retained as an archive-only entry point. Starting the old
	// detached worker would violate vSRO GameServer thread ownership.
	printf("%s - Archived unsafe consumer worker is disabled", __FUNCTION__);
}

DWORD WINAPI CCustomTimedJobManager::ConsumeThreadWorker(LPVOID lpParam)
{
	while (true)
	{
		CCustomTimedJobManager::ConsumeTimedItemPlusRecords();
        Sleep(1000);
    //    CCustomTimedJobManager::ConsumeTimedItemPlusRecordsDevill();
	}

	return 0;
}
