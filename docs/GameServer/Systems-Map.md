# KMTGuard — GameServer Systems Map (خريطة الأنظمة)

> **Status tags:** ✅ Confirmed | 🔍 Inference | ⏱️ Needs Profiling | ❓ Unknown
> **الملف المرجعي:** `KMTGuardCustom/CGObjPCCustom.cpp` (6365 سطر) + جميع ملفات `KMTGuardCustom/`.

---

## 1. Filter Session Keys ✅

1. **الوصف:** يخزّن مفتاح جلسة 64 hex per CharID. كل حزمة مخصصة من الفلتر يجب أن تحمل هذا المفتاح.
2. **Entry:** `ReaderPacket` → `0x35FE`.
3. **Opcode:** `0x35FE` ← Filter (ExploitFixPackets.cs بعد 0x3012 GameReady + 250ms delay).
4. **Handler:** `SetFilterSessionKey` + `ValidateFilterSessionKey` (CGObjPCCustom.cpp أسطر 110-167).
5. **الملفات:** `CGObjPCCustom.cpp` — `g_FilterSessionKeys` (std::map<int,string> + CRITICAL_SECTION), `GSKEY = "KMTGUARD710632TEST"`.
6. **DB:** لا — في الذاكرة فقط.
7. **التنفيذ:** Synchronous (CRITICAL_SECTION).
8. **الخطورة:** **Critical**. بدونها كل الحزم المخصصة ترفض.
9. **ملف للمراجعة:** `CGObjPCCustom.cpp` 110-167.

---

## 2. New Alchemy (0x3534) ✅

1. **الوصف:** ألكيمي مخصص مع 15 مستوى + Proof Stone + بدون Proof Stone. كل النتائج (نجاح/فشل) محسوبة داخل الـ GS.
2. **Entry:** `ReaderPacket` → `0x3534`.
3. **Opcode:** `0x3534` → GS; result `0x5017` → Filter (SERVER_NEW_ALCHEMY_RESULT).
4. **Handler:** `HandleNewAlchemyRequest` (1606-6246).
5. **الملفات:** `CGObjPCCustom.cpp` + `sqlCon.cpp` (GetItemBindingOpt, TryExecNonQuery).
6. **الجداول:** `SRO_VT_SHARD.._Items` (UPDATE OptLevel عبر `UpdateItemPlusInDatabase`), `_BindingOptionWithItem` (SELECT عبر `GetItemBindingOpt`).
7. **الـ SP:** لا — مباشر SQL.
8. **التنفيذ:** **Synchronous** على خيط Network — يستدعي `TryExecNonQuery` + `GetItemBindingOpt` لكل فرع.
9. **انتظار SQL:** نعم — اللاعب ينتظر نتيجة الـ GS.
10. **فشل SQL:** `printf` فقط — الحالة في الذاكرة تتغير والـ DB لا تتحدث → **Lost Update**.
11. **لا Retry/Rollback.**
12. **الخطورة:** **Critical** — كتابة أيتم مباشرة.
13. **ملاحظة:** الكود يستخدم 15 `if-else` متسلسل لكل OptLevel لكل نوع (Weapon/Shield/Accessory/Armor) مع Proof وبدون Proof = 15×4×2 = 120+ فرع.
14. **خطر مؤكد:** عند فشل SQL، تبقى الذاكرة مرفوعة لكن DB لا تتحدث. إذا اللاعب logout قبل كتابة DB → اختفاء الـ Plus.

---

## 3. Item Lock/Unlock (0x3531/0x3532) ✅

1. **الوصف:** قفل/فك قفل أيتم بمادة محددة (0xCEED للقفل، 0xD6ED للفك).
2. **Entry:** `ReaderPacket` → `0x3531/0x3532`.
3. **Handler:** `HandleItemLockRequest` (1351-1406), `HandleItemUnlockRequest` (1407-1462).
4. **الملفات:** `CGObjPCCustom.cpp`, `sqlCon.cpp` (LockedItemList), `Game.cpp` (0x5061/0x5063).
5. **الجداول:** `KMTGuard.dbo.Item_Locked` (INSERT/DELETE عبر الفلتر — `CustomGameServerPacketHandler.cs`).
6. **التنفيذ:** الـ GS يضيف/يحذف من `CSqlCon::LockedItemList` في الذاكرة أولاً، ثم يرسل `0x5060/0x5062` إلى ShardManager الذي يبث `0x5061/0x5063` لكل الـ GS.
7. **الخطورة:** **High** — حماية أصول اللاعب.
8. **Race Condition:** محتمل — الفلتر يكتب DB (Async) بينما الـ GS يعتمد على رسائل ShardManager.

