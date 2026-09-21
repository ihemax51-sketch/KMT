add_library(GFXFileManagerLIB STATIC)

target_sources(GFXFileManagerLIB PRIVATE
        src/IFileManager.cpp
)

target_include_directories(GFXFileManagerLIB
        PUBLIC include/
        PRIVATE include/GFXFM)
