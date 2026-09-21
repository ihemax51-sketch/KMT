#pragma once
#include <IFStatic.h>
#include <IFWnd.h>
#include <IFBarWnd.h>

class CIFUniqueHistorySlot : public CIFWnd
{
GFX_DECLARE_DYNCREATE(CIFUniqueHistorySlot)
GFX_DECLARE_MESSAGE_MAP(CIFUniqueHistorySlot)
public:
    CIFUniqueHistorySlot(void);
    ~CIFUniqueHistorySlot(void);
    bool OnCreate(long ln) override;
    void OnUpdate() override;
    int OnMouseLeftUp(int a1, int x, int y) override;
    void SetName(int Num, const wchar_t* uniquename, byte state, __int64 time, const wchar_t* killer,
                 int RegionID, float KilledX, float KilledY, float KilledZ,
                 int WorldID, byte MapType, int MapIndex, int UQID);
    void ClearDDJ();
    void SelectDDJ();
    void Clear();
public:
    unsigned __int64 times;
    int KilledRegID;
    float X;
    float Z;
    float Y;
    int WorldID;
    byte MapType;
    int MapIndex;


    int UniqueID;
    std::n_wstring UniqName;

};
