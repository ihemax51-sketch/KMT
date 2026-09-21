//
// Created by Kurama on 12/11/2022.
//

#include "GObj.h"
#include <memory/MemoryUtility.h>

IMPLEMENT_RUNTIMECLASS_EXISTING(CGObj, 0x00adebb8)


IGObj* IGObj::GetOwner() const
{
	return MEMUTIL_READ_BY_PTR_OFFSET(this, 0x1CD8, IGObj*);
}
