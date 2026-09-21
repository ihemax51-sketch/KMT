add_library(BSLib STATIC)

target_sources(BSLib PRIVATE
        src/BSLib/_internal/MemPoolManager.cpp
        src/BSLib/BSLib.cpp
        src/BSLib/MsgHandler.cpp
        src/BSLib/multibyte.cpp
        src/BSLib/StringCheck.cpp
        src/BSLib/Debug.cpp
)

target_include_directories(BSLib PUBLIC src/)
target_link_libraries(BSLib
        ClientNet
        support
)
