#pragma once

class CIFMenu;

namespace MenuDesign {

/// Loads and applies clientlibrary\config\menu_design.json from Media.pk2.
/// Missing or invalid files leave the original resinfo design untouched.
bool Apply(CIFMenu &menu);

/// Applies the optional design while containing client-side access violations.
/// This is intentionally kept separate from Apply() so MSVC can use SEH
/// without disabling C++ object unwinding in the parser itself.
bool ApplySafely(CIFMenu &menu);

} // namespace MenuDesign
