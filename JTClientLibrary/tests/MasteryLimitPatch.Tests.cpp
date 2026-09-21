#include "../source/libs/ClientLib/src/CustomData/MasteryLimitPatch.h"
#include <cassert>
#include <cstring>

static unsigned int ReadCap(const unsigned char* bytes, int offset)
{
    unsigned int result = 0;
    std::memcpy(&result, bytes + offset, sizeof(result));
    return result;
}

int main()
{
    const unsigned char originalEdi[16] = {
        0x3D, 0xF0, 0x00, 0x00, 0x00, 0x7D, 0x04, 0x8B,
        0xF8, 0xEB, 0x05, 0xBF, 0xF0, 0x00, 0x00, 0x00
    };
    const unsigned char originalEsi[16] = {
        0x3D, 0xF0, 0x00, 0x00, 0x00, 0x7D, 0x04, 0x8B,
        0xF0, 0xEB, 0x05, 0xBE, 0xF0, 0x00, 0x00, 0x00
    };
    const int limits[] = {1, 220, 240, 255, 256, 300, 330, 360, 10000};
    for (int i = 0; i < sizeof(limits) / sizeof(limits[0]); ++i) {
        unsigned char patched[16];
        assert(BuildMasteryTotalSelection(originalEdi, patched, 0xBF, limits[i]));
        assert(patched[5] == 0xEB && patched[6] == 0x04);
        assert(ReadCap(patched, 1) == static_cast<unsigned int>(limits[i]));
        assert(ReadCap(patched, 12) == static_cast<unsigned int>(limits[i]));
        assert(BuildMasteryTotalSelection(originalEsi, patched, 0xBE, limits[i]));
        assert(ReadCap(patched, 1) == static_cast<unsigned int>(limits[i]));
        assert(ReadCap(patched, 12) == static_cast<unsigned int>(limits[i]));
    }
    unsigned char invalid[16];
    std::memcpy(invalid, originalEdi, sizeof(invalid));
    invalid[9] = 0x90;
    unsigned char untouched[16];
    std::memset(untouched, 0xCC, sizeof(untouched));
    assert(!BuildMasteryTotalSelection(invalid, untouched, 0xBF, 300));
    assert(!BuildMasteryTotalSelection(originalEdi, untouched, 0xBF, 0));
    assert(!BuildMasteryTotalSelection(originalEdi, untouched, 0xBF, 10001));
    return 0;
}
