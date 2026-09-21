//
// Created by Kurama on 12/14/2022.
//
#include "CriticalSectionBS.h"

CSynchObject::~CSynchObject() {
    if (m_hObject != NULL) {
        CloseHandle(m_hObject);
        m_hObject = NULL;
    }
}

bool CSynchObject::LockForObject(DWORD dwMS) {
    return (WaitForSingleObject(m_hObject, dwMS) == FALSE);
}

CCriticalSectionBS::CCriticalSectionBS() {
    InitializeCriticalSection(&m_rtlCS);
}

CCriticalSectionBS::~CCriticalSectionBS() {
    DeleteCriticalSection(&m_rtlCS);
}

bool CCriticalSectionBS::LockForObject(DWORD dwMS) {
    EnterCriticalSection(&m_rtlCS);
    return true;
}

bool CCriticalSectionBS::UnLockObject() {
    LeaveCriticalSection(&m_rtlCS);
    return true;
}