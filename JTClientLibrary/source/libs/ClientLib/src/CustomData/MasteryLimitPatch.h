#pragma once
#include <cstring>

// Recognize the native min(2 * level, cap) selection, or our already
// patched selection. Never guess at an unfamiliar executable's layout.
inline bool BuildMasteryTotalSelection(const unsigned char* original,
    unsigned char* patched, unsigned char destination, int limit)
{
    if (limit < 1 || limit > 10000 ||
        (destination != 0xBF && destination != 0xBE))
        return false;
    const unsigned char reg = destination == 0xBF ? 0xF8 : 0xF0;
    const unsigned char reverseReg = destination == 0xBF ? 0xC7 : 0xC6;
    const bool move = (original[7] == 0x8B && original[8] == reg) ||
        (original[7] == 0x89 && original[8] == reverseReg);
    const bool branch = original[5] == 0x7D || original[5] == 0x7F ||
        original[5] == 0x73 || original[5] == 0x77 || original[5] == 0xEB;
    if (original[0] != 0x3D || !branch || original[6] != 4 || !move ||
        original[9] != 0xEB || original[10] != 5 || original[11] != destination)
        return false;
    std::memcpy(patched, original, 16);
    const unsigned int cap = static_cast<unsigned int>(limit);
    std::memcpy(patched + 1, &cap, 4);
    std::memcpy(patched + 12, &cap, 4);
    patched[5] = 0xEB; // Always select the configured total, preserving cmp flags.
    return true;
}
