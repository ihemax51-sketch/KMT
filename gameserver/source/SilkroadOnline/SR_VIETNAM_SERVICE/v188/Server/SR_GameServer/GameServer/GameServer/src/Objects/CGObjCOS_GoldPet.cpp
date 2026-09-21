//
// Created by YUMBUL on 18.03.2023.
//

#include "CGObjCOS_GOldPet.h"
#include "GObjPC.h"


void CGObjCOS_GoldPet::setspeed(float p1, float p2)
{
    this->speed1 = p1;
    this->speed2 = p2;
}


void CGObjCOS_GoldPet::GoldPetFunc314()
{
    if (Moving == 256)
    {
        std::string charname = this->GetCodeName();
        size_t pos_f = charname.find("FELLOW");
        if (pos_f != -1)
        {
            CGObjPC* pc = static_cast<CGObjPC*>(CGObjPCptr);
            if (pc != NULL)
            {
                pc->speed1 = 80;
                pc->speed2 = 150;
            }
            this->speed1 = 80;
            this->speed2 = 150;
        }
    }
    std::string charname = this->GetCodeName();
    size_t pos_f = charname.find("FELLOW");
    if (pos_f != -1)
    {
        CGObjPC* pc = static_cast<CGObjPC*>(CGObjPCptr);
        if (pc != NULL)
        {
            pc->speed1 = 80;
            pc->speed2 = 150;
        }
        this->speed1 = 80;
        this->speed2 = 150;
    }
    reinterpret_cast<void(__thiscall*)(CGObjCOS_GoldPet*)>(0x004d4a30)(this);

}


void CGObjCOS_GoldPet::FUNC2155()
{
    std::string charname = this->GetCodeName();
    size_t pos_f = charname.find("FELLOW");
    if (pos_f != -1)
    {
        CGObjPC* pc = static_cast<CGObjPC*>(CGObjPCptr);
        if (pc != NULL)
        {
            pc->speed1 = 16;
            pc->speed2 = 150;
        }
        this->speed1 = 16;
        this->speed2 = 150;
    }
    reinterpret_cast<void(__thiscall*)(CGObjCOS_GoldPet*)>(0x004d5d50)(this);
}

void CGObjCOS_GoldPet::FUNC362()
{
    std::string charname = this->GetCodeName();
    size_t pos_f = charname.find("FELLOW");
    if (pos_f != -1)
    {
        CGObjPC* pc = static_cast<CGObjPC*>(CGObjPCptr);
        if (pc != NULL)
        {
            if (pc->ZERKING == 0)
            {
                this->FUN_004d2ad0();
            }

        }

    }
    reinterpret_cast<void(__thiscall*)(CGObjCOS_GoldPet*)>(0x004c3e70)(this);
}
bool firstspawn;
void CGObjCOS_GoldPet::FUN_004d2ad0()
{
    const std::string codeName = this->GetCodeName();
    if (codeName.find("FELLOW") != std::string::npos && this->SpeedParamPtr != NULL)
        this->SpeedParamPtr->SpeedValue = 150.0f;



    reinterpret_cast<void(__thiscall*)(CGObjCOS_GoldPet*)>(0x004d2ad0)(this);

    //CallVirtual<void* (__thiscall*)(CGObjCOS_GoldPet*)>(this, 314)(this);
}

void CGObjCOS_GoldPet::FUNCMovement(unsigned int param)
{
    //printf("%p\n", param);

    std::string charname = this->GetCodeName();
    size_t pos_f = charname.find("FELLOW");
    if (pos_f != -1)
    {
        CGObjPC* pc = static_cast<CGObjPC*>(CGObjPCptr);
        if (pc != NULL)
        {

            if (pc->OnPet)
            {

                this->speed1 = 16.00;
                this->speed2 = 100.00;
                this->speed3 = 100.00;
                pc->speed1 = 16.00;
                pc->speed2 = 100.00;
                pc->speed3 = 100.00;
            }
        }

    }
    reinterpret_cast<void(__thiscall*)(CGObjCOS_GoldPet*, unsigned int)>(0x004b0ea0)(this, param);
}

unsigned int CGObjCOS_GoldPet::FunNew(unsigned int param)
{

    unsigned int aaa = reinterpret_cast<unsigned int(__thiscall*)(CGObjCOS_GoldPet*, unsigned int)>(0x004d29a0)(this, param);
    //printf("%p aaa\n", aaa);
    //printf("%p param\n", param);
    //printf("%p this\n", this);
    this->speed1 = 100;
    this->speed2 = 150;


    return aaa;
}
void CGObjCOS_GoldPet::Func308(unsigned int param1)
{
    std::string charname = this->GetCodeName();
    size_t pos_f = charname.find("FELLOW");
    if (pos_f != -1)
    {
        CGObjPC* pc = static_cast<CGObjPC*>(CGObjPCptr);
        if (pc != NULL)
        {
            if (pc->OnPet == 1)
            {
                pc->speed1 = this->speed1;
                pc->speed2 = this->speed2;
                pc->speed3 = this->speed3;
            }

        }

    }
    reinterpret_cast<void(__thiscall*)(CGObjCOS_GoldPet*, unsigned int)>(0x004d5e00)(this, param1);
}


void CGObjCOS_GoldPet::FUN230(unsigned int param)
{
    std::string charname = this->GetCodeName();
    size_t pos_f = charname.find("FELLOW");
    if (pos_f != -1)
    {
        CGObjPC* pc = static_cast<CGObjPC*>(CGObjPCptr);
        if (pc != NULL)
        {
            if (pc->OnPet == 1)
            {

            }
            pc->speed1 = 100;
            pc->speed2 = 150;
            this->speed1 = 100;
            this->speed2 = 150;
        }
    }


    reinterpret_cast<void(__thiscall*)(CGObjCOS_GoldPet*, unsigned int)>(0x0048ba10)(this, param);
}


void CGObjCOS_GoldPet::FUN343(unsigned int param1, unsigned int param2)
{
    //printf("%p !!! %p !!! %p\n", param1, param2);
    std::string charname = this->GetCodeName();
    size_t pos_f = charname.find("FELLOW");
    if (pos_f != -1)
    {
        CGObjPC* pc = static_cast<CGObjPC*>(CGObjPCptr);
        if (pc != NULL)
        {
            pc->speed1 = 80;
            pc->speed2 = 150;
            this->speed1 = 80;
            this->speed2 = 150;
        }

    }
    reinterpret_cast<void(__thiscall*)(CGObjCOS_GoldPet*, unsigned int, unsigned int)>(0x004d2a00)(this, param1, param2);
}


void CGObjCOS_GoldPet::FUN348(float param)
{
    //printf("%.2lf 22222222222222222\n", param);

    std::string charname = this->GetCodeName();
    size_t pos_f = charname.find("FELLOW");
    if (pos_f != -1)
    {
        this->speed1 = 80;
        this->speed2 = 150;
    }

    reinterpret_cast<void(__thiscall*)(CGObjCOS_GoldPet*, unsigned int)>(0x004c19f0)(this, param);
}



