#include <ClientNet\MsgStreamBuffer.h>
#include "IFAchievementsSlot.h"
#include "IFAchievements.h"
#include <GInterface.h>
#include <GlobalDataManager.h>
#include <TextStringManager.h>
#include <sstream>
#include <CustomData/CustomDataManager.h>
#include <sstream> // Gerekli kütüphane
#include <CustomData/CustomCICPlayer.h>



GFX_IMPLEMENT_DYNCREATE(CIFAchievementsSlot, CIFWnd)
GFX_BEGIN_MESSAGE_MAP(CIFAchievementsSlot, CIFWnd)
GFX_END_MESSAGE_MAP()

CIFAchievementsSlot::CIFAchievementsSlot(void){
    Description = L"";
}
CIFAchievementsSlot::~CIFAchievementsSlot(void){

}


bool CIFAchievementsSlot::OnCreate(long ln)
{
    // Populate inherited members
    CIFWnd::OnCreate(ln);
    m_IRM.LoadFromFile("clientlibrary\\resinfo\\ifachievementsslot.txt");
    m_IRM.CreateInterfaceSection("Create", this);

    if (!m_IRM.GetResObj<CIFBarWnd>(3, 1) ||
        !m_IRM.GetResObj<CIFStatic>(4, 1) ||
        !m_IRM.GetResObj<CIFStatic>(5, 1) ||
        !m_IRM.GetResObj<CIFBarWnd>(6, 1) ||
        !m_IRM.GetResObj<CIFStatic>(7, 1) ||
        !m_IRM.GetResObj<CIFBarWnd>(8, 1) ||
        !m_IRM.GetResObj<CIFStatic>(9, 1) ||
        !m_IRM.GetResObj<CIFStatic>(10, 1))
        return false;

    const int decorativeIds[] = {3, 4, 5, 6, 7, 8, 9, 10};
    for (int i = 0; i < sizeof(decorativeIds) / sizeof(decorativeIds[0]); ++i) {
        CIFWnd* decorative = m_IRM.GetResObj<CIFWnd>(decorativeIds[i], 1);
        if (decorative)
            decorative->SetClickable(false);
    }
    m_IRM.GetResObj(4, 1)->BringToFront();
    m_IRM.GetResObj(5, 1)->BringToFront();
    m_IRM.GetResObj(9, 1)->BringToFront();
    m_IRM.GetResObj(10, 1)->BringToFront();

    Description = L"";
    DatabaseID = 0;
    this->ShowGWnd(false);
    return true;
}

int CIFAchievementsSlot::OnMouseLeftUp(int a1, int x, int y) {

    CIFAchievements* achievements = g_pCGInterface
        ? g_pCGInterface->m_IRM.GetResObj<CIFAchievements>(AchievementsID, 1)
        : 0;
    if (!achievements)
        return false;

    achievements->ClearDDJ();
    if(DatabaseID != 0)
    {
        achievements->SetDescBoxText(Description);
        if(m_Player->m_Achievements.find(DatabaseID) != m_Player->m_Achievements.end())
        {
            if(m_Player->m_Achievements[DatabaseID] == 1)
            {
                achievements->SelectedItemID = DatabaseID;
                achievements->SetUseButtonState(true);
            }
            else
            {
                achievements->SetUseButtonState(false);
            }
        }

        CIFBarWnd* row = this->m_IRM.GetResObj<CIFBarWnd>(3, 1);
        if (row)
            row->TB_Func_13("interface\\ifcommon\\com_bar02_", 1, 1);
        CIFStatic* name = this->m_IRM.GetResObj<CIFStatic>(5, 1);
        if (name)
            name->m_FontTexture.SetColor(D3DCOLOR_ARGB(255, 255, 244, 205));
    }


    return true;
}
void CIFAchievementsSlot::ClearDDJ()
{
    CIFBarWnd* row = m_IRM.GetResObj<CIFBarWnd>(3, 1);
    if (row)
        row->TB_Func_13("interface\\ifcommon\\com_bar01_", 0, 0);
    CIFStatic* name = m_IRM.GetResObj<CIFStatic>(5, 1);
    if (name)
        name->m_FontTexture.SetColor(D3DCOLOR_ARGB(255, 238, 215, 168));
}

