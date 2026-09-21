# Player Style System

آخر تحديث: 2026-07-27

هذا هو المرجع النهائي لنظام شكل اللاعب. أي أسماء قديمة تبدأ بـ
`Style_` أو تحتوي على `TitleNameNew` لم تعد جزءًا من النظام.

## الفرق بين العناصر

| الاسم | ما يظهر للاعب | مثال |
|---|---|---|
| Title | لقب اللعبة الأصلي فوق الشخصية (Hwan) | General |
| Tag | كلمة قصيرة بجوار اسم اللاعب | VIP |
| Guild Nickname | نيك نيم داخل اسم الجيلد | Trader |
| Title Color | لون الـTitle الأصلي | Gold |
| Name Color | لون اسم الشخصية | Red |
| Left Icon | أيقونة ناحية الشمال | VIP icon |
| Right Icon | أيقونة ناحية اليمين | Rank icon |

## معنى الأوامر

كل عنصر يستخدم أربع كلمات ثابتة:

- `Add`: يضيف العنصر إلى ممتلكات اللاعب، لكنه لا يعرضه.
- `Remove`: يحذف العنصر من ممتلكات اللاعب.
- `Activate`: يعرض عنصرًا يملكه اللاعب حاليًا.
- `Deactivate`: يخفي العنصر الحالي، لكنه يظل مملوكًا.

مثال: `LeftIcon_Add` يعطي الأيقونة للاعب. بعدها
`LeftIcon_Activate` يعرضها. `LeftIcon_Deactivate` يخفيها فقط، بينما
`LeftIcon_Remove` يسحب ملكيتها.

## أسماء الجداول النهائية

| النوع | التعريف | ملكية اللاعب | العنصر المفعل |
|---|---|---|---|
| Title | `SRO_SHARD.dbo._RefHWANLevel` | `PlayerTitles` | حالة الـGameServer |
| Tag | `Tags` | `PlayerTags` | `ActiveTags` |
| Title Color | — | `PlayerTitleColors` | `ActiveTitleColors` |
| Name Color | — | `PlayerNameColors` | `ActiveNameColors` |
| Left Icon | `Icons` | `PlayerLeftIcons` | `ActiveLeftIcons` |
| Right Icon | `Icons` | `PlayerRightIcons` | `ActiveRightIcons` |

الجداول المساعدة:

- `GlobalChatColors`: ألوان رسائل الـglobal حسب الآيتم.
- `AutoCapeRegions`: المناطق التي تطبق Auto Cape.

## أسماء الـProcedures النهائية

```text
Title_Add             Title_Remove
Title_Activate        Title_Deactivate

Tag_Add               Tag_Remove
Tag_Activate          Tag_Deactivate

TitleColor_Add        TitleColor_Remove
TitleColor_Activate   TitleColor_Deactivate

NameColor_Add         NameColor_Remove
NameColor_Activate    NameColor_Deactivate

LeftIcon_Add          LeftIcon_Remove
LeftIcon_Activate     LeftIcon_Deactivate

RightIcon_Add         RightIcon_Remove
RightIcon_Activate    RightIcon_Deactivate

GuildNickname_Set     GuildNickname_Clear
ItemGlow_Change       ItemModel_Change
```

## أمثلة

إعطاء Title أصلي ثم تفعيله:

```sql
EXEC dbo.Title_Add @CharID = 12345, @TitleID = 1;
EXEC dbo.Title_Activate @CharID = 12345, @TitleID = 1;
```

إضافة Tag ثم تفعيله:

```sql
EXEC dbo.Tag_Add @CharID = 12345, @TagID = 3;
EXEC dbo.Tag_Activate
    @CharID = 12345,
    @CharName16 = 'PlayerName',
    @TagID = 3;
```

إعطاء أيقونة شمال ثم تفعيلها:

```sql
EXEC dbo.LeftIcon_Add @CharID = 12345, @IconID = 7;
EXEC dbo.LeftIcon_Activate
    @CharID = 12345,
    @CharName16 = 'PlayerName',
    @IconID = 7;
```

إخفاء الأيقونة بدون سحبها:

```sql
EXEC dbo.LeftIcon_Deactivate @CharName16 = 'PlayerName';
```

سحب الأيقونة من اللاعب:

```sql
EXEC dbo.LeftIcon_Remove @CharID = 12345, @IconID = 7;
```

تعيين Guild Nickname:

```sql
EXEC dbo.GuildNickname_Set @CharID = 12345, @Nickname = 'Trader';
```

## الترحيل من النظام القديم

شغّل `database\v1.0.0\20260727_player_style_system_rebuild.sql` والـFilter والـGameServer
متوقفان. الـmigration ينقل البيانات، يقسم ملكية أيقونات الشمال واليمين،
ثم يحذف الجداول والـviews والـsynonyms والـprocedures القديمة. العملية داخل
transaction واحدة؛ إذا فشل النقل أو بقي object قديم فلن يتم اعتماد التغيير.
