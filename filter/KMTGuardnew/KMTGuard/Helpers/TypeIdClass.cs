using System.Collections.Generic;

public struct STypeID
{

    private ushort typeID;

    public STypeID(ushort value)
    {
        this.typeID = value;
    }

    public bool IsCashItem => (typeID & 1) != 0;
    public bool IsBionic => (typeID & 2) != 0;
    public byte First => (byte)((typeID >> 2) & 0x07);
    public byte Second => (byte)((typeID >> 5) & 0x03);
    public byte Third => (byte)((typeID >> 7) & 0x0F);
    public byte Fourth => (byte)((typeID >> 11) & 0x1F);

    public bool IsItem => First == 3;
    public bool IsEquipment => IsItem && Second == 1;
    public bool IsCOS => IsItem && Second == 2;
    public bool IsETC => IsItem && Second == 3;

    public bool IsPotion => IsETC && Third == 1;
    public bool IsPill => IsETC && Third == 2;
    public bool IsScroll => IsETC && Third == 3;
    public bool IsAmmo => IsETC && Third == 4;
    public bool IsMoney => IsETC && Third == 5;
    public bool IsFirework => IsETC && Third == 6;
    public bool IsCampfire => IsETC && Third == 7;
    public bool IsTradeGood => IsETC && Third == 8;
    public bool IsQuest_n_EventItem => IsETC && Third == 9;
    public bool IsElixir => IsETC && Third == 10;
    public bool IsAlchemyMaterial => IsETC && Third == 11;
    public bool IsEtcOther => IsETC && Third == 12;
    public bool IsEtcSpecial => IsETC && Third == 13;
    public bool IsMagicPop => IsETC && Third == 14;
    public bool IsMonsterGenerator => IsETC && Third == 15;

    public bool IsGlobalChatting => IsScroll && Fourth == 5;
    public bool IsReverseReturnScroll => IsScroll && Fourth == 3;
}
