# مرجع قاعدة بيانات KMTGuard للعملاء

> نظام Title/Tag/Colors/Icons أُعيد بناؤه بتاريخ 2026-07-27. المرجع
> النهائي الوحيد لأسماء جداول وإجراءات هذا الجزء هو
> [player_style_system.md](player_style_system.md). أي اسم `Style_*` مذكور
> لاحقًا في النسخة التاريخية من هذا الملف لا يُستخدم بعد تطبيق الـmigration.

آخر تحديث: 2026-07-26  
قاعدة البيانات الأساسية: `KMTGuard`

نطاق المراجعة الحالية: قاعدة `KMTGuard` الفعلية (`106` جدول و`109`
Procedure)، جداول أحداث KMTGuard الحديثة داخل قاعدة `Events`، وكود تنفيذ
`Command_FilterQueue` و`Command_PlannedQueue` داخل الفلتر، وكود تنفيذ
`Command_GameServerQueue` داخل إضافة الـShardManager. لذلك أرقام الأوامر
المذكورة هنا مطابقة للكود الحالي وليست تخمينًا من أسماء الجداول.

هذا الملف معمول لصاحب السيرفر أو مسؤول الإدارة، ولا يفترض إنك مبرمج SQL. ستجد فيه الأدوات التي تحتاجها فعلًا لتشغيل الفلتر: الإعدادات، الحماية، المكافآت، الرانكات، الـEvents، الـHooks، الـItem Mall، الـTitles والـIcons، والتقارير المهمة.

الدليل لا يحوّل قاعدة البيانات إلى قائمة أسماء بلا فائدة. إجراءات حفظ حالة واجهة اللاعب يتركها صاحب السيرفر للفلتر. أما الـ`Hook_*` فهي جزء مهم من التخصيص، ولذلك لها فصل عملي يشرح متى يناديها الفلتر وكيف تضيف من خلالها شرطًا أو مكافأة بأمان.

## ابدأ منين؟

| لو عايز تعمل | اذهب إلى |
|---|---|
| تشغّل أو تقفل ميزة أو تغيّر Limit/Delay | مركز التحكم: `System_Settings` |
| ترسل Notice أو تفصل لاعب | Procedures الأوامر |
| تضيف Silk أو Gold أو Item | `Live_*` و`Item_AddChest` |
| تعمل Rank جديد | إنشاء رانك خطوة بخطوة |
| تعمل Auto Event أو Survival أو LMS | تشغيل وإدارة Auto Events |
| تخصص مكافأة عند Kill أو Alchemy أو Unique | دليل الـHooks |
| تضبط Event Suit أو قيود Region | إعداد Region Rules |
| تضيف Item Mall أو Offer أو Lucky Spin | أقسام المتجر والمكافآت |
| تعرف فائدة Table معينة | كتالوج الجداول |
| تعرف Parameters أي Procedure | ملحق صيغ الاستدعاء |

### معاني الأسماء التي ستتكرر

| الاسم | معناه |
|---|---|
| `CharID` | رقم الشخصية من `SRO_VT_SHARD.dbo._Char`. |
| `JID` | رقم الحساب من `SRO_VT_ACCOUNT.dbo.TB_User`. |
| `RefObjID` أو `ItemRefObjID` | رقم التعريف من `_RefObjCommon.ID`. |
| `CodeName128` | الاسم النصي الثابت للآيتم أو الموب. |
| `WorldID` | نسخة العالم التي يعمل داخلها الحدث أو الماب. |
| `RegionID` | رقم المنطقة داخل العالم. |
| `OUTPUT` | قيمة ترجعها الـProcedure؛ يجب استقبالها في Variable. |

## قبل أي تعديل

- لا تعدل جداول الـ queue يدويًا إلا لو فاهم نتيجة الأمر، لأنها أوامر ينتظرها الفلتر أو GameServer.
- لا تحذف logs قديمة إلا بعد backup.
- أي قيمة فيها `CharID` تعني رقم الشخصية من `SRO_VT_SHARD.dbo._Char`.
- أي قيمة فيها `JID` تعني رقم الحساب من `SRO_VT_ACCOUNT.dbo.TB_User`.
- أي قيمة فيها `ItemRefObjID` تعني رقم الآيتم من `SRO_VT_SHARD.dbo._RefObjCommon.ID`.
- أي قيمة فيها `CodeName128` تعني اسم الآيتم أو الموب النصي، مثل `ITEM_MALL_GLOBAL_CHATTING`.
- الأفضل في المكافآت تستخدم procedures جاهزة مثل `Item_AddChest`, `Live_Silk`, `Command_NoticeByID` بدل إدخال مباشر في الجداول.

## أشهر الاستخدامات الجاهزة

### إرسال Notice للاعب

```sql
EXEC KMTGuard.dbo.Command_NoticeByID
    @CharID = 12345,
    @NoticeType = 3,
    @Notice = 'Your reward has been added.';
```

### إرسال Notice للسيرفر كله

```sql
EXEC KMTGuard.dbo.Command_NoticeAll
    @NoticeType = 7,
    @Notice = 'Event will start in 5 minutes.';
```

### إضافة آيتم للـ Chest

```sql
DECLARE @ItemID int;

SELECT @ItemID = ID
FROM SRO_VT_SHARD.dbo._RefObjCommon
WHERE CodeName128 = 'ITEM_MALL_GLOBAL_CHATTING';

EXEC KMTGuard.dbo.Item_AddChest
    @CharID = 12345,
    @ItemRefObjID = @ItemID,
    @Quantity = 10,
    @From = 'Reward',
    @Plus = 0;
```

### إضافة Silk live

```sql
EXEC KMTGuard.dbo.Live_Silk
    @CharID = 12345,
    @nSilk = 100,
    @nSilkGift = 0,
    @nSilkPoint = 0;
```

لو عندك بروسيجر `_addsilklive` مثبتة في KMTGuard، استخدمها لما تحتاج تحديث `SK_Silk` مع سجل `SK_SilkChange_BY_Web`.

```sql
EXEC KMTGuard.dbo._addsilklive
    @CharID = 12345,
    @Amount = 100,
    @GiftAmount = 0,
    @PointAmount = 0;
```

### تغيير Title أو Icon

```sql
EXEC KMTGuard.dbo.Style_AddTitle @CharID = 12345, @TitleID = 1;
EXEC KMTGuard.dbo.Style_UpdateTitle @CharName16 = 'PlayerName', @TitleID = 1;

EXEC KMTGuard.dbo.Style_AddIcon @CharID = 12345, @IconID = 7, @Side = 0;
EXEC KMTGuard.dbo.Style_UpdateLeftIcon @CharName16 = 'PlayerName', @IconID = 7;
```

## أنواع الـ Notice

`NoticeType` يحدد شكل الرسالة داخل اللعبة. القيم الدقيقة تعتمد على إعدادات الكلاينت/الفلتر، لكن الاستخدامات الشائعة:

| القيمة | الاستخدام المعتاد |
|---:|---|
| `3` | رسالة للاعب أو تنبيه عادي |
| `6` | رسالة نظام/حدث |
| `7` | Notice عام واضح |
| `8` | رسالة إنجاز أو حالة خاصة |
| `11` | ذهبي في منتصف الشاشة وبوكس الشات للجوائز والأحداث المهمة |
| `12` | بنفسجي في منتصف الشاشة وبوكس الشات للـVIP والإنجازات النادرة |
| `13` | سماوي في منتصف الشاشة وبوكس الشات للمعلومات وتحديثات السيرفر |
| `14` | برتقالي في منتصف الشاشة وبوكس الشات للتحذيرات المهمة غير الحرجة |

## مركز التحكم: جدول `System_Settings`

جدول `KMTGuard.dbo.System_Settings` هو أول مكان تراجعه عندما تريد تشغيل ميزة، إخفاء زر، تغيير Limit أو Delay، أو تعديل سلوك الدخول والكلاينت. كل صف يمثل إعدادًا واحدًا:

| العمود | معناه |
|---|---|
| `SettingName` | اسم الإعداد الذي يبحث عنه الفلتر. لا تغيّره ولا تترجمه. |
| `Value` | قيمة الإعداد كنص: `True` أو `False` أو رقم أو اسم Database أو رابط. |
| `Category` | المجموعة التي ينتمي لها الإعداد لتسهيل ترتيبه في لوحة الإدارة. |
| `DisplayOrder` | ترتيب ظهوره داخل المجموعة. |
| `Description` | وصف مختصر للإعداد. |

### قراءة الإعدادات والبحث فيها

عرض كل الإعدادات مرتبة:

```sql
SELECT
    SettingName,
    Value,
    Category,
    Description
FROM KMTGuard.dbo.System_Settings
ORDER BY ISNULL(Category, 'Uncategorized'), DisplayOrder, SettingName;
```

البحث عن إعداد بالاسم:

```sql
SELECT *
FROM KMTGuard.dbo.System_Settings
WHERE SettingName = 'HWID_LIMIT';
```

البحث عن مجموعة إعدادات:

```sql
SELECT SettingName, Value, Description
FROM KMTGuard.dbo.System_Settings
WHERE SettingName LIKE '%Stall%'
   OR Category LIKE '%Stall%'
ORDER BY SettingName;
```

### الطريقة الآمنة لتعديل أي Setting

لا تستخدم `INSERT` إذا كان الإعداد موجودًا، لأن تكرار الاسم يسبب نتيجة غير واضحة. عدّل الصف الموجود، واعرض القيمة قبل وبعد التعديل:

```sql
BEGIN TRANSACTION;

SELECT SettingName, Value
FROM KMTGuard.dbo.System_Settings
WHERE SettingName = 'HWID_LIMIT';

UPDATE KMTGuard.dbo.System_Settings
SET Value = '3'
WHERE SettingName = 'HWID_LIMIT';

SELECT SettingName, Value
FROM KMTGuard.dbo.System_Settings
WHERE SettingName = 'HWID_LIMIT';

COMMIT TRANSACTION;
```

لو النتيجة ليست صحيحة قبل تنفيذ `COMMIT`، استخدم:

```sql
ROLLBACK TRANSACTION;
```

الإعدادات تُحمّل في ذاكرة خدمات الفلتر عند التشغيل. بعد إنهاء مجموعة التعديلات، أعد تشغيل خدمات KMTGuard في وقت مناسب لكي تتأكد أن كل الخدمات قرأت نفس القيم. لا تحاول استخدام Procedures التهيئة الداخلية كبديل عن إعادة تشغيل الخدمات.

### مفاتيح التشغيل العامة

| SettingName | وظيفته | القيم |
|---|---|---|
| `EnableOfflineStall` | تشغيل نظام الـOffline Stall. | `True` تشغيل، `False` إيقاف |
| `OfflineStallMaxHours` | أقصى مدة يظل فيها الـStall Offline بالساعات. | من `1` إلى `8760`، أو `0` بدون حد زمني |
| `EnablePvpChallenge` | تشغيل تحديات الـPVP بين اللاعبين. | `True` / `False` |
| `EnableQuickLogin` | تشغيل الدخول السريع المرتبط بالجهاز. | `True` / `False` |
| `EnableSpecialOffers` | تشغيل نافذة العروض الخاصة. | `True` / `False` |
| `NewInventoryDesign` | تشغيل تصميم الـInventory الجديد. | `True` / `False` |
| `EnableLuckySpin` | تشغيل نظام Lucky Spin. | `True` / `False` |
| `EnableLuckySpinSilk` | استخدام Silk كتكلفة للفة. | `True` / `False` |
| `LuckySpinPrice` | سعر اللفة الواحدة. | رقم صحيح غير سالب |
| `Macro` | تشغيل نظام الـMacro نفسه للاعبين. | `True` / `False` |

### قواعد البوت والحماية

| SettingName | وظيفته | ملاحظة مهمة |
|---|---|---|
| `AllowBotLogin` | السماح لجلسات البوت/Clientless الموثقة بالدخول. | `False` يمنع دخولها ويتطلب إثبات DLL الرسمي عند الـGateway. |
| `AllowBotTrade` | السماح لجلسة البوت باستخدام Exchange وTrade Goods وTrade Transport. | لا يعمل وحده إذا كان دخول البوت ممنوعًا. |
| `BotProtectionLogEnabled` | تسجيل قرارات حماية البوت في `Security_BotProtectionLog`. | فعّله مؤقتًا عند التشخيص أو اتركه مفعّلًا مع خطة تنظيف للـLog. |
| `IPLimit` | أقصى عدد Sessions من نفس الـIP. | اختر رقمًا مناسبًا للكافيهات واللاعبين المشتركين في اتصال واحد. |
| `HWID_LIMIT` | أقصى عدد Sessions من نفس الجهاز. | أقوى من IP Limit في منع Multi Client. |
| `HWID_JOB_LIMIT` | أقصى عدد شخصيات Job من نفس الجهاز. | يطبق على نشاط الـJob. |
| `DisableAcademy` | تعطيل نظام Academy. | `True` تعطيل |
| `DisableAutoAttack` | تعطيل الـAuto Attack. | `True` تعطيل |
| `AutoAttackMaxLevel` | أعلى Level يسمح عنده بالـAuto Attack إذا كان النظام مستخدمًا. | رقم Level |
| `DisableReverseInJob` | منع Reverse أثناء لبس الـJob Suit. | `True` منع |
| `DisableTraceWhileJob` | منع Trace أثناء الـJob. | `True` منع |
| `AlchemyItemLinkMinLevel` | أقل Plus يسمح عنده الفلتر بعرض/ربط نتيجة الألكيمي حسب النظام. | رقم Plus |
| `MaxPlus` | أعلى Plus مسموح للآيتمات العادية. | رقم Plus |
| `MaxPlusDevil` | أعلى Plus مسموح للـDevil/الآيتم الخاص. | رقم Plus |

مثال ضبط Limits لسيرفر يسمح بثلاثة Clients، لكن شخصية Job واحدة فقط من الجهاز:

```sql
UPDATE KMTGuard.dbo.System_Settings
SET Value =
    CASE SettingName
        WHEN 'HWID_LIMIT'     THEN '3'
        WHEN 'HWID_JOB_LIMIT' THEN '1'
        WHEN 'IPLimit'        THEN '6'
    END
WHERE SettingName IN ('HWID_LIMIT', 'HWID_JOB_LIMIT', 'IPLimit');
```

### الدخول والـGateway وقائمة السيرفر

| SettingName | وظيفته |
|---|---|
| `ServerName` | الاسم الذي يعرضه نظام الفلتر للسيرفر. |
| `FakePlayerCount` | الرقم الإضافي/الافتراضي المستخدم في عرض Online Count حسب إعداد الفلتر. |
| `ShowOnlinePlayers` | إظهار عدد اللاعبين Online. |
| `CheckStatus` | تفعيل فحص حالة الخدمة/السيرفر في مسار الدخول. |
| `CaptchaValue` | قيمة/وضع Captcha المستخدم في الـGateway. لا تغيّرها بالتجربة على Production. |
| `RemoveCaptcha` | إزالة Captcha الدخول التقليدي. |
| `SecondaryPassword` | تشغيل كلمة المرور الثانية. |
| `NewCharInfo` | تشغيل Packet/عرض معلومات الشخصية الجديد المدعوم. |
| `NewIdPw` | تشغيل سلوك ID/Password الجديد المدعوم بالكلاينت. |
| `OldLogin` | تشغيل توافق شاشة/مسار Login القديم. |

لا تجمع بين تغييرات `NewIdPw` و`OldLogin` و`RemoveCaptcha` في مرة واحدة. غيّر إعدادًا واحدًا، أعد تشغيل الخدمات، واختبر الدخول بحساب Test قبل تعميمه.

### أسماء قواعد البيانات

| SettingName | القيمة المعتادة | وظيفته |
|---|---|---|
| `AccountDB` | `SRO_VT_ACCOUNT` | قاعدة بيانات الحسابات. |
| `ShardDB` | `SRO_VT_SHARD` | قاعدة بيانات الشخصيات والآيتمات. |
| `LogDB` | `SRO_VT_SHARDLOG` | قاعدة الـLog التي تحتوي سجلات مثل `_LogEventItem`. |

هذه القيم ليست أسماء للعرض. يجب أن تطابق أسماء Databases الموجودة على SQL Server حرفيًا. تغييرها يحتاج Restart واختبار اتصال، ولا يتم لمجرد تغيير اسم البراند.

### الـDelays والحد الأدنى للـLevel

