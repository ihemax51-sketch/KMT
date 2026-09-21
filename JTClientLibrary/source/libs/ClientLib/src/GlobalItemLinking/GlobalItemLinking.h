#ifndef Q_LIBRARY_GLOBAL_ITEM_LINKING_H
#define Q_LIBRARY_GLOBAL_ITEM_LINKING_H

#include <IFEdit.h>
#include "SOItem.h"

struct GlobalChat {
    int index;
    bool completed;
    std::n_wstring Message;
    std::list<BYTE> listBytes;
};
struct AlchemyLogStr {
    int index;
    bool completed;
    std::n_wstring Message;
    BYTE itemtimestatus;
    INT64 itemtime;
    INT64 itemtime2;
    int CosIDForPet;
    std::n_wstring petname;
    std::list<BYTE> listBytes;
};

class GlobalItemLinking {
public:
    void Initialize();
    void FUN_00636cd0(CIFEdit *pCIFEdit);
    void AppendTextToChatBox(std::wstring *text);

    std::wstring m_CurrentLinkedItemName;
    DWORD m_LinkedGlobalItemBegin;
    DWORD m_LinkedGlobalItemEnd;
    unsigned __int8 m_LinkedGlobalSlot;
    std::list<GlobalChat> GlobalChatLog;

    std::list<AlchemyLogStr> AlchemyLog;


private:
void Apply(CIFEdit* pCIFEdit);
};
#endif

extern GlobalItemLinking *g_global;