# KMTGuard — GameServer Packets Map (خريطة الـ Packets)

> **Status tags:** ✅ Confirmed | 🔍 Inference | ❓ Unknown
> **المصدر:** `CGObjPCCustom.cpp` + `Game.cpp` + `DamageMeter.cpp` + `HonorRankRuntimeRefresh.cpp`

---

## 1. ✅ حزم الكلاينت → GameServer عبر الفلتر (CGObjPC::ReaderPacket)

| Opcode | الاتجاه | الـ Handler | البيانات | Validation | SQL? | ينتظر SQL? | نسخ Packet? | الخطورة |
|---|---|---|---|---|---|---|---|---|
| `0x35FE` | C→GS | `ReaderPacket` inline | `bootstrapKey + sessionKey(64hex)` | `bootstrapKey==GSKEY && IsHexKey` | لا | لا | لا | **Critical** |
| `0x3500` | C→GS | `HandleGrantNameRequest` | `Key + NewGrantName` | `ValidateFilterSessionKey` + `GuildLevel>=4` | لا | لا | لا | High |
| `0x3501` | C→GS | `HandleTitleChangeRequest` | `Key + TitleID` | `ValidateFilterSessionKey` + `!BODYMODE_HWAN` | لا | لا | لا | Medium |
| `0x3502` | C→GS | `HandleChangePvpCapeRequest` | `Key + Cape` | `ValidateFilterSessionKey` + `CanSendMessage` | لا | لا | لا | Medium |
| `0x3504` | C→GS | `HandleCustomReverseUseRequest` | `Key + SlotID + WorldID + wRegionID + X/Y/Z` | `ValidateFilterSessionKey` + `TID in [6636,6637]` | لا | لا | لا | Medium |
| `0x3505` | C→GS | `HandleItemTranslateRequest` | `Key + PaymentMethod + ItemSlot + ItemCodeName + TargetItemCodeName [+ TargetGold]` | `ValidateFilterSessionKey` + `LockedItemList` + `m_strObjectCode == ItemCodeName` | لا | لا | لا | **High** |
| `0x3506` | C→GS | `HandleLiveItemChestPacket` | `Key + ItemDBID + ItemCodeName + Amount + RandomizeStats + OptLevel` | `ValidateFilterSessionKey` + `ItemDBID>0 && Amount in [1,1000000]` | لا | لا | لا | **High** |
| `0x3511` | C→GS | `HandleFellowSkill` | `Key + SkillID + petuqID + AnimationID` | `ValidateFilterSessionKey` + `CanSendMessage` + dupe skill check | لا | لا | لا | Medium |
| `0x3527` | C→GS | `HandleSilkPacket` | `Key + silkOwn + silkGift + silkPoint` | `ValidateFilterSessionKey` + `CanSendMessage` | لا | لا | لا | **High** |
| `0x3528` | C→GS | `HandleCosSkill` | `Key + PetUQID + PetSkillID` | `ValidateFilterSessionKey` | لا | لا | لا | Medium |
| `0x3530` | C→GS | `HandleCustomScrollUsage` | `Key + itemId + itemSlotId + itemTypeId` | `ValidateFilterSessionKey` + `RefItemID==itemId && TID==0xC6ED` | لا | لا | لا | Medium |
| `0x3531` | C→GS | `HandleItemLockRequest` | `Key + itemId + itemSlotId + itemTypeId + LockedItemSlot` | `ValidateFilterSessionKey` + `!LockedItemList.find` + `RefItemID==itemId && TID==0xCEED` | لا | لا | لا (يرسل 0x5060 للـ SM) | **High** |
| `0x3532` | C→GS | `HandleItemUnlockRequest` | `Key + itemId + itemSlotId + itemTypeId + LockedItemSlot` | `ValidateFilterSessionKey` + `LockedItemList.find` + `RefItemID==itemId && TID==0xD6ED` | لا | لا | لا (يرسل 0x5062 للـ SM) | **High** |
| `0x3533` | C→GS | `HandleAlchemyLinkRequest` | `Key + ItemSlot + AdvPlus` | `ValidateFilterSessionKey` + item valid | لا | لا | **نعم** (يبني 0x5034 مع nLen+itemData+RefItemID+OptLevel+AdvPlus) | Medium |
| `0x3534` | C→GS | `HandleNewAlchemyRequest` | `Key + FuseType + ItemSlot + EnhancerSlot + [ProofSlot]` | `ValidateFilterSessionKey` + `LockedItemList` + TID check + Param3/Degree check + ProofStone check | **نعم (UPDATE _Items)** | **نعم (Sync)** | **نعم** (يبني 0x5017 response) | **Critical** |
| `0x3537` | C→GS | `HandleDisplayCharInfoRequest` | `Key + SenderName` | `ValidateFilterSessionKey` | لا | لا | **نعم** (لكل slot 0x5038 مع itemData كاملة) | Low |
| `0x3538` | C→GS | `ReaderPacket` inline | `Key + uniquetype (0-4)` | `ValidateFilterSessionKey` | لا | لا | لا | **High** |
| `0x3539` | C→GS | `HandleSelfTeleportRequest` | `Key` | `ValidateFilterSessionKey` + `wRegionID!=0 && dwWorldID!=0` | لا | لا | لا | Medium |
| `0x3540` | C→GS | `HandleFilterTeleportRequest` | `Key + gameWorldId + regionId + posX/Y/Z` | `ValidateFilterSessionKey` + `gameWorldId>0 && regionId>0` | لا | لا | لا | Medium |
| `0x704F` | C→GS | `ReaderPacket` inline | `state` | `state==2 && GoldPetPtr!=NULL` → block | لا | لا | لا | Low |
| `0x705C` | C→GS | `HandleGlobalItemLink` | `GlobalType + GlobalSlot + GlobalItemType + GlobalItemID + Message` | — | لا | لا | **نعم** (يبني 0x5033 ويرسله للفلتر) | Low |
| `0x705D` | C→GS | `ReaderPacket` inline | `chatType + receiver + text + linkedItemSlot` | — | لا | لا | **نعم** (يبني 0x5068 للـ SM) | Medium |
| `0x7074` | C→GS | `ReaderPacket` inline | `action + target + skill/stats` | `Monsterclass==3` STR/INT check + `CanMobInteractWithPlayer` | لا | لا | لا | **High** |
| `0x7150` | C→GS | `ReaderPacket` inline | `type + slots` | `LockedItemList.find || TimedItemList.find` → 0x5015 | لا | لا | لا | **High** |
| `0x7151` | C→GS | `ReaderPacket` inline | `type + slots` | `LockedItemList.find || TimedItemList.find` | لا | لا | لا | **High** |
| `0x716A` | C→GS | `ReaderPacket` inline | `type + slots` (3 cases) | `LockedItemList.find || TimedItemList.find` | لا | لا | لا | **High** |
| `0x7157` | C→GS | `ReaderPacket` inline | `type + ItemSlot` | `LockedItemList.find || TimedItemList.find` | لا | لا | لا | **High** |
| `0x7034` | C→GS | `ReaderPacket` inline | `type + slots` (7 inventory ops) | `LockedItemList.find || TimedItemList.find` لكل فرع | لا | لا | لا | **High** |
| `0x70BA` | C→GS | `ReaderPacket` inline | `updateType + Slot + SourceSlot + StackCount + Price` | `LockedItemList.find || TimedItemList.find` | لا | لا | لا | **High** |

