#pragma once

#include "AnimationCallback.h"
#include "CObjAnimation.h"
#include "IFStatic.h"
#include "PSOuterInterface.h"
#include <ghidra/undefined.h>

#define m_Characterselect (*reinterpret_cast<CharacterSelect *>(0xec2d6c))
class CharacterSelect {
    struct Vec3 {
    public:
        float X;
        float Y;
        float Z;
    };
    struct AnimationFrame {
    public:
        float time;    //0x0000
        Vec3 position; //0x0004
        Vec3 rotation; //0x0010
        float distance;//0x001C
    };                 //Size: 0x0020

    struct Character3DLocation {
    public:
        Vec3 Position; //0x0000
        float rotation;//0x000C
    };                 //Size: 0x0010

public:
    void *RTTI_INFO;             //0x0000
    char pad_0004[24];           //0x0004
    AnimationFrame KeyframeStart;//0x001C
    AnimationFrame KeyframeEnd;  //0x003C
    AnimationFrame N00000058;    //0x005C
    AnimationFrame N00000059;    //0x007C
    AnimationFrame N0000005A;    //0x009C
    AnimationFrame BoxFrame;     //0x00BC
    Character3DLocation Char3D1; //0x00DC
    Character3DLocation Char3D2; //0x00EC
    /* BEGIN_FIXTURE()
ENSURE_SIZE(0x0120)
ENSURE_OFFSET(KeyframeStart, 0x001C)
ENSURE_OFFSET(KeyframeEnd, 0x003C)
ENSURE_OFFSET(N00000058, 0x005C)
ENSURE_OFFSET(N00000059, 0x007C)
ENSURE_OFFSET(N0000005A, 0x009C)
ENSURE_OFFSET(N0000005B, 0x00BC)
ENSURE_OFFSET(Char3D1, 0x00DC)
ENSURE_OFFSET(Char3D2, 0x00EC)
END_FIXTURE()

RUN_FIXTURE(CharacterSelect)*/
};
class CPSCharacterSelect : public CPSOuterInterface, public CAnimationCallback {
    struct wtf
    {
    public:
        char pad_0000[432]; //0x0000
        wchar_t * CharNamePtr; //0x01B0
    };
    struct CPSCharacterSelect_SCharacterInfo
    {
    public:
        char pad_0000[4];     //0x0000
        void *CCompChar;      //0x0004
        char pad_0008[48];    //0x0008
        int CharheadID;       //0x0038
        int Charhead5;    //0x003C
        int Charhead13;   //0x0040
        int CharChestID;  //0x0044
        int CharChest5;   //0x0048
        int CharChest15;  //0x004C
        int CharSoulder;  //0x0050
        int CharSoulder5; //0x0054
        int CharSoulder14;//0x0058
        int CharHandsID;  //0x005C
        int CharHands5;   //0x0060
        int CharHands17;  //0x0064
        int CharLegsID;   //0x0068
        int CharLegs5;    //0x006C
        int CharLegs16;   //0x0070
        int CharFootID;   //0x0074
        int CharFoot5;    //0x0078
        int CharFoot18;   //0x007C
        int CharWeaponID; //0x0080
        int CharWeapon5;  //0x0084
        int CharWeapon39; //0x0088
        int CharShieldID; //0x008C
        int CharShield5;  //0x0090
        int CharShield53; //0x0094
        int CharSuitID;   //0x0098
        int CharSuitIDUnk;//0x009C
        int CharSuitID100;//0x00A0
        char pad_00A4[260];   //0x00A4
        int ObjID;        //0x01A8
        std::n_wstring CharName;   //0x01AC
        char pad_01C8[12]; //0x01C8
        int N000000BE; //0x01D4
        char pad_01D8[52]; //0x01D8