void CIFAchievementsSlot::OnUpdate() {

}
void CIFAchievementsSlot::WriteLine(int DbID, byte Category, std::wstring Name){
    DatabaseID = DbID;
    UpdateDefaultIcons(Category, DbID);
    CIFStatic* name = this->m_IRM.GetResObj<CIFStatic>(5, 1);
    if (name)
        name->SetText(Name.c_str());
    UpdateReward(DbID);
    UpdateProgress(DbID);
    UpdateDefaultDescs(DbID);
}

void CIFAchievementsSlot::UpdateReward(int AchievementID)
{
    CIFStatic* reward = m_IRM.GetResObj<CIFStatic>(10, 1);
    if (!reward)
        return;

    std::map<int, CustomDataManager::Achievements>::iterator achievement =
        m_CustomDataManager->m_RefAchievement.find(AchievementID);
    if (achievement == m_CustomDataManager->m_RefAchievement.end()) {
        reward->SetText(L"");
        return;
    }

    std::wstringstream value;
    value << KmtGetText(L"UIIT_KMT_REWARD") << L": ";
    if (achievement->second.RewardType == 0)
        value << achievement->second.RewardTagName.c_str();
    else if (achievement->second.RewardType == 1)
        value << achievement->second.RewardSkillPoint;
    else if (achievement->second.RewardType == 2)
        value << Insert(achievement->second.RewardGold);

    reward->SetText(value.str().c_str());
    reward->BringToFront();
}

void CIFAchievementsSlot::UpdateProgress(int AchievementID)
{
    __int64 progress = 0;
    __int64 total = 0;

    for (size_t i = 0; i < m_CustomDataManager->m_RefAchievementCondition.size(); ++i) {
        const CustomDataManager::SRefAchievementCondition& condition =
            m_CustomDataManager->m_RefAchievementCondition[i];
        if (condition.RefAchievementID != AchievementID || condition.CompleteCount <= 0)
            continue;

        total += condition.CompleteCount;
        std::map<int, CustomCICPlayer::SAchievementsCondition>::iterator current =
            m_Player->m_AchievementsCondition.find(condition.ID);
        if (current == m_Player->m_AchievementsCondition.end() ||
            current->second.RefAchievementID != AchievementID)
            continue;

        __int64 value = current->second.ProgressCount;
        if (value < 0)
            value = 0;
        if (value > condition.CompleteCount)
            value = condition.CompleteCount;
        progress += value;
    }

    std::map<int, byte>::iterator state = m_Player->m_Achievements.find(AchievementID);
    const bool completed = state != m_Player->m_Achievements.end() && state->second != 0;
    if (total <= 0) {
        total = 1;
        progress = completed ? 1 : 0;
    } else if (completed) {
        progress = total;
    }

    int percent = static_cast<int>((progress * 100) / total);
    if (percent < 0)
        percent = 0;
    if (percent > 100)
        percent = 100;

    CIFBarWnd* fill = m_IRM.GetResObj<CIFBarWnd>(8, 1);
    CIFStatic* text = m_IRM.GetResObj<CIFStatic>(9, 1);
    if (!fill || !text)
        return;

    if (percent > 0) {
        int width = (356 * percent) / 100;
        if (width < 1)
            width = 1;
        fill->SetGWndSize(width, 12);
        fill->ShowGWnd(true);
        fill->BringToFront();
    } else {
        fill->SetGWndSize(1, 12);
        fill->ShowGWnd(false);
    }

    std::wstringstream value;
    value << Insert(progress) << L" / " << Insert(total) << L" (" << percent << L"%)";
    text->SetText(value.str().c_str());
    text->BringToFront();
}


