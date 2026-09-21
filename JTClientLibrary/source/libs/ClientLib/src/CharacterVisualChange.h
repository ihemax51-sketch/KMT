#pragma once

#define g_CharacterVisualChange (*(CharacterVisualChange::CDataManager**)0x00ee73c0)
#define g_CharacterVisualChangeDataManager (*reinterpret_cast<CharacterVisualChange::CDataManager*>(0x00ee73c0))

class CharacterVisualChange
{
public:
    struct CDataManager
    {
        unsigned int FUN_004ec340(int param_1);
    };
public:

};