| SettingName | وظيفته |
|---|---|
| `ExchangeDelay` | مدة الانتظار بين محاولات Exchange. |
| `ExchangeLevel` | أقل Level يسمح له باستخدام Exchange. |
| `ExitDelay` | مدة تنفيذ Exit. |
| `GlobalDelay` | Cooldown استخدام Global Chat. |
| `GlobalLevel` | أقل Level لاستخدام Global Chat. |
| `GuildInviteDelay` | Cooldown دعوات Guild. |
| `LiveItemDelay` | Cooldown للأوامر أو الـScrolls التي تغير الآيتم Live. |
| `RestartDelay` | مدة تنفيذ Restart. |
| `ReverseDelay` | Cooldown استخدام Reverse. |
| `SHOW_CHAR_INFO_DELAY` | Cooldown طلب عرض معلومات شخصية أخرى. |
| `StallDelay` | Cooldown فتح Stall. |
| `StallLevel` | أقل Level لفتح Stall. |
| `TradePetSpawnDelay` | Cooldown استدعاء Transport الخاص بالـTrade. |
| `UnionInviteDelay` | Cooldown دعوات Union. |

القيم الزمنية في هذه المجموعة تُعامل كثوانٍ في تشغيل الفلتر الحالي. مثال:

```sql
UPDATE KMTGuard.dbo.System_Settings
SET Value = '30'
WHERE SettingName IN ('GlobalDelay', 'ReverseDelay');
```

### أزرار القائمة والـGuide Icons

هذه المفاتيح لا تحذف الميزة أو بياناتها؛ هي تتحكم غالبًا في ظهور الزر أو الـGuide داخل الكلاينت.

| SettingName | العنصر الذي يتحكم فيه |
|---|---|
| `Achievements` | زر Achievements |
| `Changelog` | زر Changelog |
| `DynamicRanking` | زر الرانكات |
| `EventRegister` | زر تسجيل الأحداث |
| `EventSchedule` | زر جدول الأحداث |
| `GrantNameButton` | زر Grant Name |
| `IconManagerButton` | زر إدارة الأيقونات |
| `IconManagerRight` | دعم/إظهار جهة الأيقونة اليمنى |
| `TitleManager` | زر إدارة الـTitles |
| `TitleManagerColor` | ألوان الـTitle |
| `UniqueHistory` | سجل الـUniques |
| `ShowChangelogFirstSpawn` | فتح Changelog عند أول Spawn |
| `ShowGuideAutoEquip` | Guide خاص بـAuto Equip |
| `ShowGuideDailyLogin` | Guide الحضور اليومي |
| `ShowGuideDiscord` | Guide Discord |
| `ShowGuideDropLogs` | Guide Drop Logs |
| `ShowGuideFacebook` | Guide Facebook |
| `ShowGuideItemChest` | Guide Item Chest |
| `ShowGuideKillerAnimation` | Guide Killer Animation |
| `ShowGuideLuckySpin` | Guide Lucky Spin |
| `ShowGuideMacro` | Guide Macro |
| `ShowGuideMapLocation` | Guide Map Location |
| `ShowGuideMenu` | Guide القائمة الرئيسية |
| `ShowGuidePvpChallenge` | Guide PVP Challenge |
| `ShowGuideSpecialOffers` | Guide Special Offers |
| `ShowGuideWebsite` | Guide Website |
| `ShowGuideWebViewer` | Guide Web Viewer |

مثال إخفاء أزرار غير مستخدمة:

```sql
UPDATE KMTGuard.dbo.System_Settings
SET Value = 'False'
WHERE SettingName IN
(
    'Achievements',
    'Changelog',
    'ShowGuideDiscord',
    'ShowGuideFacebook'
);
```

### روابط الكلاينت

| SettingName | وظيفته |
|---|---|
| `WebsiteURL` | رابط الموقع الذي يفتحه زر Website. |
| `FacebookURL` | رابط صفحة Facebook. |
| `DiscordURL` | رابط دعوة/صفحة Discord. |

ضع رابطًا كاملًا يبدأ بـ`https://`. لو الزر غير مستخدم، أخفه من مفتاح الـGuide/الزر المناسب بدل وضع قيمة عشوائية.

```sql
UPDATE KMTGuard.dbo.System_Settings
SET Value = 'https://example.com'
WHERE SettingName = 'WebsiteURL';
```




### إعدادات واجهة الكلاينت

| SettingName | وظيفته |
|---|---|
| `AutoSkillUpdate` | تحديث بيانات/عرض المهارات تلقائيًا. |
| `AutoSort` | زر أو سلوك ترتيب الـInventory تلقائيًا. |
| `AutoStrInt` | ميزة توزيع STR/INT المدعومة. |
| `EmojiSystem` | نظام الإيموجي في الشات. |
| `FixDamageText` | إصلاح/تنسيق عرض أرقام الضرر. |
| `FixNewJobSuit` | توافق الـJob Suit الجديد. |
| `HideOldTitleWhileNewTitle` | إخفاء الـTitle القديم عند تفعيل النظام الجديد. |
| `InsertCommaPrices` | إضافة فواصل للأرقام والأسعار الكبيرة. |
| `ItemComparison` | مقارنة الآيتمات في الواجهة. |
| `MasteryLimit` | الحد المعروض/المطبق للـMastery في الواجهة المدعومة. |
| `Menu-like-maxi` | يبدّل شكل/سلوك القائمة إلى النمط المتوافق مع Maxi؛ قيمة Boolean. |
| `MoveSkillBoard` | السماح بتحريك نافذة Skill Board. |
| `NewAlchemy` | تشغيل واجهة Alchemy الجديدة. |
| `OldAlchemy` | تشغيل واجهة Alchemy القديمة. |
| `PermanentAlchemy` | تشغيل سلوك/نافذة Permanent Alchemy المدعومة. |
| `NewItemMall` | تشغيل Item Mall الجديد. |
| `OldItemMall` | تشغيل Item Mall القديم. |
| `NewJobUI` | تشغيل واجهة Job الجديدة. |
| `NewPartyMatch` | تشغيل Party Match الجديد. |
| `NonClosePTForm` | إبقاء نافذة Party Form مفتوحة حسب تصميم الكلاينت. |
| `OldExpBar` | استخدام شريط EXP القديم. |
| `OldMainPopup` | استخدام الـMain Popup القديم. |
| `PartyMemberViewer` | عرض نافذة أعضاء الـParty المحسنة. |
| `PickupEffect` | مؤثر التقاط الآيتمات. |
| `SecondarySlot` | تشغيل الـSecondary Slot المدعوم. |
| `ServerInfoSkill` | إظهار Server Info من خلال Skill/واجهة النظام. |
| `ServerMaxLevel` | أقصى Level يستخدمه عرض الكلاينت. |
| `ShowGuildInJobMode` | إظهار اسم الـGuild في وضع Job. |
| `UniqueTarget` | تحسين استهداف الـUnique. |
| `WriteCharacterBound` | إظهار وصف Character Bound على الآيتم. |

لا تفعّل نسختين متعارضتين من نفس الواجهة. أمثلة واضحة:

- استخدم `NewAlchemy=True` مع `OldAlchemy=False`، أو العكس.
- استخدم `NewItemMall=True` مع `OldItemMall=False`، أو العكس.
- قيمة `ServerMaxLevel` يجب أن تطابق Cap السيرفر، وليست وسيلة لرفع Cap قاعدة الشارد.

### Captcha بيع الـTrade

| SettingName | وظيفته |
|---|---|
| `EnableTradeSellCaptcha` | تشغيل سؤال التحقق عند بيع Trade Goods. |
| `TradeSellCaptchaMaxAttempts` | أقصى عدد إجابات خاطئة. |
| `TradeSellCaptchaTimeoutSeconds` | المهلة المسموحة للإجابة بالثواني. |

مثال إعداد متوازن:

```sql
UPDATE KMTGuard.dbo.System_Settings
SET Value =
    CASE SettingName
        WHEN 'EnableTradeSellCaptcha'           THEN 'True'
        WHEN 'TradeSellCaptchaMaxAttempts'      THEN '3'
        WHEN 'TradeSellCaptchaTimeoutSeconds'   THEN '60'
    END
WHERE SettingName IN
(
    'EnableTradeSellCaptcha',
    'TradeSellCaptchaMaxAttempts',
    'TradeSellCaptchaTimeoutSeconds'
);
```

### مراجعة سريعة بعد تعديل Settings

```sql
SELECT SettingName, Value, Category
FROM KMTGuard.dbo.System_Settings
WHERE SettingName IN
(
    'HWID_LIMIT',
    'HWID_JOB_LIMIT',
    'IPLimit',
    'EnableOfflineStall',
    'EnablePvpChallenge',
    'EnableLuckySpin'
)
ORDER BY SettingName;
```

بعدها أعد تشغيل خدمات KMTGuard واختبر بحساب لاعب عادي، وليس بحساب GM فقط؛ بعض القيود تستثني الـGM فتظهر لك نتيجة غير حقيقية.

## Procedures مهمة للعميل

هذه أكثر procedures العميل غالبًا سيستخدمها بنفسه أو في سكربتات rewards.

| Procedure | فائدتها | مثال الاستخدام |
|---|---|---|
| `Command_NoticeAll` | إرسال رسالة لكل اللاعبين | `EXEC KMTGuard.dbo.Command_NoticeAll 7, 'Message'` |
| `Command_NoticeByID` | إرسال رسالة للاعب بالـ CharID | `EXEC KMTGuard.dbo.Command_NoticeByID 12345, 3, 'Message'` |
| `Command_NoticeByName` | إرسال رسالة للاعب بالاسم | `EXEC KMTGuard.dbo.Command_NoticeByName 'Player', 3, 'Message'` |
| `Command_DisconnectByID` | فصل لاعب محدد | `EXEC KMTGuard.dbo.Command_DisconnectByID 12345` |
| `Command_DisconnectByName` | فصل لاعب بالاسم | `EXEC KMTGuard.dbo.Command_DisconnectByName 'Player'` |
| `Command_DisconnectAll` | فصل كل اللاعبين | يستخدم بحذر شديد |
| `Command_GetUp` | رفع/إحياء شخصية عند الفلتر | `EXEC KMTGuard.dbo.Command_GetUp 12345` |
| `Item_AddChest` | إضافة آيتم للـ Chest الجديد | يحتاج `ItemRefObjID` |
| `Item_AddChestOld` | نسخة قديمة للتوافق | الأفضل استخدام `Item_AddChest` |
| `Item_GetInfo` | قراءة بيانات آيتم من slot لاعب | مفيد قبل تغيير موديل/جلو |
| `Live_Silk` | إضافة/تغيير Silk live من خلال أوامر الفلتر | مناسب للمكافآت المباشرة |
| `Live_Gold` | إضافة أو خصم Gold | يستخدم مع قيمة واتجاه |
| `Live_AddBuff` | إضافة Buff بالـ Skill CodeName | للاحتفالات/events |
| `Live_RemoveBuff` | إزالة Buff بالـ SkillID | تنظيف بعد event |
| `Live_ChangeItem` | تغيير آيتم في slot إلى آيتم آخر | يستخدم بحذر |
| `Live_UseItem` | استهلاك كمية من آيتم في slot | scrolls/custom items |
| `Live_UseAndChangeItem` | يستهلك آيتم ويغير آيتم آخر | مناسب لـ model/glow scrolls |
| `Teleport_PlayerToTown` | إرسال لاعب للمدينة | عقوبة أو reset |
| `Teleport_AllToTown` | إرسال كل لاعبي world للمدينة | أحداث/صيانة |
| `Teleport_Position` | نقل لاعب لمكان محدد | يحتاج world/region/position |
| `Teleport_PositionFreeze` | نقل لاعب وتجميده مدة | مفيد لبداية events |
| `NPC_Spawn` | Spawn mob/NPC بالـ CodeName | events |
| `NPC_SpawnAtPosition` | Spawn بالـ RefObjID | events |
| `NPC_Kill` | حذف mob/NPC بالـ CodeName | تنظيف event |
| `NPC_KillByWorld` | حذف mob/NPC من world محدد | تنظيف event أدق |
| `Style_AddTitle` | إعطاء title للاعب | قبل اختيار/تفعيل title |
| `Style_UpdateTitle` | تفعيل title للاعب | بالـ CharName16 |
| `Style_RemoveTitle` | إزالة title مفعّل | بالـ CharName16 |
| `Style_AddIcon` | إعطاء icon للاعب | `Side=0` يسار، `Side=1` يمين |
| `Style_UpdateLeftIcon` | تفعيل icon يسار | بالاسم |
| `Style_UpdateRightIcon` | تفعيل icon يمين | بالاسم |
| `Style_RemoveLeftIcon` | إزالة icon يسار | بالاسم |
| `Style_RemoveRightIcon` | إزالة icon يمين | بالاسم |
| `Style_AddTitleColor` | إعطاء لون title | كود لون واسم |
| `Style_UpdateTitleColor` | تفعيل لون title | بالاسم |
| `Style_UpdateNameColor` | تفعيل لون اسم اللاعب | بالاسم |
| `Events.dbo._AutoEventEnqueueStart` | بدء Auto Event حديث | بالـ EventCode |
| `Events.dbo._AutoEventEnqueueStop` | إيقاف الـAuto Event الحالي | يوقف الحدث الحالي |
| `Events.dbo._AutoEventEnqueueReload` | إعادة تحميل إعدادات الأحداث | بعد تعديل config/content/rewards |
| `Event_Attendance` | تسجيل حضور لاعب | غالبًا تستخدمه الواجهة |



### كل `CommandID` داخل `Command_FilterQueue`

| ID | البيانات | ماذا يفعل | الواجهة الآمنة |
|---:|---|---|---|
| `1` | `Data1=CharName16` | يفصل لاعبًا بالاسم | `Command_DisconnectByName` |
| `2` | لا شيء | يفصل كل جلسات اللاعبين | `Command_DisconnectAll`؛ خطر |
| `3` | `Data1=sendall/sendchar`, `Data2=NoticeType`, `Data3=Message`, `Data4=CharName16` عند الإرسال لشخص | يرسل Notice عام أو بالاسم | `Command_NoticeAll` أو `Command_NoticeByName` |
| `4` | `CharID, TitleID` | يضيف Hwan title لقائمة اللاعب داخل الكلاينت | `Style_AddTitle` |
| `5` | `CharID, ColorName, ColorCode, RowID` | يضيف لون title إلى قائمة اللاعب | `Style_AddTitleColor` |
| `6` | `CharID, IconID, Side, RowID` | يضيف icon إلى قائمة اللاعب | `Style_AddIcon` |
| `7` | `CharID, TitleID` | يغيّر Hwan title live إذا اللاعب ليس Berserk | `Style_GrantAndActivateHwanTitle` أو الإجراء المسؤول عن اختيار Hwan |
| `8` | `CharName16, ColorCode` | يفعّل/يحدّث لون title ويبثه للجميع | `Style_UpdateTitleColor` |
| `9` | `CharName16` | يزيل لون title المفعل | `Style_RemoveTitleColor` |
| `10` | `CharName16, IconID` | يفعّل icon يسار | `Style_UpdateLeftIcon` |
| `11` | `CharName16, IconID` | يفعّل icon يمين | `Style_UpdateRightIcon` |
| `12` | `CharName16` | يزيل icon اليسار | `Style_RemoveLeftIcon` |
| `13` | `CharName16` | يزيل icon اليمين | `Style_RemoveRightIcon` |
| `14` | — | غير معرّف في الكود الحالي | لا تستخدمه |
| `15` | `CharName16, TitleID` | يفعّل custom title النصي | `Style_UpdateTitle` |
| `16` | `CharName16` | يزيل custom title النصي | `Style_RemoveTitle` |
| `17` | بيانات صف الـChest | يضيف reward جديد live لقائمة Chest للاعب الأونلاين | `Item_AddChest` |
| `18` | `GateID, State` | مسار Live Teleport قديم، وجسم التنفيذ الحالي معطّل | لا تعتمد على `Live_Teleport` حتى إعادة تفعيل المسار |
| `19` | `RegionID, State` | يفتح/يقفل الهجوم live داخل Region | تستخدمه أنظمة الأحداث |
| `20` | `WorldID, State` | يفتح/يقفل الهجوم live داخل World | `Live_Skill`؛ الاسم قديم والوظيفة الفعلية attack state |
| `21` | `Seconds, WorldID` | يعرض timer مرة واحدة للاعبي World | داخلي للأحداث |
| `22` | `Seconds, RegionID` | يعرض timer مرة واحدة للاعبي Region | داخلي للأحداث |
| `23` | `Seconds, WorldID` | يبدأ/يحدّث timer متابع على World | `Event_AddWorldTimer` |
| `24` | `Seconds, RegionID` | يبدأ/يحدّث timer متابع على Region | `Event_AddRegionTimer` |
| `25` | `Title, WorldID` | يشغّل/يقفل kill counter العام ويصفر قائمته عند الإغلاق | `Event_AddKillCounter` |
| `26` | `WorldID, CharName16, Kill` | يزيد kill counter العام ويبث أفضل 5 | `Event_AddKill` |
| `27` | `CharID, JID, SilkAmount, SilkRank` | يحدّث Silk rank live للاعب | داخلي لنظام الرانك |
| `28` | `CharID, Message, NoticeType` | Notice مباشر للاعب بالـCharID | `Command_NoticeByID` |
| `29` | `CharID, AchievementID, ConditionID, Progress, State` | يحدّث achievement live | `Achievement_Update` |
| `30` | `WorldID` | يشغّل/يقفل team kill counter | `Event_AddTeamKillCounter` |
| `31` | `WorldID, CharName16, Kill, Team` | يزيد team counter ويبث النتيجة | `Event_AddTeamKill` |
| `32` | `WorldID` | يشغّل/يقفل job kill counter | `Event_AddJobKillCounter` |
| `33` | `WorldID, CharName16, Kill, Job/Team` | يزيد job counter ويبث النتيجة | `Event_AddJobKill` |
| `34` | `CharID` | يفصل لاعبًا بالـCharID | `Command_DisconnectByID` |
| `35` | بيانات FTW kill | مسار Fortress counter قديم؛ التنفيذ الحالي معلق | لا تستخدمه |
| `36` | `CharID, RegionID, X, Y, Z, Seconds` | يرسل map ping للاعب | `Command_MapPingByID` |
| `37` | `CharName16, ColorCode` | يفعّل لون اسم ويبثه | `Style_UpdateNameColor` |
| `38` | `CharName16` | يزيل لون الاسم | `Style_RemoveNameColor` |
| `39` | `CharID` | يرسل طلب Get Up إلى GameServer من جلسة اللاعب | `Command_GetUp` في الأنظمة التي تعتمد هذا المسار |
| `40` | أي قيمة غير فارغة | يعيد تحميل ranks وachievement definitions من SQL | أمر صيانة داخلي |
| `41` | `CharID` | Self teleport مع منع التكرار وcooldown | `Teleport_Self` |
| `1000` | لا شيء | يعيد تحميل NPC Unique IDs وRefObj data | داخلي بعد تغيير مراجع NPC |
| `9999` | `MobID` | يسجل unique spawn في الذاكرة ويبث حالته | ينشئه `Hook_UniqueSpawn` |
| `10000` | `MobID, KillerName` | يسجل unique kill ويبث القاتل وdamage meter | ينشئه `Hook_UniqueKill` |