        int Unk7;         //0x020C
        int HeadID;       //0x0210
        int Unk5;         //0x0214
        char pad_0218[4];     //0x0218
        int ChestItemID;      //0x021C
        int unkchest5;    //0x0220
        char pad_0224[4];     //0x0224
        int ShoulderID;   //0x0228
        int unksohulder5; //0x022C
        char pad_0230[4];     //0x0230
        int HandsID;      //0x0234
        int unkHandsID;   //0x0238
        char pad_023C[4];     //0x023C
        int LegsID;       //0x0240
        int unkLegs5;     //0x0244
        char pad_0248[4];     //0x0248
        int footID;       //0x024C
        int unkfoot5;     //0x0250
        char pad_0254[4];     //0x0254
        int WeaponID;     //0x0258
        int UnkWep5;      //0x025C
        char pad_0260[4];     //0x0260
        int ShieldID;     //0x0264
        int unkShield5;   //0x0268
        char pad_026C[4];     //0x026C
        int SuitID;       //0x0270
        int SUitID5;      //0x0274
        char pad_0278[1504];  //0x0278

    };

    enum {
        GDR_STA_SCREENUP = 1,
        GDR_STA_SCREENDOWN = 2,
        GDR_STA_TITLE = 3,
        GDR_STA_REGIONTITLE = 4,
        GDR_STA_PLAYER1 = 6,
        GDR_STA_PLAYER2 = 7,
        GDR_STA_PLAYER3 = 8,
        GDR_STA_CHINA = 9,
        GDR_STA_ISLAM = 10,
        GDR_STA_EUROPE = 11,
        GDR_BTN_CREATE = 12,
        GDR_BTN_BACK = 13,
        GDR_BTN_START = 14,
        GDR_BTN_DELETE = 15,
        GDR_BTN_BACK2 = 16,
        GDR_BTN_CANCEL = 17,
        GDR_STA_CHARINFO = 18,
        GDR_SCREENFADE = 19,
        GDR_STA_WPOPUP = 20,
        GDR_FADE = 21,
        GDR_LOADING_CHINA = 22,
        GDR_LOADINGFRAME = 23,
        GDR_LOADINGG = 24,
        GDR_BTN_RESTORE = 25,
        GDR_STA_REMAINTIME = 26,
        GDR_LOADING_STA = 27,
        GDR_LOADING_EUROPE = 28,
        GDR_STA_NAME = 31,
        GDR_GAU_HP = 32,
        GDR_GAU_MP = 33,
        GDR_STA_EXP = 34,
        GDR_STA_SP = 35,
        GDR_STA_LEVEL = 36,
        GDR_STA_WNAME = 41,
        GDR_BTN_WACCEPT = 42,
        GDR_BTN_WCANCEL = 43,
        GDR_TB_INFO = 44,
        GDR_BTN_PREV = 45,
        GDR_BTN_NEXT = 46,
        GDR_STA_NAMECHANGE1 = 47,
        GDR_STA_NAMECHANGE2 = 48,
        GDR_STA_NAMECHANGE3 = 49,
        GDR_STA_NAMEPAGE = 50,
        GDR_STA_NAMECHANGE4 = 51,
        GDR_STA_WREMINTIME1 = 51,
        GDR_PML_WREMINTIME2 = 52,
        GDR_REMAING = 53,
        GDR_REMAINGBOX = 54,
        GDR_TB_NDTEXT = 94,
        GDR_STATIC1 = 101,
        GDR_STATIC2 = 102,
        GDR_STATIC3 = 103,
        GDR_TEXT_CHINA = 200,
        GDR_TEXT_EUROPE = 201,
        GDR_TEXT_ISLAM = 202,
        GDR_TEXT_MESSAGE = 500,
        GDR_TEXT_MESSAGE2 = 501,
        GDR_STA_UNITY_SERVER_NOTIFY = 900,
        GDR_PML_SUINFO = 901,
        GDR_BTN_SUACCEPT = 902,
        GDR_STA_SUTITLE = 903,
        GDR_VSC_SUINFO = 904,
        GDR_STA_UNITY_SERVER_CHANGENAME = 920,
        GDR_STA_CHANGENAME = 921,
        GDR_STA_BEFORENAME = 922,
        GDR_STA_AFTERNAME = 923,
        GDR_EDIT_BEFORENAME = 924,
        GDR_EDIT_AFTERNAME = 925,
        GDR_BTN_CNCHECK = 926,
        GDR_BTN_CNACCEPT = 927,
        GDR_BTN_CNCANCEL = 928,
        GDR_TEXT_CHANGENAME = 929,
        GDR_STA_NAME_DUPLICATE = 940,
        GDR_BTN_NDOK = 942,
        GDR_STA_NDSUBJECT = 943,
        GDR_TEXT = 10000,
        GDR_REGION = 1925,

    };

public:
    void aCPSCharacterSelect();
    /// \address 0085eb10
    bool OnCreateIMPL(long ln);

public:
    /// \address 0085b400
    void FUN_0085b400();

