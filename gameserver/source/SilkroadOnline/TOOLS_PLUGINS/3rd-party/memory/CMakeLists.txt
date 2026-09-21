set(SOURCE_FILES
        src/hook.cpp
        src/util.cpp include/memory/MemoryUtility.h src/MemoryUtility.cpp include/memory/RemodelUtility.h include/memory/Process.h)

add_library(memory STATIC ${SOURCE_FILES})
target_link_directories(memory
        PUBLIC lib)
target_include_directories(memory
        PUBLIC include
        PRIVATE include/memory
        PRIVATE include/protect)
