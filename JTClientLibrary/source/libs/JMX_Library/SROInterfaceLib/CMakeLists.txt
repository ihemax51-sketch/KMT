add_library(SROInterfaceLIB STATIC src/SRIFLib/NIFWorldMap.cpp src/SRIFLib/NIFEnchantWnd.cpp src/SRIFLib/NIFEnchantWnd.h src/SRIFLib/NIFLattice.h src/SRIFLib/NIFAlchemyWnd.cpp src/SRIFLib/NIFAlchemyWnd.h src/SRIFLib/IFSlotWithHelpEx.cpp)

target_sources(SROInterfaceLIB PRIVATE
        src/SRIFLib/NInterfaceResource.cpp
        src/SRIFLib/NGFontTexture.cpp
        src/SRIFLib/NIFTextBoard.cpp
        src/SRIFLib/NIFWnd.cpp
        src/SRIFLib/NIFUnderMenuBar.cpp
        src/SRIFLib/NIFCommunityWnd.cpp
        src/SRIFLib/NIFBlockWnd.cpp
        src/SRIFLib/NIFTileWnd.cpp
        src/SRIFLib/NIFFrame.cpp
        src/SRIFLib/NIFMainFrame.cpp
)

target_include_directories(SROInterfaceLIB PUBLIC src/)

target_compile_definitions(SROInterfaceLIB PRIVATE KMT_UNIQUE_MAP_DIAGNOSTICS=0)

target_link_libraries(SROInterfaceLIB
        BSLib
        # at the end need to add its, shit!
        ClientLib
        GFXFileManagerLIB
        GFX3DFunction
        support
)