    /// \address 008560e0
    void FUN_008560e0();
    void CreatePIN();
    GFX_MSGMAP* MessageMap();
    /// \address 008548d0
    void FUN_008548d0(bool a1);

public: /* only public for hook */
    /// \address 0085b1f0
    void TriggerAnimation_Intro();
    int OnStart();
    bool OnServerPacketRecv(class CMsgStreamBuffer *MsgBuffer);
    wchar_t *Convert(std::n_string name);
    void TriggerCharacterAnimation(CPSCharacterSelect_SCharacterInfo *pInfo);
    void TriggerSelectedCharacterAnimation();
    void TriggerLoadedCharactersAnimation();
    void TryEnterPendingQuickLoginCharacter();

public:
    /// \address 00854860
    void FUN_00854860(CIFStatic *obj, unsigned char opacity, float time, float a4, char a5);
    int FUN_0085B680(BYTE a2);
    void FUN_008548b0();
    int FUN_0085ddb0(Event3D* p1);
    unsigned int OnDisappear(bool a1);
    CGWndBase *GetWindowByUniqueID(int Id);
    void FUN_00855400(int p1, float p2);
    void ClickIdol();
    int* un(int p1);
public:
    undefined field_0x110;
    undefined field_0x111;
    undefined field_0x112;
    undefined field_0x113;
    undefined field_0x114;
    undefined field_0x115;
    undefined field_0x116;
    undefined field_0x117;
    undefined field_0x118;
    undefined field_0x119;
    undefined field_0x11a;
    undefined field_0x11b;
    undefined1 field_0x11c;
    undefined field_0x11d;
    undefined field_0x11e;
    undefined field_0x11f;
    float m_seconds_elapsed;
    undefined4 m_minutes_elapsed;
    undefined4 m_last_update;
    undefined1 current_charcount;
    undefined field_0x12d;
    undefined field_0x12e;
    undefined1 m_btCurState;
    std::n_vector<CPSCharacterSelect_SCharacterInfo*> m_characters;


    undefined field_0x141;
    undefined field_0x142;
    undefined field_0x143;
    undefined field_0x144;
    undefined field_0x145;
    undefined field_0x146;
    undefined field_0x147;
    undefined field_0x148;
    undefined field_0x149;
    undefined field_0x14a;
    undefined field_0x14b;
    undefined field_0x14c;
    undefined field_0x14d;
    undefined field_0x14e;
    undefined field_0x14f;
    undefined field_0x150;
    undefined field_0x151;
    undefined field_0x152;
    undefined field_0x153;
    undefined field_0x154;
    undefined field_0x155;
    undefined field_0x156;
    undefined field_0x157;
    undefined field_0x158;
    undefined field_0x159;
    undefined field_0x15a;
    undefined field_0x15b;
    undefined field_0x15c;
    undefined field_0x15d;
    undefined field_0x15e;
    undefined field_0x15f;
    CCObjAnimation idols[3];
    CCObjAnimation m_box;
    undefined4 field_0x220;
    undefined field_0x224;
    undefined field_0x225;
    undefined field_0x226;
    undefined field_0x227;
    CIFStatic *idol_label[2];

private:
BEGIN_FIXTURE()
        ENSURE_SIZE(0x230)
        ENSURE_OFFSET(m_seconds_elapsed, 0x120)
        ENSURE_OFFSET(idols, 0x160)
        ENSURE_OFFSET(m_box, 0x1f0)
        ENSURE_OFFSET(idol_label, 0x228)
    END_FIXTURE()

    RUN_FIXTURE(CPSCharacterSelect)




};
