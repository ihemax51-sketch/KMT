#include "IFTargetWindowSpecialMob.h"
#include "CustomInterface/IFDropLogWnd.h"
#include "GInterface.h"
#include "GEffSoundBody.h"
#include "IFStatic.h"
#include "IFGauge.h"
#include "unsorted.h"

GFX_IMPLEMENT_DYNAMIC_EXISTING(CIFTargetWindowSpecialMob, 0x00eea5fc)

GFX_IMPLEMENT_DYNCREATE_FN(CIFTargetWindowSpecialMob, CIFWnd)

enum {
    GDR_TWSM_GEM = 10, // CIFStatic
    GDR_TWSM_LEVEL = 4, // CIFStatic
    GDR_TWSM_ICON = 3, // CIFStatic
    GDR_TWSM_TEXT_LEV = 2, // CIFStatic
    GDR_TWSM_GAUGE_HPGAUGE = 1, // CIFGauge
    GDR_TWSM_TEXT_ID = 0, // CIFStatic
};

namespace {

const DWORD HP_PERCENT_TEXT_COLOR = 0xFFFFD853;
const DWORD HP_PERCENT_LOW_TEXT_COLOR = 0xFFFF5555;

std::n_wstring StripHpPercentSuffix(const wchar_t* text)
{
    std::n_wstring value = text ? text : L"";
    size_t marker = value.rfind(L" [");
    if (marker != std::n_wstring::npos)
        value = value.substr(0, marker);
    return value;
}

int GetHpPercentFromTarget(int objectId, CIFGauge* gauge)
{
    CICharactor* target = GetCharacterObjectByID_MAYBE(objectId);
    if (target != NULL && target->GetMaxHp() > 0) {
        unsigned int currentHp = target->GetCurrentHp();
        unsigned int maxHp = target->GetMaxHp();
        if (currentHp > maxHp)
            currentHp = maxHp;

        unsigned int percent = static_cast<unsigned int>(((unsigned __int64)currentHp * 100 + (maxHp / 2)) / maxHp);
        return percent > 100 ? 100 : static_cast<int>(percent);
    }

    if (gauge == NULL)
        return 0;

    float value = gauge->m_valueFg;
    if (value < 0.0f)
        value = 0.0f;
    if (value > 1.0f)
        value = 1.0f;

    return static_cast<int>(value * 100.0f + 0.5f);
}

void UpdateHpPercentTextWithPercent(CIFWnd* wnd, CIFStatic* nameText, int percent)
{
    if (wnd == NULL || nameText == NULL || !wnd->IsVisible())
        return;

    if (percent < 0)
        percent = 0;
    if (percent > 100)
        percent = 100;

    std::n_wstring name = StripHpPercentSuffix(nameText->GetText());
    if (name.empty())
        name = KmtGetText(L"UIIT_KMT_TARGET");

    wchar_t buffer[256];
    swprintf_s(
        buffer,
        256,
        KmtGetText(L"UIIT_KMT_TEXT_VALUE_PERCENT"),
        name.c_str(),
        percent);

    if (wcscmp(nameText->GetText(), buffer) != 0)
        nameText->SetText(buffer);

    nameText->m_FontTexture.SetColor(percent <= 25 ? HP_PERCENT_LOW_TEXT_COLOR : HP_PERCENT_TEXT_COLOR);
}

void UpdateHpPercentText(CIFWnd* wnd, CIFGauge* gauge, CIFStatic* nameText, int objectId)
{
    UpdateHpPercentTextWithPercent(wnd, nameText, GetHpPercentFromTarget(objectId, gauge));
}

int GetHpPercentFromPacketHp(int objectId, unsigned int currentHp, CIFGauge* gauge)
{
    CICharactor* target = GetCharacterObjectByID_MAYBE(objectId);
    if (target != NULL && target->GetMaxHp() > 0) {
        unsigned int maxHp = target->GetMaxHp();
        if (currentHp > maxHp)
            currentHp = maxHp;

        unsigned int percent = static_cast<unsigned int>(((unsigned __int64)currentHp * 100 + (maxHp / 2)) / maxHp);
        return percent > 100 ? 100 : static_cast<int>(percent);
    }

    return GetHpPercentFromTarget(objectId, gauge);
}

}