---

## 2. ✅ حزم ShardManager → GameServer (CGame::ProcessMessage)

| Opcode | الاتجاه | الـ Handler | البيانات | Validation | SQL? | ينتظر SQL? | نسخ? | الخطورة |
|---|---|---|---|---|---|---|---|---|
| `0x8888` | Shard→GS | `ProcessMessage` switch | `Type + 1-9 data params` حسب النوع | 28 switch case | لا | لا | لا | **Critical** |
| `0x5061` | Shard→GS | `case 0x5061` | `INT64 ItemID64` | `LockedItemList.find` | لا | لا | لا | High |
| `0x5063` | Shard→GS | `case 0x5063` | `INT64 ItemID64` | `LockedItemList.find` | لا | لا | لا | High |
| `0x5066` | Shard→GS | `case 0x5066` | `CharID + ID64 + OldOptLevel + endtime` | `TimedItemList.find` | لا | لا | لا | High |
| `0x5068` | Shard→GS (Timed) | `case 0x5068` | `INT64 ID64` | `TimedItemList.find` + erase | لا | لا | لا | High |
| `0x5069` | Shard→GS (Chat) | `case 0x5069` | `SenderCharID + chatType + receiver + text + linkedItemSlot` | ❌ الكود مُعلّق بالكامل | لا | لا | — | Medium (معطل) |
| `0x3C80` | Shard→GS | `HonorRankRuntimeRefresh` | subtype `0x0B` + بيانات الرتب | — | لا | لا | **نعم** (بث لكل اللاعبين) | Medium |

---

## 3. ✅ GameServer → ShardManager (مرسلة)

| Opcode | المكان | الوظيفة |
|---|---|---|
| `0x5060` | `HandleItemLockRequest` | طلب قفل أيتم → يبث `0x5061` لكل GS |
| `0x5062` | `HandleItemUnlockRequest` | طلب فك قفل → يبث `0x5063` لكل GS |
| `0x5065` | `CGObjPCCustom.cpp` (inline) | إعلام بقفل Timed Plus → يبث `0x5066` لكل GS |
| `0x5067` | `CustomTimedJobManager.cpp` | إزالة Timed Plus من كل GS |
| `0x5068` | `CGObjPCCustom.cpp` (0x705D) | Chat مع item link |
| `0x7808` | ShardManager (MainProcess.cpp) | غلاف يحتوي `0x300C` + `0xC05/0xC06` |