---

## 4. Timed Plus ✅

1. **الوصف:** أيتم له +X مؤقت. عند انتهاء المدة، `ConsumeThreadWorker` يعيد Plus الأصلي.
2. **Entry:** `CCustomTimedJobManager::CreateConsumeThread`.
3. **Opcode:** `0x5066` (Shard→GS add), `0x5068` (Shard→GS remove), `0x5067` (GS→Shard remove broadcast).
4. **الملفات:** `CustomTimedJobManager.cpp` + `Game.cpp` + `sqlCon.cpp`.
5. **الجداول:** `SRO_VT_SHARD.._Items` (UPDATE), `KMTGuard.Item_TimedPlus`, `Item_TimedDevilPlus`, `Item_Locked` (DELETE).
6. **التنفيذ:** Thread خلفي — كل 1-3 ثواني.
7. **الخطورة:** **High** — ❌ `ConsumeTimedItemPlusRecordsDevill` معلقة السطر 368 → Timed Devil Plus لا تُستهلك أبداً.
8. **❌ Bug مؤكد:** السطر 87 `sprintf` بدون buffer → Undefined Behavior.

---

## 5. Silk Packet (0x3527) ✅

1. **Entry:** `ReaderPacket` → `0x3527`.
2. **Handler:** `HandleSilkPacket` (1302-1315).
3. **البيانات:** `int silkOwn, silkGift, silkPoint`.
4. **التنفيذ:** `this->UpdateSilk(silkOwn, silkGift, silkPoint, true)` — مباشرة على الكائن.
5. **الخطورة:** **High** — تحديث أرصدة Silk مباشرة.

---

## 6. COS Skill (0x3528) ✅

1. **Entry:** `ReaderPacket` → `0x3528`.
2. **Handler:** `HandleCosSkill` (1316-1326).
3. **التنفيذ:** `CGObjCOS_GoldPet::LiveSkill2(PetSkillID)`.
4. **الخطورة:** Medium.

---

## 7. Fellow Skill (0x3511) ✅

1. **Entry:** `ReaderPacket` → `0x3511`.
2. **Handler:** `HandleFellowSkill` (1273-1301).
3. **التنفيذ:** يتحقق من عدم تكرار SkillID في `UsedSkillList`، ثم `BroadcastMsgToNearbyPlayers(0x182C)` + `LiveSkill(SkillID)`.
4. **الخطورة:** Medium.

---

## 8. Custom Scroll Usage (0x3530) ✅

1. **Entry:** `ReaderPacket` → `0x3530`.
2. **Handler:** `HandleCustomScrollUsage` (1327-1350).
3. **الشرط:** `RefItemID == itemId && TID.m_type_id_value == 0xC6ED` — scroll محدد.
4. **التنفيذ:** `SetLiveDeleteItem(itemSlotId, 1)` + `0x210B`.
5. **الخطورة:** Medium.

---

## 9. Item Translation (0x3505) ✅

1. **Entry:** `ReaderPacket` → `0x3505`.
2. **Handler:** `HandleItemTranslateRequest` (1184-1239).
3. **البيانات:** `PaymentMethod` (0=free, 1=gold), `ItemSlot`, `ItemCodeName`, `TargetItemCodeName`, `TargetGold`.
4. **التنفيذ:** فحص `LockedItemList` → فحص `m_strObjectCode == ItemCodeName` → إذا PaymentMethod=1 يفحص الذهب → `SetLiveItem(ItemSlot, TargetItemCodeName)`.
5. **الخطورة:** **High** — خطأ في Check قد يسمح بترجمة أيتم بدون الذهب المطلوب.

---

## 10. Live Item Chest (0x3506) ✅

1. **Entry:** `ReaderPacket` → `0x3506`.
2. **Handler:** `HandleLiveItemChestPacket` (1240-1272).
3. **البيانات:** `ItemDBID`, `ItemCodeName`, `Amount`, `RandomizeStats`, `OptLevel`.
4. **التنفيذ:** `AddItem(itemCodeName, Amount, RandomizeStats, OptLevel)` → response `0xA405` (response byte + ItemDBID).
5. **الخطورة:** **High** — إضافة أيتم مباشرة.

---

## 11. Display Char Info (0x3537) ✅

1. **Entry:** `ReaderPacket` → `0x3537`.
2. **Handler:** `HandleDisplayCharInfoRequest` (1532-1588).
3. **التنفيذ:** لكل slot 0-12 (ما عدا 8) يبني `0x5038` مع بيانات الأيتم الكاملة ويرسلها للفلتر.
4. **الخطورة:** Low.

---

## 12. Global Item Link (0x705C) ✅