void CIFAchievementsSlot::UpdateDefaultDescs(int AchievementID)
{
    std::map<int, CustomDataManager::Achievements>::iterator achievement =
        m_CustomDataManager->m_RefAchievement.find(AchievementID);
    if (achievement == m_CustomDataManager->m_RefAchievement.end()) {
        Description = L"Achievement details are unavailable.";
        return;
    }

    std::vector<int> foundedIndexes;

    // Eşleşen indeksleri bul
    for (size_t i = 0; i < m_CustomDataManager->m_RefAchievementCondition.size(); ++i)
    {
        if (m_CustomDataManager->m_RefAchievementCondition[i].RefAchievementID == AchievementID)
        {
            foundedIndexes.push_back(i);
        }
    }

    std::wstringstream buffer;  // Stringstream kullanarak dinamik tampon oluştur
    std::n_wstring Name = this->m_IRM.GetResObj<CIFStatic>(5, 1)->GetNText();

    // İlk satırı ekle
    buffer << L"<" << Name << L">\n\nRequirements\n";

    // Eşleşmeler varsa devam et
    if (!foundedIndexes.empty())
    {
        for (size_t j = 0; j < foundedIndexes.size(); ++j)
        {
            int foundedIndex = foundedIndexes[j];
                if(m_Player->m_AchievementsCondition.find(m_CustomDataManager->m_RefAchievementCondition[foundedIndex].ID)
                   != m_Player->m_AchievementsCondition.end())
                {
                    if(m_Player->m_AchievementsCondition[m_CustomDataManager->m_RefAchievementCondition[foundedIndex].ID].RefAchievementID == AchievementID)
                    {
                        if(m_Player->m_AchievementsCondition[m_CustomDataManager->m_RefAchievementCondition[foundedIndex].ID].ProgressCount >=
                                m_CustomDataManager->m_RefAchievementCondition[foundedIndex].CompleteCount)
                        {
                            buffer << m_CustomDataManager->m_RefAchievementCondition[foundedIndex].Name.c_str()
                                   << L" (" <<  Insert(m_Player->m_AchievementsCondition[m_CustomDataManager->m_RefAchievementCondition[foundedIndex].ID].ProgressCount)
                                   << L"/" << Insert(m_CustomDataManager->m_RefAchievementCondition[foundedIndex].CompleteCount) << L") "
                                   << L"<Finished>\n";
                        }
                        else
                        {
                            buffer << m_CustomDataManager->m_RefAchievementCondition[foundedIndex].Name.c_str()
                                   << L" (" <<  Insert(m_Player->m_AchievementsCondition[m_CustomDataManager->m_RefAchievementCondition[foundedIndex].ID].ProgressCount)
                                   << L"/" << Insert(m_CustomDataManager->m_RefAchievementCondition[foundedIndex].CompleteCount) << L") "
                                   << L"<Not Finished>\n";
                        }

                    }
                    else
                    {
                        buffer << m_CustomDataManager->m_RefAchievementCondition[foundedIndex].Name.c_str()
                               << L" (" <<  0 << L"/" << Insert(m_CustomDataManager->m_RefAchievementCondition[foundedIndex].CompleteCount) << L") "
                               << L"<Not Finished>\n";
                    }

                }
                else
                {
                    buffer << m_CustomDataManager->m_RefAchievementCondition[foundedIndex].Name.c_str()
                           << L" (" << 0 << L"/" << Insert(m_CustomDataManager->m_RefAchievementCondition[foundedIndex].CompleteCount) << L") "
                           << L"<Not Finished>\n";
                }
        }
    }
    std::map<int, byte>::iterator state = m_Player->m_Achievements.find(AchievementID);
    if (state != m_Player->m_Achievements.end() && state->second != 0)
        buffer << L"\nStatus\n<Completed>\n";
    else
        buffer << L"\nStatus\n<In Progress>\n";

    if(achievement->second.RewardType == 0)
    {
        buffer << L"\nRewards"
               << L"\nTitle : "<< L"<"
                  << achievement->second.RewardTagName.c_str() << L">";
    }
    else if(achievement->second.RewardType == 1)
    {
        buffer << L"\nRewards\n"
               << L"\nSkill Point : "<< L"<"
               << achievement->second.RewardSkillPoint << L">";
    }
    else if(achievement->second.RewardType == 2)
    {
        buffer << L"\nRewards\n"
               << L"\nGold : "<< L"<"
               << achievement->second.RewardGold << L">";
    }

    Description = buffer.str().c_str();  // Sonuç olarak buffer'ı al
}

