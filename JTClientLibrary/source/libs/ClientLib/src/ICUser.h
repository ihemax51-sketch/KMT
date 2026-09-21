#pragma once

#include "ICharactor.h"
#include "UserGuildInfo.h"

class CICUser : public CICharactor {
    GFX_DECLARE_DYNAMIC_EXISTING(CICUser, 0x00ef1bc0)
public:
    enum eUserTimer : int {
        TIMER_MESSAGE_ABOVE = 10,
    };

public:
    /// \address 009dea30

    void Func_15(int param_1, float *param_2) override;
    void Func_15_impl(int param_1, float *param_2);
    /// \address 009db100
    const std::n_wstring &GetGuildName() const;

    void Render(void);

    UserGuildInfo *GetGuildInfo() const;

    /// \address 009db0e0
    unsigned char GetJobLevel() const;

    /// \address 009db0d0
    undefined1 FUN_009db0d0() const;
    void Func_23_IMPL();
    /// \address 009dad40
    bool GetBeginnerState() const;
    
    /// \brief Display the given text message above the entity for a certain amount of time.
    /// \address 009c77c0
    /// \param message The message to be displayed
    /// \param color Color of the letters being displayed
    void ShowMessageAboveEntity(std::n_wstring &message, D3DCOLOR color);

private:
    void RenderFortressIcons(D3DVECTOR *dataOut, float *param_2);

private:
    undefined4 field_078c; //0x078C CObjAnimation ptr?
    char pad_0790[48]; //0x0790
public:
    int ScrollStateMaybe; // 0x07c0
private:
    UserGuildInfo *m_pGuildInfo; //0x07C4
    std::n_wstring m_wstrGrantName; //0x07C8
    std::n_wstring m_wstrGuildName; //0x07E4
    std::n_wstring m_wstrFortressWarTitle; //0x0800
    undefined1 field_81c; //0x081C
public:
    undefined1 field_81d; //0x081D
    char pad_081E[6];   //0x081E
public:
    byte m_PVPCapeState;//0x0824
    char pad_0825[7];   //0x0825

    BEGIN_FIXTURE()
        ENSURE_SIZE(2092)
        ENSURE_OFFSET(field_078c, 0x078c)
        ENSURE_OFFSET(m_pGuildInfo, 0x07C4)
        ENSURE_OFFSET(m_wstrGrantName, 0x07C8)
        ENSURE_OFFSET(m_wstrGuildName, 0x07E4)
        ENSURE_OFFSET(m_wstrFortressWarTitle, 0x0800)
    END_FIXTURE()

    RUN_FIXTURE(CICUser)
};