أي رقم آخر لا يملك تنفيذًا حاليًا؛ الفلتر سيعلّم الصف كمكتمل من غير فعل. لا
تخترع CommandID جديدًا من SQL.

### كل `CommandID` داخل `Command_PlannedQueue`

| ID | البيانات | الوظيفة |
|---:|---|---|
| `1` | `Data1=CharName16` | فصل لاعب في `DateToExecute` |
| `2` | لا شيء | فصل الجميع في الموعد؛ خطر |
| `3` | نفس تنسيق Notice في Filter Queue | Notice عام أو لشخص في الموعد |
| `100` | `Data1` استدعاء SQL مسموح | ينفذ فقط `EXEC/EXECUTE` لاسم Procedure مطابق لقائمة السماح داخل الفلتر؛ ليس SQL حرًا |

استخدم `Status=1` ووقتًا محليًا متوافقًا مع ساعة جهاز الفلتر. أمر `100`
مقصود للجدولة التي أنشأها النظام؛ لا تضع فيه `UPDATE/DELETE/INSERT` لأنها
ستُرفض.

### كل `Action_ID` داخل `Command_GameServerQueue`

كل الأوامر التالية تُغلّف داخل packet داخلي `0x8888`، ما عدا تحديث Honor
Rank (`200`) الذي تشغله إضافة الـShardManager مباشرة ثم تبث packet التحديث
الرسمي لكل GameServers.

| ID | البيانات بالترتيب من `Data1` | ماذا يفعل | Procedure المفضلة |
|---:|---|---|---|
| `1` | `CharID, GrantName` | تغيير grant/name للشخصية | `Command_ChangeName` |
| `2` | `MobID, WorldID, RegionID, X, Y, Z, Radius` | Spawn mob في مكان | `NPC_Spawn` أو `NPC_SpawnAtPosition` |
| `3` | `CharID, MobID` | Spawn mob قرب لاعب | `NPC_SpawnNearPlayer` |
| `4` | `MobRefObjID` | قتل/إزالة mob من النوع المحدد | `NPC_Kill` |
| `5` | `WorldID, MobRefObjID` | قتل/إزالة mob داخل World محدد | `NPC_KillByWorld` |
| `6` | `CharID, SkillID` | إضافة buff بدون limit | `Live_AddBuffNoLimit` |
| `7` | `CharID, SkillID` | إزالة buff | `Live_RemoveBuff` |
| `8` | `CharID, SkillCodeName` | إضافة buff بالـCodeName | `Live_AddBuff` |
| `9` | — | غير معرّف | لا تستخدمه |
| `10` | `CharID` | To Town للاعب؛ مسار native قديم | استخدم `Teleport_PlayerToTown` الحديث |
| `11` | `WorldID` | To Town لكل لاعبي World | `Teleport_AllToTown` |
| `12` | `PetUniqueID, PetSkillID` | تشغيل skill لحيوان Fellow | داخلي لنظام Fellow |
| `13` | `CharID, CapeID` | تغيير cape live | `Live_Cape` |
| `14` | `CharID, WorldID, RegionID, X, Y, Z` | نقل لمكان؛ مسار native قديم | استخدم `Teleport_Position` الحديث |
| `15` | `CharID` | Get Up | `Command_GetUp` |
| `16` | `CharID, ExpRate` | إرسال قيمة EXP rate live | داخلي/قديم؛ لا توجد Procedure عامة حالية |
| `17` | `CharID, Slot, CodeName128` | تغيير الآيتم في slot | `Live_ChangeItem` |
| `18` | `CharID, Slot, Amount` | استهلاك كمية من آيتم | `Live_UseItem` |
| `19` | `CharID, MutateSlot, CodeName128, ConsumeSlot, Amount` | يغيّر آيتم ويستهلك آيتمًا آخر | `Live_UseAndChangeItem` |
| `20` | `CharID, SilkOwn, SilkGift, SilkPoint` | يرسل أرصدة Silk الجديدة live | `Live_Silk` أو `_addsilklive` |
| `21` | `CharID, Gold, AddOrRemove` | يضيف/يخصم Gold live | `Live_Gold` |
| `22` | `WorldID, LayerID` | يرسل حالة/طبقة World للـGameServer | داخلي للأحداث |
| `23` | `CharID, WorldID, RegionID, X, Y, Z` | Get Up في مكان | `Command_GetUpAtPosition` |
| `24` | `CharID, WorldID, RegionID, X, Y, Z` | نقل إلى Safe Zone | `Teleport_SafeZone` |
| `38` | `Enabled, WorldID, RegionID, Team1MobID, Team1Cape, Team2MobID, Team2Cape` | قاعدة ملكية أبراج Defend The Tower | ينشئه نظام Competitive Events |
| `39` | `Enabled, WorldID, RegionID` | تفعيل/إلغاء free-for-all arena rule | ينشئه نظام Competitive Events |
| `131` | `CharID, ItemID, Plus, Slot, AdvLevel` | تحديث alchemy item live | مسار legacy داخل `Hook_AlchemySuccess` وهو معلق حاليًا |
| `200` | `RequestedBy` | يشغّل تحديث Training Camp Honor Rank ثم يبث refresh لكل GameServers | `Command_RefreshHonorRank` |

## الفرق بين Procedure تستخدمها وProcedure تعدّلها

هناك نوعان مهمان لصاحب السيرفر:

1. Procedure تشغلها بأمر `EXEC`، مثل `Item_AddChest` أو `Command_NoticeAll`.
2. Hook يناديه الفلتر تلقائيًا، وأنت تضيف داخله منطق السيرفر، مثل مكافأة عند قتل Unique أو Notice عند نجاح Plus معين.

لا تشغّل الـHook يدويًا على Production لمجرد التجربة؛ تعديل جسمه هو الاستخدام الطبيعي. إجراءات حفظ واجهة اللاعب يناديها الكلاينت، وليس لصاحب السيرفر إعداد مفيد بداخلها، لذلك ليست جزءًا من دليل التشغيل.

| المجموعة | أهميتها لصاحب السيرفر |
|---|---|
| `Hook_*` | مهمة جدًا للتخصيص. أضف بداخلها شروطًا ومكافآت مع الحفاظ على الاسم والـParameters. |
| `Achievement_*` | مهمة لو تستخدم نظام Achievements أو تريد ربط تقدم مخصص بأحداث السيرفر. |
| `Event_Add*` | مهمة لبناء Counters وTimers داخل Events مخصصة. |
| `_CustomTrade*` | مهمة لتخصيص حماية ومكافآت الـCustom Trade. |
| `Log_*` | Procedures القراءة مهمة للتقارير؛ إجراءات التسجيل يترك استدعاؤها للفلتر. |
| إجراءات حفظ الواجهة | حفظ داخلي لحالة اللاعب، ولا يحتاج العميل تعديلها. |

## دليل الـHooks والتخصيص

### قاعدة لا تتغير عند تعديل أي Hook

خذ نسخة من تعريف الـProcedure أولًا:

```sql
SELECT OBJECT_DEFINITION
(
    OBJECT_ID('KMTGuard.dbo.Hook_UniqueKill')
) AS CurrentDefinition;
```

استخدم `ALTER PROCEDURE` مع **نفس الاسم ونفس المعاملات بنفس أنواعها**. أضف كودك بعد `SET NOCOUNT ON`، ولا تحذف منطق KMTGuard الموجود أصلًا. جرّب على Test Database، ثم نفّذ على Production بعد Backup.

### كتالوج الـHooks التي يقدر صاحب السيرفر يستفيد منها

| Hook | الفلتر يناديه إمتى؟ | أفكار استخدام صحيحة |
|---|---|---|
| `Hook_AlchemySuccess` | عند نجاح عملية Alchemy | Notice عند +12، Broadcast عند رقم قياسي، Reward مشروط |
| `Hook_AutoEquip` | عند تشغيل/تنفيذ Auto Equip للشخصية | تجهيز مكافأة البداية أو تسجيل الاستخدام |
| `Hook_CharacterGetUp` | عند قيام أو إحياء الشخصية | تطبيق قواعد Event أو إعادة Cape/Buff |
| `Hook_CharacterKill` | عند قتل لاعب للاعب | Kill Ranking، Event Points، Anti-farm، Job Rewards |
| `Hook_EventCancel` | عند إلغاء اللاعب تسجيله في Event | إزالة تسجيل مؤقت أو إعادة رسوم |
| `Hook_EventRegister` | عند تسجيل اللاعب في Event | شروط Level/HWID، رسوم تسجيل، Notice تأكيد |
| `Hook_GameServerStart` | عند بداية GameServer | تهيئة بيانات مرتبطة بالسيرفر أو إرسال Log إداري |
| `Hook_ItemMallBuy` | بعد شراء آيتم من Item Mall | Purchase Log، Bonus حسب الإنفاق، عرض لاحق |
| `Hook_NPCBuy` | عند شراء آيتم من NPC | Scrolls مخصصة، Limits، Rewards مرتبطة بآيتم |
| `Hook_PartyJoin` | عند دخول Party | منع تكوينات معينة في Event أو تحديث Team |
| `Hook_PartyMatchCreate` | عند إنشاء Party Matching | مراقبة العنوان/المنطقة أو تسجيل النشاط |
| `Hook_SelectScroll` | عند استخدام Select/Change Scroll | تغيير Model أو Glow من Slot محدد |
| `Hook_StallCreate` | عند فتح Stall | Log، منع كلمات، شروط Region/Level |
| `Hook_Teleport` | قبل تنفيذ Teleport | السماح أو المنع من خلال `@CanTeleport OUTPUT` |
| `Hook_UniqueKill` | عند قتل Unique مع اسم القاتل | Reward وBroadcast وترتيب |
| `Hook_UniqueSpawn` | عند Spawn Unique | Notice عام أو بدء Timer |

### مثال 1: مكافأة عند قتل Unique

الفكرة هنا: الفلتر يرسل `RefObjID` واسم القاتل إلى `Hook_UniqueKill`. الكود يحوّل الاسم إلى `CharID`، ويتأكد أن الـUnique المطلوب هو الذي مات، ثم يرسل Reward وNotice.

```sql
-- هذا Block يضاف داخل Hook_UniqueKill الحالي قبل END الأخيرة.
DECLARE @RewardCharID int;
DECLARE @RewardItemID int;

SELECT @RewardCharID = CharID
FROM SRO_VT_SHARD.dbo._Char
WHERE CharName16 = @KillerCharName;

-- استبدل 1954 بالـRefObjID الحقيقي للـUnique المطلوب.
IF @RewardCharID IS NOT NULL AND @RefObjID = 1954
BEGIN
    SELECT @RewardItemID = ID
    FROM SRO_VT_SHARD.dbo._RefObjCommon
    WHERE CodeName128 = 'ITEM_MALL_GLOBAL_CHATTING';

    IF @RewardItemID IS NOT NULL
    BEGIN
        EXEC KMTGuard.dbo.Item_AddChest
            @CharID = @RewardCharID,
            @ItemRefObjID = @RewardItemID,
            @Quantity = 5,
            @From = 'Unique Reward',
            @Plus = 0;
    END;

    EXEC KMTGuard.dbo.Command_NoticeAll
        @NoticeType = 7,
        @Notice = @KillerCharName + ' killed the event unique.';
END;
```

في التطبيق الحقيقي لا تستبدل التعريف كاملًا بهذا المثال بصورة عمياء. انسخ جسم الـHook الحالي، ثم أضف شرطك داخله حتى لا تفقد وظائف موجودة.

### مثال 2: Broadcast عند نجاح Plus مرتفع

```sql
-- هذا Block يضاف داخل Hook_AlchemySuccess الحالي قبل END الأخيرة.
IF @Plus >= 12
BEGIN
    EXEC KMTGuard.dbo.Command_NoticeAll
        @NoticeType = 7,
        @Notice = @CharName + ' succeeded in alchemy +' +
                  CONVERT(varchar(3), @Plus) + '.';
END;
```

### مثال 3: منع Teleport بشرط مخصص

`Hook_Teleport` مختلف لأن نتيجة القرار ترجع للفلتر في `@CanTeleport OUTPUT`. ابدأ بالسماح، ثم غيّرها إلى `0` عند تحقق سبب المنع:

```sql
-- هذا Block يندمج داخل Hook_Teleport الحالي.
-- لا تكرر SET إذا كان التعريف الأصلي يعيّن النتيجة بالفعل.
SET @CanTeleport = 1;

-- مثال: Gate رقم 1001 يحتاج Level 100 على الأقل.
IF @RefTeleportID = 1001
   AND EXISTS
   (
       SELECT 1
       FROM SRO_VT_SHARD.dbo._Char
       WHERE CharID = @CharID
         AND CurLevel < 100
   )
BEGIN
    SET @CanTeleport = 0;

    EXEC KMTGuard.dbo.Command_NoticeByID
        @CharID = @CharID,
        @NoticeType = 3,
        @Notice = 'You need level 100 to use this teleport.';
END;
```

أخطر خطأ هنا هو نسيان تعيين `@CanTeleport`. لازم كل مسار داخل الـProcedure ينتهي بقرار واضح.

### مثال 4: تسجيل Kill Ranking بدون احتساب قتل النفس

```sql
-- أضف هذا الجزء داخل Hook_CharacterKill الموجود، ولا تحذف جسمه الأصلي.
IF @KillerCharID <> @DeadCharID
BEGIN
    INSERT INTO KMTGuard.dbo.Log_PlayerKills
    (
        WorldID,
        RegionID,
        KillerCharID,
        KillerCharName,
        KillerGuildID,
        KillerGuildName,
        DeadCharID,
        DeadCharName,
        DeadGuildID,
        DeadGuildName
    )
    VALUES
    (
        @WorldID,
        @RegionID,
        @KillerCharID,
        @KillerCharName,
        @KillerGuildID,
        @KillerGuildName,
        @DeadCharID,
        @DeadCharName,
        @DeadGuildID,
        @DeadGuildName
    );
END;
```

