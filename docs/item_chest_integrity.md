# Item Chest Integrity

شغّل ملف الترحيل التالي على قاعدة `KMTGuard` قبل تشغيل نسخة الـFilter
والـClient الجديدة:

```text
database/v1.0.0/20260727_item_chest_integrity_and_online_broadcast.sql
```

## البروسيدجرات الأساسية الإلزامية

إضافة آيتم للاعب واحد بالـItem ID:

```sql
EXEC KMTGuard.dbo.Item_AddChest
    @CharID = 12345,
    @ItemRefObjID = 3851,
    @Quantity = 10,
    @From = 'AdminReward',
    @Plus = 0;
```

إضافة آيتم للاعب واحد بالـCodeName:

```sql
EXEC KMTGuard.dbo.Item_AddChestByCodeName
    @CharID = 12345,
    @ItemCodeName = 'ITEM_MALL_GLOBAL_CHATTING',
    @Quantity = 10,
    @From = 'AdminReward',
    @Plus = 0;
```

## الإرسال لكل اللاعبين الأونلاين

```sql
EXEC KMTGuard.dbo.Item_ChestSendToOnline
    @ItemCodeName = 'ITEM_MALL_GLOBAL_CHATTING',
    @Quantity = 10,
    @From = 'OnlineEventReward',
    @Plus = 0;
```

البروسيدجر يلتقط اللاعبين الموجودين فعلًا داخل الجيم من جلسات الـAgent
الـgame-ready. كل عملية لها `BatchID`، ولذلك إعادة محاولة أمر الـqueue لا تضيف
الجائزة مرتين لنفس الشخصية.

## الإرسال لكل اللاعبين سواء أونلاين أو أوفلاين

```sql
EXEC KMTGuard.dbo.Item_ChestSendToAll
    @ItemCodeName = 'ITEM_MALL_GLOBAL_CHATTING',
    @Quantity = 10,
    @From = 'GlobalReward',
    @Plus = 0;
```

البروسيدجر يضيف الجائزة في Transaction واحدة لكل الشخصيات غير المحذوفة. اللاعب
الأونلاين يشاهدها مباشرة، والأوفلاين يجدها في الـItem Chest عند دخوله.

## مصالحة claim معلّق

اعرض الحالات المعلقة أولًا:

```sql
SELECT
    ClaimRow.*,
    ChestRow.ItemCodeName,
    ChestRow.Quantity
FROM KMTGuard.dbo.Item_ChestClaim AS ClaimRow
LEFT JOIN KMTGuard.dbo.Item_Chest AS ChestRow
    ON ChestRow.ID = ClaimRow.ChestID
WHERE ClaimRow.State = 0
ORDER BY ClaimRow.StartedAtUtc;
```

لا تستخدم `RETRY` إلا بعد التأكد أن الآيتم لم يصل إلى الـinventory:

```sql
EXEC KMTGuard.dbo.Item_ChestProcessClaim
    @Action = 'RETRY',
    @ChestID = 1001,
    @Reason = N'Confirmed that delivery did not occur.';
```

إذا تأكدت أن الآيتم وصل:

```sql
EXEC KMTGuard.dbo.Item_ChestProcessClaim
    @Action = 'DELIVERED',
    @ChestID = 1001,
    @Reason = N'Confirmed delivered from GameServer and inventory audit.';
```

كل مصالحة يدوية تُسجل في `Item_ChestClaimResolutionLog`.
