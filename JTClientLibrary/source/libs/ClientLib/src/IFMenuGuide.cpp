#include "IFMenuGuide.h"

#include "GEffSoundBody.h"
#include "GInterface.h"
#include "Menu/IFMenu.h"
#include "IFMagicStateBoard.h"
#include "ICPlayer.h"
#include "ICMonster.h"
#include "EntityManagerClient.h"
#include "CharacterDependentData.h"

#include <BSLib/Debug.h>
#include <CustomData/CustomCICPlayer.h>
#include <CustomData/CustomSettingManager.h>
#include <CustomData/CustomDataManager.h>
#include <CustomInterface/IFFortressWar.h>
#include <WebViewer/WebViewerConfig.h>

GFX_IMPLEMENT_DYNCREATE(CIFMenuGuide, CIFDecoratedStatic)

bool CIFMenuGuide::OnCreate(long ln) {
    BS_DEBUG_LOW("> " __FUNCTION__ "(%d)", ln);
    CIFDecoratedStatic::OnCreate(ln);

    TB_Func_13("clientlibrary\\icon\\kmt_menu_1.ddj", 0, 0);
    sub_634470("clientlibrary\\icon\\kmt_menu_2.ddj");




    set_N00009BD4(2);
    set_N00009BD3(500);

   //CGEffSoundBody::get()->PlaySound(L"snd_quest");

    m_IRM.LoadFromFile("clientlibrary\\resinfo\\ifsimple.txt");
    m_IRM.CreateInterfaceSection("Create", this);

    RECT rect = { 0,0,40,40 };
    namelabel = this->m_IRM.GetResObj<CIFStatic>(1, 0);
    std::n_wstring msg(KmtGetText(L"UIIT_KMT_MENU"));

    namelabel->SetTooltip(msg);
    namelabel->SetStyleThingy(TOOLTIP);

    if(m_Settings->EnableServerInfoSkill)
    {
        wnd_rect sz;
        sz.pos.x = 0;
        sz.pos.y = 0;
        sz.size.width = 20;
        sz.size.height = 20;

        CIFMagicStateBoard *winder = g_pCGInterface->m_IRM.GetResObj<CIFMagicStateBoard>(22, 1);

        CIFStateSlot* ServerInfoSlot = (CIFStateSlot*)CGWnd::CreateInstance(winder, GFX_RUNTIME_CLASS(CIFStatic), sz, 999, 0);

        ServerInfoSlot->TB_Func_13("clientlibrary\\guides\\server_max_level.ddj", 0, 0);


        wchar_t NoticeStr2[1000];
        swprintf_s(NoticeStr2, KmtGetText(L"UIIT_KMT_MAX_LEVEL_OF_THE_CURRENT_SERVER_IS_VALUE"), m_Settings->ServerMaxLevel);

        ServerInfoSlot->SetTooltip(NoticeStr2);
        ServerInfoSlot->SetStyleThingy(TOOLTIP);
    }

    return true;
}

int CIFMenuGuide::OnMouseLeftUp(int a1, int x, int y) {
    BS_DEBUG_LOW("> " __FUNCTION__ "(%d, %d, %d)", a1, x, y);

    if(g_pCGInterface->m_IRM.GetResObj<CIFMenu>(MainMenuID, 1)->IsVisible())
    {
        g_pCGInterface->m_IRM.GetResObj<CIFMenu>(MainMenuID, 1)->ShowGWnd(false);
        CGEffSoundBody::get()->PlaySound(L"snd_window_close");
    }
    else
    {
        CGEffSoundBody::get()->PlaySound(L"snd_window_open");
        g_pCGInterface->m_IRM.GetResObj<CIFMenu>(MainMenuID, 1)->ShowGWnd(true);
        g_pCGInterface->m_IRM.GetResObj<CIFMenu>(MainMenuID, 1)->UpdateMenuSize();
    }


    return 0;
}
#define STRTIMERR 110077
#define INTTIMERR 110088

void CIFMenuGuide::OnTimer(int timerId) {
    if (timerId == STRTIMERR) {
        this->KillTimer(STRTIMERR);
        g_pCGInterface->RequestStatIncrement(0x4b, 1, 0, 1);
    }
    if (timerId == INTTIMERR) {
        this->KillTimer(INTTIMERR);
        g_pCGInterface->RequestStatIncrement(0x4b, 2, 0, 1);
    }
}



