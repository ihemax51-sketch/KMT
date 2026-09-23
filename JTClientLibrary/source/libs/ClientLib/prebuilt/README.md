# DesktopCharacterHud prebuilt boundary

`DesktopCharacterHud.vc80.obj` is a PE-i386 COFF object retained because its
original C++ implementation is not present in this repository or its available
Git history.

* SHA-256: `b0f51f4b34c687f096ff81108d4a2d32681c63cec4bd19ddc4d2d23d8956f54a`
* Recorded compiler marker: COFF `@comp.id` value `0x006ec627`; the filename and
  project convention identify the expected toolchain as Microsoft Visual C++
  2005 (VC80), but full provenance is unknown.
* Registration: `DesktopCharacterHud::GameWndProcHook` is registered by
  `DevKit_DLL/src/DllMain.cpp` through `OnWndProc` before hook publication.
* ABI assumptions: 32-bit PE/COFF, MSVC x86 ABI, current
  `ExtraUI/DesktopCharacterHud.h` declarations, and process-lifetime linkage.

CMake verifies the hash before linking. A changed object is rejected until its
source/provenance and ABI have been reviewed and this pinned hash is deliberately
updated. The safety of the object's internal implementation remains **Unknown / Needs Verification**.