void CIFAchievementsSlot::UpdateDefaultIcons(byte CateID, int AchievementID)
{    // 2 unique

    if(CateID == 0)
    {
        this->m_IRM.GetResObj<CIFStatic>(4, 1)->TB_Func_13("clientlibrary\\title\\title_icon_mini_all_gray.ddj", 0, 0);
    }
    else if(CateID == 1)
    {
        this->m_IRM.GetResObj<CIFStatic>(4, 1)->TB_Func_13("clientlibrary\\title\\title_icon_mini_quest_gray.ddj", 0, 0);
    }
    else if(CateID == 2)
    {
        this->m_IRM.GetResObj<CIFStatic>(4, 1)->TB_Func_13("clientlibrary\\title\\title_icon_mini_unique_gray.ddj", 0, 0);
    }
    else if(CateID == 3)
    {
        this->m_IRM.GetResObj<CIFStatic>(4, 1)->TB_Func_13("clientlibrary\\title\\title_icon_mini_match_gray.ddj", 0, 0);
    }
    else if(CateID == 4)
    {
        this->m_IRM.GetResObj<CIFStatic>(4, 1)->TB_Func_13("clientlibrary\\title\\title_icon_mini_event_gray.ddj", 0, 0);
    }


    if(CateID == 0)
    {
        if(m_Player->m_Achievements.find(AchievementID) != m_Player->m_Achievements.end())
        {
            if(m_Player->m_Achievements[AchievementID] == 0)
            {
                this->m_IRM.GetResObj<CIFStatic>(4, 1)->TB_Func_13("clientlibrary\\title\\title_icon_mini_all_gray.ddj", 0, 0);
            }
            else if(m_Player->m_Achievements[AchievementID] == 1)
            {
                this->m_IRM.GetResObj<CIFStatic>(4, 1)->TB_Func_13("clientlibrary\\title\\title_icon_mini_all.ddj", 0, 0);
            }
            else if(m_Player->m_Achievements[AchievementID] == 2)
            {
                this->m_IRM.GetResObj<CIFStatic>(4, 1)->TB_Func_13("clientlibrary\\title\\title_icon_mini_all.ddj", 0, 0);
            }

        }
        else
        {
            this->m_IRM.GetResObj<CIFStatic>(4, 1)->TB_Func_13("clientlibrary\\title\\title_icon_mini_all_gray.ddj", 0, 0);
        }
    }
    else if(CateID == 1)
    {
        if(m_Player->m_Achievements.find(AchievementID) != m_Player->m_Achievements.end())
        {
            if(m_Player->m_Achievements[AchievementID] == 0)
            {
                this->m_IRM.GetResObj<CIFStatic>(4, 1)->TB_Func_13("clientlibrary\\title\\title_icon_mini_quest_gray.ddj", 0, 0);
            }
            else if(m_Player->m_Achievements[AchievementID] == 1)
            {
                this->m_IRM.GetResObj<CIFStatic>(4, 1)->TB_Func_13("clientlibrary\\title\\title_icon_mini_quest.ddj", 0, 0);
            }
            else if(m_Player->m_Achievements[AchievementID] == 2)
            {
                this->m_IRM.GetResObj<CIFStatic>(4, 1)->TB_Func_13("clientlibrary\\title\\title_icon_mini_quest.ddj", 0, 0);
            }

        }
        else
        {
            this->m_IRM.GetResObj<CIFStatic>(4, 1)->TB_Func_13("clientlibrary\\title\\title_icon_mini_quest_gray.ddj", 0, 0);
        }
    }
    else if(CateID == 2)
    {
        if(m_Player->m_Achievements.find(AchievementID) != m_Player->m_Achievements.end())
        {
            if(m_Player->m_Achievements[AchievementID] == 0)
            {
                this->m_IRM.GetResObj<CIFStatic>(4, 1)->TB_Func_13("clientlibrary\\title\\title_icon_mini_unique_gray.ddj", 0, 0);
            }
            else if(m_Player->m_Achievements[AchievementID] == 1)
            {
                this->m_IRM.GetResObj<CIFStatic>(4, 1)->TB_Func_13("clientlibrary\\title\\title_icon_mini_unique.ddj", 0, 0);
            }
            else if(m_Player->m_Achievements[AchievementID] == 2)
            {
                this->m_IRM.GetResObj<CIFStatic>(4, 1)->TB_Func_13("clientlibrary\\title\\title_icon_mini_unique.ddj", 0, 0);
            }

        }
        else
        {
            this->m_IRM.GetResObj<CIFStatic>(4, 1)->TB_Func_13("clientlibrary\\title\\title_icon_mini_unique_gray.ddj", 0, 0);
        }
    }
    else if(CateID == 3)
    {
        if(m_Player->m_Achievements.find(AchievementID) != m_Player->m_Achievements.end())
        {
            if(m_Player->m_Achievements[AchievementID] == 0)
            {
                this->m_IRM.GetResObj<CIFStatic>(4, 1)->TB_Func_13("clientlibrary\\title\\title_icon_mini_match_gray.ddj", 0, 0);
            }
            else if(m_Player->m_Achievements[AchievementID] == 1)
            {
                this->m_IRM.GetResObj<CIFStatic>(4, 1)->TB_Func_13("clientlibrary\\title\\title_icon_mini_match.ddj", 0, 0);
            }
            else if(m_Player->m_Achievements[AchievementID] == 2)
            {
                this->m_IRM.GetResObj<CIFStatic>(4, 1)->TB_Func_13("clientlibrary\\title\\title_icon_mini_match.ddj", 0, 0);
            }

        }
        else
        {
            this->m_IRM.GetResObj<CIFStatic>(4, 1)->TB_Func_13("clientlibrary\\title\\title_icon_mini_match_gray.ddj", 0, 0);
        }
    }
    else if(CateID == 4)
    {
        if(m_Player->m_Achievements.find(AchievementID) != m_Player->m_Achievements.end())
        {
            if(m_Player->m_Achievements[AchievementID] == 0)
            {
                this->m_IRM.GetResObj<CIFStatic>(4, 1)->TB_Func_13("clientlibrary\\title\\title_icon_mini_event_gray.ddj", 0, 0);
            }
            else if(m_Player->m_Achievements[AchievementID] == 1)
            {
                this->m_IRM.GetResObj<CIFStatic>(4, 1)->TB_Func_13("clientlibrary\\title\\title_icon_mini_event.ddj", 0, 0);
            }
            else if(m_Player->m_Achievements[AchievementID] == 2)
            {
                this->m_IRM.GetResObj<CIFStatic>(4, 1)->TB_Func_13("clientlibrary\\title\\title_icon_mini_event.ddj", 0, 0);
            }

        }
        else
        {
            this->m_IRM.GetResObj<CIFStatic>(4, 1)->TB_Func_13("clientlibrary\\title\\title_icon_mini_event_gray.ddj", 0, 0);
        }
    }
}

