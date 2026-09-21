set(SOURCE
        Common/src/Helpers.h
        Common/src/SRO_GS_GLOBAL.cpp Common/src/SRO_GS_GLOBAL.h
        GameServer/src/Interface/IGObj.h
        GameServer/src/Interface/IStorage.h
        GameServer/src/Data.h
        GameServer/src/CmdSource.h
        GameServer/src/CmdSrcNet.h
        GameServer/src/Objects/GObj.cpp GameServer/src/Objects/GObj.h
        GameServer/src/World/GameWorldContext.h
        GameServer/src/World/GameWorldContextEX.h
        GameServer/src/Objects/GObjFactory.h
         GameServer/src/Objects/GItem.cpp
        GameServer/src/Objects/GItem.h
                 GameServer/src/Objects/GObjPCData.cpp
        GameServer/src/Objects/GObjPCData.h
                 GameServer/src/Objects/Skill.cpp
        GameServer/src/Objects/Skill.h

        GameServer/src/Objects/GObjMobile.cpp GameServer/src/Objects/GObjMobile.h
        GameServer/src/Objects/GObjChar.cpp GameServer/src/Objects/GObjChar.h
        GameServer/src/Storage/GStorage.cpp
        GameServer/src/Storage/GStorage.h
        GameServer/src/StorageOP/GAvatarInventoryOP.h
        GameServer/src/StorageOP/GChestOP.h
        GameServer/src/StorageOP/GCOSInventoryOP.h
        GameServer/src/StorageOP/GGuildChestOP.h
        GameServer/src/StorageOP/GMagicCubeStorageOP.h
        GameServer/src/StorageOP/GNPCInventoryOP.h
        GameServer/src/StorageOP/GPCInventoryOP.h
        GameServer/src/StorageOP/GStorageOP.h
        App/src/MainProcess.cpp App/src/MainProcess.h
        App/src/Game.cpp App/src/Game.h
        GameServer/src/World/GameWorld.h
        GameServer/src/World/GameWorldMgr.cpp GameServer/src/World/GameWorldMgr.h GameServer/src/Objects/InstancePC.h GameServer/src/Objects/Param.h GameServer/src/Objects/GObjPC.cpp GameServer/src/Objects/GObjPC.h GameServer/src/Objects/TypeId.cpp GameServer/src/Objects/TypeId.h GameServer/src/Objects/InstanceItem.cpp GameServer/src/Objects/InstanceItem.h GameServer/src/Objects/MyInstanceObj.h GameServer/src/Objects/CGObjCOS_GoldPet.cpp GameServer/src/Objects/CGObjCOS_GoldPet.h GameServer/src/Objects/CGObjCOS_Pet.cpp GameServer/src/Objects/CGObjCOS_Pet.h GameServer/src/Objects/CGObjCOS.cpp GameServer/src/Objects/CGObjCOS.h GameServer/src/Objects/GObjNPC.cpp GameServer/src/Objects/GObjNPC.h GameServer/src/Objects/GObjMob.cpp GameServer/src/Objects/GObjMob.h GameServer/src/Objects/RegionManagerBody.cpp GameServer/src/Objects/RegionManagerBody.h GameServer/src/SettingMgr/IniReader.cpp GameServer/src/SettingMgr/IniReader.h GameServer/src/NetHelper.cpp GameServer/src/NetHelper.h GameServer/src/Objects/GObjSiegeStruct.cpp GameServer/src/Objects/GObjSiegeStruct.h GameServer/src/RegionRestrictionDBSet.cpp GameServer/src/RegionRestrictionDBSet.h GameServer/src/SqlConnection/AutoCriticalSection.cpp GameServer/src/SqlConnection/AutoCriticalSection.h GameServer/src/SqlConnection/DbConnection.cpp GameServer/src/SqlConnection/DbConnection.h GameServer/src/SqlConnection/sqlCon.cpp GameServer/src/SqlConnection/sqlCon.h GameServer/src/Objects/InstanceGuild.h GameServer/src/Objects/InstanceGuild.cpp GameServer/src/Objects/Guild.h CustomNetwork/MsgStreamBuffer.cpp CustomNetwork/MsgStreamBuffer.h GameServer/src/Objects/SkillManager.cpp GameServer/src/Objects/SkillManager.h GameServer/src/DevNew/CustomNPCEvent.cpp GameServer/src/DevNew/CustomNPCEvent.h GameServer/src/SettingMgr/NewSettings.cpp GameServer/src/SettingMgr/NewSettings.h 
        GameServer/src/KMTGuardCustom/GObjEvents.cpp 
        GameServer/src/KMTGuardCustom/DamageMeter.h
        GameServer/src/KMTGuardCustom/DamageMeter.cpp
        GameServer/src/KMTGuardCustom/DurabilityControl.h
        GameServer/src/KMTGuardCustom/DurabilityControl.cpp
        GameServer/src/KMTGuardCustom/PartyMonsterControl.h
        GameServer/src/KMTGuardCustom/PartyMonsterControl.cpp
        GameServer/src/KMTGuardCustom/TradeGoldControl.h
        GameServer/src/KMTGuardCustom/TradeGoldControl.cpp
        GameServer/src/KMTGuardCustom/GameServerTelemetry.h
        GameServer/src/KMTGuardCustom/GameServerTelemetry.cpp
        GameServer/src/KMTGuardCustom/GameServerRuntimeSafety.h
        GameServer/src/KMTGuardCustom/GameServerRuntimeSafety.cpp
        GameServer/src/KMTGuardCustom/InternalPacketAuth.h
        GameServer/src/KMTGuardCustom/InternalPacketAuth.cpp
        GameServer/src/KMTGuardCustom/ItemRegionTravelGuard.h
        GameServer/src/KMTGuardCustom/ItemRegionTravelGuard.cpp
        GameServer/src/KMTGuardCustom/GObjEvents.h GameServer/src/KMTGuardCustom/CGObjPCCustom.cpp GameServer/src/GSLog/LogCustoms.cpp GameServer/src/GSLog/LogCustoms.h GameServer/src/GSLog/Logger.cpp GameServer/src/GSLog/Logger.h GameServer/src/GSLog/MsgCustom.cpp GameServer/src/GSLog/MsgCustom.h GameServer/src/Objects/RgnTerrain.cpp GameServer/src/Objects/RgnTerrain.h)

add_library(GameServer STATIC ${SOURCE})
target_link_libraries(GameServer ServerFramework BSLib BSObj NavMesh memory remodel)
target_include_directories(GameServer PUBLIC
        "${CMAKE_SOURCE_DIR}/../native"
        App/src
        Common/src
        GameServer/src
        GameServer/src/Interface
        GameServer/src/Objects
        GameServer/src/Storage
        GameServer/src/StorageOP
        )
