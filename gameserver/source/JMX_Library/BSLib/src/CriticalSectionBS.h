//
// Created by Kurama on 12/14/2022.
//
#pragma once

#include <Windows.h>
#include <string>

#include "Test/Test.h"

class CSynchObject;
class CCriticalSectionBS;

class CSynchObject {
public:
    virtual ~CSynchObject();

public:
    virtual bool LockForObject(DWORD dwMS);

    virtual bool Func_2() { return true; }

    virtual bool UnLockObject() = 0;

    virtual bool Func_4() { return true; }

private:
    HANDLE m_hObject; //0x0004
    std::string m_str; //0x0008 im not sure if it a string
private:
BEGIN_FIXTURE()
        ENSURE_SIZE(0x0024)
        ENSURE_OFFSET(m_hObject, 0x0004)
        ENSURE_OFFSET(m_str, 0x0008)
    END_FIXTURE()

    RUN_FIXTURE(CSynchObject)
};

class CCriticalSectionBS : public CSynchObject {
public:
    CCriticalSectionBS();

    ~CCriticalSectionBS();

public:
    bool LockForObject(DWORD dwMS) override;

    bool UnLockObject() override;

    bool Func_4() override { return true; }

private:
    CRITICAL_SECTION m_rtlCS; //0x0024
private:
BEGIN_FIXTURE()
        ENSURE_SIZE(0x003C)
        ENSURE_OFFSET(m_rtlCS, 0x0024)
    END_FIXTURE()

    RUN_FIXTURE(CCriticalSectionBS)
};