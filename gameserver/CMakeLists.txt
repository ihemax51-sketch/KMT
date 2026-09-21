cmake_minimum_required(VERSION 3.15)
project(SRO_Server DESCRIPTION "Re-written SilkroadOnline server source code")

# Keep the legacy GameServer add-on self-contained even when a modern CMake
# drives the Visual Studio 2005 toolchain.
set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_LIST_DIR}/cmake")

# I have no idea why this is required. MSVC is weird.
link_directories($ENV{LIB})
include_directories($ENV{INCLUDE})

set(DirectX_ROOT ${CMAKE_SOURCE_DIR}/source/SilkroadOnline/TOOLS_PLUGINS/3rd-party/)
find_package(DirectX)

if (NOT DirectX_FOUND)
    message(FATAL_ERROR "DirectX was NOT found. We need it for Navmesh! Make sure you checked out all submodules.")
endif()

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /X")
set(CMAKE_OBJECT_PATH_MAX 5000)

# Include the cmake file and call the genrate func
include(GenerateProductVersion)
generate_product_version(VERSION_INFO
        NAME "KMTGuard_GameServer"
        BUNDLE "KMTGuard"
        VERSION_MAJOR 7
        VERSION_MINOR 0
        VERSION_PATCH 0
        VERSION_REVISION 0
        COMPANY_NAME "KMTGuard"
        COMMENTS "KMTGuard GameServer add-on v7.0.0"
        ORIGINAL_FILENAME "KMTGuard_GameServer.dll"
        INTERNAL_NAME "KMTGuard_GameServer.dll"
        FILE_DESCRIPTION "KMTGuard GameServer add-on"
        )

# Enforce static linking of the standard library because dynamic linking sucks
set(CompilerFlags
        CMAKE_CXX_FLAGS
        CMAKE_CXX_FLAGS_DEBUG
        CMAKE_CXX_FLAGS_RELEASE
        CMAKE_CXX_FLAGS_RELWITHDEBINFO
        CMAKE_CXX_FLAGS_MINSIZEREL
        CMAKE_C_FLAGS
        CMAKE_C_FLAGS_DEBUG
        CMAKE_C_FLAGS_RELEASE
        CMAKE_C_FLAGS_RELWITHDEBINFO
        CMAKE_C_FLAGS_MINSIZEREL
        )
foreach (CompilerFlag ${CompilerFlags})
    string(REPLACE "/MD" "/MT" ${CompilerFlag} "${${CompilerFlag}}")
endforeach()

message(${CMAKE_CXX_FLAGS})
message(${CMAKE_CXX_FLAGS_DEBUG})
message(${CMAKE_CXX_FLAGS_RELEASE})
message(${CMAKE_CXX_FLAGS_RELWITHDEBINFO})
message(${CMAKE_CXX_FLAGS_MINSIZEREL})

# We are no idiots, ignore these stupid deprecations!
add_compile_definitions(_CRT_SECURE_NO_DEPRECATE _CRT_NON_CONFORMING_SWPRINTFS)

# Set target platform ... to ... Windows XP ... oops
add_definitions(-DWINVER=0x0500)
add_definitions(-D_WIN32_WINNT=0x0500)

include(options.cmake)

add_subdirectory(source)
add_subdirectory(tests)
