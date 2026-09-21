set(SOURCE_FILES
        src/BSNet/BSNet.h
        src/BSNet/NetEngine.cpp src/BSNet/NetEngine.h)

add_library(BSNet STATIC ${SOURCE_FILES})
target_link_libraries(BSNet BSLib)

target_include_directories(BSNet
        PUBLIC src/
        )