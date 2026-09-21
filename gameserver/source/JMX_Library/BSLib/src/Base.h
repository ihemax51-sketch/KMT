//
// Created by Kurama on 11/14/2022.
//
#pragma once

#include "Windows.h"

#include <Test/Test.h>

#include "remodel/GlobalVar.h"

struct CRuntimeClass;
class CBase;

class CBase {
public:
    static const GlobalVar<CRuntimeClass, 0x00add8c4> classCBase;

public:
    virtual CRuntimeClass const *GetRuntimeClass() const;

    virtual ~CBase();

public:
    static CBase *__stdcall CreateObject();

    static void __cdecl DeleteObject(CBase *pObj);

};

struct CRuntimeClass {
public:
    char *m_lpszClassName; //0x0000
    int m_nObjectSize; //0x0004
    CBase *(__stdcall *m_pfnCreateObject)(); //0x0008 NULL => abstract class
    void *(__cdecl *m_pfnDeleteObject)(CBase *); //0x000C NULL => abstract class
    CRuntimeClass *m_pBaseClass; //0x0010
};

#define _RUNTIME_CLASS(class_name) (class_name::class##class_name)
#define RUNTIME_CLASS(class_name) _RUNTIME_CLASS(class_name)

#define DECLARE_DYNCREATE_FN(class_name) \
    static CBase* PASCAL CreateObject(); \
    static void __cdecl DeleteObject(CBase*);

#define DECLARE_DYNCREATE(class_name) \
    DECLARE_DYNAMIC(class_name) \
    DECLARE_DYNCREATE_FN(class_name)

#define DECLARE_DYNAMIC(class_name) \
public: \
    static const CRuntimeClass class##class_name; \
    virtual CRuntimeClass const* GetRuntimeClass() const;

#define IMPLEMENT_DYNAMIC(class_name, base_class_name) \
    IMPLEMENT_RUNTIMECLASS(class_name, NULL, NULL, base_class_name)

#define IMPLEMENT_DYNCREATE_FN(class_name) \
        CBase* PASCAL class_name::CreateObject() \
        { return new class_name; } \
        void __cdecl class_name::DeleteObject(CBase* pObj) \
        { delete pObj; }

#define IMPLEMENT_DYNCREATE(class_name, base_class_name) \
        IMPLEMENT_DYNCREATE_FN(class_name, base_class_name) \
        IMPLEMENT_RUNTIMECLASS(class_name, class_name::CreateObject, class_name::DeleteObject, base_class_name)

#define IMPLEMENT_RUNTIMECLASS(class_name, pfnNew, pfnDelete, base_class_name) \
    const CRuntimeClass class_name::class##class_name = { \
    #class_name, sizeof(class class_name), \
    pfnNew, pfnDelete, base_class_name::class##base_class_name}; \
    CRuntimeClass const* class_name::GetRuntimeClass() const \
    { return &RUNTIME_CLASS(class_name); }

// Existing

#define DECLARE_DYNAMIC_EXISTING(class_name, offset) \
public: \
    static const GlobalVar<CRuntimeClass, offset> class##class_name; \
    virtual CRuntimeClass const* GetRuntimeClass() const;

#define IMPLEMENT_DYNAMIC_EXISTING(class_name, offset) \
    IMPLEMENT_RUNTIMECLASS_EXISTING(class_name, offset)

#define IMPLEMENT_RUNTIMECLASS_EXISTING(class_name, offset) \
    const GlobalVar<CRuntimeClass, offset> class_name::class##class_name; \
    CRuntimeClass const* class_name::GetRuntimeClass() const \
    { return RUNTIME_CLASS(class_name); }