---

## 4. ✅ GameServer → Client (مباشرة أو عبر الفلتر)

| Opcode | المكان | الوظيفة |
|---|---|---|
| `0x3040` | `HandleNewAlchemyRequest` success, `CustomTimedJobManager` | تحديث OptLevel للعميل |
| `0x305C` | `HandleItemLock/Unlock/CustomReverseUse` | Visual effect |
| `0x3571` | `GObjEvents::CheckRegionNeedChange` | إعلام الفلتر بتغيير المنطقة |
| `0x5010` | `DamageMeter::MyCGObjNPC_HandleAggroMap` | DPS meter data للفلتر |
| `0x5015` | `ReaderPacket` item block checks | إعلام العميل بأن الأيتم مقفول/موقوت |
| `0x5017` | `HandleNewAlchemyRequest` | نتيجة New Alchemy للفلتر |
| `0x5025` | `ConsumeTimedItemPlusRecords` | إعلام العميل بإزالة Timed Plus |
| `0x5030` | `HandleItemLockRequest` | إعلام LockedItemSlot للفلتر |
| `0x5031` | `HandleItemUnlockRequest` | إعلام UnlockedItemSlot للفلتر |
| `0x5033` | `HandleGlobalItemLink` | Global item link data للفلتر |
| `0x5034` | `HandleAlchemyLinkRequest`, Type 131 في 0x8888 | Alchemy link broadcast للفلتر |
| `0x5038` | `HandleDisplayCharInfoRequest` | إرسال بيانات الأيتم لشخص آخر |
| `0x34FE` | `HonorRankRuntimeRefresh` | Honor Rank snapshot |
| `0x3563` | `ReaderPacket` 0x7074 | STR/INT block notice |
| `0x182C` | `HandleFellowSkill` | Fellow skill broadcast |
| `0x210B` | `HandleCustomScrollUsage` | Scroll usage broadcast |
| `0xA405` | `HandleLiveItemChestPacket` | Response for Item Chest delivery |
| `0x3026` | Chat | Ascii chat notice |

---

## 5. ✅ حزم في ShardManager (MainProcess.cpp)

| Opcode | الوظيفة |
|---|---|
| `0x5060` → `0x5061` | Lock Item: GS يرسل للـ SM → SM يبث 0x5061 لكل GS |
| `0x5062` → `0x5063` | Unlock Item: نفس المنطق |
| `0x5065` → `0x5066` | Timed Lock: GS → SM → كل GS |
| `0x5067` → `0x5068` | Timed Remove: GS → SM → كل GS |
| `0x5068` → `0x5069` | Chat with item: GS → SM → كل GS (معطل في الـ GS) |
| `0x7808` يحتوي `0x300C` | غلاف مع Unique Spawn/Kill: 0xC05/0xC06 |

---

## 6. ✅ نسخ الحزم (Packet Copying)

| الحالة | الوصف |
|---|---|
| `HandleNewAlchemyRequest` | يبني `0x5017` response جديد ببيانات كاملة عن الأيتم (باستخدام `AllocMsg+BindStreamBufferWithMsg+FlushStreamBufferMsg`) |
| `HandleAlchemyLinkRequest` / `HandleGlobalItemLink` / `HandleDisplayCharInfoRequest` | يبني `0x5034/0x5033/0x5038` عن طريق: حجز `pTmpMsg`، كتابة item data عليه، `FlushStreamBufferMsg`، قراءة البيانات منه، ثم إعادة كتابتها في الرسالة الجديدة |
| `0x705D` (Chat redirect) | يقرأ الحزمة من الكلاينت ويبني `0x5068` جديدة بالكامل للـ SM |
| 0x8888 Type 131 (Alchemy legacy) | نفس نمط item data copy |
| `PacketHandler.ExecutePipeline` (في الفلتر) | ينشئ `new Packet(currentPacket)` لكل Handler في الـ Pipeline |

---

## 7. جدول حساسية الـ Packets

| الخطورة | الـ Opcodes |
|---|---|
| **Critical** | `0x35FE`, `0x3534`, `0x8888` |
| **High** | `0x3500`, `0x3505`, `0x3506`, `0x3527`, `0x3531`, `0x3532`, `0x3538`, `0x7074`, `0x7150`, `0x7151`, `0x716A`, `0x7157`, `0x7034`, `0x70BA`, `0x5061`, `0x5063`, `0x5066`, `0x5068` |
| **Medium** | `0x3501`, `0x3502`, `0x3504`, `0x3511`, `0x3528`, `0x3530`, `0x3533`, `0x3539`, `0x3540`, `0x704F`, `0x705D`, `0x5069`, `0x3C80` |
| **Low** | `0x3505` (جزئياً), `0x3537`, `0x705C` |