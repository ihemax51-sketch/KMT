
set(SOURCE_FILES
        src/BSObj/BSObj.cpp src/BSObj/BSObj.h
        )

add_library(BSObj STATIC ${SOURCE_FILES})

target_link_libraries(BSObj BSLib)

target_include_directories(BSObj
        PUBLIC src/
        )

