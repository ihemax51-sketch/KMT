#pragma once

#include "BSLib/BSLib.h"
#include "GlobalDataManager.h"
#include "EntityManagerClient.h"

class CDropItemManager {
public:
    int sub_8AF7F0(char a2,unsigned __int16 PetRegionID,int PetPosition, DWORD a5);
};

#define g_CDropItemMgr (*(CDropItemManager *) 0x00eedd14)
