//
// Created by Kurama on 12/11/2022.
//
#include "Base.h"

const GlobalVar<CRuntimeClass, 0x00add8c4> CBase::classCBase;

CRuntimeClass const *CBase::GetRuntimeClass() const {
    return (classCBase);
}

CBase *CBase::CreateObject() {
    return new CBase;
}

void CBase::DeleteObject(CBase *pObj) {
    delete pObj;
};