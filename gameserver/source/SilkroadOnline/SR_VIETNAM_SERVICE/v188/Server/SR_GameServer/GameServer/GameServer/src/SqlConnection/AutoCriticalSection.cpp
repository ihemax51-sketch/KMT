#include "AutoCriticalSection.h"

CAutoCriticalSection::CAutoCriticalSection()
{
    InitializeCriticalSection(&m_cs);
}

CAutoCriticalSection::~CAutoCriticalSection()
{
    DeleteCriticalSection(&m_cs);
}

void CAutoCriticalSection::Enter()
{
    EnterCriticalSection(&m_cs);
}

void CAutoCriticalSection::Leave()
{
    LeaveCriticalSection(&m_cs);
}