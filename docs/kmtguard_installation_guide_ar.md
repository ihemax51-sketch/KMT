# دليل تثبيت KMTGuard الموحد

يوجد إصدار واحد كامل فقط. ناتج البناء الرسمي موجود داخل `D:\KMTGuard-build` ويحتوي كل المكونات المطلوبة.

## محتويات التسليم

- `Filter`: برنامج الإدارة والـAgent/Gateway/Download hosts وملفات اللغة وتحديثات قاعدة البيانات.
- `DLL`: `KMTGuardKit.dll` و`WebViewerBridge.dll`.
- `Media`: مجلدات `clientlibrary` و`media` و`client-resources`.
- `ServerAddons\GameServer`: إضافة الـGameServer.
- `ServerAddons\ShardManager`: إضافة الـShardManager.
- `SHA256SUMS.txt`: بصمات جميع ملفات التسليم.

## البناء

لبناء كل المكونات:

```powershell
powershell -ExecutionPolicy Bypass -File build-scripts\Build-All.ps1
```

لبناء مكوّن واحد فقط:

```powershell
powershell -ExecutionPolicy Bypass -File build-scripts\Build-Component.ps1 -Component Filter
```

القيم المتاحة للمكوّن هي: `Filter` و`ClientDll` و`GameServer` و`ShardManager` و`Media`.

## التثبيت

1. خذ نسخة احتياطية من ملفات التشغيل الحالية وقواعد البيانات قبل الاستبدال.
2. انسخ محتويات `Filter` إلى مجلد تشغيل الفلتر، ثم راجع `Settings.json` وأبقه خارج أي مشاركة عامة لأنه يحتوي بيانات اتصال SQL.
3. ضع `KMTGuardKit.dll` و`WebViewerBridge.dll` في مسار تحميل DLL الخاص بالكلاينت.
4. ادمج محتويات `Media` مع ملفات الميديا مع الحفاظ على بنية المجلدات.
5. استبدل إضافتي الـGameServer والـShardManager من `ServerAddons` أثناء توقف الخدمات.
6. طبّق تحديثات SQL الموجودة تحت `Filter\database\vMAJOR.MINOR.PATCH` بالترتيب عند الحاجة فقط.
7. ابدأ ShardManager ثم GameServer ثم أدوار الفلتر، وافتح Admin Desktop لمراجعة الحالة.

## التحقق بعد التشغيل

- تأكد من بدء `KMTGuard.exe` وأدوار Agent/Gateway/Download بدون أخطاء إعداد أو SQL.
- تأكد من تحميل إضافتي GameServer وShardManager وظهور سجلات البدء الطبيعية.
- افتح كلاينت v188 المدعوم وتحقق من تحميل الواجهة والوسائط.
- قارن الملفات المنشورة مع `SHA256SUMS.txt` قبل نقلها إلى الخادم.
- نفّذ اختبار دخول بحساب عادي، انتقال مدينة، Teleport، Chat، والميزات المخصصة الأساسية.

لا يتطلب الإصدار الموحد أي ملف منفصل أو خدمة خارجية لبدء مكونات KMTGuard.
