#pragma once

#include <Test/Test.h>
#include "InstanceObj.h"
#include "MyInstanceObj.h"
#include <vector>

class CInstanceItem : public MyCInstanceObj
{
public:
    void SetPlus(BYTE btValue);
 
public:
	INT64 ID64; //0x0020
	int RefItemID; //0x0028
	BYTE OptLevel; //0x002C
	BYTE N00001958; //0x002D
	BYTE N0000195C; //0x002E
	BYTE N00001959; //0x002F
	INT64 Variance; //0x0030
	int Data; //0x0038
	char pad_003C[28]; //0x003C
	int MagParamNumber; //0x0058
	char pad_005C[4]; //0x005C
	INT64 MagParam1; //0x0060
	INT64 MagParam2; //0x0068
	INT64 MagParam3; //0x0070
	INT64 MagParam4; //0x0078
	INT64 MagParam5; //0x0080
	INT64 MagParam6; //0x0088
	INT64 MagParam7; //0x0090
	INT64 MagParam8; //0x0098
	INT64 MagParam9; //0x00A0
	INT64 MagParam10; //0x00A8
	INT64 MagParam11; //0x00B0
	INT64 MagParam12; //0x00B8
	INT64 Serial64; //0x00C0
	std::n_list<byte> map;
	//char pad_00C8[4]; //0x00C8

private:
BEGIN_FIXTURE()
        ENSURE_SIZE(216)
        ENSURE_OFFSET(ID64, 0x0020)
        ENSURE_OFFSET(map, 0x00C8)
    END_FIXTURE()

    RUN_FIXTURE(CInstanceItem)
}; //Size: 0x00D8

