//
// Created by YUMBUL on 19.03.2023.
//
#pragma once

#include <cstdio>

struct PosInfo
{
    unsigned __int16 region_id;
    float x;
    float y;
    float z;

    PosInfo() : region_id(0), x(0), y(0), z(0)
    {
    }
    PosInfo(unsigned __int16 region_id, float x, float y, float z) : region_id(region_id), x(x), y(y), z(z)
    {
    }
};

struct NavInfo
{
    unsigned int crt_nav_cell_quad_ptr;
    __int32 unk0;
    __int16 region_id;
    __int16 unk1;
    float x;
    float y;
    float z;

    NavInfo() : crt_nav_cell_quad_ptr(NULL), unk0(0), region_id(0), unk1(0), x(0), y(0), z(0)
    {
    }

    NavInfo(__int16 region_id, float x, float y, float z) : crt_nav_cell_quad_ptr(NULL), unk0(0), region_id(region_id), unk1(0), x(x), y(y), z(z)
    {
    }

    NavInfo(const PosInfo& pos_info) : crt_nav_cell_quad_ptr(NULL), unk0(0), region_id(pos_info.region_id), unk1(0), x(pos_info.x), y(pos_info.y), z(pos_info.z)
    {

    }
};

class CRegionManagerBody
{
private:
    static CRegionManagerBody* GetInstance();
public:
    // Check if the navigation info provided can be solved for a successfully spawn
    static bool ResolveCellAndHeight(NavInfo* NavInfoPtr);
};