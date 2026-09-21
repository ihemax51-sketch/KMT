//
// Created by YUMBUL on 25.03.2024.
//

#include "InstanceGuild.h"

int CInstanceGuild::GetID() const
{
    return MEMUTIL_READ_BY_PTR_OFFSET(this, 0x18, int);
}

int CInstanceGuild::GetAllianceID() const
{
    return MEMUTIL_READ_BY_PTR_OFFSET(this, 0x50, int);
}