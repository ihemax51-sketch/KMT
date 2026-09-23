#!/usr/bin/env python3
"""Source-level regression gates for the process-bound vSRO 188 Client DLL.

These checks complement, but never replace, a VC80 Win32 build and live-client
staging. They deliberately validate architectural invariants that previously
regressed silently.
"""

from pathlib import Path
import hashlib
import re

ROOT = Path(__file__).resolve().parents[1]


def read(relative: str) -> str:
    return (ROOT / relative).read_text(encoding="utf-8-sig", errors="strict")


def require(condition: bool, message: str) -> None:
    if not condition:
        raise AssertionError(message)


hook_h = read("source/support/include/support/hook.h")
hook_cpp = read("source/support/src/hook.cpp")
member_h = read("source/support/include/support/MemberFunctionHook.h")
util_cpp = read("source/DevKit_DLL/src/Util.cpp")
d3d_cpp = read("source/DevKit_DLL/src/hooks/GFXVideo3d_Hook.cpp")
discord_cpp = read("source/libs/DiscordRichPresence/src/DiscordRichPresence/DiscordManager.cpp")
options = read("cmake/Options.cmake")

for name in ("placeHook", "replaceOffset", "replaceAddr", "vftableHook"):
    require(re.search(r"bool\s+" + name + r"\s*\(", hook_h) is not None,
            f"{name} must return an explicit result")
require("memcmp(destination, data, size)" in hook_cpp, "patch writes require readback")
require("FlushInstructionCache" in hook_cpp, "patch writes require cache flush")
require("if (!replaceOffset(address, target))" in util_cpp,
        "diagnostic wrapper must propagate patch failure")
require("static bool ApplyAll()" in member_h and "entry->installed = verified" in member_h,
        "member-hook registry must propagate verified status")

end_scene = d3d_cpp.split("bool CGFXVideo3D_Hook::EndSceneHook", 1)[1].split(
    "bool CGFXVideo3D_Hook::SetSizeHook", 1)[0]
require("ApplyTextureQualityBoost" not in end_scene and "ApplyColorQualityBoost" not in end_scene,
        "quality state must not be re-applied every frame")
require("TestCooperativeLevel" in end_scene, "EndScene requires device lifecycle check")
require("return resized;" in d3d_cpp, "SetSize must preserve native result")

thread_body = discord_cpp.split("DWORD WINAPI DiscordManager::DiscordThread", 1)[1]
for forbidden in ("g_pMyPlayerObj", "g_CTextStringManager", "g_pCGInterface"):
    require(forbidden not in thread_body, f"Discord worker accesses native object: {forbidden}")
require("WaitForSingleObject(m_Thread, 10000)" in discord_cpp,
        "Discord worker needs deterministic join")

require('option(CONFIG_IMGUI "Enable ImGui (unsupported without complete D3D reset integration)" OFF)' in options,
        "unsupported ImGui integration must default off")
require('option(KMT_ENABLE_QUICKSTART "Enable developer QuickStart INI support" OFF)' in options,
        "QuickStart must default off")

hud = ROOT / "source/libs/ClientLib/prebuilt/DesktopCharacterHud.vc80.obj"
actual = hashlib.sha256(hud.read_bytes()).hexdigest()
require(actual == "b0f51f4b34c687f096ff81108d4a2d32681c63cec4bd19ddc4d2d23d8956f54a",
        "prebuilt Desktop HUD hash changed")

for relative in (
    "source/libs/ClientLib/src/Macro/IFMacroMenuAutoSkill.cpp",
    "source/libs/ClientLib/src/Macro/IFMacroMenuAutoHunt.cpp",
    "source/libs/ClientLib/src/Macro/IFMacroMenuPickFilter.cpp",
):
    require("KmtFormatPath" in read(relative), f"bounded path helper missing in {relative}")

print("Client DLL stability source gates: PASS")