1. **Entry:** `ReaderPacket` → `0x705C`.
2. **Handler:** `HandleGlobalItemLink` (1035-1114).
3. **البيانات:** `GlobalType` (0=no item, 1=with item), `GlobalSlot`, `GlobalItemType`, `GlobalItemID`, `Message`.
4. **التنفيذ:** يبني `0x5033` → Filter يبث `0x179B` أو `0x705E`.
5. **الخطورة:** Low.

---

## 13. Alchemy Link (0x3533) ✅

1. **Entry:** `ReaderPacket` → `0x3533`.
2. **Handler:** `HandleAlchemyLinkRequest` (1463-1528).
3. **التنفيذ:** يبني `0x5034` مع بيانات الأيتم الكاملة (nLen bytes + RefItemID + OptLevel + AdvPlus).
4. **الخطورة:** Medium.

---

## 14. Self/Filter Teleport (0x3539/0x3540) ✅

1. **Entry:** `ReaderPacket` → `0x3539/0x3540`.
2. **Handler:** `HandleSelfTeleportRequest` (6308-6334), `HandleFilterTeleportRequest` (6336-6365).
3. **Self Teleport:** `MoveTo(currentWorld, currentRegion, currentX, currentY, currentZ, 2)` — يحاول mode 2 أولاً.
4. **Filter Teleport:** `MoveTo(gameWorldId+0x10000, regionId, posX, posY, posZ, 2)` — بمعلومات من الفلتر.
5. **الخطورة:** Medium.

---

## 15. Grant Name (0x3500) ✅

1. **Entry:** `ReaderPacket` → `0x3500`.
2. **Handler:** `HandleGrantNameRequest` (1115-1127).
3. **الشرط:** `MyGuild != NULL && IntanceGuild->GuildLevel >= 4`.
4. **الخطورة:** Medium.

---

## 16. Title/PvP Cape (0x3501/0x3502) ✅

1. **0x3501:** `HandleTitleChangeRequest` (1128-1138) — `UpdateHwan(TitleID)` إذا ليس في وضع Berserk.
2. **0x3502:** `HandleChangePvpCapeRequest` (1139-1148) — `UpdatePVPCapeType(Cape)`.
3. **الخطورة:** Low.

---

## 17. Custom Reverse Use (0x3504) ✅

1. **Entry:** `ReaderPacket` → `0x3504`.
2. **Handler:** `HandleCustomReverseUseRequest` (1149-1183).
3. **الشرط:** `TID.m_type_id_value == 6636 || 6637` (Reverse Return Scrolls).
4. **التنفيذ:** `SetLiveDeleteItem(SlotID, 1)` + effect `0x305C` + `MoveTo(1+0x10000, wRegionID, X, Y, Z, 2)`.
5. **الخطورة:** Medium.

---

## 18. Spawn Unique (0x3538) ✅

1. **Entry:** `ReaderPacket` → `0x3538` (inline في ReaderPacket).
2. **البيانات:** `byte uniquetype` (0-4).
3. **التنفيذ:** `CGObjMob::CreateMob(46406 + uniquetype, worldID, regionID, x, y, z, 5)`.
4. **الخطورة:** **High** — أي شخص بمفتاح جلسة صحيح يمكنه spawn unique.

---

## 19. STR/INT + Attack Restrictions (0x7074) ✅

1. **Entry:** `ReaderPacket` → `0x7074`.
2. **التنفيذ:** فحص `pMob->Monsterclass == 3` (Unique) → فحص `_STR/_INT` في الاسم → فحص `m_pObjDataInstance->Strength/Intellect` → `0x3563` block إذا خالف.
3. **ثم:** `CRegionAttackRestrictionsMgr::CanMobInteractWithPlayer` لفحص قيود الهجوم.
4. **الخطورة:** **High** — حماية Uniques.

---

## 20. Item Powerup Block ✅

1. **Opcodes محمية:** `0x7150/0x7151/0x716A/0x7157/0x7034/0x70BA`.
2. **التنفيذ:** فحص `LockedItemList` و`TimedItemList` قبل كل عملية.
3. **النتيجة:** إذا was locked → `0x5015` type=TYPE_OF_ITEM_LOCKED; إذا TimedPlus → `0x5015` type=0 + "VFILTER_ITEM_POWERUP".
4. **الخطورة:** **High**.

---

## 21. Chat Redirect (0x705D) ✅

1. **Entry:** `ReaderPacket` → `0x705D`.
2. **التنفيذ:** يقرأ `chatType, receiver, text, linkedItemSlot`، ثم يبني `0x5068` ويرسله إلى ShardManager.
3. **ShardManager:** `MainProcess.cpp` — 0x5068 → 0x5069 لكل GS.
4. **Game.cpp:** case 0x5069 — ❌ **الكود مُعلَّق بالكامل** → لا معالجة للشات الخاص مع item link داخل الـ GS.
5. **الخطورة:** **Medium** — ميزة غير مكتملة.

