cmake_minimum_required(VERSION 3.12)
project(SRO_DevKit)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_LIST_DIR}/cmake")

set(DirectX_ROOT ${CMAKE_SOURCE_DIR}/source/third-party/)
find_package(DirectX)

if (NOT DirectX_FOUND)
    message(FATAL_ERROR "DirectX was NOT found. DevKit can not be used without it. Make sure you checked out all submodules.")
endif ()

include(DevKit)

devkit_enforce_static_linking()
devkit_enforce_include_path()

find_package(CxxTest)
if (CXXTEST_FOUND)
    message(STATUS "CxxTest found. Testing enabled!")
    include_directories(${CXXTEST_INCLUDE_DIR})
    enable_testing()
endif ()

# We are no idiots, ignore these stupid deprecations!
add_compile_definitions(
        _CRT_SECURE_NO_DEPRECATE
        _CRT_NON_CONFORMING_SWPRINTFS
        _SILENCE_STDEXT_HASH_DEPRECATION_WARNINGS
)

# GetModuleHandleEx is part of the Windows XP API surface and is required by
# the client module lifetime guard. Keep the legacy VC80 toolchain while
# exposing the correct Platform SDK declarations.
add_definitions(-DWINVER=0x0501)
add_definitions(-D_WIN32_WINNT=0x0501)

include(Options)

# Set loglevel if supplied as parameter
if (DEFINED PUT_LOGLEVEL)
    message(STATUS "Loglevel: ${PUT_LOGLEVEL}")
    add_definitions(-DPUT_LOGLEVEL=${PUT_LOGLEVEL})
endif ()

add_subdirectory(source)