void CIFMenuGuide::OnCIFReady() {
    BS_DEBUG_LOW("> " __FUNCTION__);

    CIFDecoratedStatic::OnCIFReady();
    sub_633990();
}
void CIFMenuGuide::GiveStr(int timeoutSeconds) {
    this->StartTimer(STRTIMERR, timeoutSeconds);
}
void CIFMenuGuide::GiveInt(int timeoutSeconds) {
    this->StartTimer(INTTIMERR, timeoutSeconds);
}
void CIFMenuGuide::OnUpdate() {
    static bool s_webViewerGuidesCreated = false;
    static int s_webViewerRetryFrame = 0;

    if(!m_Settings->ShowGuideWebViewer) {
        int i;
        for(i = 0; i < WEBVIEWER_MAX_BUTTONS; ++i)
            g_pCGInterface->GetAlarmManager()->RemoveGuide(WEBVIEWER_GUIDE_BASE_ID + i);
        s_webViewerGuidesCreated = true;
    } else if(!s_webViewerGuidesCreated && s_webViewerRetryFrame < 1800) {
        ++s_webViewerRetryFrame;
        if((s_webViewerRetryFrame % 60) == 1) {
            if(CWebViewerConfig::Load()) {
                int count = CWebViewerConfig::GetCount();
                int i;
                for(i = 0; i < count; ++i) {
                    int guideId = WEBVIEWER_GUIDE_BASE_ID + i;
                    if(!g_pCGInterface->GetAlarmManager()->IsAvailableGuide(guideId))
                        g_pCGInterface->GetAlarmManager()->CreateGuideIcon(guideId);
                }
                g_pCGInterface->GetAlarmManager()->PrepareGuides();
                s_webViewerGuidesCreated = true;
            }
        }
    }

    g_pCGInterface->GetAlarmManager()->PrepareGuides();
    if(m_Player->Enabled_AutoStr)
    {
        GiveStr(100);
    }
    else if(m_Player->Enabled_AutoInt)
    {
        GiveInt(100);
    }
    if(g_pMyPlayerObj != NULL)
    {
        if(m_CustomDataManager->m_EventMapSettings.find(g_pMyPlayerObj->GetRegion().r) != m_CustomDataManager->m_EventMapSettings.end())
        {
            if(m_CustomDataManager->m_EventMapSettings[g_pMyPlayerObj->GetRegion().r].HideMiniMap)
            {
                CIFWnd * minimap = g_pCGInterface->m_IRM.GetResObj(10, 1);
                if(minimap != NULL)
                {
                    if(minimap->IsVisible())
                    {
                        minimap->ShowGWnd(false);
                    }
                }
            }
             else
            {
                CIFWnd * minimap = g_pCGInterface->m_IRM.GetResObj(10, 1);
                if(minimap != NULL)
                {
                    if(!minimap->IsVisible())
                    {
                        minimap->ShowGWnd(true);
                    }
                }
            }
        }
        else
        {
            CIFWnd * minimap = g_pCGInterface->m_IRM.GetResObj(10, 1);
            if(minimap != NULL)
            {
                if(!minimap->IsVisible())
                {
                    minimap->ShowGWnd(true);
                }
            }
        }
    }

    if(g_pCGInterface->m_IRM.GetResObj<CIFMenu>(MainMenuID, 1) != NULL)
    {
        CIFMenu* menu = g_pCGInterface->m_IRM.GetResObj<CIFMenu>(MainMenuID, 1);
        const SPartyData& partyData = g_CCharacterDependentData.GetPartyData();
        if(!partyData.bInParty)
        {
            if(menu->PingedRegionID != 0)
            {
                menu->PingedRegionID = 0;
                menu->PingedPosX = 0;
                menu->PingedPosY = 0;
                menu->PingedPosZ = 0;

                if(g_pCGInterface->GetAlarmManager()->GetGuide(GDR_WEB_GUIDE) != NULL)
                {
                    g_pCGInterface->GetAlarmManager()->RemoveGuide((GDR_WEB_GUIDE));
                }
            }
        }
    }


    /*   if(g_pMyPlayerObj != NULL)
       {
           if(g_pMyPlayerObj->GetWorldID() >= 2 && g_pMyPlayerObj->GetWorldID() <= 9)
           {
               if(!g_pCGInterface->m_IRM.GetResObj<CIFFortressWar>(FtwKillCounter, 1)->IsVisible())
               {
                   g_pCGInterface->m_IRM.GetResObj<CIFFortressWar>(FtwKillCounter, 1)->ShowGWnd(true);
               }
           }
           else
           {
               if(g_pCGInterface->m_IRM.GetResObj<CIFFortressWar>(FtwKillCounter, 1)->IsVisible())
               {
                   g_pCGInterface->m_IRM.GetResObj<CIFFortressWar>(FtwKillCounter, 1)->ShowGWnd(false);

               }
           }

       }

       if(g_pMyPlayerObj != NULL)
       {
           std::map<int, int>::iterator iter = m_CustomDataManager->UniqueTargetHashmap.begin();
           while (iter != m_CustomDataManager->UniqueTargetHashmap.end()) {
               bool mobFound = false;
               for (std::map<int, CIObject*>::iterator it = g_pGfxEttManager->entities.begin(); it != g_pGfxEttManager->entities.end(); ++it) {
                   if (it->second->IsSame(GFX_RUNTIME_CLASS(CICMonster))) {
                       CICMonster* nearmonster = (CICMonster*)it->second;
                       if (nearmonster) {
                           if (nearmonster->GetCommonData() != NULL) {
                               if (nearmonster->GetCommonData()->Rarity == 3 || nearmonster->GetCommonData()->Rarity == 8) {
                                   if (nearmonster->GetUniqueId() == iter->first) {
                                       mobFound = true;
                                       break;
                                   }
                               }
                           }
                       }
                   }
               }
               if (!mobFound) {
                   CICPlayer *pUser = g_pMyPlayerObj->GetCICPlayerByUniqueID(iter->second); // Doğru arama
                   if (pUser != NULL) { // pUser geçerli mi kontrol et
                       DWORD effect = g_CGlobalDataManager->GetEffectIdByName(L"SYSTEM_AGGRO_EFFECT");
                       if (effect) {
                           if (pUser->GetObjEffect() == effect) {
                               pUser->RemoveEffect(effect);
                           }
                       }
                   }
                   iter = m_CustomDataManager->UniqueTargetHashmap.erase(iter); // Mob id bulunamazsa hashmap'ten sil
               } else {
                   ++iter; // Mob id bulunduysa bir sonraki elemana geç
               }
           }


       }*/
}

