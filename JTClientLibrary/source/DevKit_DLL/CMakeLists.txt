add_library(DevKit_DLL SHARED src/CircleDrawTool.cpp src/CircleDrawTool.h)
option(KMT_DIAGNOSTIC_SKIP_SETUP "Build without installing client hooks" OFF)

target_sources(DevKit_DLL PRIVATE
        src/DllMain.cpp
        src/KMTGuardKit.rc
        src/ClientStartupCompatibility.cpp
        src/ClientStartupCompatibility.h
        src/hooks/Hooks.cpp
        src/hooks/GFXVideo3d_Hook.cpp
        src/hooks/WndProc_Hook.cpp
        src/hooks/CGame_Hook.cpp
        src/imgui_windows/ImGui_Windows.cpp
        src/imgui_windows/BasicWindow.cpp
        src/imgui_windows/InterfaceDebugger.cpp
        src/imgui_windows/SoundTool.cpp
        src/imgui_windows/NavMeshTool.cpp
        src/imgui_windows/EntityExplorer.cpp
        src/imgui_windows/NotificationTool.cpp
        src/imgui_windows/SystemMessage.cpp
        src/imgui_windows/InterfaceTree.cpp
        src/imgui_windows/ProcessViewer.cpp
        src/imgui_windows/PartyInfo.cpp
        src/imgui_windows/CharacterData.cpp
        src/imgui_windows/ItemData.cpp
        src/imgui_windows/ErrorMessageTool.cpp
        src/imgui_windows/Inventory.cpp
        src/Util.cpp
        src/QuickStart.cpp
        src/MathUtil.cpp
        src/imgui_windows/NavigationTool.cpp
        src/imgui_windows/NavigationTool.h
        src/PathFinder.cpp
        src/PathFinder.h
        src/Node.cpp
        src/Node.h
        src/node_set.h
        src/location_equals.h
)

if (KMT_DIAGNOSTIC_SKIP_SETUP)
    target_compile_definitions(DevKit_DLL PRIVATE KMT_DIAGNOSTIC_SKIP_SETUP=1)
else ()
    target_compile_definitions(DevKit_DLL PRIVATE KMT_DIAGNOSTIC_SKIP_SETUP=0)
endif ()

target_link_libraries(DevKit_DLL
        BSLib
        SROInterfaceLIB
        ClientLib
        MathHelpers
        support
        winmm
        ws2_32
        advapi32
        dbghelp
        DiscordRichPresence
)

if (MSVC)
    target_compile_options(DevKit_DLL PRIVATE /O2 /Ob2 /Oi /Gy /GS)
    set_property(TARGET DevKit_DLL APPEND_STRING PROPERTY LINK_FLAGS
            " /OPT:REF /OPT:ICF /INCREMENTAL:NO /DYNAMICBASE /NXCOMPAT")
endif ()

set(KMT_CLIENT_OUTPUT_DIRECTORY
        "${PROJECT_SOURCE_DIR}/BinOut/${CMAKE_BUILD_TYPE}"
        CACHE PATH "Directory for the KMTGuard client DLL")
set_target_properties(DevKit_DLL
        PROPERTIES
        ARCHIVE_OUTPUT_DIRECTORY "${KMT_CLIENT_OUTPUT_DIRECTORY}/"
        LIBRARY_OUTPUT_DIRECTORY "${KMT_CLIENT_OUTPUT_DIRECTORY}/"
        RUNTIME_OUTPUT_DIRECTORY "${KMT_CLIENT_OUTPUT_DIRECTORY}/"



        OUTPUT_NAME "KMTGuardKit"
)
