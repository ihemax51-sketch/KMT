set(TableData
        ../SR_GameServer/GameServer/GameServer/src/Objects/InstanceObj.h
        )

set(TableDef
        src/TableDef/RefObjCommon.h
        src/TableDef/RefObjItem.h
        )

set(REF_DATA
        src/ReferenceData/ReferenceData.h
        src/ReferenceData/ReferenceDataMgr.cpp src/ReferenceData/ReferenceDataMgr.h)

set(SOURCE
        src/ShardDB.cpp src/ShardDB.h
        src/ReferenceData/TypeId.cpp src/ReferenceData/TypeId.h)

add_library(ServerCommon STATIC ${SOURCE} ${REF_DATA} ${TableData} ${TableDef})
target_link_libraries(ServerCommon BSLib ghidra)
target_include_directories(ServerCommon
        PUBLIC src/
        )