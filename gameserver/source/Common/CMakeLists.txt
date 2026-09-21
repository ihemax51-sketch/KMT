set(SOURCE_FILES
        Framework/MassiveMsg.cpp Framework/MassiveMsg.h
        Framework/Msg.cpp Framework/Msg.h
        Define/SRO_MSG_DEF.h)

add_library(FrameworkCommon STATIC ${SOURCE_FILES})
target_link_libraries(FrameworkCommon abi-testing)
target_include_directories(FrameworkCommon
        PUBLIC
        Framework/
        Define/
        )

