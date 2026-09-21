# شرح تعديل تصميم المنيو من JSON

هذا النظام مخصص لتعديل منيو `CIFMenu` فقط.

لا يقوم بتغيير:

- أيقونات الأسلحة والملابس والأغراض.
- أيقونات المهارات.
- واجهات اللعبة الأصلية.
- صور النوافذ الأخرى.
- صور الـFilter أو الـClient Library خارج المنيو.

إذا لم يوجد ملف التصميم، أو كان ملف JSON غير صالح، ستعمل المنيو بالتصميم
الأصلي الموجود في:

```text
clientlibrary\resinfo\ifmenu.txt
```

---

## 1. مكان ملف التصميم

الملف الموجود داخل المشروع:

```text
JTClientLibrary\media\clientlibrary\config\menu_design.json
```

بعد الانتهاء من التعديل يجب إدخاله إلى `Media.pk2` في المسار التالي:

```text
clientlibrary\config\menu_design.json
```

بعد تحديث `Media.pk2` أعد تشغيل الكلاينت فقط.

لا تحتاج إلى إعادة تشغيل:

- Filter.
- GameServer.
- ShardManager.

---

## 2. الشكل الأساسي للملف

```json
{
  "version": 1,
  "enabled": true,
  "menu": {
    "width": 360,
    "height": 430,
    "text": "Menu"
  },
  "elements": [
    {
      "id": 12,
      "name": "grant_name",
      "x": 30,
      "y": 170,
      "width": 145,
      "height": 28,
      "texture": "clientlibrary\\menu\\grant_name.ddj",
      "pressed_texture": "clientlibrary\\menu\\grant_name_pressed.ddj",
      "disabled_texture": "clientlibrary\\menu\\grant_name_disabled.ddj",
      "text": "Grant Name",
      "tooltip": "Change your grant name",
      "font_color": "#FFFBE0",
      "visible": true
    }
  ]
}
```

---

## 3. إعدادات المنيو الرئيسية

الإعدادات الموجودة داخل `menu` تتحكم في نافذة المنيو نفسها:

```json
"menu": {
  "x": 500,
  "y": 100,
  "width": 360,
  "height": 430,
  "text": "Server Menu",
  "texture": "clientlibrary\\menu\\menu_background.ddj"
}
```

| الخاصية | الوظيفة |
|---|---|
| `x` | مكان المنيو أفقيًا على الشاشة |
| `y` | مكان المنيو رأسيًا على الشاشة |
| `width` | عرض المنيو بالبكسل |
| `height` | ارتفاع المنيو بالبكسل |
| `text` | عنوان المنيو |
| `texture` | صورة أو Frame خلفية المنيو |

خصائص `x` و`y` اختيارية. إذا لم تكتبها، ستظهر المنيو تلقائيًا في مكانها
الافتراضي ناحية يمين الشاشة.

---

## 4. نظام عناصر المنيو

كل عنصر يوضع داخل مصفوفة `elements`:

```json
"elements": [
  {
    "id": 12,
    "x": 30,
    "y": 170
  },
  {
    "id": 13,
    "x": 185,
    "y": 170
  }
]
```

النظام يتعرف على العنصر من خلال `id`. قيمة `name` للتوضيح فقط، ويمكن
تغييرها أو حذفها بدون التأثير على عمل العنصر.

أماكن العناصر `x` و`y` محسوبة داخل المنيو، وليست من أول الشاشة.

مثال:

```json
{
  "id": 12,
  "x": 20,
  "y": 50
}
```

يعني وضع الزر على بعد 20 بكسل من يسار المنيو و50 بكسل من أعلى المنيو.

---

## 5. خصائص كل عنصر

| الخاصية | النوع | الوظيفة |
|---|---|---|
| `id` | رقم | ID الخاص بالعنصر، وهو إجباري |
| `name` | نص | اسم توضيحي داخل JSON فقط |
| `x` | رقم | مكان العنصر أفقيًا داخل المنيو |
| `y` | رقم | مكان العنصر رأسيًا داخل المنيو |
| `width` | رقم | عرض العنصر |
| `height` | رقم | ارتفاع العنصر |
| `texture` | نص | الصورة العادية للعنصر |
| `pressed_texture` | نص | صورة الزر أثناء الضغط |
| `disabled_texture` | نص | صورة الزر عندما يكون معطلًا |
| `text` | نص | النص الظاهر على العنصر |
| `tooltip` | نص | النص الذي يظهر عند وضع الماوس |
| `font_color` | نص | لون الكتابة |
| `visible` | true/false | إظهار أو إخفاء العنصر |

كل الخصائص اختيارية باستثناء `id`.

إذا أردت تعديل مكان العنصر فقط:

```json
{
  "id": 12,
  "x": 25,
  "y": 150
}
```

إذا أردت تعديل الصورة فقط:

```json
{
  "id": 12,
  "texture": "clientlibrary\\menu\\grant_name.ddj"
}
```

---

## 6. IDs عناصر المنيو

| ID | العنصر |
|---:|---|
| 4 | صورة وجه الشخصية |
| 6 | اسم الشخصية |
| 7 | أيقونة العِرق China/Europe |
| 9 | اسم الـGuild |
| 12 | Grant Name |
| 13 | Title Manager |
| 14 | Icon Manager |
| 15 | Dynamic Ranking |
| 16 | Unique History |
| 17 | Event Register |
| 18 | Event Schedule |
| 19 | Achievements |
| 20 | Changelog |
| 21 | Alchemy Macro |
| 22 | Settings |
| 24 | Discord |
| 25 | Website |

