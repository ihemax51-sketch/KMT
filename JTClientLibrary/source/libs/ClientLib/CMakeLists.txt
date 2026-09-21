add_library(ClientLib STATIC ${SOURCE_FILES} src/CustomInterface/IFJobCounter.cpp src/CustomInterface/IFJobCounter.h src/MacroAlchemy/IFAlchemyMacroBlueSlot.cpp src/MacroAlchemy/IFAlchemyMacroBlueSlot.h src/MacroAlchemy/IFAlchemyMacroAttrSlot.cpp src/MacroAlchemy/IFAlchemyMacroAttrSlot.h src/PSVersionCheck.cpp src/Macro/IFMacroMenuAutoHuntPartySlot.cpp src/Macro/IFMacroMenuAutoHuntPartySlot.h src/LockItems/IFNewMsgBox.cpp src/LockItems/IFNewMsgBox.h src/LockItems/IFItemLocker.cpp src/LockItems/IFItemLocker.h src/LockItems/IFItemLockSlot.cpp src/LockItems/IFItemLockSlot.h src/LockItems/IFItemUnlocker.cpp src/LockItems/IFItemUnlocker.h src/LockItems/IFItemUnlockSlot.cpp src/LockItems/IFItemUnlockSlot.h src/CustomInterface/IFFortressWar.cpp src/CustomInterface/IFFortressWar.h src/CustomInterface/IFCustomEmojiList.cpp src/CustomInterface/IFCustomEmojiList.h src/CustomInterface/IFCustomEmojiListSlot.cpp src/CustomInterface/IFCustomEmojiListSlot.h src/IFNPCWindow.cpp src/IFNPCTalk.cpp src/ExtraUI/IFItemTranslationWnd.cpp src/ExtraUI/IFItemTranslationWnd.h src/ExtraUI/IFItemTranslationSlot.cpp src/ExtraUI/IFItemTranslationSlot.h src/SecondPW/IFSecondaryPassword.cpp src/SecondPW/IFSecondaryPassword.h src/DailyLogin/IFAttendance.cpp src/DailyLogin/IFAttendance.h src/DailyLogin/IFAttendanceDaySlot.cpp src/DailyLogin/IFAttendanceDaySlot.h src/ExtraUI/IFTargetPlayerEquip.cpp src/ExtraUI/IFTargetPlayerEquip.h src/ExtraUI/IFSettings.cpp src/ExtraUI/IFSettings.h src/ExtraUI/IFMSFPS.cpp src/ExtraUI/IFMSFPS.h src/ExtraUI/IFCounterWnd.cpp src/ExtraUI/IFCounterWnd.h src/Social/IFWebSGuide.cpp src/Social/IFWebSGuide.h)

set(KMT_DESKTOP_HUD_OBJECT
        "${CMAKE_CURRENT_SOURCE_DIR}/prebuilt/DesktopCharacterHud.vc80.obj")
set_source_files_properties(${KMT_DESKTOP_HUD_OBJECT}
        PROPERTIES EXTERNAL_OBJECT TRUE GENERATED TRUE)
target_sources(ClientLib PRIVATE ${KMT_DESKTOP_HUD_OBJECT})
target_sources(ClientLib PRIVATE
        src/ExtraUI/DesktopCharacterHudPortraitBridge.cpp)

