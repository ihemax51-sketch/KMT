
set(SOURCE_FILES
        src/Database/DBSessionMgr.h
        src/Database/DBTable.h
        src/Database/DBColum.h
        src/Database/DBRecord.h
        src/Database/DBRecStream.h
        src/Database/TableKeeper.h
        src/STL/SingletonT.h
        src/STL/Que.h src/STL/_Que.h
        src/CriticalSectionBS.cpp src/CriticalSectionBS.h
        src/Base.cpp src/Base.h
        src/ServiceObject.cpp src/ServiceObject.h
        src/Database/DBSet.h src/Database/DBColumMgr.h)

add_library(BSLib STATIC ${SOURCE_FILES})
target_link_libraries(BSLib remodel abi-testing)

target_include_directories(BSLib
        PUBLIC src/
        )