قبل استخدام المثال، اعرض أعمدة `Log_PlayerKills` في نسختك؛ بعض الإصدارات تضيف وقت التسجيل تلقائيًا وبعضها يحتاج أعمدة إضافية. والأفضل عدم تكرار `INSERT` إذا كان الـHook الحالي يسجل الـKill بالفعل.

## طرق عمل كاملة خطوة بخطوة

هذا الجزء هو أهم جزء للعميل. هنا لا نشرح اسم الجدول فقط، بل نشرح كيف تنفذ مهمة كاملة من البداية للنهاية.

### إنشاء رانك جديد

نظام الرانكات في KMTGuard مقسم إلى:

| الجزء | معناه |
|---|---|
| `Rank_Categories` | أسماء أقسام الرانك التي تظهر للعميل |
| `Rank_Data01` إلى `Rank_Data09` | بيانات كل رانك: اللاعب، الجيلد، النقاط |

كل جدول من `Rank_Data01` إلى `Rank_Data09` يمثل صفحة/قسم رانك. مثال: لو `Rank_Categories.ID = 1` غالبًا يقرأ من `Rank_Data01`.

#### الخطوة 1: اختار رقم الرانك

شوف الأقسام الحالية:

```sql
SELECT *
FROM KMTGuard.dbo.Rank_Categories
ORDER BY ID;
```

لو عايز تستخدم الرانك رقم 5 مثلًا، يبقى هتتعامل مع:

```text
Rank_Categories.ID = 5
Rank_Data05
```

#### الخطوة 2: أضف أو عدل اسم الرانك

```sql
IF EXISTS (SELECT 1 FROM KMTGuard.dbo.Rank_Categories WHERE ID = 5)
BEGIN
    UPDATE KMTGuard.dbo.Rank_Categories
    SET Active = 1,
        Category = 'Top Job Kills'
    WHERE ID = 5;
END
ELSE
BEGIN
    INSERT INTO KMTGuard.dbo.Rank_Categories (ID, Active, Category)
    VALUES (5, 1, 'Top Job Kills');
END;
```

#### الخطوة 3: امسح بيانات الرانك القديم لهذا القسم

```sql
TRUNCATE TABLE KMTGuard.dbo.Rank_Data05;
```

استخدم `TRUNCATE` فقط على جدول الرانك المختار، وليس على كل جداول الرانكات.

#### الخطوة 4: املأ الرانك من مصدر البيانات

مثال رانك حسب عدد kills من جدول `Log_PlayerKills`:

```sql
INSERT INTO KMTGuard.dbo.Rank_Data05 (CharID, CharName16, GuildName, Point)
SELECT TOP (100)
    KillerCharID,
    KillerCharName,
    ISNULL(KillerGuildName, ''),
    COUNT(*) AS Point
FROM KMTGuard.dbo.Log_PlayerKills
WHERE KillerCharID IS NOT NULL
GROUP BY KillerCharID, KillerCharName, KillerGuildName
ORDER BY COUNT(*) DESC;
```

مثال رانك حسب Silk history:

```sql
TRUNCATE TABLE KMTGuard.dbo.Rank_Data06;

INSERT INTO KMTGuard.dbo.Rank_Data06 (CharID, CharName16, GuildName, Point)
SELECT TOP (100)
    C.CharID,
    C.CharName16,
    ISNULL(G.Name, ''),
    R.SilkHistory
FROM KMTGuard.dbo.Rank_Silk R
INNER JOIN SRO_VT_SHARD.dbo._Char C
    ON C.CharID = R.CharID
LEFT JOIN SRO_VT_SHARD.dbo._Guild G
    ON G.ID = C.GuildID
ORDER BY R.SilkHistory DESC;
```

#### الخطوة 5: جدولة تحديث الرانك

لو عايز الرانك يتحدث كل يوم الساعة 00:05، استخدم `System_Schedule`:

```sql
CREATE OR ALTER PROCEDURE KMTGuard.dbo.UpdateTopJobKillsRank
AS
BEGIN
    SET NOCOUNT ON;

    TRUNCATE TABLE KMTGuard.dbo.Rank_Data05;

    INSERT INTO KMTGuard.dbo.Rank_Data05 (CharID, CharName16, GuildName, Point)
    SELECT TOP (100)
        KillerCharID,
        KillerCharName,
        ISNULL(KillerGuildName, ''),
        COUNT(*)
    FROM KMTGuard.dbo.Log_PlayerKills
    WHERE KillerCharID IS NOT NULL
    GROUP BY KillerCharID, KillerCharName, KillerGuildName
    ORDER BY COUNT(*) DESC;
END;

INSERT INTO KMTGuard.dbo.System_Schedule
(
    Name,
    Query,
    ScheduledDate,
    Time,
    RepeatType,
    RepeatDayOfWeek,
    IsEnabled
)
VALUES
(
    N'Update Top Job Kills Rank',
    N'EXEC dbo.UpdateTopJobKillsRank',
    NULL,
    '00:05',
    N'Daily',
    NULL,
    1
);
```

الـ scheduler يقبل استدعاء stored procedure واحدة فقط بصيغة `EXEC`. القيم
الافتراضية تسمح للـ procedure بالعمل لمدة ساعتين
(`ExecutionTimeoutSeconds = 7200`) وتسمح بتعويض الموعد خلال خمس دقائق
(`CatchUpWindowSeconds = 300`). يمكن تعديل القيم لكل job مباشرة من
`System_Schedule`.

### إضافة آيتم جديد في Item Mall

الجداول المهمة:

| Table | وظيفته |
|---|---|
| `Mall_Items` | الآيتمات العامة في المول |
| `Mall_Avatars` | الآفاتارز |
| `Mall_Categories` | الأقسام |

#### الخطوة 1: هات ItemID من الشارد

```sql
SELECT ID, CodeName128
FROM SRO_VT_SHARD.dbo._RefObjCommon
WHERE CodeName128 = 'ITEM_MALL_GLOBAL_CHATTING';
```

#### الخطوة 2: أضف الآيتم للمول

```sql
INSERT INTO KMTGuard.dbo.Mall_Items
(
    Service,
    CategoryName,
    Type,
    CodeName128,
    ItemID,
    ItemCount,
    Silk,
    ShowInNewBest,
    ItemIndex,
    ShowInNewBestIndex
)
VALUES
(
    1,
    'Scrolls',
    0,
    'ITEM_MALL_GLOBAL_CHATTING',
    3851,
    11,
    50,
    1,
    1,
    1
);
```

لو عايز توقفه من غير حذف:

```sql
UPDATE KMTGuard.dbo.Mall_Items
SET Service = 0
WHERE CodeName128 = 'ITEM_MALL_GLOBAL_CHATTING';
```

### عمل Special Offer

الجداول:

| Table | وظيفته |
|---|---|
| `Offer_List` | تعريف العروض |
| `Offer_PurchaseLog` | سجل الشراء |

مثال عرض آيتم بسعر مخفض:

```sql
INSERT INTO KMTGuard.dbo.Offer_List
(
    Service,
    SortOrder,
    Title,
    ItemID,
    ItemCount,
    CodeName128,
    MainPrice,
    SalePrice,
    PaymentType,
    PreviewMode,
    PreviewRefObjID,
    PreviewImagePath,
    StartDate,
    EndDate
)
VALUES
(
    1,
    1,
    N'Global Chatting Offer',
    3851,
    11,
    'ITEM_MALL_GLOBAL_CHATTING',
    100,
    50,
    0,
    0,
    3851,
    NULL,
    GETDATE(),
    DATEADD(day, 7, GETDATE())
);
```

`PaymentType` يعتمد على تصميم النظام، لكن غالبًا:

| القيمة | معناها المتوقع |
|---:|---|
| `0` | Silk |
| `1` | Gift Silk أو عملة بديلة حسب إعداد النظام |

### إعداد Lucky Spin

الجداول:

| Table | وظيفته |
|---|---|
| `LuckySpin_Rewards` | الجوائز واحتمالاتها |
| `LuckySpin_Log` | سجل اللفات والفائزين |

إضافة جائزة:

```sql
INSERT INTO KMTGuard.dbo.LuckySpin_Rewards
(
    ItemID,
    Amount,
    Rate,
    IsActive,
    CreatedAt
)
VALUES
(
    3851,
    11,
    100,
    1,
    SYSDATETIME()
);
```

تعطيل جائزة:

```sql
UPDATE KMTGuard.dbo.LuckySpin_Rewards
SET IsActive = 0,
    UpdatedAt = SYSDATETIME()
WHERE ID = 1;
```

قراءة آخر الفائزين:

```sql
SELECT TOP (50) *
FROM KMTGuard.dbo.LuckySpin_Log
ORDER BY CreatedAt DESC;
```

### إعداد Attendance Rewards

الجداول:

| Table | وظيفته |
|---|---|
| `Attendance_Rewards` | جائزة كل يوم |
| `Attendance_Players` | تقدم اللاعب |
| `Attendance_RewardLog` | هل استلم الجائزة أم لا |

إضافة جائزة اليوم الأول:

```sql
INSERT INTO KMTGuard.dbo.Attendance_Rewards
(
    ItemID,
    ItemCodeName128,
    ItemCount,
    DayCount
)
VALUES
(
    3851,
    'ITEM_MALL_GLOBAL_CHATTING',
    5,
    1
);
```

تشخيص لاعب:

```sql
SELECT *
FROM KMTGuard.dbo.Attendance_Players
WHERE CharID = 12345;

SELECT *
FROM KMTGuard.dbo.Attendance_RewardLog
WHERE CharID = 12345
ORDER BY ID DESC;
```

### إضافة Title جديد

الجداول:

| Table | وظيفته |
|---|---|
| `Style_Titles` | قائمة الألقاب |
| `Style_PlayerTitles` | الألقاب المملوكة |
| `Style_ActiveTitles` | اللقب المفعل |

#### تعريف title في النظام

```sql
INSERT INTO KMTGuard.dbo.Style_Titles (TitleID, TitleName)
VALUES (10, 'Legend');
```

#### إعطاء title للاعب

```sql
EXEC KMTGuard.dbo.Style_AddTitle
    @CharID = 12345,
    @TitleID = 10;
```

#### تفعيل title للاعب

```sql
EXEC KMTGuard.dbo.Style_UpdateTitle
    @CharName16 = 'PlayerName',
    @TitleID = 10;
```

#### إزالة title المفعل

```sql
EXEC KMTGuard.dbo.Style_RemoveTitle
    @CharName16 = 'PlayerName';
```

### إضافة Icon جديد

الجداول:

| Table | وظيفته |
|---|---|
| `Style_IconFiles` | تعريف ID ومسار الأيقونة في media |
| `Style_PlayerIcons` | الأيقونات المملوكة |
| `Style_ActiveLeftIcons` | الأيقونة اليسار المفعلة |
| `Style_ActiveRightIcons` | الأيقونة اليمين المفعلة |

#### تعريف icon

```sql
INSERT INTO KMTGuard.dbo.Style_IconFiles (IconID, MediaPath)
VALUES (50, 'clientlibrary\\rudiments\\custom_icon.ddj');
```

#### إعطاء icon للاعب

```sql
EXEC KMTGuard.dbo.Style_AddIcon
    @CharID = 12345,
    @IconID = 50,
    @Side = 0;
```

`Side = 0` يعني يسار، و`Side = 1` يعني يمين.

#### تفعيل icon

```sql
EXEC KMTGuard.dbo.Style_UpdateLeftIcon
    @CharName16 = 'PlayerName',
    @IconID = 50;
```

### إضافة ألوان Title أو Name

إعطاء لون title للاعب:

```sql
EXEC KMTGuard.dbo.Style_AddTitleColor
    @CharID = 12345,
    @ColorCode = '#FFCC00',
    @ColorName = 'Gold';
```

تفعيل لون title:

```sql
EXEC KMTGuard.dbo.Style_UpdateTitleColor
    @CharName16 = 'PlayerName',
    @ColorCode = '#FFCC00';
```

تفعيل لون الاسم:

```sql
EXEC KMTGuard.dbo.Style_UpdateNameColor
    @CharName16 = 'PlayerName',
    @ColorCode = '#00CCFF';
```

### إضافة مكافأة Chest من CodeName

استخدم هذا القالب في أي event أو scroll:

```sql
DECLARE @CharID int = 12345;
DECLARE @ItemCodeName varchar(128) = 'ITEM_MALL_GLOBAL_CHATTING';
DECLARE @ItemID int;

SELECT @ItemID = ID
FROM SRO_VT_SHARD.dbo._RefObjCommon
WHERE CodeName128 = @ItemCodeName
  AND Service = 1;

IF (@ItemID IS NULL)
BEGIN
    RAISERROR('Item CodeName was not found in _RefObjCommon.', 16, 1);
    RETURN;
END;

EXEC KMTGuard.dbo.Item_AddChest
    @CharID = @CharID,
    @ItemRefObjID = @ItemID,
    @Quantity = 10,
    @From = 'EventReward',
    @Plus = 0;
```

### عمل Scroll يعطي جوائز

استخدام Scroll يظهر داخل Procedure الشارد لوج `dbo._AddLogItem`. اسم Log Database في هذا الدليل هو `SRO_VT_SHARDLOG`، لكن بعض السيرفرات تسميها `SRO_VT_LOG`; راجع قيمة `LogDB` في `System_Settings` قبل التعديل.

خذ نسخة من التعريف الحالي أولًا:

```sql
USE SRO_VT_SHARDLOG
GO

SELECT OBJECT_DEFINITION
(
    OBJECT_ID('dbo._AddLogItem')
) AS CurrentDefinition;
GO
```

ثم أضف الـBlock التالي داخل `_AddLogItem` الحالي قبل `END` الأخيرة، مع الإبقاء على الـParameters وكل منطق التسجيل الأصلي:

```sql
-- 43999 هو RefObjID الخاص بالـScroll في هذا المثال.
-- Operation 41 تعني مسار استخدام الآيتم في هذا النظام.
IF (@Operation = 41 AND @ItemRefID = 43999)
BEGIN
    DECLARE @ItemID int;

    SELECT @ItemID = ID
    FROM SRO_VT_SHARD.dbo._RefObjCommon
    WHERE CodeName128 = 'ITEM_MALL_GLOBAL_CHATTING'
      AND Service = 1;

    IF @ItemID IS NOT NULL
    BEGIN
        EXEC KMTGuard.dbo.Item_AddChest
            @CharID = @CharID,
            @ItemRefObjID = @ItemID,
            @Quantity = 10,
            @From = 'CustomRewardScroll',
            @Plus = 0;
    END;

    EXEC KMTGuard.dbo.Command_NoticeByID
        @CharID = @CharID,
        @NoticeType = 3,
        @Notice = 'Reward added to your chest.';
END
```

لا تنشئ Procedure جديدة وتفترض أنها ستعمل تلقائيًا؛ الفلتر/GameServer ينادي `_AddLogItem` المعروفة، ولذلك كود الـScroll يجب أن يكون داخل نقطة الربط الموجودة أو يتم استدعاؤه منها صراحة.

### تشغيل وإدارة Auto Events

نظام الأحداث الحديث يخزن إعداداته في Database مستقلة اسمها `Events`. لا تضع محتوى الأحداث الحديثة داخل `KMTGuard`، ولا تستخدم طوابير الأحداث القديمة.

#### الأحداث النصية والتلقائية

الأكواد المدعومة:

```text
Retype
Trivia
FirstType
Math
LongestOnline
LuckyStaller
LuckyStall
LuckyParty
LuckyGlobal
Alchemy
```

الجداول الأساسية:

| Table | وظيفته |
|---|---|
| `Events.dbo._AutoEventConfig` | تشغيل كل Event وعدد الـRounds والمدد والحماية |
| `Events.dbo._AutoEventRoundContent` | الأسئلة والنصوص المستخدمة |
| `Events.dbo._AutoEventReward` | جوائز الفائزين |
| `Events.dbo._AutoEventRun` | كل مرة تم فيها تشغيل Event |
| `Events.dbo._AutoEventRound` | تفاصيل الـRounds داخل كل تشغيل |
| `Events.dbo._AutoEventWinnerLog` | سجل الفائز والدليل والمكافأة |

