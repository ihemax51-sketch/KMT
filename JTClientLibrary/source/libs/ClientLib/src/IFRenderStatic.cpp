#include "IFRenderStatic.h"
#include "GlobalDataManager.h"
#include "ICPlayer.h"

GFX_IMPLEMENT_DYNAMIC_EXISTING(CIFRenderStatic, 0x00ee7a78)

CIFRenderStatic::CIFRenderStatic()
{
    reinterpret_cast<CIFRenderStatic *(__thiscall *)(CIFRenderStatic *)>(0x0055f7f0)(this);
    //
}
CIFRenderStatic::~CIFRenderStatic()
{
    reinterpret_cast<CIFRenderStatic *(__thiscall *)(CIFRenderStatic *)>(0x00560a50)(this);
}

int CIFRenderStatic::OnMouseLeftUpIMPL(int a1, int x, int y){
    //WhenClickedis1 = 1;
    return true;
}
void CIFRenderStatic::RenderMyselfIMPL() {

    reinterpret_cast<void (__thiscall *)(CIFRenderStatic *)>(0x00560fc0)(this);
}

unsigned int CIFRenderStatic::FUN_0055ee20(unsigned int param_1, short p2, short p3) {

    return reinterpret_cast<unsigned int(__thiscall *)(CIFRenderStatic *, unsigned int, short, short)>(0x0055ee20)(this, param_1, p2, p3);

}

unsigned int CIFRenderStatic::FUN_005602c0(unsigned int param_1, unsigned int param_2)
{
    return reinterpret_cast<unsigned int(__thiscall*)(CIFRenderStatic*, unsigned int, unsigned int)>(0x005602c0)(this, param_1, param_2);
}

void CIFRenderStatic::Test(unsigned int * p1)
{
    reinterpret_cast<void *(__thiscall *)(CIFRenderStatic *, unsigned int*)>(0x0055e820)(this, p1);
}

undefined CIFRenderStatic::Initialize()
{
    return reinterpret_cast<undefined(__thiscall *)(CIFRenderStatic *)>(0x005612f0)(this);
}

void CIFRenderStatic::RemovePet(int p1)
{
    reinterpret_cast<void *(__thiscall *)(CIFRenderStatic *, int)>(0x00561480)(this, p1);
}

void CIFRenderStatic::Test1()
{
    reinterpret_cast<void *(__thiscall *)(CIFRenderStatic *)>(0x00561250)(this);
}


void CIFRenderStatic::Test2(unsigned int * p1, unsigned int p2)
{
    reinterpret_cast<void *(__thiscall *)(CIFRenderStatic *, unsigned int*, unsigned int)>(0x0055efd0)(this, p1, p2);
}


void CIFRenderStatic::Test3(unsigned int p1, int p2)
{
    reinterpret_cast<void *(__thiscall *)(CIFRenderStatic *, unsigned int, int)>(0x005603b0)(this, p1, p2);
}


unsigned int CIFRenderStatic::GetCharacterObj(undefined4 param_1, CCObjCharacter *param_2) {
    unsigned int a = reinterpret_cast<unsigned int(__thiscall *)(CIFRenderStatic *,int, CCObjCharacter *)>(0x00561930)(this, param_1, param_2);
    return a;
}

CCObjCharacter* CIFRenderStatic::GetCharacterAnimationObject(unsigned int objectIndex)
{
    // CIFRenderStatic owns a vector of preview entries at +0x380. GetCharacterObj
    // returns an index into that vector; each entry stores its CCObjCharacter at +4.
    __try {
        unsigned int* entriesBegin = *reinterpret_cast<unsigned int**>(reinterpret_cast<char*>(this) + 0x384);
        unsigned int* entriesEnd = *reinterpret_cast<unsigned int**>(reinterpret_cast<char*>(this) + 0x388);
        if (!entriesBegin || !entriesEnd || entriesEnd < entriesBegin) {
            return 0;
        }

        const unsigned int entryCount = static_cast<unsigned int>(entriesEnd - entriesBegin);
        if (entryCount > 256 || objectIndex >= entryCount) {
            return 0;
        }

        const unsigned int entry = entriesBegin[objectIndex];
        if (!entry) {
            return 0;
        }

        return *reinterpret_cast<CCObjCharacter**>(entry + 4);
    } __except(EXCEPTION_EXECUTE_HANDLER) {
        return 0;
    }
}

undefined CIFRenderStatic::Clear(void)
{
    return reinterpret_cast<undefined(__thiscall *)(CIFRenderStatic *)>(0x00561040)(this);
}

int CIFRenderStatic::FUN_00561780(unsigned int param_1) {

    return reinterpret_cast<int (__thiscall *) (CIFRenderStatic *, unsigned int)>(0x00561780)(this, param_1);
}

void CIFRenderStatic::RemoveItem(undefined4 param_1, int param_2, int param_3)
{

    reinterpret_cast<void*(__thiscall*)(CIFRenderStatic*, undefined4, int, int)>(0x0055ff20)(this, param_1, param_2, param_3);
}

void CIFRenderStatic::WearItem(unsigned int param_1, int param_2, int param_3)
{
    // printf("test new %d\n %d\n %d\n", param_1, param_2, param_3);


    reinterpret_cast<void*(__thiscall*)(CIFRenderStatic*, unsigned int, int, int)>(0x00560030)(this, param_1, param_2, param_3);


}
void CIFRenderStatic::FUN_0055fe70(unsigned int param_1, unsigned int *param_2){

    reinterpret_cast<void*(__thiscall *)(CIFRenderStatic *, unsigned int, unsigned int*)>(0x0055fe70)(this, param_1, param_2);
}

void CIFRenderStatic::FUN_00561170(undefined4 param_1, undefined4 param_2) {
    reinterpret_cast<void*(__thiscall *)(CIFRenderStatic *, undefined4, undefined4)>(0x00560140)(this, param_1, param_2);
}

undefined4 CIFRenderStatic::FUN_00560720(int param_1)
{
    return reinterpret_cast<undefined4(__thiscall *)(CIFRenderStatic *, int)>(0x00560720)(this, param_1);
}
