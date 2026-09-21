set(SOURCE_FILES
        src/ServerFramework/ServerProcessBase.cpp src/ServerFramework/ServerProcessBase.h
        src/ServerFramework/OverlapJobMgr.cpp src/ServerFramework/OverlapJobMgr.h
        src/ServerFramework/ServerProcessMain.cpp src/ServerFramework/ServerProcessMain.h
        src/ServerFramework/ServerFrameWindow.cpp src/ServerFramework/ServerFrameWindow.h
        src/ServerFramework/Resource.h
        src/ServerFramework/WindowBase.cpp src/ServerFramework/WindowBase.h
        src/ServerFramework/Win32SizeRuler.cpp src/ServerFramework/Win32SizeRuler.h
        src/ServerFramework/ServerChildWindowBase.cpp src/ServerFramework/ServerChildWindowBase.h
        src/ServerFramework/ServerFramework.cpp src/ServerFramework/ServerFramework.h)

add_library(ServerFramework STATIC ${SOURCE_FILES})
target_link_libraries(ServerFramework ServerCommon FrameworkCommon BSNet abi-testing)

target_include_directories(ServerFramework
        PUBLIC src/
        )

