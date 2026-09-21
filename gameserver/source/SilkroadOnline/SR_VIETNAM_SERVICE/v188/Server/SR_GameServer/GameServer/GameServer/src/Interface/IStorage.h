///******************************************************************************
/// \File IStorage.h
///
/// \Desc
///
///******************************************************************************

#pragma once

enum eStorageType : int {
    NORMAL_STORAGE_TYPE = -1,
    PC_INVENTORY_TYPE,
    NPC_INVENTORY_TYPE,
    COS_INVENTORY_TYPE,
    CHEST_TYPE,
    GUILD_CHEST_TYPE,
    AVATAR_INVENTORY_TYPE,
    MAGIC_CUBE_STORAGE_TYPE,
};

// It doesn't have anything. As I know for now
/// added "_" because microsoft shit real name "IStorage"
class IStorage_ {

};