تشغيل event:

```sql
EXEC Events.dbo._AutoEventEnqueueStart
    @EventCode = N'Trivia',
    @RequestedBy = N'Admin';
```

إيقاف event:

```sql
EXEC Events.dbo._AutoEventEnqueueStop
    @RequestedBy = N'Admin';
```

إعادة تحميل الإعدادات:

```sql
EXEC Events.dbo._AutoEventEnqueueReload
    @RequestedBy = N'Admin';
```

إعداد Event: ثلاث Rounds، دقيقة لكل Round، وفائز واحد فقط لكل HWID في نفس التشغيل:

```sql
UPDATE Events.dbo._AutoEventConfig
SET Enabled = 1,
    RoundCount = 3,
    StartDelaySeconds = 60,
    RoundDurationSeconds = 60,
    InterRoundDelaySeconds = 10,
    MinLevel = 80,
    HwidLimit = 1,
    RequireHwid = 1,
    UniqueWinnerPerRun = 1,
    AnswerCooldownMs = 750,
    UpdatedAtUtc = SYSUTCDATETIME()
WHERE EventCode = N'Trivia';

EXEC Events.dbo._AutoEventEnqueueReload N'Admin';
```

إضافة سؤال Trivia:

```sql
INSERT INTO Events.dbo._AutoEventRoundContent
(
    EventCode,
    IsActive,
    Prompt,
    Answer,
    Weight,
    CreatedAtUtc
)
VALUES
(
    N'Trivia',
    1,
    N'اكتب اسم عاصمة الصين داخل اللعبة بالإنجليزية.',
    N'Jangan',
    10,
    SYSUTCDATETIME()
);

EXEC Events.dbo._AutoEventEnqueueReload N'Admin';
```

إضافة reward للمركز الأول:

```sql
INSERT INTO Events.dbo._AutoEventReward
(
    EventCode,
    Placement,
    RewardType,
    Amount,
    ItemCodeName128,
    ItemID,
    ItemCount,
    Plus,
    IsActive,
    CreatedAtUtc
)
VALUES
(
    N'Trivia',
    1,
    N'ItemChest',
    0,
    'ITEM_MALL_GLOBAL_CHATTING',
    3851,
    10,
    0,
    1,
    SYSUTCDATETIME()
);

EXEC Events.dbo._AutoEventEnqueueReload N'Admin';
```

أنواع المكافآت الصحيحة:

| RewardType | معناه |
|---|---|
| `SilkOwn` | Silk عادي |
| `SilkGift` | Gift Silk |
| `SilkPoint` | Silk Point |
| `Gold` | Gold |
| `ItemChest` | آيتم يصل إلى Chest |

عطّل السؤال أو الجائزة بدل حذفها، حتى يظل عندك سجل واضح:

```sql
UPDATE Events.dbo._AutoEventReward
SET IsActive = 0
WHERE RewardID = 10;
```

#### Survival Solo وSurvival Party

| النظام | Config | Rewards | Schedule |
|---|---|---|---|
| Solo | `Events.dbo._SurvivalSoloConfig` | `Events.dbo._SurvivalSoloReward` | `Events.dbo._SurvivalSoloSchedule` |
| Party | `Events.dbo._SurvivalPartyConfig` | `Events.dbo._SurvivalPartyReward` | `Events.dbo._SurvivalPartySchedule` |

مثال إعداد Survival Solo:

```sql
UPDATE Events.dbo._SurvivalSoloConfig
SET Enabled = 1,
    StartDelaySeconds = 60,
    RegistrationSeconds = 120,
    FightSeconds = 600,
    MinLevel = 100,
    HwidLimit = 1,
    RequireHwid = 1,
    MaxPlayers = 100,
    ArenaWorldID = 107,
    ArenaRegionID = 25580,
    ArenaX = 500,
    ArenaY = 0,
    ArenaZ = 500,
    UpdatedAtUtc = SYSUTCDATETIME()
WHERE EventCode = N'SSOLO';
```

إضافة ميعاد يومي الساعة 9 مساءً:

```sql
INSERT INTO Events.dbo._SurvivalSoloSchedule
(
    StartTime,
    DaysMask,
    IsActive,
    UpdatedAtUtc
)
VALUES
(
    '21:00:00',
    127,
    1,
    SYSUTCDATETIME()
);
```

`DaysMask = 127` يعني كل أيام الأسبوع. لا تكرر نفس الموعد أكثر من مرة. راجع الموجود أولًا:

```sql
SELECT *
FROM Events.dbo._SurvivalSoloSchedule
ORDER BY StartTime;
```

#### الأحداث التنافسية

| EventCode | EventMode | الحدث |
|---|---|---|
| `LMS` | `LastManStanding` | آخر لاعب حي يفوز |
| `MADNESS` | `MadnessSolo` | تجميع Kills فردية مع Anti-farm limits |
| `DTT` | `DefendTower` | فريقان ودفاع عن الأبراج |

إعداد LMS كمثال:

```sql
UPDATE Events.dbo._CompetitiveEventConfig
SET Enabled = 1,
    RegistrationSeconds = 180,
    PrepareSeconds = 20,
    FightSeconds = 600,
    MinPlayers = 5,
    MaxPlayers = 100,
    MinLevel = 100,
    HwidLimit = 1,
    RequireHwid = 1,
    RequireNoParty = 1,
    ArenaWorldID = 107,
    ArenaRegionID = 25580,
    ArenaX = 500,
    ArenaY = 0,
    ArenaZ = 500,
    UpdatedAtUtc = SYSUTCDATETIME()
WHERE EventCode = N'LMS';
```

جائزة المركز الأول:

```sql
INSERT INTO Events.dbo._CompetitiveEventReward
(
    EventCode,
    Placement,
    RewardType,
    Amount,
    ItemCodeName128,
    ItemCount,
    Plus,
    IsActive
)
VALUES
(
    N'LMS',
    1,
    N'SilkOwn',
    100,
    NULL,
    1,
    0,
    1
);
```

جدولة LMS يوميًا الساعة 10 مساءً:

```sql
INSERT INTO Events.dbo._CompetitiveEventSchedule
(
    EventCode,
    StartTime,
    DaysMask,
    IsActive,
    UpdatedAtUtc
)
VALUES
(
    N'LMS',
    '22:00:00',
    127,
    1,
    SYSUTCDATETIME()
);
```

قبل تشغيل أي Event Arena، تأكد أن `WorldID` و`RegionID` والإحداثيات صحيحة، وأن Region Rules لا تمنع الحركة أو الـPVP المطلوب. اختبر التسجيل والدخول والخروج والمكافأة بحسابين Test على الأقل.

### إعداد Region Rules

الجداول المهمة:

| Table | وظيفته |
|---|---|
| `Security_RegionFeatures` | فتح/قفل خصائص كاملة في region |
| `Security_HideNameRegions` | إخفاء أسماء اللاعبين |
| `Map_Settings` | إعدادات ماب مجمعة |

#### Event Suit من `Security_RegionFeatures`

يمكن تشغيل Event Suit مباشرة من هذا الجدول بدون إضافة صف في `Map_Settings`:

| العمود | القيمة | النتيجة |
|---|---:|---|
| `Enable_EventSuit` | `0` | Event Suit متوقف |
| `Enable_EventSuit` | `1` | Event Suit يعمل داخل الـregion |
| `EventSuit_Team` | `0` | Free-for-all: كل لاعب ضد باقي اللاعبين، والفلتر يستخدم cape 5 |
| `EventSuit_Team` | `1` | Teams: الـcape يأتي من `dbo.Event_CurrentTeams.Team` بالقيم من 1 إلى 4 |

الصف يتحدد الآن بمفتاحين معًا: `WorldID` و`RegionID`. نفس الـRegion يمكن أن يظهر في أكثر من World، فلا تعمل `UPDATE` بالـRegion وحده. لو نفس `RegionID` موجود في `Map_Settings`، يظل صف `Map_Settings` هو الإعداد الأعلى أولوية للتوافق مع النظام القديم. يفضّل ضبط `Enable_AutoPvP = 0` عند استخدام Event Suit؛ لأن Event Suit يدير الـcape بنفسه.

مثال Free-for-all:

```sql
UPDATE KMTGuard.dbo.Security_RegionFeatures
SET Enable_EventSuit = 1,
    EventSuit_Team = 0,
    Enable_AutoPvP = 0,
    UpdatedAt = SYSUTCDATETIME()
WHERE WorldID = 107
  AND RegionID = 14664;
```

مثال Teams، بعد تجهيز `Event_CurrentTeams`:

```sql
UPDATE KMTGuard.dbo.Security_RegionFeatures
SET Enable_EventSuit = 1,
    EventSuit_Team = 1,
    Enable_AutoPvP = 0,
    UpdatedAt = SYSUTCDATETIME()
WHERE WorldID = 107
  AND RegionID = 14664;

MERGE KMTGuard.dbo.Event_CurrentTeams AS target
USING (VALUES (12345, 'CharacterName', 1, 'MANUAL'))
    AS source (CharID, CharName, Team, EventName)
ON target.CharID = source.CharID
WHEN MATCHED THEN
    UPDATE SET CharName = source.CharName,
               Team = source.Team,
               EventName = source.EventName
WHEN NOT MATCHED THEN
    INSERT (CharID, CharName, Team, EventName)
    VALUES (source.CharID, source.CharName, source.Team, source.EventName);
```

مثال منع party, reverse, zerk في region:

```sql
INSERT INTO KMTGuard.dbo.Security_RegionFeatures
(
    WorldID,
    RegionID,
    Allow_IntCharacter,
    Allow_StrCharacter,
    Enable_AdvElixir,
    Enable_Alchemy,
    Enable_AutoPvP,
    Enable_Chat,
    Enable_Exchange,
    Enable_EventSuit,
    Enable_FellowScroll,
    Enable_Global,
    Enable_JobMode,
    Enable_Move,
    Enable_Party,
    Enable_PvP,
    Enable_ResurrectionScroll,
    Enable_Reverse,
    Enable_Stall,
    Enable_Trace,
    Enable_Zerk,
    Enable_NoBot,
    NoBot_TimeSeconds,
    Enabled,
    EventSuit_Team
)
VALUES
(
    107,
    25000,
    1, 1,
    0, 0, 0,
    1, 0, 1, 0, 1,
    0, 1, 0, 1,
    0, 0, 0, 0, 0,
    0, 0,
    1,
    0
);
```

### إعداد Teleport بوقت أو شروط

جدول `Teleport_TimeRules` يربط gate بشروط وقت/نوع شخصية/job:

```sql
INSERT INTO KMTGuard.dbo.Teleport_TimeRules
(
    teleportID,
    OpenTime,
    ClosedTime,
    OlnyINT,
    OnlySTR,
    OnlyJOB,
    OnlyEu,
    OnlyCh,
    IsEnabled
)
VALUES
(
    1001,
    '20:00',
    '22:00',
    0,
    0,
    1,
    0,
    0,
    1
);
```

### إعداد Custom Trade

الـCustom Trade في النسخة الحالية مبني على أربع نقاط دخول:

| Procedure | وظيفتها |
|---|---|
| `_CustomTradeGoodsBuyingRequest` | يقرر هل محاولة الشراء مسموحة قبل تنفيذها ويرجع `@IsBlocked OUTPUT`. |
| `_CustomTradeGoodsBuying` | يسجل نجاح شراء Goods بعد التنفيذ. |
| `_CustomTradeGoodsSellingRequest` | يقرر هل محاولة البيع مسموحة قبل تنفيذها ويرجع `@IsBlocked OUTPUT`. |
| `_CustomTradeGoodsSelling` | ينفذ منطق ما بعد نجاح البيع، مثل العداد والمكافأة. |

هذه Procedures أشبه بالـHooks: الفلتر هو الذي يرسل لها CharID وNPC والكمية والـSlot. صاحب السيرفر يخصص جسمها حسب نظام الـTrade عنده، ولا يناديها يدويًا لصرف مكافأة.

مثال النمط الصحيح داخل Procedure الطلب:

```sql
-- يبدأ القرار بالسماح.
SET @IsBlocked = 0;

-- مثال: منع كمية غير منطقية.
IF @Quantity <= 0 OR @Quantity > 50
BEGIN
    SET @IsBlocked = 1;

    EXEC KMTGuard.dbo.Command_NoticeByID
        @CharID = @CharID,
        @NoticeType = 3,
        @Notice = 'The allowed trade quantity is from 1 to 50.';

    RETURN;
END;
```

مثال Anti-spam بسيط يضاف داخل منطق الطلب بعد إنشاء جدول Audit خاص بنظامك:

```sql
IF EXISTS
(
    SELECT 1
    FROM YourTradeDatabase.dbo.TradeAttempts
    WHERE CharID = @CharID
      AND AttemptAtUtc >= DATEADD(second, -5, SYSUTCDATETIME())
)
BEGIN
    SET @IsBlocked = 1;
    RETURN;
END;
```

`YourTradeDatabase.dbo.TradeAttempts` اسم توضيحي وليس جدولًا يأتي مع KMTGuard. استبدله بجدول نظامك الفعلي. لا تنسخ أي Trade Schema من سيرفر آخر؛ أنظمة الـJob تختلف في Job mapping والجداول والمكافآت.

قبل تعديل أي واحدة من الأربع:

```sql
SELECT OBJECT_DEFINITION
(
    OBJECT_ID('KMTGuard.dbo._CustomTradeGoodsSelling')
) AS CurrentDefinition;
```

احتفظ بالتعريف الأصلي، ولا تغيّر ترتيب أو أنواع الـParameters لأن الفلتر يستدعيها بالعقد الموجود.

### أوامر صيانة يومية مفيدة

عرض آخر أوامر filter queue:

```sql
SELECT TOP (100) *
FROM KMTGuard.dbo.Command_FilterQueue
ORDER BY ID DESC;
```

عرض آخر chest rewards:

```sql
SELECT TOP (100) *
FROM KMTGuard.dbo.Item_Chest
ORDER BY ID DESC;
```

عرض online/HWID data:

```sql
SELECT TOP (100) *
FROM KMTGuard.dbo.Auth_HWIDs
ORDER BY ID DESC;
```

بحث عن لاعب بالاسم:

```sql
SELECT CharID, CharName16, CurLevel, RefObjID
FROM SRO_VT_SHARD.dbo._Char
WHERE CharName16 = 'PlayerName';
```

بحث عن آيتم:

```sql
SELECT ID, CodeName128, ObjName128, Service
FROM SRO_VT_SHARD.dbo._RefObjCommon
WHERE CodeName128 LIKE '%GLOBAL%';
```

## كتالوج الجداول التي تهم صاحب السيرفر

### النظام والأوامر

| Table | فائدته | تستخدمه إزاي |
|---|---|---|
| `System_Settings` | إعدادات عامة للفلتر والأنظمة | تعديل values من لوحة أو SQL بحذر |
| `System_SettingsBackup` | Backup لتغييرات الإعدادات | للرجوع للتغييرات القديمة |
| `System_GameServerSettings` | إعدادات مرتبطة بالـ GameServer | غالبًا يقرأها الفلتر |
| `System_ShardSettings` | إعدادات مرتبطة بالـ shard | تستخدمها الأنظمة العامة |
| `System_Notices` | نصوص رسائل جاهزة | تعديل نصوص messages بدل تغيير الكود |
| `System_ProxyServices` | تعريف خدمات proxy/bind | لا تعدلها إلا أثناء setup |
| `System_Schedule` | جدولة queries عامة | لتشغيل أوامر SQL في وقت محدد |
| `Event_Schedule` | جدولة events بصيغة query/schedule | لإدارة auto events من DB |
| `Event_ScheduleSettings` | جدول أوقات events أبسط | day/time |
| `Event_ScheduleVersion` | رقم نسخة الجدولة | الفلتر يستخدمه لمعرفة وجود تحديث |
| `Command_FilterQueue` | أوامر تنتظر تنفيذ الفلتر | لا تضيف يدويًا إلا فاهم CommandID |
| `Command_GameServerQueue` | أوامر موجهة للـ GameServer | داخلي |
| `Command_PlannedQueue` | أوامر مؤجلة لوقت لاحق | يستخدم للـ planned notices/actions |
| `HonorRankRefreshHistory` | تاريخ طلبات تحديث Training Camp Honor Rank ونتيجتها | اقرأه من `Command_GetHonorRankRefreshStatus` |
| `Events.dbo._AutoEventCommandQueue` | طلبات start/stop/reload للأحداث الحديثة | راقبه للتشخيص، واستخدم Procedures الـEnqueue بدل الإدخال اليدوي |
| `Admin_AuditLog` | سجل أفعال الأدمن | للمراجعة والمحاسبة |

