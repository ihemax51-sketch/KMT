add_executable(KmtGuardGameServerLoadTest
        KmtGuardGameServerLoadTest.cpp
        ../source/SilkroadOnline/SR_VIETNAM_SERVICE/v188/Server/SR_GameServer/GameServer/GameServer/src/KMTGuardCustom/InternalPacketAuth.cpp)

target_link_libraries(KmtGuardGameServerLoadTest FrameworkCommon)
target_include_directories(KmtGuardGameServerLoadTest PRIVATE
        ../source/SilkroadOnline/SR_VIETNAM_SERVICE/v188/Server/SR_GameServer/GameServer/GameServer/src)

set_target_properties(KmtGuardGameServerLoadTest PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/load-test")

add_executable(KmtGuardGameServerCommandContractTests
        GameServerCommandContractTests.cpp)
target_include_directories(KmtGuardGameServerCommandContractTests PRIVATE
        "${CMAKE_SOURCE_DIR}/../native"
        "${CMAKE_SOURCE_DIR}/source/SilkroadOnline/SR_VIETNAM_SERVICE/v188/Server/SR_GameServer/GameServer/GameServer/src")
set_target_properties(KmtGuardGameServerCommandContractTests PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/contract-tests")
