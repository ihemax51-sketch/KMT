//
// Created by YUMBUL on 18.03.2023.
//

#include "CGOBjCOS_Pet.h"

void CGObjCOS_Pet::FUN215()
{

    std::string charname = this->GetCodeName();
    size_t pos_f = charname.find("FELLOW");
    if (pos_f != -1)
    {
        this->speed1 = 80;
        this->speed2 = 150;
        return;
    }
    reinterpret_cast<void(__thiscall*)(CGObjCOS_Pet*)>(0x004a8310)(this);

}