### الحسابات والحماية

| Table | فائدته | تستخدمه إزاي |
|---|---|---|
| `Auth_HWIDs` | آخر HWID/IP/region/level لكل لاعب online | متابعة limits أو تشخيص مشاكل دخول |
| `Auth_HWIDBypassIPs` | IPs مستثناة أو لها limit خاص | أضف IP وlimit عند الحاجة |
| `Auth_GMIPs` | IPs مسموحة للـ GM | لحماية صلاحيات GM |
| `Auth_SecondaryPasswords` | كلمات مرور ثانية للشخصيات/الحسابات | لا تعدلها يدويًا إلا لإعادة ضبط مدروسة |
| `QuickLogin_Tokens` | أجهزة وتوكنات quick login | تعطيل token بتغيير `IsActive` |
| `QuickLogin_Log` | سجل محاولات quick login | للتحقيق في مشاكل الدخول |
| `QuickLogin_ServerSecret` | المفتاح السري الذي يوقّع Quick Login tokens | داخلي وحساس جدًا؛ لا تعرضه ولا تعدله يدويًا |
| `Clientless_Accounts` | حسابات بوت/عميل بدون واجهة يديرها النظام | enable/disable ومعلومات reconnect |

### الأمن والمناطق

| Table | فائدته | تستخدمه إزاي |
|---|---|---|
| `Security_Blacklist` | حظر packets/messages حسب MsgId | حماية متقدمة |
| `Security_Whitelist` | السماح packets/messages حسب MsgId | حماية متقدمة |
| `Security_BlockedWords` | كلمات ممنوعة في الشات | أضف الكلمة وفعّلها |
| `Security_AttackRules` | قواعد من يقدر يضرب mob معين | STR/INT/job/party/guild |
| `Security_RegionFeatures` | فتح/قفل خصائص في region | chat, party, zerk, exchange, reverse, إلخ |
| `Security_RegionChat` | تعطيل الشات في region | region-level chat control |
| `Security_HiddenSkillEffects` | إخفاء effects مهارات معينة | لتحسين الرؤية أو event rules |
| `Security_HideNameRegions` | إخفاء أسماء اللاعبين في regions | Events/PVP |
| `Map_Settings` | إعدادات خريطة/region مجمعة | event suit, hide map, auto cape, disable party |
| `Security_BotProtectionLog` | سجل قرارات حماية البوت: الحدث والإجراء والحساب/HWID/IP | للتدقيق وتشخيص false positives؛ لا تستخدمه كقائمة حظر |

### الآيتمات والمكافآت

| Table | فائدته | تستخدمه إزاي |
|---|---|---|
| `Item_Chest` | صندوق مكافآت اللاعب | لا تدخل يدويًا عادة، استخدم `Item_AddChest` |
| `Item_Locked` | آيتمات مقفولة بكلمة مرور | item lock system |
| `Item_TimedPlus` | plus مؤقت لآيتمات عادية | النظام يرجعه بعد انتهاء الوقت |
| `Item_TimedDevilPlus` | plus مؤقت لـ devil/spacial items | داخلي |
| `Mall_Items` | عناصر Item Mall داخل واجهة KMT | تعديل السعر، الترتيب، التفعيل |
| `Mall_Avatars` | avaters في mall | تعديل السعر/category |
| `Mall_Categories` | أقسام mall | أسماء وتصنيفات |
| `Offer_List` | عروض خاصة داخل الواجهة | سعر أصلي/خصم/مدة العرض |
| `Offer_PurchaseLog` | سجل شراء العروض | مراجعة وتحقيق |
| `LuckySpin_Rewards` | جوائز Lucky Spin | item/rate/active |
| `LuckySpin_Log` | سجل لفات Lucky Spin | متابعة الفائزين والتكلفة |
| `Attendance_Rewards` | جوائز الحضور اليومي | day/item/count |
| `Attendance_Players` | تقدم حضور اللاعبين | آخر يوم وعدد أيام |
| `Attendance_RewardLog` | سجل استلام جوائز الحضور | منع تكرار الاستلام |

### الشكل والألقاب والأيقونات

| Table | فائدته | تستخدمه إزاي |
|---|---|---|
| `Style_Titles` | قائمة الـ titles المتاحة | أضف TitleID وTitleName |
| `Style_PlayerTitles` | titles المملوكة للاعب | استخدم `Style_AddTitle` |
| `Style_ActiveTitles` | title المفعل حاليًا | استخدم `Style_UpdateTitle` |
| `Style_IconFiles` | قائمة icons ومسار ddj في الميديا | أضف icon path مطابق للميديا |
| `Style_PlayerIcons` | icons المملوكة للاعب | استخدم `Style_AddIcon` |
| `Style_ActiveLeftIcons` | icon يسار مفعل | استخدم `Style_UpdateLeftIcon` |
| `Style_ActiveRightIcons` | icon يمين مفعل | استخدم `Style_UpdateRightIcon` |
| `Style_PlayerTitleColors` | ألوان title يملكها اللاعب | استخدم `Style_AddTitleColor` |
| `Style_ActiveTitleColors` | لون title مفعل | استخدم `Style_UpdateTitleColor` |
| `Style_ActiveNameColors` | لون اسم مفعل | استخدم `Style_UpdateNameColor` |
| `Style_GlobalColors` | ألوان globals حسب item | إعداد ألوان chat/global |
| `Style_AutoCapeRegions` | regions يشتغل فيها auto cape | event/pvp areas |
| `KillerAnimation_List` | قائمة animations المتاحة للقتل | price, payment, display |
| `KillerAnimation_Owned` | animations المملوكة للاعب | الشراء يضيف هنا |
| `KillerAnimation_Active` | animation مفعل للاعب | الاختيار الحالي |
| `KillerAnimation_PurchaseLog` | سجل شراء animations | مراجعة العمليات |

### الأحداث والـ PVP

| Table | فائدته | تستخدمه إزاي |
|---|---|---|
| `Events.dbo._AutoEventConfig` | إعدادات كل Auto Event نصي/تلقائي | enable, rounds, المدة، level وHWID |
| `Events.dbo._AutoEventRoundContent` | أسئلة ومحتوى الـRounds | prompt/answer/weight |
| `Events.dbo._AutoEventRun` | تشغيلات الأحداث | status/start/finish |
| `Events.dbo._AutoEventRound` | Rounds داخل كل Run | winner/status |
| `Events.dbo._AutoEventReward` | جوائز الأحداث | placement/reward type/item/gold |
| `Events.dbo._AutoEventWinnerLog` | سجل الفائزين | مراجعة الفائز والدليل والمكافأة |
| `Events.dbo._SurvivalSoloConfig` | إعداد Survival Solo | التسجيل والقتال والـArena |
| `Events.dbo._SurvivalSoloReward` | جوائز Survival Solo حسب المركز | عدّل النوع والقيمة والتفعيل |
| `Events.dbo._SurvivalSoloSchedule` | مواعيد Survival Solo | الوقت وDaysMask والتفعيل |
| `Events.dbo._SurvivalPartyConfig` | إعداد Survival Party | التسجيل والقتال والـArena |
| `Events.dbo._SurvivalPartyReward` | جوائز Survival Party | جوائز أعضاء الفريق حسب المركز |
| `Events.dbo._SurvivalPartySchedule` | مواعيد Survival Party | الوقت وDaysMask والتفعيل |
| `Events.dbo._CompetitiveEventConfig` | إعداد LMS وMADNESS وDTT | mode, players, arena, anti-farm |
| `Events.dbo._CompetitiveEventReward` | جوائز الأحداث التنافسية | placement/reward type/amount/item |
| `Events.dbo._CompetitiveEventSchedule` | مواعيد LMS/MADNESS/DTT | EventID والوقت والأيام |
| `Events.dbo._CompetitiveEventScore` | النتيجة الحية لكل لاعب/فريق في التشغيل | يحدّثها النظام؛ مفيدة للتشخيص |
| `Events.dbo._CompetitiveEventKillLedger` | دفتر kills ومنع احتساب kill مكرر أو farming | سجل داخلي للمراجعة |
| `Events.dbo._HideAndSeekConfig` | إعداد Hide & Seek والبوت والمدة والـlimits ومكان العودة | عدّل الحساب والشخصية والمدة بحذر |
| `Events.dbo._HideAndSeekLocation` | أماكن اختباء البوت مع weight والتفعيل | أضف/عطّل مواقع الحدث |
| `Events.dbo._HideAndSeekReward` | جوائز الفائز في Hide & Seek | Silk/Gold/Item حسب `RewardType` |
| `Events.dbo._HideAndSeekSchedule` | مواعيد Hide & Seek | StartTime وDaysMask |
| `Events.dbo._HideAndSeekRun` | سجل كل تشغيل ومكانه والفائز ونتيجة الجائزة | للتاريخ والتشخيص، لا تعدل التشغيل الجاري |
| `Event_CurrentTeams` | فرق event الحالية | team لكل char |
| `Event_RegisterSettings` | أنواع التسجيل للـ events | أسماء ووصف |
| `Event_ShadowDungeon` | تقدم shadow dungeon لكل لاعب | uniques التي تم استدعاؤها |
| `PVP_Settings` | إعدادات PVP challenge العامة | arena, cape, gold, timeouts |
| `PVP_Arenas` | Arenas متاحة للـ PVP | world/region/position |
| `PVP_Matches` | مباريات PVP | status, players, wager, winner |
| `PVP_KillLog` | kills داخل PVP matches | للمراجعة والنتيجة |

### التجارة والـ Job

| Table | فائدته | تستخدمه إزاي |
|---|---|---|
| `Rank_Silk` | ترتيب/تاريخ silk | ranking/reporting |

لا توجد مجموعة جداول `Trade_Config` ثابتة ضمن عقد KMTGuard العام؛ تخزين نظام الـCustom Trade يعتمد على تصميم السيرفر. نقاط الربط الثابتة هي Procedures الشراء والبيع المشروحة في فصل Custom Trade.

### NPC و Uniques

| Table | فائدته | تستخدمه إزاي |
|---|---|---|
| `NPC_GameServerIDs` | UniqueID الحالي للموبات/NPC spawned | يستخدمه kill/sync |
| `NPC_MobAffinity` | تحديد من مسموح له يتعامل مع mob | allowed mask |
| `Log_MobKillSettings` | mobs التي نتابع kill logs لها | أضف RefMobID |

### اللاعب والواجهة

| Table | فائدته | تستخدمه إزاي |
|---|---|---|
| `Teleport_SavedLocations` | أماكن محفوظة للاعب | self teleport locations |
| `Teleport_TimeRules` | قواعد فتح/غلق teleport بالوقت | بوابات بوقت أو شروط |
| `Teleport_RulesLegacy` | قواعد teleport قديمة | استخدمها لو النظام legacy مفعل |
| `Teleport_FreezeQueue` | طابور النقل مع Freeze ونتيجة التنفيذ | استخدم `Teleport_PositionFreeze` وراقب `Status/ErrorMessage` |
| `Web_Buttons` | أزرار web داخل الكلاينت | name/icon/url/size |
| `Party_Members` | snapshot لأعضاء الـParty وحالة الـmaster/job | يحدّثه الفلتر للتكاملات؛ لا تجعله مصدرًا دائمًا |
| `Player_Settings` | تفضيلات واجهة اللاعب مثل إخفاء Item Info | يحفظه `UI_SavePlayer` |
| `Macro_Settings` | مفاتيح AutoPotion/AutoSkill/AutoHunt/AutoPickup/AutoScroll | يحفظها `UI_SaveMacro` |
| `Macro_AutoPotion` | إعداد كل slot داخل Auto Potion | يحفظه `UI_SavePotion` |
| `Fellow_PetObjectIDs` | RefObjIDs التي يعاملها النظام كـFellow pets | تعريف نطاق حيوانات Fellow |
| `Fellow_Settings` | skills والـlevels والanimations الخاصة بكل نوع Fellow | إعداد مرجعي للنظام |
| `Fellow_Skills` | skills المفعلة لكل Fellow item instance (`ID64`) | يحفظه `Player_SaveFellow` |

### Logs والتقارير

| Table | فائدته | تستخدمه إزاي |
|---|---|---|
| `Log_Chat` | سجل الشات | مراجعة إساءة/بلاغات |
| `Log_PlayerKills` | سجل قتل اللاعبين | PVP/job/events |
| `Party_MatchingLog` | إنشاء party matching | تتبع abuse أو activity |
| `Stall_CreateLog` | سجل فتح stalls | مراقبة |
| `Stall_Offline` | offline stall sessions | status/expiry |
| `Stall_SilkTransactions` | عمليات شراء/بيع silk stall | audit/refund |
| `Rank_Categories` | تعريف أقسام الرانك | enable/category |
| `Rank_Data01` | بيانات الرانك للتصنيف 1 | CharID/Name/Guild/Point |
| `Rank_Data02` | بيانات الرانك للتصنيف 2 | CharID/Name/Guild/Point |
| `Rank_Data03` | بيانات الرانك للتصنيف 3 | CharID/Name/Guild/Point |
| `Rank_Data04` | بيانات الرانك للتصنيف 4 | CharID/Name/Guild/Point |
| `Rank_Data05` | بيانات الرانك للتصنيف 5 | CharID/Name/Guild/Point |
| `Rank_Data06` | بيانات الرانك للتصنيف 6 | CharID/Name/Guild/Point |
| `Rank_Data07` | بيانات الرانك للتصنيف 7 | CharID/Name/Guild/Point |
| `Rank_Data08` | بيانات الرانك للتصنيف 8 | CharID/Name/Guild/Point |
| `Rank_Data09` | بيانات الرانك للتصنيف 9 | CharID/Name/Guild/Point |

### Achievements

| Table | فائدته | تستخدمه إزاي |
|---|---|---|
| `Achievement_List` | تعريف achievements | name/category/reward |
| `Achievement_Conditions` | شروط كل achievement | complete count/type |
| `Achievement_Players` | achievements المكتملة/حالة اللاعب | يحدّثها النظام |
| `Achievement_PlayerConditions` | تقدم اللاعب داخل الشروط | progress count |

## كتالوج Procedures التي يستخدمها صاحب السيرفر

تمت مراجعة قاعدة `KMTGuard` الفعلية اسمًا وParameters اسمًا. وقت إعداد هذا
الإصدار تحتوي على `109` Procedures. الكتالوج التالي يوثقها كلها، بما فيها
إجراءات الحفظ والتحميل الداخلية، لكنه يعلّم الداخلي والقديم بوضوح حتى لا
يشغّله العميل كأداة إدارة. وتوجد كذلك `3` Procedures للأحداث الحديثة في
قاعدة `Events`.

بعض الأسماء تظهر في الجدول المختصر ثم في ملحق الأمثلة. الجدول يشرح **لماذا ومتى** تستخدمها، والملحق يعطيك **صيغة الاستدعاء الكاملة** بكل Parameters.

### Achievement

| Procedure | وظيفتها |
|---|---|
| `Achievement_AddPlayer` | تهيئة achievements للاعب جديد أو أول استخدام |
| `Achievement_Update` | تحديث تقدم condition داخل achievement |
| `Achievement_UniqueKill` | تحديث achievements عند قتل unique بالاسم |
| `Achievement_UniqueKillByID` | تحديث achievements عند قتل unique بالـ ID |

### Commands

| Procedure | وظيفتها |
|---|---|
| `Command_ChangeName` | تغيير grant/name مرتبط بالشخصية |
| `Command_DisconnectAll` | فصل كل اللاعبين |
| `Command_DisconnectByID` | فصل لاعب بالـ CharID |
| `Command_DisconnectByName` | فصل لاعب بالاسم |
| `Command_GetUp` | تنفيذ get up للاعب |
| `Command_GetUpAtPosition` | get up مع موقع محدد |
| `Command_MapPingByID` | إرسال علامة على الماب للاعب |
| `Command_NoticeAll` | notice للجميع |
| `Command_NoticeByID` | notice لشخصية بالـ CharID |
| `Command_NoticeByName` | notice لشخصية بالاسم |
| `Command_RefreshHonorRank` | يضع طلبًا واحدًا آمنًا لتحديث Training Camp Honor Rank لكل GameServers |
| `Command_GetHonorRankRefreshStatus` | يعرض آخر 20 طلبًا أو طلبًا محددًا ونجاحه/خطأه |

