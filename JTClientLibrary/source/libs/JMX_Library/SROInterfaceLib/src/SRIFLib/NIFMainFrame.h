//
// Created by Kurama on 2/25/2023.
//
#pragma once

#include "NIFFrame.h"
#include "NIFStatic.h"
#include "NIFDragableArea.h"
#include "NIFButton.h"

class CNIFMainFrame : public CNIFFrame {
GFX_DECLARE_DYNAMIC_EXISTING(CNIFMainFrame, 0x0115dfa8)

GFX_DECLARE_MESSAGE_MAP(CNIFMainFrame)

public:
    /// \address 004799c0
    CNIFMainFrame();

public:
    /// \address 00479a50
    bool OnCreate(long ln) override;

    /// \address 00479dd0
    void SetGWndSize(int nWidth, int nHeight) override;

    /// \address 00479e10
    bool SetText(const wchar_t *pSrc) override;

private:
    /// \address 00479d20
    void UpdateElementsDimension();

    /// \address 00479e20
    void OnCloseBtnClick();

private:
    CNIFStatic *m_pTitleCtrl; //0x078C
    CNIFDragableArea *m_pDragAreaCtrl; //0x0790
    CNIFButton *m_pCloseBtnCtrl; //0x0794
private:
BEGIN_FIXTURE()
        ENSURE_SIZE(0x0798)
        ENSURE_OFFSET(m_pTitleCtrl, 0x078C)
        ENSURE_OFFSET(m_pDragAreaCtrl, 0x0790)
        ENSURE_OFFSET(m_pCloseBtnCtrl, 0x0794)
    END_FIXTURE()

    RUN_FIXTURE(CNIFMainFrame)
};