GFX_BEGIN_MESSAGE_MAP(CIFTargetWindowSpecialMob, CIFWnd)

GFX_END_MESSAGE_MAP()

bool CIFTargetWindowSpecialMob::OnCreate(long ln) {
    //printf("%s\n", __FUNCTION__);
    //return reinterpret_cast<bool (__thiscall *)(const CIFTargetWindowSpecialMob *, long)>(0x0069b410)(this, ln);

    m_IRM.LoadFromFile("resinfo\\iftw_specialmob.txt");
    m_IRM.CreateInterfaceSection("Create", this);

    m_pGDR_TWSM_TEXT_LEV = m_IRM.GetResObj<CIFStatic>(GDR_TWSM_TEXT_ID, 1); // 0x370
    m_pGDR_TWSM_GAUGE_HPGAUGE = m_IRM.GetResObj<CIFGauge>(GDR_TWSM_GAUGE_HPGAUGE, 1); // 0x374
    m_pGDR_TWSM_TEXT_ID = m_IRM.GetResObj<CIFStatic>(GDR_TWSM_TEXT_LEV, 1); // 0x378

    m_pGDR_TWSM_GAUGE_HPGAUGE->field_0x38c = 0.1;
    m_pGDR_TWSM_GAUGE_HPGAUGE->SetGWndSize(208, 4);

    return true;
}

void CIFTargetWindowSpecialMob::OnUpdate() {
    //printf("%s\n", __FUNCTION__);
    reinterpret_cast<void (__thiscall *)(const CIFTargetWindowSpecialMob *)>(0x0069b240)(this);
}

void CIFTargetWindowSpecialMob::SetTargetObject(int objectId) {
    reinterpret_cast<void (__thiscall *)(CIFTargetWindowSpecialMob *, int)>(0x0069b4c0)(this, objectId);
    RefreshHpPercentText();
}

void CIFTargetWindowSpecialMob::SetTargetName(std::n_wstring* name, int unk) {
    reinterpret_cast<void (__thiscall *)(CIFTargetWindowSpecialMob *, std::n_wstring*, int)>(0x0069b380)(this, name, unk);
    RefreshHpPercentText();
}

void CIFTargetWindowSpecialMob::RefreshHpPercentText() {
    UpdateHpPercentText(this, m_pGDR_TWSM_GAUGE_HPGAUGE, m_pGDR_TWSM_TEXT_LEV, m_objectId);
}

bool CIFTargetWindowSpecialMob::RefreshHpPercentTextForObjectHp(unsigned int objectId, unsigned int currentHp) {
    if (m_objectId != static_cast<int>(objectId))
        return false;

    UpdateHpPercentTextWithPercent(this, m_pGDR_TWSM_TEXT_LEV,
                                   GetHpPercentFromPacketHp(m_objectId, currentHp, m_pGDR_TWSM_GAUGE_HPGAUGE));
    return true;
}

void CIFTargetWindowSpecialMob::OnClickPossibleDrops() {
    if (!g_pCGInterface) {
        return;
    }

    CICharactor* target = GetCharacterObjectByID_MAYBE(m_objectId);
    if (target == NULL || target->GetCommonData() == NULL || target->GetCommonData()->RefObjectId <= 0) {
        g_pCGInterface->ShowMessage_Warning(KmtGetText(L"UIIT_KMT_PLEASE_SELECT_A_MONSTER_FIRST"));
        return;
    }

    CIFDropLogWnd* window = g_pCGInterface->GetGuiFromList<CIFDropLogWnd>(DROP_LOG_WINDOW_ID);
    if (window == NULL) {
        return;
    }

    std::n_wstring monsterName = StripHpPercentSuffix(
        m_pGDR_TWSM_TEXT_LEV
            ? m_pGDR_TWSM_TEXT_LEV->GetText()
            : KmtGetText(L"UIIT_KMT_MONSTER"));
    window->OpenPossibleDrops(target->GetCommonData()->RefObjectId, monsterName.c_str());
    CGEffSoundBody::get()->PlaySound(L"snd_window_open");
}
