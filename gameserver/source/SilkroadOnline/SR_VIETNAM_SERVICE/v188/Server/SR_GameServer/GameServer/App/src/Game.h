//
// Created by kyuubi09 on 3/30/2023.
//
#pragma once

#include "STL/SingletonT.h"

#include "Test/Test.h"
#include <list>
#include <map>
#include "GObjPC.h"

class   CGObj;
class   CMsg;

class CGame : public CSingletonT<CGame> {
public:
    virtual ~CGame();

public:
    static CGObjPC *GetCharObjSTAT(const char *pName);

    static CGObjPC *GetCharObjSTAT(DWORD dwCharId);

    static CGObjPC *GetCharObjJID_STAT(DWORD dwAccountJID);

    static CGObj *GetGameObjSTAT(DWORD dwGameId);
    void RemoveGameObjByObjId(DWORD ObjID);
    void RemoveGameObjByWorldId(DWORD ObjID, DWORD dwWorldID);
    void DespawnGObj(IGObj* pObj);
public:
    /// \brief Handling the packets
    ///
    /// \address 00414260
    ///
    void ProcessMessage(CMsg *pMsg);

public:
    CGObjPC *GetCharObjByName(const char *pName);

    CGObjPC *GetCharObjById(DWORD dwCharId);

    void ToTownObjByWorldId(DWORD dwWorldID);

    CGObjPC *GetCharObjByIdNew(DWORD CharId);

    CGObjPC *GetCharObjByJID(DWORD dwAccountJID);

    IGObj *GetObjByGameID(DWORD dwObjID);

    CGObj *GetGameObjById(DWORD dwGameId);
    void ToTownbyWorldAndLayerID(DWORD dwWorldID, WORD LayerID);
private:

public:
    char pad_0004[0x2240]; //0x0004
    std::list<std::pair<std::string, CGObjPC *>> m_listPcCharName; //0x2244
    char pad_2250[0x0040]; //0x2250
    std::map<DWORD, CGObjPC *> m_mapPcJID; //0x2290
    std::map<DWORD, CGObjPC *> m_mapPcCharId; //0x229C
    char pad_22A8[0x0010]; //0x22A8
    std::list<std::pair<DWORD, CGObj *>> m_listObjGameId; //0x22B8
    char pad_22C4[0x001C]; //0x22C4
private:
    BEGIN_FIXTURE()
        ENSURE_SIZE(0x22E0)
        ENSURE_OFFSET(m_listPcCharName, 0x2244)
        ENSURE_OFFSET(m_mapPcJID, 0x2290)
        ENSURE_OFFSET(m_mapPcCharId, 0x229C)
        ENSURE_OFFSET(m_listObjGameId, 0x22B8)
        END_FIXTURE()

    RUN_FIXTURE(CGame)
};

#define g_pCGame (*((CGame**)0x00d6a8e8))