---

## 22. DamageMeter (DPS) ✅

1. **Entry:** `CDamageMeter::Initialize()` — Detour على `CGObjNPC_HandleAggroMap` (0x004C44A0).
2. **التنفيذ:** بعد استدعاء الأصلي، ينظف aggro map من اللاعبين الممنوعين (عبر `CanMobInteractWithPlayer`)، ثم يبني `0x5010` بأول PC في القائمة مع max 15 victims.
3. **الخطورة:** Low.

---

## 23. Durability Control ✅

1. **Entry:** `DurabilityControl::Initialize()` — Hook على 0x00496E34.
2. **الوصف:** يمنع نقصان المتانة (EDI < EBX) مع السماح بالزيادة (repair/stones).
3. **القراءة:** `DisableDurability` من `System_Settings` مرة واحدة عند الإقلاع.
4. **الخطورة:** Medium.

---

## 24. Honor Rank Runtime Refresh ✅

1. **Entry:** `HonorRankRuntimeRefresh::Initialize()` — `replaceOffset` على `kTrainingCampQueryCompletionCall` (0x005E13A8).
2. **التنفيذ:** بعد اكتمال query الأصلي، يستدعي `BroadcastHonorRankSnapshot` الذي يقرأ `cachedRanking` من `CTrainingCampManager` + 0x2C ويبث `0x34FE` لكل لاعب.
3. **الخطورة:** Medium.

---

## 25. Party Monster Control ✅

1. **Entry:** `PartyMonsterControl::Initialize()`.
2. **التنفيذ:** يعدّل `immediate byte` في 0x00558F20 (minimumMembers) و 0x005608E2 (spawnRate) عن طريق `WriteImmediateByte`.
3. **التحقق:** `VirtualProtect` + `FlushInstructionCache` + مقارنة بعد الكتابة.
4. **الخطورة:** Low.

---

## 26. HideName Regions ✅

1. **Entry:** `CRegionRestrictionMgr::Initialize()` — 3 hooks.
2. **الوصف:** إذا كان اللاعب في منطقة مخفية الاسم (`Security_HideNameRegions`):
   - اسم الشخصية يصبح "KMTGuard"
   - اسم الجيلد فارغ
   - مستوى الـ Hwan يصفّر
3. **الخطورة:** Medium.

---

## 27. Command_GameServerQueue (0x8888) ✅

1. **28 Type مؤكد من Game.cpp:**
   Type 1 (GrantName), 2 (Spawn NPC), 3 (Spawn near char), 4/5 (Remove mob), 6/7/8 (Buff), 10/11 (To Town), 12 (Pet skill), 13 (PVP Cape), 14 (Teleport), 15 (GetUp), 16 (EXP), 17/18/19 (Live Item), 20 (Silk), 21 (Gold), 22 (Layer To Town), 23 (GetUp At Position), 24 (SafeZone No Job), 38 (Tower Defense), 39 (Free-for-all), 131 (Alchemy legacy), 22222 (Plus item — partially disabled).
2. **الخطورة:** **Critical**.

---

## 28. ❓ Custom NPC Event (0x704F blocked if GoldPet) ✅

1. **Entry:** `ReaderPacket` → `0x704F` (inline في ReaderPacket السطر 1016).
2. **التنفيذ:** إذا `state == 2 && GoldPetPtr != NULL` → يمنع العملية (early return).
3. **الخطورة:** Low.

---

## ملخص الخطورة

| النظام | الخطورة | SQL Sync? | Lost Update Risk |
|---|---|---|---|
| New Alchemy | **Critical** | ✅ Yes | ✅ Yes |
| Session Keys | **Critical** | No | No |
| 0x8888 Commands | **Critical** | Indirect | ✅ Yes (ShardManager Delete) |
| Item Lock/Unlock | High | No (GS memory only) | ✅ Yes (Filter writes DB) |
| Timed Plus | High | ✅ Yes (Thread) | ✅ Yes (no TX) |
| Item Translation | High | No | No |
| Live Item Chest | High | No | No |
| Silk Update | High | No | No |
| Spawn Unique (0x3538) | High | No | No |
| STR/INT + Attack | High | No | No |
| Item Powerup Block | High | No | No |
| Timed Devil Plus | **Critical** (معلق) | — | N/A (never runs) |
| Chat Redirect | Medium | No | N/A (معطل) |
| Rest | Low-Medium | No | No |