#!/usr/bin/env python3
from pathlib import Path

root = Path(__file__).resolve().parents[1]
def read(path): return (root / path).read_text(encoding='utf-8-sig')
net = read('source/libs/ClientLib/src/NetProcessIn.cpp')
hunt = read('source/libs/ClientLib/src/Macro/IFMacroMenuAutoHunt.cpp')
pick = read('source/libs/ClientLib/src/Macro/IFMacroMenuPickFilter.cpp')
gui = read('source/libs/ClientLib/src/GInterface.cpp')
safety = read('source/libs/ClientLib/src/Macro/MacroSafety.h')

checks = {
    'bounded party member parser': '%255ls' in net and 'Auto party member %d: %ls' not in net,
    'native 3305 dispatch helper': net.count('DispatchNative3305(this, msg)') >= 7,
    'transactional character maps': 'loadedPartyMembers' in net and 'loadedPartyBuffs' in net and '.swap(loadedPartyMembers)' in net,
    'validated persisted hunt values': net.count('KmtClampMacroSetting') >= 4,
    'macro UI readiness gate': 'featureUiReady' in gui and 'IsRuntimeReady()' in gui,
    'death recovery backoff': 'm_deathRecoveryPending' in hunt and '5000UL' in hunt,
    'pickup inventory cache': 'mergeableItems' in pick and pick.count('GetEmptyInventorySlots()') == 1,
    'dormant pickup removed': 'FoundedCosUniqueID' not in pick and len(pick.splitlines()) < 800,
    'atomic save helper': 'MoveFileExA' in safety and 'MOVEFILE_WRITE_THROUGH' in safety,
    'safe character component': 'KmtSanitizeMacroCharacterName' in safety,
}
failed = [name for name, ok in checks.items() if not ok]
for name, ok in checks.items(): print(('PASS ' if ok else 'FAIL ') + name)
if failed: raise SystemExit('macro stability gates failed: ' + ', '.join(failed))
