
set(SOURCE_FILES
        src/DllMain.cpp
        src/Util.cpp src/Util.h
        src/GameServerConsole.cpp src/GameServerConsole.h
        src/GameServerCrashHandler.cpp src/GameServerCrashHandler.h
        src/StaticPatches.cpp src/StaticPatches.h)

add_library(OutPutGS SHARED ${SOURCE_FILES} ${VERSION_INFO})
target_link_libraries(OutPutGS GameServer memory winmm)
if (MSVC)
    target_compile_options(OutPutGS PRIVATE /O2 /Ob2 /Oi /Gy /GS)
    set_property(TARGET OutPutGS APPEND_STRING PROPERTY LINK_FLAGS
            " /OPT:REF /OPT:ICF /INCREMENTAL:NO /DYNAMICBASE /NXCOMPAT")
endif ()

set(KMT_GAMESERVER_OUTPUT_DIRECTORY "C:/Gs" CACHE PATH
        "Directory for the KMTGuard GameServer add-on")
set_target_properties(OutPutGS
        PROPERTIES
        ARCHIVE_OUTPUT_DIRECTORY "${KMT_GAMESERVER_OUTPUT_DIRECTORY}/"
        LIBRARY_OUTPUT_DIRECTORY "${KMT_GAMESERVER_OUTPUT_DIRECTORY}/"
        RUNTIME_OUTPUT_DIRECTORY "${KMT_GAMESERVER_OUTPUT_DIRECTORY}/"
        ARCHIVE_OUTPUT_DIRECTORY_RELEASE "${KMT_GAMESERVER_OUTPUT_DIRECTORY}/"
        LIBRARY_OUTPUT_DIRECTORY_RELEASE "${KMT_GAMESERVER_OUTPUT_DIRECTORY}/"
        RUNTIME_OUTPUT_DIRECTORY_RELEASE "${KMT_GAMESERVER_OUTPUT_DIRECTORY}/"
        OUTPUT_NAME "KMTGuard_GameServer"
        )
