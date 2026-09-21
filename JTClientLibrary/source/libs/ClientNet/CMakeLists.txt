add_library(ClientNet STATIC)

target_sources(ClientNet PRIVATE
        src/ClientNet/MsgStreamBuffer.cpp
        src/ClientNet/ClientNet.cpp
)

target_link_libraries(ClientNet PUBLIC BSLib support)
target_include_directories(ClientNet PUBLIC src/)