### Events

| Procedure | وظيفتها |
|---|---|
| `Events.dbo._AutoEventEnqueueStart` | بدء حدث حديث بالـ EventCode |
| `Events.dbo._AutoEventEnqueueStop` | إيقاف الحدث الحديث الحالي |
| `Events.dbo._AutoEventEnqueueReload` | إعادة تحميل إعدادات الأحداث الحديثة |
| `Event_Attendance` | تسجيل حضور يومي |
| `Event_RegisterGoldLottery` | تسجيل لاعب في gold lottery |
| `Event_MobKilled` | معالجة قتل mob داخل event |
| `Event_AddKill` | إضافة kill counter |
| `Event_AddJobKill` | إضافة kill counter حسب job |
| `Event_AddTeamKill` | إضافة kill counter حسب team |
| `Event_AddKillCounter` | إنشاء/تحديث عداد kills |
| `Event_AddJobKillCounter` | إنشاء/تحديث عداد job kills |
| `Event_AddTeamKillCounter` | إنشاء/تحديث عداد team kills |
| `Event_AddWorldTimer` | إضافة timer للـ world |
| `Event_AddRegionTimer` | إضافة timer للـ region |
| `Event_Start` | wrapper قديم يشير إلى `KMTGuard.dbo.AutoEvent_CommandQueue` غير الموجود في العقد الحالي؛ لا تستخدمه |
| `Event_Stop` | wrapper قديم لنفس الـqueue القديمة؛ لا تستخدمه |
| `Event_Reload` | wrapper قديم لنفس الـqueue القديمة؛ استخدم إجراءات `Events.dbo._AutoEventEnqueue*` |

### Hooks

| Procedure | وظيفتها |
|---|---|
| `Hook_AlchemySuccess` | يتنفذ عند نجاح alchemy |
| `Hook_AutoEquip` | يتنفذ عند auto equip |
| `Hook_CharacterGetUp` | يتنفذ عند قيام شخصية |
| `Hook_CharacterKill` | يتنفذ عند قتل لاعب للاعب |
| `Hook_EventCancel` | يتنفذ عند إلغاء تسجيل event |
| `Hook_EventRegister` | يتنفذ عند تسجيل event |
| `Hook_GameServerStart` | يتنفذ عند بداية GameServer |
| `Hook_ItemMallBuy` | يتنفذ عند شراء item mall |
| `Hook_NPCBuy` | يتنفذ عند الشراء من NPC |
| `Hook_PartyJoin` | يتنفذ عند دخول party |
| `Hook_PartyMatchCreate` | يتنفذ عند إنشاء party matching |
| `Hook_SelectScroll` | يتنفذ عند استخدام select scroll |
| `Hook_StallCreate` | يتنفذ عند فتح stall |
| `Hook_Teleport` | يتحكم/يراقب teleport |
| `Hook_UniqueKill` | يتنفذ عند قتل unique |
| `Hook_UniqueSpawn` | يتنفذ عند spawn unique |

### Items و Live

| Procedure | وظيفتها |
|---|---|
| `Item_AddChest` | إضافة reward item للـ chest |
| `Item_AddChestOld` | نسخة توافق قديمة |
| `Item_GetInfo` | قراءة بيانات آيتم من inventory slot |
| `Live_AddBuff` | إضافة buff بالـ code name |
| `Live_AddBuffNoLimit` | إضافة buff بالـ SkillID بدون limit |
| `Live_RemoveBuff` | إزالة buff |
| `Live_Cape` | تغيير cape |
| `Live_Gold` | إضافة/خصم gold |
| `Live_Silk` | إضافة/خصم silk/gift/point |
| `Live_Skill` | فتح/قفل skill أو حالة skill على world |
| `Live_Teleport` | فتح/قفل teleport gate |
| `Live_ChangeItem` | تغيير item في slot |
| `Live_UseItem` | استهلاك item من slot |
| `Live_UseAndChangeItem` | استهلاك item وتغيير item آخر |

### NPC و Teleport

| Procedure | وظيفتها |
|---|---|
| `NPC_Spawn` | spawn بالـ CodeName |
| `NPC_SpawnAtPosition` | spawn بالـ MonsterID |
| `NPC_SpawnNearPlayer` | spawn قريب من لاعب |
| `NPC_Kill` | حذف NPC/mob بالـ CodeName |
| `NPC_KillByWorld` | حذف NPC/mob من world |
| `NPC_Sync` | ربط الـUniqueID العائد من GameServer بالـRefObjID بعد Spawn مخصص |
| `Teleport_PlayerToTown` | نقل لاعب للمدينة |
| `Teleport_AllToTown` | نقل كل world للمدينة |
| `Teleport_Position` | نقل لاعب لمكان محدد |
| `Teleport_PositionFreeze` | نقل لاعب وتجميده |
| `Teleport_SafeZone` | نقل إلى safe zone |
| `Teleport_Self` | self teleport |

### Player Services المفيدة للتكامل

| Procedure | وظيفتها |
|---|---|
| `Player_GetFellowPet` | قراءة `ItemID64` و`RefItemID` لحيوان Fellow في Slot محدد قبل تعديل مخصص |
| `Player_SaveLocation` | حفظ موقع في نظام الأماكن المحفوظة وإرجاع النتيجة في `@Success OUTPUT` |
| `Player_SaveConfig` | داخلي: يحفظ slot من إعدادات الكلاينت في `SRO_VT_SHARD.dbo._ClientConfig` |
| `Player_SaveFellow` | داخلي: يحفظ skills المفعلة لحيوان Fellow في `Fellow_Skills` |

### Style

| Procedure | وظيفتها |
|---|---|
| `Style_AddTitle` | إعطاء title |
| `Style_ChooseTitle` | اختيار title من الواجهة |
| `Style_UpdateTitle` | تفعيل title |
| `Style_RemoveTitle` | إزالة title |
| `Style_AddIcon` | إعطاء icon |
| `Style_ChooseIcon` | اختيار icon من الواجهة |
| `Style_UpdateLeftIcon` | تفعيل icon يسار |
| `Style_UpdateRightIcon` | تفعيل icon يمين |
| `Style_RemoveLeftIcon` | إزالة icon يسار |
| `Style_RemoveRightIcon` | إزالة icon يمين |
| `Style_AddTitleColor` | إعطاء لون title |
| `Style_ChooseTitleColor` | اختيار لون title |
| `Style_UpdateTitleColor` | تفعيل لون title |
| `Style_RemoveTitleColor` | إزالة لون title |
| `Style_UpdateNameColor` | تفعيل لون اسم |
| `Style_RemoveNameColor` | إزالة لون اسم |
| `Style_ChangeGlow` | تغيير glow لآيتم |
| `Style_ChangeModel` | تغيير model لآيتم |
| `Style_GrantAndActivateHwanTitle` | يعطي Hwan title ويحدّث `_Char.HwanLevel` ويرسله live في عملية واحدة |

### Logs وCustom Trade

| Procedure | وظيفتها |
|---|---|
| `Log_Character` | تسجيل event character |
| `Log_Drops` | قراءة drop logs بصفحات وفلاتر |
| `Log_MonsterDrops` | قراءة drops لموب محدد |
| `Log_RareDrops` | قراءة rare drops |
| `_CustomTradeGoodsBuyingRequest` | سؤال هل شراء goods مسموح |
| `_CustomTradeGoodsBuying` | تسجيل شراء goods |
| `_CustomTradeGoodsSellingRequest` | سؤال هل بيع goods مسموح |
| `_CustomTradeGoodsSelling` | تسجيل بيع goods وصرف rewards |

### Authentication وUI وSystem الداخلية

| Procedure | وظيفتها | هل يشغلها العميل يدويًا؟ |
|---|---|---|
| `Auth_Login` | يتحقق من اسم الحساب وكلمة المرور أمام `TB_User` ويرجع `1/0` | لا؛ داخلي، ويتعامل مع بيانات اعتماد حساسة |
| `Auth_UpdateHWID` | upsert لحالة اللاعب وIP/HWID/region/world/level ويجدد WebToken | لا؛ يناديه الفلتر عند تغير الجلسة |
| `UI_SaveMacro` | يحفظ مفاتيح الـMacro في `Macro_Settings` | لا؛ الواجهة تناديه |
| `UI_SavePlayer` | يحفظ تفضيل `HideItemInfo` بالاسم | لا؛ الواجهة تناديه |
| `UI_SavePotion` | يحفظ Active/Value لكل AutoPotion slot | لا؛ الواجهة تناديه |
| `System_Start` | placeholder بلا تنفيذ في النسخة الحالية | لا توجد فائدة من تشغيله |

## Appendix: صيغ استدعاء الـProcedures المهمة

استخدم هذا الجزء كمرجع سريع. القيم الموجودة أمثلة تعليمية وليست بيانات جاهزة لسيرفرك؛ استبدل `CharID` والـIDs والـCodeNames بعد التحقق منها. الأمثلة ذات `OUTPUT` تعرّف Variable لاستقبال النتيجة بالطريقة الصحيحة.

### Custom Trade

```sql
EXEC KMTGuard.dbo._CustomTradeGoodsBuying @CharID = 12345, @Charname = 'PlayerName', @NpcID = 1, @Petid = 1, @NpcCodename = 'REPLACE_WITH_VALID_VALUE', @NpcTab = 1, @NpcSlot = 1, @Quantity = 10;
EXEC KMTGuard.dbo._CustomTradeGoodsSelling @CharID = 12345, @Charname = 'PlayerName', @Petid = 1, @NpcID = 1, @NpcCodename = 'REPLACE_WITH_VALID_VALUE', @PetSlot = 1, @Quantity = 10;

DECLARE @BuyIsBlocked bit;
EXEC KMTGuard.dbo._CustomTradeGoodsBuyingRequest
    @CharID = 12345, @Charname = 'PlayerName', @NpcID = 1,
    @NpcCodename = 'REPLACE_WITH_VALID_NPC_CODE', @NpcTab = 1,
    @NpcSlot = 1, @Quantity = 10, @IsBlocked = @BuyIsBlocked OUTPUT;
SELECT @BuyIsBlocked AS IsBlocked;

DECLARE @SellIsBlocked bit;
EXEC KMTGuard.dbo._CustomTradeGoodsSellingRequest
    @CharID = 12345, @Charname = 'PlayerName', @Petid = 1, @NpcID = 1,
    @NpcCodename = 'REPLACE_WITH_VALID_NPC_CODE', @PetSlot = 1,
    @Quantity = 10, @IsBlocked = @SellIsBlocked OUTPUT;
SELECT @SellIsBlocked AS IsBlocked;
```

### Achievement

```sql
EXEC KMTGuard.dbo.Achievement_AddPlayer @CharID = 12345;
EXEC KMTGuard.dbo.Achievement_UniqueKill @RefObjID = 1954, @KillerrName = 'PlayerName';
EXEC KMTGuard.dbo.Achievement_UniqueKillByID @RefObjID = 1954, @KillerCharName = 'PlayerName';
EXEC KMTGuard.dbo.Achievement_Update @CharID = 12345, @RefAchievementID = 1, @RefAchievementConditionID = 1, @Progress = 1;

```

### Commands

```sql
EXEC KMTGuard.dbo.Command_ChangeName @CharID = 12345, @GrantName = 'Champion';
EXEC KMTGuard.dbo.Command_DisconnectAll;
EXEC KMTGuard.dbo.Command_DisconnectByID @CharID = 12345;
EXEC KMTGuard.dbo.Command_DisconnectByName @CharName16 = 'PlayerName';
EXEC KMTGuard.dbo.Command_GetUp @CharID = 12345;
EXEC KMTGuard.dbo.Command_GetUpAtPosition @CHARID = 12345, @GameWorldID = 107, @RegionId = 25580, @PosX = 500, @PosY = 0, @PosZ = 500;
EXEC KMTGuard.dbo.Command_MapPingByID @CharID = 12345, @RegionID = 25580, @PosX = 500, @PosZ = 500, @PosY = 0, @Seconds = 60;
EXEC KMTGuard.dbo.Command_NoticeAll @NoticeType = 7, @Notice = 'Event message';
EXEC KMTGuard.dbo.Command_NoticeByID @CharID = 12345, @NoticeType = 7, @Notice = 'Event message';
EXEC KMTGuard.dbo.Command_NoticeByName @CharName16 = 'PlayerName', @NoticeType = 7, @Notice = 'Event message';
```

### Events

```sql
EXEC KMTGuard.dbo.Event_AddJobKill @WorldID = 107, @CharName16 = 'PlayerName', @Kill = 1, @Job = 1;
EXEC KMTGuard.dbo.Event_AddJobKillCounter @WorldID = 107;
EXEC KMTGuard.dbo.Event_AddKill @WorldID = 107, @CharName16 = 'PlayerName', @Kill = 1;
EXEC KMTGuard.dbo.Event_AddKillCounter @Title = 'Event Ranking', @WorldID = 107;
EXEC KMTGuard.dbo.Event_AddRegionTimer @Seconds = 60, @RegionID = 25580;
EXEC KMTGuard.dbo.Event_AddTeamKill @WorldID = 107, @CharName16 = 'PlayerName', @Kill = 1, @Team = 1;
EXEC KMTGuard.dbo.Event_AddTeamKillCounter @WorldID = 107;
EXEC KMTGuard.dbo.Event_AddWorldTimer @Seconds = 60, @WorldID = 107;
DECLARE @AttendanceResult int;
EXEC KMTGuard.dbo.Event_Attendance
    @CharID = 12345,
    @Date = '2026-07-23',
    @ReturnValue = @AttendanceResult OUTPUT;
SELECT @AttendanceResult AS AttendanceResult;
EXEC KMTGuard.dbo.Event_MobKilled @RefObjID = 1954, @KilledWorldID = 107, @KilledRegionID = 25580, @KilledPosX = 500.0, @KilledPosY = 0.0, @KilledPosZ = 500.0, @CharID = 12345, @Charname = 'PlayerName';
EXEC KMTGuard.dbo.Event_RegisterGoldLottery @CharID = 12345, @CharName16 = 'PlayerName';
EXEC Events.dbo._AutoEventEnqueueReload @RequestedBy = N'Admin';
EXEC Events.dbo._AutoEventEnqueueStart @EventCode = N'Trivia', @RequestedBy = N'Admin';
EXEC Events.dbo._AutoEventEnqueueStop @RequestedBy = N'Admin';
```

### Hooks

Hooks غالبًا يستدعيها الفلتر تلقائيًا. هذه الصيغ للاختبار فقط:

```sql
EXEC KMTGuard.dbo.Hook_AlchemySuccess @CharID = 12345, @CharName = 'PlayerName', @ItemID = 3851, @Plus = 1, @AdvLevel = 1, @Slot = 1;
EXEC KMTGuard.dbo.Hook_AutoEquip @CharID = 12345, @CharLevel = 90;
EXEC KMTGuard.dbo.Hook_CharacterGetUp @CharID = 12345, @CharName = 'PlayerName', @LatestRegion = 25580, @LatestWorld = 107, @PVPState = 1;
EXEC KMTGuard.dbo.Hook_CharacterKill @WorldID = 107, @RegionID = 25580, @KillerCharID = 1, @KillerCharName = 'PlayerName', @KillerPVPState = 1, @KillerJobStatus = 1, @KillerGuildID = 1, @KillerGuildName = 'REPLACE_WITH_VALID_VALUE', @DeadCharID = 1, @DeadCharName = 'OtherPlayer', @DeadPVPState = 1, @DeadJobStatus = 1, @DeadGuildID = 1, @DeadGuildName = 'REPLACE_WITH_VALID_VALUE';
EXEC KMTGuard.dbo.Hook_EventCancel @CharID = 12345, @CharName16 = 'PlayerName', @EventID = 1, @LiveRegionID = 25580, @WorldID = 107;
EXEC KMTGuard.dbo.Hook_EventRegister @CharID = 12345, @CharName16 = 'PlayerName', @EventID = 1, @LiveRegionID = 25580, @WorldID = 107;
EXEC KMTGuard.dbo.Hook_GameServerStart @GameServerExeName = 'SR_GameServer';
EXEC KMTGuard.dbo.Hook_ItemMallBuy @JID = 1, @CharID = 12345, @CharName16 = 'PlayerName', @ItemID = 3851, @Silk = 1;
EXEC KMTGuard.dbo.Hook_NPCBuy @CharID = 12345, @Slot_From = 1, @BuyedItemID = 3851;
EXEC KMTGuard.dbo.Hook_PartyJoin @CharID = 12345, @CharName = 'PlayerName', @CurrentRegionId = 25580, @CurrentWorldId = 107, @PVPCapeType = 1, @CurrentJobType = 1;
EXEC KMTGuard.dbo.Hook_PartyMatchCreate @CharID = 12345, @CharName = 'PlayerName', @CurrentRegionId = 25580, @CurrentWorldID = 107, @PartyNo = 1;
EXEC KMTGuard.dbo.Hook_SelectScroll @CharName16 = 'PlayerName', @CharID = 12345, @UsedItemSlot = 1, @TargetItemSlot = 1;
EXEC KMTGuard.dbo.Hook_StallCreate @CharID = 12345, @CharName = N'REPLACE_WITH_VALID_VALUE', @UniqueCharID = 1000000, @RegionID = 1, @WorldID = 107, @StallTitle = N'Player Stall';
DECLARE @CanTeleport bit;
EXEC KMTGuard.dbo.Hook_Teleport
    @CharID = 12345,
    @RefTeleportID = 1,
    @CanTeleport = @CanTeleport OUTPUT;
SELECT @CanTeleport AS CanTeleport;
EXEC KMTGuard.dbo.Hook_UniqueKill @RefObjID = 1954, @KillerCharName = 'PlayerName';
EXEC KMTGuard.dbo.Hook_UniqueSpawn @RefObjID = 1954;
```