target_sources(ClientLib PRIVATE
        src/AlramGuideMgrWnd.cpp
        src/AutoPotion.cpp
        src/CObjAnimation.cpp
        src/CameraWorking.cpp
        src/CharacterDependentData.cpp
        src/CompSimple.cpp
        src/Data/CharacterData.cpp
        src/Data/ItemData.cpp
        src/Data/LevelData.cpp
        src/GEffSoundBody.cpp
        src/GInterface.cpp
        src/GInterfaceSend.cpp
        src/GInterfaceWndRelation.cpp
        src/Game.cpp
        src/GameCfg.cpp
        src/GlobalDataManager.cpp
        src/ICCos.cpp
        src/ICMonster.cpp
        src/ICNonuser.cpp
        src/ICPlayer.cpp
        src/ICUser.cpp
        src/ICharactor.cpp
        src/IFBuffViewer.cpp
        src/IFButton.cpp
        src/IFChatModeViewer.cpp
        src/IFChatModule.cpp
        src/IFChatOptionBoard.cpp
        src/IFChatViewer.cpp
        src/IFCheckBox.cpp
        src/IFCloseButton.cpp
        src/IFConfirmReputationGuide.cpp
        src/IFConsole.cpp
        src/IFDecoratedStatic.cpp
        src/IFDragableArea.cpp
        src/IFEdit.cpp
        src/IFEventGuide.cpp
        src/IFEventGuideSecond.cpp
        src/IFExtQuickSlot.cpp
        src/IFExtQuickSlotOption.cpp
        src/IFExtQuickSlotSlot.cpp
        src/IFFade.cpp
        src/IFFrame.cpp
        src/IFInventory.cpp
        src/IFListCtrl.cpp
        src/IFMainFrame.cpp
        src/IFMainPopup.cpp
        src/IFNotify.cpp
        src/IFNotify.cpp
        src/IFOpenMarketAlramGuide.cpp
        src/IFPlayerInfo.cpp
        src/IFPlayerMiniInfo.cpp
        src/IFQuestInfoGuide.cpp
        src/IFScrollBar.cpp
        src/IFSelectableArea.cpp
        src/IFServerEventGuide.cpp
        src/IFSkill.cpp
        src/IFSlotWithHelp.cpp
        src/IFStatic.cpp
        src/IFStretchWnd.cpp
        src/IFSystemMessage.cpp
        src/IFTargetWindow.cpp
        src/IFTargetWindowCommonEnemy.cpp
        src/IFTargetWindowFortressStructure.cpp
        src/IFTargetWindowJobPlayer.cpp
        src/IFTargetWindowPlayer.cpp
        src/IFTargetWindowSpecialMob.cpp
        src/IFTextBox.cpp
        src/IFTileWnd.cpp
        src/IFVerticalScroll.cpp
        src/IFWhisperList.cpp
        src/IFWholeChat.cpp
        src/IFWnd.cpp
        src/IFMenuGuide.cpp
        src/IGIDObject.cpp
        src/IObject.cpp
        src/IRMManager.cpp
        src/Keyframe.cpp
        src/LinkedScroll.cpp
        src/LocalTime.cpp
        src/NavigationDeadreckon.cpp
        src/NetProcessIn.cpp
        src/NetProcessSecond.cpp
        src/NetProcessThird.cpp
        src/PSCharacterSelect.cpp
        src/PSVersionCheck.h
        src/PSOuterInterface.cpp
        src/PSQuickStart.cpp
        src/PSilkroad.cpp
        src/SOItem.cpp
        src/TextBoard.cpp
        src/TextStringManager.cpp
        src/UserGuildInfo.cpp
        src/World.cpp
        src/unsorted.cpp
        src/Menu/IFMenu.cpp
        src/Menu/IFMenu.h
        src/Menu/IFGrantName.cpp
        src/Menu/IFGrantName.h
        src/Menu/IFTitleManager.cpp src/Menu/IFTitleManager.h
        src/CustomData/CustomSettingManager.cpp src/CustomData/CustomSettingManager.h
        src/Menu/IFTitleManagerSlot.cpp src/Menu/IFTitleManagerSlot.h
        src/IFBarWnd.h src/IFScrollManager.cpp
        src/IDecoSkill.cpp src/IDecoSkill.h
        src/CustomData/CustomDataManager.cpp src/CustomData/CustomDataManager.h
        src/Menu/IFIconManager.cpp src/Menu/IFIconManager.h
        src/Menu/IFIconManagerSlot.cpp src/Menu/IFIconManagerSlot.h
        src/Menu/IFDynamicRanking.cpp src/Menu/IFDynamicRanking.h
        src/Menu/IFDynamicRankingSlot.cpp src/Menu/IFDynamicRankingSlot.h
        src/CustomInterface/IFPopupList.cpp src/CustomInterface/IFPopupList.h
        src/Menu/IFUniqueHistory.cpp src/Menu/IFUniqueHistory.h
        src/Menu/IFUniqueHistorySlot.cpp src/Menu/IFUniqueHistorySlot.h
        src/Menu/IFEventRegister.cpp src/Menu/IFEventRegister.h
        src/Menu/IFEventRegisterSlot.cpp
        src/Menu/IFEventRegisterSlot.h
        src/Menu/IFEventSchedule.cpp src/Menu/IFEventSchedule.h
        src/Menu/IFEventScheduleSlot.cpp src/Menu/IFEventScheduleSlot.h
        src/Menu/IFAchievements.cpp src/Menu/IFAchievements.h
        src/Menu/IFAchievementsSlot.cpp src/Menu/IFAchievementsSlot.h
        src/Menu/IFChangelog.cpp src/Menu/IFChangelog.h src/IFPML.h
        src/PSTitle.cpp src/PSTitle.h src/IFOListCtrl.h
        src/IFOListCtrl.cpp src/IFSimpleListCtrl.cpp src/IFSimpleListCtrl.h
        src/CObjCharacter.cpp src/CObjCharacter.h src/IFHelperBubbleWindow.cpp
        src/IFSkillBoard.cpp src/CustomData/CustomCICPlayer.cpp src/CustomData/CustomCICPlayer.h
        src/SkillAutomationController.cpp src/SkillAutomationController.h
        src/Data/SkillData.h src/Data/ItemMallGroupData.h src/Data/ItemMallData.h
        src/Data/SOItemPackage.cpp src/Data/SOItemPackage.h src/Data/SSkillData.h
        src/CustomInterface/IFCustomMessageBox.cpp src/CustomInterface/IFCustomMessageBox.h
        src/IFStateSlot.cpp src/IFStateSlot.h src/IFMagicStateBoard.cpp
        src/IFMagicStateBoard.h src/Data/ObjectData.h
        src/IFEquipment.cpp src/IFEquipSkillSlotWnd.h src/IFSkillSlot.h
        src/IDamageText.cpp src/IDamageText.h src/IOBillboard.h src/IOBillboard.cpp
        src/AnimateableSomething.cpp src/AnimateableSomething.h
        src/SecondBar/IFExtQuickSlotCustom.cpp
        src/SecondBar/IFExtQuickSlotCustom.h src/SecondBar/IFExtQuickSlotCustomOption.cpp
        src/SecondBar/IFExtQuickSlotCustomOption.h src/SecondBar/IFExtQuickSlotSecond.cpp
        src/SecondBar/IFExtQuickSlotSecond.h src/SecondBar/IFExtQuickSlotSecondOption.cpp
        src/SecondBar/IFExtQuickSlotSecondOption.h src/SecondBar/IFMySlot.cpp
        src/SecondBar/IFMySlot.h src/Guides/IFChestGuide.cpp
        src/Guides/IFChestGuide.h src/Guides/IFChest.cpp src/Guides/IFChest.h
        src/Guides/IFChestSlot.cpp src/Guides/IFChestSlot.h
        src/Social/IFFacebookGuide.cpp src/Social/IFFacebookGuide.h
        src/Social/IFDiscordGuide.cpp src/Social/IFDiscordGuide.h
        src/Social/IFWebSGuide.cpp src/Social/IFWebSGuide.h
	src/Social/IFAutoEquipGuide.cpp src/Social/IFAutoEquipGuide.h
        src/Social/IFSocial.cpp src/Social/IFSocial.h
        src/CustomInterface/IFDps.cpp src/CustomInterface/IFDps.h
        src/IFPartyMatch.cpp src/IFPartyMatch.h src/IFPartyMatchSlot.cpp src/IFPartyMatchSlot.h
        src/CustomInterface/IFPartyMemberViewer.cpp src/CustomInterface/IFPartyMemberViewer.h
        src/IFMessageBox.cpp src/IFMessageBox.h src/IFStallSlot.cpp src/IFStallSlot.h src/CustomInterface/IFSavedLocation.cpp
        src/CustomInterface/IFSavedLocation.h src/CustomInterface/IFDropLogWnd.cpp src/CustomInterface/IFDropLogWnd.h
        src/CustomInterface/IFDropLogsWnd.cpp src/CustomInterface/IFDropLogsWnd.h
        src/CustomInterface/IFLuckySpinWnd.cpp src/CustomInterface/IFLuckySpinWnd.h
        src/CustomInterface/IFSpecialOffersWnd.cpp src/CustomInterface/IFSpecialOffersWnd.h
        src/CustomInterface/IFKillerAnimationWnd.cpp src/CustomInterface/IFKillerAnimationWnd.h
        src/CustomInterface/IFPvpChallengeWnd.cpp src/CustomInterface/IFPvpChallengeWnd.h
        src/CustomInterface/IFTradeCaptchaWnd.cpp src/CustomInterface/IFTradeCaptchaWnd.h
        src/CustomInterface/IFOfflineStall.cpp src/CustomInterface/IFOfflineStall.h
        src/CustomInterface/IFLoginRegisterWnd.cpp src/CustomInterface/IFLoginRegisterWnd.h
        src/CustomInterface/IFQuickLoginPanel.cpp src/CustomInterface/IFQuickLoginPanel.h
        src/CustomInterface/IFMovePartyMember.cpp
        src/CustomInterface/IFMovePartyMember.h
        src/CustomInterface/IFMovePartyMemberSlot.cpp
        src/CustomInterface/IFMovePartyMemberSlot.h src/NewItemMall/IFVAvatarMall.cpp
        src/NewItemMall/IFVAvatarMall.h src/NewItemMall/IFVAvatarMallBuyItemList.cpp
        src/NewItemMall/IFVAvatarMallBuyItemList.h src/NewItemMall/IFVAvatarMallBuyItemListSlot.cpp
        src/NewItemMall/IFVAvatarMallBuyItemListSlot.h src/NewItemMall/IFVAvatarMallSlot.cpp
        src/NewItemMall/IFVAvatarMallSlot.h src/IFRenderStatic.cpp src/IFRenderStatic.h
        src/NewItemMall/IFVItemMall.cpp src/NewItemMall/IFVItemMall.h src/NewItemMall/IFVItemMallBuyItem.cpp
        src/NewItemMall/IFVItemMallBuyItem.h src/NewItemMall/IFVItemMallFavList.cpp
        src/NewItemMall/IFVItemMallFavList.h src/NewItemMall/IFVItemMallSlot.cpp src/NewItemMall/IFVItemMallSlot.h
        src/NewItemMall/IFVItemMallSlotNews.cpp src/NewItemMall/IFVItemMallSlotNews.h src/NewItemMall/IFVItemMallSlotViewer.cpp
        src/NewItemMall/IFVItemMallSlotViewer.h src/NewItemMall/IFVSelectMall.cpp src/NewItemMall/IFVSelectMall.h
        src/Web/SilkroadWeb.cpp src/Web/SilkroadWeb.h
        src/Web/IFWeb.cpp src/Web/IFWeb.h
        src/Web/IFWebGuide.cpp src/Web/IFWebGuide.h
        src/WebViewer/WebViewerConfig.cpp src/WebViewer/WebViewerConfig.h
        src/WebViewer/WebViewerBridgeClient.cpp src/WebViewer/WebViewerBridgeClient.h
        src/WebViewer/IFWebViewerGuide.cpp src/WebViewer/IFWebViewerGuide.h
        src/WebViewer/IFWebViewerFrame.cpp src/WebViewer/IFWebViewerFrame.h
        src/DailyLogin/IFDailyLoginGuide.cpp
        src/DailyLogin/IFDailyLoginGuide.h src/DailyLogin/IFDailyLogin.cpp
        src/DailyLogin/IFDailyLogin.h
        src/DailyLogin/IFDailyLoginDaySlot.cpp
        src/DailyLogin/IFDailyLoginDaySlot.h
        src/DailyLogin/IFDailyLoginRewardSlot.cpp
        src/DailyLogin/IFDailyLoginRewardSlot.h src/IFItemMall.cpp src/IFItemMall.h
        src/IFItemMallConfirmBuy.cpp src/IFItemMallConfirmBuy.h src/IFItemMallShopSlot.cpp
        src/IFItemMallShopSlot.h src/InterfaceNetSender.cpp
        src/InterfaceNetSender.h src/CustomInterface/IFSoxEffect.cpp
        src/CustomInterface/IFSoxEffect.h
        src/IFItemMallZzim.cpp src/IFItemMallZzim.h
        src/IFGhaCha.cpp src/IFGhaCha.h
        src/IFGhachaSelectWnd.cpp src/IFGhachaSelectWnd.h
        src/Macro/IFMacroGuide.cpp src/Macro/IFMacroGuide.h
        src/Macro/IFMacro.cpp src/Macro/IFMacro.h
        src/Macro/IFMacroMenu.cpp src/Macro/IFMacroMenu.h
        src/Macro/IFMacroMenuAutoPotion.cpp src/Macro/IFMacroMenuAutoPotion.h
        src/Macro/IFMacroSlot.cpp src/Macro/IFMacroSlot.h src/IFSliderCtrl.cpp src/IFSliderCtrl.h
        src/Managers/ItemReuseDelayManager.cpp src/Managers/ItemReuseDelayManager.h
        src/Managers/COSDataMgr.h src/Macro/IFMacroMenuAutoSkill.cpp src/Macro/IFMacroMenuAutoSkill.h
        src/Macro/IFMacroMenuAutoSkillSlotViewer.cpp src/Macro/IFMacroMenuAutoSkillSlotViewer.h
        src/Macro/IFMacroMenuAutoSkillSlotSlot.cpp src/Macro/IFMacroMenuAutoSkillSlotSlot.h
        src/Macro/IFMacroSlotWep.cpp src/Macro/IFMacroSlotWep.h
        src/Macro/IFMacroSlotSkill.cpp src/Macro/IFMacroSlotSkill.h
        src/SkillCoolTimeManager.cpp src/SkillCoolTimeManager.h
        src/Macro/IFMacroMenuAutoHunt.cpp src/Macro/IFMacroMenuAutoHunt.h
        src/Macro/IFMacroMenuAutoSkillPartySlot.cpp src/Macro/IFMacroMenuAutoSkillPartySlot.h
        src/CustomInterface/IFPopupList2.cpp src/CustomInterface/IFPopupList2.h
        src/Macro/IFMacroMenuPickFilter.cpp src/Macro/IFMacroMenuPickFilter.h
        src/IItem.cpp src/IItem.h src/DropItemManager.cpp src/DropItemManager.h
        src/Macro/IFMacroMenuAutoScrollSlot.cpp src/Macro/IFMacroMenuAutoScrollSlot.h
        src/Macro/IFMacroSCSlot.cpp src/Macro/IFMacroSCSlot.h

        src/MacroAlchemy/IFMacroAlchemySlot.cpp src/MacroAlchemy/IFMacroAlchemySlot.h
        src/MacroAlchemy/IFAlchemyMacroGuide.cpp src/MacroAlchemy/IFAlchemyMacroGuide.h
        src/MacroAlchemy/IFAlchemyMacro.cpp src/MacroAlchemy/IFAlchemyMacro.h

        src/NIFAlchemySubWndType1.cpp src/NIFAlchemySubWndType1.h
        src/GlobalItemLinking/GlobalItemLinking.cpp src/GlobalItemLinking/GlobalItemLinking.h
        src/GlobalItemLinking/MemoryHelper.cpp src/GlobalItemLinking/MemoryHelper.h
        src/IFTimerWnd.cpp src/CustomInterface/IFKillCounter.cpp src/CustomInterface/IFKillCounter.h
        src/KillerAnimationPlayer.cpp src/KillerAnimationPlayer.h
        src/IFCOS.cpp src/IFCOS.h src/IFCOSCommand.cpp src/IFCOSCommand.h src/IFCOSInfo.cpp src/IFCOSInfo.h
        src/IFCOSInventory.cpp src/IFCOSInventory.h src/IFCOSManager.cpp src/IFCOSManager.h src/IFCOSSetup.cpp
        src/IFCOSSetup.h src/IFCOSStatus.cpp src/IFCOSStatus.h src/IFPetMiniInfo.cpp
        src/IFPetMiniInfo.h src/IFNormalTile.h src/Hwid/HWIDGenerator.cpp
        src/Hwid/HWIDGenerator.h src/ICScriptObjManager.cpp
        src/ICScriptObjManager.h src/CharacterVisualChange.cpp
        src/CharacterVisualChange.h
        src/CustomInterface/IFTeamCounter.cpp src/CustomInterface/IFTeamCounter.h
        )

target_link_libraries(ClientLib SROInterfaceLIB GFXMainFrame GFX3DFunction GFXFileManagerLIB NavMesh ClientNet SimpleViewer TypeId imgui support ghidra DiscordRichPresence)
target_include_directories(ClientLib PUBLIC src/)
