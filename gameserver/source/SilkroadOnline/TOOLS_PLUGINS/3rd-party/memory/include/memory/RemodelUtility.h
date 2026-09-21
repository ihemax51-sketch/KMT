#pragma once


#include <windows.h>
#include <iostream>
#include <string>
#include <vector>
#include <list>
#include <map>
#include <functional>
#include <algorithm>
#include <sstream>
#include <fstream>
#include <intrin.h>
#include <queue>
#include "detours.h"

#pragma comment(lib, "detours.lib")
#pragma comment(lib, "ws2_32.lib")

#pragma intrinsic(_ReturnAddress)


#define REMODEL_PLACEHOLDER

#define REMODEL_DECLARE_VIRTUAL_NOIMPL_DCTOR(className) \
	virtual ~className() { };

#define REMODEL_DECLARE_VIRTUAL_NOIMPL(ret_type, fn, ret_value) \
	virtual ret_type fn { return ret_value; }

#define REMODEL_GET_RETURN_ADDRESS() \
	(uintptr_t)_ReturnAddress()

//Bit tricky, some magic with virtual function tables (can be improved later if necessary).
//Works with function prototypes.

#define REMODEL_GET_VIRTUAL_FN(class_ptr, fn_type, fn_offset) \
    CRemodelUtility::GetVirtualFn<fn_type>(class_ptr, fn_offset)

class CRemodelUtility
{
public:
    template<typename T>
    static T GetVirtualFn(void* class_ptr, DWORD fn_offset)
    {
        uintptr_t dwVftFnAddr = *(uintptr_t*)(class_ptr)+fn_offset;
        return reinterpret_cast<T>(*(uintptr_t*)dwVftFnAddr);
    }
};
