
add_library(TypeId STATIC src/TypeIdBase.cpp)

target_sources(TypeId PRIVATE
        src/TypeIdRegistry.cpp
)

target_include_directories(TypeId PUBLIC include/)

if (CxxTest_FOUND)
    CXXTEST_ADD_TEST(TestTypeId GeneratedTestTypeId.cpp ${CMAKE_CURRENT_SOURCE_DIR}/test/TestTypeId.hpp)
    target_link_libraries(TestTypeId TypeId)
endif ()