لا تغير قيمة `id` الخاصة بزر إلى قيمة زر آخر، لأن وظيفة الضغط مرتبطة بالـID.

يمكنك تغيير التصميم والمكان والنص، لكن يجب الاحتفاظ بالـID حتى تظل وظيفة
الزر تعمل بصورة صحيحة.

---

## 7. تغيير صور الأزرار

يمكن استخدام ثلاث صور لكل زر:

```json
{
  "id": 12,
  "texture": "clientlibrary\\menu\\grant_name.ddj",
  "pressed_texture": "clientlibrary\\menu\\grant_name_pressed.ddj",
  "disabled_texture": "clientlibrary\\menu\\grant_name_disabled.ddj"
}
```

- `texture`: الحالة العادية.
- `pressed_texture`: أثناء الضغط على الزر.
- `disabled_texture`: عندما تكون الميزة غير مفعلة.

يجب إضافة ملفات الصور إلى `Media.pk2` بنفس المسارات المكتوبة في JSON.

مثال:

```text
clientlibrary\menu\grant_name.ddj
clientlibrary\menu\grant_name_pressed.ddj
clientlibrary\menu\grant_name_disabled.ddj
```

استخدم `\\` داخل JSON بدل `\`:

```json
"texture": "clientlibrary\\menu\\button.ddj"
```

---

## 8. تغيير لون الخط

يمكن كتابة اللون بصيغة RGB:

```json
"font_color": "#FFFBE0"
```

أمثلة:

```text
#FFFFFF = أبيض
#000000 = أسود
#FF0000 = أحمر
#00FF00 = أخضر
#0000FF = أزرق
#FFD700 = ذهبي
```

ويمكن استخدام لون ARGB مكون من 8 خانات:

```json
"font_color": "#FFFFD700"
```

أول خانتين تمثلان الشفافية.

---

## 9. إخفاء عنصر

```json
{
  "id": 20,
  "visible": false
}
```

لإظهاره مرة أخرى:

```json
{
  "id": 20,
  "visible": true
}
```

إخفاء الزر لا يغير إعداد تشغيل الميزة في الفلتر؛ هو يخفي العنصر من المنيو فقط.

---

## 10. مثال على ترتيب الأزرار في عمودين

```json
{
  "version": 1,
  "enabled": true,
  "menu": {
    "width": 360,
    "height": 430,
    "text": "KMTGuard Menu"
  },
  "elements": [
    {
      "id": 12,
      "x": 25,
      "y": 150,
      "width": 145,
      "height": 28,
      "text": "Grant Name"
    },
    {
      "id": 13,
      "x": 185,
      "y": 150,
      "width": 145,
      "height": 28,
      "text": "Title Manager"
    },
    {
      "id": 14,
      "x": 25,
      "y": 185,
      "width": 145,
      "height": 28,
      "text": "Icon Manager"
    },
    {
      "id": 15,
      "x": 185,
      "y": 185,
      "width": 145,
      "height": 28,
      "text": "Ranking"
    }
  ]
}
```

---

## 11. تعطيل التصميم المخصص

لتشغيل التصميم الأصلي بدون حذف الملف:

```json
"enabled": false
```

لتشغيل تصميم JSON:

```json
"enabled": true
```

---

## 12. خطوات إدخال التصميم إلى Media.pk2

1. احتفظ بنسخة احتياطية من `Media.pk2`.
2. عدّل `menu_design.json` باستخدام VS Code أو Notepad++.
3. تأكد أن الملف صالح بصيغة JSON.
4. أضف ملفات `.ddj` الجديدة إلى المسارات المكتوبة في الملف.
5. أضف `menu_design.json` داخل:

   ```text
   clientlibrary\config\
   ```

6. احفظ تحديثات `Media.pk2`.
7. أغلق الكلاينت وافتحه من جديد.
8. افتح المنيو واختبر كل زر.

---

## 13. أخطاء شائعة

### المنيو تعمل بالتصميم القديم

تحقق من:

- أن اسم الملف هو `menu_design.json`.
- أن مكانه داخل `Media.pk2` صحيح.
- أن `"enabled": true`.
- عدم وجود فاصلة زائدة في JSON.
- استخدام علامتي `\\` في مسارات الصور.

### الصورة لا تظهر

تحقق من:

- وجود ملف `.ddj` داخل `Media.pk2`.
- تطابق اسم الملف وحالة الحروف.
- عدم استخدام مسار من الهارد مثل `C:\Images\button.ddj`.
- أن المسار لا يحتوي على `..`.

### الزر يظهر ولكن لا يعمل

تحقق من قيمة `id`. وظيفة الزر مرتبطة بالـID الأصلي.

### العنصر يظهر في مكان خاطئ

قيم `x` و`y` الخاصة بالعناصر محسوبة بالنسبة إلى مكان المنيو، وليست بالنسبة
إلى الشاشة.

### المنيو لا تظهر أو حجمها غير صحيح

- لا تستخدم عرضًا أو ارتفاعًا يساوي صفرًا.
- ابدأ بالمقاسات الأصلية ثم عدّل تدريجيًا.
- تأكد أن العناصر موجودة داخل حدود عرض وارتفاع المنيو.

---

## 14. قواعد مهمة

- احتفظ دائمًا بنسخة احتياطية من JSON و`Media.pk2`.
- عدّل عنصرًا أو عنصرين في كل تجربة.
- لا تغير IDs الأزرار.
- اجعل أبعاد ملفات DDJ مناسبة لأبعاد العناصر.
- لا تستخدم مسارات ملفات Windows داخل JSON.
- تغيير الملف يحتاج إعادة تشغيل الكلاينت فقط.
