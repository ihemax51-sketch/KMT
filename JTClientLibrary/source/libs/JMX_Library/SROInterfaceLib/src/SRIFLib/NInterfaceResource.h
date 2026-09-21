//
// Created by Kurama on 2/1/2023.
//
#pragma once

#include "BSLib/BSLib.h"

#include "GFXFM/IFileManager.h"

#include "NIFWnd.h"

struct UISize {
    int width;
    int height;
};

class CNIRMManager {
public:
    ///\inline 004693b4
    CNIRMManager();

private: // Avoid direct use
    virtual ~CNIRMManager() = 0;

private:
    char pad_0004[0x0028]; //0x0004 unknown struct. container for the rsrc file data maybe
    IFileManager *m_pInterfaceFM; //0x002c
    bool m_bDiskFilePathLoad; //0x0030 // "false Load from pk2"
private:
BEGIN_FIXTURE()
        ENSURE_SIZE(0x0034) // Taken from 0046938f
        ENSURE_OFFSET(m_pInterfaceFM, 0x002c)
        ENSURE_OFFSET(m_bDiskFilePathLoad, 0x0030)
    END_FIXTURE()

    RUN_FIXTURE(CNIRMManager)
};

class CNInterfaceManager {
    // No vftable
public:
    ///\address 004688e0
    CNInterfaceManager();

public:
    ///\address 00469350
    void InitializeResourceData(IFileManager *pMediaFM, BYTE btDefaultLangId, BYTE, bool bDiskFilePathLoad,
                                UISize *pMaxUISize,
                                std::n_string *pStrFont);

    /// Create the interface from 2dt file
    /// \param p2DFilePath Path and filename in Media.pk2 to load. Example: "res_ui/file.2dt"
    /// \param pParent The Parent of the window like "Ginterface"
    /// \param b Unknown  always "false"
    CNIFWnd *InstantiateDimensional(const char *p2DFilePath, CGWnd *pParent, bool b);

    /// Get a pointer to a control identified by the Id
    /// \param nId The Id of the main control in 2dt file
    /// \returns Pointer to the control, NULL if Id was not found.
    CNIFWnd *GetInterfaceObj(int nId);

    /// Get a pointer to a control identified by the Id
    /// \tparam T Expected type of the control. The type is never validated.
    /// \param nId The Id of the main control in 2dt file
    /// \returns Pointer to the control, NULL if Id was not found.
    template<typename T>
    T *GetInterfaceObj(int nId) {
        return (T *) GetInterfaceObj(nId);
    }

public:
    CNIRMManager *m_pResourceMgr; //0x0000
    std::n_map<DWORD, CNIFWnd *> m_mapInterface; //0x0004
    char pad_0010[0x0020]; //0x0010
    bool m_bDiskFilePathLoad; //0x0030
    BYTE m_btDefaultLangId; //0x0031
    char pad_0032[2]; //0x0032
    UISize m_UIClientSize; //0x0034
private:
BEGIN_FIXTURE()
        // ENSURE_SIZE() the size is unknown but iguess its 0x70
        ENSURE_OFFSET(m_pResourceMgr, 0x0000)
        ENSURE_OFFSET(m_mapInterface, 0x0004)
        ENSURE_OFFSET(m_bDiskFilePathLoad, 0x0030)
        ENSURE_OFFSET(m_btDefaultLangId, 0x0031)
        ENSURE_OFFSET(m_UIClientSize, 0x0034)
    END_FIXTURE()

    RUN_FIXTURE(CNInterfaceManager)
};

#define g_sNewInterfaceMgr (*reinterpret_cast<CNInterfaceManager *>(0x0115dd40))
