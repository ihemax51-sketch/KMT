add_library(GFX3DFunction STATIC)

target_sources(GFX3DFunction PRIVATE
        src/GFX3DFunction/DrawingHelpers.cpp
        src/GFX3DFunction/GFontTexture.cpp
        src/GFX3DFunction/GFXVideo3d.cpp
        src/GFX3DFunction/Common/d3dapp.cpp
        src/GFX3DFunction/Common/d3denumeration.cpp
        src/GFX3DFunction/Common/d3dsettings.cpp
        src/GFX3DFunction/Common/d3dutil.cpp
        src/GFX3DFunction/Common/dxutil.cpp
        src/GFX3DFunction/RStateMgr.cpp
        src/GFX3DFunction/RTLoading.cpp
)

target_link_libraries(GFX3DFunction GFXMainFrame d3d9 GFXFileManagerLIB)

target_include_directories(GFX3DFunction PUBLIC src/)