### Items/Live/Logs

```sql
EXEC KMTGuard.dbo.Item_AddChest @CharID = 12345, @ItemRefObjID = 3851, @Quantity = 10, @From = 'AdminReward', @Plus = 1;
EXEC KMTGuard.dbo.Item_AddChestOld @CharID = 12345, @ItemID = 3851, @Quantity = 10, @Type = 'AdminReward', @Plus = 1;
DECLARE
    @RefItemID int,
    @OptLevel tinyint,
    @CodeName varchar(128),
    @ItemDBID int,
    @AdvOptLevel tinyint;

EXEC KMTGuard.dbo.Item_GetInfo
    @CharID = 12345,
    @SlotIndex = 1,
    @RefItemID = @RefItemID OUTPUT,
    @OptLevel = @OptLevel OUTPUT,
    @CodeName = @CodeName OUTPUT,
    @ItemDBID = @ItemDBID OUTPUT,
    @AdvOptLevel = @AdvOptLevel OUTPUT;

SELECT @RefItemID AS RefItemID, @OptLevel AS OptLevel,
       @CodeName AS CodeName, @ItemDBID AS ItemDBID,
       @AdvOptLevel AS AdvOptLevel;

EXEC KMTGuard.dbo.Live_AddBuff @CharID = 12345, @SkillCodeName = 'REPLACE_WITH_SKILL_CODENAME';
EXEC KMTGuard.dbo.Live_AddBuffNoLimit @CharID = 12345, @SkillID = 12345;
EXEC KMTGuard.dbo.Live_Cape @CharID = 12345, @CapeID = 1;
EXEC KMTGuard.dbo.Live_ChangeItem @CharID = 12345, @Slot = 1, @CodeName128 = 'REPLACE_WITH_VALID_CODENAME';
EXEC KMTGuard.dbo.Live_Gold @CharID = 12345, @Gold = 1000000, @AddOrRemove = 1;
EXEC KMTGuard.dbo.Live_RemoveBuff @CharID = 12345, @SkillID = 12345;
EXEC KMTGuard.dbo.Live_Silk @CharID = 12345, @nSilk = 1, @nSilkGift = 1, @nSilkPoint = 1;
EXEC KMTGuard.dbo.Live_Skill @WorldID = 107, @State = 1;
EXEC KMTGuard.dbo.Live_Teleport @Gate = 1, @State = 1;
EXEC KMTGuard.dbo.Live_UseAndChangeItem @CharID = 12345, @MutateSlot = 1, @CodeName128 = 'REPLACE_WITH_VALID_CODENAME', @ConsumeSlot = 1, @ConsumeAmount = 1;
EXEC KMTGuard.dbo.Live_UseItem @CharID = 12345, @Slot = 1, @ReduceAmount = 1;

EXEC KMTGuard.dbo.Log_Character @CharID = 12345, @EventID = 1, @Data1 = 1, @Data2 = 1, @strPos = 'REPLACE_WITH_VALID_VALUE', @Desc = 'REPLACE_WITH_VALID_VALUE';
EXEC KMTGuard.dbo.Log_Drops @Page = 1, @PageSize = 1, @SearchText = N'', @Filter = 1;
EXEC KMTGuard.dbo.Log_MonsterDrops @MonsterID = 1954;
EXEC KMTGuard.dbo.Log_RareDrops @TopCount = 1;
```

### NPC وStyle وTeleport

```sql
EXEC KMTGuard.dbo.NPC_Kill @CodeName128 = 'REPLACE_WITH_VALID_CODENAME';
EXEC KMTGuard.dbo.NPC_KillByWorld @CodeName128 = 'REPLACE_WITH_VALID_CODENAME', @WorldID = 107;
EXEC KMTGuard.dbo.NPC_Spawn @CodeName128 = 'REPLACE_WITH_VALID_CODENAME', @GameWorldID = 107, @RegionId = 25580, @PosX = 500, @PosY = 0, @PosZ = 500, @GenerateRadius = 1;
EXEC KMTGuard.dbo.NPC_SpawnAtPosition @MonsterID = 1954, @GameWorldID = 107, @RegionId = 25580, @PosX = 500, @PosY = 0, @PosZ = 500, @GenerateRadius = 1;
EXEC KMTGuard.dbo.NPC_SpawnNearPlayer @CharID = 12345, @MonsterID = 1954;
EXEC KMTGuard.dbo.NPC_Sync @UniqueID = 100000, @RefObjId = 1954, @CodeName128 = 'MOB_CH_TIGERWOMAN';

EXEC KMTGuard.dbo.Style_AddIcon @CharID = 12345, @IconID = 1, @Side = 1;
EXEC KMTGuard.dbo.Style_AddTitle @CharID = 12345, @TitleID = 1;
EXEC KMTGuard.dbo.Style_AddTitleColor @CharID = 12345, @ColorCode = 'FF7300', @ColorName = 'Orange';
EXEC KMTGuard.dbo.Style_ChangeGlow @CharName16 = 'PlayerName', @CharID = 12345, @ItemCodeName = N'REPLACE_WITH_VALID_VALUE', @NewGlow = N'REPLACE_WITH_VALID_VALUE', @TargetSlot = 1, @UsedItemSlot = 1;
EXEC KMTGuard.dbo.Style_ChangeModel @CharName16 = 'PlayerName', @CharID = 12345, @ItemCodeName = N'REPLACE_WITH_VALID_VALUE', @NewModel = N'REPLACE_WITH_VALID_VALUE', @TargetSlot = 1, @UsedItemSlot = 1;
EXEC KMTGuard.dbo.Style_ChooseIcon @CharName16 = 'PlayerName', @IconID = 1, @Side = 1;
EXEC KMTGuard.dbo.Style_ChooseTitle @CharName16 = 'PlayerName', @TitleID = 1;
EXEC KMTGuard.dbo.Style_ChooseTitleColor @CharName16 = 'PlayerName', @ColorCode = 'FF7300';
EXEC KMTGuard.dbo.Style_RemoveLeftIcon @CharName16 = 'PlayerName';
EXEC KMTGuard.dbo.Style_RemoveNameColor @CharName16 = 'PlayerName';
EXEC KMTGuard.dbo.Style_RemoveRightIcon @CharName16 = 'PlayerName';
EXEC KMTGuard.dbo.Style_RemoveTitle @CharName16 = 'PlayerName';
EXEC KMTGuard.dbo.Style_RemoveTitleColor @CharName16 = 'PlayerName';
EXEC KMTGuard.dbo.Style_UpdateLeftIcon @CharName16 = 'PlayerName', @IconID = 1;
EXEC KMTGuard.dbo.Style_UpdateNameColor @CharName16 = 'PlayerName', @ColorCode = 'FF7300';
EXEC KMTGuard.dbo.Style_UpdateRightIcon @CharName16 = 'PlayerName', @IconID = 1;
EXEC KMTGuard.dbo.Style_UpdateTitle @CharName16 = 'PlayerName', @TitleID = 1;
EXEC KMTGuard.dbo.Style_UpdateTitleColor @CharName16 = 'PlayerName', @ColorCode = 'FF7300';

EXEC KMTGuard.dbo.Teleport_AllToTown @WorldID = 107;
EXEC KMTGuard.dbo.Teleport_PlayerToTown @CharID = 12345;
EXEC KMTGuard.dbo.Teleport_Position @CharID = 12345, @GameWorldID = 107, @RegionId = 25580, @PosX = 500, @PosY = 0, @PosZ = 500;
EXEC KMTGuard.dbo.Teleport_PositionFreeze @CharID = 12345, @GameWorldID = 107, @RegionId = 25580, @PosX = 500, @PosY = 0, @PosZ = 500, @FreezeSeconds = 10;
EXEC KMTGuard.dbo.Teleport_SafeZone @CHARID = 12345, @GameWorldID = 107, @RegionId = 25580, @PosX = 500, @PosY = 0, @PosZ = 500;
EXEC KMTGuard.dbo.Teleport_Self @CharID = 12345;

```

### Player Services للتكاملات المخصصة

هذه ليست إعدادات يومية، لكنها مفيدة عند بناء Scroll أو نافذة مخصصة تعتمد على Fellow أو Saved Teleport.

```sql
DECLARE @PetItemID64 bigint;
DECLARE @PetRefItemID int;

EXEC KMTGuard.dbo.Player_GetFellowPet
    @CharID = 12345,
    @Slot = 13,
    @ItemID64 = @PetItemID64 OUTPUT,
    @RefItemID = @PetRefItemID OUTPUT;

SELECT @PetItemID64 AS ItemID64, @PetRefItemID AS RefItemID;

DECLARE @LocationSaved bit;

EXEC KMTGuard.dbo.Player_SaveLocation
    @CharID = 12345,
    @LocationID = 1,
    @RegionID = 25580,
    @PosX = 500,
    @PosY = 0,
    @PosZ = 500,
    @WorldID = 107,
    @Success = @LocationSaved OUTPUT;

SELECT @LocationSaved AS LocationSaved;
```

### Honor Rank وHwan Title

```sql
-- يطلب Refresh واحدًا فقط حتى لو ضغط الأدمن مرتين.
EXEC KMTGuard.dbo.Command_RefreshHonorRank
    @RequestedBy = N'AdminPanel';

-- آخر 20 طلبًا.
EXEC KMTGuard.dbo.Command_GetHonorRankRefreshStatus;

-- طلب محدد.
EXEC KMTGuard.dbo.Command_GetHonorRankRefreshStatus
    @RequestID = 123;

-- إعطاء Hwan title وتفعيله في الشارد والـlive معًا.
EXEC KMTGuard.dbo.Style_GrantAndActivateHwanTitle
    @CharID = 12345,
    @TitleID = 5;
```

تحديث Honor Rank يحتاج `KMTGuard_ShardManager.dll` الحالي محمّلًا داخل
ShardManager. النجاح الحقيقي هو `Status='Succeeded'`، وليس مجرد ظهور
`Queued=1`.

### Procedures الداخلية: الصيغة والمالك

الصيغ التالية موثقة لكي يعرف مطور التكامل عقدها، وليست أوامر إدارة يومية.
الفلتر أو واجهة اللاعب هما المالك الطبيعي لها:

```sql
-- داخلي: يرجع result set بقيمة 1 أو 0.
EXEC KMTGuard.dbo.Auth_Login
    @UserName = 'AccountName',
    @Password = 'PlainPassword';

-- داخلي: يحدّث snapshot الجلسة ويجدد WebToken.
EXEC KMTGuard.dbo.Auth_UpdateHWID
    @Active = 1,
    @CharID = 12345,
    @CharName16 = 'PlayerName',
    @IP = '127.0.0.1',
    @Hwid = N'NORMALIZED_HWID',
    @JobStatus = 0,
    @LatestRegionId = 25580,
    @LatestWorldId = 1,
    @CurLevel = 110;

EXEC KMTGuard.dbo.Player_SaveConfig
    @CharID = 12345, @SlotSeq = 1, @SlotType = 1, @Data = 0;

EXEC KMTGuard.dbo.Player_SaveFellow
    @ID64 = 100000000,
    @Enable_Skill_1 = 1, @Enable_Skill_2 = 1,
    @Enable_Skill_3 = 0, @Enable_Skill_4 = 0,
    @Enable_Skill_5 = 0;

EXEC KMTGuard.dbo.UI_SaveMacro
    @CharID = 12345,
    @AutoPotion = 1, @AutoSkill = 0, @AutoHunt = 0,
    @AutoPickup = 1, @AutoScroll = 0;

EXEC KMTGuard.dbo.UI_SavePlayer
    @CharName16 = 'PlayerName', @Value = 1;

EXEC KMTGuard.dbo.UI_SavePotion
    @CharID = 12345, @Slot = 1, @Active = 1, @Value = 70;

-- Placeholder حاليًا ولا ينفذ شيئًا.
EXEC KMTGuard.dbo.System_Start;
```

لا تستخدم `Auth_Login` من web endpoint عام، ولا تسجل قيمة `@Password` أو
محتوى `QuickLogin_ServerSecret` في logs.

### إعداد Hide & Seek

الحدث كوده الافتراضي `HNS`. بعد تعديل الإعدادات أو الأماكن أو الجوائز، نفّذ
reload من Queue الأحداث الحديثة:

```sql
-- راجع الإعداد من غير إظهار BotPassword في تقارير الدعم.
SELECT
    EventCode, DisplayName, Enabled,
    StartDelaySeconds, SearchSeconds, ReminderIntervalSeconds,
    MinLevel, HwidLimit, RequireHwid,
    BotAccountName, BotCharacterName,
    ReturnWorldID, ReturnRegionID, ReturnX, ReturnY, ReturnZ
FROM Events.dbo._HideAndSeekConfig
WHERE EventCode = N'HNS';

-- إضافة مكان جديد.
INSERT Events.dbo._HideAndSeekLocation
    (LocationName, WorldID, RegionID, PosX, PosY, PosZ, Weight, IsActive)
VALUES
    (N'Jangan secret spot', 1, 25580, 500, 0, 500, 10, 1);

-- المواعيد: DaysMask=127 يعني كل أيام الأسبوع.
INSERT Events.dbo._HideAndSeekSchedule
    (StartTime, DaysMask, IsActive)
VALUES
    ('21:00:00', 127, 1);

EXEC Events.dbo._AutoEventEnqueueReload
    @RequestedBy = N'AdminPanel';

EXEC Events.dbo._AutoEventEnqueueStart
    @EventCode = N'HNS',
    @RequestedBy = N'AdminPanel';
```

لا تستخدم `KMTGuard.dbo.Event_Start/Event_Stop/Event_Reload` لهذا النظام؛
هذه wrappers قديمة تشير إلى Queue غير موجودة في العقد الحالي. استخدم
`Events.dbo._AutoEventEnqueueStart/Stop/Reload`.

## Checklist للعميل قبل تشغيل أي سكربت على KMTGuard

1. تأكد أن `CharID` صحيح.
2. تأكد أن `CodeName128` موجود في `SRO_VT_SHARD.dbo._RefObjCommon`.
3. استخدم `Item_AddChest` للآيتمات بدل إدخال مباشر في `Item_Chest`.
4. استخدم `Command_NoticeByID` للرسائل الفردية، و`Command_NoticeAll` للرسائل العامة.
5. لا تعدل `Command_FilterQueue` مباشرة إلا في حالات صيانة معروفة.
6. خذ backup قبل تعديل إعدادات كبيرة مثل security أو trade أو events.
7. لا تعتبر اختفاء صف `Command_GameServerQueue` دليل نجاح؛ راجع تأثير الأمر أو سجل الحالة المخصص.
8. لا تنشر `QuickLogin_ServerSecret` أو HWID أو كلمات المرور في screenshots أو logs.
9. بعد تعديل Events نفّذ `Events.dbo._AutoEventEnqueueReload`.
10. اختبر أولًا على لاعب تجريبي وراجع Queue وlogs قبل تعميم المكافآت.
